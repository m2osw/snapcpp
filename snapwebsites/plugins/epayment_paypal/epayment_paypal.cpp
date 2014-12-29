// Snap Websites Server -- handle the Paypal payment facility
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

#include "epayment_paypal.h"

#include "../editor/editor.h"
#include "../epayment/epayment.h"
#include "../messages/messages.h"

#include "log.h"
#include "not_reached.h"
#include "tcp_client_server.h"

#include <as2js/json.h>

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(epayment_paypal, 1, 0)


/* \brief Get a fixed epayment name.
 *
 * The epayment plugin makes use of different names in the database. This
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
    case SNAP_NAME_EPAYMENT_PAYPAL_CANCEL_URL:
        return "epayment/paypal/cancel";

    case SNAP_NAME_EPAYMENT_PAYPAL_CLICKED_POST_FIELD:
        return "epayment__epayment_paypal";

    case SNAP_NAME_EPAYMENT_PAYPAL_DEBUG:
        return "epayment_paypal::debug";

    case SNAP_NAME_EPAYMENT_PAYPAL_RETURN_URL:
        return "epayment/paypal/return";

    case SNAP_NAME_EPAYMENT_PAYPAL_SETTINGS_PATH:
        return "/admin/settings/epayment/paypal";

    case SNAP_NAME_EPAYMENT_PAYPAL_TABLE:
        return "epayment_paypal";


    // ******************
    //    SECURE NAMES
    // ******************
    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CLIENT_ID:
        return "epayment_paypal::client_id";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT:
        return "epayment_paypal::created_payment";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT_HEADER:
        return "epayment_paypal::created_payment_header";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTE_PAYMENT:
        return "epayment_paypal::execute_payment";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_ACCESS_TOKEN:
        return "epayment_paypal::oauth2_access_token";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_APP_ID:
        return "epayment_paypal::oauth2_app_id";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_DATA:
        return "epayment_paypal::oauth2_data";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_EXPIRES:
        return "epayment_paypal::oauth2_expires";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_HEADER:
        return "epayment_paypal::oauth2_header";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_SCOPE:
        return "epayment_paypal::oauth2_scope";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_TOKEN_TYPE:
        return "epayment_paypal::oauth2_token_type";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_ID:
        return "epayment_paypal::payment_id";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_TOKEN:
        return "epayment_paypal::payment_token";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYER_ID:
        return "epayment_paypal::payer_id";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SANDBOX_CLIENT_ID:
        return "epayment_paypal::sandbox_client_id";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SANDBOX_SECRET:
        return "epayment_paypal::sandbox_secret";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SECRET:
        return "epayment_paypal::secret";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_EPAYMENT_PAYPAL_...");

    }
    NOTREACHED();
}









/** \brief Initialize the epayment_paypal plugin.
 *
 * This function is used to initialize the epayment_paypal plugin object.
 */
epayment_paypal::epayment_paypal()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the epayment_paypal plugin.
 *
 * Ensure the epayment_paypal object is clean before it is gone.
 */
epayment_paypal::~epayment_paypal()
{
}


/** \brief Initialize the epayment_paypal.
 *
 * This function terminates the initialization of the epayment_paypal plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void epayment_paypal::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(epayment_paypal, "server", server, process_post, _1);
    SNAP_LISTEN(epayment_paypal, "layout", layout::layout, generate_header_content, _1, _2, _3, _4);
}


/** \brief Get a pointer to the epayment_paypal plugin.
 *
 * This function returns an instance pointer to the epayment_paypal plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the epayment_paypal plugin.
 */
