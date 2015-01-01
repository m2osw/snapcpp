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
#include "../output/output.h"

#include "log.h"
#include "not_reached.h"
#include "tcp_client_server.h"

#include <as2js/json.h>

#include <iostream>
#include <openssl/rand.h>

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
        return "epayment/paypal/ready";

    case SNAP_NAME_EPAYMENT_PAYPAL_SETTINGS_PATH:
        return "/admin/settings/epayment/paypal";

    case SNAP_NAME_EPAYMENT_PAYPAL_TABLE:
        return "epayment_paypal";

    case SNAP_NAME_EPAYMENT_PAYPAL_TOKEN_POST_FIELD:
        return "epayment__epayment_paypal_token";


    // ******************
    //    SECURE NAMES
    // ******************
    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CLIENT_ID:
        return "epayment_paypal::client_id";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT:
        return "epayment_paypal::created_payment";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT_HEADER:
        return "epayment_paypal::created_payment_header";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTED_PAYMENT:
        return "epayment_paypal::executed_payment_response";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTED_PAYMENT_HEADER:
        return "epayment_paypal::executed_payment_header";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTE_PAYMENT:
        return "epayment_paypal::execute_payment";

    case SNAP_SECURE_NAME_EPAYMENT_PAYPAL_INVOICE_SECRET_ID:
        return "epayment_paypal::invoice_secret_id";

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
 * by registering for various events.
 *
 * \param[in] snap  The child handling this request.
 */
void epayment_paypal::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(epayment_paypal, "server", server, process_post, _1);
    SNAP_LISTEN(epayment_paypal, "layout", layout::layout, generate_header_content, _1, _2, _3, _4);
    SNAP_LISTEN(epayment_paypal, "filter", filter::filter, replace_token, _1, _2, _3, _4);
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
    SNAP_PLUGIN_UPDATE(2014, 12, 30, 22, 45, 40, content_update);

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


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the \<page\> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  The path to a template page in case cpath is not defined.
 */
