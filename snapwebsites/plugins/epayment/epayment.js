/** @preserve
 * Name: epayement
 * Version: 0.0.1
 * Browsers: all
 * Depends: editor (>= 0.0.3.262)
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
// ==/ClosureCompiler==
//

/*
 * JSLint options are defined online at:
 *    http://www.jshint.com/docs/options/
 */
/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false, FileReader: true, Blob: true */



/** \file
 * \brief The Snap! e-Payment organization.
 *
 * This file defines a the e-Payment facilities offered in JavaScript
 * classes.
 *
 * This includes ways to add electronic and not so electronic payment
 * facilities to a website.
 *
 * The current e-Payment environment looks like this:
 *
 * \code
 *  +-----------------------+
 *  |                       |
 *  | ServerAccessCallbacks |
 *  | (cannot instantiate)  |       +-----------------------------+
 *  +-----------------------+       |                             |
 *       ^                          | ePaymentFacilityBase        |
 *       |         +--------------->| (cannot instantiate)        |
 *       |         |                +-----------------------------+
 *       |         |                      ^
 *       |         | Reference            | Inherit
 *       |         |                      |
 *  +----+---------+----+           +-----+--------------------+
 *  |                   |<----------+                          |
 *  | ePayment          | Register  | ePaymentFacility...      |
 *  | (final)           | (1,n)     |                          |
 *  +-----+-------------+           +--------------------------+
 *    ^   |
 *    |   | Create (1,1)
 *    |   |      +-------------------+
 *    |   +----->|                   +--... (widgets, etc.)
 *    |          | EditorForm        |
 *    |          |                   |
 *    |          +-------------------+
 *    |
 *    | Create (1,1)
 *  +-+----------------------+
 *  |                        |
 *  |   jQuery()             |
 *  |   Initialization       |
 *  |                        |
 *  +------------------------+
 * \endcode
 *
 * The e-Payment facility will generally be available when the e-Commerce
 * cart is around. However, ultimately we will want to offer full
 * AJAX support so we can load the facility when needed to avoid slow
 * down on all pages of the website.
 */



/** \brief Snap ePaymentFacilityBase constructor.
 *
 * The e-Payment system accepts multiple payment facilities.
 *
 * The base class derives from the ServerAccessCallbacks class so that
 * way the facility can may use of AJAX if necessary (which is very
 * likely for quite a few facilities...)
 *
 * Various facilities may make use of common code offered by the e-Payment
 * implementation and other plugins. For example, any facility asking for
 * a Credit Card number can make use of the e-Payment forms that include
 * the credit card info, the user address, and the registration for an
 * account for the user.
 *
 * \code
 *  class ePaymentFacilityBase extends ServerAccessCallbacks
 *  {
 *  public:
 *      ePaymentFacilityBase();
 *
 *      abstract function getFacilityName() : String;
 *      abstract function getDisplayName() : String;
 *      abstract function getIcon() : String;
 *
 *      virtual function serverAccessSuccess(result: ServerAccessCallbacks.ResultData);
 *      virtual function serverAccessError(result: ServerAccessCallbacks.ResultData);
 *      virtual function serverAccessComplete(result: ServerAccessCallbacks.ResultData);
 *  };
 * \endcode
 *
 * @return {snapwebsites.ePaymentFacilityBase}
 * @constructor
 * @struct
 */
snapwebsites.ePaymentFacilityBase = function()
{
    return this;
};


/** \brief ePaymentFacilityBase inherits the ServerAccessCallbacks.
 *
 * This class is the only facility visible to the e-Payment plugin
 * implementation. The actual facilities have to be added along to
 * enable them on your website. In other words, by adding an e-payment
 * facility plugin, you offer that facility to your customers. This
 * means you should only add the facilities you want to use and not
 * just all facilities.
 *
 * Facilities can be marked as in conflict, although this can be
 * done at a user level only since in reality facilities won't be
 * in conflict. Only offering more than one direct credit card
 * gate way will be (very) confusing to end users. (i.e. if you
 * have PaypalPro + Chase + CitiBank + Bank of America and other
 * such credit card gate way facilities, how is the customer to
 * choose which gate way to use for his payment?)
 */
snapwebsites.inherits(snapwebsites.ePaymentFacilityBase, snapwebsites.ServerAccessCallbacks);


/** \brief Get the technical name of this facility.
 *
 * This function returns a string with the technical name of this facility.
 * The technical name does not change and is generally all lowercase with
 * underscores for spaces. This allows that name to be used in places such
 * as the class of the tags used to create HTML output for this facility.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override of abstract function.)
 *
 * @return {string}  The technical name of the product.
 */
