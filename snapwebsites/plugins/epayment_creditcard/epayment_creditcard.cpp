// Snap Websites Server -- handle credit card data for other plugins
// Copyright (C) 2014-2016  Made to Order Software Corp.
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

#include "epayment_creditcard.h"

//#include "../messages/messages.h"
//#include "../permissions/permissions.h"
//#include "../server_access/server_access.h"
//#include "../users/users.h"

#include "not_reached.h"
#include "not_used.h"

#include "poison.h"


SNAP_PLUGIN_START(epayment_creditcard, 1, 0)



/** \brief Initialize the epayment_creditcard plugin.
 *
 * This function is used to initialize the epayment_creditcard plugin object.
 */
epayment_creditcard::epayment_creditcard()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the epayment_creditcard plugin.
 *
 * Ensure the epayment_creditcard object is clean before it is gone.
 */
epayment_creditcard::~epayment_creditcard()
{
}


/** \brief Get a pointer to the epayment_creditcard plugin.
 *
 * This function returns an instance pointer to the epayment_creditcard plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the epayment_creditcard plugin.
 */
epayment_creditcard * epayment_creditcard::instance()
{
    return g_plugin_epayment_creditcard_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString epayment_creditcard::settings_path() const
{
    return "/admin/settings/epayment/creditcard";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString epayment_creditcard::icon() const
{
    return "/images/epayment/epayment-credit-card-logo-64x64.png";
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
QString epayment_creditcard::description() const
{
    return "Generate a credit card form that the end user is expected to"
          " fill in. This plugin is generally not installed by itself,"
          " instead it is marked as a dependency of a plugin that is"
          " capable of processing credit cards.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString epayment_creditcard::dependencies() const
{
    return "|date_widgets|editor|epayment|messages|path|permissions|users|";
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
int64_t epayment_creditcard::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 3, 30, 15, 4, 16, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update (in
 *                                 micro-seconds).
 */
void epayment_creditcard::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the epayment_creditcard.
 *
 * This function terminates the initialization of the epayment_creditcard plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void epayment_creditcard::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(epayment_creditcard, "server", server, process_post, _1);
}


/** \brief Accept a POST to request information about the server.
 *
 * This function manages the data sent to the server by a client script.
 * In many cases, it is used to know whether something is true or false,
 * although the answer may be any valid text.
 *
 * The function verifies that the "editor_session" variable is set, if
 * not it ignores the POST sine another plugin may be the owner.
 *
 * \note
 * This function is a server signal generated by the snap_child
 * execute() function.
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void epayment_creditcard::on_process_post(QString const & uri_path)
{
    NOTUSED(uri_path);
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
