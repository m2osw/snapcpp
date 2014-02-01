// Snap Websites Server -- search capability
// Copyright (C) 2012-2014  Made to Order Software Corp.
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

#include "search.h"
#include "not_reached.h"
#include "dom_util.h"
#include "../content/content.h"


SNAP_PLUGIN_START(search, 1, 0)

/** \brief Get a fixed path name.
 *
 * The path plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char *get_name(name_t name)
{
    switch(name) {
    case SNAP_NAME_SEARCH_STATUS:
        return "search::status";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_SEARCH_...");

    }
    NOTREACHED();
}


/** \brief Initialize the search plugin.
 *
 * This function initializes the search plugin.
 */
search::search()
    //: f_snap(NULL) -- auto-init
{
}

/** \brief Destroy the search plugin.
 *
 * This function cleans up the search plugin.
 */
search::~search()
{
}

/** \brief Bootstrap the search.
 *
 * This function adds the events the search plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void search::on_bootstrap(::snap::snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(search, "server", server, improve_signature, _1, _2);
    SNAP_LISTEN(search, "layout", layout::layout, generate_page_content, _1, _2, _3, _4, _5);
}

/** \brief Get a pointer to the search plugin.
 *
 * This function returns an instance pointer to the search plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the search plugin.
 */
search *search::instance()
{
    return g_plugin_search_factory.instance();
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
QString search::description() const
{
    return "The search plugin index your website public pages in order to"
        " allow your users to search its content.";
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
int64_t search::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 11, 3, 3, 58, 54, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our search references.
 *
 * Do nothing at this point.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void search::content_update(int64_t variables_timestamp)
{
    content::content::instance()->add_xml("search");
}
#pragma GCC diagnostic pop


/** \brief Improves the error signature.
 *
 * This function adds the search page to the brief signature of die()
 * errors.
 *
 * \param[in,out] signature  The HTML signature to improve.
 */
void search::on_improve_signature(QString const& path, QString& signature)
{
    // TODO: we probably want translations? (in which case we
    //       want the signature to be in an XML document.)
    QString query(path);
    query.replace('/', ' ');
    query = query.simplified();
    // the query should never be empty since the home page should always work...
    if(!query.isEmpty())
    {
        query.replace(" ", "%20");
        signature += " <a href=\"/search?search=" + query + "\">Search Our Website</a>";
    }
}


/** \brief Generate a link to the search page.
 *
 * This function generates a link to the search page so users with advanced
 * browsers such as Mozilla can go to our search page without having to search
 * for it (ha! ha!)
 *
 * \param[in] l  The layout pointer.
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  A path used in case ipath is not defined.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void search::on_generate_page_content(layout::layout *l, content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    QDomDocument doc(page.ownerDocument());

    QDomElement bookmarks;
    dom_util::get_tag("bookmarks", body, bookmarks);

    QDomElement link(doc.createElement("link"));
    link.setAttribute("rel", "search");
    link.setAttribute("title", "Search"); // TODO: translate
    link.setAttribute("type", "text/html");
    link.setAttribute("href", f_snap->get_site_key_with_slash() + "search");
    bookmarks.appendChild(link);
}
#pragma GCC diagnostic pop



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