epayment_paypal *epayment_paypal::instance()
{
    return g_plugin_epayment_paypal_factory.instance();
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
QString epayment_paypal::description() const
{
    return "The PayPal e-Payment Facility plugin offers payment from the"
          " client's PayPal account.";
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
int64_t epayment_paypal::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2014, 12, 29, 0, 54, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief First update to run for the epayment_paypal plugin.
 *
 * This function is the first update for the epayment_paypal plugin.
 * It creates the tables.
 *
 * \note
 * We reset the cached pointer to the tables to make sure that they get
 * synchronized when used for the first time (very first initialization
 * only, do_update() is not generally called anyway, unless you are a
 * developer with the debug mode turned on.)
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void epayment_paypal::initial_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    get_epayment_paypal_table();
    f_epayment_paypal_table.reset();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void epayment_paypal::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the epayment_paypal table.
 *
 * This function creates the epayment_paypal table if it does not already
 * exist. Otherwise it simply initializes the f_payment_paypal_table
 * variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The epayment_paypal table is used to save the payment identifiers so
 * we get an immediate reference back to the invoice. We use the name of
 * the website as the row (no protocol), then the PayPal payment identifier
 * for each invoice.
 *
 * \code
 *    snapwebsites.org
 *       PAY-4327271037362
 *          77  (as an int64_t)
 * \endcode
 *
 * \note
 * The table makes use of the domain only because the same website may
 * support HTTP and HTTPS for the exact same data. However, if your
 * website uses a sub-domain, that will be included. So in the example
 * above it could have been "www.snapwebsites.org" in which case it
 * is different from "snapwebsites.org".
 *
 * \return The pointer to the content table.
 */
QtCassandra::QCassandraTable::pointer_t epayment_paypal::get_epayment_paypal_table()
{
    if(!f_epayment_paypal_table)
    {
        f_epayment_paypal_table = f_snap->create_table(get_name(SNAP_NAME_EPAYMENT_PAYPAL_TABLE), "Website epayment_paypal table.");
    }
    return f_epayment_paypal_table;
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
void epayment_paypal::on_generate_header_content(content::path_info_t& ipath, QDomElement& header, QDomElement& metadata, QString const& ctemplate)
{
    static_cast<void>(ipath);
    static_cast<void>(metadata);
    static_cast<void>(ctemplate);

    QDomDocument doc(header.ownerDocument());

    // // make sure this is a product, if so, add product fields
    // links::link_info product_info(content::get_name(content::SNAP_NAME_CONTENT_PAGE_TYPE), true, ipath.get_key(), ipath.get_branch());
    // QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(product_info));
    // links::link_info product_child_info;
    // if(link_ctxt->next_link(product_child_info))
    // {
    //     // the link_info returns a full key with domain name
    //     // use a path_info_t to retrieve the cpath instead
    //     content::path_info_t type_ipath;
    //     type_ipath.set_path(product_child_info.key());
    //     if(type_ipath.get_cpath().startsWith(get_name(SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH)))
    //     {
    //         // if the content is the main page then define the titles and body here
    //         FIELD_SEARCH
    //             (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
    //             (content::field_search::COMMAND_ELEMENT, metadata)
    //             (content::field_search::COMMAND_PATH_INFO_REVISION, ipath)

    //             // /snap/head/metadata/epayment
    //             (content::field_search::COMMAND_CHILD_ELEMENT, "epayment")

    //             // /snap/head/metadata/epayment/product-name
    //             (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_EPAYMENT_PRODUCT_DESCRIPTION))
    //             (content::field_search::COMMAND_SELF)
    //             (content::field_search::COMMAND_IF_FOUND, 1)
    //                 // use page title as a fallback
    //                 (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_TITLE))
    //                 (content::field_search::COMMAND_SELF)
    //             (content::field_search::COMMAND_LABEL, 1)
    //             (content::field_search::COMMAND_SAVE, "product-description")

    //             // /snap/head/metadata/epayment/product-price
    //             (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_EPAYMENT_PRICE))
    //             (content::field_search::COMMAND_SELF)
    //             (content::field_search::COMMAND_SAVE, "product-price")

    //             // generate!
    //             ;
    //     }
    // }

    // TODO: find a way to include e-Payment data only if required
    //       (it may already be done! search on add_javascript() for info.)
    content::content::instance()->add_javascript(doc, "epayment-paypal");
    content::content::instance()->add_css(doc, "epayment-paypal");
}


/** \brief The e-Commerce plugin dynamically generates a JavaScript file.
 *
 * This function lets the system know that the file named ecommerce-cart.js is
 * dynamically generated by this plugin.
 *
 * \param[in,out] ipath  The path being checked.
 * \param[in,out] plugin_info  Info about the plugin which is to take control.
 */
void epayment_paypal::on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info)
{
    static_cast<void>(plugin_info);

    QString const cpath(ipath.get_cpath());
    if(cpath == get_name(SNAP_NAME_EPAYMENT_PAYPAL_CANCEL_URL))
    {
        // the user canceled that invoice...
    }
    else if(cpath == get_name(SNAP_NAME_EPAYMENT_PAYPAL_RETURN_URL))
    {
        QtCassandra::QCassandraTable::pointer_t epayment_paypal_table(get_epayment_paypal_table());
        // the user made the payment!
        //
        // http://www.your-domain.com/epayment/paypal/return?paymentId=PAY-123&token=EC-123&PayerID=123
        snap_uri const main_uri(f_snap->get_uri());
        if(!main_uri.has_query_option("paymentId"))
        {
            messages::messages::instance()->set_error(
                "PayPal Missing Option",
                "PayPal replied without a paymentId parameter", 
                "Without the \"paymentId\" parameter we cannot know which invoice this is linked with.",
                false
            );
            return;
        }
        QString const id(main_uri.option("paymentId"));
        QString const invoice(epayment_paypal_table->row(main_uri.full_domain())->cell(id)->value().stringValue());
        content::path_info_t invoice_ipath;
        invoice_ipath.set_path(invoice);


        if(!main_uri.has_query_option("PayerID"))
        {
            messages::messages::instance()->set_error(
                "PayPal Missing Option",
                "PayPal replied without a paymentId parameter", 
                "Without the \"paymentId\" parameter we cannot know which invoice this is linked with.",
                false
            );
            return;
        }
        QString const payer_id(main_uri.option("PayerID"));

        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
        QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(invoice_ipath.get_key()));

        // save the PayerID value
        secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYER_ID))->setValue(payer_id);

        // Optionally, we may get a token that we save, just in case
        // (for PayPal payments this token is not used at this time)
        if(main_uri.has_query_option("token"))
        {
            QString const token(main_uri.option("token"));
            secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_TOKEN))->setValue(token);
        }
    }
}


