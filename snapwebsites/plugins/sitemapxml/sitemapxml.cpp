// Snap Websites Server -- Sitemap XML
// Copyright (C) 2011-2015  Made to Order Software Corp.
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

#include "sitemapxml.h"

#include "../permissions/permissions.h"
#include "../shorturl/shorturl.h"

#include "plugins.h"
#include "log.h"
#include "qdomnodemodel.h"
#include "qxmlmessagehandler.h"
#include "not_reached.h"

#include <iostream>

#include <QXmlQuery>
#include <QDomDocument>
#include <QFile>
#include <QDateTime>
#include <QDomProcessingInstruction>

#include "poison.h"


SNAP_PLUGIN_START(sitemapxml, 1, 0)


/** \brief Get a fixed sitemapxml name.
 *
 * The sitemapxml plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_SITEMAPXML_COUNT: // in site table, int32
        return "sitemapxml::count";

    case name_t::SNAP_NAME_SITEMAPXML_FREQUENCY:
        return "sitemapxml::frequency";

    case name_t::SNAP_NAME_SITEMAPXML_INCLUDE:
        return "sitemapxml::include";

    case name_t::SNAP_NAME_SITEMAPXML_NAMESPACE:
        return "sitemapxml";

    case name_t::SNAP_NAME_SITEMAPXML_SITEMAP_XML: // in site table, string
        return "sitemapxml::sitemap.xml";

    case name_t::SNAP_NAME_SITEMAPXML_PRIORITY: // float
        return "sitemapxml::priority";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_SITEMAPXML_...");

    }
    NOTREACHED();
}


/** \brief Initialize the URL information to default values.
 *
 * This function initializes the URL info class to default
 * values. Especially, the priority is set to 0.5.
 */
sitemapxml::url_info::url_info()
    //: f_uri("") -- auto-init
    : f_priority(0.5f)
    //, f_last_modification() -- auto-init
    //, f_frequency() -- auto-init
{
}


/** \brief Set the URI of this resource.
 *
 * This is the URI (often called URL) of the resource being
 * added to the XML sitemap.
 *
 * \param[in] uri  The URI of the resource being added.
 */
void sitemapxml::url_info::set_uri(QString const & uri)
{
    f_uri = uri;
}


/** \brief Set the priority of the resource.
 *
 * This function let you defines the priority of the resource.
 * Resources with a higher priority will be checked out by search
 * engines first. It is also customary to present those first in
 * the XML sitemap which Snap! does.
 *
 * By default the priority is set to 0.5 which is the usual value
 * for most pages (blogs, information pages, documentation.)
 *
 * The most prominent pages should be given a priority of 1.0.
 * This is done automatically for the home page. Pages that get
 * a lot of traffic should be given a value larger than 0.5. Pages
 * with poor traffic or not much value (intermediate pages) should
 * be given a priority of less than 0.5.
 *
 * \param[in] priority  The priority of this resource.
 */
void sitemapxml::url_info::set_priority(float priority)
{
    if(priority < 0.001f)
    {
        priority = 0.001f;
    }
    if(priority > 1.0f)
    {
        priority = 1.0f;
    }
    f_priority = priority;
}


/** \brief Set the last modification date.
 *
 * This function let you set the last modification date of the resource.
 * By default this is set to zero which means no modification date will
 * be saved in the XML sitemap.
 *
 * The date is in seconds.
 *
 * \param[in] last_modification  The last modification Unix date.
 */
void sitemapxml::url_info::set_last_modification(time_t last_modification)
{
    if(last_modification < 0)
    {
        last_modification = 0;
    }
    f_last_modification = last_modification;
}


/** \brief Change the frequency.
 *
 * This function allows you to change the frequency with which
 * the page changes.
 *
 * You may use the special value FREQUENCY_NONE (0) to prevent the
 * system from saving a frequency parameter in the XML sitemap.
 *
 * You may use the special value FREQUENCY_NEVER (-1) to represent
 * the special frequency "never".
 *
 * The frequency is clamped between 60 (1 min.) and 3,153,600
 * (1 year.)
 *
 * \param[in] frequency  The new frequency.
 */
void sitemapxml::url_info::set_frequency(int frequency)
{
    if(frequency < FREQUENCY_MIN && frequency != FREQUENCY_NONE && frequency != FREQUENCY_NEVER)
    {
        // 1 min.
        frequency = FREQUENCY_MIN;
    }
    else if(frequency > FREQUENCY_MAX)
    {
        // yearly
        frequency = FREQUENCY_MAX;
    }
    f_frequency = frequency;
}


