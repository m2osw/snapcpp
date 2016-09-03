// Snap Websites Server -- handle Snap! files cassandra settings
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

// cassandra
//
#include "cassandra.h"

// our lib
//
#include "lib/form.h"

// snapwebsites lib
//
#include "file_content.h"
#include "log.h"
#include "not_reached.h"
#include "not_used.h"
#include "process.h"

// C++ lib
//
#include <fstream>

// last entry
//
#include "poison.h"


SNAP_PLUGIN_START(cassandra, 1, 0)


namespace
{

char const * g_cassandra_yaml = "/etc/cassandra/cassandra.yaml";

} // no name namespace



/** \brief Get a fixed cassandra plugin name.
 *
 * The cassandra plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_CASSANDRA_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_CASSANDRA_...");

    }
    NOTREACHED();
}




/** \brief Initialize the cassandra plugin.
 *
 * This function is used to initialize the cassandra plugin object.
 */
cassandra::cassandra()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the cassandra plugin.
 *
 * Ensure the cassandra object is clean before it is gone.
 */
cassandra::~cassandra()
{
}


/** \brief Get a pointer to the cassandra plugin.
 *
 * This function returns an instance pointer to the cassandra plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the cassandra plugin.
 */
cassandra * cassandra::instance()
{
    return g_plugin_cassandra_factory.instance();
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
QString cassandra::description() const
{
    return "Handle the settings in the cassandra.yaml file.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString cassandra::dependencies() const
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
int64_t cassandra::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize cassandra.
 *
 * This function terminates the initialization of the cassandra plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void cassandra::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(cassandra, "server", snap_manager::manager, retrieve_status, _1);
    SNAP_LISTEN(cassandra, "server", snap_manager::manager, handle_affected_services, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void cassandra::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    // get the data
    //
    file_content fc(g_cassandra_yaml);
    if(fc.read_all())
    {
        std::string const & content(fc.get_content());
        retrieve_parameter(server_status, content, "cluster_name");
        retrieve_parameter(server_status, content, "seeds");
        retrieve_parameter(server_status, content, "listen_address");
        retrieve_parameter(server_status, content, "rpc_address");
        retrieve_parameter(server_status, content, "broadcast_rpc_address");
    }
    else if(fc.exists())
    {
        // create an error field which is not editable
        //
        snap_manager::status_t const conf_field(
                          snap_manager::status_t::state_t::STATUS_STATE_WARNING
                        , get_plugin_name()
                        , "cassandra_yaml"
                        , QString("\"%1\" is not editable at the moment.").arg(g_cassandra_yaml));
        server_status.set_field(conf_field);
    }
    // else -- file does not exist
}


void cassandra::retrieve_parameter(snap_manager::server_status & server_status, std::string const & content, std::string const & parameter_name)
{
    // could read the file, go on
    //
    bool found(false);
    std::string::size_type const pos(snap_manager::manager::search_parameter(content, parameter_name + ":", 0, true));

    // make sure that there is nothing "weird" before that name
    // (i.e. "rpc_address" and "broadcast_rpc_address")
    //
    if(pos != std::string::size_type(-1)
    && (pos == 0
    || content[pos - 1] == '\r'
    || content[pos - 1] == '\n'
    || content[pos - 1] == '\t'
    || content[pos - 1] == ' '))
    {
        // found it, get the value
        //
        std::string::size_type const p1(content.find_first_not_of(" \t", pos + parameter_name.length() + 1));
        if(p1 != std::string::size_type(-1))
        {
            std::string::size_type p2(content.find_first_of("\r\n", p1));
            if(p2 != std::string::size_type(-1))
            {
                std::string value;

                // trim spaces at the end
                while(p2 > p1 && isspace(content[p2 - 1]))
                {
                    --p2;
                }
                if(content[p1] == '"' || content[p1] == '\''
                && p2 - 1 > p1
                && content[p2 - 1] == content[p1])
                {
                    // remove quotation (this is random in this configuration file)
                    //
                    value = content.substr(p1 + 1, p2 - p1 - 2);
                }
                else
                {
                    value = content.substr(p1, p2 - p1);
                }

                snap_manager::status_t const conf_field(
                                  snap_manager::status_t::state_t::STATUS_STATE_INFO
                                , get_plugin_name()
                                , QString::fromUtf8(parameter_name.c_str())
                                , QString::fromUtf8(value.c_str()));
                server_status.set_field(conf_field);

                found = true;
            }
        }
    }

    if(!found)
    {
        // we got the file, but could not find the field as expected
        //
        snap_manager::status_t const conf_field(
                          snap_manager::status_t::state_t::STATUS_STATE_WARNING
                        , get_plugin_name()
                        , QString::fromUtf8(parameter_name.c_str())
                        , QString("\"%1\" is not defined in \"%2\".").arg(parameter_name.c_str()).arg(g_cassandra_yaml));
        server_status.set_field(conf_field);
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
 * \param[in,out] parent  The parent element where we add the fields.
 * \param[in] s  The field being worked on.
 * \param[in] uri  The URI to send the POST.
 *
 * \return true if we handled this field.
 */
bool cassandra::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "cluster_name")
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the cluster name
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        QString const user_name(s.get_field_name().mid(8));
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra 'ClusterName'"
                        , s.get_field_name()
                        , s.get_value()
                        , "The name of the Cassandra cluster. All your Cassandra Nodes must be using the exact same name or they won't be able to join the cluster."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "seeds")
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the list of seeds
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        QString const user_name(s.get_field_name().mid(8));
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra Seeds"
                        , s.get_field_name()
                        , s.get_value()
                        , "This is a list of comma separated IP addresses representing Cassandra seeds."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "listen_address")
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the list of seeds
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        QString const user_name(s.get_field_name().mid(8));
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra Listen Address"
                        , s.get_field_name()
                        , s.get_value()
                        , "This is the Private IP Address of this computer, which Cassandra listens on for of Cassandra node connections."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "rpc_address")
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the list of seeds
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        QString const user_name(s.get_field_name().mid(8));
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra RPC Address"
                        , s.get_field_name()
                        , s.get_value()
                        , "Most often, this is the Private IP Address of this computer, which Cassandra listens on for client connections. It is possible to set this address to 0.0.0.0 to listen for connections from anywhere. However, that is not considered safe and by default the firewall blocks the Cassandra port."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "broadcast_rpc_address")
    {
        // in case it is not marked as INFO, it is "not editable" (we are
        // unsure of the current file format)
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_WARNING)
        {
            return false;
        }

        // the broadcast RCP address
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        QString const user_name(s.get_field_name().mid(8));
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra Broadcast RPC Address"
                        , s.get_field_name()
                        , s.get_value()
                        , "This is the Private IP Address of this computer, which Cassandra uses to for broadcast information between Cassandra nodes and client connections."
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
 *
 * \return true if the new_value was applied successfully.
 */