///** \brief This function gets called when a dynamic path gets executed.
// *
// * This function checks the dynamic path supported. If the path
// * is the ecommerce-cart.js file, then the file generates a JavaScript file
// * and returns that to the client. This file is always marked as
// * requiring a reload (i.e. no caching allowed.)
// *
// * \param[in] ipath  The path of the page being executed.
// */
//bool payment_paypal::on_path_execute(content::path_info_t& ipath)
//{
//    QString const cpath(ipath.get_cpath());
//    if(cpath == get_name(SNAP_NAME_ECOMMERCE_JAVASCRIPT_CART))
//    {
//        // check whether we have some products in the cart, if so
//        // spit them out now! (with the exception of the product
//        // this very page represents if it does represent a product)
//
//        // we do not start spitting out any code up until the time we
//        // know that there is at least one product in the cart
//
//        // get the session information
//        QString const cart_xml(users::users::instance()->get_from_session(get_name(SNAP_NAME_ECOMMERCE_CART_PRODUCTS)));
//
//        QString js(QString("// e-Commerce Cart generated on %1\n").arg(QDateTime::currentDateTime().toString()));
//        QDomDocument doc;
//        doc.setContent(cart_xml);
//        QDomXPath products_xpath;
//        products_xpath.setXPath("/cart/product");
//        QDomXPath::node_vector_t product_tags(products_xpath.apply(doc));
//        int const max_products(product_tags.size());
//
//        snap_uri const main_uri(f_snap->get_uri());
//        bool const no_types(main_uri.has_query_option("no-types"));
//
//        // first add all the product types
//        bool first(true);
//        if(!no_types) for(int i(0); i < max_products; ++i)
//        {
//            // we found the product, retrieve its description and price
//            QDomElement product(product_tags[i].toElement());
//            QString const guid(product.attribute("guid"));
//            if(ipath.get_key() != guid) // this page is the product?
//            {
//                // TODO: We must verify that the GUID points to a product
//                //       AND that the user has enough permissions to see
//                //       that product; if not then the user should not be
//                //       able to add that product to the cart in the first
//                //       place so we can err and stop the processing
//                //
//                // get the data in local variables
//                content::path_info_t product_ipath;
//                product_ipath.set_path(guid);
//                content::field_search::search_result_t product_result;
//                FIELD_SEARCH
//                    (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
//                    (content::field_search::COMMAND_PATH_INFO_REVISION, product_ipath)
//
//                    // DESCRIPTION
//                    (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_ECOMMERCE_PRODUCT_DESCRIPTION))
//                    (content::field_search::COMMAND_SELF)
//                    (content::field_search::COMMAND_IF_FOUND, 1)
//                        // use page title as a fallback
//                        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_TITLE))
//                        (content::field_search::COMMAND_SELF)
//                    (content::field_search::COMMAND_LABEL, 1)
//
//                    // PRICE
//                    (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_ECOMMERCE_PRICE))
//                    (content::field_search::COMMAND_SELF)
//
//                    // get the 2 results
//                    (content::field_search::COMMAND_RESULT, product_result)
//
//                    // retrieve!
//                    ;
//
//                if(product_result.size() == 2)
//                {
//                    // add a product type
//                    if(first)
//                    {
//                        first = false;
//                        js += "jQuery(document).ready(function(){"
//                            "snapwebsites.eCommerceCartInstance.setInitializing(true)\n";
//                    }
//                    QString guid_safe_quotes(guid);
//                    guid_safe_quotes.replace("'", "\\'");
//                    QString product_description(product_result[0].stringValue());
//                    product_description.replace("'", "\\'");
//                    js += ".registerProductType({"
//                            "'ecommerce::features':    'ecommerce::basic',"
//                            "'ecommerce::guid':        '" + guid_safe_quotes + "',"
//                            "'ecommerce::description': '" + product_description + "',"
//                            "'ecommerce::price':       " + product_result[1].stringValue() +
//                        "})\n";
//                }
//            }
//        }
//        if(!first)
//        {
//            js += ";\n";
//        }
//
//        // second add the product to the cart, including their quantity
//        // and attributes
//        for(int i(0); i < max_products; ++i)
//        {
//            if(first)
//            {
//                first = false;
//                js += "jQuery(document).ready(function(){\n";
//            }
//
//            // retrieve the product GUID and quantity
//            // TBD: check that the product is valid? Here it is less of a
//            //      problem since that's the cart itself
//            QDomElement product(product_tags[i].toElement());
//            QString const guid(product.attribute("guid"));
//            QString const quantity(product.attribute("q"));
//            QString guid_safe_quotes(guid);
//            guid_safe_quotes.replace("'", "\\'");
//            js += "snapwebsites.eCommerceCartInstance.addProduct('" + guid_safe_quotes + "', " + quantity + ");\n";
//            // TODO: we need to add support for attributes
//        }
//
//        if(!first)
//        {
//            js += "snapwebsites.eCommerceCartInstance.setInitializing(false);});\n";
//        }
//
//        f_snap->output(js);
//        // make sure it is a text/javascript and it was expired already
//        f_snap->set_header("Content-Type", "text/javascript; charset=utf8", snap_child::HEADER_MODE_EVERYWHERE);
//        f_snap->set_header("Expires", "Sat,  1 Jan 2000 00:00:00 GMT", snap_child::HEADER_MODE_EVERYWHERE);
//        f_snap->set_header("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0", snap_child::HEADER_MODE_EVERYWHERE);
//
//        return true;
//    }
//
//    return false;
//}


/** \brief Check whether we are running in debug mode or not.
 *
 * This function retrieves the current status of the debug flag from
 * the database.
 *
 * The function caches the result. Backends have to be careful to either
 * not use this value, or force a re-read by clearing the f_debug_defined
 * flag (although the Cassandra cache will also need a reset if we want
 * to really read the current value.)
 *
 * \return true if the PayPal system is currently setup in debug mode.
 */
bool epayment_paypal::get_debug()
{
    if(!f_debug_defined)
    {
        content::path_info_t settings_ipath;
        settings_ipath.set_path(get_name(SNAP_NAME_EPAYMENT_PAYPAL_SETTINGS_PATH));

        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(settings_ipath.get_revision_key()));

        // TODO: if backends require it, we want to add a reset of the
        //       revision_row before re-reading the debug flag here

        QtCassandra::QCassandraValue debug_value(revision_row->cell(get_name(SNAP_NAME_EPAYMENT_PAYPAL_DEBUG))->value());
        f_debug = !debug_value.nullValue() && debug_value.signedCharValue();
    }

    return f_debug;
}


/** \brief Get a current PayPal OAuth2 token.
 *
 * This function returns a currently valid OAuth2 token from the database
 * if available, or from PayPal if the one in the database timed out.
 *
 * Since the default timeout of an OAuth2 token from PayPal is 8h
 * (28800 seconds), we keep and share the token between all clients
 * (however, we do not share between websites since each website may
 * have a different client identifier and secret and thus there is
 * no point in trying to share between websites.)
 *
 * This means the same identifier may end up being used by many end
 * users within the 8h offered.
 *
 * \param[in,out] http  The HTTP request handler.
 * \param[out] token_type  Returns the type of OAuth2 used (i.e. "Bearer").
 * \param[out] access_token  Returns the actual OAuth2 cookie.
 * \param[in,out] secret_row  The row were invoice related secret data
 *                            is to be saved.
 *
 * \return true if the OAuth2 token is valid; false in all other cases.
 */
