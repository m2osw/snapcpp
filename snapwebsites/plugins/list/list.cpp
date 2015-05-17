// Snap Websites Server -- advanced handling of lists
// Copyright (C) 2014-2015  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "list.h"

#include "../links/links.h"
#include "../path/path.h"
#include "../output/output.h"

#include "not_reached.h"
#include "snap_expr.h"
#include "qdomhelpers.h"
#include "dbutils.h"
#include "log.h"
#include "snap_backend.h"

#include <iostream>

#include <sys/time.h>

#include "poison.h"


SNAP_PLUGIN_START(list, 1, 0)

/** \brief Get a fixed list name.
 *
 * The list plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_LIST_ITEM_KEY_SCRIPT: // compiled
        return "list::item_key_script";

    case SNAP_NAME_LIST_KEY: // list of ordered pages
        return "list::key"; // + "::<list uri>" (cell includes <item sort key>)

    case SNAP_NAME_LIST_LAST_UPDATED:
        return "list::last_updated";

    case SNAP_NAME_LIST_LINK: // standard link between list and list items
        return "list::link";

    case SNAP_NAME_LIST_NAME: // name for query string
        return "list::name";

    case SNAP_NAME_LIST_NAMESPACE:
        return "list";

    case SNAP_NAME_LIST_NUMBER_OF_ITEMS:
        return "list::number_of_items";

    case SNAP_NAME_LIST_ORDERED_PAGES: // list of ordered pages
        return "list::ordered_pages"; // + "::<item sort key>"

    case SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT: // text format
        return "list::original_item_key_script";

    case SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT: // text format
        return "list::original_test_script";

    case SNAP_NAME_LIST_PAGE: // query string name "...?page=..."
        return "page";

    case SNAP_NAME_LIST_PAGELIST: // --action pagelist
        return "pagelist";

    case SNAP_NAME_LIST_PAGE_SIZE:
        return "list::page_size";

    case SNAP_NAME_LIST_PROCESSLIST: // --action processlist
        return "processlist";

    case SNAP_NAME_LIST_RESETLISTS:
        return "resetlist";

    case SNAP_NAME_LIST_SELECTOR: // all, public, children, hand-picked, type=name, ...
        return "list::selector";

    case SNAP_NAME_LIST_SIGNAL_NAME:
        return "pagelist_udp_signal";

    case SNAP_NAME_LIST_STANDALONE: // when present in list table as a column name of a site row: signals a website managed as a standalone site
        return "*standalone*";

    case SNAP_NAME_LIST_STANDALONELIST: // --action standalonelist
        return "standalonelist";

    case SNAP_NAME_LIST_TABLE:
        return "list";

    case SNAP_NAME_LIST_TABLE_REF:
        return "listref";

    case SNAP_NAME_LIST_TAXONOMY_PATH:
        return "types/taxonomy/system/list";

    case SNAP_NAME_LIST_THEME: // filter function
        return "list::theme";

    case SNAP_NAME_LIST_TEST_SCRIPT: // compiled
        return "list::test_script";

    case SNAP_NAME_LIST_TYPE:
        return "list::type";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_LIST_...");

    }
    NOTREACHED();
}




/** \class list
 * \brief The list plugin to handle list of pages.
 *
 * The list plugin makes use of many references and links and thus it
 * is documented here:
 *
 *
 * 1) Pages that represent lists are all categorized under the following
 *    system content type:
 *
 * \code
 *     /types/taxonomy/system/list
 * \endcode
 *
 * We use that list to find all the lists defined on a website so we can
 * manage them all in our loops.
 *
 *
 *
 * 2) Items are linked to their list so that way when deleting an item
 *    we can immediately remove that item from that list. Note that an
 *    item may be part of many lists so it is a "multi" on both sides
 *    ("*:*").
 *
 *
 * 3) The list page includes links to all the items that are part of
 *    the list. These links do not use the standard link capability
 *    because the items are expected to be ordered and that is done
 *    using the Cassandra sort capability, in other words, we need
 *    to have a key which includes the sort parameters (i.e. an index).
 *
 * \code
 *    list::items::<sort key>
 * \endcode
 *
 * Important Note: This special link is double linked too, that is, the
 * item page links back to the standard list too (more precisly, it knows
 * of the special ordered key used in the list.) This is important to
 * make sure we can manage lists properly. That is, if the expression
 * used to calculate the key changes, then we could not instantly find
 * the old key anymore (i.e. we'd have to check each item in the list
 * to find the one that points to a given item... in a list with 1 million
 * pages, it would be really slow.)
 *
 *
 * Recap:
 *
 * \li Standard Link: List Page \<-\> /types/taxonomy/system/list
 * \li Standard Link: List Page \<-\> Item Page
 * \li Ordered List: List Page -\> Item Page,
 *                   Item Page includes key used in List Page
 *
 * \note
 * We do not repair list links when a page is cloned. If the clone is
 * to be part of a list the links will be updated accordingly.
 */





/** \brief Initializes an object to access a list with paging capability.
 *
 * This function initializes this paging object with defaults.
 *
 * The \p ipath parameter is the page that represent a Snap list. It
 * will be read later when you call the read_list() function.
 *
 * \param[in,out] snap  Pointer to the snap_child object.
 * \param[in,out] ipath  The path to the page representing a list.
 *
 * \sa get_query_string_info()
 */
paging_t::paging_t(snap_child *snap, content::path_info_t & ipath)
    : f_snap(snap)
    , f_ipath(ipath)
    //, f_retrieved_list_name(false) -- auto-init
    //, f_list_name("") -- auto-init
    //, f_number_of_items(-1) -- auto-init
    //, f_start_offset(-1) -- auto-init
    //, f_page(1) -- auto-init
    //, f_page_size(-1) -- auto-init
{
}


/** \brief Read the current page of this list.
 *
 * This function calls the list read_list() function with the parameters
 * as defined in this paging object.
 *
 * \return The list of items as read using the list plugin.
 *
 * \sa get_start_offset()
 * \sa get_page_size()
 */
list_item_vector_t paging_t::read_list()
{
std::cerr << "read list built with " << get_start_offset() << " for " << get_page_size() << "\n";
    return list::list::instance()->read_list(f_ipath, get_start_offset() - 1, get_page_size());
}


/** \brief Retrieve the name of the list.
 *
 * This function returns the name of this paging object. This is the
 * name used to retrieve the current information about the list position
 * from the query string.
 *
 * The name is retrieved from the database using the referenced page.
 * It is valid to not define a name. Without a name, the simple "page"
 * query string variable is used. A name is important if the page is
 * to appear in another which also represents a list.
 *
 * \note
 * The name is cached so calling this function more than once is fast.
 *
 * \return The name of the list.
 */
QString paging_t::get_list_name() const
{
    if(!f_retrieved_list_name)
    {
        f_retrieved_list_name = true;

        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
        f_list_name = branch_table->row(f_ipath.get_branch_key())->cell(get_name(SNAP_NAME_LIST_NAME))->value().stringValue();
    }
    return f_list_name;
}


/** \brief Retrieve the total number of items in a list.
 *
 * This function retrieves the total number of items found in a list.
 * This value is defined in the database under the name
 * SNAP_NAME_LIST_NUMBER_OF_ITEMS.
 *
 * \note
 * This function always returns a positive number or zero.
 *
 * \note
 * The number is cached so this function can be called any number of
 * times.
 *
 * \warning
 * This is not the number of pages. Use the get_total_pages() to
 * determine the total number of pages available in a list.
 *
 * \todo
 * The code necessary to count the items in a list is not yet written.
 *
 * \return The number of items in the list.
 */
int32_t paging_t::get_number_of_items() const
{
    if(f_number_of_items < 0)
    {
        // if the number of items is not (yet) defined in the database
        // then it will be set to zero
        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
        f_number_of_items = branch_table->row(f_ipath.get_branch_key())->cell(get_name(SNAP_NAME_LIST_NUMBER_OF_ITEMS))->value().safeInt32Value();
    }

    return f_number_of_items;
}


/** \brief Define the start offset to use with read_list().
 *
 * This function is used to define the start offset. By default this
 * value is set to -1 meaning that the start page parameter is used
 * instead. This is useful in case you want to show items at any
 * offset instead of an exact page multiple.
 *
 * You make set the parameter back to -1 to ignore it.
 *
 * If the offset is larger than the total number of items present in
 * the list, the read_list() will return an empty list. You may test
 * the limit using the get_number_of_items() function. This function
 * does not prevent you from using an offsets larger than the
 * number of available items.
 *
 * \warning
 * The first item offset is 1, not 0 as generally expected in C/C++.
 *
 * \param[in] start_offset  The offset at which to start showing the list.
 */
void paging_t::set_start_offset(int32_t start_offset)
{
    // any invalid number, convert to -1 (ignore)
    if(start_offset < 1)
    {
        f_start_offset = -1;
    }
    else
    {
        f_start_offset = start_offset;
    }
}


/** \brief Retrieve the start offset.
 *
 * This function returns the start offset. This represents the number
 * of the first item to return to the caller of the read_list() function.
 * The offset may point to an item after the last item in which case the
 * read_list() function will return an empty list of items.
 *
 * If the start offset is not defined (is -1) then this function calculates
 * the start offset using the start page information:
 *
 * \code
 *      return (f_page - 1) * get_page_size() + 1;
 * \endcode
 *
 * Note that since f_page can be set to a number larger than the maximum
 * number of pages, the offset returned in that situation may also be
 * larger than the total number of items present in the list.
 *
 * \note
 * The function returns one for the first item (and NOT zero as generally
 * expected in C/C++).
 *
 * \warning
 * There is no way to retrieve the f_start_offset value directly.
 *
 * \return The start offset.
 */
int32_t paging_t::get_start_offset() const
{
    if(f_start_offset < 1)
    {
        // the caller did not force the offset, first check the query
        // string to see whether the end user defined a page parameter

        // calculate the offset using the start page
        // and page size instead, adjusting for the
        // fact that page numbers start at one instead
        // of zero
        return (f_page - 1) * get_page_size() + 1;
    }

    return f_start_offset;
}


/** \brief Retrieve the query string page information.
 *
 * This function reads the query string page information and saves
 * it in this paging object.
 *
 * The query string name is defined as:
 *
 * \code
 *      page
 *   or
 *      page-<list_name>
 * \endcode
 *
 * If the list name is empty or undefined, then the name of the query
 * string variable is simply "page". If the name is defined, then the
 * system adds a dash and the name of the list.
 *
 * The value of the query string is generally just the page number.
 * The number is expected to be between 1 and the total number of
 * pages available in this list. The number 1 is not required as it
 * is the default.
 *
 * Multiple numbers can be specified by separating them with commas
 * and preceeding them with a letter as follow:
 *
 * \li 'p' -- page number, the 'p' is always optional
 * \li 'o' -- start offset, an item number, ignores the page number
 * \li 's' -- page size, the number of items per page
 *
 * For example, to show page 3 of a list named blog with 300 items,
 * showing 50 items per page, you can use:
 *
 * \code
 *      page-blog=3,s50
 *   or
 *      page-blog=p3,s50
 * \endcode
 *
 * \sa get_page()
 * \sa get_page_size()
 * \sa get_start_offset()
 */
