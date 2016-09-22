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
#include "snapmanager/form.h"

// snapwebsites lib
//
#include <snapwebsites/chownnm.h>
#include <snapwebsites/file_content.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/process.h>

// Qt lib
//
#include <QDir>
#include <QFile>
#include <QTextStream>

// C++ lib
//
#include <fstream>
#include <sys/stat.h>

// last entry
//
#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(cassandra, 1, 0)


namespace
{

const QString g_ssl_keys_dir        = "/etc/cassandra/ssl/";
const QString g_cassandra_yaml      = "/etc/cassandra/cassandra.yaml";
const QString g_keystore_password   = "qZ0LK74eiPecWcTQJCX2";
const QString g_truststore_password = "fu87kxWq4ktrkuZqVLQX";


class cassandra_info
{
public:
    cassandra_info()
        : f_cassandra_configuration(g_cassandra_yaml.toUtf8().data())
    {
    }

    bool read_configuration()
    {
        // try reading only once
        //
        if(!f_read)
        {
            f_read = true;
            if(f_cassandra_configuration.read_all())
            {
                f_valid = true;
            }
        }

        return f_valid;
    }

    bool exists()
    {
        return f_cassandra_configuration.exists();
    }

    std::string retrieve_parameter 
        ( bool & found
        , std::string const & parameter_name
        , std::string const & section_name = std::string()
        )
    {
        found = false;

        // get the file content in a string reference
        //
        std::string const & content = f_cassandra_configuration.get_content();

        std::string::size_type const section_pos = section_name.empty()
            ? 0
            : snap_manager::manager::search_parameter( content, section_name + ":", 0, true )
            ;

        // search for the parameter
        //
        std::string::size_type const pos(snap_manager::manager::search_parameter(content, parameter_name + ":", section_pos, true));

        std::string::size_type const start_of_line = section_name.empty()
            ? 0
            : 4
            ;

        // make sure that there is nothing "weird" before that name
        // (i.e. "rpc_address" and "broadcast_rpc_address")
        //
        if(pos != std::string::size_type(-1)
        && (pos == start_of_line
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
                    found = true;

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
                        return content.substr(p1 + 1, p2 - p1 - 2);
                    }
                    else
                    {
                        return content.substr(p1, p2 - p1);
                    }
                }
            }
        }

        return std::string();
    }

private:
    file_content        f_cassandra_configuration;
    bool                f_read = false;
    bool                f_valid = false;
};