/** \brief Get the URI.
 *
 * This URL has a URI which represents the location of the page including
 * the protocol and the domain name.
 *
 * \return The URI pointing to this resource.
 */
QString sitemapxml::url_info::get_uri() const
{
    return f_uri;
}


/** \brief Get the priority of this page.
 *
 * Get the priority of the page. This value represents the importance of the
 * page and the willingness of the author to have this page in search indexes.
 * Obviously search engines still do whatever they want about each page.
 *
 * \return The priority as a number between 0.001 and 1.0.
 */
float sitemapxml::url_info::get_priority() const
{
    return f_priority;
}

/** \brief Get the date when it was last modified.
 *
 * This function returns the date when that page was last modified.
 * This is a Unix date and time in seconds.
 *
 * \return The last modification date of this resource.
 */
time_t sitemapxml::url_info::get_last_modification() const
{
    return f_last_modification;
}


/** \brief Get frequency with which this page is modified.
 *
 * By default the frequency is set to 1 week. You may set
 * it to 0 to avoid including frequency information in the
 * output. Otherwise enter a valid that defines how often
 * the page changes.
 *
 * \return Frequency of change in seconds.
 */
int sitemapxml::url_info::get_frequency() const
{
    return f_frequency;
}


/** \brief Compare two sitemap entries to sort them.
 *
 * The < operator is used to sort sitemap entries so we can put
 * the most important once first (higher priority, lastest modified,
 * more frequence first.)
 *
 * The function returns true when the priority of this is larger
 * than the priority of \p rhs (i.e. it is inverted!) This is
 * because we need the largest first, not last.
 *
 * \param[in] rhs  The other URL to compare with this one
 *
 * \return true if this is considered smaller than rhs.
 */
bool sitemapxml::url_info::operator < (const url_info& rhs) const
{
    if(f_priority > rhs.f_priority)
    {
        return true;
    }
    if(f_priority < rhs.f_priority)
    {
        return false;
    }
    if(f_last_modification > rhs.f_last_modification)
    {
        return true;
    }
    if(f_last_modification < rhs.f_last_modification)
    {
        return false;
    }
    if(f_frequency > rhs.f_frequency)
    {
        return true;
    }
    if(f_frequency < rhs.f_frequency)
    {
        return false;
    }
    return f_uri >= rhs.f_uri;
}


/** \brief Initialize the sitemapxml plugin.
 *
 * This function is used to initialize the sitemapxml plugin object.
 */
sitemapxml::sitemapxml()
    //: f_snap(NULL) -- auto-init
{
}


/** \brief Clean up the sitemapxml plugin.
 *
 * Ensure the sitemapxml object is clean before it is gone.
 */
sitemapxml::~sitemapxml()
{
}


/** \brief Get a pointer to the sitemapxml plugin.
 *
 * This function returns an instance pointer to the sitemapxml plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the sitemapxml plugin.
 */