void paging_t::process_query_string_info()
{
    // define the query string variable name
    QString const list_name(get_list_name());
    QString variable_name(get_name(SNAP_NAME_LIST_PAGE));
    if(!list_name.isEmpty())
    {
        variable_name += "-";
        variable_name += list_name;
    }

    // check whether such a variable exists in the query string
    if(!f_snap->get_uri().has_query_option(variable_name))
    {
        return;
    }

    // got such, retrieve it
    QString const variable(f_snap->get_uri().query_option(variable_name));
    QStringList const params(variable.split(","));
    bool defined_page(false);
    bool defined_size(false);
    bool defined_offset(false);
    for(int idx(0); idx < params.size(); ++idx)
    {
        QString const p(params[idx]);
        if(p.isEmpty())
        {
            continue;
        }
        switch(p[0].unicode())
        {
        case 'p':   // explicit page number
            if(!defined_page)
            {
                defined_page = true;
                bool ok(false);
                int page = p.mid(1).toInt(&ok);
                if(ok && page > 0)
                {
                    f_page = page;
                }
            }
            break;

        case 's':   // page size (number of items per page)
            if(!defined_size)
            {
                defined_size = true;
                bool ok(false);
                int const size = p.mid(1).toInt(&ok);
                if(ok && size > 0 && size <= list::LIST_MAXIMUM_ITEMS)
                {
                    f_page_size = size;
                }
            }
            break;

        case 'o':   // start offset (specific number of items)
            if(!defined_offset)
            {
                defined_offset = true;
                bool ok(false);
                int offset = p.mid(1).toInt(&ok);
                if(ok && offset > 0)
                {
                    f_start_offset = offset;
                }
            }
            break;

        case '0': // the page number
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if(!defined_page)
            {
                defined_page = true;
                bool ok(false);
                int page = p.toInt(&ok);
                if(ok && page > 0)
                {
                    f_page = page;
                }
            }
            break;

        }
    }
}


/** \brief Generate the query string representing this paging information.
 *
 * This function is used to generate a link to a page as defined by this
 * paging information.
 *
 * The \p page_offset parameter is expected to be zero (0) for a link
 * to the current page. It is expected to be negative to go to a previous
 * page and positive to go to a following page.
 *
 * \param[in] page_offset  The offset to the page to generate a query string for.
 *
 * \return The query string variable and value for the specified page.
 */
QString paging_t::generate_query_string_info(int32_t page_offset) const
{
    QString result(get_name(SNAP_NAME_LIST_PAGE));
    QString const list_name(get_list_name());
    if(!list_name.isEmpty())
    {
        result += "-";
        result += list_name;
    }
    result += "=";

    int32_t const page_size(get_page_size());

    bool need_comma(false);
    if(f_start_offset > 0)
    {
        // keep using the offset if defined
        int32_t offset(f_start_offset + page_offset * page_size);
        if(offset <= 0)
        {
            offset = 1;
        }
        else if(offset > get_number_of_items())
        {
            offset = f_number_of_items;
        }
        result += QString("o%1").arg(offset);
        need_comma = true;
    }
    else
    {
        int32_t page(f_page + page_offset);
        int32_t const max_pages(get_total_pages());
        if(page > max_pages && max_pages != -1)
        {
            // maximum limit
            page = max_pages;
        }
        if(page < 1)
        {
            // minimum limit
            page = 1;
        }

        if(page != f_page)
        {
            // use the page only if no offset specified
            // also we do not need to specify page=1 since that is the default
            result += QString("%1").arg(page);
            need_comma = true;
        }
std::cerr << "**** page_offset " << page_offset << " p+o = " << (f_page + page_offset) << " page = " << page << " / " << f_page << ", max pages = " << max_pages << " -- result = [" << result << "]\n";
    }

    if(page_size != f_default_page_size)
    {
        if(need_comma)
        {
            result += "%2C";
        }
        result += QString("s%1").arg(page_size);
        need_comma = true;
    }

std::cerr << ">>> " << (need_comma ? "Need Comma TRUE" : "no comma?!") << " -> " << result << "\n";
    if(!need_comma)
    {
        // page 1 with default size, add nothing to the query string
        return QString();
    }

    return result;
}


/** \brief Generate the query string to access the first page.
 *
 * This function calculates the query string to send the user to the
 * first page of this list. The first page is often represented by an
 * empty query string so this function may return such when the offset
 * was not specified and no specific page size was defined.
 *
 * \return The query string to send the user to the last page.
 */
QString paging_t::generate_query_string_info_for_first_page() const
{
    if(f_start_offset > 0)
    {
        int32_t const page_size(get_page_size());
        return generate_query_string_info((1 - f_start_offset + page_size - 1) / page_size);
    }

    return generate_query_string_info(1 - f_page);
}


/** \brief Generate the query string to access the last page.
 *
 * This function calculates the query string to send the user to the
 * last page of this list. The last page may be the first page in
 * which case the function may return an empty string.
 *
 * \return The query string to send the user to the last page.
 */
QString paging_t::generate_query_string_info_for_last_page() const
{
    int32_t const max_pages(get_total_pages());
    if(max_pages == -1)
    {
        // this also represents the very first page with the default
        // page size... but without a valid max_pages, what can we do
        // really?
        return QString();
    }

    if(f_start_offset > 0)
    {
        int32_t const page_size(get_page_size());
        return generate_query_string_info((get_number_of_items() - f_start_offset + page_size - 1) / page_size);
    }

    return generate_query_string_info(max_pages - f_page);
}


/** \brief Generate a set of anchors for navigation purposes.
 *
 * This function generates the navigation anchors used to let the
 * end user move between pages quickly.
 *
 * \todo
 * The next / previous anchors make use of characters that the end
 * user should be able to change (since we have access to the list
 * we can define them in the database.)
 *
 * \param[in,out] element  A QDomElement object where we add the navigation
 *                         elements.
 * \param[in] uri  The URI used to generate the next/previous, pages 1, 2, 3...
 * \param[in] next_previous_count  The number of anchors before and after
 *                                 the current page.
 * \param[in] next_previous  Whether to add a next and previous set of anchors.
 * \param[in] first_last  Whether to add a first and last set of anchors.
 */
void paging_t::generate_list_navigation(QDomElement element, snap_uri uri, int32_t next_previous_count, bool const next_previous, bool const first_last) const
{
    if(element.isNull())
    {
        return;
    }

    QString const qs_path(f_snap->get_server_parameter("qs_path"));
    uri.unset_query_option(qs_path);

    QDomDocument doc(element.ownerDocument());
    QDomElement ul(doc.createElement("ul"));

    // add a root tag to encompass all the other tags
    QString list_name(get_list_name());
    if(!list_name.isEmpty())
    {
        list_name = " " + list_name;
    }
    ul.setAttribute("class", "list-navigation" + list_name);
    element.appendChild(ul);

std::cerr << "----------------------- LIST ----------------\n";
    // generate the URIs in before/after the current page
    int32_t first(0);
    int32_t last(0);
    int32_t current_index(0);
    QStringList qs;
    QString const current_page_query_string(generate_query_string_info(0));
    qs.push_back(current_page_query_string);
std::cerr << "+++++++ C [" << current_page_query_string << "]\n";
std::cerr << "/previous\n";
    for(int32_t i(-1); i >= -next_previous_count; --i)
    {
        QString const query_string(generate_query_string_info(i));
std::cerr << "+++++++ P [" << query_string << "] (qs first: [" << qs.first() << "] )\n";
        if(qs.first() == query_string)
        {
            break;
        }
        if(i < first)
        {
            first = i;
        }
std::cerr << "----------------------- [" << query_string << "]\n";
        qs.push_front(query_string);
    }
std::cerr << "/next\n";
    current_index = qs.size() - 1;
    for(int32_t i(1); i <= next_previous_count; ++i)
    {
        QString const query_string(generate_query_string_info(i));
std::cerr << "+++++++ N [" << query_string << "] (qs last: [" << qs.last() << "] )\n";
        if(qs.last() == query_string)
        {
            break;
        }
        if(i > last)
        {
            last = i;
        }
std::cerr << "----------------------- [" << query_string << "]\n";
        qs.push_back(query_string);
    }
std::cerr << "----------------------- FIRST " << first << " /LAST " << last << " -- index " << current_index << " ----------------\n";

    // add the first anchor only if we are not on the first page
    if(first_last && first < 0)
    {
        // add the first button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "list-navigation-first");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info_for_first_page());
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x21E4))));
        anchor.appendChild(text);
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }

    // add the previous anchor only if we are not on the first page
    if(next_previous && first < 0)
    {
        // add the previous button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "list-navigation-previous");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info(-1));
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x2190))));
        anchor.appendChild(text);
        //anchor.setAttribute("href", anchor_uri.get_uri());
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }

    // add the navigation links now
    int32_t const max_qs(qs.size());
    for(int32_t i(0); i < max_qs; ++i)
    {
        QString query_string(qs[i]);
        if(i == current_index)
        {
            // the current page (not an anchor)
std::cerr << "/current/ " << i << "\n";
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "list-navigation-current");
            ul.appendChild(li);
            QDomText text(doc.createTextNode(QString("%1").arg(f_page)));
            li.appendChild(text);
        }
        else if(i < current_index)
        {
            // a previous anchor
std::cerr << "/previous/ " << i << "\n";
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "list-navigation-preceeding-page");
            ul.appendChild(li);

            snap_uri anchor_uri(uri);
            anchor_uri.set_query_string(query_string);
            QDomElement anchor(doc.createElement("a"));
            QDomText text(doc.createTextNode(QString("%1").arg(f_page + i - current_index)));
            anchor.appendChild(text);
            //anchor.setAttribute("href", anchor_uri.get_uri());
            anchor.setAttribute("href", "?" + anchor_uri.query_string());
            li.appendChild(anchor);
        }
        else
        {
            // a next anchor
            QDomElement li(doc.createElement("li"));
            li.setAttribute("class", "list-navigation-following-page");
            ul.appendChild(li);

            snap_uri anchor_uri(uri);
            anchor_uri.set_query_string(query_string);
            QDomElement anchor(doc.createElement("a"));
            QDomText text(doc.createTextNode(QString("%1").arg(f_page + i - current_index)));
            anchor.appendChild(text);
            //anchor.setAttribute("href", anchor_uri.get_uri());
            anchor.setAttribute("href", "?" + anchor_uri.query_string());
            li.appendChild(anchor);
        }
    }

    // add the previous anchor only if we are not on the first page
    if(next_previous && last > 0)
    {
        // add the previous button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "list-navigation-next");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info(1));
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x2192))));
        anchor.appendChild(text);
        //anchor.setAttribute("href", anchor_uri.get_uri());
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }

    // add the last anchor only if we are not on the last page
    if(first_last && last > 0)
    {
        // add the last button
        QDomElement li(doc.createElement("li"));
        li.setAttribute("class", "list-navigation-last");
        ul.appendChild(li);

        snap_uri anchor_uri(uri);
        anchor_uri.set_query_string(generate_query_string_info_for_last_page());
        QDomElement anchor(doc.createElement("a"));
        QDomText text(doc.createTextNode(QString("%1").arg(QChar(0x21E5))));
        anchor.appendChild(text);
        //anchor.setAttribute("href", anchor_uri.get_uri());
        anchor.setAttribute("href", "?" + anchor_uri.query_string());
        li.appendChild(anchor);
    }
}


