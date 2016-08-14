// Snap Websites Server -- handle user VPN installation
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

// vpn
//
#include "vpn.h"

// our lib
//
#include "lib/form.h"

// snapwebsites lib
//
#include "chownnm.h"
#include "log.h"
#include "mkdir_p.h"
#include "not_reached.h"
#include "not_used.h"

// C++ lib
//
#include <fstream>

// C lib
//
#include <glob.h>

// last entry
//
#include "poison.h"


SNAP_PLUGIN_START(vpn, 1, 0)


/** \brief Get a fixed vpn plugin name.
 *
 * The vpn plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_SNAPMANAGERCGI_VPN_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_VPN_...");

    }
    NOTREACHED();
}




/** \brief Initialize the vpn plugin.
 *
 * This function is used to initialize the vpn plugin object.
 */
vpn::vpn()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the vpn plugin.
 *
 * Ensure the vpn object is clean before it is gone.
 */
vpn::~vpn()
{
}


/** \brief Get a pointer to the vpn plugin.
 *
 * This function returns an instance pointer to the vpn plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the vpn plugin.
 */
vpn * vpn::instance()
{
    return g_plugin_vpn_factory.instance();
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
QString vpn::description() const
{
    return "Manage the vpn public key for users on a specific server.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString vpn::dependencies() const
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
int64_t vpn::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize vpn.
 *
 * This function terminates the initialization of the vpn plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void vpn::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(vpn, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void vpn::on_retrieve_status(snap_manager::server_status & server_status)
{
    NOTUSED(server_status);

    if(f_snap->stop_now_prima())
    {
        return;
    }

    if(!is_installed())
    {
        // no fields whatsoever if the package is not installed
        // (remember that we are part of snapmanagercgi and that's
        // going to be installed!)
        //
        return;
    }

#if 0
    // we want one field per user on the system, at this point we
    // assume that the system does not have hundreds of users since
    // only a few admins should be permitted on those computers
    // anyway...
    //
    glob_t dir = glob_t();
    int const r(glob(
                  "/home/*"
                , GLOB_NOESCAPE
                , glob_error_callback
                , &dir));
    std::shared_ptr<glob_t> ai(&dir, glob_deleter);

    if(r != 0)
    {
        // do nothing when errors occur
        //
        switch(r)
        {
        case GLOB_NOSPACE:
            SNAP_LOG_ERROR("vpn.cpp: glob() did not have enough memory to alllocate its buffers.");
            break;

        case GLOB_ABORTED:
            SNAP_LOG_ERROR("vpn.cpp: glob() was aborted after a read error.");
            break;

        case GLOB_NOMATCH:
            SNAP_LOG_ERROR("vpn.cpp: glob() could not find any users on this computer.");
            break;

        default:
            SNAP_LOG_ERROR("vpn.cpp: unknown glob() error code: ")(r)(".");
            break;

        }

        // do not create any fields on error
        return;
    }

    // check each user
    // (TBD: how to "blacklist" some users so they do not appear here?)
    //
    for(size_t idx(0); idx < dir.gl_pathc; ++idx)
    {
        // TODO: replace the direct handling of the file with a file_content object
        //
        std::string const home_path(dir.gl_pathv[idx]);
        std::string const user_name(home_path.substr(6));
        std::string const authorized_keys_path(home_path + "/.vpn/authorized_keys");
        std::ifstream authorized_keys_in;
        authorized_keys_in.open(authorized_keys_path, std::ios::in | std::ios::binary);
        if(authorized_keys_in.is_open())
        {
            authorized_keys_in.seekg(0, std::ios::end);
            std::ifstream::pos_type const size(authorized_keys_in.tellg());
            authorized_keys_in.seekg(0, std::ios::beg);
            std::string key;
            key.resize(size, '?');
            authorized_keys_in.read(&key[0], size);
            if(!authorized_keys_in.fail()) // note: eof() will be true so good() will return false
            {
                // create a field for this one, it worked
                //
                snap_manager::status_t const rsa_key(
                                  snap_manager::status_t::state_t::STATUS_STATE_INFO
                                , get_plugin_name()
                                , QString::fromUtf8(("authorized_keys::" + user_name).c_str())
                                , QString::fromUtf8(key.c_str()));
                server_status.set_field(rsa_key);
            }
            else
            {
                SNAP_LOG_DEBUG("could not read \"")(authorized_keys_path)("\" file for user \"")(user_name)("\".");

                // create an error field which is not editable
                //
                snap_manager::status_t const rsa_key(
                                  snap_manager::status_t::state_t::STATUS_STATE_ERROR
                                , get_plugin_name()
                                , QString::fromUtf8(("authorized_keys::" + user_name).c_str())
                                , QString());
                server_status.set_field(rsa_key);
            }
        }
        else
        {
            // no authorized_keys file for that user
            // create an empty field so one can add that file
            //
            snap_manager::status_t const rsa_key(
                              snap_manager::status_t::state_t::STATUS_STATE_INFO
                            , get_plugin_name()
                            , QString::fromUtf8(("authorized_keys::" + user_name).c_str())
                            , QString());
            server_status.set_field(rsa_key);
        }
    }
#endif
}


bool vpn::is_installed()
{
    // for now we just check whether the executable is here, this is
    // faster than checking whether the package is installed and
    // should be enough proof that the server is installed and
    // running... and thus offer the editing of
    // /home/*/.vpn/authorized_keys files
    //
    return access("/usr/sbin/openvpn", R_OK | X_OK) == 0;
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
bool vpn::display_value ( QDomElement parent
                        , snap_manager::status_t const & s
                        , snap::snap_uri const & uri
                        )
{
    NOTUSED(parent);
    NOTUSED(s);
    NOTUSED(uri);
#if 0
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name().startsWith("authorized_keys::"))
    {
        // in case of an error, we do not let the user do anything
        // so let the default behavior do its thing, it will show the
        // field in a non-editable manner
        //
        if(s.get_state() == snap_manager::status_t::state_t::STATUS_STATE_ERROR)
        {
            return false;
        }

        // the list of authorized_keys files
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                ,   snap_manager::form::FORM_BUTTON_RESET
                  | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                  | snap_manager::form::FORM_BUTTON_SAVE
                );

        QString const user_name(s.get_field_name().mid(17));
        snap_manager::widget_text::pointer_t field(std::make_shared<snap_manager::widget_text>(
                          "Authorized keys for \"" + user_name + "\""
                        , s.get_field_name()
                        , s.get_value()
                        , "Enter your authorized_keys file in this field and click Save."
                          " Then you will have access to this server via vpn. Use the"
                          " \"Restore Default\" button to remove the file from this server."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }
#endif

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
bool vpn::apply_setting ( QString const & button_name
                        , QString const & field_name
                        , QString const & new_value
                        , QString const & old_or_installation_value
                        , std::set<QString> & affected_services
                        )
{
    NOTUSED(button_name);
    NOTUSED(field_name);
    NOTUSED(new_value);
    NOTUSED(old_or_installation_value);
    NOTUSED(affected_services);
#if 0

    // we support Save and Restore Default of the authorized_keys file
    //
    if(field_name.startsWith("authorized_keys::"))
    {
        // generate the path to the authorized_keys file
        QString const user_name(field_name.mid(17));
        std::string authorized_keys_path("/home/");
        authorized_keys_path += user_name.toUtf8().data();
        std::string const vpn_path(authorized_keys_path + "/.vpn");
        authorized_keys_path += "/.vpn/authorized_keys";

        // first check whether the user asked to restore the defaults
        //
        if(button_name == "restore_default")
        {
            // "Restore Default" means deleting the file (i.e. no more VPN
            // access although we do not yet break existing connection which
            // we certainly should do too...)
            //
            unlink(authorized_keys_path.c_str());
            return true;
        }

        // next make sure the .vpn directory exists, if not create it
        // as expected by vpn
        //
        struct stat s;
        if(stat(vpn_path.c_str(), &s) != 0)
        {
            QString const q_vpn_path(QString::fromUtf8(vpn_path.c_str()));
            if(mkdir_p(q_vpn_path, false) != 0)
            {
                SNAP_LOG_ERROR("we could not create the .vpn directory \"")(vpn_path)("\"");
                return false;
            }
            chmod(vpn_path.c_str(), S_IRWXU); // 0700
            chownnm(q_vpn_path, user_name, user_name);
        }

        if(button_name == "save")
        {
            // TODO: replace the direct handling of the file with a file_content object
            //
            std::ofstream authorized_keys_out;
            authorized_keys_out.open(authorized_keys_path, std::ios::out | std::ios::trunc | std::ios::binary);
            if(authorized_keys_out.is_open())
            {
                authorized_keys_out << new_value.trimmed().toUtf8().data() << std::endl;

                chmod(authorized_keys_path.c_str(), S_IRUSR | S_IWUSR);

                // WARNING: we would need to get the default name of the
                // user main group instead of assuming it is his name
                //
                chownnm(QString::fromUtf8(authorized_keys_path.c_str()), user_name, user_name);
                return true;
            }

            SNAP_LOG_ERROR("we could not open authorized_keys file \"")(authorized_keys_path)("\"");
        }
    }

#endif
    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