sitemapxml *sitemapxml::instance()
{
    return g_plugin_sitemapxml_factory.instance();
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
QString sitemapxml::description() const
{
    return "Generates the sitemap.xml file which is used by search engines to"
        " discover your website pages. You can change the settings to hide"
        " different pages or all your pages.";
}


/** \brief Initialize the sitemapxml.
 *
 * This function terminates the initialization of the sitemapxml plugin
 * by registering for different events.
 */
void sitemapxml::on_bootstrap(::snap::snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN0(sitemapxml, "server", server, backend_process);
    SNAP_LISTEN(sitemapxml, "content", content::content, copy_branch_cells, _1, _2, _3);
    SNAP_LISTEN(sitemapxml, "robotstxt", robotstxt::robotstxt, generate_robotstxt, _1);
    SNAP_LISTEN(sitemapxml, "shorturl", shorturl::shorturl, allow_shorturl, _1, _2, _3, _4);
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
int64_t sitemapxml::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2013, 12, 23, 18, 46, 42, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the content with our references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void sitemapxml::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    // additional sitemap<###>.xml are added dynamically as the CRON processes
    // find out that additional pages are required.
    content::content::instance()->add_xml("sitemapxml");
}


/** \brief Implementation of the robotstxt signal.
 *
 * This function adds the Sitemap field to the robotstxt file as a global field.
 * (i.e. you're expected to have only one main sitemap.)
 *
 * \note
 * Note that at this time the sitemap.xml file is expected to reside on the
 * exact same domain. This would need to be a parameter we can change.
 * For example, for all our websites we could make use of a specialized
 * computer to handle all the sitemaps and place the results on:
 *
 * http://sitemap.snapwebsites.net/
 *
 * That would reduce the load on the important servers that need to respond
 * to normal users as fast as possible.
 *
 * \param[in] r  The robotstxt object.
 */
void sitemapxml::on_generate_robotstxt(robotstxt::robotstxt *r)
{
    r->add_robots_txt_field(f_snap->get_site_key() + "sitemap.xml", "Sitemap", "", true);
}


/** \brief Called whenever the user tries to access a sitemap.xml file.
 *
 * This function generates and returns the sitemap.xml file contents.
 *
 * The sitemap.xml is generated by reading all the pages defined in the
 * database and removing any page that is clearly marked as "not for
 * the sitemap" (most often non-public pages, and any page the user
 * marks as hidden.)
 *
 * The sitemap is really generated by the backend. The front end only
 * spits out the map that is ready to be sent to the requested.
 *
 * \bug
 * When the backend regenerates a new set of XML sitemap files, it
 * will quickly replace all the old XML sitemaps. If a robot was
 * reading the old sitemaps (assuming there are multiple of them)
 * then it may end up reading a mix of old and new sitemaps. To
 * avoid this problem, we need to keep track of who reads what and
 * keep a copy of the old sitemaps for a little while.
 *
 * \param[in] ipath  The URL path used to access this page.
 *
 * \return true if the sitemap.xml file is properly generated, false otherwise.
 */
bool sitemapxml::on_path_execute(content::path_info_t& ipath)
{
    if(ipath.get_cpath() == "sitemap.xsl")
    {
        // this is the XSL file used to transform the XML sitemap to HTML
        // and thus make it human readable (outside of the text version)
        QFile xsl(":/plugins/sitemapxml/sitemapxml-to-html.xsl");
        if(!xsl.open(QIODevice::ReadOnly))
        {
            SNAP_LOG_FATAL("sitemapxml::on_path_execute() could not open sitemapxml-to-html.xsl resource file.");
            return false;
        }
        QByteArray data(xsl.readAll());
        f_snap->set_header("Content-Type", "text/xml; charset=utf-8");
        f_snap->output(data);
        return true;
    }

    // TODO: add support for any number of sitemaps
    //       (i.e. sitemap1.xml, sitemap2.xml, etc.)

    // We don't generate the sitemap from here, that's reserved
    // for the backend... instead we get information from the
    // database such as the count & actual XML
    // Until the backend runs, the sitemap does not exist and the
    // site returns a 404.
    //
    // Try something like this to get the XML sitemaps:
    //       snapbackend -c snapserver.conf

    QtCassandra::QCassandraValue count_value(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_SITEMAPXML_COUNT)));
    if(count_value.nullValue() || 0 == count_value.int32Value())
    {
        // no sitemap available at this point
        return false;
    }

    QtCassandra::QCassandraValue sitemap_data;
    int count(count_value.int32Value());
    if(1 == count)
    {
        // special case when there is just one file
        if(ipath.get_cpath() != "sitemap.xml" && ipath.get_cpath() != "sitemap.txt")
        {
            // wrong filename!
            return false;
        }
        sitemap_data = f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_SITEMAPXML_SITEMAP_XML));
    }
    else
    {

        if(count < 0)
        {
            // invalid sitemap count?!
            return false;
        }

        // there are "many" files, that's handled differently than 1 file
        QRegExp re("sitemap([0-9]*).xml");
        if(!re.exactMatch(ipath.get_cpath()))
        {
            // invalid filename for a sitemap
            return false;
        }

        // check the sitemap number
        snap_string_list sitemap_number(re.capturedTexts());
        if(sitemap_number.size() == 1)
        {
            // send sitemap listing all the available sitemaps
            sitemap_data = f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_SITEMAPXML_SITEMAP_XML));
        }
        else
        {
            if(sitemap_number.size() != 2)
            {
                // invalid filename?! (this case should never happen)
                return false;
            }

            // we know that the number is only composed of valid digits
            int index(sitemap_number[1].toInt());
            if(index == 0 || index > count)
            {
                // this index is out of whack!?
                return false;
            }

            // send the requested sitemap
            sitemap_data = f_snap->get_site_parameter("sitemapxml::" + ipath.get_cpath());
        }
    }

    QString xml(sitemap_data.stringValue());
    QString extension(f_snap->get_uri().option("extension"));
    if(extension == ".txt")
    {
        f_snap->set_header("Content-type", "text/plain; charset=utf-8");
        QDomDocument d("urlset");
        if(!d.setContent(xml, true))
        {
            SNAP_LOG_FATAL("sitemapxml::on_path_execute() could not set the DOM content.");
            return false;
        }
        QXmlQuery q(QXmlQuery::XSLT20);
        QMessageHandler msg;
        q.setMessageHandler(&msg);
        QDomNodeModel m(q.namePool(), d);
        QXmlNodeModelIndex x(m.fromDomNode(d.documentElement()));
        QXmlItem i(x);
        q.setFocus(i);
        QFile xsl(":/plugins/sitemapxml/sitemapxml-to-text.xsl");
        if(!xsl.open(QIODevice::ReadOnly))
        {
            SNAP_LOG_FATAL("sitemapxml::on_path_execute() could not open sitemapxml-to-text.xsl resource file.");
            return false;
        }
        q.setQuery(&xsl);
        if(!q.isValid())
        {
            throw sitemapxml_exception_invalid_xslt_data(QString("invalid XSLT query for SITEMAP.XML \"%1\" detected by Qt (text format)").arg(ipath.get_key()));
        }
        QString out;
        q.evaluateTo(&out);
        f_snap->output(out);
    }
    else
    {
        f_snap->set_header("Content-type", "text/xml; charset=utf-8");
        f_snap->output(xml);
    }
    return true;
}


