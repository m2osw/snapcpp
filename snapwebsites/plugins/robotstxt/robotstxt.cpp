// Snap Websites Server -- robots.txt
// Copyright (C) 2011-2013  Made to Order Software Corp.
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

#include "robotstxt.h"
#include "../../lib/plugins.h"
#include "../content/content.h"
#include <iostream>
#include "poison.h"


SNAP_PLUGIN_START(robotstxt, 1, 0)

const char *        robotstxt::ROBOT_NAME_ALL = "*";
const char *        robotstxt::ROBOT_NAME_GLOBAL = "";
const char *        robotstxt::FIELD_NAME_DISALLOW = "Disallow";

/** \brief Initialize the robotstxt plugin.
 *
 * This function is used to initialize the robotstxt plugin object.
 */
robotstxt::robotstxt()
    //: f_snap(NULL) -- auto-init
    : f_robots_path("#")
{
}


/** \brief Clean up the robotstxt plugin.
 *
 * Ensure the robotstxt object is clean before it is gone.
 */
robotstxt::~robotstxt()
{
}


/** \brief Initialize the robotstxt.
 *
 * This function terminates the initialization of the robotstxt plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void robotstxt::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(robotstxt, "layout", layout::layout, generate_header_content, _1, _2, _3, _4, _5);
    SNAP_LISTEN(robotstxt, "layout", layout::layout, generate_page_content, _1, _2, _3, _4, _5);
}


/** \brief Get a pointer to the robotstxt plugin.
 *
 * This function returns an instance pointer to the robotstxt plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the robotstxt plugin.
 */
robotstxt *robotstxt::instance()
{
    return g_plugin_robotstxt_factory.instance();
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
QString robotstxt::description() const
{
    return "Generates the robots.txt file which is used by search engines to"
        " discover your website pages. You can change the settings to hide"
        " different pages or all your pages.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run yet.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t robotstxt::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2012, 10, 13, 17, 16, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief First update to run for the robotstxt plugin.
 *
 * This function is the first update for the robotstxt plugin. It installs
 * the initial robots.txt page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void robotstxt::initial_update(int64_t variables_timestamp)
{
    // this is now done by the install content process
    //path::path::instance()->add_path("robotstxt", "robots.txt", variables_timestamp);
}
#pragma GCC diagnostic pop

/** \brief Update the content with our references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void robotstxt::content_update(int64_t variables_timestamp)
{
    content::content::instance()->add_xml("robotstxt");
}
#pragma GCC diagnostic pop

/** \brief Check for the "robots.txt" path.
 *
 * This function ensures that the URL is robots.txt and if so write
 * the robots.txt file content in the QBuffer of the path.
 *
 * \param[in] p  The plugin path pointer.
 * \param[in] url  The URL being managed.
 *
 * \return true if the robots.txt file is properly generated, false otherwise.
 */
bool robotstxt::on_path_execute(const QString& url)
{
    if(url == "robots.txt")
    {
        generate_robotstxt(this);
        output();
        return true;
    }

    return false;
}

/** \brief Output the results.
 *
 * This function outputs the contents of the robots.txt file.
 */
void robotstxt::output() const
{
    f_snap->set_header("Content-Type", "text/plain; charset=utf-8");
    // TODO: change the "Expires" header to 1 day because we don't need
    //       users to check for the robots.txt that often!?

//std::cout << "----------------------------- GENERATING robots.txt\n";
    f_snap->output("# More info http://www.robotstxt.org/\n");
    f_snap->output("# Generated by http://snapwebsites.org/\n");

    robots_txt_t::const_iterator global = f_robots_txt.find("");
    if(global != f_robots_txt.end())
    {
        // in this case we don't insert any User-agent
        for(robots_field_array_t::const_iterator i = global->second.begin(); i != global->second.end(); ++i)
        {
            f_snap->output(i->f_field);
            f_snap->output(": ");
            f_snap->output(i->f_value);
            f_snap->output("\n");
        }
    }

    robots_txt_t::const_iterator all = f_robots_txt.find("*");
    if(all != f_robots_txt.end())
    {
        f_snap->output("User-agent: *\n");
        for(robots_field_array_t::const_iterator i = all->second.begin(); i != all->second.end(); ++i)
        {
            f_snap->output(i->f_field);
            f_snap->output(": ");
            f_snap->output(i->f_value);
            f_snap->output("\n");
        }
    }

    for(robots_txt_t::const_iterator r = f_robots_txt.begin(); r != f_robots_txt.end(); ++r)
    {
        if(r->first == "*" || r->first == "")
        {
            // skip the all robots ("*") and global ("") entries
            continue;
        }
        f_snap->output("User-agent: ");
        f_snap->output(r->first);
        f_snap->output("\n");
        for(robots_field_array_t::const_iterator i = r->second.begin(); i != r->second.end(); ++i)
        {
            f_snap->output(i->f_field);
            f_snap->output(": ");
            f_snap->output(i->f_value);
            f_snap->output("\n");
        }
    }
}

/** \brief Implementation of the generate_robotstxt signal.
 *
 * This function readies the generate_robotstxt signal.
 *
 * This function generates the header of the robots.txt.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool robotstxt::generate_robotstxt_impl(robotstxt *r)
{
    r->add_robots_txt_field("/admin/");
    r->add_robots_txt_field("/cgi-bin/");

    return true;
}

/** \brief Add Disallows to the robots.txt file.
 *
 * This function can be used to disallow a set of folders your plugin is
 * responsible for. All the paths that are protected in some way (i.e. the
 * user needs to be logged in to access that path) should be disallowed in
 * the robots.txt file.
 *
 * Note that all the system administrative functions are found under /admin/
 * which is already disallowed by the robots.txt plugin itself. So is the
 * /cgi-bin/ folder.
 *
 * \todo
 * The order can be important so we'll need to work on that part at some point.
 * At this time we print the entries in this order:
 *
 * \li global entries (i.e. robot = "")
 * \li the "all" robots list of fields
 * \li the other robots
 *
 * One way to setup the robots file goes like this:
 *
 * User-agent: *
 * Disallow: /
 *
 * User-agent: Good-guy
 * Disallow: /admin/
 *
 * This way only Good-guy is expected to spider your website.
 *
 * \param[in] value  The content of this field
 * \param[in] field  The name of the field being added (default "Disallow")
 * \param[in] robot  The name of the robot (default "*")
 * \param[in] unique  The field is unique, if already defined throw an error
 */
void robotstxt::add_robots_txt_field(const QString& value,
                                     const QString& field,
                                     const QString& robot,
                                     bool unique)
{
    if(field.isEmpty())
    {
        throw robotstxt_exception_invalid_field_name("robots.txt field name cannot be empty");
    }

    robots_field_array_t& d = f_robots_txt[robot];
    if(unique)
    {
        // verify unicity
        for(robots_field_array_t::const_iterator i = d.begin(); i != d.end(); ++i)
        {
            if(i->f_field == field)
            {
                throw robotstxt_exception_already_defined("field \"" + field + "\" is already defined");
            }
        }
    }
    robots_field_t f;
    f.f_field = field;
    f.f_value = value;
    d.push_back(f);
}


/** \brief Retrieve the robots setup for a page.
 *
 * This function loads the robots setup for the specified page.
 *
 * Note that the function returns an empty string if the current setup is
 * index,follow pr index,follow,archive since those represent the default
 * value of the robots meta tag.
 *
 * \todo
 * At this time there are problems with links (at least it seems that way
 * because I don't recall adding a nofollow link on the home page and yet
 * it gets the nofollow. Yet lookat at the path of the link, it appears
 * that we're reading the link for "/admin" instead if "/[index.html]".
 * I probably use some kind of default. Not that the noindex has the exact
 * same problem.
 *
 * \param[in] path  The path of the page for which links are checked to determine the robots setup.
 */
void robotstxt::define_robots(const QString& path)
{
    if(path != f_robots_path)
    {
        // Define the X-Robots HTTP header
        QStringList robots;
        QString site_key(f_snap->get_site_key_with_slash());
        {
// linking [http://csnap.m2osw.com/] / [http://csnap.m2osw.com/types/taxonomy/system/robotstxt/noindex]
// <link name="noindex" to="noindex" mode="1:*">/types/taxonomy/system/robotstxt/noindex</link>
            links::link_info xml_sitemap_info("robotstxt::noindex", false, site_key + path);
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(xml_sitemap_info));
            links::link_info robots_txt;
            if(link_ctxt->next_link(robots_txt))
            {
                robots += QString("noindex");
            }
        }
        {
            links::link_info xml_sitemap_info("robotstxt::nofollow", false, site_key + path);
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(xml_sitemap_info));
            links::link_info robots_txt;
            if(link_ctxt->next_link(robots_txt))
            {
                robots += QString("nofollow");
            }
        }
        {
            // TBD -- here I had this path "types/taxonomy/system/robotstxt/noarchive", but `path` seems correct...
            links::link_info xml_sitemap_info("robotstxt::noarchive", false, site_key + path);
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(xml_sitemap_info));
            links::link_info robots_txt;
            if(link_ctxt->next_link(robots_txt))
            {
                robots += QString("noarchive");
            }
        }
        // TODO: add the search engine specific tags

        f_robots_cache = robots.join(",");
        f_robots_path = path;
    }
}


