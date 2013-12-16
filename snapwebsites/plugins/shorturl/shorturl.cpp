// Snap Websites Server -- short URL handling
// Copyright (C) 2013  Made to Order Software Corp.
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

#include "shorturl.h"
#include "../content/content.h"
#include "../messages/messages.h"
#include "not_reached.h"
#include <QtCassandra/QCassandraLock.h>
#include "poison.h"


SNAP_PLUGIN_START(shorturl, 1, 0)


/** \brief Get a fixed shorturl name.
 *
 * The shorturl plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char *get_name(name_t name)
{
    switch(name) {
    case SNAP_NAME_SHORTURL_DATE:
        return "shorturl::date";

    case SNAP_NAME_SHORTURL_HTTP_LINK:
        return "Link";

    case SNAP_NAME_SHORTURL_IDENTIFIER:
        return "shorturl::identifier";

    case SNAP_NAME_SHORTURL_ID_ROW:
        return "*id_row*";

    case SNAP_NAME_SHORTURL_INDEX_ROW:
        return "*index_row*";

    case SNAP_NAME_SHORTURL_NO_SHORTURL:
        return "shorturl::no_shorturl";

    case SNAP_NAME_SHORTURL_TABLE:
        return "shorturl";

    case SNAP_NAME_SHORTURL_URL:
        return "shorturl::url";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_SHORTURL_...");

    }
    NOTREACHED();
}

/** \brief Initialize the shorturl plugin.
 *
 * This function is used to initialize the shorturl plugin object.
 */
shorturl::shorturl()
    //: f_snap(NULL) -- auto-init
{
}

/** \brief Clean up the shorturl plugin.
 *
 * Ensure the shorturl object is clean before it is gone.
 */
shorturl::~shorturl()
{
}

/** \brief Initialize the shorturl.
 *
 * This function terminates the initialization of the shorturl plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void shorturl::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(shorturl, "layout", layout::layout, generate_header_content, _1, _2, _3, _4, _5);
    SNAP_LISTEN(shorturl, "content", content::content, create_content, _1, _2, _3);
    SNAP_LISTEN(shorturl, "path", path::path, can_handle_dynamic_path, _1, _2);
}

/** \brief Get a pointer to the shorturl plugin.
 *
 * This function returns an instance pointer to the shorturl plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the shorturl plugin.
 */