snapwebsites.ePaymentFacilityBase.prototype.getFacilityName = function() // abstract
{
    throw new Error("snapwebsites.ePaymentFacilityBase.getFacilityName() is not implemented");
};


/** \brief Get the display name of this facility.
 *
 * This function returns a string that can be displayed to represent
 * this facility. This name may be translated in various languages
 * or at least change slightly depending on the language or country
 * selected by the current user.
 *
 * This is generally used as a fallback if the icon cannot be used or
 * no icon was made available for this facility.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override of abstract function.)
 *
 * @return {string}  The display name of the product.
 */
snapwebsites.ePaymentFacilityBase.prototype.getDisplayName = function() // abstract
{
    throw new Error("snapwebsites.ePaymentFacilityBase.getDisplayName() is not implemented");
};


/** \brief Get the technical name of this facility.
 *
 * This function returns a string with the URL to an icon representing
 * this facility. The string may be empty or null if the facility does
 * not (yet) offer an icon. In this case, the system is likely to
 * fallback to the display name.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override of abstract function.)
 *
 * @return {string}  The technical name of the product.
 */
snapwebsites.ePaymentFacilityBase.prototype.getIcon = function() // abstract
{
    return null;
};


/*jslint unparam: true */
/** \brief Function called on success.
 *
 * This function is called if the remote access was successful. The
 * \p result object includes a reference to the XML document found in the
 * data sent back from the server.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.ePaymentFacilityBase.prototype.serverAccessSuccess = function(result) // virtual
{
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Function called on error.
 *
 * This function is called if the remote access generated an error.
 * In this case errors include I/O errors, server errors, and application
 * errors. All call this function so you do not have to repeat the same
 * code for each type of error.
 *
 * \li I/O errors -- somehow the AJAX command did not work, maybe the
 *                   domain name is wrong or the URI has a syntax error.
 * \li server errors -- the server received the POST but somehow refused
 *                      it (maybe the request generated a crash.)
 * \li application errors -- the server received the POST and returned an
 *                           HTTP 200 result code, but the result includes
 *                           a set of errors (not enough permissions,
 *                           invalid data, etc.)
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.ePaymentFacilityBase.prototype.serverAccessError = function(result) // virtual
{
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Function called on completion.
 *
 * This function is called once the whole process is over. It is most
 * often used to do some cleanup.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.ePaymentFacilityBase.prototype.serverAccessComplete = function(result) // virtual
{
};
/*jslint unparam: false */