void epayment_paypal::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, const QString& ctemplate)
{
    // our pages are like any standard pages
    output::output::instance()->on_generate_main_content(ipath, page, body, ctemplate);
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
bool epayment_paypal::on_path_execute(content::path_info_t& ipath)
{
    QString const cpath(ipath.get_cpath());
std::cerr << "***\n*** cpath = [" << cpath << "]\n***\n";
    if(cpath == get_name(SNAP_NAME_EPAYMENT_PAYPAL_CANCEL_URL))
    {
        QtCassandra::QCassandraTable::pointer_t epayment_paypal_table(get_epayment_paypal_table());

        // the user canceled that invoice...
        //
        // http://www.your-domain.com/epayment/paypal/return?token=EC-123
        snap_uri const main_uri(f_snap->get_uri());
        if(!main_uri.has_query_option("token"))
        {
            messages::messages::instance()->set_error(
                "PayPal Missing Option",
                "PayPal returned to cancel invoice without a token parameter", 
                "Without the \"token\" parameter we cannot know which invoice this is linked with.",
                false
            );
        }
        else
        {
            QString const token(main_uri.query_option("token"));

            cancel_invoice(token);
        }
    }
    else if(cpath == get_name(SNAP_NAME_EPAYMENT_PAYPAL_RETURN_URL))
    {
        QtCassandra::QCassandraTable::pointer_t epayment_paypal_table(get_epayment_paypal_table());

        for(;;)
        {
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
                break;
            }

            QString const id(main_uri.query_option("paymentId"));
std::cerr << "*** paymentId is [" << id << "] [" << main_uri.full_domain() << "]\n";
            QString const invoice(epayment_paypal_table->row(main_uri.full_domain())->cell("id/" + id)->value().stringValue());
            content::path_info_t invoice_ipath;
            invoice_ipath.set_path(invoice);

            epayment::epayment *epayment_plugin(epayment::epayment::instance());

            // TODO: add a test to see whether the invoice has already been
            //       accepted, if so running the remainder of the code here
            //       may not be safe (i.e. this would happen if the user hits
            //       Reload on his browser.)
            epayment::name_t status(epayment_plugin->get_invoice_status(invoice_ipath));
            if(status != epayment::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING)
            {
                // TODO: support a default page in this case if the user is
                //       the correct user (this is only for people who hit
                //       reload, so no big deal right now)
                messages::messages::instance()->set_error(
                    "PayPal Processed",
                    "PayPal invoice was already processed. Please go to your account to view your existing invoices.", 
                    QString("Found the invoice, but somehow it is not marked \"pending\" (it is \"%1\" instead).").arg(epayment::get_name(status)),
                    false
                );
                break;
            }

            // Now get the payer identifier
            if(!main_uri.has_query_option("PayerID"))
            {
                messages::messages::instance()->set_error(
                    "PayPal Missing Option",
                    "PayPal replied without a paymentId parameter", 
                    "Without the \"paymentId\" parameter we cannot know which invoice this is linked with.",
                    false
                );
                break;
            }
            QString const payer_id(main_uri.query_option("PayerID"));

            content::content *content_plugin(content::content::instance());
            QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
            QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
            QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(invoice_ipath.get_key()));

            // save the PayerID value
            secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYER_ID))->setValue(payer_id);

            // Optionally, we may get a token that we check, just in case
            // (for PayPal payments this token is not used at this time)
            if(main_uri.has_query_option("token"))
            {
                // do we have a match?
                QString const token(main_uri.query_option("token"));
                QString const expected_token(secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_TOKEN))->value().stringValue());
                if(expected_token != token)
                {
                    messages::messages::instance()->set_error(
                        "Invalid Token",
                        "Somehow the token identifier returned by PayPal was not the same as the one saved in your purchase. We cannot proceed with your payment.", 
                        QString("The payment token did not match (expected \"%1\", got \"%2\").").arg(expected_token).arg(token),
                        false
                    );
                    break;
                }
            }

            // Finally verify that the user is still the same guy using
            // our cookie
            users::users *users_plugin(users::users::instance());
            QString const saved_id(users_plugin->detach_from_session(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_ID)));
            if(saved_id != id)
            {
                messages::messages::instance()->set_error(
                    "Invalid Identifier",
                    "Somehow the payment identifier returned by PayPal was not the same as the one saved in your session.", 
                    "If the identifiers do not match, we cannot show that user the corresponding cart if the user is not logged in.",
                    false
                );
                break;
            }

            // TODO: add settings so the administrator can choose to setup
            //       the amount of time to or or less than 1 day
            int64_t const invoice_created(content_table->row(invoice_ipath.get_key())->cell(content::get_name(content::SNAP_NAME_CONTENT_CREATED))->value().safeInt64Value());
            int64_t const start_date(f_snap->get_start_date());
            if(start_date > invoice_created + 86400000000LL) // 1 day in micro seconds
            {
                messages::messages::instance()->set_error(
                    "Session Timedout",
                    "You generated this payment more than a day ago. It timed out. Sorry about the trouble, but you have to start your order over.", 
                    "The invoice was created 1 day ago so this could be a hacker trying to get this invoice validated.",
                    false
                );
            }
            break;
        }
    }

    // output the page as the output plugin otherwise would by itself
    //
    // TBD: We may want to display an error page instead whenever the
    //      process fails in some way
    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return true;
}


