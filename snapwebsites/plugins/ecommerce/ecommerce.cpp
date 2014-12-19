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

#include "qdomxpath.h"
#include "not_reached.h"

#include <iostream>

#include <QDateTime>

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
    case SNAP_NAME_ECOMMERCE_CART_PRODUCTS:
        return "ecommerce::cart_products";

    case SNAP_NAME_ECOMMERCE_CART_PRODUCTS_POST_FIELD:
        return "ecommerce__cart_products";

    case SNAP_NAME_ECOMMERCE_JAVASCRIPT_CART:
        return "js/ecommerce/ecommerce-cart.js";

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

    SNAP_LISTEN(ecommerce, "server", server, process_post, _1);
    SNAP_LISTEN(ecommerce, "layout", layout::layout, generate_header_content, _1, _2, _3, _4);
    SNAP_LISTEN(ecommerce, "path", path::path, can_handle_dynamic_path, _1, _2);
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

    SNAP_PLUGIN_UPDATE(2014, 12, 19, 2, 27, 40, content_update);

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
    content::content::instance()->add_javascript(doc, "ecommerce-cart");
    content::content::instance()->add_css(doc, "ecommerce");
}


/** \brief Check the URL and process the POST data accordingly.
 *
 * This function manages the posted cart data. All we do, really,
 * is save the cart in the user's session. That simple. We do
 * this as fast as possible so as to quickly reply to the user.
 * Since we do not have to check permissions for more pages and
 * do not have to generate any heavy HTML output, it should be
 * dead fast.
 *
 * The cart data is not checked here. It will be once we generate
 * the actual invoice.
 *
 * The function counts the number of products to make sure that
 * it does not go over the imposed limit. Also each tag cannot
 * be any larger than a certain size so we also calculate the
 * total byte size and impose that limit too.
 *
 * \todo
 * Add a cart session? I think that the user session is enough plus
 * we will have an editor session since the cart is to have the
 * quantity fields accessible as editor widgets. At this point, I
 * leave this open. It won't matter much if the user is logged in
 * on a secure server (i.e. using HTTPS which is generally
 * mandatory when you use the e-Commerce feature.)
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void ecommerce::on_process_post(QString const& uri_path)
{
    // TODO: see doc above
    //QString const editor_full_session(f_snap->postenv("_editor_session"));
    //if(editor_full_session.isEmpty())
    //{
    //    // if the _editor_session variable does not exist, do not consider this
    //    // POST as an Editor POST
    //    return;
    //}

    // make sure this is a cart post
    char const *cart_products(get_name(SNAP_NAME_ECOMMERCE_CART_PRODUCTS_POST_FIELD));
    if(!f_snap->postenv_exists(cart_products))
    {
        return;
    }

    content::path_info_t ipath;
    ipath.set_path(uri_path);

    QString const cart_contents(f_snap->postenv(cart_products));
    users::users::instance()->attach_to_session(get_name(SNAP_NAME_ECOMMERCE_CART_PRODUCTS), cart_contents);

    // create the AJAX response
    server_access::server_access *server_access_plugin(server_access::server_access::instance());
    server_access_plugin->create_ajax_result(ipath, true);
    server_access_plugin->ajax_output();
}


/** \brief The e-Commerce plugin dynamically generates a JavaScript file.
 *
 * This function lets the system know that the file named ecommerce-cart.js is
 * dynamically generated by this plugin.
 *
 * \param[in,out] ipath  The path being checked.
 * \param[in,out] plugin_info  Info about the plugin which is to take control.
 */
void ecommerce::on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info)
{
    QString const cpath(ipath.get_cpath());
    if(cpath == get_name(SNAP_NAME_ECOMMERCE_JAVASCRIPT_CART))
    {
        // tell the path plugin that this is ours
        plugin_info.set_plugin(this);
        return;
    }
}


/** \brief This function gets called when a dynamic path gets executed.
 *
 * This function checks the dynamic path supported. If the path
 * is the ecommerce-cart.js file, then the file generates a JavaScript file
 * and returns that to the client. This file is always marked as
 * requiring a reload (i.e. no caching allowed.)
 *
 * \param[in] ipath  The path of the page being executed.
 */
