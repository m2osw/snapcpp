// Snap Websites Server -- info plugin to control the core settings
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

#include "info.h"
#include "../messages/messages.h"
#include "../users/users.h"
#include "not_reached.h"
#include "poison.h"


SNAP_PLUGIN_START(info, 1, 0)


/** \class info
 * \brief Support for the basic core information.
 *
 * The core information, such are your website name, are managed by the
 * plugin.
 *
 * It is a separate plugin because the content plugin (Which would probably
 * make more sense) is a dependency of the form plugin and the information
 * requires special handling which mean the content plugin would have to
 * include the form plugin (which is not possible since the form plugin
 * includes the content plugin.)
 */


/** \brief Get a fixed info name.
 *
 * The info plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * Note that since this plugin is used to edit core and content data
 * more of the names come from those places.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_INFO_LONG_NAME:
        return "long_name";

    case SNAP_NAME_INFO_NAME:
        return "name";

    case SNAP_NAME_INFO_SHORT_NAME:
        return "short_name";


    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_INFO_...");

    }
    NOTREACHED();
}


/** \brief Initialize the info plugin.
 *
 * This function is used to initialize the info plugin object.
 */
info::info()
    //: f_snap(NULL) -- auto-init
{
}

/** \brief Clean up the info plugin.
 *
 * Ensure the info object is clean before it is gone.
 */
info::~info()
{
}

/** \brief Initialize the info.
 *
 * This function terminates the initialization of the info plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void info::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

	SNAP_LISTEN(info, "server", server, improve_signature, _1, _2);
    //SNAP_LISTEN(info, "layout", layout::layout, generate_header_content, _1, _2, _3, _4, _5);
    //SNAP_LISTEN(info, "layout", layout::layout, generate_page_content, _1, _2, _3, _4, _5);
    //SNAP_LISTEN(info, "path", path::path, can_handle_dynamic_path, _1, _2);
}


/** \brief Get a pointer to the info plugin.
 *
 * This function returns an instance pointer to the info plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the info plugin.
 */