void epayment_paypal::cancel_invoice(QString const& token)
{
    QtCassandra::QCassandraTable::pointer_t epayment_paypal_table(get_epayment_paypal_table());
    snap_uri const main_uri(f_snap->get_uri());
    QString const invoice(epayment_paypal_table->row(main_uri.full_domain())->cell("token/" + token)->value().stringValue());
    content::path_info_t invoice_ipath;
    invoice_ipath.set_path(invoice);

    epayment::epayment *epayment_plugin(epayment::epayment::instance());

    // the current state must be pending for us to cancel anythying
    epayment::name_t status(epayment_plugin->get_invoice_status(invoice_ipath));
    if(status != epayment::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING)
    {
        // TODO: support a default page in this case if the user is
        //       the correct user (this is only for people who hit
        //       reload, so no big deal right now)
        messages::messages::instance()->set_error(
            "PayPal Processed",
            "PayPal invoice was already processed. Please go to your account to view your existing invoices.", 
            QString("Found the invoice, but somehow it is not marked \"pending\" (it is \"%1\" instead).").arg(epayment::get_name(status)),
            false
        );
        return;
    }

    epayment_plugin->set_invoice_status(invoice_ipath, epayment::SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED);

    // we can show this invoice to the user, the status will appear
    // those the user can see it was canceled
}


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
        SNAP_LOG_ERROR("JSON parser failed parsing 'oauth2' response");
        throw epayment_paypal_exception_io_error("JSON parser failed parsing 'oauth2' response");
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
    char const *clicked_post_field(get_name(SNAP_NAME_EPAYMENT_PAYPAL_CLICKED_POST_FIELD));
    if(!f_snap->postenv_exists(clicked_post_field))
    {
        return;
    }

    // get the value to determine which button was clicked
    QString const click(f_snap->postenv(clicked_post_field));
    QString redirect_url;
    bool success(true);

    content::path_info_t ipath;
    ipath.set_path(uri_path);

    if(click == "checkout")
    {
        // "checkout" -- the big PayPal button in the Checkout screen
        //               we start a payment with PayPal
        uint64_t invoice_number(0);
        content::path_info_t invoice_ipath;
        epayment::epayment *epayment_plugin(epayment::epayment::instance());
        epayment_plugin->generate_invoice(invoice_ipath, invoice_number);
        epayment_plugin->set_invoice_status(invoice_ipath, epayment::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING);
        success = invoice_number != 0;

        content::content *content_plugin(content::content::instance());
        users::users *users_plugin(users::users::instance());

        QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
        QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(invoice_ipath.get_key()));
        QtCassandra::QCassandraTable::pointer_t epayment_paypal_table(get_epayment_paypal_table());

        // TODO: this will not work, it has to be in the epayment plugin because
        //       if we are to allow users to come back to view one of their
        //       invoices without having an account, it has to be with any one
        //       payment facility and not with a particular one
        //
        // generate a random identifier to safely link the invoice to the user
        //unsigned char rnd[16];
        //int const r(RAND_bytes(rnd, sizeof(rnd)));
        //if(r != 1)
        //{
        //    throw epayment_paypal_exception_io_error("RAND_bytes() could not generate a random number.");
        //}
        //QtCassandra::QCassandraValue rnd_value(reinterpret_cast<char const *>(rnd), static_cast<int>(sizeof(rnd)));
        //secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_INVOICE_SECRET_ID))->setValue(rnd_value);
        //QString const rnd_hex(rnd_value.binaryValue().toHex());
        //users_plugin->attach_to_session(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_INVOICE_SECRET_ID), rnd_hex);
        //http_cookie cookie(f_snap, "snap_epayment_paypal", rnd_hex);
        //f_snap->set_cookie(cookie);

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
            payment_request.set_path("/v1/payments/payment");
            payment_request.set_port(443); // https
            payment_request.set_header("Accept", "application/json");
            payment_request.set_header("Accept-Language", "en_US");
            payment_request.set_header("Content-Type", "application/json");
            payment_request.set_header("Authorization", QString("%1 %2").arg(token_type.c_str()).arg(access_token.c_str()).toUtf8().data());
            payment_request.set_header("PayPal-Request-Id", invoice_ipath.get_key().toUtf8().data());
            payment_request.set_data(body.toUtf8().data());
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
            as2js::JSON::JSONValue::object_t object(value->get_object());

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
            // get the "id" (also called "paymentId" in the future GET)
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
            epayment_paypal_table->row(main_uri.full_domain())->cell("id/" + id)->setValue(invoice_ipath.get_key());

            // we need a way to verify that the user coming back is indeed the
            // user who started the process so the thank you page can show the
            // cart or at least something in link with the cart; this is done
            // using the user's cookie (which thus needs to last long enough
            // for the "round trip")
            //
            // TODO: for this reason we may want to have a signal that allows
            //       plugins to define the minimum amount of time the user
            //       cookie must survive...
            users_plugin->attach_to_session(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_ID), id);

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

                        // retrieve the token, somehow it is not present anywhere
                        // else in the answer... (i.e. the "paymentId" is properly
                        // defined, just not this token!)
                        snap_uri redirect_uri(redirect_url);
                        if(!redirect_uri.has_query_option("token"))
                        {
                            SNAP_LOG_ERROR("paypal link \"approval_url\" has no \"token\" query string parameter");
                            throw epayment_paypal_exception_io_error("paypal link \"approval_url\" has no \"token\" query string parameter");
                        }
                        // The Cancel URL only receives the token,
                        // not the payment identifier!
                        QString const token(redirect_uri.query_option("token"));
                        epayment_paypal_table->row(main_uri.full_domain())->cell("token/" + token)->setValue(invoice_ipath.get_key());
                        secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_TOKEN))->setValue(token);
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

        if(redirect_url.isEmpty())
        {
            throw epayment_paypal_exception_io_error("paypal redirect URL (\"approval_url\") was not found");
        }
        if(!found_execute)
        {
            throw epayment_paypal_exception_io_error("paypal execute URL (\"execute\") was not found");
        }

        // now we are going on PayPal so the payment is pending...
        epayment_plugin->set_invoice_status(invoice_ipath, epayment::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING);
    }
    else if(click == "cancel")
    {
        // "cancel" -- the user just clicked the cancel button in the .../ready page
        //             we cancel the invoice and forget about that payment
        QString const token(f_snap->postenv(get_name(SNAP_NAME_EPAYMENT_PAYPAL_TOKEN_POST_FIELD)));
        cancel_invoice(token);
    }
    else if(click == "process")
    {
        // "process" -- the user just clicked the cancel button in the .../ready page
        //              we "execute" the payment (i.e. capture the money)

        QtCassandra::QCassandraTable::pointer_t epayment_paypal_table(get_epayment_paypal_table());

        // the invoice is linked by the "paymentId" sent in the token field
        // TODO: should we make use of both: paymentId and token here too?
        QString const id(f_snap->postenv(get_name(SNAP_NAME_EPAYMENT_PAYPAL_TOKEN_POST_FIELD)));
        snap_uri const main_uri(f_snap->get_uri());
        QString const invoice(epayment_paypal_table->row(main_uri.full_domain())->cell("id/" + id)->value().stringValue());
        content::path_info_t invoice_ipath;
        invoice_ipath.set_path(invoice);

        // the invoice has to still be pending, otherwise it possibly
        // was already marked as canceled or failed
        epayment::epayment *epayment_plugin(epayment::epayment::instance());
        epayment::name_t status(epayment_plugin->get_invoice_status(invoice_ipath));
        if(status != epayment::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING)
        {
            // TODO: support a default page in this case if the user is
            //       the correct user (this is only for people who hit
            //       reload, so no big deal right now)
            //messages::messages::instance()->set_error(
            //    "PayPal Processed",
            //    "PayPal invoice was already processed. Please go to your account to view your existing invoices.", 
            //    QString("Found the invoice, but somehow it is not marked \"pending\" (it is \"%1\" instead).").arg(epayment::get_name(status)),
            //    false
            //);
            //return;
            SNAP_LOG_ERROR("PayPal invoice was already processed. Please go to your account to view your existing invoices.");
            throw epayment_paypal_exception_io_error("PayPal invoice was already processed. Please go to your account to view your existing invoices.");
        }

        // the URL to send the execute request to PayPal is saved in the
        // invoice secret area
        content::content *content_plugin(content::content::instance());
        //QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
        QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
        QtCassandra::QCassandraRow::pointer_t secret_row(secret_table->row(invoice_ipath.get_key()));

        QString const execute_url(secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTE_PAYMENT))->value().stringValue());

        http_client_server::http_client http;
        http.set_keep_alive(true);

        std::string token_type;
        std::string access_token;
        if(!get_oauth2_token(http, token_type, access_token))
        {
            return;
        }

        //
        // Ready to send the Execute message to PayPal, the payer identifier
        // is the identifier we received in the last GET. The HTTP header is
        // about the same as when sending a create payment order:
        //
        //   {
        //     "payer_id": "123"
        //   }
        //
        // Execute replies look like this:
        //
        //   {
        //     "id": "PAY-123",
        //     "create_time": "2014-12-31T23:18:55Z",
        //     "update_time": "2014-12-31T23:19:39Z",
        //     "state": "approved",
        //     "intent": "sale",
        //     "payer":
        //     {
        //       "payment_method": "paypal",
        //       "payer_info":
        //       {
        //         "email": "paypal-buyer@paypal.com",
        //         "first_name": "Test",
        //         "last_name": "Buyer",
        //         "payer_id": "123",
        //         "shipping_address":
        //         {
        //           "line1": "1 Main St",
        //           "city": "San Jose",
        //           "state": "CA",
        //           "postal_code": "95131",
        //           "country_code": "US",
        //           "recipient_name": "Test Buyer"
        //         }
        //       }
        //     },
        //     "transactions":
        //     [
        //       {
        //         "amount":
        //         {
        //           "total": "111.34",
        //           "currency": "USD",
        //           "details":
        //           {
        //             "subtotal": "111.34"
        //           }
        //         },
        //         "description": "Hello from Snap! Websites",
        //         "related_resources":
        //         [
        //           {
        //             "sale":
        //             {
        //               "id": "123",
        //               "create_time": "2014-12-31T23:18:55Z",
        //               "update_time": "2014-12-31T23:19:39Z",
        //               "amount":
        //               {
        //                 "total": "111.34",
        //                 "currency": "USD"
        //               },
        //               "payment_mode": "INSTANT_TRANSFER",
        //               "state": "completed",
        //               "protection_eligibility": "ELIGIBLE",
        //               "protection_eligibility_type": "ITEM_NOT_RECEIVED_ELIGIBLE,UNAUTHORIZED_PAYMENT_ELIGIBLE",
        //               "parent_payment": "PAY-123",
        //               "links":
        //               [
        //                 {
        //                   "href": "https://api.sandbox.paypal.com/v1/payments/sale/123",
        //                   "rel": "self",
        //                   "method": "GET"
        //                 },
        //                 {
        //                   "href": "https://api.sandbox.paypal.com/v1/payments/sale/123/refund",
        //                   "rel": "refund",
        //                   "method": "POST"
        //                 },
        //                 {
        //                   "href": "https://api.sandbox.paypal.com/v1/payments/payment/PAY-123",
        //                   "rel": "parent_payment",
        //                   "method": "GET"
        //                 }
        //               ]
        //             }
        //           }
        //         ]
        //       }
        //     ],
        //     "links":
        //     [
        //       {
        //         "href": "https://api.sandbox.paypal.com/v1/payments/payment/PAY-123",
        //         "rel": "self",
        //         "method": "GET"
        //       }
        //     ]
        //   }
        //
        QString const payer_id(secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYER_ID))->value().stringValue());
        QString const body(QString(
                    "{"
                        "\"payer_id\":\"%1\""
                    "}"
                ).arg(payer_id)
            );

        http_client_server::http_request execute_request;
        // execute_url is a full URL, for example:
        //   https://api.sandbox.paypal.com/v1/payments/payment/PAY-123/execute
        // and the set_uri() function takes care of everything for us in that case
        execute_request.set_uri(execute_url.toUtf8().data());
        //execute_request.set_path("...");
        //execute_request.set_port(443); // https
        execute_request.set_header("Accept", "application/json");
        execute_request.set_header("Accept-Language", "en_US");
        execute_request.set_header("Content-Type", "application/json");
        execute_request.set_header("Authorization", QString("%1 %2").arg(token_type.c_str()).arg(access_token.c_str()).toUtf8().data());
        execute_request.set_header("PayPal-Request-Id", invoice_ipath.get_key().toUtf8().data());
        execute_request.set_data(body.toUtf8().data());
        http_client_server::http_response::pointer_t response(http.send_request(execute_request));

        secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTED_PAYMENT_HEADER))->setValue(QString::fromUtf8(response->get_original_header().c_str()));
        secret_row->cell(get_name(SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTED_PAYMENT))->setValue(QString::fromUtf8(response->get_response().c_str()));

        // looks pretty good, check the actual answer...
        as2js::JSON::pointer_t json(new as2js::JSON);
        as2js::StringInput::pointer_t in(new as2js::StringInput(response->get_response()));
        as2js::JSON::JSONValue::pointer_t value(json->parse(in));
        if(!value)
        {
            SNAP_LOG_ERROR("JSON parser failed parsing 'execute' response");
            throw epayment_paypal_exception_io_error("JSON parser failed parsing 'execute' response");
        }
        as2js::JSON::JSONValue::object_t object(value->get_object());

        // ID
        // verify that the payment identifier corresponds to what we expect
        if(object.find("id") == object.end())
        {
            SNAP_LOG_ERROR("'id' missing in 'execute' response");
            throw epayment_paypal_exception_io_error("'id' missing in 'execute' response");
        }
        QString const execute_id(QString::fromUtf8(object["id"]->get_string().to_utf8().c_str()));
        if(execute_id != id)
        {
            SNAP_LOG_ERROR("'id' in 'execute' response is not the same as the invoice 'id'");
            throw epayment_paypal_exception_io_error("'id' in 'execute' response is not the same as the invoice 'id'");
        }

        // INTENT
        // verify that: "intent" == "sale"
        if(object.find("intent") == object.end())
        {
            SNAP_LOG_ERROR("'intent' missing in 'execute' response");
            throw epayment_paypal_exception_io_error("'intent' missing in 'execute' response");
        }
        if(object["intent"]->get_string() != "sale")
        {
            SNAP_LOG_ERROR("'intent' in 'execute' response is not 'sale'");
            throw epayment_paypal_exception_io_error("'intent' in 'execute' response is not 'sale'");
        }

        // STATE
        // now check the state of the sale
        if(object.find("state") == object.end())
        {
            SNAP_LOG_ERROR("'state' missing in 'execute' response");
            throw epayment_paypal_exception_io_error("'state' missing in 'execute' response");
        }
        if(object["state"]->get_string() == "approved")
        {
            // the execute succeeded, mark the invoice as paid
            epayment_plugin->set_invoice_status(invoice_ipath, epayment::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID);
        }
        else
        {
            // the execute did not approve the sale
            // mark the invoice as failed...
            epayment_plugin->set_invoice_status(invoice_ipath, epayment::SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED);
        }
    }
    else
    {
        success = false;
        messages::messages::instance()->set_error(
            "PayPal Missing Unknown Command",
            QString("Your last request sent command \"%1\" which the server does not understand.").arg(click), 
            "Hacker send a weird 'click' value or we did not update the server according to the JavaScript code.",
            false
        );
    }

    // create the AJAX response
    server_access::server_access *server_access_plugin(server_access::server_access::instance());
    server_access_plugin->create_ajax_result(ipath, success);
    server_access_plugin->ajax_append_data(get_name(SNAP_NAME_EPAYMENT_PAYPAL_TOKEN_POST_FIELD), click.toUtf8());
    server_access_plugin->ajax_redirect(redirect_url);
    server_access_plugin->ajax_output();
}