bool cassandra::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);

    bool const use_default(button_name == "restore_default");

    if(field_name == "cluster_name")
    {
        affected_services.insert("cassandra-restart");

        f_snap->replace_configuration_value(
                      g_cassandra_yaml
                    , field_name
                    , new_value
                    ,   snap_manager::REPLACE_CONFIGURATION_VALUE_COLON
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_CREATE_BACKUP
                    );
        return true;
    }

    if(field_name == "seeds")
    {
        affected_services.insert("cassandra-restart");

        f_snap->replace_configuration_value(
                      g_cassandra_yaml
                    , field_name
                    , new_value
                    ,   snap_manager::REPLACE_CONFIGURATION_VALUE_COLON
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_DOUBLE_QUOTE
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_CREATE_BACKUP
                    );
        return true;
    }

    if(field_name == "listen_address")
    {
        affected_services.insert("cassandra-restart");

        f_snap->replace_configuration_value(
                      g_cassandra_yaml
                    , field_name
                    , use_default ? "localhost" : new_value
                    ,   snap_manager::REPLACE_CONFIGURATION_VALUE_COLON
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_CREATE_BACKUP
                    );
        return true;
    }

    if(field_name == "rpc_address")
    {
        affected_services.insert("cassandra-restart");

        f_snap->replace_configuration_value(
                      g_cassandra_yaml
                    , field_name
                    , use_default ? "localhost" : new_value
                    ,   snap_manager::REPLACE_CONFIGURATION_VALUE_COLON
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_CREATE_BACKUP
                    );
        return true;
    }

    if(field_name == "broadcast_rpc_address")
    {
        affected_services.insert("cassandra-restart");

        f_snap->replace_configuration_value(
                      g_cassandra_yaml
                    , field_name
                    , use_default ? "localhost" : new_value
                    ,   snap_manager::REPLACE_CONFIGURATION_VALUE_COLON
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_HASH_COMMENT
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_CREATE_BACKUP
                    );
        return true;
    }

    return false;
}



void cassandra::on_handle_affected_services(std::set<QString> & affected_services)
{
    bool restarted(false);

    auto const & it_restart(std::find(affected_services.begin(), affected_services.end(), QString("cassandra-restart")));
    if(it_restart != affected_services.end())
    {
        // remove since we are handling that one here
        //
        affected_services.erase(it_restart);

        // restart cassandra
        //
        // the stop can be extremely long and because of that, a
        // system restart does not always work correctly so we have
        // our own tool to restart cassandra
        //
        snap::process p("restart cassandra");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
        p.set_command("snaprestartcassandra");
        NOTUSED(p.run());           // errors are automatically logged by snap::process

        restarted = true;
    }

    auto const & it_reload(std::find(affected_services.begin(), affected_services.end(), QString("cassandra-reload")));
    if(it_reload != affected_services.end())
    {
        // remove since we are handling that one here
        //
        affected_services.erase(it_reload);

        // do the reload only if we did not already do a restart (otherwise
        // it is going to be useless)
        //
        if(!restarted)
        {
            // reload cassandra
            //
            snap::process p("reload cassandra");
            p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
            p.set_command("systemctl");
            p.add_argument("reload");
            p.add_argument("cassandra");
            NOTUSED(p.run());           // errors are automatically logged by snap::process
        }
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
