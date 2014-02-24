// Snap Websites Server -- different feed handlers (RSS, PubSubHubHub, etc.)
// Copyright (C) 2013-2014  Made to Order Software Corp.
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

#include "feed.h"

#include "../content/content.h"
#include "../messages/messages.h"

#include "not_reached.h"

#include <QtCassandra/QCassandraLock.h>

#include "poison.h"


SNAP_PLUGIN_START(feed, 1, 0)


/** \brief Get a fixed feed name.
 *
 * The feed plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name) {
    case SNAP_NAME_SHORTURL_DATE:
        return "feed::date";

    case SNAP_NAME_SHORTURL_HTTP_LINK:
        return "Link";

    case SNAP_NAME_SHORTURL_IDENTIFIER:
        return "feed::identifier";

    case SNAP_NAME_SHORTURL_ID_ROW:
        return "*id_row*";

    case SNAP_NAME_SHORTURL_INDEX_ROW:
        return "*index_row*";

    case SNAP_NAME_SHORTURL_NO_SHORTURL:
        return "feed::no_feed";

    case SNAP_NAME_SHORTURL_TABLE:
        return "feed";

    case SNAP_NAME_SHORTURL_URL:
        return "feed::url";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_SHORTURL_...");

    }
    NOTREACHED();
}

/** \brief Initialize the feed plugin.
 *
 * This function is used to initialize the feed plugin object.
 */
feed::feed()
    //: f_snap(NULL) -- auto-init
{
}

/** \brief Clean up the feed plugin.
 *
 * Ensure the feed object is clean before it is gone.
 */
feed::~feed()
{
}

/** \brief Initialize the feed.
 *
 * This function terminates the initialization of the feed plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void feed::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(feed, "layout", layout::layout, generate_header_content, _1, _2, _3, _4);
    SNAP_LISTEN(feed, "content", content::content, create_content, _1, _2, _3);
    SNAP_LISTEN(feed, "path", path::path, can_handle_dynamic_path, _1, _2, _3);
}

/** \brief Get a pointer to the feed plugin.
 *
 * This function returns an instance pointer to the feed plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the feed plugin.
 */
feed *feed::instance()
{
    return g_plugin_feed.instance();
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
QString feed::description() const
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
int64_t feed::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2013, 12, 7, 16, 18, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief First update to run for the feed plugin.
 *
 * This function is the first update for the feed plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void feed::initial_update(int64_t variables_timestamp)
{
    get_feed_table();
}


/** \brief Update the database with our feed references.
 *
 * Send our feed to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void feed::content_update(int64_t variables_timestamp)
{
    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the content table.
 *
 * This function creates the feed table if it doesn't exist yet. Otherwise
 * it simple initializes the f_feed variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \return The pointer to the feed table.
 */
QSharedPointer<QtCassandra::QCassandraTable> feed::get_feed_table()
{
    if(f_feed.isNull())
    {
        f_feed = f_snap->create_table(get_name(SNAP_NAME_SHORTURL_TABLE), "Feed management table.");
    }
    return f_feed;
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
bool feed::on_path_execute(QString const& cpath)
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
 * Note that this is NOT the HTML output. It is the \<page\> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  The template to use in case other parameters are
 *                       not available.
 */
void feed::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    if(cpath.startsWith("s/"))
    {
        bool ok;
        int64_t const identifier(cpath.mid(2).toLongLong(&ok, 36));
        if(ok)
        {
            QSharedPointer<QtCassandra::QCassandraTable> feed(get_feed());
            QString const index(f_snap->get_website_key() + "/" + get_name(SNAP_NAME_SHORTURL_INDEX_ROW));
            QtCassandra::QCassandraValue identifier_value;
            identifier_value.setInt64Value(identifier);
            QtCassandra::QCassandraValue url(feed->row(index)->cell(identifier_value.binaryValue())->value());
            if(!url.nullValue())
            {
                // redirect the user
                QString http_link("<" + cpath + ">; rel=feed");
                f_snap->set_header(get_name(SNAP_NAME_SHORTURL_HTTP_LINK), http_link, snap_child::HEADER_MODE_REDIRECT);
                f_snap->page_redirect(url.stringValue(), snap_child::HTTP_CODE_FOUND);
                NOTREACHED();
            }
        }
        // else -- warn or something?
        content::content::instance()->on_generate_main_content("s", page, body, ctemplate);
    }
    else
    {
        // a type is just like a regular page
        content::content::instance()->on_generate_main_content(cpath, page, body, ctemplate);
    }
}



/** \brief Generate the header common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The header being generated.
 * \param[in,out] metadata  The metadata being generated.
 * \param[in] ctemplate  The path to a template if cpath does not exist.
 */
void feed::on_generate_header_content(content::path_info_t& ipath, QDomElement& header, QDomElement& metadata, QString const& ctemplate)
{
	static_cast<void>(header);

    content::field_search::search_result_t result;

    FIELD_SEARCH
        (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
        (content::field_search::COMMAND_ELEMENT, metadata)
        (content::field_search::COMMAND_PATH, cpath)

        // /snap/head/metadata/desc[@type="feed"]/data
        (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_SHORTURL_URL))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_RESULT, result)
        (content::field_search::COMMAND_SAVE, "desc[type=feed]/data")

        // generate!
        ;

    if(!result.isEmpty())
    {
        QString http_link("<" + result[0].stringValue() + ">; rel=feed");
        f_snap->set_header(get_name(SNAP_NAME_SHORTURL_HTTP_LINK), http_link);
    }
}


/** \brief Check whether \p cpath matches our introducer.
 *
 * This function checks that cpath matches the feed introducer which
 * is "/s/" by default.
 *
 * \param[in,out] ipath  The path being handled dynamically.
 * \param[in,out] plugin_info  If you understand that cpath, set yourself here.
 */
void feed::on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info)
{
    if(cpath.left(2) == "s/")
    {
        // tell the path plugin that this is ours
        plugin_info.set_plugin(this);
    }
}



// Google PubSubHubHub documentation:
// https://pubsubhubbub.googlecode.com/git/pubsubhubbub-core-0.4.html
// 

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