bool ecommerce::on_path_execute(content::path_info_t& ipath)
{
    QString const cpath(ipath.get_cpath());
    if(cpath == get_name(SNAP_NAME_ECOMMERCE_JAVASCRIPT_CART))
    {
        // check whether we have some products in the cart, if so
        // spit them out now! (with the exception of the product
        // this very page represents if it does represent a product)

        // we do not start spitting out any code up until the time we
        // know that there is at least one product in the cart

        // get the session information
        QString const cart_xml(users::users::instance()->get_from_session(get_name(SNAP_NAME_ECOMMERCE_CART_PRODUCTS)));

        QString js(QString("// e-Commerce Cart generated on %1\n").arg(QDateTime::currentDateTime().toString()));
        QDomDocument doc;
        doc.setContent(cart_xml);
        QDomXPath products_xpath;
        products_xpath.setXPath("/cart/product");
        QDomXPath::node_vector_t product_tags(products_xpath.apply(doc));
        int const max_products(product_tags.size());

        // first add all the product types
        bool first(true);
        for(int i(0); i < max_products; ++i)
        {
            // we found the widget, display its label instead
            QDomElement product(product_tags[i].toElement());
            QString const guid(product.attribute("guid"));
            if(ipath.get_key() != guid)
            {
                // get the data in local variables
                content::path_info_t ipath_product;
                ipath_product.set_path(guid);
                content::field_search::search_result_t product_result;
                FIELD_SEARCH
                    (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
                    (content::field_search::COMMAND_PATH_INFO_REVISION, ipath_product)

                    // DESCRIPTION
                    (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_ECOMMERCE_PRODUCT_DESCRIPTION))
                    (content::field_search::COMMAND_SELF)
                    (content::field_search::COMMAND_IF_FOUND, 1)
                        // use page title as a fallback
                        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_TITLE))
                        (content::field_search::COMMAND_SELF)
                    (content::field_search::COMMAND_LABEL, 1)
                    //(content::field_search::COMMAND_RESULT, result_description)

                    // PRICE
                    (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_ECOMMERCE_PRICE))
                    (content::field_search::COMMAND_SELF)

                    // get the 2 results
                    (content::field_search::COMMAND_RESULT, product_result)

                    // retrieve!
                    ;

                if(product_result.size() == 2)
                {
                    // add a product type
                    if(first)
                    {
                        first = false;
                        js += "jQuery(document).ready(function(){"
                            "snapwebsites.eCommerceCartInstance.setInitializing(true)\n";
                    }
                    QString guid_safe_quotes(guid);
                    guid_safe_quotes.replace("\'", "\\'");
                    QString product_description(product_result[0].stringValue());
                    product_description.replace("\'", "\\'");
                    js += ".registerProductType({"
                            "'ecommerce::features':    'ecommerce::basic',"
                            "'ecommerce::guid':        '" + guid_safe_quotes + "',"
                            "'ecommerce::description': '" + product_description + "',"
                            "'ecommerce::price':       " + product_result[1].stringValue() +
                        "})\n";
                }
            }
        }
        if(!first)
        {
            js += ";\n";
        }

        // second add the product to the cart, including their quantity
        // and attributes
        for(int i(0); i < max_products; ++i)
        {
            if(first)
            {
                first = false;
                js += "jQuery(document).ready(function(){\n";
            }

            // we found the widget, display its label instead
            QDomElement product(product_tags[i].toElement());
            QString const guid(product.attribute("guid"));
            QString const quantity(product.attribute("q"));
            QString guid_safe_quotes(guid);
            guid_safe_quotes.replace("\'", "\\'");
            js += "snapwebsites.eCommerceCartInstance.addProduct('" + guid_safe_quotes + "', " + quantity + ");\n";
            // TODO: we need to add support for attributes
        }

        if(!first)
        {
            js += "snapwebsites.eCommerceCartInstance.setInitializing(false);});\n";
        }

        f_snap->output(js);
        // make sure it is a text/javascript and it was expired already
        f_snap->set_header("Content-Type", "text/javascript; charset=utf8", snap_child::HEADER_MODE_EVERYWHERE);
        f_snap->set_header("Expires", "Sat,  1 Jan 2000 00:00:00 GMT", snap_child::HEADER_MODE_EVERYWHERE);
        f_snap->set_header("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0", snap_child::HEADER_MODE_EVERYWHERE);

        return true;
    }

    return false;
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
