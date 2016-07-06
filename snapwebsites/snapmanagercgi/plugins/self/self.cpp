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

#include "not_reached.h"
#include "not_used.h"

#include <snapwebsites/qdomhelpers.h>

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
void self::on_retrieve_status(snap_manager::server_status_t & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    server_status["status"] = "Up";

    server_status["ip"] = f_snap->get_public_ip();

    if(!f_snap->has_snapmanager_frontend())
    {
        server_status["warning:snapmanager_no_frontend"] = "The snapmanager_frontend variable is empty. This is most likely not what you want.";
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
