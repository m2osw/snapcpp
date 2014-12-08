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
#include "../locale/snap_locale.h"
#include "../path/path.h"

#include "log.h"
#include "not_reached.h"
#include "qdomhelpers.h"
#include "qdomxpath.h"
#include "qhtmlserializer.h"
#include "qxmlmessagehandler.h"

#include <QFile>
#include <QXmlQuery>

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

    case SNAP_NAME_FEED_DESCRIPTION:
        return "feed::description";

    case SNAP_NAME_FEED_PAGE_LAYOUT:
        return "feed::page_layout";

    case SNAP_NAME_FEED_TTL:
        return "feed::ttl";

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

    SNAP_PLUGIN_UPDATE(2014, 12, 7, 22, 19, 42, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void feed::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
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
    layout::layout *layout_plugin(layout::layout::instance());
    path::path *path_plugin(path::path::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

    // the children of this location are the XSLT 2.0 files to convert the
    // data to an actual feed file
    content::path_info_t admin_feed_ipath;
    admin_feed_ipath.set_path("admin/feed");
    QVector<QString> feed_formats;

    // first loop through the list of feeds defined under /feed
    content::path_info_t ipath;
    ipath.set_path("feed");
    if(!content_table->exists(ipath.get_key())
    || !content_table->row(ipath.get_key())->exists(content::get_name(content::SNAP_NAME_CONTENT_CREATED)))
    {
        return;
    }
    links::link_info info(content::get_name(content::SNAP_NAME_CONTENT_CHILDREN), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        // this path is to a list of pages for a specific feed
        content::path_info_t child_ipath;
        child_ipath.set_path(child_info.key());

        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(child_ipath.get_revision_key()));

        // TODO: is the page layout directly a feed XSL file or is it
        //       the name to an attachment? (or maybe we should just check
        //       for a specifically named attachment?)
        QString feed_parser_layout(revision_row->cell(get_name(SNAP_NAME_FEED_PAGE_LAYOUT))->value().stringValue());
        if(feed_parser_layout.isEmpty())
        {
            // already loaded?
            if(f_feed_parser_xsl.isEmpty())
            {
                QFile file(":/xsl/layout/feed-parser.xsl");
                if(!file.open(QIODevice::ReadOnly))
                {
                    f_snap->die(snap_child::HTTP_CODE_INTERNAL_SERVER_ERROR,
                        "Default Feed Layout Unavailable",
                        "Somehow the default feed layout was accessible.",
                        "feed::generate_feeds() could not open the feed-parser.xsl resource file.");
                    NOTREACHED();
                }
                QByteArray data(file.readAll());
                f_feed_parser_xsl = QString::fromUtf8(data.data(), data.size());
                if(f_feed_parser_xsl.isEmpty())
                {
                    f_snap->die(snap_child::HTTP_CODE_INTERNAL_SERVER_ERROR,
                        "Default Feed Layout Empty",
                        "Somehow the default feed layout is empty.",
                        "feed::generate_feeds() could not read the feed-parser.xsl resource file.");
                    NOTREACHED();
                }
            }
            feed_parser_layout = f_feed_parser_xsl;
        }
        // else -- so? load from an attachment? (TBD)

        // replace <xsl:include ...> with other XSTL files (should be done
        // by the parser, but Qt's parser does not support it yet)
        layout_plugin->replace_includes(feed_parser_layout);

        // get the list, we expect that all the feed lists are ordered by
        // creation or publication date of the page as expected by the
        // various feed APIs
        //
        // TODO: fix the max. # of entries to make use of a user defined setting instead
        list::list *list_plugin(list::list::instance());
        list::list_item_vector_t list(list_plugin->read_list(child_ipath, 0, 100));
        bool first(true);
        QDomDocument result;
        int const max_items(list.size());
        for(int i(0); i < max_items; ++i)
        {
            content::path_info_t page_ipath;
            page_ipath.set_path(list[i].get_uri());

            // only pages that can be handled by layouts are added
            // others are silently ignored
            quiet_error_callback feed_error_callback(f_snap, true);
            plugins::plugin *layout_ready(path_plugin->get_plugin(page_ipath, feed_error_callback));
            if(layout_ready)
            {
                QDomDocument doc(layout_plugin->create_document(page_ipath, layout_ready));
                // should we have a ctemplate for this create body?
                layout_plugin->create_body(doc, page_ipath, feed_parser_layout, dynamic_cast<layout::layout_content *>(layout_ready), "", false, "feed-parser");
                if(first)
                {
                    first = false;
                    result = doc;
                }
                else
                {
                    // only keep the output of further pages
                    // (the header should be the same, except for a few things
                    // such as the path and data extracted from the main page,
                    // which should not be used in the feed...)
                    QDomElement output(snap_dom::get_child_element(doc, "snap/page/body/output"));
                    QDomElement body(snap_dom::get_child_element(result, "snap/page/body"));
                    body.appendChild(output);
                }
            }
            //else -- log the error?
        }

        // only create the feed output if data was added to the result
        if(!first)
        {
            locale::locale *locale_plugin(locale::locale::instance());
            locale_plugin->set_timezone();
            locale_plugin->set_locale();

            QDomElement metadata_tag(snap_dom::get_child_element(result, "snap/head/metadata"));

            // /snap/head/metadata/desc[@type="description"]/data
            // (only if still undefined)
            //
            // avoid adding the description from the feed description
            // if the website description was already added...
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/snap/head/metadata/desc[@type='description']/data");
            QDomXPath::node_vector_t current_description(dom_xpath.apply(result));
            if(current_description.isEmpty())
            {
                QString const feed_description(revision_row->cell(get_name(SNAP_NAME_FEED_DESCRIPTION))->value().stringValue());
                QDomElement desc(result.createElement("desc"));
                metadata_tag.appendChild(desc);
                desc.setAttribute("type", "description");
                QDomElement data(result.createElement("data"));
                desc.appendChild(data);
                snap_dom::insert_html_string_to_xml_doc(data, feed_description);
            }

            {
                QDomElement desc(result.createElement("desc"));
                metadata_tag.appendChild(desc);
                desc.setAttribute("type", "feed::uri");
                QDomElement data(result.createElement("data"));
                desc.appendChild(data);
                QDomText date_text(result.createTextNode(child_ipath.get_key()));
                data.appendChild(date_text);
            }

            {
                //QString const feed_description(revision_row->cell(get_name(SNAP_NAME_FEED_))->value().stringValue());
                QString name(child_ipath.get_key());
                int pos(name.lastIndexOf('/'));
                if(pos > 0)
                {
                    name = name.mid(pos + 1);
                }
                pos = name.lastIndexOf('.');
                if(pos > 0)
                {
                    name = name.mid(0, pos);
                }
                QDomElement desc(result.createElement("desc"));
                metadata_tag.appendChild(desc);
                desc.setAttribute("type", "feed::name");
                QDomElement data(result.createElement("data"));
                desc.appendChild(data);
                QDomText date_text(result.createTextNode(name));
                data.appendChild(date_text);
            }

            // /snap/head/metadata/desc[@type="now"]/data
            //
            // for lastBuildDate
            {
                time_t now(time(nullptr));
                struct tm t;
                localtime_r(&now, &t);
                char date2822[256];
                strftime(date2822, sizeof(date2822), "%a, %d %b %Y %T %z", &t);
                QDomElement desc(result.createElement("desc"));
                metadata_tag.appendChild(desc);
                desc.setAttribute("type", "feed::now");
                QDomElement data(result.createElement("data"));
                desc.appendChild(data);
                QDomText date_text(result.createTextNode(QString::fromUtf8(date2822)));
                data.appendChild(date_text);
            }

            {
                QtCassandra::QCassandraValue const ttl(revision_row->cell(get_name(SNAP_NAME_FEED_TTL))->value());
                if(ttl.size() == sizeof(int64_t))
                {
                    int64_t const ttl_us(ttl.int64Value());
                    if(ttl_us >= 3600000000) // we force at least 1h
                    {
                        QDomElement desc(result.createElement("desc"));
                        metadata_tag.appendChild(desc);
                        desc.setAttribute("type", "ttl");
                        QDomElement data(result.createElement("data"));
                        desc.appendChild(data);
                        // convert ttl from microseconds to minutes
                        // (1,000,000 microseconds/second x 60 seconds/minute)
                        QDomText ttl_text(result.createTextNode(QString("%1").arg(ttl_us / (1000000 * 60))));
                        data.appendChild(ttl_text);
                    }
                }
            }

            QString const doc_str(result.toString());
            if(doc_str.isEmpty())
            {
                throw snap::snap_logic_exception("somehow the memory XML document is empty");
            }

            // formats loaded yet?
            if(feed_formats.isEmpty())
            {
                links::link_info feed_info(content::get_name(content::SNAP_NAME_CONTENT_CHILDREN), false, admin_feed_ipath.get_key(), admin_feed_ipath.get_branch());
                QSharedPointer<links::link_context> feed_link_ctxt(links::links::instance()->new_link_context(feed_info));
                links::link_info feed_child_info;
                while(feed_link_ctxt->next_link(feed_child_info))
                {
                    // this path is to a list of pages for a specific feed
                    //content::path_info_t child_ipath;
                    //child_ipath.set_path(feed_child_info.key());
                    //QString const cpath(child_ipath.get_cpath());
                    QString const key(feed_child_info.key());
                    int const pos(key.lastIndexOf("."));
                    QString const extension(key.mid(pos + 1));
                    if(extension == "xsl")
                    {
                        snap_child::post_file_t feed_xsl;
                        feed_xsl.set_filename("attachment:" + key);
                        if(f_snap->load_file(feed_xsl))
                        {
                            // got valid attachment!
                            QByteArray data(feed_xsl.get_data());
                            feed_formats.push_back(QString::fromUtf8(data.data()));
                        }
                        else
                        {
                            SNAP_LOG_WARNING("failed loading \"")(key)("\" as one of the feed formats.");
                        }
                    }
                }
            }

            // now generate the actual output (RSS, Atom, etc.)
            // from the data we just gathered
            int const max_feed(feed_formats.size());
            for(int i(0); i < max_feed; ++i)
            {
                QString const feed_xsl(feed_formats[i]);

                QXmlQuery q(QXmlQuery::XSLT20);
                QMessageHandler msg;
                msg.set_xsl(feed_xsl);
                msg.set_doc(doc_str);
                q.setMessageHandler(&msg);
                q.setFocus(doc_str);

                // set variables
                //q.bindVariable("...", QVariant(...));

                q.setQuery(feed_xsl);

                QBuffer output;
                output.open(QBuffer::ReadWrite);
                QHtmlSerializer xml(q.namePool(), &output, false);
                q.evaluateTo(&xml);

                QString const feed_data(QString::fromUtf8(output.data()));

                QDomDocument feed_result;
                feed_result.setContent(feed_data);
                QDomXPath feed_dom_xpath;
                feed_dom_xpath.setXPath("/rss/channel/description");
                QDomXPath::node_vector_t feed_cdata_tags(feed_dom_xpath.apply(feed_result));
                int const max_feed_cdata(feed_cdata_tags.size());
std::cout << "--- size: " << max_feed_cdata << "...\n";
                for(int j(0); j < max_feed_cdata; ++j)
                {
                    // we found the widget, display its label instead
std::cout << "--- Node type = " << static_cast<int>(feed_cdata_tags[j].nodeType()) << "\n";
                    QDomElement e(feed_cdata_tags[j].toElement());
                    e.removeAttribute("feed-cdata");
                    e.setAttribute("handled", "true");
std::cout << "--- TAG: " << e.tagName() << "...\n";
                }

//std::cout << "***\n*** SRC = [" << doc_str << "]\n";
std::cout << "*** DOC = [" << feed_result.toString() << "]\n***\n";
            }
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
