// Snap Websites Server -- Snap Software Description handling
// Copyright (C) 2012-2015  Made to Order Software Corp.
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

/** \file
 * \brief OAuth2 handling.
 *
 * This plugin handles authentication via OAuth2 by application that
 * want to access private features of a Snap! Website.
 *
 * This plugin does not offer any REST API by itself. Only an authentication
 * process.
 */

#include "snap_software_description.h"

#include "http_strings.h"

#include "poison.h"


SNAP_PLUGIN_START(snap_software_description, 1, 0)



/** \brief Get a fixed snap_software_description plugin name.
 *
 * The snap_software_description plugin makes use of different names
 * in the database. This function ensures that you get the right
 * spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_ENABLE:
        return "snap_software_description::enable";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_...");

    }
    NOTREACHED();
}


/** \class snap_software_description
 * \brief The snap_software_description plugin handles application authentication.
 *
 * Any Snap! website can be setup to accept application authentication.
 *
 * The website generates a token that can be used to log you in.
 */


/** \brief Initialize the snap_software_description plugin.
 *
 * This function initializes the snap_software_description plugin.
 */
snap_software_description::snap_software_description()
    //: f_snap(nullptr) -- auto-init
{
}

/** \brief Destroy the snap_software_description plugin.
 *
 * This function cleans up the snap_software_description plugin.
 */
snap_software_description::~snap_software_description()
{
}


/** \brief Get a pointer to the snap_software_description plugin.
 *
 * This function returns an instance pointer to the snap_software_descriptiosnap_software_descriptionin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the snap_software_description plugin.
 */
snap_software_description * snap_software_description::instance()
{
    return g_plugin_snap_software_description_factory.instance();
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
QString snap_software_description::description() const
{
    return "The Snap Software Description plugin offers you a way to"
          " define a set of descriptions for software that you are offering"
          " for download on your website. The software may be free or for"
          " a fee. It may also be a shareware.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last
 *                          updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t snap_software_description::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 1, 23, 13, 39, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the snap_software_description plugin content.
 *
 * This function updates the contents in the database using the
 * system update settings found in the resources.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void snap_software_description::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Bootstrap the snap_software_description.
 *
 * This function adds the events the snap_software_description plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void snap_software_description::on_bootstrap(::snap::snap_child *snap)
{
    f_snap = snap;
}


/** \brief Check for the "/user/snap_software_description" path.
 *
 * This function ensures that the URL is /user/snap_software_description and if so write
 * checks that the application knows the identifier and secret of this
 * website and if so, return a session identifier that can be used to
 * further access the server including private pages.
 *
 * \param[in] ipath  The URL being managed.
 *
 * \return true if the authentication parameters were properly defined,
 *         an error is generated otherwise.
 */
bool snap_software_description::on_path_execute(content::path_info_t& ipath)
{
    static_cast<void>(ipath);

    QDomDocument doc("snap");
    QDomElement snap(doc.documentElement());
    QDomElement file(doc.createElement("file"));
    snap.appendChild(file);
    QDomText text(doc.createTextNode("filename or something..."));
    file.appendChild(text);

    // generate the result
    // accept XML and JSON
    http_strings::WeightedHttpString encodings(f_snap->snapenv("HTTP_ACCEPT"));
    float const xml_level(encodings.get_level("application/xml"));
    float const json_level(encodings.get_level("application/json"));
    if(json_level > xml_level)
    {
        // TODO: not implemented yet (use XSLT)
        f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_IMPLEMENTED,
                "Not Implemented",
                "JSON support not implemented yet.",
                "We need to implement the XSLT to convert the XML to JSON.");
        NOTREACHED();
    }
    else
    {
        f_snap->output(doc.toString());
    }

    return true;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