/** \brief Implementation of the generate_sitemapxml signal.
 *
 * This function readies the generate_sitemapxml signal. This signal
 * is expected to be sent only by the sitemapxml plugin backend process
 * as it is considered to be extremely slow.
 *
 * This very function generates the XML sitemap from all the static pages
 * linked to the types/taxonomy/system/sitemapxml/include tag.
 *
 * Other plugins that have dynamic pages should implement this signal in
 * order to add their own public pages to the XML sitemap. (See the
 * char_chart plugin as such an example.)
 *
 * \param[in] r  At this point this parameter is ignored
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool sitemapxml::generate_sitemapxml_impl(sitemapxml *r)
{
    static_cast<void>(r);

    QtCassandra::QCassandraTable::pointer_t branch_table(content::content::instance()->get_branch_table());

    path::path *path_plugin(path::path::instance());

    content::path_info_t include_ipath;
    include_ipath.set_path("types/taxonomy/system/sitemapxml/include");
    links::link_info xml_sitemap_info(get_name(name_t::SNAP_NAME_SITEMAPXML_INCLUDE), false, include_ipath.get_key(), include_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(xml_sitemap_info));
    links::link_info xml_sitemap;
    while(link_ctxt->next_link(xml_sitemap))
    {
        QString const page_key(xml_sitemap.key());

        // valid for this site?
        QString const site_key(f_snap->get_site_key_with_slash());
        if(!page_key.startsWith(site_key))  // this should never be false!
        {
            // invalid page?!?
            continue;
        }

        content::path_info_t page_ipath;
        page_ipath.set_path(page_key);

        // anonymous user has access to that page??
        // check the path, not the site_key + path
        // XXX should we use VISITOR or RETURNING VISITOR as the status?
        content::permission_flag result;
        path_plugin->access_allowed
            ( ""            // anonymous user
            , page_ipath    // this page
            , "view"        // can the anonymous user view this page
            , permissions::get_name(permissions::name_t::SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR)    // anonymous users are Visitors
            , result        // give me the result here
            );

//std::cerr << "Found key [" << page_key << "] allowed? " << (result.allowed() ? "YES" : "Nope") << "\n";
        if(result.allowed())
        {

            // TODO: test that this page is accessible anonymously
            url_info url;

            // set the URI of the page
            url.set_uri(page_key);

            // author of the page defined a priority for the sitemap.xml file?
            QtCassandra::QCassandraValue priority(branch_table->row(page_ipath.get_branch_key())->cell(get_name(name_t::SNAP_NAME_SITEMAPXML_PRIORITY))->value());
            if(priority.nullValue())
            {
                // set the site priority to 1.0 for the home page
                // if not defined by the user
                if(page_key == site_key)
                {
                    // home page special case
                    url.set_priority(1.0f);
                }
            }
            else
            {
                url.set_priority(priority.floatValue());
            }

            // use the last modification date from that page
            QtCassandra::QCassandraValue modified(branch_table->row(page_ipath.get_branch_key())->cell(QString(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED)))->value());
            if(!modified.nullValue())
            {
                url.set_last_modification(modified.int64Value() / 1000000L); // micro-seconds -> seconds
            }

            // XXX ameliorate as we grow this feature
            QtCassandra::QCassandraValue frequency(branch_table->row(page_ipath.get_branch_key())->cell(get_name(name_t::SNAP_NAME_SITEMAPXML_FREQUENCY))->value());
            if(!frequency.nullValue())
            {
                QString f(frequency.stringValue());
                if(f == "never")
                {
                    url.set_frequency(url_info::FREQUENCY_NEVER);
                }
                else if(f == "always")
                {
                    url.set_frequency(url_info::FREQUENCY_MIN);
                }
                else if(f == "yearly")
                {
                    url.set_frequency(url_info::FREQUENCY_MAX);
                }
            }

            // TODO: add support for images, this can work by looking at
            //       the attachments of a page and any images there get
            //       added here
            //<image:image>
            //    <image:loc>http://example.com/image.jpg</image:loc>
            //</image:image>
            // http://googlewebmastercentral.blogspot.com/2010/04/adding-images-to-your-sitemaps.html

            add_url(url);
        }
    }
    return true;
}


/** \brief Implementation of the backend process signal.
 *
 * This function captures the backend processing signal which is sent
 * by the server whenever the backend tool is run against a site.
 *
 * The XML sitemap plugin generates XML files file the list of
 * pages that registered themselves as "sitemapxml::include".
 */