shorturl *shorturl::instance()
{
    return g_plugin_shorturl_factory.instance();
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
QString shorturl::description() const
{
    return "Fully automated management of short URLs for this website.";
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
int64_t shorturl::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2013, 12, 7, 16, 18, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief First update to run for the shorturl plugin.
 *
 * This function is the first update for the shorturl plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void shorturl::initial_update(int64_t variables_timestamp)
{
    get_shorturl_table();
}


/** \brief Update the database with our shorturl references.
 *
 * Send our shorturl to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void shorturl::content_update(int64_t variables_timestamp)
{
    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the content table.
 *
 * This function creates the shorturl table if it doesn't exist yet. Otherwise
 * it simple initializes the f_shorturl_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \return The pointer to the shorturl table.
 */
QSharedPointer<QtCassandra::QCassandraTable> shorturl::get_shorturl_table()
{
    if(f_shorturl_table.isNull())
    {
        f_shorturl_table = f_snap->create_table(get_name(SNAP_NAME_SHORTURL_TABLE), "Short URL management table.");
    }
    return f_shorturl_table;
}


/** \brief Execute a page: generate the complete output of that page.
 *
 * This function displays the page that the user is trying to view. It is
 * supposed that the page permissions were already checked and thus that
 * its contents can be displayed to the current user.
 *
 * Note that the path was canonicalized by the path plugin and thus it does
 * not require any further corrections.
 *
 * \param[in] cpath  The canonicalized path being managed.
 *
 * \return true if the content is properly generated, false otherwise.
 */
bool shorturl::on_path_execute(const QString& cpath)
{
    f_snap->output(layout::layout::instance()->apply_layout(cpath, this));

    return true;
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
 * Note that this is NOT the HTML output. It is the <page> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in] l  The layout pointer.
 * \param[in] path  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void shorturl::on_generate_main_content(layout::layout *l, const QString& cpath, QDomElement& page, QDomElement& body, const QString& ctemplate)
{
    if(cpath.startsWith("s/"))
    {
        bool ok;
        int64_t const identifier(cpath.mid(2).toLongLong(&ok, 36));
        if(ok)
        {
            QSharedPointer<QtCassandra::QCassandraTable> shorturl_table(get_shorturl_table());
            QString const index(f_snap->get_website_key() + "/" + get_name(SNAP_NAME_SHORTURL_INDEX_ROW));
            QtCassandra::QCassandraValue identifier_value;
            identifier_value.setInt64Value(identifier);
            QtCassandra::QCassandraValue url(shorturl_table->row(index)->cell(identifier_value.binaryValue())->value());
            if(!url.nullValue())
            {
                // redirect the user
                // TODO: the HTTP link header should not use the set_header()
                //       because we may have many links and they should all
                //       appear in one "Link: ..." line
                QString http_link("<" + cpath + ">; rel=shorturl");
                f_snap->set_header(get_name(SNAP_NAME_SHORTURL_HTTP_LINK), http_link, snap_child::HEADER_MODE_REDIRECT);
                f_snap->page_redirect(url.stringValue(), snap_child::HTTP_CODE_FOUND);
                NOTREACHED();
            }
        }
        // else -- warn or something?
        content::content::instance()->on_generate_main_content(l, "s", page, body, ctemplate);
    }
    else
    {
        // a type is just like a regular page
        content::content::instance()->on_generate_main_content(l, cpath, page, body, ctemplate);
    }
}



/** \brief Generate the header common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in] l  The layout pointer.
 * \param[in] cpath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  The path to a template if cpath does not exist.
 */
void shorturl::on_generate_header_content(layout::layout *l, const QString& cpath, QDomElement& header, QDomElement& metadata, const QString& ctemplate)
{
    content::field_search::search_result_t result;

    FIELD_SEARCH
        (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
        (content::field_search::COMMAND_ELEMENT, metadata)
        (content::field_search::COMMAND_PATH, cpath)

        // /snap/head/metadata/desc[@type="shorturl"]/data
        (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_SHORTURL_URL))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_RESULT, result)
        (content::field_search::COMMAND_SAVE, "desc[type=shorturl]/data")

        // generate!
        ;

    if(!result.isEmpty())
    {
        QString http_link("<" + result[0].stringValue() + ">; rel=shorturl");
        f_snap->set_header(get_name(SNAP_NAME_SHORTURL_HTTP_LINK), http_link);
    }
}


void shorturl::on_create_content(QString const& path, QString const& owner, QString const& type)
{
    // do not ever create short URLs for admin pages
    if(path == "admin" || path.startsWith("admin/"))
    {
        return;
    }

    // XXX do not generate a shorturl if the existing URL is less than
    //     a certain size?

    // TODO change to support a per content type short URL scheme

    QSharedPointer<QtCassandra::QCassandraTable> shorturl_table(get_shorturl_table());

    // first generate a site wide unique identifier for that page
    int64_t identifier(0);
    QString const id_key(f_snap->get_website_key() + "/" + get_name(SNAP_NAME_SHORTURL_ID_ROW));
    QString const identifier_key(get_name(SNAP_NAME_SHORTURL_IDENTIFIER));
    QtCassandra::QCassandraValue new_identifier;
    new_identifier.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);

    {
        QtCassandra::QCassandraLock lock(f_snap->get_context(), QString("shorturl"));

        // In order to register the user in the contents we want a
        // unique identifier for each user, for that purpose we use
        // a special row in the users table and since we have a lock
        // we can safely do a read-increment-write cycle.
        if(shorturl_table->exists(id_key))
        {
            QSharedPointer<QtCassandra::QCassandraRow> id_row(shorturl_table->row(id_key));
            QSharedPointer<QtCassandra::QCassandraCell> id_cell(id_row->cell(identifier_key));
            id_cell->setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
            QtCassandra::QCassandraValue current_identifier(id_cell->value());
            if(current_identifier.nullValue())
            {
                // this means no user can register until this value gets
                // fixed somehow!
                messages::messages::instance()->set_error(
                    "Failed Creating Short URL Unique Identifier",
                    "Somehow the Short URL plugin could not create a unique identifier for your new page.",
                    "shorturl::on_create_content() could not load the *id_row* identifier, the row exists but the cell did not make it ("
                                 + id_key + "/" + identifier_key + ").",
                    false
                );
                return;
            }
            identifier = current_identifier.int64Value();
        }

        // XXX -- we could support a randomize too?
        // Note: generally, public URL shorteners will randomize this number
        //       so no two pages have the same number and they do not appear
        //       in sequence; here we do not need to do that because the
        //       website anyway denies access to all the pages that are to
        //       be hidden from preying eyes
        ++identifier;

        new_identifier.setInt64Value(identifier);
        shorturl_table->row(id_key)->cell(identifier_key)->setValue(new_identifier);

        // the lock automatically goes away here
    }

    QString const site_key(f_snap->get_site_key_with_slash());
    QString const key(site_key + path);

    QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
    QSharedPointer<QtCassandra::QCassandraRow> row(content_table->row(key));

    row->cell(identifier_key)->setValue(new_identifier);

    // save the date when the Short URL is generated so if the user changes
    // the parameters we can regenerate only those that were generated before
    // the date of the change
    uint64_t const start_date(f_snap->get_uri().option("start_date").toLongLong());
    row->cell(get_name(SNAP_NAME_SHORTURL_DATE))->setValue(start_date);

    // TODO allow the user to change the number parameters
    QString const shorturl_url(site_key + QString("s/%1").arg(identifier, 0, 36, QChar('0')));
    QtCassandra::QCassandraValue shorturl_value(shorturl_url);
    row->cell(get_name(SNAP_NAME_SHORTURL_URL))->setValue(shorturl_value);

    // create an index entry so we can find the entry and redirect the user
    // as required
    QString const index(f_snap->get_website_key() + "/" + get_name(SNAP_NAME_SHORTURL_INDEX_ROW));
    shorturl_table->row(index)->cell(new_identifier.binaryValue())->setValue(key);
}


/** \brief Check whether \p cpath matches our introducer.
 *
 * This function checks that cpath matches the shorturl introducer which
 * is "/s/" by default.
 *
 * \param[in] path_plugin  A pointer to the path plugin.
 * \param[in] cpath  The path being handled dynamically.
 */
void shorturl::on_can_handle_dynamic_path(path::path *path_plugin, const QString& cpath)
{
    if(cpath.left(2) == "s/")
    {
        // tell the path plugin that this is ours
        path_plugin->handle_dynamic_path(this);
    }
}



// API for TinyURL.com is as follow (shortening http://linux.m2osw.com/zmeu-attack)
// wget -S 'http://tinyurl.com/api-create.php?url=http%3A%2F%2Flinux.m2osw.com%2Fzmeu-attack'

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
