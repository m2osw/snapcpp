// Snap Websites Server -- info plugin to control the core settings
// Copyright (C) 2013-2015  Made to Order Software Corp.
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

#include "../editor/editor.h"
#include "../messages/messages.h"
#include "../permissions/permissions.h"
#include "../users/users.h"
#include "../output/output.h"

#include "not_reached.h"
#include "log.h"

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
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_INFO_LONG_NAME:
        return "long_name";

    case name_t::SNAP_NAME_INFO_NAME:
        return "name";

    case name_t::SNAP_NAME_INFO_SHORT_NAME:
        return "short_name";


    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_INFO_...");

    }
    NOTREACHED();
}


/** \brief Initialize the info plugin.
 *
 * This function is used to initialize the info plugin object.
 */
info::info()
    //: f_snap(nullptr) -- auto-init
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
void info::on_bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(info, "server", server, improve_signature, _1, _2);
    SNAP_LISTEN(info, "editor", editor::editor, finish_editor_form_processing, _1, _2);
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
info * info::instance()
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

    SNAP_PLUGIN_UPDATE(2015, 9, 8, 16, 30, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our info references.
 *
 * Send our info to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void info::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

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
 * \param[in] ipath  The canonicalized path being managed.
 *
 * \return true if the content is properly generated, false otherwise.
 */
bool info::on_path_execute(content::path_info_t & ipath)
{
    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

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
 * \param[in] ctemplate  A template used when the other parameters are
 *                       not available.
 */
void info::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body, QString const & ctemplate)
{
    // our settings pages are like any standard pages
    output::output::instance()->on_generate_main_content(ipath, page, body, ctemplate);
}


void info::on_finish_editor_form_processing(content::path_info_t & ipath, bool & succeeded)
{
    if(!succeeded
    || ipath.get_cpath() != "admin/settings/info")
    {
        return;
    }

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    QtCassandra::QCassandraRow::pointer_t settings_row(revision_table->row(ipath.get_revision_key()));

    QtCassandra::QCassandraValue value;

    value = settings_row->cell(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_NAME))->value();
    f_snap->set_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_NAME), value);

    value = settings_row->cell(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_LONG_NAME))->value();
    f_snap->set_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_LONG_NAME), value);

    value = settings_row->cell(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_SHORT_NAME))->value();
    f_snap->set_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_SITE_SHORT_NAME), value);
}


/** \brief Improves the error signature.
 *
 * This function adds a link to the administration page to the signature of
 * die() errors. This is done only if the user is logged in and has enough
 * rights to access administrative pages.
 *
 * \param[in] path  The path on which the error occurs.
 * \param[in,out] signature  The HTML signature to improve.
 */
void info::on_improve_signature(QString const & path, QString & signature)
{
    static_cast<void>(path);

    // only check if user is logged in
    if(users::users::instance()->user_is_logged_in())
    {
        // only show the /admin link if the user can go there
        permissions::permissions *permissions_plugin(permissions::permissions::instance());
        QString const & login_status(permissions_plugin->get_login_status());
        content::path_info_t page_ipath;
        page_ipath.set_path("/admin");
        content::permission_flag allowed;
        path::path::instance()->access_allowed(permissions_plugin->get_user_path(), page_ipath, "administer", login_status, allowed);
        if(allowed.allowed())
        {
            // TODO: translate
            signature += " <a href=\"/admin\" target=\"_top\">Administration</a>";
        }
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