/** \brief Define the page with which the list shall start.
 *
 * This function defines the start page you want to read with the read_list()
 * function. By default this is set to 1 to represent the very first page.
 *
 * This parameter must be at least 1. If larger than the total number of
 * pages available, then the read_list() will return an empty list.
 *
 * \param[in] page  The page to read with read_list().
 */
void paging_t::set_page(int32_t page)
{
    // make sure this is at least 1
    f_page = std::max(1, page);
std::cerr << "paging f_page = " << f_page << "\n";
}


/** \brief Retrieve the start page.
 *
 * This function retrieves the page number that is to be read by the
 * read_list() function. The first page is represented with 1 and not
 * 0 as normally expected by C/C++.
 *
 * \note
 * The page number returned here will always be 1 or more.
 *
 * \return The start page.
 */
int32_t paging_t::get_page() const
{
    return f_page;
}


/** \brief Calculate the next page number.
 *
 * This function calculates the page number to use to reach the next
 * page. If the current page is the last page, then this function
 * returns -1 meaning that there is no next page.
 *
 * \warning
 * The function returns -1 if the total number of pages is not
 * yet known. That number is known only after you called The
 * read_list() at least once.
 *
 * \return The next page or -1 if there is no next page.
 */
int32_t paging_t::get_next_page() const
{
    int32_t const max_pages(get_total_pages());
    if(f_page >= max_pages || max_pages == -1)
    {
        return -1;
    }
    return f_page + 1;
}


/** \brief Calculate the previous page number.
 *
 * This function calculates the page number to use to reach the
 * previous page. If the current page is the first page, then this
 * function returns -1 meaning that there is no previous page.
 *
 * \return The previous page or -1 if there is no previous page.
 */
int32_t paging_t::get_previous_page() const
{
    if(f_page <= 1)
    {
        return -1;
    }

    return f_page - 1;
}


/** \brief Calculate the total number of pages.
 *
 * This function calculates the total number of pages available in
 * a list. This requires the total number of items available and
 * thus it is known only after the read_list() function was called
 * at least once.
 *
 * Note that a list may be empty. In that case the function returns
 * zero (no pages available.)
 *
 * \return The total number of pages available.
 */
int32_t paging_t::get_total_pages() const
{
    int32_t const page_size(get_page_size());
    return (get_number_of_items() + page_size - 1) / page_size;
}


/** \brief Set the size of a page.
 *
 * Set the number of items to be presented in a page.
 *
 * The default list paging mechanism only supports a constant
 * number of items per page.
 *
 * By default the number of items in a page is defined using the
 * database SNAP_NAME_LIST_PAGE_SIZE from the branch table. This
 * function can be used to force the size of a page and ignore
 * the size defined in the database.
 *
 * \param[in] page_size  The number of items per page for that list.
 *
 * \sa get_page_size()
 */
void paging_t::set_page_size(int32_t page_size)
{
    f_page_size = std::max(1, page_size);
}


/** \brief Retrieve the number of items per page.
 *
 * This function returns the number of items defined in a page.
 *
 * By default the function reads the size of a page for a given list
 * by reading the size from the database. This way it is easy for the
 * website owner to change that size.
 *
 * If the size is not defined in the database, then the DEFAULT_PAGE_SIZE
 * value is used (20 at the time of writing.)
 *
 * If you prefer to enforce a certain size for your list, you may call
 * the set_page_size() function. This way the data will not be hit.
 *
 * \return The number of items defined in one page.
 *
 * \sa set_page_size()
 */
int32_t paging_t::get_page_size() const
{
    if(f_default_page_size < 1)
    {
        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
        f_default_page_size = branch_table->row(f_ipath.get_branch_key())->cell(get_name(SNAP_NAME_LIST_PAGE_SIZE))->value().safeInt32Value();
        if(f_default_page_size < 1)
        {
            // not defined in the database, bump it to 20
            f_default_page_size = DEFAULT_PAGE_SIZE;
        }
    }

    if(f_page_size < 1)
    {
        f_page_size = f_default_page_size;
    }

    return f_page_size;
}





/** \brief Initialize the list plugin.
 *
 * This function is used to initialize the list plugin object.
 */
list::list()
    //: f_snap(nullptr) -- auto-init
    //, f_list_table(nullptr) -- auto-init
    //, f_listref_table(nullptr) -- auto-init
    //, f_check_expressions() -- auto-init
    //, f_item_key_expressions() -- auto-init
    //, f_ping_backend(false) -- auto-init
{
}


/** \brief Clean up the list plugin.
 *
 * Ensure the list object is clean before it is gone.
 */
list::~list()
{
}


/** \brief Initialize the list.
 *
 * This function terminates the initialization of the list plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void list::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN0(list, "server", server, attach_to_session);
    SNAP_LISTEN(list, "server", server, register_backend_action, _1);
    SNAP_LISTEN(list, "layout", layout::layout, generate_page_content, _1, _2, _3, _4);
    SNAP_LISTEN(list, "content", content::content, create_content, _1, _2, _3);
    SNAP_LISTEN(list, "content", content::content, modified_content, _1);
    SNAP_LISTEN(list, "content", content::content, copy_branch_cells, _1, _2, _3);
    SNAP_LISTEN(list, "links", links::links, modified_link, _1, _2);
    SNAP_LISTEN(list, "filter", filter::filter, replace_token, _1, _2, _3, _4);
}


/** \brief Get a pointer to the list plugin.
 *
 * This function returns an instance pointer to the list plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the list plugin.
 */
list *list::instance()
{
    return g_plugin_list_factory.instance();
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString list::description() const
{
    return "Generate lists of pages using a set of parameters as defined"
          " by the system (some lists are defined internally) and the end"
          " users.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t list::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2014, 4, 9, 20, 57, 30, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief First update to run for the list plugin.
 *
 * This function is the first update for the list plugin. It creates
 * the list and listref tables.
 *
 * \note
 * We reset the cached pointer to the tables to make sure that they get
 * synchronized when used for the first time (very first initialization
 * only, do_update() is not generally called anyway, unless you are a
 * developer with the debug mode turned on.)
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void list::initial_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    get_list_table();
    f_list_table.reset();

    get_listref_table();
    f_listref_table.reset();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void list::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the list table.
 *
 * This function creates the list table if it doesn't exist yet. Otherwise
 * it simple initializes the f_list_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The list table is used to record all the pages of a website so they can
 * get sorted. As time passes older pages get removed as they are expected
 * to already be part of the list as required. Pages that are created or
 * modified are re-added to the list table so lists that include them can
 * be updated on the next run of the backend.
 *
 * New lists are created using a different scheme which is to find pages
 * using the list definitions to find said pages (i.e. all the pages link
 * under a given type, all the children of a given page, etc.)
 *
 * The table is defined as one row per website. The site_key_with_path()
 * is used as the row key. Within each row, you have one column per page
 * that was created or updated in the last little bit (until the backend
 * receives the time to work on all the lists concerned by such data.)
 * However, we need to time those entries so the column key is defined as
 * a 64 bit number representing the start date (as the
 * f_snap->get_start_date() returns) and the full key of the page that
 * was modified. This means the exact same page may appear multiple times
 * in the table. The backend is capable of ignoring duplicates.
 *
 * The content of the row is simple a boolean (signed char) set to 1.
 *
 * \return The pointer to the list table.
 */
QtCassandra::QCassandraTable::pointer_t list::get_list_table()
{
    if(!f_list_table)
    {
        f_list_table = f_snap->create_table(get_name(SNAP_NAME_LIST_TABLE), "Website list table.");
    }
    return f_list_table;
}


/** \brief Initialize the list reference table.
 *
 * This function creates the list reference table if it doesn't exist yet.
 * Otherwise it simple initializes the f_listref_table variable member.
 *
 * This table is used to reference existing rows in the list table. It is
 * separate for two reasons: (1) that way we can continue to go through
 * all the rows of a list, we do not have to skip each other row; (2) we
 * can us different attributes (because we do not need the reference
 * table to survive loss of data--that said right now it is just the same
 * as the other tables)
 *
 * \todo
 * Look into changing the table parameters to make it as effective as
 * possible for what it is used for.
 *
 * \return The pointer to the list table.
 */
QtCassandra::QCassandraTable::pointer_t list::get_listref_table()
{
    if(!f_listref_table)
    {
        f_listref_table = f_snap->create_table(get_name(SNAP_NAME_LIST_TABLE_REF), "Website list reference table.");
    }
    return f_listref_table;
}


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the \<page\> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  A fallback path in case ipath is not satisfactory.
 */
void list::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    output::output::instance()->on_generate_main_content(ipath, page, body, ctemplate);
}


/** \brief Generate the page common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  The body being generated.
 */
void list::on_generate_page_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    static_cast<void>(ipath);
    static_cast<void>(page);
    static_cast<void>(body);
    static_cast<void>(ctemplate);
}


/** \brief Signal that a page was created.
 *
 * This function is called whenever the content plugin creates a new page.
 * At that point the page may not yet be complete so we could not handle
 * the possible list updates.
 *
 * So instead the function saves the full key to the page that was just
 * created so lists that include this page can be updated by the backend
 * as required.
 *
 * \param[in,out] ipath  The path to the page being modified.
 * \param[in] owner  The plugin owner of the page.
 * \param[in] type  The type of the page.
 */
void list::on_create_content(content::path_info_t& ipath, QString const& owner, QString const& type)
{
    static_cast<void>(owner);
    static_cast<void>(type);

    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());

    // if a list is defined in this content, make sure to mark the
    // row as having a list with the last updated data set to zero
    //
    // Note: the exists() call is going to be very fast since the data will
    //       be in memory if true (if false, we still send a network request
    //       to Cassandra... but you never know in case the cache was reset!)
    //       this is going to be faster than such a test in the backend loop
    //       and replacing that with the test of the last update is going to
    //       make it a lot faster overall.
    QString const branch_key(ipath.get_branch_key());
    if(branch_table->row(branch_key)->exists(get_name(SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT)))
    {
        // zero marks the list as brand new so we use a different
        // algorithm to check the data in that case (i.e. the list of
        // rows in the list table is NOT complete!)
        QString const key(ipath.get_key());
        int64_t const zero(0);
        branch_table->row(key)->cell(get_name(SNAP_NAME_LIST_LAST_UPDATED))->setValue(zero);
    }

    on_modified_content(ipath); // then it is the same as on_modified_content()
}


/** \brief Signal that a page was modified by a new link.
 *
 * This function is called whenever the links plugin modifies a page by
 * adding a link or removing a link. By now the page should be quite
 * complete, outside of other links still missing.
 *
 * \param[in] link  The link that was just created or deleted.
 * \param[in] created  Whether the link was created (true) or deleted (false).
 */
void list::on_modified_link(links::link_info const & link, bool const created)
{
    static_cast<void>(created);

    content::path_info_t ipath;
    ipath.set_path(link.key());
    on_modified_content(ipath); // same as on_modified_content()
}