void create_field(snap_manager::server_status & server_status, cassandra_info & info, QString const & plugin_name, std::string const & parameter_name)
{
    bool found(false);
    std::string const value(info.retrieve_parameter(found, parameter_name));
    if(found)
    {
        snap_manager::status_t const conf_field(
                          snap_manager::status_t::state_t::STATUS_STATE_INFO
                        , plugin_name
                        , QString::fromUtf8(parameter_name.c_str())
                        , QString::fromUtf8(value.c_str()));
        server_status.set_field(conf_field);

    }
    else
    {
        // we got the file, but could not find the field as expected
        //
        snap_manager::status_t const conf_field(
                          snap_manager::status_t::state_t::STATUS_STATE_WARNING
                        , plugin_name
                        , QString::fromUtf8(parameter_name.c_str())
                        , QString("\"%1\" is not defined in \"%2\".").arg(parameter_name.c_str()).arg(g_cassandra_yaml));
        server_status.set_field(conf_field);
    }
}




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

    SNAP_LISTEN  ( cassandra, "server", snap_manager::manager, retrieve_status,          _1     );
    SNAP_LISTEN  ( cassandra, "server", snap_manager::manager, handle_affected_services, _1     );
    SNAP_LISTEN  ( cassandra, "server", snap_manager::manager, add_plugin_commands,      _1     );
    SNAP_LISTEN  ( cassandra, "server", snap_manager::manager, process_plugin_message,   _1, _2 );
    SNAP_LISTEN0 ( cassandra, "server", snap_manager::manager, communication_ready              );
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
    cassandra_info info;
    if(info.read_configuration())
    {
        create_field(server_status, info, get_plugin_name(), "cluster_name"         );
        create_field(server_status, info, get_plugin_name(), "seeds"                );
        create_field(server_status, info, get_plugin_name(), "listen_address"       );
        create_field(server_status, info, get_plugin_name(), "rpc_address"          );
        create_field(server_status, info, get_plugin_name(), "broadcast_rpc_address");
        create_field(server_status, info, get_plugin_name(), "auto_snapshot"        );

        // also add a "join a cluster" field
        //
        // TODO: add the field ONLY if the node does not include a
        //       snap_websites context!
        {
            snap_manager::status_t const conf_field(
                              snap_manager::status_t::state_t::STATUS_STATE_INFO
                            , get_plugin_name()
                            , "join_a_cluster"
                            , "");
            server_status.set_field(conf_field);
        }

        // if joined, we want the user to be able to change the replication factor
        //
        {
            QString const replication_factor(get_replication_factor());
            // TBD: if replication_factor.isEmpty() do not show?
            snap_manager::status_t const conf_field(
                              snap_manager::status_t::state_t::STATUS_STATE_INFO
                            , get_plugin_name()
                            , "replication_factor"
                            , replication_factor);
            server_status.set_field(conf_field);
        }

        // Present the server SSL option (to allow node-to-node encryption).
        //
        bool found(false);
        std::string const use_server_ssl( info.retrieve_parameter( found, "internode_encryption", "server_encryption_options" ) );
        if(found)
        {
            snap_manager::status_t const conf_field(
                              snap_manager::status_t::state_t::STATUS_STATE_INFO
                            , get_plugin_name()
                            , "use_server_ssl"
                            , use_server_ssl.c_str());
            server_status.set_field(conf_field);
        }

        // Present the clien SSL option (to allow client-to-server encryption).
        //
        found =false;
        std::string const use_client_ssl( info.retrieve_parameter( found, "enabled", "client_encryption_options" ) );
        if(found)
        {
            snap_manager::status_t const conf_field(
                              snap_manager::status_t::state_t::STATUS_STATE_INFO
                            , get_plugin_name()
                            , "use_client_ssl"
                            , use_client_ssl.c_str());
            server_status.set_field(conf_field);
        }
    }
    else if(info.exists())
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
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

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
                  | snap_manager::form::FORM_BUTTON_SAVE
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

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

    if(s.get_field_name() == "auto_snapshot")
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
                  | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Cassandra Auto-Snapshot"
                        , s.get_field_name()
                        , s.get_value()
                        , "Cassandra says that you should set this parameter to \"true\"."
                         " However, when to true, the DROP TABLE and TRUNCATE commands"
                         " become extremely slow because the database creates a snapshot"
                         " of the table before dropping or truncating it. We change this"
                         " parameter to \"false\" by default because if you DROP TABLE or"
                         " TRUNCATE by mistake, you probably have a bigger problem."
                         " Also, we offer a \"snapbackup\" tool which should be more than"
                         " enough to save all the data from all the tables. And somehow,"
                         " \"snapbackup\" goes a huge whole lot faster. (although if you"
                         " start having a really large database, you could end up not"
                         " being able to use \"snapbackup\" at all... once you reach"
                         " that limit, you may want to turn the auto_snapshot feature"
                         " back on."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "join_a_cluster")
    {
        // the name of the computer to connect to
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        // TODO: get the list of names and show as a dropdown
        //
        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Enter the server_name of the computer to join:"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>The <code>server_name</code> parameter is used to contact that specific server, get the"
                         " Cassandra node information from that server, and then add the Cassandra"
                         " node running on this computer to the one on that other computer.</p>"
                         "<p><strong>WARNING:</strong> There is currently no safeguard for this"
                         " feature. The computer will proceed and possibly destroy some of your"
                         " data in the process if this current computer node is not a new node."
                         " If you have a replication factor larger than 1, then it should be okay.<p>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "replication_factor")
    {
        // the replication factor for Cassandra
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Enter the replication factor (RF):"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>By default we create the Snap! cluster with a replication factor of 1"
                         " (since you need 2 or more nodes to have a higher replication factor...)"
                         " This option let you change the factor. It must be run on a computer with"
                         " a Cassandra node. Make sure you do not enter a number larger than the"
                         " total number of nodes or your cluster will be stuck.<p>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "use_server_ssl")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Turn on server-to-server encryption (none, all, dc:<name>, rack:<name>):"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>By default, Cassandra communicates in the clear on the listening address."
                          " When you change this option to anything except 'none', 'server to server'' encryption will be turned on between"
                          " nodes. Also, if it is not already created, a server key pair will be created also,"
                          " and the trusted keys will be exchanged with each node on the network.<p>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "use_client_ssl")
    {
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Turn on client-to-server encryption (true or false):"
                        , s.get_field_name()
                        , s.get_value()
                        , "<p>By default, Cassandra communicates in the clear on the listening address."
                          " When you turn on this flag, client to server encryption will be turned on between"
                          " clients and nodes. If it is not already present, a trusted client key will be generated."
                          " <i>snapdbproxy</i> will then query the nodes it's connected to and request the keys.<p>"
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    return false;
}


void cassandra::set_server_ssl( bool const enabled )
{
    // Make a backup before we modify this extensively.
    //
    QFile yaml_file( g_cassandra_yaml );
    yaml_file.copy( g_cassandra_yaml + ".bak" );

    f_snap->replace_configuration_value(
                  g_cassandra_yaml
                , "server_encryption_options::internode_encryption"
                , enabled? "all": "none"
                ,   snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                );

    f_snap->replace_configuration_value(
                  g_cassandra_yaml
                , "server_encryption_options::keystore"
                , "/etc/cassandra/ssl/keystore.jks"
                ,   snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                );

    f_snap->replace_configuration_value(
                  g_cassandra_yaml
                , "server_encryption_options::keystore_password"
                , g_keystore_password
                ,   snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                );

    f_snap->replace_configuration_value(
                  g_cassandra_yaml
                , "server_encryption_options::truststore"
                , "/etc/cassandra/ssl/truststore.jks"
                ,   snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                );

    f_snap->replace_configuration_value(
                  g_cassandra_yaml
                , "server_encryption_options::truststore_password"
                , g_truststore_password
                ,   snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                );
}


void cassandra::set_client_ssl( bool const enabled )
{
    f_snap->replace_configuration_value(
                  g_cassandra_yaml
                , "client_encryption_options::enabled"
                , enabled? "true": "false"
                ,   snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                );

    f_snap->replace_configuration_value(
                  g_cassandra_yaml
                , "client_encryption_options::optional"
                , "false"
                ,   snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                );

    f_snap->replace_configuration_value(
                  g_cassandra_yaml
                , "client_encryption_options::keystore"
                , "/etc/cassandra/ssl/keystore.jks"
                ,   snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                );

    f_snap->replace_configuration_value(
                  g_cassandra_yaml
                , "client_encryption_options::keystore_password"
                , g_keystore_password
                ,   snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                );

    f_snap->replace_configuration_value(
                  g_cassandra_yaml
                , "client_encryption_options::truststore"
                , "/etc/cassandra/ssl/truststore.jks"
                ,   snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                );

    f_snap->replace_configuration_value(
                  g_cassandra_yaml
                , "client_encryption_options::truststore_password"
                , g_truststore_password
                ,   snap_manager::REPLACE_CONFIGURATION_VALUE_SECTION
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_SINGLE_QUOTE
                  | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                );
}


void cassandra::generate_keys()
{
    cassandra_info info;
    //
    // check whether the configuration file exists, if not then do not
    // bother, Cassandra is not even installed
    //
    if( !info.read_configuration() )
    {
        SNAP_LOG_ERROR("Cannot read Cassandra configuration! Not generating keys!");
        return;
    }

    bool found = false;
    std::string const listen_address( info.retrieve_parameter(found, "listen_address") );
    if( !found )
    {
        SNAP_LOG_ERROR("'listen_address' is not defined in your cassandra.yaml! Cannot generate keys!");
        return;
    }

    char const * sd = "/etc/cassandra/ssl";
    QDir ssl_dir(sd);
    if( ssl_dir.exists() )
    {
        SNAP_LOG_TRACE("/etc/cassandra/ssl already exists, so we do nothing.");
        return;
    }

    // Create the directory, make sure it's in the snapwebsites group,
    // and make it so we have full access to it, but nothing for the rest
    // of the world.
    //
    ssl_dir.mkdir( sd );
    snap::chownnm( sd, "root", "snapwebsites" );
    ::chmod( sd, 0770 );

    // Now generate the keys...
    //
    QStringList command_list;
    command_list << QString
       (
       "keytool -noprompt -genkeypair -keyalg RSA "
       " -alias      node"
       " -validity   36500"
       " -keystore   %1/keystore.jks"
       " -storepass  %2"
       " -keypass    %3"
       " -dname \"CN=%4, OU=Cassandra Backend, O=Made To Order Software Corp, L=Orangevale, ST=California, C=US\""
       )
          .arg(ssl_dir.path())
          .arg(g_truststore_password)
          .arg(g_keystore_password)
          .arg(listen_address.c_str())
          ;

     command_list << QString
       (
       "keytool -export -alias node"
       " -file %1/node.cer"
       " -keystore %1/keystore.jks"
       )
          .arg(ssl_dir.path())
       ;

    command_list << QString
       (
       "keytool -import -v -trustcacerts "
       " -alias node"
       " -file %1/node.cer"
       " -keystore %1/truststore.jks"
       )
          .arg(ssl_dir.path())
       ;


    command_list << QString
       (
       "keytool -exportcert -rfc -noprompt"
       " -alias node"
       " -keystore %1/keystore.jks"
       " -storepass %2"
       " -file %1/client.pem"
       )
          .arg(ssl_dir.path())
          .arg(g_truststore_password)
       ;

    for( auto const& cmd : command_list )
    {
        if( system( cmd.toUtf8().data() ) != 0 )
        {
            SNAP_LOG_ERROR("Cannot execute command '")(qPrintable(cmd))("'!");
        }
    }
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

    if(field_name == "auto_snapshot")
    {
        affected_services.insert("cassandra-restart");

        f_snap->replace_configuration_value(
                      g_cassandra_yaml
                    , field_name
                    , use_default ? "false" : new_value
                    ,   snap_manager::REPLACE_CONFIGURATION_VALUE_COLON
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_SPACE_AFTER
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_MUST_EXIST
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_HASH_COMMENT
                      | snap_manager::REPLACE_CONFIGURATION_VALUE_CREATE_BACKUP
                    );
        return true;
    }

    if(field_name == "join_a_cluster")
    {
        if(new_value == f_snap->get_server_name())
        {
            SNAP_LOG_ERROR("trying to join yourself (\"")(new_value)("\") is not going to work.");
        }
        else if(f_joining)
        {
            SNAP_LOG_ERROR("trying to join when you already ran that process. If it failed, restart snapmanagerdaemon and try again.");
        }
        else
        {
            f_joining = true;

            snap::snap_communicator_message cassandra_query;
            cassandra_query.set_server(new_value);
            cassandra_query.set_service("snapmanagerdaemon");
            cassandra_query.set_command("CASSANDRAQUERY");
            get_cassandra_info(cassandra_query);
            f_snap->forward_message(cassandra_query);
        }

        return true;
    }

    if(field_name == "replication_factor")
    {
        set_replication_factor(new_value);
        return true;
    }

    if( field_name == "use_server_ssl" )
    {
        // Modify values and generate keys if enabled for server_encryption_options.
        // Disable if user turns them off.
        set_server_ssl( new_value != "none" );
        return true;
    }

    if( field_name == "use_client_ssl" )
    {
        // Modify values and generate keys if enabled for client_encryption_options.
        // Disable if user turns them off.
        set_client_ssl( new_value == "enabled" );
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


void cassandra::on_communication_ready()
{
    // now we can broadcast our CASSANDRAQUERY so we have
    // information about all our accomplices
    //
    // IMPORTANT: this won't work properly if all the other nodes are
    //            not yet fired up; for that reason the CASSANDRAQUERY
    //            includes the information that the CASSANDRAFIELDS
    //            reply includes because that way we avoid re-sending
    //            the message when we later receive a CASSANDRAQUERY
    //            message from a node that just woke up
    //
    // TODO:
    // At this point, I am thinking we should not send this message
    // until later enough so we know whether Cassandra started and
    // whether the context is defined or not... but I'm not implementing
    // that now.
    //
    //snap::snap_communicator_message cassandra_query;
    //cassandra_query.set_service("*");
    //cassandra_query.set_command("CASSANDRAQUERY");
    //get_cassandra_info(cassandra_query);
    //f_snap->forward_message(cassandra_query);

    // Request all of the server keys from all of the nodes
    //
    snap::snap_communicator_message cassandra_query;
    cassandra_query.set_service("*");
    cassandra_query.set_command("CASSANDRASERVERKEYS");
    get_cassandra_info(cassandra_query);
    f_snap->forward_message(cassandra_query);

    // Make sure server keys are generated.
    //
    generate_keys();
}


void cassandra::on_add_plugin_commands(snap::snap_string_list & understood_commands)
{
    understood_commands << "CASSANDRAQUERY";
    understood_commands << "CASSANDRAFIELDS";
    understood_commands << "CASSANDRAKEYS";         // Send our public key to the requesting server...
    understood_commands << "CASSANDRASERVERKEYS";   // Send our node key to the requesting server...
}


void cassandra::on_process_plugin_message(snap::snap_communicator_message const & message, bool & processed)
{
    QString const command(message.get_command());

    if(command == "CASSANDRAFIELDS")
    {
        //QString const server(message.get_sent_from_server());

        //
        // WARNING: Right now we assume that this reply is directly
        //          a reply to a CASSANDRAQUERY we sent to a specific
        //          computer and as a result we JOIN that other computer
        //          Cassandra cluster... We still have a flag, to make
        //          sure we are in the correct state, but as we want
        //          to implement a CASSANDRAQUERY that gets broadcast
        //          we may need to fix up the algorithm quite a bit
        //          (and actually the join won't require sending the
        //          CASSANDRAQUERY because we should already have the
        //          information anyway...)
        //

        if(f_joining)
        {
            join_cassandra_node(message);
            f_joining = false;
        }

        processed = true;
    }
    else if(command == "CASSANDRAQUERY")
    {
        // reply with a CASSANDRAINFO directly to the computer that
        // asked for it
        //
        snap::snap_communicator_message cassandra_status;
        cassandra_status.reply_to(message);
        cassandra_status.set_command("CASSANDRAFIELDS");
        get_cassandra_info(cassandra_status);
        f_snap->forward_message(cassandra_status);

        processed = true;
    }
    else if( command == "CASSANDRAKEYS" )
    {
        // A client requested the public key for authentication.
        //
        QFile file( g_ssl_keys_dir + "client.pem" );
        if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            QTextStream in(&file);
            //
            snap::snap_communicator_message cmd;
            cmd.reply_to(message);
            cmd.set_command("CASSANDRAKEY");
            cmd.add_parameter( "key"   , in.readAll()    );
            cmd.add_parameter( "cache" , "ttl=60"        );
            get_cassandra_info(cmd);
            f_snap->forward_message(cmd);
        }
        else
        {
            QString const errmsg(QString("Cannot open '%1' for reading!").arg(file.fileName()));
            SNAP_LOG_ERROR(qPrintable(errmsg));
            //throw vpn_exception( errmsg );
        }
    }
    else if( command == "CASSANDRASERVERKEY" )
    {
        // Open the file...
        QString const full_path( QString("%1%2.pem")
                                 .arg(g_ssl_keys_dir)
                                 .arg(message.get_parameter("listen_address"))
                                 );
        QFile file( full_path );
        if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        {
            QString const errmsg = QString("Cannot open '%1' for writing!").arg(file.fileName());
            SNAP_LOG_ERROR(qPrintable(errmsg));
            return;
        }
        //
        // ...and stream the file out to disk so we have the node key for
        // node-to-node SSL connections.
        //
        QTextStream out( &file );
        out << message.get_parameter("key");
    }
    else if( command == "CASSANDRASERVERKEYS" )
    {
        // Send the node key for the requesting peer.
        //
        QFile file( g_ssl_keys_dir + "node.cer" );
        if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            QTextStream in(&file);
            //
            snap::snap_communicator_message cmd;
            cmd.reply_to(message);
            cmd.set_command("CASSANDRASERVERKEY");
            cmd.add_parameter( "key"   , in.readAll()    );
            cmd.add_parameter( "cache" , "ttl=60"        );
            get_cassandra_info(cmd);
            f_snap->forward_message(cmd);
        }
        else
        {
            QString const errmsg(QString("Cannot open '%1' for reading!").arg(file.fileName()));
            SNAP_LOG_ERROR(qPrintable(errmsg));
            //throw vpn_exception( errmsg );
        }
    }
}


void cassandra::get_cassandra_info(snap::snap_communicator_message & status)
{
    cassandra_info info;

    // check whether the configuration file exists, if not then do not
    // bother, Cassandra is not even installed
    //
    struct stat st;
    if(stat("/usr/sbin/cassandra", &st) != 0
    || !info.read_configuration())
    {
        status.add_parameter("status", "not-installed");
        return;
    }

    status.add_parameter("status", "installed");

    // if installed we want to include the "cluster_name" and "seeds"
    // parameters
    //
    bool found(false);
    std::string const cluster_name(info.retrieve_parameter(found, "cluster_name"));
    if(found)
    {
        status.add_parameter("cluster_name", QString::fromUtf8(cluster_name.c_str()));
    }

    found = false;
    std::string const seeds(info.retrieve_parameter(found, "seeds"));
    if(found)
    {
        status.add_parameter("seeds", QString::fromUtf8(seeds.c_str()));
    }

    // Add listen_address as well, so we can know what IP to use
    //
    found = false;
    std::string const listen_address(info.retrieve_parameter(found, "listen_address"));
    if(found)
    {
        status.add_parameter("listen_address", QString::fromUtf8(listen_address.c_str()));
    }

#if 0
    // the following actually requires polling for the port...
    //
    // i.e. we cannot assume that cassandra is not running just because
    // we cannot yet connect to the port...
    //
    // polling can be done without connecting by reading the /proc/net/tcp
    // file and analyzing the data (the field names are defined on the
    // first line)
    //
    // There is the code from netstat.c which parses one of those lines:
    //
    //   num = sscanf(line,
    //   "%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X %lX:%lX %X:%lX %lX %d %d %ld %512s\n",
    //       &d, local_addr, &local_port, rem_addr, &rem_port, &state,
    //       &txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout, &inode, more);
    //
    try
    {
        // if the connection fails, we cannot have either of the following
        // fields so the catch() makes sure to avoid them
        //
        int port(9042);
        QString const port_str(snap_dbproxy_conf["cassandra_port"]);
        if(!port_str.isEmpty())
        {
            bool ok(false);
            port = port_str.toInt(&ok, 10);
            if(!ok
            || port <= 0
            || port > 65535)
            {
                SNAP_LOG_ERROR( "Invalid cassandra_port specification in snapdbproxy.conf (invalid number, smaller than 1 or larger than 65535)" );
                port = 0;
            }
        }
        if(port > 0)
        {
            auto session( QtCassandra::QCassandraSession::create() );
            session->connect(snap_dbproxy_conf["cassandra_host_list"], port);
            if( !session->isConnected() )
            {
                SNAP_LOG_WARNING( "Cannot connect to cassandra host! Check cassandra_host_list and cassandra_port in snapdbproxy.conf!" );
            }
            else
            {
                auto meta( QtCassandra::QCassandraSchema::SessionMeta::create( session ) );
                meta->loadSchema();
                auto const & keyspaces( meta->getKeyspaces() );
                QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
                if( keyspaces.find(context_name) == std::end(keyspaces) )
                {
                    // no context yet, offer to create the context
                    //
                    snap_manager::status_t const create_context(
                                  snap_manager::status_t::state_t::STATUS_STATE_INFO
                                , get_plugin_name()
                                , "cassandra_create_context"
                                , context_name
                                );
                    server_status.set_field(create_context);
                }
                else
                {
                    // database is available and context is available,
                    // offer to create the tables (it should be automatic
                    // though, but this way we can click on this one last
                    // time before installing a website)
                    //
                    snap_manager::status_t const create_tables(
                                  snap_manager::status_t::state_t::STATUS_STATE_INFO
                                , get_plugin_name()
                                , "cassandra_create_tables"
                                , context_name
                                );
                    server_status.set_field(create_tables);
                }
            }
        }
    }
    catch( std::exception const & e )
    {
        SNAP_LOG_ERROR("Caught exception: ")(e.what());
    }
    catch( ... )
    {
        SNAP_LOG_ERROR("Caught unknown exception!");
    }
#endif
}


void cassandra::join_cassandra_node(snap::snap_communicator_message const & message)
{
    QString const cluster_name(message.get_parameter("cluster_name"));
    QString const seeds(message.get_parameter("seeds"));

    QString preamble("#!/bin/sh\n");

    preamble += "BUNDLE_UPDATE_CLUSTER_NAME=";
    preamble += cluster_name;
    preamble += "\n";

    preamble += "BUNDLE_UPDATE_SEEDS=";
    preamble += seeds;
    preamble += "\n";

    QFile script_original(":/manager-plugins/cassandra/join_cassandra_node.sh");
    if(!script_original.open(QIODevice::ReadOnly))
    {
        SNAP_LOG_ERROR("failed to open the join_cassandra_node.sh resouce file.");
        return;
    }
    QByteArray const original(script_original.readAll());

    QByteArray const script(preamble.toUtf8() + original);

    // Put the script in the cache and run it
    //
    // TODO: add a /scripts/ sub-directory so all scripts can be found
    //       there instead of the top directory?
    //
    QString const cache_path(f_snap->get_cache_path());
    std::string const script_filename(std::string(cache_path.toUtf8().data()) + "/join_cassandra_node.sh");
    snap::file_content output_file(script_filename);
    output_file.set_content(script.data());
    output_file.write_all();
    chmod(script_filename.c_str(), 0700);

    snap::process p("join cassandra node");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
    p.set_command(QString::fromUtf8(script_filename.c_str()));
    NOTUSED(p.run());           // errors are automatically logged by snap::process
}


QString cassandra::get_replication_factor()
{
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));

    // initialize the reading of the configuration file
    //
    snap::snap_config config("snapdbproxy");

    // get the list of Cassandra hosts, "127.0.0.1" by default
    //
    QString cassandra_host_list("127.0.0.1");
    if(config.has_parameter("cassandra_host_list"))
    {
        cassandra_host_list = config[ "cassandra_host_list" ];
        if(cassandra_host_list.isEmpty())
        {
            SNAP_LOG_ERROR("cassandra_host_list cannot be empty.");
            return QString();
        }
    }

    // get the Cassandra port, 9042 by default
    //
    int cassandra_port(9042);
    if(config.has_parameter("cassandra_port"))
    {
        std::size_t pos(0);
        std::string const port(config["cassandra_port"]);
        cassandra_port = std::stoi(port, &pos, 10);
        if(pos != port.length()
        || cassandra_port < 0
        || cassandra_port > 65535)
        {
            SNAP_LOG_ERROR("cassandra_port to connect to Cassandra must be defined between 0 and 65535.");
            return QString();
        }
    }

    // create a new Cassandra session
    //
    auto session( QtCassandra::QCassandraSession::create() );

    // increase the request timeout "dramatically" because creating a
    // context is very slow
    //
    // note: we do not make use of the QCassandraRequestTimeout class
    //       because we will just create the context and be done with it
    //       so there is no real need for us to restore the timeout
    //       at a later time
    //
    session->setTimeout(5 * 60 * 1000); // timeout = 5 min.

    // connect to the Cassandra cluster
    //
    try
    {
        session->connect( cassandra_host_list, cassandra_port ); // throws on failure!
        if(!session->isConnected())
        {
            // this error should not ever appear since the connect()
            // function throws on errors, but for completeness...
            //
            SNAP_LOG_ERROR("error: could not connect to Cassandra cluster.");
            return QString();
        }
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_ERROR("error: could not connect to Cassandra cluster. Exception: ")(e.what());
        return QString();
    }

    auto meta( QtCassandra::QCassandraSchema::SessionMeta::create( session ) );
    meta->loadSchema();
    auto const & keyspaces( meta->getKeyspaces() );
    auto const & context( keyspaces.find(context_name) );
    if( context == std::end(keyspaces) )
    {
        SNAP_LOG_ERROR("error: could not find \"")(context_name)("\" context in Cassandra.");
        return QString();
    }

//    auto fields(context->second->getFields());
//    for(auto f = fields.begin(); f != fields.end(); ++f)
//    {
//        SNAP_LOG_ERROR("field: ")(f->first);
//    }

    auto const & fields(context->second->getFields());
    auto const replication(fields.find("replication"));
    if(replication == fields.end())
    {
        SNAP_LOG_ERROR("error: could not find \"replication\" as one of the context fields.");
        return QString();
    }
    //QtCassandra::QCassandraSchema::Value const & value(replication->second);
//SNAP_LOG_ERROR("value type: ")(static_cast<int>(value.type()));
    //QVariant const v(value.variant());
    //QString const json(v.toString());

    QtCassandra::QCassandraSchema::Value::map_t const map(replication->second.map());
//for(auto m : map)
//{
//    SNAP_LOG_ERROR("map: [")(m.first)("] value type: ")(static_cast<int>(m.second.type()));
//}
    auto const item(map.find("dc1"));
    if(item == map.end())
    {
        SNAP_LOG_ERROR("error: could not find \"dc1\" in the context replication definition.");
        return QString();
    }

//SNAP_LOG_ERROR("got item with type: ")(static_cast<int>(item->second.type()));
//SNAP_LOG_ERROR("Value as string: ")(item->second.variant().toString());

    return item->second.variant().toString();
}