/** \brief Snap ePayment constructor.
 *
 * The ePayment includes everything required to handle electronic and
 * not so electronic payments:
 *
 * - Allow listing all the available payments
 * - Allow assignment of a priority to each payment facility
 *   (facilities with the highest priority appear first, possibly hiding
 *   the others and we add a 'More Choices ->' button)
 * - Allow AJAX to register a user account
 * - Allow AJAX to process the payment
 *
 * Note that many of the AJAX features have to directly be handled by
 * the facility since e-Payment does not otherwise know how to handle
 * those requests. e-Payment handles the AJAX user registration though.
 *
 * Note that this class is marked as final because it is used as a
 * singleton. To extend e-Payments you have to create payment facilities.
 *
 * \code
 *  final class ePayment extends ServerAccessCallbacks
 *  {
 *  public:
 *      ePayment();
 *
 *      function registerPaymentFacility(payment_facility: ePaymentFacilityBase) : Void;
 *      function hasPaymentFacility(feature_name: string) : boolean;
 *      function getPaymentFacility(feature_name: string) : ePaymentFacilityBase;
 *
 *      virtual function serverAccessSuccess(result: ServerAccessCallbacks.ResultData);
 *      virtual function serverAccessError(result: ServerAccessCallbacks.ResultData);
 *      virtual function serverAccessComplete(result: ServerAccessCallbacks.ResultData);
 *
 *  private:
 *      var paymentFacilities_: ePaymentFacilitiesBase;
 *  };
 * \endcode
 *
 * The e-Payment plugin supports several steps to handle a payment. All the
 * steps are always being processed, although certain payment facilities
 * may ignore (skip) a step.
 *
 * Each facility can tell what is required, what is optional, what is
 * not acceptable (i.e. Google Checkout used to prevent systems from
 * asking for any user information, including their address and real
 * name!)
 *
 * \li The e-Payment plugin expects the user to start a Payment Request.
 * This allows the e-Payment system and the user of the the e-Payment to
 * know how to proceed.
 *
 * \li Check list of facilities; once the user specified what needs to be
 * purchased, a list of facilities may be offered to the user. In most cases,
 * all facilities will be available, although it can happen that the user
 * is registered and, for example, Checks are allowed for him. Other reasons
 * for a facility to be available or not is they user country, or the items
 * purchased.
 *
 * \li Log In or Register the user. This is an option offered by the
 * e-Payment system because many situations require a user to have an
 * account. Purchasing software means getting a download and we want to
 * limit downloads to registered users. Purchasing a page to put up an
 * ad will also require an account.
 *
 * \li From the list of facilities allowed, you can offer the user to
 * select which payment facility to use to pay for his purchase.
 * Future versions will allow the user to choose more than once facility
 * to make their payment (i.e. pay X with Paypal and Y with a credit card).
 * For companies that have bank accounts in various countries, we could
 * also offer payment to be made in various currency (i.e. user can choose
 * between paying in USD or EUR, for example.) If only one facility is
 * available to the user, we expect no selection here since there is no
 * choice anyway (i.e. only accept Paypal payments...)
 *
 * \li User information: real name, billing/shipping address (street, city,
 * postal code, country), phone number, credit card (number, expiration
 * date, verification code [CVV]).
 *
 * \li Leave room for a comment, just in case. It can be useful in many
 * situations (i.e. when ordering food, maybe you want to ask for the onions
 * to be removed!) We Want to offer a button "Add Comment" as well as a
 * comment box already open.
 *
 * Once a payment is ready to be processed, the user can call the process()
 * function. This has multiple phases which may or may not be required by
 * the various payment facilities. This processing is possible only once
 * all the necessary data is available (i.e. for a credit card payment, you
 * need to have the user name, a credit card number, expiration date, CVV,
 * and probably a zip code or address to further verify the card.) This
 * processing includes the following states:
 *
 * \li processing -- started payment processing;
 * \li canceled -- the payment was canceled before it was completed
 * \li pending -- waiting for an asynchroneous reply from a payment facility
 * (i.e. Paypal does not send the reply immediately, although generally
 * it is very fast, it is definitively 100% asynchroneous); this step
 * only applies to facilities that require our system to wait for
 * confirmation;
 * \li failed -- the payment failed (i.e. credit card refused, check
 * bounced, etc.);
 * \li paid -- the payment was confirmed, now we can ship (if shipping
 * or equivalent there is);
 * \li completed -- the shipping was done, or the payment was recieved
 * and there was no shipping.
 *
 * Note: We mention shipping in the table before because in most cases
 * a purchase goes from Paid to Completed when a required operator action
 * was performed to complete the sale. This could be something else than
 * shipping such as making a phone call or manually adding the name of the
 * user to a list (i.e. maybe you will have a draw and need each user's
 * name printed on a piece of paper...) or rendering a service.
 *
 * When a feature needs more information about its own status, it has to
 * use its own variable and not count on the e-Payment plugin to handle
 * its special status. In other words, as far as the e-Payment plugin is
 * concerned, while processing a payment, it is in mode "pending", nothing
 * more. It would otherwise make things way more complicated than they
 * need to be (i.e. dynamic statuses?)
 *
 * Products that do not require shipping anything, go to the completed
 * status directly.
 *
 * A payment is attached to the same session as the cart session. That
 * session can then be used to generate an invoice (assuming your plugin
 * does not itself generate an invoice and uses the payment facility
 * to get it paid for.)
 *
 * The e-Payment is not in charge of the invoice capability. The e-Commerce
 * system (or your own implementation) is in charge of that capability. The
 * e-Payment plugin will request for the invoice to be created only once it
 * starts the payment processing. This way we avoid creating invoices for
 * users who come visit (Especially because you can add items to a cart using
 * a simple link... and thus you could end up creating hundred of useless
 * invoices!) The e-Commerce cart only creates a Sales Order which is
 * converted to an invoice once the payment processing starts.
 *
 * \note
 * The e-Payment has one limit which should pretty much never be a problem:
 * it does not allow for the processing of more than one payment at a time.
 *
 * \todo
 * Allow for the use of more than one payment facility to pay for
 * one fee.
 *
 * \todo
 * Allow for selecting an address from the user account. This is assuming
 * we allow for more than one address to be registered with one user.
 *
 * \todo
 * We save all of that information in the invoice we generate along
 * the payment. We can also save part of that information in the user's
 * account. It is important to save it in the invoice because the
 * user can change his information and the invoice needs to not
 * change over time.
 *
 * @return {snapwebsites.ePayment}  The newly created object.
 *
 * @constructor
 * @extends {snapwebsites.ServerAccessCallbacks}
 * @struct
 */
