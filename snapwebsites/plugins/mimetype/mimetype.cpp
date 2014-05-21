// Snap Websites Server -- find out the MIME type of client's files
// Copyright (C) 2014  Made to Order Software Corp.
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

#include "mimetype.h"

//#include "../messages/messages.h"

#include "not_reached.h"

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(mimetype, 1, 0)

/* \brief Get a fixed MIME type name.
 *
 * The MIME type plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
//char const *get_name(name_t name)
//{
//    switch(name)
//    {
//    case SNAP_NAME_MIMETYPE_ACCEPTED:
//        return "mimetype::accepted";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid SNAP_NAME_MIMETYPE_...");
//
//    }
//    NOTREACHED();
//}









/** \brief Initialize the MIME type plugin.
 *
 * This function is used to initialize the MIME type plugin object.
 */
mimetype::mimetype()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the MIME type plugin.
 *
 * Ensure the MIME type object is clean before it is gone.
 */
mimetype::~mimetype()
{
}


/** \brief Initialize the MIME type.
 *
 * This function terminates the initialization of the MIME type plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void mimetype::on_bootstrap(snap_child *snap)
{
    f_snap = snap;
}


/** \brief Get a pointer to the MIME type plugin.
 *
 * This function returns an instance pointer to the MIME type plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the MIME type plugin.
 */
mimetype *mimetype::instance()
{
    return g_plugin_mimetype_factory.instance();
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
QString mimetype::description() const
{
    return "Add support detection of many file MIME types in JavaScript.";
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
int64_t mimetype::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2014, 5, 20, 23, 44, 30, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void mimetype::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}







SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
