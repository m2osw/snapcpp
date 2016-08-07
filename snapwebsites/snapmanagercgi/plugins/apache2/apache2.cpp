// Snap Websites Server -- handle Snap! files apache2 settings
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

// apache2
//
#include "apache2.h"

// our lib
//
#include "lib/form.h"

// snapwebsites lib
//
// #include "chownnm.h"
#include "log.h"
// #include "mkdir_p.h"
#include "not_reached.h"
#include "not_used.h"

// C++ lib
//
#include <fstream>

// // C lib
// //
// #include <glob.h>

// last entry
//
#include "poison.h"


SNAP_PLUGIN_START(apache2, 1, 0)


namespace
{

//void file_descriptor_deleter(int * fd)
//{
//    if(close(*fd) != 0)
//    {
//        int const e(errno);
//        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
//    }
//}



//void glob_deleter(glob_t * g)
//{
//    globfree(g);
//}
//
//int glob_error_callback(const char * epath, int eerrno)
//{
//    SNAP_LOG_ERROR("an error occurred while reading directory under \"")
//                  (epath)
//                  ("\". Got error: ")
//                  (eerrno)
//                  (", ")
//                  (strerror(eerrno))
//                  (".");
//
//    // do not abort on a directory read error...
//    return 0;
//}

} // no name namespace



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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_APACHE2_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_APACHE2_...");

    }
    NOTREACHED();
}




/** \brief Initialize the apache2 plugin.
 *
 * This function is used to initialize the apache2 plugin object.
 */
apache2::apache2()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the apache2 plugin.
 *
 * Ensure the apache2 object is clean before it is gone.
 */
apache2::~apache2()
{
}


/** \brief Get a pointer to the apache2 plugin.
 *
 * This function returns an instance pointer to the apache2 plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the apache2 plugin.
 */
apache2 * apache2::instance()
{
    return g_plugin_apache2_factory.instance();
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
QString apache2::description() const
{
    return "Handle the settings in the apache2.conf files provided by Snap! Websites.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString apache2::dependencies() const
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
int64_t apache2::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize apache2.
 *
 * This function terminates the initialization of the apache2 plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void apache2::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(apache2, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void apache2::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    // retrieve the two status
    //
    retrieve_status_of_snapmanagercgi_conf(server_status);
    retrieve_status_of_snapcgi_conf(server_status);
}


void apache2::retrieve_status_of_snapmanagercgi_conf(snap_manager::server_status & server_status)
{
    std::string const conf_filename("/etc/apache2/sites-available/snapmanager-apache2.conf");

    std::ifstream conf_in;
    conf_in.open(conf_filename);
    if(conf_in.is_open())
    {
        conf_in.seekg(0, std::ios::end);
        std::ifstream::pos_type const size(conf_in.tellg());
        conf_in.seekg(0, std::ios::beg);

        std::string conf;
        conf.resize(size, '#');
        conf_in.read(&conf[0], size);
        if(!conf_in.fail()) // note: eof() will be true so good() will return false
        {
            std::string const server_name("servername");
            auto const it(std::search(
                    conf.begin(),
                    conf.end(),
                    server_name.begin(),
                    server_name.end(),
                    [](char c1, char c2)
                    {
                        return std::tolower(c1) == std::tolower(c2);
                    }));
            if(it != conf.end())
            {
                it += server_name.length();
                if(it != conf.end()
                && std::isspace(*it))
                {
                    do
                    {
                        ++it; // skip spaces
                    }
                    while(it != conf.end() && std::isspace(*it));

                    if(it != conf.end() && *it != '\n' && *it != '\r')
                    {
                        // create a field for this one, it worked
                        //
                        snap_manager::status_t const conf_field(
                                          snap_manager::status_t::state_t::STATUS_STATE_INFO
                                        , get_plugin_name()
                                        , "snapmanager_apache2_conf"
                                        , QString::fromUtf8(key.c_str()));
                        server_status.set_field(conf_field);
                    }
                }
            }
            // else -- no ServerName field, ignore
        }
        else
        {
            SNAP_LOG_DEBUG("could not read \"")(conf_filename)("\" file.");

            // create an error field which is not editable
            //
            snap_manager::status_t const conf_field(
                              snap_manager::status_t::state_t::STATUS_STATE_WARNING
                            , get_plugin_name()
                            , "snapmanager_apache2_conf"
                            , QString());
            server_status.set_field(conf_field);
        }
    }
    // else -- cannot find that .conf, ignore
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
bool apache2::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name().startsWith("id_rsa::"))
    {
        // in case of an error, we do not let the user do anything
        // so let the default behavior do its thing, it will show the
        // field in a non-editable manner
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_ERROR)
        {
            return false;
        }

        // the list of id_rsa.pub files
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
                          "RSA file for \"" + user_name + "\""
                        , s.get_field_name()
                        , s.get_value()
                        , "Enter your id_rsa.pub file in this field and click Save. Then you will have access to this server via apache2. Use the Reset button to remove the file from this server."
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
bool apache2::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::vector<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);
    NOTUSED(affected_services);

    // we support Save and Restore Default of the id_rsa.pub file
    //
    if(field_name.startsWith("id_rsa::"))
    {
        // generate the path to the id_rsa file
        QString const user_name(field_name.mid(8));
        std::string id_rsa_path("/home/");
        id_rsa_path += user_name.toUtf8().data();
        std::string const ssh_path(id_rsa_path + "/.ssh");
        id_rsa_path += "/.ssh/id_rsa.pub";

        // first check whether the user asked to restore the defaults
        //
        if(button_name == "restore_default")
        {
            // "Restore Default" means deleting the file (i.e. no more SSH
            // access although we do not yet break existing connection which
            // we certainly should do too...)
            //
            unlink(id_rsa_path.c_str());
            return true;
        }

        // next make sure the .ssh directory exists, if not create it
        // as expected by ssh
        //
        struct stat s;
        if(stat(ssh_path.c_str(), &s) != 0)
        {
            QString const q_ssh_path(QString::fromUtf8(ssh_path.c_str()));
            if(mkdir_p(q_ssh_path, false) != 0)
            {
                SNAP_LOG_ERROR("we could not create the .ssh directory \"")(ssh_path)("\"");
                return false;
            }
            chmod(ssh_path.c_str(), S_IRWXU); // 0700
            chownnm(q_ssh_path, user_name, user_name);
        }

        if(button_name == "save")
        {
            std::ofstream id_rsa_out;
            id_rsa_out.open(id_rsa_path);
            if(id_rsa_out.is_open())
            {
                id_rsa_out << new_value.trimmed().toUtf8().data() << std::endl;

                chmod(id_rsa_path.c_str(), S_IRUSR | S_IWUSR);

                // WARNING: we would need to get the default name of the
                // user main group instead of assuming it is his name
                //
                chownnm(QString::fromUtf8(id_rsa_path.c_str()), user_name, user_name);
                return true;
            }

            SNAP_LOG_ERROR("we could not open id_rsa file \"")(id_rsa_path)("\"");
        }
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