/** \brief Signal that a page was modified.
 *
 * This function is called whenever a plugin modified a page and then called
 * the modified_content() signal of the content plugin.
 *
 * This function saves the full key to the page that was just modified so
 * lists that include this page can be updated by the backend as required.
 *
 * \todo
 * When a page is modified multiple times in the same request, as mentioned,
 * only the last request sticks (i.e. because all requests will use the
 * same start date). However, since the key used in the list table includes
 * start_date as the first 8 bytes, we do not detect the fact that we
 * end up with a duplicate when updating the same page in different requests.
 * I am thinking that we should be able to know the column to be deleted by
 * saving the key of the last entry in the page (ipath->get_key(), save
 * list::key or something of the sort.) One potential problem, though, is
 * that a page that is constantly modified may never get listed.
 *
 * \param[in,out] ipath  The path to the page being modified.
 */
void list::on_modified_content(content::path_info_t& ipath)
{
    // if the same page is modified multiple times then we overwrite the
    // same entry multiple times
    QString site_key(f_snap->get_site_key_with_slash());
    QtCassandra::QCassandraTable::pointer_t list_table(get_list_table());
    QtCassandra::QCassandraTable::pointer_t listref_table(get_listref_table());
    int64_t const start_date(f_snap->get_start_date());
    QByteArray key;
    QtCassandra::appendInt64Value(key, start_date);
    QtCassandra::appendStringValue(key, ipath.get_key());
    bool const modified(true);
    list_table->row(site_key)->cell(key)->setValue(modified);

    // handle a reference so it is possible to delete the old key for that
    // very page later (i.e. if the page changes multiple times before the
    // list processes have time to catch up)
    QString const ref_key(QString("%1#ref").arg(site_key));
    QtCassandra::QCassandraValue existing_entry(listref_table->row(ref_key)->cell(ipath.get_key())->value());
    if(!existing_entry.nullValue())
    {
        QByteArray old_key(existing_entry.binaryValue());
        if(old_key != key)
        {
            // drop only if the key changed (i.e. if the code modifies the
            // same page over and over again within the same child process,
            // then the key will not change.)
            list_table->row(site_key)->dropCell(old_key, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, start_date);
        }
    }
    //
    // TBD: should we really time these rows? at this point we cannot
    //      safely delete them so the best is certainly to do that
    //      (unless we use the start_date time to create/delete these
    //      entries safely) -- the result if these row disappear too
    //      soon is that duplicates will appear in the main content
    //      which is not a big deal (XXX I really think we can delete
    //      those using the start_date saved in the cells to sort them!)
    //
    QtCassandra::QCassandraValue timed_key;
    timed_key.setBinaryValue(key);
    timed_key.setTtl(86400 * 3); // 3 days--the list should be updated within 5 min. so 3 days is in case it crashed or did not start, maybe?
    listref_table->row(ref_key)->cell(ipath.get_key())->setValue(timed_key);

    // just in case the row changed, we delete the pre-compiled (cached)
    // scripts (this could certainly be optimized but really the scripts
    // are compiled so quickly that it won't matter.)
    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
    QString const branch_key(ipath.get_branch_key());
    branch_table->row(branch_key)->dropCell(get_name(SNAP_NAME_LIST_TEST_SCRIPT), QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, start_date);
    branch_table->row(branch_key)->dropCell(get_name(SNAP_NAME_LIST_ITEM_KEY_SCRIPT), QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, start_date);

    f_ping_backend = true;
}


/** \brief Capture this event which happens last.
 *
 * \note
 * We may want to create another "real" end of session message?
 *
 * \todo
 * The on_attach_to_session() does NOT get called when we are running
 * a backend. We probably want two additional signals: "before execute"
 * and "after execute" (names are still TBD). Then this event would be
 * changed to the "after execute" event.
 *
 * \bug
 * There is a 10 seconds latency between the last hit and the time when
 * the list data is taken in account (see LIST_PROCESSING_LATENCY).
 * At this point I am not too sure how we can handle this problem
 * although I added a 10 seconds pause in the code receiving a PING which
 * seems to help quite a bit.
 */
void list::on_attach_to_session()
{
    if(f_ping_backend)
    {
        // send a PING to the backend
        f_snap->udp_ping(get_signal_name(get_name(SNAP_NAME_LIST_PAGELIST)));
    }
}


/** \brief Read a set of URIs from a list.
 *
 * This function reads a set of URIs from the list specified by \p ipath.
 *
 * The first item returned is defined by \p start. It is inclusive and the
 * very first item is number 0.
 *
 * The maximum number of items returned is defined by \p count. The number
 * may be set of -1 to returned as many items as there is available starting
 * from \p start. However, the function limits all returns to 10,000 items
 * so if the returned list is exactly 10,000 items, it is not unlikely that
 * you did not get all the items after the \p start point.
 *
 * The items are sorted by key as done by Cassandra.
 *
 * The count parameter cannot be set to zero. The function throws if you do
 * that.
 *
 * \todo
 * Note that at this point this function reads ALL item item from 0 to start
 * and throw them away. Later we'll add sub-indexes that will allow us to
 * reach any item very quickly. The sub-index will be something like this:
 *
 * \code
 *     list::index::100 = <key of the 100th item>
 *     list::index::200 = <key of the 200th item>
 *     ...
 * \endcode
 *
 * That way we can go to item 230 be starting the list scan at the 200th item.
 * We read the list::index:200 and us that key to start reading the list
 * (i.e. in the column predicate would use that key as the start key.)
 *
 * When a list name is specified, the \em page query string is checked for
 * a parameter that starts with that name, followed by a dash and a number.
 * Multiple lists can exist on a web page, and each list may be at a
 * different page. In this way, each list can define a different page
 * number, you only have to make sure that all the lists that can appear
 * on a page have a different name.
 *
 * The syntax of the query string for pages is as follow:
 *
 * \code
 *      page-<name>=<number>
 * \endcode
 *
 * \exception snap_logic_exception
 * The function raises the snap_logic_exception exception if the start or
 * count values are incompatible. The start parameter must be positive or
 * zero. The count value must be position (larger than 0) or -1 to use
 * the system maximum allowed.
 *
 * \param[in,out] ipath  The path to the list to be read.
 * \param[in] start  The first item to be returned (must be 0 or larger).
 * \param[in] count  The number of items to return (-1 for the maximum allowed).
 *
 * \return The list of items
 */
list_item_vector_t list::read_list(content::path_info_t & ipath, int start, int count)
{
    list_item_vector_t result;

    if(count == -1 || count > LIST_MAXIMUM_ITEMS)
    {
        count = LIST_MAXIMUM_ITEMS;
    }
    if(start < 0 || count <= 0)
    {
        throw snap_logic_exception(QString("list::read_list(ipath, %1, %2) called with invalid start and/or count values...")
                    .arg(start).arg(count));
    }

    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());

    QString const branch_key(ipath.get_branch_key());
    QtCassandra::QCassandraRow::pointer_t list_row(branch_table->row(branch_key));

    char const *ordered_pages(get_name(SNAP_NAME_LIST_ORDERED_PAGES));
    int const len(static_cast<int>(strlen(ordered_pages) + 2));

    QtCassandra::QCassandraColumnRangePredicate column_predicate;
    column_predicate.setStartColumnName(QString("%1::").arg(ordered_pages));
    column_predicate.setEndColumnName(QString("%1;").arg(ordered_pages));
    column_predicate.setCount(std::min(100, count)); // optimize the number of cells transferred
    column_predicate.setIndex(); // behave like an index
    for(;;)
    {
        // clear the cache before reading the next load
        list_row->clearCache();
        list_row->readCells(column_predicate);
        QtCassandra::QCassandraCells const cells(list_row->cells());
        if(cells.empty())
        {
            // all columns read
            break;
        }
        for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
        {
            if(start > 0)
            {
                --start;
            }
            else
            {
                // we keep the sort key in the item
                list_item_t item;
                item.set_sort_key(cell_iterator.key().mid(len));
                item.set_uri(cell_iterator.value()->value().stringValue());
                result.push_back(item);
                if(result.size() == count)
                {
                    // we got the count we wanted, return now
                    return result;
                }
            }
        }
    }

    return result;
}


/** \brief Register the pagelist and standalonelist actions.
 *
 * This function registers this plugin as supporting the "pagelist" and
 * the "standalonelist" actions.
 *
 * This is used by the backend to continuously and as fast as possible build
 * lists of pages. It understands PINGs so one can wake this backend up as
 * soon as required.
 *
 * \note
 * At this time there is a 10 seconds delay between a PING and the
 * processing of the list. This is to make sure that all the data
 * was saved by the main server before running the backend.
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void list::on_register_backend_action(server::backend_action_map_t& actions)
{
    actions[get_name(SNAP_NAME_LIST_PAGELIST)] = this;
    actions[get_name(SNAP_NAME_LIST_PROCESSLIST)] = this;
    actions[get_name(SNAP_NAME_LIST_STANDALONELIST)] = this;
    actions[get_name(SNAP_NAME_LIST_RESETLISTS)] = this;
}


/** \brief Retrieve the name of the signal used by the list plugin.
 *
 * This function returns "pagelist_udp_signal". Note that it says "pagelist"
 * instead of just "list" because the --list command line option already
 * "allocates" the list action name.
 *
 * See also the SNAP_NAME_LIST_SIGNAL_NAME.
 *
 * \param[in] action  The concerned action.
 *
 * \return The name of the list UDP signal.
 */
char const * list::get_signal_name(QString const & action) const
{
    if(action == get_name(SNAP_NAME_LIST_PAGELIST))
    {
        return get_name(SNAP_NAME_LIST_SIGNAL_NAME);
    }
    return backend_action::get_signal_name(action);
}


/** \brief Start the page list server.
 *
 * When running the backend the user can ask to run the pagelist
 * server (--action pagelist). This function captures those events.
 * It loops until stopped with a STOP message via the UDP address/port.
 * Note that Ctrl-C won't work because it does not support killing
 * both: the parent and child processes (we do a fork() to create
 * this child.)
 *
 * The loop updates all the lists as required, then it
 * falls asleep until the next UDP PING event received via the
 * "pagelist_udp_signal" IP:Port information. (see get_signal_name().)
 *
 * Note that because the UDP signals are not 100% reliable, the
 * server actually sleeps for 5 minutes and checks for new pages
 * whether a PING signal was received or not.
 *
 * The lists data is found in the Cassandra cluster and never
 * sent along the UDP signal. This means the UDP signals do not need
 * to be secure.
 *
 * The server should be stopped with the snapsignal tool using the
 * STOP event as follow:
 *
 * \code
 * snapsignal -a pagelist STOP
 * \endcode
 *
 * \note
 * The \p action parameter is here because some plugins may
 * understand multiple actions in which case we need to know
 * which action is waking us up.
 *
 * \param[in] action  The action this function is being called with.
 */
