/** @preserve
 * Name: epayment-paypal
 * Version: 0.0.1.14
 * Browsers: all
 * Depends: epayment (>= 0.0.1)
 * Copyright: Copyright 2013-2014 (c) Made to Order Software Corporation  All rights reverved.
 * License: GPL 2.0
 */

//
// Inline "command line" parameters for the Google Closure Compiler
// https://developers.google.com/closure/compiler/docs/js-for-compiler
//
// See output of:
//    java -jar .../google-js-compiler/compiler.jar --help
//
// ==ClosureCompiler==
// @compilation_level ADVANCED_OPTIMIZATIONS
// @externs $CLOSURE_COMPILER/contrib/externs/jquery-1.9.js
// @externs plugins/output/externs/jquery-extensions.js
// @js plugins/output/output.js
// @js plugins/output/popup.js
// @js plugins/server_access/server-access.js
// @js plugins/listener/listener.js
// @js plugins/editor/editor.js
// @js plugins/epayment/epayment.js
// ==/ClosureCompiler==
//

/*
 * JSLint options are defined online at:
 *    http://www.jshint.com/docs/options/
 */
/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false, FileReader: true, Blob: true */



/** \file
 * \brief The PayPal e-Payment Facility
 *
 * This file defines a the e-Payment facility that gives you the ability
 * to get payment via PayPal.
 *
 * \code
 *  +-----------------------------+
 *  |                             |
 *  | ePaymentFacilityBase        |
 *  | (cannot instantiate)        |
 *  +-----------------------------+
 *        ^
 *        | Inherit
 *        |
 *  +-----+-----------------------+
 *  |                             |
 *  | ePaymentFacilityPayPal      |
 *  |                             |
 *  +-----------------------------+
 * \endcode
 *
 * \note
 * There is another way to use PayPal which is through PayPal Pro. That
 * version is a direct gateway (i.e. a gateway that allows you to get
 * customers credit card payments directly from your website, without
 * having to send them to PayPal.) This very facility is the one that
 * sends people to PayPal for payment.
 */



/** \brief Snap ePaymentFacilityPayPal constructor.
 *
 * The e-Payment facility to process a payment through PayPal.
 *
 * \code
 *  class ePaymentFacilityPayPal extends ePaymentFacilityBase
 *  {
 *  public:
 *      ePaymentFacilityPayPal();
 *
 *      virtual function getFacilityName() : String;
 *      virtual function getDisplayName() : String;
 *      virtual function getIcon() : String;
 *
 *      virtual function serverAccessSuccess(result: ServerAccessCallbacks.ResultData);
 *      //virtual function serverAccessError(result: ServerAccessCallbacks.ResultData);
 *      //virtual function serverAccessComplete(result: ServerAccessCallbacks.ResultData);
 *  };
 * \endcode
 *
 * @return {snapwebsites.ePaymentFacilityPayPal}
 *
 * @extends {snapwebsites.ePaymentFacilityBase}
 * @constructor
 * @struct
 */
snapwebsites.ePaymentFacilityPayPal = function()
{
    // if we are on page "/epayment/paypal/ready", we have buttons to
    // connect to and we want to do that here
    var that = this,
        process_buttons = jQuery(".epayment_paypal-process-buttons");

    process_buttons
        .children(".epayment_paypal-cancel")
        .click(function(e)
            {
                var token = snapwebsites.OutputInstance.qsParam("token");

                e.preventDefault();
                e.stopPropagation();

                that.sendClick("cancel", token);
            });

    process_buttons
        .children(".epayment_paypal-process")
        .click(function(e)
            {
                var paymentId = snapwebsites.OutputInstance.qsParam("paymentId");

                e.preventDefault();
                e.stopPropagation();

                that.sendClick("process", paymentId); // PayPal calls this "execute"
            });

    return this;
};


/** \brief ePaymentFacilityPayPal inherits the ePaymentFacilityBase.
 *
 * This class implements the PayPal payment facility.
 */
snapwebsites.inherits(snapwebsites.ePaymentFacilityPayPal, snapwebsites.ePaymentFacilityBase);


/** \brief The server access used by this facility.
 *
 * Used to send the server a request when that facility button is clicked.
 *
 * PayPal is triggered from the server side since (1) the server needs to
 * save the identifier returned by PayPal and (2) laster the server gets
 * another hit via the IPN and needs to have that identifier handy. Not
 * only that, the identifier is viewed as a secret so not having it travel
 * to the client is always a good idea.
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.ePaymentFacilityPayPal.prototype.serverAccess_ = null;


/** \brief Get the technical name of this facility.
 *
 * This function returns "paypal".
 *
 * @return {string}  The technical name of the product.
 */
snapwebsites.ePaymentFacilityPayPal.prototype.getFacilityName = function() // virtual
{
    return "paypal";
};


/** \brief Get the display name of this facility.
 *
 * This function returns "PayPal".
 *
 * @return {string}  The display name of the product.
 */
snapwebsites.ePaymentFacilityPayPal.prototype.getDisplayName = function() // virtual
{
    return "PayPal";
};


/** \brief Get the icon used to represent that facility.
 *
 * This function returns a URI to an image representing the facilty.
 * In most cases, that image is the logo of the payment facility.
 *
 * By default the function returns an empty string. If you do not
 * override this default, then the facility has no icon.
 *
 * @return {string}  The URI to an image representing the facility.
 */