/** \brief Add the X-Robots to the header.
 *
 * If the robots metadata is set to something else than index,follow[,archive]
 * then we want to add an X-Robots to the HTTP header. This is useful
 * to increase changes that the robots understand what we're trying to
 * tell it.
 *
 * \param[in] l  The layout being worked on.
 * \param[in] path  The path concerned by this request.
 * \param[in] header  The HTML header element.
 * \param[in] metadata  The XML metadata used with the XSLT parser.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void robotstxt::on_generate_header_content(layout::layout *l, const QString& path, QDomElement& header, QDomElement& metadata, const QString& ctemplate)
{
    define_robots(path);
    if(!f_robots_cache.isEmpty())
    {
        f_snap->set_header("X-Robots", f_robots_cache);
    }
}
#pragma GCC diagnostic pop


/** \brief Implement the main content for this class.
 *
 * If this object becomes the content object, the the layout will call this
 * function to generate the content.
 *
 * In case of the robots.txt file, we use a lower level function
 *
 * \param[in] l  Layout generating the main content.
 * \param[in] path  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void robotstxt::on_generate_main_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate)
{
}
#pragma GCC diagnostic pop


/** \brief Generate the page common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in] l  The layout pointer.
 * \param[in] path  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void robotstxt::on_generate_page_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate)
{
    QDomDocument doc(page.ownerDocument());

    define_robots(path);
    if(!f_robots_cache.isEmpty())
    {
        QDomElement created_root(doc.createElement("robots"));
        body.appendChild(created_root);
        QDomElement created(doc.createElement("tracking"));
        created_root.appendChild(created);
        QDomText text(doc.createTextNode(f_robots_cache));
        created.appendChild(text);
    }
}
#pragma GCC diagnostic pop



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