void cassandra::set_replication_factor(QString const & replication_factor)
{
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));

    // initialize the reading of the configuration file
    //
    snap::snap_config config("snapdbproxy");

    // get the list of Cassandra hosts, "127.0.0.1" by default
    //
    QString cassandra_host_list("127.0.0.1");
    if(config.has_parameter("cassandra_host_list"))
    {
        cassandra_host_list = config[ "cassandra_host_list" ];
        if(cassandra_host_list.isEmpty())
        {
            SNAP_LOG_ERROR("cassandra_host_list cannot be empty.");
            return;
        }
    }

    // get the Cassandra port, 9042 by default
    //
    int cassandra_port(9042);
    if(config.has_parameter("cassandra_port"))
    {
        std::size_t pos(0);
        std::string const port(config["cassandra_port"]);
        cassandra_port = std::stoi(port, &pos, 10);
        if(pos != port.length()
        || cassandra_port < 0
        || cassandra_port > 65535)
        {
            SNAP_LOG_ERROR("cassandra_port to connect to Cassandra must be defined between 0 and 65535.");
            return;
        }
    }

    // create a new Cassandra session
    //
    auto session( QtCassandra::QCassandraSession::create() );

    // increase the request timeout "dramatically" because creating a
    // context is very slow
    //
    // note: we do not make use of the QCassandraRequestTimeout class
    //       because we will just create the context and be done with it
    //       so there is no real need for us to restore the timeout
    //       at a later time
    //
    session->setTimeout(5 * 60 * 1000); // timeout = 5 min.

    // connect to the Cassandra cluster
    //
    try
    {
        session->connect( cassandra_host_list, cassandra_port ); // throws on failure!
        if(!session->isConnected())
        {
            // this error should not ever appear since the connect()
            // function throws on errors, but for completeness...
            //
            SNAP_LOG_ERROR("error: could not connect to Cassandra cluster.");
            return;
        }
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_ERROR("error: could not connect to Cassandra cluster. Exception: ")(e.what());
        return;
    }

    // when called here we have f_session defined but no context yet
    //
    QString query_str( QString("ALTER KEYSPACE %1").arg(context_name) );

    query_str += QString( " WITH replication = { 'class': 'NetworkTopologyStrategy', 'dc1': '%1' }" ).arg(replication_factor);

    auto query( QtCassandra::QCassandraQuery::create( session ) );
    query->query( query_str, 0 );
    //query->setConsistencyLevel( ... );
    //query->setTimestamp(...);
    //query->setPagingSize(...);
    query->start();
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