void sitemapxml::on_backend_process()
{
    SNAP_LOG_TRACE("sitemapxml::on_backend_process(): process sitemap.xml content.");

    uint64_t const start_date(f_snap->get_start_date());

    // now give other plugins a chance to add dynamic links to the sitemap.xml
    // file; we don't give the users access to the XML file, they call our
    // add_url() function instead
    generate_sitemapxml(this);

    // sort the result by priority, see operator < () for details
    sort(f_url_info.begin(), f_url_info.end());

    QDomDocument doc;
    // add the XML "processing instruction"
    QDomProcessingInstruction xml_marker(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\""));
    doc.appendChild(xml_marker);

    // add a little comment as some humans look at that stuff...
    QDomComment comment(doc.createComment(QString(
            "\n  Generator: sitemapxml plugin"
            "\n  Creation Date: %1"
            "\n  URL Count: %2"
            "\n  System: http://snapwebsites.org/\n").arg(f_snap->date_to_string(start_date, f_snap->date_format_t::DATE_FORMAT_HTTP)).arg(f_url_info.size())));
    doc.appendChild(comment);

    // The stylesheet makes use of a processing instruction entry
    // The XSLT file transforms the XML in an HTML table with styles
    // <?xml-stylesheet type="text/xsl" href="/sitemap.xsl"?>
    QDomProcessingInstruction stylesheet(doc.createProcessingInstruction("xml-stylesheet", "type=\"text/xsl\" href=\"/sitemap.xsl\""));
    doc.appendChild(stylesheet);

    QDomElement root(doc.createElement("urlset"));
    root.setAttribute("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9");

    doc.appendChild(root);
    // TODO: if f_url_info.count() > 50,000 then break the table in multiple files
    int count(0); // prevent an XML sitemap of more than 50000 entries for safety
    for(url_info_list_t::const_iterator u(f_url_info.begin());
                u != f_url_info.end() && count < 50000;
                ++u, ++count)
    {
        // create /url
        QDomElement url(doc.createElement("url"));
        root.appendChild(url);

        // create /url/loc
        QDomElement loc(doc.createElement("loc"));
        url.appendChild(loc);
        QDomText page_url(doc.createTextNode(u->get_uri()));
        loc.appendChild(page_url);

        // create /url/priority
        QDomElement priority(doc.createElement("priority"));
        url.appendChild(priority);
        QDomText prio(doc.createTextNode(QString("%1").arg(u->get_priority())));
        priority.appendChild(prio);

        // create /url/lastmod (optional)
        time_t t(u->get_last_modification());
        if(t != 0)
        {
            QDomElement lastmod(doc.createElement("lastmod"));
            url.appendChild(lastmod);
            QDomText mod(doc.createTextNode(f_snap->date_to_string(t * 1000000, snap_child::date_format_t::DATE_FORMAT_LONG)));
            lastmod.appendChild(mod);
        }

        // create /url/changefreq (optional)
        int f(u->get_frequency());
        if(f != url_info::FREQUENCY_NONE)
        {
            QString frequency("never");
            if(f > 0)
            {
                if(f < 86400 + 86400 / 2)
                {
                    frequency = "daily";
                }
                else if(f < 86400 * 7 + 86400 * 7 / 2)
                {
                    frequency = "weekly";
                }
                else if(f < 86400 * 7 * 5 + 86400 * 7 * 5 / 2)
                {
                    frequency = "monthly";
                }
                else if(f < 86400 * 7 * 5 * 3 + 86400 * 7 * 5 * 3 / 2)
                {
                    frequency = "quarterly";
                }
                else
                {
                    frequency = "yearly";
                }
            }
            QDomElement changefreq(doc.createElement("changefreq"));
            url.appendChild(changefreq);
            QDomText freq(doc.createTextNode(frequency));
            changefreq.appendChild(freq);
        }

        // create the /url/xhtml:link (rel="alternate")
        // see http://googlewebmastercentral.blogspot.com/2012/05/multilingual-and-multinational-site.html
        // (requires a pattern to generate the right URIs)
        // see layouts/white-theme-parser.xsl for the pattern information,
        // we have the mode that defines the "pattern" for the URI, but we
        // need to know where it is defined which is not done yet
    }

    // TODO: we need to look into supporting multiple sitemap.xml files
    f_snap->set_site_parameter(get_name(name_t::SNAP_NAME_SITEMAPXML_COUNT), 1);
    f_snap->set_site_parameter(get_name(name_t::SNAP_NAME_SITEMAPXML_SITEMAP_XML), doc.toString(-1));

    //QString content_table_name(snap::get_name(snap::name_t::SNAP_NAME_CONTENT));
    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    // we also save updated because the user doesn't directly interact with this
    // data and thus content::updated would otherwise never be changed
    QString content_updated(content::get_name(content::name_t::SNAP_NAME_CONTENT_UPDATED));
    QString content_modified(content::get_name(content::name_t::SNAP_NAME_CONTENT_MODIFIED));
    QString site_key(f_snap->get_site_key_with_slash());
    QString sitemap_xml(site_key + "sitemap.xml");
    content_table->row(sitemap_xml)->cell(content_updated)->setValue(start_date);
    content_table->row(sitemap_xml)->cell(content_modified)->setValue(start_date);
    QString sitemap_txt(site_key + "sitemap.txt");
    content_table->row(sitemap_txt)->cell(content_updated)->setValue(start_date);
    content_table->row(sitemap_txt)->cell(content_modified)->setValue(start_date);