bool epayment_paypal::get_oauth2_token(http_client_server::http_client& http, std::string& token_type, std::string& access_token)
{
    // make sure token data is as expected by default
    token_type.clear();
    access_token.clear();

    // Save the authentication information in the paypal settings
    // (since it needs to be secret, use the secret table)
    content::path_info_t settings_ipath;
    settings_ipath.set_path(get_name(SNAP_NAME_EPAYMENT_PAYPAL_SETTINGS_PATH));

    content::content *content_plugin(content::content::instance());
    //QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    //QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(settings_ipath.get_revision_key()));
    QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
    QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(settings_ipath.get_key()));

    bool const debug(get_debug());

    // If there is a saved OAuth2 which is not out of date, use that
    QtCassandra::QCassandraValue secret_debug_value(secret_row->cell(get_name(SNAP_NAME_EPAYMENT_PAYPAL_DEBUG))->value());
    if(!secret_debug_value.nullValue()
    && (secret_debug_value.signedCharValue() != 0) == debug) // if debug flag changed, it's toasted
    {
        QtCassandra::QCassandraValue expires_value(secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_EXPIRES))->value());
        int64_t current_date(f_snap->get_current_date());
        if(!expires_value.nullValue()
        && expires_value.int64Value() > current_date) // we do not use 'start date' here because it could be wrong if the process was really slow
        {
            token_type = secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_TOKEN_TYPE))->value().stringValue().toUtf8().data();
            access_token = secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_ACCESS_TOKEN))->value().stringValue().toUtf8().data();
            return true;
        }
    }

    QString client_id;
    QString secret;

    if(debug)
    {
        // User setup debug mode for now
        client_id = secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SANDBOX_CLIENT_ID))->value().stringValue();
        secret = secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SANDBOX_SECRET))->value().stringValue();
    }
    else
    {
        // Normal user settings
        client_id = secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CLIENT_ID))->value().stringValue();
        secret = secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SECRET))->value().stringValue();
    }

    if(client_id.isEmpty()
    || secret.isEmpty())
    {
        messages::messages::instance()->set_error(
            "PayPal not Properly Setup",
            "Somehow this website PayPal settings are not complete.",
            "The client_id or secret parameters were not yet defined.",
            false
        );
        return false;
    }

    // get authorization code
    //
    // PayPal example:
    //   curl -v https://api.sandbox.paypal.com/v1/oauth2/token
    //     -H "Accept: application/json"
    //     -H "Accept-Language: en_US"
    //     -u "EOJ2S-Z6OoN_le_KS1d75wsZ6y0SFdVsY9183IvxFyZp:EClusMEUk8e9ihI7ZdVLF5cZ6y0SFdVsY9183IvxFyZp"
    //     -d "grant_type=client_credentials"
    //
    // Curl output (when using "--trace-ascii -" on the command line):
    //     0000: POST /v1/oauth2/token HTTP/1.1
    //     0020: Authorization: Basic RU9KMlMtWjZPb05fbGVfS1MxZDc1d3NaNnkwU0ZkVnN
    //     0060: ZOTE4M0l2eEZ5WnA6RUNsdXNNRVVrOGU5aWhJN1pkVkxGNWNaNnkwU0ZkVnNZOTE
    //     00a0: 4M0l2eEZ5WnA=
    //     00af: User-Agent: curl/7.35.0
    //     00c8: Host: api.sandbox.paypal.com
    //     00e6: Accept: application/json
    //     0100: Accept-Language: en_US
    //     0118: Content-Length: 29
    //     012c: Content-Type: application/x-www-form-urlencoded
    //     015d:
    //
    http_client_server::http_request authorization_request;
    authorization_request.set_host(debug ? "api.sandbox.paypal.com" : "https://api.paypal.com");
    //authorization_request.set_host("private.m2osw.com");
    authorization_request.set_path("/v1/oauth2/token");
    authorization_request.set_port(443); // https
    authorization_request.set_header("Accept", "application/json");
    authorization_request.set_header("Accept-Language", "en_US");
    //authorization_request.set_header("Content-Type", "application/x-www-form-urlencoded"); -- automatic
    //authorization_request.set_header("Authorization", "Basic " + base64_authorization_token.data());
    authorization_request.set_basic_auth(client_id.toUtf8().data(), secret.toUtf8().data());
    authorization_request.set_post("grant_type", "client_credentials");
    //authorization_request.set_body(...);
    http_client_server::http_response::pointer_t response(http.send_request(authorization_request));

    // we need a successful response
    if(response->get_response_code() != 200)
    {
        SNAP_LOG_ERROR("OAuth2 request failed");
        throw epayment_paypal_exception_io_error("OAuth2 request failed");
    }

    // the response type must be application/json
    if(!response->has_header("content-type")
    || response->get_header("content-type") != "application/json")
    {
        SNAP_LOG_ERROR("OAuth2 request did not return application/json data");
        throw epayment_paypal_exception_io_error("OAuth2 request did not return application/json data");
    }

    // save that info in case of failure we may have a chance to check
    // what went wrong
    signed char debug_flag(debug ? 1 : 0);
    secret_row->cell(get_name(SNAP_NAME_EPAYMENT_PAYPAL_DEBUG))->setValue(debug_flag);
    secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_HEADER))->setValue(QString::fromUtf8(response->get_original_header().c_str()));
    secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_DATA))->setValue(QString::fromUtf8(response->get_response().c_str()));

    // looks pretty good...
    as2js::JSON::pointer_t json(new as2js::JSON);
    as2js::StringInput::pointer_t in(new as2js::StringInput(response->get_response()));
    as2js::JSON::JSONValue::pointer_t value(json->parse(in));
    if(!value)
    {
        SNAP_LOG_ERROR("JSON parser failed parsing oauth response");
        throw epayment_paypal_exception_io_error("JSON parser failed parsing oauth response");
    }
    as2js::JSON::JSONValue::object_t object(value->get_object());

    // TOKEN TYPE
    // we should always have a token_type
    if(object.find("token_type") == object.end())
    {
        SNAP_LOG_ERROR("oauth token_type missing");
        throw epayment_paypal_exception_io_error("oauth token_type missing");
    }
    // at this point we expect "Bearer", but we assume it could change
    // since they are sending us a copy of that string
    token_type = object["token_type"]->get_string().to_utf8();
    secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_TOKEN_TYPE))->setValue(QString::fromUtf8(token_type.c_str()));

    // ACCESS TOKEN
    // we should always have an access token
    if(object.find("access_token") == object.end())
    {
        SNAP_LOG_ERROR("oauth access_token missing");
        throw epayment_paypal_exception_io_error("oauth access_token missing");
    }
    access_token = object["access_token"]->get_string().to_utf8();
    secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_ACCESS_TOKEN))->setValue(QString::fromUtf8(access_token.c_str()));

    // EXPIRES IN
    // get the amount of time the token will last in seconds
    if(object.find("expires_in") == object.end())
    {
        SNAP_LOG_ERROR("oauth expires_in missing");
        throw epayment_paypal_exception_io_error("oauth expires_in missing");
    }
    // if defined, "expires_in" is an integer
    int64_t const expires(object["expires_in"]->get_int64().get());
    int64_t const start_date(f_snap->get_start_date());
    // we save an absolute time limit instead of a "meaningless" number of seconds
    secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_EXPIRES))->setValue(start_date + expires * 1000000);

    // SCOPE
    // get the scope if available (for info at this point)
    if(object.find("scope") != object.end())
    {
        std::string const scope(object["scope"]->get_string().to_utf8());
        secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_SCOPE))->setValue(QString::fromUtf8(scope.c_str()));
    }

    // APP ID
    // get the application ID if available
    if(object.find("app_id") != object.end())
    {
        std::string const app_id(object["app_id"]->get_string().to_utf8());
        secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_APP_ID))->setValue(QString::fromUtf8(app_id.c_str()));
    }

    return true;
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
 * TBD: will we have some form of session in the cart? Probably
 * not here however...
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void epayment_paypal::on_process_post(QString const& uri_path)
{
    // make sure this is a cart post
    char const *cart_products(get_name(SNAP_NAME_EPAYMENT_PAYPAL_CLICKED_POST_FIELD));
    if(!f_snap->postenv_exists(cart_products))
    {
        return;
    }

    content::path_info_t ipath;
    ipath.set_path(uri_path);

    uint64_t invoice_number(0);
    content::path_info_t invoice_ipath;
    epayment::epayment::instance()->generate_invoice(invoice_ipath, invoice_number);

    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
    QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(invoice_ipath.get_key()));
    QtCassandra::QCassandraTable::pointer_t epayment_paypal_table(get_epayment_paypal_table());