void list::on_backend_action(QString const& action)
{
    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t list_table(get_list_table());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

    if(action == get_name(SNAP_NAME_LIST_PAGELIST))
    {
        snap_backend* backend( dynamic_cast<snap_backend*>(f_snap.get()) );
        if(backend == nullptr)
        {
            throw list_exception_no_backend("list.cpp:on_backend_action(): could not determine the snap_backend pointer");
        }
        backend->create_signal( get_signal_name(action) );

// Test creating just one link (*:*)
//content::path_info_t list_ipath;
//list_ipath.set_path("admin");
//content::path_info_t page_ipath;
//page_ipath.set_path("user");
//bool const source_unique(false);
//bool const destination_unique(false);
//links::link_info source("list::links_test", source_unique, list_ipath.get_key(), list_ipath.get_branch());
//links::link_info destination("list::links_test", destination_unique, page_ipath.get_key(), page_ipath.get_branch());
//links::links::instance()->create_link(source, destination);
//return;

        QString const site_key(f_snap->get_site_key_with_slash());
        QString const core_plugin_threshold(get_name(SNAP_NAME_CORE_PLUGIN_THRESHOLD));
        // loop until stopped
        for(;;)
        {
            // verify that the site is ready, if not, do not process lists yet
            QtCassandra::QCassandraValue threshold(f_snap->get_site_parameter(core_plugin_threshold));
            if(!threshold.nullValue())
            {
                //list_table->clearCache(); -- we do that below in the loop no need here
                content_table->clearCache();
                branch_table->clearCache();
                revision_table->clearCache();

                // work as long as there is work to do
                int did_work(1);
                while(did_work != 0)
                {
                    did_work = 0;
                    QtCassandra::QCassandraRowPredicate row_predicate;
                    row_predicate.setCount(1000);
                    for(;;)
                    {
                        list_table->clearCache();
                        uint32_t const count(list_table->readRows(row_predicate));
                        if(count == 0)
                        {
                            // no more lists to process
                            break;
                        }
                        QtCassandra::QCassandraRows const rows(list_table->rows());
                        for(QtCassandra::QCassandraRows::const_iterator o(rows.begin());
                                o != rows.end(); ++o)
                        {
                            // do not work on standalone websites
                            if(!(*o)->exists(get_name(SNAP_NAME_LIST_STANDALONE)))
                            {
                                f_snap->init_start_date();
                                QString const key(QString::fromUtf8(o.key().data()));
                                if(key.startsWith(site_key))
                                {
                                    did_work |= generate_new_lists(key);
                                    did_work |= generate_all_lists(key);
                                }
                            }

                            // quickly end this process if the user requested a stop
                            if(backend->stop_received())
                            {
                                // clean STOP
                                // we have to exit otherwise we'd get called again with
                                // the next website!?
                                exit(0);
                            }
                        }
                    }
                }
            }

            // Stop on error
            //
            if( backend->get_error() )
            {
                SNAP_LOG_FATAL("list::on_backend_action(): caught a UDP server error");
                exit(1);
            }

            // sleep till next PING (but max. 5 minutes)
            //
            snap_backend::message_t message;
            if( backend->pop_message( message, 5 * 60 * 1000 ) )
            {
                // quickly end this process if the user requested a stop
                if(backend->stop_received())
                {
                    // clean STOP
                    // we have to exit otherwise we'd get called again with
                    // the next website!?
                    exit(0);
                }

                // Because there is a delay of LIST_PROCESSING_LATENCY
                // between the time when the user generates the PING and
                // the time we can make use of the data, we sleep here
                // before processing; note that in most cases that means
                // the data will be processed very quickly in comparison
                // to skipping on it now and waiting another 5 minutes
                // before doing anything on the new data (i.e. at this
                // time LIST_PROCESSING_LATENCY is only 10 seconds!)
                //
                // LIST_PROCESSING_LATENCY is in micro-seconds, whereas
                // the timespec structure expects nanoseconds
                // TBD -- should we add 1 sec., just in case?
                // TBD -- should we check for other UDP packets while
                //        waiting?
                struct timespec wait;
                wait.tv_sec = LIST_PROCESSING_LATENCY / 1000000;
                wait.tv_nsec = (LIST_PROCESSING_LATENCY % 1000000) * 1000;
                nanosleep(&wait, NULL);
            }
            // else -- 5 min. time out or we received the STOP message

            // quickly end this process if the user requested a stop
            if(backend->stop_received())
            {
                // clean STOP
                // we have to exit otherwise we'd get called again with
                // the next website!?
                exit(0);
            }
        }
    }
    else if(action == get_name(SNAP_NAME_LIST_STANDALONELIST))
    {
        // mark the site as a standalone website for its list management
        QString const site_key(f_snap->get_site_key_with_slash());
        int8_t const standalone(1);
        list_table->row(site_key)->cell(get_name(SNAP_NAME_LIST_STANDALONE))->setValue(standalone);
    }
    else if(action == get_name(SNAP_NAME_LIST_PROCESSLIST))
    {
        QString const url(f_snap->get_server_parameter("URL"));
        content::path_info_t ipath;
        ipath.set_path(url);
        on_modified_content(ipath);
        f_snap->udp_ping(get_signal_name(get_name(SNAP_NAME_LIST_PAGELIST)));
    }
    else if(action == get_name(SNAP_NAME_LIST_RESETLISTS))
    {
        // go through all the lists and delete the compiled script, this
        // will force the list code to regenerate all the lists; this
        // should be useful only when the code changes in such a way
        // that the current lists may not be 100% correct as they are
        int64_t const start_date(f_snap->get_start_date());
        content::path_info_t ipath;
        QString const site_key(f_snap->get_site_key_with_slash());
        ipath.set_path(site_key + get_name(SNAP_NAME_LIST_TAXONOMY_PATH));
        links::link_info info(get_name(SNAP_NAME_LIST_TYPE), false, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info child_info;
        while(link_ctxt->next_link(child_info))
        {
            QString const key(child_info.key());
            content::path_info_t list_ipath;
            list_ipath.set_path(key);
            branch_table->row(list_ipath.get_branch_key())->dropCell(get_name(SNAP_NAME_LIST_TEST_SCRIPT), QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, start_date);
            branch_table->row(list_ipath.get_branch_key())->dropCell(get_name(SNAP_NAME_LIST_ITEM_KEY_SCRIPT), QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, start_date);
        }
    }
    else
    {
        // unknown action (we should not have been called with that name!)
        throw snap_logic_exception(QString("list.cpp:on_backend_action(): list::on_backend_action(\"%1\") called with an unknown action...").arg(action));
    }
}


/** \brief Implementation of the backend process signal.
 *
 * This function captures the backend processing signal which is sent
 * by the server whenever the backend tool is run against a cluster.
 *
 * The list plugin refreshes lists of pages on websites when it receives
 * this signal assuming that the website has the parameter PROCESS_LIST
 * defined.
 *
 * This backend may end up taking a lot of processing time and may need to
 * run very quickly (i.e. within seconds when a new page is created or a
 * page is modified). For this reason we also offer an action which supports
 * the PING signal.
 *
 * This backend process will actually NOT run if the PROCESS_LISTS parameter
 * is not defined as a site parameter. With the command line:
 *
 * \code
 * snapbackend [--config snapserver.conf] --param PROCESS_LISTS=1
 * \endcode
 *
 * At this time the value used with PROCESS_LIST is not tested, however, it
 * is strongly recommended you use 1.
 *
 * It is also important to mark the list as a standalone list to avoid
 * parallelism which is NOT checked by the backend at this point (because
 * otherwise you take the risk of losing the list updating process
 * altogether.) So you want to run this command once:
 *
 * \code
 * snapbackend [--config snapserver.conf] --action standalonelist http://my-website.com/
 * \endcode
 *
 * Make sure to specify the URI of your website because otherwise all the
 * sites will be marked as standalone sites!
 *
 * Note that if you create a standalone site, then you have to either
 * allow its processing with the PROCESS_LISTS parameter, or you have
 * to start it with the pagelist and its URI:
 *
 * \code
 * snapbackend [--config snapserver.conf] --action pagelist http://my-website.com/
 * \endcode
 */
void list::on_backend_process()
{
    SNAP_LOG_TRACE() << "backend_process: update specialized lists.";

    // only process if the user clearly specified that we should do so;
    // we should never run in parallel with a background backend, hence
    // this flag (see the on_backend_action() function)
    QString const process_lists(f_snap->get_server_parameter("PROCESS_LISTS"));
    if(!process_lists.isEmpty())
    {
        // we ignore the result in this case, the backend will
        // run again soon and take care of the additional data
        // accordingly (with the action we process as much as
        // possible all in one go)
        generate_new_lists(f_snap->get_site_key_with_slash());
        generate_all_lists(f_snap->get_site_key_with_slash());
    }
}


/** \brief This function regenerates new lists for this websites.
 *
 * When creating a list for the first time, it is empty and yet it may
 * need to include all sorts of pages which are not in the "new pages"
 * table.
 *
 * This function goes through all the pages that this list expects and
 * checks whether those pages are part of the list. The function is
 * optimized by the fact that the list defines a selector. For example
 * the "children" selector means that only direct children of the
 * list are to be checked. This is most often used to build a tree like
 * set of pages (however, not only those because otherwise all lists
 * that are not listing children would need to be terminal!)
 *
 * The available selectors are:
 *
 * \li all -- all the pages of this site
 * \li children -- children of the list itself
 * \li children=cpath -- children of the specified canonicalized path
 * \li public -- use the list of public pages (a shortcut for
 *               type=types/taxonomy/system/content-types/page/public
 * \li type=cpath -- pages of that the specified type as a canonicalized path
 * \li hand-picked=cpath-list -- a hand defined list of paths that represent
 *                               the pages to put in the list, the cpaths are
 *                               separated by new-line (\n) characters
 *
 * \param[in] site_key  The site we want to process.
 *
 * \return 1 if the function changed anything, 0 otherwise
 */
int list::generate_new_lists(QString const& site_key)
{
    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());

    int did_work(0);

    content::path_info_t ipath;
    ipath.set_path(site_key + get_name(SNAP_NAME_LIST_TAXONOMY_PATH));
    links::link_info info(get_name(SNAP_NAME_LIST_TYPE), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        QString const key(child_info.key());
        content::path_info_t list_ipath;
        list_ipath.set_path(key);
        QtCassandra::QCassandraValue last_updated(branch_table->row(list_ipath.get_branch_key())->cell(get_name(SNAP_NAME_LIST_LAST_UPDATED))->value());
        if(last_updated.nullValue()
        || last_updated.int64Value() == 0)
        {
            SNAP_LOG_TRACE("list plugin working on new list \"")(list_ipath.get_key())("\"");

            QtCassandra::QCassandraRow::pointer_t list_row(branch_table->row(list_ipath.get_branch_key()));
            QString const selector(list_row->cell(get_name(SNAP_NAME_LIST_SELECTOR))->value().stringValue());

            if(selector == "children")
            {
                did_work |= generate_new_list_for_children(site_key, list_ipath);
            }
            else if(selector.startsWith("children="))
            {
                content::path_info_t root_ipath;
                root_ipath.set_path(selector.mid(9));
                did_work |= generate_new_list_for_all_descendants(list_ipath, root_ipath, false);
            }
            else if(selector == "public")
            {
                did_work |= generate_new_list_for_public(site_key, list_ipath);
            }
            else if(selector.startsWith("type="))
            {
                // user can specify any type!
                did_work |= generate_new_list_for_type(site_key, list_ipath, selector.mid(5));
            }
            else if(selector.startsWith("hand-picked="))
            {
                // user can specify any page directly!
                did_work |= generate_new_list_for_hand_picked_pages(site_key, list_ipath, selector.mid(12));
            }
            else // "all"
            {
                if(selector != "all")
                {
                    if(selector.isEmpty())
                    {
                        // the default is all because we cannot really know
                        // what pages should be checked (although the field
                        // is considered mandatory, but we ought to forget
                        // once in a while)
                        SNAP_LOG_WARNING("Mandatory field \"")(get_name(SNAP_NAME_LIST_SELECTOR))("\" not defined for \"")(list_ipath.get_key())("\". Using \"all\" as a fallback.");
                    }
                    else
                    {
                        // this could happen if you are running different
                        // versions of snap and an old backend hits a new
                        // still unknown selector
                        SNAP_LOG_WARNING("Field \"")(get_name(SNAP_NAME_LIST_SELECTOR))("\" set to unknown value \"")(selector)("\" in \"")(list_ipath.get_key())("\". Using \"all\" as a fallback.");
                    }
                }
                did_work |= generate_new_list_for_all_pages(site_key, list_ipath);
            }
        }
    }

    return did_work;
}


