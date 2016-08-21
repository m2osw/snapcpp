// Snap Websites Server -- manage the snapcommunicator settings
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

// communicator
//
#include "communicator.h"

// our lib
//
#include "snapmanager/form.h"

// snapwebsites lib
//
#include "join_strings.h"
#include "log.h"
#include "not_reached.h"
#include "not_used.h"
#include "qdomhelpers.h"
#include "qdomxpath.h"
#include "string_pathinfo.h"
#include "tokenize_string.h"

// Qt lib
//
#include <QFile>

// C lib
//
#include <sys/file.h>

// last entry
//
#include "poison.h"


SNAP_PLUGIN_START(communicator, 1, 0)


namespace
{

// TODO: offer the user a way to change this path?
//char const * g_service_filename = "/etc/snapwebsites/services.d/service-snapcommunicator.xml";

// TODO: get that path from the XML instead
char const * g_configuration_filename = "snapcommunicator";

// TODO: get that path from the XML instead and add the /snapwebsites.d/ part
char const * g_configuration_d_filename = "/etc/snapwebsites/snapwebsites.d/snapcommunicator.conf";


void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}


} // no name namespace



/** \brief Get a fixed communicator plugin name.
 *
 * The communicator plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_COMMUNICATOR_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_COMMUNICATOR_...");

    }
    NOTREACHED();
}




/** \brief Initialize the communicator plugin.
 *
 * This function is used to initialize the communicator plugin object.
 */
communicator::communicator()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the communicator plugin.
 *
 * Ensure the communicator object is clean before it is gone.
 */
communicator::~communicator()
{
}


/** \brief Get a pointer to the communicator plugin.
 *
 * This function returns an instance pointer to the communicator plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the communicator plugin.
 */
communicator * communicator::instance()
{
    return g_plugin_communicator_factory.instance();
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
QString communicator::description() const
{
    return "Manage the snapcommunicator settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString communicator::dependencies() const
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
int64_t communicator::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize communicator.
 *
 * This function terminates the initialization of the communicator plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void communicator::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(communicator, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void communicator::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    // TODO: find a way to get the configuration filename for snapcommunicator
    //       (i.e. take it from the XML?)
    {
        snap_config snap_communicator_conf(g_configuration_filename);

        snap_manager::status_t const my_address(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "my_address"
                    , snap_communicator_conf["my_address"]);
        server_status.set_field(my_address);

        snap_manager::status_t const neighbors(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "neighbors"
                    , snap_communicator_conf["neighbors"]);
        server_status.set_field(neighbors);
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
bool communicator::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    //if(s.get_field_name() == "disabled")
    //{
    //    // the list if frontend snapmanagers that are to receive statuses
    //    // of the cluster computers; may be just one computer; should not
    //    // be empty; shows a text input field
    //    //
    //    snap_manager::form f(
    //              get_plugin_name()
    //            , s.get_field_name()
    //            , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
    //            );

    //    snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
    //                      "Enable/Disable Firewall"
    //                    , s.get_field_name()
    //                    , s.get_value()
    //                    , "Define whether the communicator is \"enabled\" or \"disabled\"."
    //                    ));
    //    f.add_widget(field);

    //    f.generate(parent, uri);

    //    return true;
    //}

    if(s.get_field_name() == "my_address")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "The Private Network IP Address of this computer:"
                        , s.get_field_name()
                        , s.get_value()
                        , "Here you want to enter the Private Network IP Address. If you have your own private network, this is likely the eth0 or equivalent IP address. If you have OpenVPN, then it is the IP address shown in the tun0 interface (with ifconfig)."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "neighbors")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "The comma separated IP addresses of one or more neighbors:"
                        , s.get_field_name()
                        , s.get_value()
                        , "This field accepts the IP address of one or more neighbors in the same private network. WARNING: At this time we do not support cross site communication without some kind of tunnelling, and even that will probably fail because all snapcommunicators will try to connect to such IPs (so you'd have to have the tunneling available on all the machines in your cluster)."
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
 * \param[in] button_name  The name of the button the user clicked.
 * \param[in] field_name  The name of the field to update.
 * \param[in] new_value  The new value to save in that field.
 * \param[in] old_or_installation_value  The old value, just in case
 *            (usually ignored,) or the installation values (only
 *            for the self plugin that manages bundles.)
 * \param[in] affected_services  The list of services that were affected
 *            by this call.
 *
 * \return true if the new_value was applied successfully.
 */
bool communicator::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);
    NOTUSED(button_name);

    // restore defaults?
    //
    //bool const use_default_value(button_name == "restore_default");

    if(field_name == "my_address")
    {
        // this address is to connect this snapcommunicator to
        // other snapcommunicators
        //
        affected_services.insert("snapcommunicator");

        // Here we change the "my_address" and "listen" parameters because
        // the two fields are expected to have the exact same IP address in
        // nearly 100% of all cases... note that we force the port to 4040
        // because at this point we do not want to offer an end user
        // interface to deal with all the ports.
        //
        bool const success(f_snap->replace_configuration_value(g_configuration_d_filename, field_name, new_value));
        return success && f_snap->replace_configuration_value(g_configuration_d_filename, "listen", new_value + ":4040");
    }

    if(field_name == "neighbors")
    {
        // for potential new neighbors indicated in snapcommunicator
        // we have to restart it
        //
        affected_services.insert("snapcommunicator");

        return f_snap->replace_configuration_value(g_configuration_d_filename, field_name, new_value);
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