std::cerr << "***\n*** HERE: " << invoice_number << "\n***\n";

    //
    // Documentation directly in link with the following:
    //    https://developer.paypal.com/webapps/developer/docs/integration/web/accept-paypal-payment/
    //

    // first we need to "log in", which PayPal calls
    //     "an authorization token"
    http_client_server::http_client http;
    http.set_keep_alive(true);

    std::string token_type;
    std::string access_token;
    if(!get_oauth2_token(http, token_type, access_token))
    {
        return;
    }

    QString redirect_url;
    bool found_execute(false);
    {
        // create a sales payment
        //
        // PayPal example:
        //      curl -v https://api.sandbox.paypal.com/v1/payments/payment
        //          -H 'Content-Type: application/json'
        //          -H 'Authorization: Bearer <Access-Token>'
        //          -d '{
        //            "intent":"sale",
        //            "redirect_urls":{
        //              "return_url":"http://example.com/your_redirect_url.html",
        //              "cancel_url":"http://example.com/your_cancel_url.html"
        //            },
        //            "payer":{
        //              "payment_method":"paypal"
        //            },
        //            "transactions":[
        //              {
        //                "amount":{
        //                  "total":"7.47",
        //                  "currency":"USD"
        //                }
        //              }
        //            ]
        //          }'
        //
        // Sample answer:
        //      [
        //          {
        //              "id":"PAY-1234567890",
        //              "create_time":"2014-12-28T11:31:56Z",
        //              "update_time":"2014-12-28T11:31:56Z",
        //              "state":"created",
        //              "intent":"sale",
        //              "payer":
        //              {
        //                  "payment_method":"paypal",
        //                  "payer_info": {
        //                      "shipping_address": {
        //                      }
        //                  }
        //              },
        //              "transactions": [
        //                  {
        //                      "amount": {
        //                          "total":"111.34",
        //                          "currency":"USD",
        //                          "details": {
        //                              "subtotal":"111.34"
        //                          }
        //                      },
        //                      "description":"Hello from Snap! Websites",
        //                      "related_resources": [
        //                      ]
        //                  }
        //              ],
        //              "links": [
        //                  {
        //                      "href":"https://api.sandbox.paypal.com/v1/payments/payment/PAY-1234567890",
        //                      "rel":"self",
        //                      "method":"GET"
        //                  },
        //                  {
        //                      "href":"https://www.sandbox.paypal.com/cgi-bin/webscr?cmd=_express-checkout&token=EC-12345",
        //                      "rel":"approval_url",
        //                      "method":"REDIRECT"
        //                  },
        //                  {
        //                      "href":"https://api.sandbox.paypal.com/v1/payments/payment/PAY-1234567890/execute",
        //                      "rel":"execute",
        //                      "method":"POST"
        //                  }
        //              ]
        //          }
        //      ]
        //    

        // create the body first so we can save its length in the header
        content::path_info_t return_url;
        return_url.set_path(get_name(SNAP_NAME_EPAYMENT_PAYPAL_RETURN_URL));
        content::path_info_t cancel_url;
        cancel_url.set_path(get_name(SNAP_NAME_EPAYMENT_PAYPAL_CANCEL_URL));
        QString const body(QString(
                    "{"
                        "\"intent\":\"sale\","
                        // TODO: we need e-Payment PayPal defined return URLs...
                        "\"redirect_urls\":{"
                            "\"return_url\":\"%1\","
                            "\"cancel_url\":\"%2\""
                        "},"
                        "\"payer\":{"
                            "\"payment_method\":\"paypal\""
                        "},"
                        // TODO: Got to make use of our cart total & currency
                        "\"transactions\":["
                            "{"
                                "\"amount\":{"
                                    "\"total\":\"111.34\","
                                    "\"currency\":\"USD\""
                                "},"
                                "\"description\":\"Hello from Snap! Websites\""
                            "}"
                        "]"
                    "}"
                ).arg(return_url.get_key())
                 .arg(cancel_url.get_key())
            );

        http_client_server::http_request payment_request;
        bool const debug(get_debug());
        payment_request.set_host(debug ? "api.sandbox.paypal.com" : "api.paypal.com");
        //payment_request.set_host("private.m2osw.com");
        payment_request.set_path("/v1/payments/payment");
        payment_request.set_port(443); // https
        payment_request.set_header("Accept", "application/json");
        payment_request.set_header("Accept-Language", "en_US");
        payment_request.set_header("Content-Type", "application/json");
        payment_request.set_header("Authorization", QString("%1 %2").arg(token_type.c_str()).arg(access_token.c_str()).toUtf8().data());
        payment_request.set_header("PayPal-Request-Id", invoice_ipath.get_key().toUtf8().data());
        payment_request.set_data(body.toUtf8().data());
        //payment_request.set_body(...);
SNAP_LOG_ERROR("*** Sending payment request now... ***");
        http_client_server::http_response::pointer_t response(http.send_request(payment_request));

        secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT_HEADER))->setValue(QString::fromUtf8(response->get_original_header().c_str()));
        secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT))->setValue(QString::fromUtf8(response->get_response().c_str()));

        // we need a successful response
        if(response->get_response_code() != 200
        && response->get_response_code() != 201)
        {
            SNAP_LOG_ERROR("creating a sale payment failed");
            throw epayment_paypal_exception_io_error("creating a sale payment failed");
        }

        // the response type must be application/json
        if(!response->has_header("content-type")
        || response->get_header("content-type") != "application/json")
        {
            SNAP_LOG_ERROR("sale request did not return application/json data");
            throw epayment_paypal_exception_io_error("sale request did not return application/json data");
        }

        // looks pretty good...
        as2js::JSON::pointer_t json(new as2js::JSON);
        as2js::StringInput::pointer_t in(new as2js::StringInput(response->get_response()));
        as2js::JSON::JSONValue::pointer_t value(json->parse(in));
        if(!value)
        {
            SNAP_LOG_ERROR("JSON parser failed parsing sale response");
            throw epayment_paypal_exception_io_error("JSON parser failed parsing sale response");
        }
