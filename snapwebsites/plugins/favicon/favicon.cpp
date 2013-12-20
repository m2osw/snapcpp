// Snap Websites Server -- favicon generator and settings
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

#include "favicon.h"
#include "../messages/messages.h"
#include "not_reached.h"
#include <QFile>
#include "poison.h"


SNAP_PLUGIN_START(favicon, 1, 0)


/** \brief Get a fixed favicon name.
 *
 * The favicon plugin makes use of different names in the database. This
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
    case SNAP_NAME_FAVICON_ICON: // icon is in Cassandra
        return "favicon::icon";

    case SNAP_NAME_FAVICON_ICON_PATH:
        return "favicon::icon::path";

    case SNAP_NAME_FAVICON_IMAGE: // image is in XML
        return "favicon::image";

    case SNAP_NAME_FAVICON_SETTINGS:
        return "admin/settings/favicon";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_FAVICON_...");

    }
    NOTREACHED();
}


/** \class favicon
 * \brief Support for the favicon (favorite icon) of a website.
 *
 * The favorite icon plugin adds a small icon in your browser tab,
 * location, or some other location depending on the browser.
 *
 * With Snap! C++ the favicon.ico file must be in the Cassandra database.
 * We first check the page being accessed, its type and the parents of
 * that type up to and including content-types. If no favicon.ico is
 * defined in these, try the site parameter favicon::image. If still
 * not defined, we return the default Snap! resource file. (the blue "S").
 *
 * The following shows the existing support by browser. The file
 * format is .ico by default (old media type image/x-icon, new
 * media type: image/vnd.microsoft.icon).
 *
 * \code
 *     Support by browser versus format
 *  
 *   Browser   .ico  PNG  GIF  AGIF  JPEG  APNG  SVG
 *   Chrome      1    1    4    4      4    --    --
 *   Firefox     1    1    1    1      1     3    --
 *   IE          5   11   11   --     --    --    --
 *   Opera       7    7    7    7      7   9.5   9.6
 *   Safari      1    4    4   --      4    --    --
 * \endcode
 *
 * The plugin allows any page, theme, content type, etc. to have a
 * different favicon. Note, however, that it is very unlikely that the
 * browser will read each different icon for each different page.
 * (i.e. you are expected to have one favicon per website.)
 *
 * In most cases website owners should only define the site wide favicon.
 * The settings should allow for the module not to search the page and
 * type so as to save processing time.
 *
 * \note
 * To refresh your site's favicon you can force browsers to download a
 * new version using the link tag and a querystring on your filename. This
 * is especially helpful in production environments to make sure your users
 * get the update.
 *
 * \code
 * <link rel="shortcut icon" href="http://www.yoursite.com/favicon.ico?v=2"/>
 * \endcode
 *
 * Source: http://stackoverflow.com/questions/2208933/how-do-i-force-a-favicon-refresh
 */


/** \brief Initialize the favicon plugin.
 *
 * This function is used to initialize the favicon plugin object.
 */
favicon::favicon()
    //: f_snap(NULL) -- auto-init
{
}

/** \brief Clean up the favicon plugin.
 *
 * Ensure the favicon object is clean before it is gone.
 */
favicon::~favicon()
{
}

/** \brief Initialize the favicon.
 *
 * This function terminates the initialization of the favicon plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void favicon::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    //SNAP_LISTEN(favicon, "layout", layout::layout, generate_header_content, _1, _2, _3, _4, _5);
    SNAP_LISTEN(favicon, "layout", layout::layout, generate_page_content, _1, _2, _3, _4, _5);
    SNAP_LISTEN(favicon, "path", path::path, can_handle_dynamic_path, _1, _2);
}

/** \brief Get a pointer to the favicon plugin.
 *
 * This function returns an instance pointer to the favicon plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the favicon plugin.
 */