#ifdef DEBUG
SNAP_LOG_TRACE() << "Updating [" << sitemap_xml << "]";
#endif
}


/** \brief Add a URL to the XML sitemap.
 *
 * This function adds the specified URL information to the XML sitemap.
 * This is generally called from the different implementation of the
 * generate_sitemapxml signal.
 *
 * \param[in] url  The URL information to add to the sitemap.
 */
void sitemapxml::add_url(const url_info& url)
{
    f_url_info.push_back(url);
}


/** \brief Prevent short URL on sitemap.xml files.
 *
 * sitemap.xml really do not need a short URL so we prevent those on
 * such paths.
 *
 * \param[in,out] ipath  The path being checked.
 * \param[in] owner  The plugin that owns that page.
 * \param[in] type  The type of this page.
 * \param[in,out] allow  Whether the short URL is allowed.
 */
void sitemapxml::on_allow_shorturl(content::path_info_t& ipath, QString const& owner, QString const& type, bool& allow)
{
    static_cast<void>(owner);
    static_cast<void>(type);

    if(!allow)
    {
        // already forbidden, cut short
        return;
    }

    //
    // a sitemap XML file may include a number as in:
    //
    //    sitemap101.xml
    //
    // so our test uses the start and end of the filename; this is not
    // 100% correct since sitemap-video.xml will match too...
    //
    QString const cpath(ipath.get_cpath());
    if((cpath.startsWith("sitemap") && cpath.endsWith(".xml"))
    || cpath == "sitemap.txt"
    || cpath == "sitemap.xsl")
    {
        allow = false;
    }
}


void sitemapxml::on_copy_branch_cells(QtCassandra::QCassandraCells& source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    static_cast<void>(destination_branch);

    content::content::copy_branch_cells_as_is(source_cells, destination_row, get_name(name_t::SNAP_NAME_SITEMAPXML_NAMESPACE));
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
