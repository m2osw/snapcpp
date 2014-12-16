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

#include "../editor/editor.h"
//#include "../output/output.h"

#include "not_reached.h"

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(ecommerce, 1, 0)


/* \brief Get a fixed ecommerce name.
 *
 * The ecommerce plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_ECOMMERCE_PRICE:
        return "ecommerce::price";

    case SNAP_NAME_ECOMMERCE_PRODUCT_DESCRIPTION:
        return "ecommerce::product_name";

    case SNAP_NAME_ECOMMERCE_PRODUCT_TYPE_PATH:
        return "types/taxonomy/system/content-types/ecommerce/product";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_ECOMMERCE_...");

    }
    NOTREACHED();
}









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

    SNAP_LISTEN(ecommerce, "layout", layout::layout, generate_header_content, _1, _2, _3, _4);
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

    SNAP_PLUGIN_UPDATE(2014, 12, 16, 0, 37, 40, content_update);

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


/** \brief Setup page for the editor.
 *
 * The editor has a set of dynamic parameters that the users are offered
 * to setup. These parameters need to be sent to the user and we use this
 * function for that purpose.
 *
 * \todo
 * Look for a way to generate the editor data only if necessary (too
 * complex for now.)
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The header being generated.
 * \param[in,out] metadata  The metadata being generated.
 * \param[in] ctemplate  The template in case path does not exist.
 */
void ecommerce::on_generate_header_content(content::path_info_t& ipath, QDomElement& header, QDomElement& metadata, QString const& ctemplate)
{
    static_cast<void>(ipath);
    static_cast<void>(metadata);
    static_cast<void>(ctemplate);

    QDomDocument doc(header.ownerDocument());

    // make sure this is a product, if so, add product fields
    links::link_info product_info(content::get_name(content::SNAP_NAME_CONTENT_PAGE_TYPE), true, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(product_info));
    links::link_info product_child_info;
    if(link_ctxt->next_link(product_child_info))
    {
        // the link_info returns a full key with domain name
        // use a path_info_t to retrieve the cpath instead
        content::path_info_t type_ipath;
        type_ipath.set_path(product_child_info.key());
        if(type_ipath.get_cpath().startsWith(get_name(SNAP_NAME_ECOMMERCE_PRODUCT_TYPE_PATH)))
        {
            // if the content is the main page then define the titles and body here
            FIELD_SEARCH
                (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
                (content::field_search::COMMAND_ELEMENT, metadata)
                (content::field_search::COMMAND_PATH_INFO_REVISION, ipath)

                // /snap/head/metadata/ecommerce
                (content::field_search::COMMAND_CHILD_ELEMENT, "ecommerce")

                // /snap/head/metadata/ecommerce/product-name
                (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_ECOMMERCE_PRODUCT_DESCRIPTION))
                (content::field_search::COMMAND_SELF)
                (content::field_search::COMMAND_IF_FOUND, 1)
                    // use page title as a fallback
                    (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_TITLE))
                    (content::field_search::COMMAND_SELF)
                (content::field_search::COMMAND_LABEL, 1)
                (content::field_search::COMMAND_SAVE, "product-description")

                // /snap/head/metadata/ecommerce/product-price
                (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_ECOMMERCE_PRICE))
                (content::field_search::COMMAND_SELF)
                (content::field_search::COMMAND_SAVE, "product-price")

                // generate!
                ;
        }
    }

    // TODO: find a way to include e-Commerce data only if required
    //       (it may already be done! search on add_javascript() for info.)
    content::content::instance()->add_javascript(doc, "ecommerce");
    content::content::instance()->add_css(doc, "ecommerce");
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