SNAP_LOG_ERROR("*** Ready to get sale object... ***");
        as2js::JSON::JSONValue::object_t object(value->get_object());
SNAP_LOG_ERROR("*** Got top sale object... ***");

        // STATE
        //
        // the state should be "created" at this point
        if(object.find("state") == object.end())
        {
            SNAP_LOG_ERROR("payment status missing");
            throw epayment_paypal_exception_io_error("payment status missing");
        }
        if(object["state"]->get_string() != "created")
        {
            SNAP_LOG_ERROR("paypal payment status is not \"created\" as expected");
            throw epayment_paypal_exception_io_error("paypal payment status is not \"created\" as expected");
        }

        // INTENT
        //
        // verify the intent if defined
        if(object.find("intent") != object.end())
        {
            // "intent" should always be defined, we expect it to be "sale"
            if(object["intent"]->get_string() != "sale")
            {
                SNAP_LOG_ERROR("paypal payment status is not \"created\" as expected");
                throw epayment_paypal_exception_io_error("paypal payment status is not \"created\" as expected");
            }
        }

        // ID
        //
        // get the "id" (also called "paymentId")
        if(object.find("id") == object.end())
        {
            SNAP_LOG_ERROR("payment identifier missing");
            throw epayment_paypal_exception_io_error("payment identifier missing");
        }
        as2js::String const id_string(object["id"]->get_string());
        QString const id(QString::fromUtf8(id_string.to_utf8().c_str()));
        secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_ID))->setValue(id);

        // save a back reference in the epayment_paypal table
        snap_uri const main_uri(f_snap->get_uri());
        epayment_paypal_table->row(main_uri.full_domain())->cell(id)->setValue(invoice_ipath.get_key());

        // LINKS
        //
        // get the "links"
        if(object.find("links") == object.end())
        {
            SNAP_LOG_ERROR("payment identifier missing");
            throw epayment_paypal_exception_io_error("payment identifier missing");
        }
        as2js::JSON::JSONValue::array_t const links(object["links"]->get_array());
        size_t const max_links(links.size());
        for(size_t idx(0); idx < max_links; ++idx)
        {
            as2js::JSON::JSONValue::object_t link_object(links[idx]->get_object());
            if(link_object.find("rel") != link_object.end())
            {
                as2js::String const rel(link_object["rel"]->get_string());
                if(rel == "approval_url")
                {
                    // this is it! the URL to send the user to
                    // the method has to be REDIRECT
                    if(link_object.find("method") == link_object.end())
                    {
                        SNAP_LOG_ERROR("paypal link \"approval_url\" has no \"method\" parameter");
                        throw epayment_paypal_exception_io_error("paypal link \"approval_url\" has no \"method\" parameter");
                    }
                    if(link_object["method"]->get_string() != "REDIRECT")
                    {
                        SNAP_LOG_ERROR("paypal link \"approval_url\" has a \"method\" other than \"REDIRECT\"");
                        throw epayment_paypal_exception_io_error("paypal link \"approval_url\" has a \"method\" other than \"REDIRECT\"");
                    }
                    if(link_object.find("href") == link_object.end())
                    {
                        SNAP_LOG_ERROR("paypal link \"approval_url\" has no \"href\" parameter");
                        throw epayment_paypal_exception_io_error("paypal link \"approval_url\" has no \"href\" parameter");
                    }
                    as2js::String const href(link_object["href"]->get_string());
                    redirect_url = QString::fromUtf8(href.to_utf8().c_str());
                }
                else if(rel == "execute")
                {
                    // this is it! the URL to send the user to
                    // the method has to be POST
                    if(link_object.find("method") == link_object.end())
                    {
                        SNAP_LOG_ERROR("paypal link \"execute\" has no \"method\" parameter");
                        throw epayment_paypal_exception_io_error("paypal link \"execute\" has no \"method\" parameter");
                    }
                    if(link_object["method"]->get_string() != "POST")
                    {
                        SNAP_LOG_ERROR("paypal link \"execute\" has a \"method\" other than \"POST\"");
                        throw epayment_paypal_exception_io_error("paypal link \"execute\" has a \"method\" other than \"POST\"");
                    }
                    if(link_object.find("href") == link_object.end())
                    {
                        SNAP_LOG_ERROR("paypal link \"approval_url\" has no \"href\" parameter");
                        throw epayment_paypal_exception_io_error("paypal link \"approval_url\" has no \"href\" parameter");
                    }
                    as2js::String href(link_object["href"]->get_string());
                    secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTE_PAYMENT))->setValue(QString::fromUtf8(href.to_utf8().c_str()));
                    found_execute = true;
                }
            }
        }
    }