info *info::instance()
{
    return g_plugin_info_factory.instance();
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
QString info::description() const
{
    return "The info plugin offers handling of the core information of your"
           "system. It is opens a settings page where all that information"
           "can directly be edited online.";
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
int64_t info::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2013, 12, 23, 14, 21, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief First update to run for the info plugin.
 *
 * This function is the first update for the info plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void info::initial_update(int64_t variables_timestamp)
{
}
#pragma GCC diagnostic pop


/** \brief Update the database with our info references.
 *
 * Send our info to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void info::content_update(int64_t variables_timestamp)
{
    content::content::instance()->add_xml(get_plugin_name());
}
#pragma GCC diagnostic pop


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
bool info::on_path_execute(QString const& cpath)
{
    f_snap->output(layout::layout::instance()->apply_layout(cpath, this));

    return true;
}


//void favicon::output(QString const& cpath)
//{
//    QByteArray image;
//    content::field_search::search_result_t result;
//
//    // check for a favicon.ico on this very page and then its type tree
//    bool const default_icon(cpath == "default-favicon.ico");
//    if(!default_icon)
//    {
//        QString sub_path(cpath.left(cpath.length() - (sizeof("favicon.ico") - 1)));
//        f_snap->canonicalize_path(sub_path);
//        get_icon(sub_path, result, true);
//    }
//
//    if(result.isEmpty())
//    {
//        // try the site wide parameter core::favicon
//        QtCassandra::QCassandraValue image_value;
//        if(!default_icon)
//        {
//            // try the site wide settings for an attachment
//            FIELD_SEARCH
//                // /admin/settings/favicon/@favicon::icon::path
//                (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
//                (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_FAVICON_ICON_PATH))
//                (content::field_search::COMMAND_PATH, get_name(SNAP_NAME_FAVICON_SETTINGS))
//                (content::field_search::COMMAND_SELF)
//                (content::field_search::COMMAND_RESULT, result)
//
//                // generate
//                ;
//
//            if(!result.isEmpty())
//            {
//                // we got the path to the row with the icon data
//                QtCassandra::QCassandraValue path_value(result[0]);
//                if(!path_value.nullValue())
//                {
//                    QString icon_key(path_value.stringValue());
//                    QString const site_key(f_snap->get_site_key_with_slash());
//                    if(icon_key.startsWith(site_key))
//                    {
//                        icon_key.remove(0, site_key.length());
//                    }
//                    FIELD_SEARCH
//                        // Use the path from the previous search
//                        // /admin/settings/favicon/<filename>.ico/@favicon::icon
//                        (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
//                        (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_FAVICON_ICON))
//                        (content::field_search::COMMAND_PATH, icon_key)
//                        (content::field_search::COMMAND_SELF)
//                        (content::field_search::COMMAND_RESULT, result)
//
//                        // generate
//                        ;
//
//                    if(!result.isEmpty())
//                    {
//                        image_value = result[0];
//                    }
//                }
//            }
//        }
//
//        if(image_value.nullValue())
//        {
//            // last resort we use the version saved in our resources
//            QFile file(":/plugins/favicon/snap-favicon.ico");
//            if(!file.open(QIODevice::ReadOnly))
//            {
//                f_snap->die(snap_child::HTTP_CODE_NOT_FOUND, "Icon Not Found",
//                        "This website does not have a favorite icon.",
//                        "Could not load the default resource favicon \":/plugins/favicon/snap-favicon.ico\".");
//                NOTREACHED();
//            }
//            image = file.readAll();
//        }
//        else
//        {
//            image = image_value.binaryValue();
//        }
//    }
//    else
//    {
//        image = result[0].binaryValue();
//    }
//
//    // Note: since IE v11.x PNG and GIF are supported.
//    //       support varies between browsers
//    //
//    // we know that this image is an ICO, although if someone changes
//    // it to something else (PNG, GIF...) the agent could fail
//    // the newer media type is image/vnd.microsoft.icon
//    // the old media type was image/x-icon
//    f_snap->set_header("Content-Type", "image/vnd.microsoft.icon");
//
//    f_snap->output(image);
//}


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
void info::on_generate_main_content(layout::layout *l, const QString& cpath, QDomElement& page, QDomElement& body, const QString& ctemplate)
{
printf("*** info main content...\n");
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
//void favicon::on_generate_page_content(layout::layout *l, QString const& cpath, QDomElement& page, QDomElement& body, QString const& ctemplate)
//{
//    content::field_search::search_result_t result;
//
//    get_icon(cpath, result, false);
//
//    // add the favicon.ico name at the end of the path we've found
//    QString icon_path;
//    if(result.isEmpty())
//    {
//        icon_path = f_snap->get_site_key_with_slash() + "favicon.ico";
//    }
//    else
//    {
//        icon_path = result[0].stringValue();
//        if(!icon_path.endsWith("/"))
//        {
//            icon_path += "/";
//        }
//        icon_path += "favicon.ico";
//    }
//
//    FIELD_SEARCH
//        (content::field_search::COMMAND_ELEMENT, body)
//        (content::field_search::COMMAND_CHILD_ELEMENT, "image")
//        (content::field_search::COMMAND_CHILD_ELEMENT, "shortcut")
//        (content::field_search::COMMAND_ELEMENT_ATTR, "type=image/vnd.microsoft.icon")
//        (content::field_search::COMMAND_ELEMENT_ATTR, "href=" + icon_path)
//        // TODO get the image sizes when saving the image in the database
//        //      so that way we can retrieve them around here
//        (content::field_search::COMMAND_ELEMENT_ATTR, "width=16")
//        (content::field_search::COMMAND_ELEMENT_ATTR, "height=16")
//
//        // generate
//        ;
//}



/** \brief Process a post from the info settings form.
 *
 * This function processes the post of the info settings form.
 *
 * This function is defined because some of the parameters in the form
 * cannot be auto-saved (although they kind of could, some parameters
 * are expected in the site information table instead.)
 *
 * \param[in] cpath  The path the user is accessing now.
 * \param[in] info  The user session being processed.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void info::on_process_post(QString const& cpath, sessions::sessions::session_info const& session_info)
{
    if(cpath != "admin/settings/info")
    {
        // this should not happen because invalid paths will not pass the
        // session validation process
        throw info_exception_invalid_path("info::on_process_post() was called with an unsupported path: \"" + cpath + "\"");
    }

    QtCassandra::QCassandraValue value;
    QString name;

    name = f_snap->postenv(get_name(SNAP_NAME_INFO_NAME));
    value = name;
    f_snap->set_site_parameter(snap::get_name(SNAP_NAME_CORE_SITE_NAME), value);

    name = f_snap->postenv(get_name(SNAP_NAME_INFO_LONG_NAME));
    value = name;
    f_snap->set_site_parameter(snap::get_name(SNAP_NAME_CORE_SITE_LONG_NAME), value);

    name = f_snap->postenv(get_name(SNAP_NAME_INFO_SHORT_NAME));
    value = name;
    f_snap->set_site_parameter(snap::get_name(SNAP_NAME_CORE_SITE_SHORT_NAME), value);
}
#pragma GCC diagnostic pop


/** \brief Improves the error signature.
 *
 * This function adds a link to the administration page to the signature of
 * die() errors. This is done only if the user is logged in.
 *
 * \param[in] path  The path on which the error occurs.
 * \param[in,out] signature  The HTML signature to improve.
 */
void info::on_improve_signature(QString const& path, QString& signature)
{
    (void)path;
    if(!users::users::instance()->get_user_key().isEmpty())
    {
        // TODO: translate
        signature += " <a href=\"/admin\">Administration</a>";
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
