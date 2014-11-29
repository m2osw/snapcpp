// Snap Websites Server -- different feed handlers (RSS, Atom, RSS_Cloud, PubSubHubbub, etc.)
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
#include "../list/list.h"

#include "log.h"
#include "not_reached.h"

//#include <QtCassandra/QCassandraLock.h>

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
    case SNAP_NAME_FEED_AGE:
        return "feed::age";

    case SNAP_NAME_FEED_PAGE_LAYOUT:
        return "feed::page_layout";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_FEED_...");

    }
    NOTREACHED();
}


/** \brief Initialize the feed plugin.
 *
 * This function is used to initialize the feed plugin object.
 */
feed::feed()
    //: f_snap(nullptr) -- auto-init
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

    SNAP_LISTEN0(feed, "server", server, backend_process);
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
    return g_plugin_feed_factory.instance();
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
    return "System used to generate RSS, Atom and other feeds. It also"
          " handles subscriptions for subscription based feed systems"
          " such as RSS Cloud and PubSubHubbub.";
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

    SNAP_PLUGIN_UPDATE(2014, 1, 1, 0, 0, 0, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Implementation of the backend process signal.
 *
 * This function captures the backend processing signal which is sent
 * by the server whenever the backend tool is run against a site.
 *
 * The feed plugin generates XML files with the list of
 * pages that are saved in various lists defined under /feed.
 * By default we offer the /feed/main list which presents all the
 * public pages marked as a feed using the feed::feed tag named
 * /types/taxonomy/system/feed/main
 */
void feed::on_backend_process()
{
    SNAP_LOG_TRACE() << "backend_process: process feed.rss content.";

    generate_feeds();
}


/** \brief Generate all the feeds.
 *
 * This function goes through the list of feeds defined under /feed and
 * generates an XML document with the complete list of pages found in
 * each feed. The XML document is then parsed through the various feed
 * XSLT transformation stylesheets to generate the final output (RSS,
 * Atom, etc.)
 */
void feed::generate_feeds()
{
    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

    // first loop through the list of feeds defined under /feed
    content::path_info_t ipath;
    ipath.set_path("feed");
    links::link_info info(content::get_name(content::SNAP_NAME_CONTENT_CHILDREN), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        // this path is to a list of pages for a specific feed
        content::path_info_t child_ipath;
        child_ipath.set_path(child_info.key());

        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(child_ipath.get_revision_key()));
        QString feed_page_layout(revision_row->cell(get_name(SNAP_NAME_FEED_PAGE_LAYOUT))->value().stringValue());
        if(feed_page_layout.isEmpty())
        {
            feed_page_layout = ":/feed/xsl/feed-page-parser.xsl";
        }

        // TODO: fix the max. # of entries to make use of a user defined setting instead
        list::list *list_plugin(list::list::instance());
        list::list_item_vector_t list(list_plugin->read_list(child_ipath, 0, 100));
        int const max_items(list.size());
        for(int i(0); i < max_items; ++i)
        {
            content::path_info_t page_ipath;
            page_ipath.set_path(list[i].get_uri());
        }
    }
}







//
// Google PubSubHubHub documentation:
// https://pubsubhubbub.googlecode.com/git/pubsubhubbub-core-0.4.html
//
// RSS documentation:
// http://www.rssboard.org/rss-specification (2.x)
// http://web.resource.org/rss/1.0/
// http://www.rssboard.org/rss-0-9-1-netscape
// http://www.rssboard.org/rss-0-9-0
// 

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