#if 0
    QString const paypal_payment(QString(
                "GET /v1/payments/payment HTTP/1.1\r\n"
                "Host: private.m2osw.com\r\n"
                "User-Agent: snapwebsites/" SNAPWEBSITES_VERSION_STRING "\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %1\r\n"
                "Accept: application/json\r\n"
                "Accept-Encoding: gzip,deflate\r\n"
                "Authorization: Basic %2\r\n"
                "PayPal-Request-Id: %3\r\n"
                "\r\n"
                "%4"
            )
            .arg(body.length())
            .arg(access_token.constData())
            .arg(invoice_ipath.get_key())
            .arg(body)
        );
    std::string payment_order(paypal_payment.toUtf8().data());

    // TODO: switch between sandbox and full API depending on the debug
    //       flag as defined in the settings
    QString redirect_url;
    int response_code(500);
    QString response_message("No Response");
    bool found_execute(false);
    for(int attempts(0); attempts < 3; ++attempts)
    {
        try
        {
            tcp_client_server::bio_client secure_tcp(, 443, tcp_client_server::bio_client::mode_t::MODE_ALWAYS_SECURE);
            //tcp_client_server::bio_client secure_tcp("private.m2osw.com", 443, tcp_client_server::bio_client::mode_t::MODE_ALWAYS_SECURE);
            secure_tcp.write(payment_order.c_str(), payment_order.length());

            // read the response from the PayPal server
            //
            // since this is HTTP, we want to read the header as such because
            // it may include a Content-Size which has to be taken in account
            // (in case the server decides to keep the connection open)
            QString header;
            {
                // we only accept HTTP/1.0 and HTTP/1.1 as protocol
                std::string protocol;
                int const r(secure_tcp.read_line(protocol));
                if(r < 0)
                {
                    SNAP_LOG_ERROR("read I/O error while reading protocol");
                    throw epayment_paypal_exception_io_error("read I/O error while reading protocol");
                }
                if(protocol.length() > 0
                && *protocol.rbegin() == '\r')
                {
                    // remove the '\r' if present (should be)
                    protocol.erase(protocol.end() - 1);
                }
                QString const p(QString::fromUtf8(protocol.c_str()));
                // keep all the header fields in 'header'
                header += p;
                QStringList l(p.split(' '));
                if(l.size() < 2)
                {
                    SNAP_LOG_ERROR("read I/O error while reading protocol: \"")(protocol)("\".");
                    throw epayment_paypal_exception_io_error(QString("read I/O error while reading protocol: \"%1\".").arg(p));
                }
std::cerr << "*** Got protocol: [" << l[0] << "] / [" << l[1] << "] / [" << l[2] << "]\n";
                if(l[0] != "HTTP/1.1"
                && l[0] != "HTTP/1.0")
                {
                    SNAP_LOG_ERROR("read error, unknown protocol: \"")(protocol)("\".");
                    throw epayment_paypal_exception_io_error(QString("read error, unknown protocol: \"%1\".").arg(p));
                }
                bool ok(false);
                response_code = l[1].toInt(&ok, 10);
                l.pop_front();
                l.pop_front();
                response_message = l.join(" ").trimmed();
                // although the response may be an error, we still want to
                // read the entire HTTP response before disconnecting
            }
            size_t content_length(0);
            for(;;)
            {
                std::string field;
                int const r(secure_tcp.read_line(field));
                if(r < 0)
                {
                    SNAP_LOG_ERROR("read I/O error while reading header");
                    throw epayment_paypal_exception_io_error("read I/O error while reading header");
                }
                if(*field.rbegin() == '\r')
                {
                    // remove the '\r' if present (should be)
                    field.erase(field.end() - 1);
                }
                if(field.empty()) // r may be 0 or (more likely) 1
                {
                    // found the empty line after the header
                    break;
                }
                QString f(QString::fromUtf8(field.c_str()));
SNAP_LOG_ERROR("got a header field: ")(field);
                // keep all the header fields in 'header'
                header += f;
                int const pos(f.indexOf(':'));
                if(pos < 0)
                {
                    SNAP_LOG_ERROR("invalid header field read, colon (:) missing");
                    throw epayment_paypal_exception_io_error("invalid header field read, colon (:) missing");
                }
                QString const name(f.left(pos).trimmed().toLower());
                if(name.isEmpty())
                {
                    SNAP_LOG_ERROR("invalid header field read, no name found");
                    throw epayment_paypal_exception_io_error("invalid header field read, no name found");
                }
                QString const value(f.mid(pos + 1).trimmed());
SNAP_LOG_ERROR("with value: ")(value);
                if(name.startsWith("content-length"))
                {
                    bool ok(false);
                    content_length = value.toInt(&ok, 10);
                    if(!ok)
                    {
                        SNAP_LOG_ERROR("invalid Content-Length header field, expected a valid decimal number, not \"")(value)("\".");
                        throw epayment_paypal_exception_io_error(QString("invalid Content-Length header field, expected a valid decimal number, not \"%1\".").arg(value));
                    }
SNAP_LOG_ERROR("  +---> content length: ")(content_length);
                }
                else if(name == "content-type")
                {
                    if(!value.startsWith("application/json"))
                    {
                        SNAP_LOG_ERROR("invalid Content-Type header field, expected \"application/json\", not \"")(value)("\".");
                        //throw epayment_paypal_exception_io_error(QString("invalid Content-Type header field, expected \"application/json\", not \"%1\".").arg(value));
                    }
                }
            }
            // When PayPal replies with 401, the response body is empty
            std::string response;
            if(content_length > 0)
            {
                // TBD: should we read the response 1024 bytes at a time
                //      (or some other magical size like BUFSIZ)
                //      I think it will always be relatively small so we
                //      probably do not need to at this point...
                std::vector<char> buffer;
                buffer.resize(content_length);
SNAP_LOG_ERROR("read response...");
                int const rb(secure_tcp.read(&buffer[0], content_length));
                if(rb < 0)
                {
                    SNAP_LOG_ERROR("read I/O error while reading response body");
                    throw epayment_paypal_exception_io_error("read I/O error while reading response body");
                }
                response = std::string(&buffer[0], content_length);
            }
SNAP_LOG_ERROR("got response: [")(response)("]");

            // TODO: we may want to save that data in another table that the
            //       user filters cannot easily access so it is more secure
            secret_table->row(invoice_ipath.get_key())->cell(get_name(SNAP_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT_HEADER))->setValue(header);
            secret_table->row(invoice_ipath.get_key())->cell(get_name(SNAP_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT))->setValue(QString::fromUtf8(response.c_str()));

            // if we get a response other than 200, there is really no
            // need to check the JSON (we likely did not receive a JSON!)
            if(response_code != 200)
            {
                break;
            }

            as2js::JSON::pointer_t json(new as2js::JSON);
            as2js::StringInput::pointer_t in(new as2js::StringInput(response));
            as2js::JSON::JSONValue::pointer_t value(json->parse(in));
            if(!value)
            {
                SNAP_LOG_ERROR("JSON parser failed parsing payment response");
                throw epayment_paypal_exception_io_error("JSON parser failed parsing payment response");
            }
SNAP_LOG_ERROR("*** Ready to get object... ***");
            as2js::JSON::JSONValue::object_t object(value->get_object());
SNAP_LOG_ERROR("*** Got top object... ***");
            // the state should be "created" at this point
            if(object.find("state") == object.end())
            {
                SNAP_LOG_ERROR("payment status missing");
                throw epayment_paypal_exception_io_error("payment status missing");
            }
            if(object["state"]->get_string() != "created")
            {
                SNAP_LOG_ERROR("paypal payment status is not \"created\" as expected");
                throw epayment_paypal_exception_io_error("paypal payment status is not \"created\" as expected");
            }
            // verify the intent if defined
            if(object.find("intent") != object.end())
            {
                // "intent" should always be defined, we expect it to be "sale"
                if(object["intent"]->get_string() != "sale")
                {
                    SNAP_LOG_ERROR("paypal payment status is not \"created\" as expected");
                    throw epayment_paypal_exception_io_error("paypal payment status is not \"created\" as expected");
                }
            }
            // get the "id"
            if(object.find("id") == object.end())
            {
                SNAP_LOG_ERROR("payment identifier missing");
                throw epayment_paypal_exception_io_error("payment identifier missing");
            }
            as2js::String const id(object["id"]->get_string());
            // get the "links"
            if(object.find("links") == object.end())
            {
                SNAP_LOG_ERROR("payment identifier missing");
                throw epayment_paypal_exception_io_error("payment identifier missing");
            }
            as2js::JSON::JSONValue::array_t const links(object["links"]->get_array());
            size_t const max_links(links.size());
            for(size_t idx(0); idx < max_links; ++idx)
            {
                as2js::JSON::JSONValue::object_t link_object(links[idx]->get_object());
                if(link_object.find("rel") != link_object.end())
                {
                    as2js::String const rel(link_object["rel"]->get_string());
                    if(rel == "approval_url")
                    {
                        // this is it! the URL to send the user to
                        // the method has to be REDIRECT
                        if(link_object.find("method") == link_object.end())
                        {
                            SNAP_LOG_ERROR("paypal link \"approval_url\" has no \"method\" parameter");
                            throw epayment_paypal_exception_io_error("paypal link \"approval_url\" has no \"method\" parameter");
                        }
                        if(link_object["method"]->get_string() != "REDIRECT")
                        {
                            SNAP_LOG_ERROR("paypal link \"approval_url\" has a \"method\" other than \"REDIRECT\"");
                            throw epayment_paypal_exception_io_error("paypal link \"approval_url\" has a \"method\" other than \"REDIRECT\"");
                        }
                        if(link_object.find("href") == link_object.end())
                        {
                            SNAP_LOG_ERROR("paypal link \"approval_url\" has no \"href\" parameter");
                            throw epayment_paypal_exception_io_error("paypal link \"approval_url\" has no \"href\" parameter");
                        }
                        as2js::String const href(link_object["href"]->get_string());
                        redirect_url = QString::fromUtf8(href.to_utf8().c_str());
                    }
                    else if(rel == "execute")
                    {
                        // this is it! the URL to send the user to
                        // the method has to be POST
                        if(link_object.find("method") == link_object.end())
                        {
                            SNAP_LOG_ERROR("paypal link \"execute\" has no \"method\" parameter");
                            throw epayment_paypal_exception_io_error("paypal link \"execute\" has no \"method\" parameter");
                        }
                        if(link_object["method"]->get_string() != "POST")
                        {
                            SNAP_LOG_ERROR("paypal link \"execute\" has a \"method\" other than \"POST\"");
                            throw epayment_paypal_exception_io_error("paypal link \"execute\" has a \"method\" other than \"POST\"");
                        }
                        if(link_object.find("href") == link_object.end())
                        {
                            SNAP_LOG_ERROR("paypal link \"approval_url\" has no \"href\" parameter");
                            throw epayment_paypal_exception_io_error("paypal link \"approval_url\" has no \"href\" parameter");
                        }
                        as2js::String href(link_object["href"]->get_string());
                        secret_table->row(invoice_ipath.get_key())->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTE_PAYMENT))->setValue(QString::fromUtf8(href.to_utf8().c_str()));
                        found_execute = true;
                    }
                }
            }
            // done, exit the retry loop
            break;
        }
        catch(std::runtime_error const& e)
        {
            SNAP_LOG_ERROR("Got a failed attempt at connecting with remote server or parsing the results. Error: ")(e.what());
        }
        catch(...)
        {
            // TODO: we probably only want to catch the specific exceptions
            //       we know mean we can retry; some are certainly deadly
            //       and should stop the whole process
            //
            // keep trying instead of ending dramatically
        }
        struct timespec require;
        struct timespec remember;
        require.tv_sec = 0;
        require.tv_nsec = 500000000;  // wait 500ms
        remember.tv_sec = 0;
        remember.tv_nsec = 0;
        nanosleep(&require, &remember);
    }
    if(response_code != 200)
    {
        SNAP_LOG_ERROR("server did not like our request, response code was not 200 but ")(response_code)(" instead.");
        throw epayment_paypal_exception_io_error(QString("server did not like our request, response code was not 200 but \"%1\" instead.").arg(response_code));
    }
#endif

    if(redirect_url.isEmpty())
    {
        throw epayment_paypal_exception_io_error("paypal redirect URL (\"approval_url\") was not found");
    }
    if(!found_execute)
    {
        throw epayment_paypal_exception_io_error("paypal execute URL (\"execute\") was not found");
    }

    // create the AJAX response
    server_access::server_access *server_access_plugin(server_access::server_access::instance());
    server_access_plugin->create_ajax_result(ipath, invoice_number != 0);
    server_access_plugin->ajax_redirect(redirect_url);
    server_access_plugin->ajax_output();
}


// PayPal REST documentation at time of writing
//   https://developer.paypal.com/webapps/developer/docs/api/

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