int list::generate_new_list_for_all_pages(QString const& site_key, content::path_info_t& list_ipath)
{
    // This is an extremely costly search which is similar to descendants starting from root instead of list_ipath
    content::path_info_t root_ipath;
    root_ipath.set_path(site_key);
    return generate_new_list_for_all_descendants(list_ipath, root_ipath, true);
}


int list::generate_new_list_for_descendant(QString const& site_key, content::path_info_t& list_ipath)
{
    static_cast<void>(site_key);
    return generate_new_list_for_all_descendants(list_ipath, list_ipath, true);
}


int list::generate_new_list_for_children(QString const& site_key, content::path_info_t& list_ipath)
{
    static_cast<void>(site_key);
    return generate_new_list_for_all_descendants(list_ipath, list_ipath, false);
}


int list::generate_new_list_for_all_descendants(content::path_info_t& list_ipath, content::path_info_t& parent, bool const descendants)
{
    int did_work(0);

    links::link_info info(content::get_name(content::SNAP_NAME_CONTENT_CHILDREN), false, parent.get_key(), parent.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        content::path_info_t child_ipath;
        child_ipath.set_path(child_info.key());
        did_work |= generate_list_for_page(child_ipath, list_ipath);

        if(descendants)
        {
            did_work |= generate_new_list_for_all_descendants(list_ipath, child_ipath, true);
        }
    }

    return did_work;
}


int list::generate_new_list_for_public(QString const& site_key, content::path_info_t& list_ipath)
{
    return generate_new_list_for_type(site_key, list_ipath, "types/taxonomy/system/content-types/page/public");
}


int list::generate_new_list_for_type(QString const& site_key, content::path_info_t& list_ipath, QString const& type)
{
#ifdef DEBUG
    if(type.startsWith("/"))
    {
        throw snap_logic_exception("list type cannot start with a slash (it won't work because we do not canonicalize the path here)");
    }
    if(type.endsWith("/"))
    {
        throw snap_logic_exception("list type cannot end with a slash (it won't work because we do not canonicalize the path here)");
    }
#endif

    int did_work(0);

    content::path_info_t ipath;
    ipath.set_path(QString("%1%2").arg(site_key).arg(type));
    links::link_info info(content::get_name(content::SNAP_NAME_CONTENT_PAGE), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        content::path_info_t child_ipath;
        child_ipath.set_path(child_info.key());
        did_work |= generate_list_for_page(child_ipath, list_ipath);
    }

    return did_work;
}


int list::generate_new_list_for_hand_picked_pages(QString const& site_key, content::path_info_t& list_ipath, QString const& hand_picked_pages)
{
    static_cast<void>(site_key);

    int did_work(0);

    QStringList pages(hand_picked_pages.split("\n"));
    int const max_pages(pages.size());
    for(int i(0); i < max_pages; ++i)
    {
        QString const path(pages[i]);
        if(path.isEmpty())
        {
            continue;
        }
        content::path_info_t page_ipath;
        page_ipath.set_path(path);
        did_work |= generate_list_for_page(page_ipath, list_ipath);
    }

    return did_work;
}


/** \brief This function regenerates all the lists of all the websites.
 *
 * This function reads the complete list of all the lists as defined in the
 * lists table for each website defined in there.
 *
 * The process can take a very long time, especially if you have a large
 * number of websites with a lot of activity. For this reason the system
 * allows you to run this process on a backend server with the --action
 * command line option.
 *
 * The process is to:
 *
 * 1. go through all the rows of the list table (one row per website)
 * 2. go through all the columns of each row of the list table
 *    (one column per page that changed since the last update; note that
 *    it can continue to grow as we work on the list!)
 * 3. if the last update(s) happened more than LIST_PROCESSING_LATENCY
 *    then that specific page is processed and any list that include
 *    this page get updated appropriately
 * 4. entries that were processed between now and now + latency are
 *    ignored in this run (this way we avoid some problems where a client
 *    is still working on that page and thus the resulting sort of the
 *    list is not going to be accurate)
 *    TBD -- we may want to preprocess these and reprocess them at least
 *    LIST_PROCESSING_LATENCY later to make sure that the sort is correct;
 *    that way lists are still, in most cases, updated really quickly
 * 5. once we got a page that needs to be checked, we look whether this
 *    page is part of a list, if not then there is nothing to do
 *
 * \param[in] site_key  The site we want to process.
 *
 * \return 1 if the function changed anything, 0 otherwise
 */
