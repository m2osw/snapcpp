// Snap Websites Server -- manage the snapmanager.cgi and snapmanagerdaemon
// Copyright (C) 2016  Made to Order Software Corp.
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

#include "self.h"

#include "lib/form.h"

#include "join_strings.h"
#include "log.h"
#include "not_reached.h"
#include "not_used.h"
#include "qdomhelpers.h"

#include <QFile>

#include "poison.h"


SNAP_PLUGIN_START(self, 1, 0)


/** \brief Get a fixed cpu plugin name.
 *
 * The cpu plugin makes use of different fixed names. This function
 * ensures that you always get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_SNAPMANAGERCGI_SELF_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_SELF_...");

    }
    NOTREACHED();
}




/** \brief Initialize the self plugin.
 *
 * This function is used to initialize the self plugin object.
 */
self::self()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the self plugin.
 *
 * Ensure the self object is clean before it is gone.
 */
self::~self()
{
}


/** \brief Get a pointer to the self plugin.
 *
 * This function returns an instance pointer to the self plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the self plugin.
 */
self * self::instance()
{
    return g_plugin_self_factory.instance();
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
QString self::description() const
{
    return "Manage the snapmanager.cgi and snapmanagerdaemon settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString self::dependencies() const
{
    return "|server|";
}


/** \brief Check whether updates are necessary.
 *
 * This function is ignored in snapmanager.cgi and snapmanagerdaemon plugins.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t self::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize self.
 *
 * This function terminates the initialization of the self plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void self::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(self, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void self::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    {
        snap_manager::status_t const up(snap_manager::status_t::state_t::STATUS_STATE_INFO, get_plugin_name(), "status", "up");
        server_status.set_field(up);
    }

    {
        snap_manager::status_t const ip(snap_manager::status_t::state_t::STATUS_STATE_INFO, get_plugin_name(), "ip", f_snap->get_public_ip());
        server_status.set_field(ip);
    }

    {
        snap::snap_string_list const & frontend_servers(f_snap->get_snapmanager_frontend());
        snap_manager::status_t const frontend(
                    frontend_servers.empty()
                            ? snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            : snap_manager::status_t::state_t::STATUS_STATE_INFO,
                    get_plugin_name(),
                    "snapmanager_frontend",
                    frontend_servers.join(","));
        server_status.set_field(frontend);
    }

    {
        std::vector<std::string> const & bundle_uri(f_snap->get_bundle_uri());
        snap_manager::status_t const bundle(
                    bundle_uri.empty()
                            ? snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            : snap_manager::status_t::state_t::STATUS_STATE_INFO,
                    get_plugin_name(),
                    "bundle_uri",
                    QString::fromUtf8(snap::join_strings(bundle_uri, ",").c_str()));
        server_status.set_field(bundle);
    }
}


/** \brief Transform a value to HTML for display.
 *
 * This function expects the name of a field and its value. It then adds
 * the necessary HTML to the specified element to display that value.
 *
 * If the value is editable, then the function creates a form with the
 * necessary information (hidden fields) to save the data as required
 * by that field (i.e. update a .conf/.xml file, create a new file,
 * remove a file, etc.)
 *
 * \param[in] server_status  The map of statuses.
 * \param[in] s  The field being worked on.
 *
 * \return true if we handled this field.
 */
bool self::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "snapmanager_frontend")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "List of Front End Servers"
                        , s.get_field_name()
                        , s.get_value()
                        , QString("This is a list of Front End servers that accept requests to snapmanager.cgi. Only the few computers that accept such request need to be named here. Names are expected to be comma separated.%1")
                                .arg(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING
                                    ? " <span style=\"color: red;\">The Warning Status is due to the fact that the list on this computer is currently empty. If it was not defined yet, add the value. If it is defined on other servers, you may want to go on that server page and click Save Everywhere from there.</span>"
                                    : "")
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "bundle_uri")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "List of URIs to Directories of Bundles"
                        , s.get_field_name()
                        , s.get_value()
                        , QString("This is a list of comma separated URIs specifying the location of Directory Bundles. Usually, this is just one URI.")
                                .arg(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING
                                    ? " <span style=\"color: red;\">The WARNING status signals that you have not specified any such URI. Also, to be able to install any bundle on any computer, you want to have the same list of URIs on all your computers.</span>"
                                    : "")
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    return false;
}


/** \brief Save 'new_value' in field 'field_name'.
 *
 * This function saves 'new_value' in 'field_name'.
 *
 * \param[in] field_name  The name of the field to update.
 * \param[in] new_value  The new value to save in that field.
 * \param[in] old_value  The old value, just in case (usually ignored.)
 *
 * \return true if the new_value was applied successfully.
 */
bool self::apply_setting(QString const & field_name, QString const & new_value, QString const & old_value, std::vector<QString> & affected_services)
{
    snap::NOTUSED(old_value);

    bool const reset_bundle_uri(field_name == "bundle_uri");
    if(reset_bundle_uri)
    {
        // if a failure happens, we do not create the last update time
        // file, that means we will retry to read the bundles each time;
        // so deleting that file is like requesting an immediate reload
        // of the bundles
        //
        QString const reset_filename(QString("%1/bundles.reset").arg(f_snap->get_bundles_path()));
        QFile reset_file(reset_filename);
        if(!reset_file.open(QIODevice::WriteOnly))
        {
            SNAP_LOG_WARNING("failed to create the \"")(reset_filename)("\", changes to the bundles URI may not show up as expected.");
        }
    }

    if(field_name == "snapmanager_frontend"
    || reset_bundle_uri)
    {
        affected_services.push_back("snapmanagerdaemon");

        // TODO: the path to the snapmanager.conf is hard coded, it needs to
        //       use the path of the file used to load the .conf in the
        //       first place (I'm just not too sure how to get that right
        //       now, probably from the "--config" parameter, but how do
        //       we do that for each service?)
        //
        return f_snap->replace_configuration_value("/etc/snapwebsites/snapwebsites.d/snapmanager.conf", field_name, new_value);
    }

    return false;
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
