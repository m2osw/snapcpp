// Snap Websites Server -- handle a cart, checkout, wishlist, affiliates...
// Copyright (C) 2011-2014  Made to Order Software Corp.
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

#include "ecommerce.h"

//#include "../editor/editor.h"
//#include "../output/output.h"

#include "not_reached.h"

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(ecommerce, 1, 0)


///* \brief Get a fixed ecommerce name.
// *
// * The ecommerce plugin makes use of different names in the database. This
// * function ensures that you get the right spelling for a given name.
// *
// * \param[in] name  The name to retrieve.
// *
// * \return A pointer to the name.
// */
//char const *get_name(name_t name)
//{
//    switch(name)
//    {
//    case SNAP_NAME_LOCALE_NAME:
//        return "ecommerce::name";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid SNAP_NAME_LOCALE_...");
//
//    }
//    NOTREACHED();
//}









/** \brief Initialize the ecommerce plugin.
 *
 * This function is used to initialize the ecommerce plugin object.
 */
ecommerce::ecommerce()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the ecommerce plugin.
 *
 * Ensure the ecommerce object is clean before it is gone.
 */
ecommerce::~ecommerce()
{
}


/** \brief Initialize the ecommerce.
 *
 * This function terminates the initialization of the ecommerce plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void ecommerce::on_bootstrap(snap_child *snap)
{
    f_snap = snap;
}


/** \brief Get a pointer to the ecommerce plugin.
 *
 * This function returns an instance pointer to the ecommerce plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the ecommerce plugin.
 */
ecommerce *ecommerce::instance()
{
    return g_plugin_ecommerce_factory.instance();
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
QString ecommerce::description() const
{
    return "The e-Commerce plugin offers all the necessary features a"
        " website needs to offer a full e-Commerce environment so your"
        " users can purchase your goods and services. The base plugin"
        " includes many features directly available to you without the"
        " need for other plugins. However, you want to install the"
        " ecommerce-payment plugin and at least one of the payments"
        " gateway in order to allow for the actual payments.";
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
int64_t ecommerce::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2014, 12, 3, 13, 56, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void ecommerce::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