int list::generate_all_lists(QString const& site_key)
{
    QtCassandra::QCassandraTable::pointer_t list_table(get_list_table());
    QtCassandra::QCassandraRow::pointer_t list_row(list_table->row(site_key));

    // note that we do not loop over all the lists, instead we work on
    // 100 items and then exit; we do so because the cells get deleted
    // and thus we work on less entries on the next call, but give a
    // chance to the main process to create new entries each time
    //
    // Note: because it is sorted by timestamp,
    //       the oldest entries are automatically worked on first
    //
    QtCassandra::QCassandraColumnRangePredicate column_predicate;
    column_predicate.setCount(100); // do one round then exit
    column_predicate.setIndex(); // behave like an index

    list_row->clearCache();
    list_row->readCells(column_predicate);
    QtCassandra::QCassandraCells const cells(list_row->cells());
    if(cells.isEmpty())
    {
        return 0;
    }

    int did_work(0);

    // handle one batch
    for(QtCassandra::QCassandraCells::const_iterator c(cells.begin());
            c != cells.end();
            ++c)
    {
        int64_t const start_date(f_snap->get_start_date());

        // the cell
        QtCassandra::QCassandraCell::pointer_t cell(*c);
        // the key starts with the "start date" and it is followed by a
        // string representing the row key in the content table
        QByteArray const& key(cell->columnKey());
        if(key.size() < 8)
        {
            // drop any invalid entries, not need to keep them here
            list_row->dropCell(key, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
            continue;
        }

        int64_t const page_start_date(QtCassandra::int64Value(key, 0));
        if(page_start_date + LIST_PROCESSING_LATENCY > start_date)
        {
            // since the columns are sorted, anything after that will be
            // inaccessible date wise
            break;
        }

        // print out the row being worked on
        // (if it crashes it is really good to know where)
        {
            QString name;
            int64_t time(QtCassandra::uint64Value(key, 0));
            char buf[64];
            struct tm t;
            time_t const seconds(time / 1000000);
            gmtime_r(&seconds, &t);
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
            name = QString("%1.%2 (%3) %4").arg(buf).arg(time % 1000000, 6, 10, QChar('0')).arg(time).arg(QtCassandra::stringValue(key, sizeof(int64_t)));
            SNAP_LOG_TRACE("list plugin working on column \"")(name)("\"");
        }

        QString const row_key(QtCassandra::stringValue(key, sizeof(int64_t)));
        did_work |= generate_all_lists_for_page(site_key, row_key);

        // we handled that page for all the lists that we have on
        // this website, so drop it now
        list_row->dropCell(key, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());

        SNAP_LOG_TRACE("list is done working on this column.");
    }

    // clear our cache
    f_check_expressions.clear();
    f_item_key_expressions.clear();

    return did_work;
}


int list::generate_all_lists_for_page(QString const& site_key, QString const& page_key)
{
    content::path_info_t page_ipath;
    page_ipath.set_path(page_key);

    int did_work(0);

    content::path_info_t ipath;
    ipath.set_path(site_key + get_name(SNAP_NAME_LIST_TAXONOMY_PATH));
    links::link_info info(get_name(SNAP_NAME_LIST_TYPE), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        // Entries are defined with the following:
        //
        // SNAP_NAME_LIST_ITEM_KEY_SCRIPT
        //    The script used to generate the item key used to sort items
        //    of the list.
        //
        // SNAP_NAME_LIST_KEY
        //    list::key::<list key>
        //
        //    The <list key> part is the the ipath.get_key() from the
        //    list page. This way we can find the lists this item is a
        //    part of.
        //
        // SNAP_NAME_LIST_ORDERED_PAGES
        //    list::ordered_pages::<item key>
        //
        //    The <item key> part is defined using the
        //    SNAP_NAME_LIST_ITEM_KEY_SCRIPT script. If not yet defined, use
        //    SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT to create the compiled
        //    script. Note that this script may change under our feet so that
        //    means we'd lose access to the reference. For this reason, the
        //    reference is saved in the item under "list::key::<list key>".
        //
        // SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT
        //    This cell includes the original script used to compute the
        //    item key. This script is compiled from the script in the
        //    SNAP_NAME_LIST_ITEM_KEY_SCRIPT.
        //
        // SNAP_NAME_LIST_TYPE
        //    The list type, used for the standard link of a list page to
        //    the list content type.
        //

        QString const key(child_info.key());
        content::path_info_t list_ipath;
        list_ipath.set_path(key);
        did_work |= generate_list_for_page(page_ipath, list_ipath);
    }

    return did_work;
}


/** \brief Check whether a page is a match for a given list.
 *
 * This function checks the page \p page_ipath agains the different script
 * defined in list \p list_ipath. If it is a match, the page is added to
 * the list (if it was not there). If it is not a match, the page is
 * removed from the list (if it was there.)
 *
 * \param[in] page_ipath  The path to the page being tested.
 * \param[in,out] list_ipath  The path to the list being worked on.
 *
 * \return Zero (0) if nothing happens, 1 if the list was modified.
 */
int list::generate_list_for_page(content::path_info_t& page_ipath, content::path_info_t& list_ipath)
{
    // whether the function did change something: 0 no, 1 yes
    int did_work(0);

    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
    QtCassandra::QCassandraRow::pointer_t list_row(branch_table->row(list_ipath.get_branch_key()));

    try
    {
        QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
        if(!content_table->exists(page_ipath.get_key())
        || !content_table->row(page_ipath.get_key())->exists(content::get_name(content::SNAP_NAME_CONTENT_CREATED)))
        {
            // the page is not ready yet, let it be for a little longer, it will
            // be taken in account by the standard process
            // (at this point we may not even have the branch/revision data)
            return 0;
        }

        // TODO testing just the row is not enough to know whether it was deleted
        if(!branch_table->exists(page_ipath.get_branch_key()))
        {
            // branch disappeared... ignore
            // (it could have been deleted or moved--i.e. renamed)
            return 0;
        }
        QtCassandra::QCassandraRow::pointer_t page_branch_row(branch_table->row(page_ipath.get_branch_key()));

        QString const link_name(get_name(SNAP_NAME_LIST_LINK));

        QString const list_key_in_page(QString("%1::%2").arg(get_name(SNAP_NAME_LIST_KEY)).arg(list_ipath.get_key()));
        bool const included(run_list_check(list_ipath, page_ipath));
        QString const new_item_key(run_list_item_key(list_ipath, page_ipath));
        QString const new_item_key_full(QString("%1::%2").arg(get_name(SNAP_NAME_LIST_ORDERED_PAGES)).arg(new_item_key));
        if(included)
        {
            // the check script says to include this item in this list;
            // first we need to check to find under which key it was
            // included if it is already there because it may have
            // changed
            if(page_branch_row->exists(list_key_in_page))
            {
                // check to see whether the current key changed
                // note that if the destination does not exist, we still attempt
                // the drop + create (that happens when there is a change that
                // affects the key and you get a duplicate which is corrected
                // later--but we probably need to fix duplicates at some point)
                QtCassandra::QCassandraValue current_item_key(page_branch_row->cell(list_key_in_page)->value());
                QString const current_item_key_full(QString("%1::%2").arg(get_name(SNAP_NAME_LIST_ORDERED_PAGES)).arg(current_item_key.stringValue()));
                if(current_item_key_full != new_item_key_full
                || !page_branch_row->exists(new_item_key_full))
                {
                    // it changed, we have to delete the old one and
                    // create a new one
                    list_row->dropCell(current_item_key_full, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
                    list_row->cell(new_item_key_full)->setValue(page_ipath.get_key());
                    page_branch_row->cell(list_key_in_page)->setValue(new_item_key);

                    did_work = 1;
                }
                // else -- nothing changed, we are done
            }
            else
            {
                // it does not exist yet, add it

                // create a standard link between the list and the page item
                bool const source_unique(false);
                bool const destination_unique(false);
                links::link_info source(link_name, source_unique, list_ipath.get_key(), list_ipath.get_branch());
                links::link_info destination(link_name, destination_unique, page_ipath.get_key(), page_ipath.get_branch());
                links::links::instance()->create_link(source, destination);

                // create the ordered list
                list_row->cell(new_item_key_full)->setValue(page_ipath.get_key());

                // save a back reference to the ordered list so we can
                // quickly find it
                page_branch_row->cell(list_key_in_page)->setValue(new_item_key);

                did_work = 1;
            }
        }
        else
        {
            // the check script says that this path is not included in this
            // list; the item may have been included earlier so we have to
            // make sure it gets removed if still there
            if(page_branch_row->exists(list_key_in_page))
            {
                QtCassandra::QCassandraValue current_item_key(page_branch_row->cell(list_key_in_page)->value());
                QString const current_item_key_full(QString("%1::%2").arg(get_name(SNAP_NAME_LIST_ORDERED_PAGES)).arg(current_item_key.stringValue()));

                list_row->dropCell(current_item_key_full, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
                page_branch_row->dropCell(list_key_in_page, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());

                bool const source_unique(false);
                bool const destination_unique(false);
                links::link_info source(link_name, source_unique, list_ipath.get_key(), list_ipath.get_branch());
                links::link_info destination(link_name, destination_unique, page_ipath.get_key(), page_ipath.get_branch());
                links::links::instance()->delete_this_link(source, destination);

                did_work = 1;
            }
        }

    }
    catch(...)
    {
        did_work = 1;
    }

    // if a new list failed in some way, we still get this value because
    // trying again will probably not help; also empty lists would otherwise
    // not get this date
    //
    // TODO: make sure we do not set this flag if we are quitting early
    //       (i.e. child receives a STOP signal)
    //
    int64_t const start_date(f_snap->get_start_date());
    list_row->cell(get_name(SNAP_NAME_LIST_LAST_UPDATED))->setValue(start_date);

    // TODO
    // if we did work, the list size changed so we have to recalculate the
    // length (list::number_of_items) -- since we cannot be totally sure that
    // something was added or removed, we recalculate the size each time for
    // now but this is very slow so we will want to fix the optimize that
    // at a later time to make sure we do not take forever to build lists
    //
    // on the other hand, once a list is complete and we just add an
    // entry every now and then, this is not an overhead at all
    //
    if(did_work != 0)
    {
        char const *ordered_pages(get_name(SNAP_NAME_LIST_ORDERED_PAGES));

        int32_t count(0);
        QtCassandra::QCassandraColumnRangePredicate column_predicate;
        column_predicate.setStartColumnName(QString("%1::").arg(ordered_pages));
        column_predicate.setEndColumnName(QString("%1;").arg(ordered_pages));
        column_predicate.setCount(100);
        column_predicate.setIndex(); // behave like an index
        for(;;)
        {
            // clear the cache before reading the next load
            list_row->clearCache();
            list_row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(list_row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            count += cells.size();
        }

        list_row->cell(get_name(SNAP_NAME_LIST_NUMBER_OF_ITEMS))->setValue(count);
    }

    return did_work;
}


/** \brief Retrieve the test script of a list.
 *
 * This function is used to run the test script of a list object against a
 * page. It returns whether it is a match.
 *
 * The function compiles the script and saves it in the "list::test_script"
 * field of the list if it is not there yet. That way we can avoid the
 * compile step on future access.
 *
 * If the script cannot be compiled for any reason, then the function returns
 * false as if the page was not part of the list.
 *
 * The script has to return a result which can be converted to a boolean.
 *
 * \param[in,out] list_ipath  The ipath used to the list.
 * \param[in,out] page_ipath  The ipath used to the page.
 *
 * \return true if the page is to be included.
 */
bool list::run_list_check(content::path_info_t & list_ipath, content::path_info_t & page_ipath)
{
    QString const branch_key(list_ipath.get_branch_key());
    snap_expr::expr::expr_pointer_t e(nullptr);
    if(!f_check_expressions.contains(branch_key))
    {
        e = snap_expr::expr::expr_pointer_t(new snap_expr::expr);
        QByteArray program;
        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
        QtCassandra::QCassandraValue compiled_script(branch_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_TEST_SCRIPT))->value());
        if(compiled_script.nullValue())
        {
            QtCassandra::QCassandraValue script(branch_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT))->value());
            if(script.nullValue())
            {
                // no list here?!
                // TODO: generate an error
                return false;
            }
            if(!e->compile(script.stringValue()))
            {
                // script could not be compiled (invalid script!)
                // TODO: generate an error

                // create a default script so we do not try to compile the
                // broken script over and over again
                if(!e->compile("0"))
                {
                    // TODO: generate a double error!
                    //       this should really not happen
                    //       because "0" is definitively a valid script
                    return false;
                }
            }
            // save the result for next time
            branch_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_TEST_SCRIPT))->setValue(e->serialize());
        }
        else
        {
            e->unserialize(compiled_script.binaryValue());
        }
        f_check_expressions[branch_key] = e;
    }
    else
    {
        e = f_check_expressions[branch_key];
    }

    // run the script with this path
    snap_expr::variable_t result;
    snap_expr::variable_t::variable_map_t variables;
    snap_expr::variable_t var_path("path");
    var_path.set_value(page_ipath.get_cpath());
    variables["path"] = var_path;
    snap_expr::variable_t var_page("page");
    var_page.set_value(page_ipath.get_key());
    variables["page"] = var_page;
    snap_expr::variable_t var_list("list");
    var_list.set_value(list_ipath.get_key());
    variables["list"] = var_list;
    snap_expr::functions_t functions;
    e->execute(result, variables, functions);

#if 0
#ifdef DEBUG
    {
        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
        QtCassandra::QCassandraValue script(branch_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT))->value());
        SNAP_LOG_TRACE() << "script [" << script.stringValue() << "] result [" << (result.is_true() ? "true" : "false") << "]";
    }
#endif
#endif

    return result.is_true();
}


/** \brief Generate the test script of a list.
 *
 * This function is used to extract the test script of a list object.
 * The test script is saved in the list::test_script field of a page,
 * on a per branch basis. This function makes use of the branch
 * defined in the ipath.
 *
 * \param[in,out] list_ipath  The ipath used to find the list.
 * \param[in,out] page_ipath  The ipath used to find the page.
 *
 * \return The item key as a string.
 */
QString list::run_list_item_key(content::path_info_t& list_ipath, content::path_info_t& page_ipath)
{
    QString const branch_key(list_ipath.get_branch_key());
    snap_expr::expr::expr_pointer_t e(nullptr);
    if(!f_item_key_expressions.contains(branch_key))
    {
        e = snap_expr::expr::expr_pointer_t(new snap_expr::expr);
        QByteArray program;
        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
        QtCassandra::QCassandraValue compiled_script(branch_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_ITEM_KEY_SCRIPT))->value());
        if(compiled_script.nullValue())
        {
            QtCassandra::QCassandraValue script(branch_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT))->value());
            if(script.nullValue())
            {
                // no list here?!
                // TODO: generate an error
                return "";
            }
            if(!e->compile(script.stringValue()))
            {
                // script could not be compiled (invalid script!)
                // TODO: generate an error

                // create a default script so we do not try to compile the
                // broken script over and over again
                if(!e->compile("\"---\""))
                {
                    // TODO: generate a double error!
                    //       this should really not happen
                    //       because "0" is definitively a valid script
                    return "";
                }
            }
            // save the result for next time
            branch_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_ITEM_KEY_SCRIPT))->setValue(e->serialize());
        }
        else
        {
            e->unserialize(compiled_script.binaryValue());
        }
        f_item_key_expressions[branch_key] = e;
    }
    else
    {
        e = f_item_key_expressions[branch_key];
    }

    // run the script with this path
    snap_expr::variable_t result;
    snap_expr::variable_t::variable_map_t variables;
    snap_expr::variable_t var_path("path");
    var_path.set_value(page_ipath.get_cpath());
    variables["path"] = var_path;
    snap_expr::variable_t var_page("page");
    var_page.set_value(page_ipath.get_key());
    variables["page"] = var_page;
    snap_expr::variable_t var_list("list");
    var_list.set_value(list_ipath.get_key());
    variables["list"] = var_list;
    snap_expr::functions_t functions;
    e->execute(result, variables, functions);

    return result.get_string("*result*");
}


/** \brief Replace a [list::...] token with the contents of a list.
 *
 * This function replaces the list tokens with themed lists.
 *
 * The supported tokens are:
 *
 * \code
 * [list::theme(path="<list path>", theme="<theme name>", start="<start>", count="<count>")]
 * \endcode
 *
 * Theme the list define at \<list path\> with the theme \<theme name\>.
 * You may skip some items and start with item \<start\> instead of item 0.
 * You may specified the number of items to display with \<count\>. Be
 * careful because by default all the items are shown (Although there is a
 * system limit which at this time is 10,000 that still a LARGE list!)
 * The theme name, start, and count paramters are optional.
 *
 * \param[in,out] ipath  The path to the page being worked on.
 * \param[in] plugin_owner  The plugin owner of the ipath data.
 * \param[in,out] xml  The XML document used with the layout.
 * \param[in,out] token  The token object, with the token name and optional parameters.
 */