void epayment_paypal::on_replace_token(content::path_info_t& ipath, QString const& plugin_owner, QDomDocument& xml, filter::filter::token_info_t& token)
{
    static_cast<void>(ipath);
    static_cast<void>(plugin_owner);
    static_cast<void>(xml);

    if(!token.is_namespace("epayment_paypal::"))
    {
        return;
    }

    if(token.is_token("epayment_paypal::process_buttons"))
    {
        // buttons used to run the final paypal process (i.e. execute
        // a payment); we also offer a Cancel button, just in case
        snap_uri const main_uri(f_snap->get_uri());
        if(main_uri.has_query_option("paymentId"))
        {
            QtCassandra::QCassandraTable::pointer_t epayment_paypal_table(get_epayment_paypal_table());
            QString const id(main_uri.query_option("paymentId"));
std::cerr << "*** paymentId is [" << id << "] [" << main_uri.full_domain() << "]\n";
            QString const invoice(epayment_paypal_table->row(main_uri.full_domain())->cell("id/" + id)->value().stringValue());
            content::path_info_t invoice_ipath;
            invoice_ipath.set_path(invoice);

            epayment::epayment *epayment_plugin(epayment::epayment::instance());

            // TODO: add a test to see whether the invoice has already been
            //       accepted, if so running the remainder of the code here
            //       may not be safe (i.e. this would happen if the user hits
            //       Reload on his browser.)
            epayment::name_t status(epayment_plugin->get_invoice_status(invoice_ipath));
            if(status == epayment::SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING)
            {
                token.f_replacement = "<div class=\"epayment_paypal-process-buttons\">"
                        "<a class=\"epayment_paypal-cancel\" href=\"#cancel\">Cancel</a>"
                        "<a class=\"epayment_paypal-process\" href=\"#process\">Process</a>"
                    "</div>";
            }
        }
    }
}


// PayPal REST documentation at time of writing
//   https://developer.paypal.com/webapps/developer/docs/api/

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