favicon *favicon::instance()
{
    return g_plugin_favicon_factory.instance();
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
QString favicon::description() const
{
    return "Handling of the favicon.ico file(s).";
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
int64_t favicon::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2013, 12, 7, 16, 18, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief First update to run for the favicon plugin.
 *
 * This function is the first update for the favicon plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void favicon::initial_update(int64_t variables_timestamp)
{
}


/** \brief Update the database with our favicon references.
 *
 * Send our favicon to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void favicon::content_update(int64_t variables_timestamp)
{
    content::content::instance()->add_xml(get_plugin_name());
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
bool favicon::on_path_execute(QString const& cpath)
{
    if(cpath == "favicon.ico" || cpath == "default-favicon.ico" || cpath.endsWith("/favicon.ico"))
    {
        // got to use the master favorite icon or a page specific icon
        // either way we search using the get_icon() function
        output(cpath);
        return true;
    }

    // not too sure right now whether we'd have a true here (most
    // certainly though)
    f_snap->output(layout::layout::instance()->apply_layout(cpath, this));

    return true;
}


void favicon::output(QString const& cpath)
{
    QByteArray image;
    content::field_search::search_result_t result;

    // check for a favicon.ico on this very page and then its type tree
    bool const default_icon(cpath == "default-favicon.ico");
    if(!default_icon)
    {
        QString sub_path(cpath.left(cpath.length() - (sizeof("favicon.ico") - 1)));
        f_snap->canonicalize_path(sub_path);
        get_icon(sub_path, result, true);
    }

    if(result.isEmpty())
    {
        // try the site wide parameter core::favicon
        QtCassandra::QCassandraValue image_value;
        if(!default_icon)
        {
            // try the site wide settings for an attachment
            FIELD_SEARCH
                // /admin/settings/favicon/@favicon::icon::path
                (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
                (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_FAVICON_ICON_PATH))
                (content::field_search::COMMAND_PATH, get_name(SNAP_NAME_FAVICON_SETTINGS))
                (content::field_search::COMMAND_SELF)
                (content::field_search::COMMAND_RESULT, result)

                // generate
                ;

            if(!result.isEmpty())
            {
                // we got the path to the row with the icon data
                QtCassandra::QCassandraValue path_value(result[0]);
                if(!path_value.nullValue())
                {
                    QString icon_key(path_value.stringValue());
                    QString const site_key(f_snap->get_site_key_with_slash());
                    if(icon_key.startsWith(site_key))
                    {
                        icon_key.remove(0, site_key.length());
                    }
                    FIELD_SEARCH
                        // Use the path from the previous search
                        // /admin/settings/favicon/<filename>.ico/@favicon::icon
                        (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
                        (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_FAVICON_ICON))
                        (content::field_search::COMMAND_PATH, icon_key)
                        (content::field_search::COMMAND_SELF)
                        (content::field_search::COMMAND_RESULT, result)

                        // generate
                        ;

                    if(!result.isEmpty())
                    {
                        image_value = result[0];
                    }
                }
            }
        }

        if(image_value.nullValue())
        {
            // last resort we use the version saved in our resources
            QFile file(":/plugins/favicon/snap-favicon.ico");
            if(!file.open(QIODevice::ReadOnly))
            {
                f_snap->die(snap_child::HTTP_CODE_NOT_FOUND, "Icon Not Found",
                        "This website does not have a favorite icon.",
                        "Could not load the default resource favicon \":/plugins/favicon/snap-favicon.ico\".");
                NOTREACHED();
            }
            image = file.readAll();
        }
        else
        {
            image = image_value.binaryValue();
        }
    }
    else
    {
        image = result[0].binaryValue();
    }

    // Note: since IE v11.x PNG and GIF are supported.
    //       support varies between browsers
    //
    // we know that this image is an ICO, although if someone changes
    // it to something else (PNG, GIF...) the agent could fail
    // the newer media type is image/vnd.microsoft.icon
    // the old media type was image/x-icon
    f_snap->set_header("Content-Type", "image/vnd.microsoft.icon");

    f_snap->output(image);
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
void favicon::on_generate_main_content(layout::layout *l, const QString& cpath, QDomElement& page, QDomElement& body, const QString& ctemplate)
{
    // our settings pages are like any standard pages
    content::content::instance()->on_generate_main_content(l, cpath, page, body, ctemplate);
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
void favicon::on_generate_page_content(layout::layout *l, QString const& cpath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    content::field_search::search_result_t result;

    get_icon(cpath, result, false);

    // add the favicon.ico name at the end of the path we've found
    QString icon_path;
    if(result.isEmpty())
    {
        icon_path = f_snap->get_site_key_with_slash() + "favicon.ico";
    }
    else
    {
        icon_path = result[0].stringValue();
        if(!icon_path.endsWith("/"))
        {
            icon_path += "/";
        }
        icon_path += "favicon.ico";
    }

    FIELD_SEARCH
        (content::field_search::COMMAND_ELEMENT, body)
        (content::field_search::COMMAND_CHILD_ELEMENT, "image")
        (content::field_search::COMMAND_CHILD_ELEMENT, "shortcut")
        (content::field_search::COMMAND_ELEMENT_ATTR, "type=image/vnd.microsoft.icon")
        (content::field_search::COMMAND_ELEMENT_ATTR, "href=" + icon_path)
        // TODO get the image sizes when saving the image in the database
        //      so that way we can retrieve them around here
        (content::field_search::COMMAND_ELEMENT_ATTR, "width=16")
        (content::field_search::COMMAND_ELEMENT_ATTR, "height=16")

        // generate
        ;
}


/** \brief Search for the favorite icon for a given page.
 *
 * This function searches for the favorite icon for a given page. If not
 * found anywhere, then the default can be used (i.e. favicon.ico in the
 * root.)
 *
 * \param[in] cpath  The page for which we are searching the icon
 * \param[out] result  The result is saved in this array.
 * \param[in] image  If true the image itself is returned, if false you
 *                   get the path to the image.
 */
void favicon::get_icon(QString const& cpath, content::field_search::search_result_t& result, bool image)
{
    result.clear();

    FIELD_SEARCH
        (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
        (content::field_search::COMMAND_PATH, cpath)

        // /snap/head/metadata/desc[@type="favicon"]/data
        (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_FAVICON_IMAGE))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_IF_FOUND, 1)
            (content::field_search::COMMAND_LINK, content::get_name(content::SNAP_NAME_CONTENT_PAGE_TYPE))
            (content::field_search::COMMAND_SELF)
            (content::field_search::COMMAND_IF_FOUND, 1)
            (content::field_search::COMMAND_PARENTS, "types/taxonomy/system/content-types")
        (content::field_search::COMMAND_LABEL, 1)
        (content::field_search::COMMAND_RESULT, result)

        // retrieve!
        ;
}


/** \brief Check whether \p cpath matches our introducer.
 *
 * This function checks that cpath matches the favicon introducer which
 * is "/s/" by default.
 *
 * \param[in] path_plugin  A pointer to the path plugin.
 * \param[in] cpath  The path being handled dynamically.
 */
void favicon::on_can_handle_dynamic_path(path::path *path_plugin, const QString& cpath)
{
    // for favicon.ico we already know since it is defined in the content.xml
    if(cpath.endsWith("/favicon.ico") || cpath == "favicon.ico" || cpath == "default-favicon.ico")
    {
        // tell the path plugin that this is ours
        path_plugin->handle_dynamic_path(this);
    }
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