void list::on_replace_token(content::path_info_t& ipath, QString const& plugin_owner, QDomDocument& xml, filter::filter::token_info_t& token)
{
    static_cast<void>(ipath);
    static_cast<void>(plugin_owner);
    static_cast<void>(xml);

    // a list::... token?
    if(!token.is_namespace("list::"))
    {
        return;
    }

    if(token.is_token(get_name(SNAP_NAME_LIST_THEME)))
    {
        // list::theme expects one to four parameters
        if(!token.verify_args(1, 4))
        {
            return;
        }

        // Path
        filter::filter::parameter_t path_param(token.get_arg("path", 0, filter::filter::TOK_STRING));
        if(token.f_error)
        {
            return;
        }
        if(path_param.f_value.isEmpty())
        {
            token.f_error = true;
            token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list path (first parameter) of the list::theme() function cannot be an empty string.</span>";
            return;
        }

        // Theme
        QString theme("qrc:/xsl/list/default"); // default theming, simple <ul>{<li>...</li>}</ul> list
        if(token.has_arg("theme", 1))
        {
            filter::filter::parameter_t theme_param(token.get_arg("theme", 1, filter::filter::TOK_STRING));
            if(token.f_error)
            {
                return;
            }
            // if user included the ".xsl" extension, ignore it
            if(theme_param.f_value.endsWith(".xsl"))
            {
                theme_param.f_value = theme_param.f_value.left(theme_param.f_value.length() - 4);
            }
            if(!theme_param.f_value.isEmpty())
            {
                theme = theme_param.f_value;
            }
        }

        // Start
        int start(0); // start with very first item
        if(token.has_arg("start", 2))
        {
            filter::filter::parameter_t start_param(token.get_arg("start", 2, filter::filter::TOK_INTEGER));
            if(token.f_error)
            {
                return;
            }
            bool ok(false);
            start = start_param.f_value.toInt(&ok, 10);
            if(!ok)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list start (third parameter) of the list::theme() function must be a valid integer.</span>";
                return;
            }
            if(start < 0)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list start (third parameter) of the list::theme() function must be a positive integer or zero.</span>";
                return;
            }
        }

        // Count
        int count(-1); // all items
        if(token.has_arg("count", 3))
        {
            filter::filter::parameter_t count_param(token.get_arg("count", 3, filter::filter::TOK_INTEGER));
            if(token.f_error)
            {
                return;
            }
            bool ok(false);
            count = count_param.f_value.toInt(&ok, 10);
            if(!ok)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list count (forth parameter) of the list::theme() function must be a valid integer.</span>";
                return;
            }
            if(count != -1 && count <= 0)
            {
                token.f_error = true;
                token.f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list count (forth parameter) of the list::theme() function must be a valid integer large than zero or -1.</span>";
                return;
            }
        }
        // IMPORTANT NOTE: We do not check the maximum with the count
        //                 because our lists may expend with time

        content::path_info_t list_ipath;
        list_ipath.set_path(path_param.f_value);
        list_ipath.set_parameter("action", "view"); // we are just viewing this list

        quiet_error_callback list_error_callback(f_snap, true);
        plugin *list_plugin(path::path::instance()->get_plugin(list_ipath, list_error_callback));
        if(!list_error_callback.has_error() && list_plugin)
        {
            layout::layout_content *list_content(dynamic_cast<layout::layout_content *>(list_plugin));
            if(list_content == nullptr)
            {
                f_snap->die(snap_child::HTTP_CODE_INTERNAL_SERVER_ERROR,
                        "Plugin Missing",
                        QString("Plugin \"%1\" does not know how to handle a list assigned to it.").arg(list_plugin->get_plugin_name()),
                        "list::on_replace_token() the plugin does not derive from layout::layout_content.");
                NOTREACHED();
            }

            // read the list of items
            list_item_vector_t items(read_list(list_ipath, start, count));
            snap_child::post_file_t f;

            // Load the list body
            f.set_filename(theme + "-list-body.xsl");
            if(!f_snap->load_file(f) || f.get_size() == 0)
            {
                token.f_error = true;
                token.f_replacement = QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list theme (%1-list-body.xsl) could not be loaded.</span>")
                                            .arg(theme);
                return;
            }
            QString const list_body_xsl(QString::fromUtf8(f.get_data()));

            // Load the list theme
            f.set_filename(theme + "-list-theme.xsl");
            if(!f_snap->load_file(f) || f.get_size() == 0)
            {
                token.f_error = true;
                token.f_replacement = QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list theme (%1-list-theme.xsl) could not be loaded.</span>")
                                            .arg(theme);
                return;
            }
            QString const list_theme_xsl(QString::fromUtf8(f.get_data()));

            // Load the item body
            f.set_filename(theme + "-item-body.xsl");
            if(!f_snap->load_file(f) || f.get_size() == 0)
            {
                token.f_error = true;
                token.f_replacement = QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list theme (%1-item-theme.xsl) could not be loaded.</span>")
                                            .arg(theme);
                return;
            }
            QString const item_body_xsl(QString::fromUtf8(f.get_data()));

            // Load the item theme
            f.set_filename(theme + "-item-theme.xsl");
            if(!f_snap->load_file(f) || f.get_size() == 0)
            {
                token.f_error = true;
                token.f_replacement = QString("<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> list theme (%1-item-theme.xsl) could not be loaded.</span>")
                                            .arg(theme);
                return;
            }
            QString const item_theme_xsl(QString::fromUtf8(f.get_data()));

            layout::layout *layout_plugin(layout::layout::instance());
            QDomDocument list_doc(layout_plugin->create_document(list_ipath, list_plugin));
            layout_plugin->create_body(list_doc, list_ipath, list_body_xsl, list_content);
            // TODO: fix this problem (i.e. /products, /feed...)
            // The following is a "working" fix so we can generate a list
            // for the page that defines the list, but of course, in
            // that case we have the "wrong" path... calling with the
            // list_ipath generates a filter loop problem
            //content::path_info_t random_ipath;
            //random_ipath.set_path("");
            //layout_plugin->create_body(list_doc, random_ipath, list_body_xsl, list_content);

            QDomElement body(snap_dom::get_element(list_doc, "body"));
            QDomElement list_element(list_doc.createElement("list"));
            body.appendChild(list_element);

            QString const main_path(f_snap->get_uri().path());
            content::path_info_t main_ipath;
            main_ipath.set_path(main_path);

            // now theme the list
            int const max_items(items.size());
            for(int i(0), index(1); i < max_items; ++i)
            {
                list_error_callback.clear_error();
                content::path_info_t item_ipath;
                item_ipath.set_path(items[i].get_uri());
                if(item_ipath.get_parameter("action").isEmpty())
                {
                    // the default action on a link is "view" unless it
                    // references an administrative task under /admin
                    if(item_ipath.get_cpath() == "admin" || item_ipath.get_cpath().startsWith("admin/"))
                    {
                        item_ipath.set_parameter("action", "administer");
                    }
                    else
                    {
                        item_ipath.set_parameter("action", "view");
                    }
                }
                // whether we're attempting to display this item
                // (opposed to the test when going to the page or generating
                // the list in the first place)
                item_ipath.set_parameter("mode", "display");
                plugin *item_plugin(path::path::instance()->get_plugin(item_ipath, list_error_callback));
                if(!list_error_callback.has_error() && item_plugin)
                {
                    // put each box in a filter tag so that way we have
                    // a different owner and path for each
                    QDomDocument item_doc(layout_plugin->create_document(item_ipath, item_plugin));
                    QDomElement item_root(item_doc.documentElement());
                    item_root.setAttribute("index", index);

                    FIELD_SEARCH
                        (content::field_search::COMMAND_ELEMENT, snap_dom::get_element(item_doc, "metadata"))
                        (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)

                        // snap/head/metadata/desc[@type="list_uri"]/data
                        (content::field_search::COMMAND_DEFAULT_VALUE, list_ipath.get_key())
                        (content::field_search::COMMAND_SAVE, "desc[type=list_uri]/data")

                        // snap/head/metadata/desc[@type="list_path"]/data
                        (content::field_search::COMMAND_DEFAULT_VALUE, list_ipath.get_cpath())
                        (content::field_search::COMMAND_SAVE, "desc[type=list_path]/data")

                        // snap/head/metadata/desc[@type="box_uri"]/data
                        (content::field_search::COMMAND_DEFAULT_VALUE, ipath.get_key())
                        (content::field_search::COMMAND_SAVE, "desc[type=box_uri]/data")

                        // snap/head/metadata/desc[@type="box_path"]/data
                        (content::field_search::COMMAND_DEFAULT_VALUE, ipath.get_cpath())
                        (content::field_search::COMMAND_SAVE, "desc[type=box_path]/data")

                        // snap/head/metadata/desc[@type="main_page_uri"]/data
                        (content::field_search::COMMAND_DEFAULT_VALUE, main_ipath.get_key())
                        (content::field_search::COMMAND_SAVE, "desc[type=main_page_uri]/data")

                        // snap/head/metadata/desc[@type="main_page_path"]/data
                        (content::field_search::COMMAND_DEFAULT_VALUE, main_ipath.get_cpath())
                        (content::field_search::COMMAND_SAVE, "desc[type=main_page_path]/data")

                        // retrieve names of all the boxes
                        ;

                    layout_content *l(dynamic_cast<layout_content *>(item_plugin));
                    if(!l)
                    {
                        throw snap_logic_exception("the item_plugin pointer was not a layout_content");
                    }
                    layout_plugin->create_body(item_doc, item_ipath, item_body_xsl, l);
//std::cerr << "source to be parsed [" << item_doc.toString(-1) << "]\n";
                    QDomElement item_body(snap_dom::get_element(item_doc, "body"));
                    item_body.setAttribute("index", index);
                    QString themed_item(layout_plugin->apply_theme(item_doc, item_theme_xsl, theme));
//std::cerr << "themed item [" << themed_item << "]\n";

                    // add that result to the list document
                    QDomElement item(list_doc.createElement("item"));
                    list_element.appendChild(item);
                    snap_dom::insert_html_string_to_xml_doc(item, themed_item);

                    ++index; // index only counts items added to the output
                }
            }
//std::cerr << "resulting XML [" << list_doc.toString(-1) << "]\n";

            // now theme the list as a whole
            // we add a wrapper so we can use /node()/* in the final theme
            token.f_replacement = layout_plugin->apply_theme(list_doc, list_theme_xsl, theme);
        }
        // else list is not accessible (permission "problem")
    }
}


void list::on_generate_boxes_content(content::path_info_t& page_cpath, content::path_info_t& ipath, QDomElement& page, QDomElement& box, QString const& ctemplate)
{
    static_cast<void>(page_cpath);

    output::output::instance()->on_generate_main_content(ipath, page, box, ctemplate);
}


void list::on_copy_branch_cells(QtCassandra::QCassandraCells& source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    static_cast<void>(destination_branch);

    QtCassandra::QCassandraCells left_cells;

    // handle one batch
    bool has_list(false);
    for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
            nc != source_cells.end();
            ++nc)
    {
        QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
        QByteArray cell_key(source_cell->columnKey());

        if(cell_key == get_name(SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT)
        || cell_key == get_name(SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT)
        || cell_key == get_name(SNAP_NAME_LIST_SELECTOR))
        {
            has_list = true;
            // copy our fields as is
            destination_row->cell(cell_key)->setValue(source_cell->value());
        }
        else
        {
            // keep the other branch fields as is, other plugins can handle
            // them as required by implementing this signal
            //
            // note that the map is a map a shared pointers so it is fast
            // to make a copy like this
            left_cells[cell_key] = source_cell;
        }
    }

    if(has_list)
    {
        // make sure the (new) list is checked so we actually get a list
        content::path_info_t ipath;
        ipath.set_path(destination_row->rowName());
        on_modified_content(ipath);
    }

    // overwrite the source with the cells we allow to copy "further"
    source_cells = left_cells;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