snapwebsites.ePayment = function()
{
    var that = this;

//#ifdef DEBUG
    if(jQuery("body").hasClass("snap-epayment-initialized"))
    {
        throw new Error("Only one e-Payment singleton can be created.");
    }
    jQuery("body").addClass("snap-epayment-initialized");
//#endif
    snapwebsites.ePayment.superClass_.constructor.call(this);

    this.paymentFacilities_ = {};

    return this;
};


/** \brief Mark the eCommerceCart as inheriting from the ServerAccessCallbacks.
 *
 * This class inherits from the ServerAccessCallbacks, which it uses
 * to send the server changes made by the client to the cart.
 * In the cart, that feature is pretty much always asynchroneous.
 */
snapwebsites.inherits(snapwebsites.eCommerceCart, snapwebsites.ServerAccessCallbacks);


/** \brief The list of payment facilities understood by the e-Payment plugin.
 *
 * This map is used to save the payment facilities when you call the
 * registerPaymentFacility() function.
 *
 * \note
 * The registered payment facilities cannot be unregistered.
 *
 * @type {Object}
 * @private
 */
snapwebsites.ePayment.prototype.paymentFacilities_; // = {}; -- initialized in constructor to avoid problems


/** \brief Register a payment facility.
 *
 * This function is used to register a payment facility in the ePayment
 * object.
 *
 * This allows for any number of payment facilities and thus any number of
 * satisfied customers as they can use any mean to pay for their purchases.
 *
 * @param {snapwebsites.ePaymentFacilityBase} payment_facility  The payment facility to register.
 *
 * @final
 */
snapwebsites.ePayment.prototype.registerPaymentFacility = function(payment_facility)
{
    var name = payment_facility.getFacilityName();
    this.paymentFacility_[name] = payment_facility;
};


/** \brief Check whether a facility is available.
 *
 * This function checks the list of payment facilities to see whether
 * \p facility_name exists.
 *
 * @param {!string} facility_name  The name of the facility to check.
 *
 * @return {!boolean}  true if the facility is defined.
 *
 * @final
 */
snapwebsites.ePayment.prototype.hasPaymentFacility = function(facility_name)
{
    return this.paymentFacility_[facility_name] instanceof snapwebsites.ePaymentFacilityBase;
};


/** \brief This function is used to get a payment facility.
 *
 * Payment facilities get registered with the registerPaymentFacility() function.
 * Later you may retrieve them using this function.
 *
 * @throws {Error} If the named \p facility_name was not yet registered, then this
 *                 function throws.
 *
 * @param {string} feature_name  The name of the product feature to retrieve.
 *
 * @return {snapwebsites.eCommerceProductFeatureBase}  The product feature object.
 *
 * @final
 */
snapwebsites.ePayment.prototype.getPaymentFacility = function(facility_name)
{
    if(this.paymentFacility_[facility_name] instanceof snapwebsites.ePaymentFacilityBase)
    {
        return this.paymentFacility_[facility_name];
    }
    throw new Error("getPaymentFacility(\"" + facility_name + "\") called when that facility is not yet defined.");
};


/*jslint unparam: true */
/** \brief Function called on success.
 *
 * This function is called if the remote access was successful. The
 * \p result object includes a reference to the XML document found in the
 * data sent back from the server.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.ePayment.prototype.serverAccessSuccess = function(result) // virtual
{
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Function called on error.
 *
 * This function is called if the remote access generated an error.
 * In this case errors include I/O errors, server errors, and application
 * errors. All call this function so you do not have to repeat the same
 * code for each type of error.
 *
 * \li I/O errors -- somehow the AJAX command did not work, maybe the
 *                   domain name is wrong or the URI has a syntax error.
 * \li server errors -- the server received the POST but somehow refused
 *                      it (maybe the request generated a crash.)
 * \li application errors -- the server received the POST and returned an
 *                           HTTP 200 result code, but the result includes
 *                           a set of errors (not enough permissions,
 *                           invalid data, etc.)
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.ePayment.prototype.serverAccessError = function(result) // virtual
{
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Function called on completion.
 *
 * This function is called once the whole process is over. It is most
 * often used to do some cleanup.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.ePayment.prototype.serverAccessComplete = function(result) // virtual
{
};
/*jslint unparam: false */



// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.ePaymentInstance = new snapwebsites.ePayment();
        // to add facilities, do something like this:
        //snapwebsites.ePaymentInstance.registerProductFeature(new snapwebsites.ePaymentFacility...());
    });

// vim: ts=4 sw=4 et