snapwebsites.ePaymentFacilityPayPal.prototype.getIcon = function() // abstract
{
    return "/images/epayment/paypal-medium.png";
};


/** \brief Generate HTML for a button to show this payment facility.
 *
 * This function is used to generate HTML that is most satisfactory
 * for that payment facility. This button is used to display a list of
 * facilities one can choose from to make a payment.
 *
 * The Base implementation generates a default entry which is likely
 * enough for most facilities.
 *
 * @return {string}  Facility button to display in the client's window.
 */
snapwebsites.ePaymentFacilityPayPal.prototype.getButtonHTML = function()
{
    var name = this.getFacilityName(),
        icon = this.getIcon(),
        html = "<img class='facility-icon icon-" + name + "' src='" + icon + "'/>";

    return html;
};


/** \brief This facility button was clicked.
 *
 * This PayPal implementation sends the click to the server directly.
 * It expects the server to register the invoice and start the checkout
 * process with PayPal Express.
 */
snapwebsites.ePaymentFacilityPayPal.prototype.buttonClicked = function()
{
    this.sendClick("checkout", "");
};


/** \brief Send a click to the server.
 *
 * This function makes use of AJAX to send a click to the server.
 *
 * The \p type parameter is used to tell the server which button was
 * clicked.
 *
 * @param {string} type  The name of the button clicked.
 * @param {string} token  The token or paymenId or "" if not available yet.
 */
snapwebsites.ePaymentFacilityPayPal.prototype.sendClick = function(type, token)
{
    if(!this.serverAccess_)
    {
        this.serverAccess_ = new snapwebsites.ServerAccess(this);
    }

    this.serverAccess_.setURI(snapwebsites.castToString(jQuery("link[rel='canonical']").attr("href") + "?a=view", "casting href of the canonical link to a string in snapwebsites.EditorForm.saveData()"));
    this.serverAccess_.setData(
        {
            epayment__epayment_paypal: type,
            epayment__epayment_paypal_token: token
        });
    this.serverAccess_.send();

    // now we wait for an answer which should give us a URL to redirect
    // the user to a PayPal page

    // darken the screen to avoid having the user click something else
    // while processing...
    snapwebsites.PopupInstance.darkenPage(150, true);
};


/*jslint unparam: true */
/** \brief Function called on success.
 *
 * This function is called if the remote access was successful. The
 * \p result object includes a reference to the XML document found in the
 * data sent back from the server.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.ePaymentFacilityPayPal.prototype.serverAccessSuccess = function(result) // virtual
{
    var data_tags = result.jqxhr.responseXML.getElementsByTagName("data"),
        name,
        event;

    for(idx = 0; idx < data_tags.length; ++idx)
    {
        // make sure it is one of our parameters
        name = data_tags[idx].getAttribute("name");
        if(name === "epayment__epayment_paypal_token")
        {
            event = data_tags[idx].textContent;
            if(event == "cancel")
            {
                // this was a cancel, mark invoice as canceled
                // and hide the various buttons
                jQuery(".epayment_paypal-process-buttons").hide();

                jQuery(".ecommerce-invoice-status .invoice-value")
                    .html("canceled");
            }
            else if(event == "process")
            {
                // this was a cancel, mark invoice as canceled
                // and hide the various buttons
                jQuery(".epayment_paypal-process-buttons").hide();

                // We use "Paid" here although it could be "Completed"
                // if a plugin forces that status automatically.
                jQuery(".ecommerce-invoice-status .invoice-value")
                    .html("paid");
            }
        }
    }

    // finally call the super class version
    snapwebsites.ePaymentFacilityPayPal.superClass_.serverAccessSuccess.call(this, result);
};
/*jslint unparam: false */


// /*jslint unparam: true */
// /** \brief Function called on error.
//  *
//  * This function is called if the remote access generated an error.
//  * In this case errors include I/O errors, server errors, and application
//  * errors. All call this function so you do not have to repeat the same
//  * code for each type of error.
//  *
//  * \li I/O errors -- somehow the AJAX command did not work, maybe the
//  *                   domain name is wrong or the URI has a syntax error.
//  * \li server errors -- the server received the POST but somehow refused
//  *                      it (maybe the request generated a crash.)
//  * \li application errors -- the server received the POST and returned an
//  *                           HTTP 200 result code, but the result includes
//  *                           a set of errors (not enough permissions,
//  *                           invalid data, etc.)
//  *
//  * By default this function does nothing.
//  *
//  * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
//  *          resulting data with information about the error(s).
//  */
// snapwebsites.ePaymentFacilityPayPal.prototype.serverAccessError = function(result) // virtual
// {
//     snapwebsites.ePaymentFacilityPayPal.superClass_.serverAccessComplete.call(result);
// };
// /*jslint unparam: false */
//
//
// /*jslint unparam: true */
// /** \brief Function called on completion.
//  *
//  * This function is called once the whole process is over. It is most
//  * often used to do some cleanup.
//  *
//  * By default this function does nothing.
//  *
//  * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
//  *          resulting data with information about the error(s).
//  */
// snapwebsites.ePaymentFacilityPayPal.prototype.serverAccessComplete = function(result) // virtual
// {
// };
// /*jslint unparam: false */



// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.ePaymentInstance.registerPaymentFacility(new snapwebsites.ePaymentFacilityPayPal());
    });

// vim: ts=4 sw=4 et
