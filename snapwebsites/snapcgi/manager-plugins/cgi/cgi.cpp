// Snap Websites Server -- manage the snapcgi settings
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

// cgi
//
#include "cgi.h"

// our lib
//
#include <snapmanager/form.h>

// snapwebsites lib
//
#include <snapwebsites/join_strings.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qdomxpath.h>
#include <snapwebsites/string_pathinfo.h>
#include <snapwebsites/tokenize_string.h>

// Qt lib
//
#include <QFile>

// C lib
//
#include <sys/file.h>

// last entry
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(cgi, 1, 0)


namespace
{

// TODO: offer the user a way to change this path?
//char const * g_service_filename = "/etc/snapwebsites/services.d/service-snapcgi.xml";

// TODO: get that path from the XML instead
char const * g_configuration_filename = "snapcgi";

// TODO: get that path from the XML instead and add the /snapwebsites.d/ part
char const * g_configuration_d_filename = "/etc/snapwebsites/snapwebsites.d/snapcgi.conf";


void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}


} // no name namespace



/** \brief Get a fixed cgi plugin name.
 *
 * The cgi plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_CGI_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_CGI_...");

    }
    NOTREACHED();
}




/** \brief Initialize the cgi plugin.
 *
 * This function is used to cgiialize the cgi plugin object.
 */
cgi::cgi()
    //: f_snap(nullptr) -- auto-cgi
{
}


/** \brief Clean up the cgi plugin.
 *
 * Ensure the cgi object is clean before it is gone.
 */
cgi::~cgi()
{
}


/** \brief Get a pointer to the cgi plugin.
 *
 * This function returns an instance pointer to the cgi plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the cgi plugin.
 */
cgi * cgi::instance()
{
    return g_plugin_cgi_factory.instance();
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
QString cgi::description() const
{
    return "Manage the snapcgi settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString cgi::dependencies() const
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
int64_t cgi::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize cgi.
 *
 * This function terminates the cgiialization of the cgi plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void cgi::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(cgi, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void cgi::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    // TODO: find a way to get the configuration filename for snapcgi
    //       (i.e. take it from the XML?)
    {
        snap_config snap_cgi(g_configuration_filename);

        snap_manager::status_t const snapserver(
                      snap_manager::status_t::state_t::STATUS_STATE_INFO
                    , get_plugin_name()
                    , "snapserver"
                    , snap_cgi["snapserver"]);
        server_status.set_field(snapserver);
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
bool cgi::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "snapserver")
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
                          "IP Address and Port (IP:Port) to connect to the snapserver service:"
                        , s.get_field_name()
                        , s.get_value()
                        , "By default this is set to 127.0.0.1:4004 as we expect that the snapserver will also be running on the server running Apache2. It is possible, though, to put snapserver on other computers for safety and increased resources. In that case, enter the Private Network IP address of a snapserver to contact. At some point, this will be a list of such IP:port, but we do not yet support such."
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
bool cgi::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);
    NOTUSED(button_name);
    NOTUSED(affected_services);

    if(field_name == "snapserver")
    {
        // fix the value in memory
        //
        snap_config snap_cgi(g_configuration_filename);
        snap_cgi["snapserver"] = new_value;

        return f_snap->replace_configuration_value(g_configuration_d_filename, "snapserver", new_value);
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
