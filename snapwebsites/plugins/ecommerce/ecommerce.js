/** @preserve
 * Name: ecommerce
 * Version: 0.0.1
 * Browsers: all
 * Depends: editor (>= 0.0.3.262)
 * Copyright: Copyright 2013-2014 (c) Made to Order Software Corporation  All rights reverved.
 * License: GPL 2.0
 */

//
// Inline "command line" parameters for the Google Closure Compiler
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
 * \brief The Snap! Instant Cart organization.
 *
 * This file defines a set of e-Commerce features using advance JavaScript
 * classes.
 *
 * This includes the Cart (the main object). You cannot create more than
 * one Cart. The Cart receives signals to add products to itself. These
 * products generally are referencing the ProductTypeBasic type. Other
 * types can be added. For example we are planning in offering a
 * ProductTypeKit which allows the e-Commerce administrator to create
 * bundles of products to be sold as one unit. The details of these
 * bundles should be shown in the cart and the specialized product type
 * is used for that purpose. It will likely be derived from the basic
 * product type.
 *
 * The resulting environment looks like this:
 *
 * \code
 *  +-----------------------+       +--------------------------+
 *  |                       |       |                          |
 *  | ServerAccessCallbacks |       | eCommerceProductTypeBase |
 *  | (cannot instantiate)  |  +--->| (cannot instantiate)     |
 *  +-----------------------+  |    +--------------------------+
 *       ^                     |          ^
 *       | Inherit   Reference |          | Inherit
 *       |                     |          |
 *  +----+--------------+      |    +-----+-------------------+
 *  |                   +------+    |                         |
 *  | eCommerceCart     +---------->| eCommerceProductType... |<------------+
 *  |                   | Register  |                         |             |
 *  +-----+-----------+-+ (1,n)     +-------------------------+             |
 *    ^   |           |                               ^                     |
 *    |   |           | Create (1,n)                  | Reference (1,1)     |
 *    |   |           V                               |                     |
 *    |   |      +-------------------+                |                     |
 *    |   |      |                   +----------------+                     |
 *    |   |      | eCommerceProduct  |                                      |
 *    |   |      |                   |                                      |
 *    |   |      +-------------------+                                      |
 *    |   |                                                                 |
 *    |   | Create (1,1)                                                    |
 *    |   |      +-------------------+                                      |
 *    |   +----->|                   |--... (widgets, etc.)                 |
 *    |          | EditorForm        |                                      |
 *    |          |                   |                                      |
 *    |          +-------------------+                                      |
 *    |                                                                     |
 *    | Create (1,1)                                                        |
 *  +-+----------------------+  Create (1,n)                                |
 *  |                        +----------------------------------------------+
 *  |   jQuery()             |
 *  |   Initialization       |
 *  |                        |
 *  +------------------------+
 * \endcode
 *
 * Note that the cart is something that happens to always be around
 * by default since an e-Commerce site is willing to sell, sell, sell
 * and thus having the cart always at hand is the best way to make
 * sure that your users know how to add items in there and how to
 * checkout and send you tons of money.
 *
 * The cart uses the ServerAccess object capabilities to send cart
 * updates to the server. This allows us to save the cart in
 * Cassandra and avoid losing it. Not only that, this means the
 * user can close everything, come back a week later, and still
 * find all his information intact.
 *
 * The following shows the implemented product types:
 *
 * \code
 *
 * \endcode
 */


/** \brief Snap eCommerceProductTypeBase constructor.
 *
 * The cart works with products and each one of these products has a
 * type. In most cases products are given the default type which is
 * ProductTypeBasic.
 *
 * To make product objects fully dynamic, we define a base class here
 * and let other programmers add new product types in their own .js
 * files by extending this class (or another core type.)
 *
 * Classes must be registered with the EditorBase class function:
 *
 * \code
 *    snapwebsites.CartInstance.registerProductType(new your_product_type);
 * \endcode
 *
 * This base class already implements a few things that are common to
 * all products.
 *
 * \code
 *  class eCommerceProductTypeBase
 *  {
 *  public:
 *      eCommerceProductTypeBase();
 *
 *      abstract function getType() : string;
 *      abstract function initializeProduct(product: Object) : void;
 *  };
 * \endcode
 *
 * @constructor
 * @struct
 */
snapwebsites.eCommerceProductTypeBase = function()
{
    // TBD
    // Maybe at some point we'd want to create yet another layer
    // so we can have an auto-register, but I'm not totally sure
    // that would really work right in all cases...
    //snapwebsites.Cart.registerProductType(this);

    return this;
};


/** \brief eCommerceProductTypeBase is a base class.
 *
 * Note that if you inherit from a product type and implements these
 * functions, make sure to call the super version too, unless declared as
 * abstract, in case they do something that is required. At this point the
 * base class does nothing in those callbacks although we may add error
 * handling in the error callback.
 */
snapwebsites.base(snapwebsites.eCommerceProductTypeBase);


/** \brief Retrieve the name of this product type.
 *
 * This function returns the product type. It is used whenever you
 * register the type in the Cart object.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (required override.)
 *
 * @return {string}  The type of this cart product type as a string.
 */
snapwebsites.eCommerceProductTypeBase.prototype.getType = function() // virtual
{
    throw new Error("snapwebsites.eCommerceProductTypeBase.getType() was not overloaded.");
};


/*jslint unparam: true */
/** \brief Initialize a product of this type.
 *
 * The parameter is really a snapwebsites.Product object,
 * but at this point we did not yet define that object type.
 *
 * @throws {Error} The base type function throws an error as it should never
 *                 get called (requires override.)
 *
 * @param {!Object} editor_widget  The widget being initialized.
 */
snapwebsites.eCommerceProductTypeBase.prototype.initializeProduct = function(product) // virtual
{
    throw new Error("snapwebsites.eCommerceProductTypeBase.initializeProduct() does not do anything (yet)");
};
/*jslint unparam: false */



/** \brief Snap eCommerceCart constructor.
 *
 * The eCommerceCart includes everything required to handle the following:
 *
 * - Add a product to the cart
 * - Edit the cart / wishlist
 *   - Global functions
 *     - Open (show)
 *     - Close (hide)
 *     - Clear cart
 *     - Checkout (button to "move" to checkout)
 *   - Actual editing
 *     - Delete a product
 *     - Change product quantity (if allowed by product type)
 *     - Move product from cart to wishlist (and vice versa from wishlist)
 *     - Links to products for details
 * - Send changes to the server
 *   - Handle server response
 *
 * \code
 * class eCommerceCart extends ServerAccessCallbacks
 * {
 * public:
 *      eCommerceCart();
 *
 *      function checkModified();
 *      function registerProductType(widget_type: eCommerceProductTypeBase);
 *      function hasProductType(type_name: string) : boolean;
 *      function getProductType(type_name: string) : ProductType;
 *      function showGlimpse() : Void;
 *      function hideGlimpse() : Void;
 *      function showCart() : Void;
 *      function hideCart() : Void;
 *
 *      virtual function generate_cart_html() : string;
 *      virtual function generate_cart_header() : Void;
 *      virtual function generate_cart_footer() : Void;
 *      function generate_product_table() : Void;
 *      virtual function generate_product_table_header() : Void;
 *      virtual function generate_product_table_footer() : Void;
 *
 *      virtual function serverAccessSuccess(result);
 *      virtual function serverAccessError(result);
 *      virtual function serverAccessComplete(result);
 *
 * private:
 *      productTypes_: ProductType;
 * };
 * \endcode
 *
 * @return {snapwebsites.eCommerceCart}  The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.eCommerceCart = function()
{
//#ifdef DEBUG
    if(jQuery("body").hasClass("snap-editor-initialized"))
    {
        throw new Error("Only one editor singleton can be created.");
    }
    jQuery("body").addClass("snap-editor-initialized");
//#endif

    this.productTypes_ = [];
    this.cartHtml_ = [];
    this.products_ = [];

    return this;
};


/** \brief Mark the eCommerceCart as inheriting from the ServerAccessCallbacks.
 *
 * This class inherits from the ServerAccessCallbacks, which it uses
 * to send the server changes made by the client to the cart.
 */
snapwebsites.inherits(snapwebsites.eCommerceCart, snapwebsites.ServerAccessCallbacks);


/** \brief The popup information for the instant cart functionality.
 *
 * This static object represents the popup object used by the Cart to
 * present to the user the currently selected products ready to be
 * purchased by going to the checkout area.
 *
 * @type {Object}
 * @private
 */
snapwebsites.eCommerceCart.createPopup_ =
{

// WARNING: we have exactly ONE instance of this variable
//          (i.e. if we were to create more than one Main object
//          we'd still have ONE instance.)

    id: "ecommerce-cart",
    title: "Cart",
    darken: 150,
    width: 750
    //beforeHide: -- defined as required
};


/** \brief The list of product types understood by the e-Commerce cart.
 *
 * This array is used to save the product types when you call the
 * registerProductType() function.
 *
 * @type {Array.<snapwebsites.eCommerceProductTypeBase>}
 * @private
 */
snapwebsites.eCommerceCart.prototype.productTypes_; // = []; -- initialized in constructor to avoid problems


/** \brief A protected variable member holding the cart HTML code.
 *
 * This DOM document holds the cart HTML code. It is regenerated
 * when we first open the cart popup. Once it exists, the cart
 * keeps the code up to date as we go (i.e. you could keep the
 * cart open and add items to it and see that it gets updated
 * on the fly.)
 *
 * This jQuery object is the one returned by the open() function
 * of the Popup class. In other words, it is not directly the
 * part where we can add the cart. You must make sure to add
 * code as children of the ".popup-body" div. Not doing so may
 * interfere with the popup code and other parts of the cart.
 *
 * @type {jQuery}
 * @protected
 */
snapwebsites.eCommerceCart.prototype.cartHtml_; // = []; -- initialized in constructor to avoid problems


/** \brief The list of products that have been added to this cart.
 *
 * This array is the list of products that have been added to this cart.
 *
 * @type {snapwebsites.eCommerceProduct}
 * @private
 */
snapwebsites.eCommerceCart.prototype.products_; // = []; -- initialized in constructor to avoid problems


/** \brief Check whether a field in the cart editor form was modified.
 *
 * When a quantity or some other parameter was modified in the cart,
 * this function is called to check whether it looks different. If so
 * then the change is sent to the server for archiving.
 */
snapwebsites.eCommerceCart.prototype.checkModified = function()
{
};


/** \brief Register a product type.
 *
 * This function is used to register an e-Commerce product type in the
 * cart.
 *
 * This allows for any number of extensions and thus any number of
 * cool advanced features that do not all need to be defined in the
 * core of the cart implementation.
 *
 * @param {snapwebsites.ProductType} product_type  The product type to register.
 */
snapwebsites.eCommerceCart.prototype.registerProductType = function(product_type)
{
    var name = product_type.getType();
    this.productTypes_[name] = product_type;
};


/** \brief Check whether a type exists.
 *
 * This function checks the list of product types to see whether
 * \p type_name exists.
 *
 * @param {!string} type_name  The name of the type to check.
 *
 * @return {!boolean}  true if the type is defined.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.hasProductType = function(type_name)
{
    return this.productTypes_[type_name] instanceof snapwebsites.eCommerceProductTypeBase;
};


/** \brief This function is used to get a product type.
 *
 * Product types get registered with the registerProductType() function.
 * Later you may retrieve them using this function.
 *
 * @throws {Error} If the named \p type was not yet registered, then this
 *                 function throws.
 *
 * @param {string} type_name  The name of the product type to retrieve.
 *
 * @return {snapwebsites.eCommerceProductType}  The product type object.
 *
 * @final
 */
snapwebsites.eCommerceCart.prototype.getProductType = function(type_name)
{
    if(!(this.productTypes_[type_name] instanceof snapwebsites.eCommerceProductTypeBase))
    {
        throw new Error("getProductType() of type \"" + type_name + "\" is not yet defined, you cannot get it now.");
    }
    return this.productTypes_[type_name];
};


/** \brief Show a "glimpse" of the cart.
 *
 * This function shows the cart icon in your theme. The theme has
 * a certain amount of control on how the cart appears as a glimpse.
 *
 * If the cart is empty, the empty cart icon is shown.
 *
 * If the cart is not empty, the standard cart icon is shown.
 */
snapwebsites.eCommerceCart.prototype.showGlimpse = function()
{
};


/** \brief Hide the "glimpse" of the cart.
 *
 * This function hides the cart icon from the theme. This means the
 * cart is not directly accessible by the end user which is certainly
 * a good idea on certain pages (i.e. a form), not so good on others.
 *
 * For example, it is generally a good idea to hide the cart and
 * glimpse when asking the user for his credit card information
 * (card number, address, phone...)
 */
snapwebsites.eCommerceCart.prototype.hideGlimpse = function()
{
};


/** \brief Show the cart.
 *
 * This function shows the cart in a popup. This is 100% done on the
 * client side (no access to the server.) This is because the cart
 * is 100% built using code.
 *
 * The cart makes use of a fully dynamic editor form.
 *
 * In most cases, clicking on the Glimpse will trigger a call this
 * function. Other methods may be used to open the cart.
 */
snapwebsites.eCommerceCart.prototype.showCart = function()
{
    if(!this.cartHtml_)
    {
        this.cartHtml_ = snapwebsites.PopupInstance.open(snapwebsites.eCommerceCart.createPopup_);
        generate_cart_html(this.cartHtml_.find(".popup-body"));
    }
    // probably need an update here?
    // although even when hidden we probably want to keep this up to date

    snapwebsites.PopupInstance.show(snapwebsites.eCommerceCart.createPopup_);
};


/** \brief Hide the cart.
 *
 * This function hides the cart, really it just closes the popup
 * opened by the showCart() function. The close button of the popup
 * will do the same thing.
 */
snapwebsites.eCommerceCart.prototype.hideCart = function()
{
    snapwebsites.PopupInstance.hide(snapwebsites.eCommerceCart.createPopup_);
};



/** \brief Generate the cart HTML code.
 *
 * This function generates the HTML code using jQuery() to append the
 * code as required.
 *
 * The function first calls the generate_cart_header() function which
 * can be overridden since it is virtual.
 *
 * Next it calls the generate_product_table() which is expected to create
 * the table with the items currently in the cart. If no item are defined,
 * it still creates a table, only it will be marked as hidden and instead
 * we show another div tag with the message "your cart is empty".
 *
 * We do not allow the generate_product_table() function to be overridden.
 * Instead we expect the product types to generate the data accordingly
 * so the dynamism is not available in the table itself. However, as you
 * can see in the function definition, the table header and footer functions
 * can be overridden.
 *
 * Finally the function calls the generate_cart_footer() function which
 * can be overridden too since it is virtual.
 *
 * @param {!jQuery} e  The body element where the header, table, and footer
 *                     can directly be added.
 */
snapwebsites.eCommerceCart.prototype.generate_cart_html(e)
{
    this.generate_cart_header(e);
    this.generate_product_table(e);
    this.generate_cart_footer(e);
};


/** \brief Generate a cart header.
 *
 * This function creates the header of the cart. This is generally
 * information about the store and such. It should remains really
 * brief (one line or two.)
 *
 * @param {!jQuery} e  The body element where the header, table, and footer
 *                     can directly be added.
 */
snapwebsites.eCommerceCart.prototype.generate_cart_header(e)
{
    e.append("<div class='cart-header'>Cart</div>");
};


/** \brief Generate a cart footer.
 *
 * The function creates the footer of the cart. This is generally
 * detailed information about the store letigimacy (i.e. certificates)
 * and eventually some reference to other products one can purchase.
 *
 * @param {!jQuery} e  The body element where the header, table, and footer
 *                     can directly be added.
 */
snapwebsites.eCommerceCart.prototype.generate_cart_footer(e)
{
    e.append("<div class='cart-footer'>Thank you.</div>");
};


/** \brief Generate the product table of the cart.
 *
 * This function generates a list of products in a table. It makes use
 * of the list of products available in the cart. In case all the products
 * get removed, we also offer a fallback when no products are defined in
 * the cart.
 *
 * @param {!jQuery} e  The body element where the header, table, and footer
 *                     can directly be added.
 */
snapwebsites.eCommerceCart.prototype.generate_product_table(e)
{
    var t,      // table
        h,      // header
        b,      // body
        f,      // footer
        i,
        max = this.products_.length;

    // table
    e.append("<table class='cart-product-table'>");
    t = e.children("table");

    // header
    t.append("<thead>");
    h = t.children("thead");
    this.generate_product_table_header(h);

    // footer (must appear before <tbody> to be HTML 4.1 compatible)
    t.append("<tfoot>");
    f = t.children("tfoot");
    this.generate_product_table_footer(f);

    // body
    t.append("<tbody>");
    b = t.children("tbody");

    for(i = 0; i < max; ++i)
    {
        this.generate_product_table_row(b, this.products_[i]);
    }
};


/** \brief Generate the product table header.
 *
 * This function generates the table header for the product table.
 *
 * It is unlikely that you'd want to override this function, unless you
 * want to add a column or two (or remove some columns.)
 *
 * If you do change the number of columns, remember that you will have
 * to also override the generate_product_table_row() function to adjust
 * the table accordingly.
 *
 * @param {!jQuery} e  The body element where the header, table, and footer
 *                     can directly be added.
 */
snapwebsites.eCommerceCart.prototype.generate_product_table_header(e)
{
    e.append("<tr>"
             + "<th class='cart-line-number'>#</th>"
             + "<th class='cart-description'>Description</th>"
             + "<th class='cart-quantity'>Quantity</th>"
             + "<th class='cart-unit-price'>Unit Price</th>"
             + "<th class='cart-total'>Total</th>"
           + "</tr>");
};


/** \brief Generate the product table footer.
 *
 * This function generates the table footer for the product table.
 *
 * It is likely that you'd want to override this function if you want
 * to add "special" rows to the table. For example the shipping extension
 * adds a shipping line and the taxes extension adds a line with the
 * total amount of taxes.
 *
 * If you do change the number of columns, remember that you will have
 * to also override the generate_product_table_row() function to adjust
 * the table accordingly.
 *
 * @param {!jQuery} e  The body element where the header, table, and footer
 *                     can directly be added.
 */
snapwebsites.eCommerceCart.prototype.generate_product_table_footer(e)
{
    //if(shipping) // this needs to be added by the shipping plugin
    //{
    //    e.append("<tr>"
    //             + "<td class='cart-line-number'></td>"
    //             + "<td class='cart-description'></td>"
    //             + "<td class='cart-quantity'></td>"
    //             + "<td class='cart-unit-price'>Grand Total: </td>"
    //             + "<td class='cart-total'>$###.##</td>"
    //           + "</tr>");
    //}
    //if(taxes) // this needs to be added by the taxes plugin
    //{
    //    e.append("<tr>"
    //             + "<td class='cart-line-number'></td>"
    //             + "<td class='cart-description'></td>"
    //             + "<td class='cart-quantity'></td>"
    //             + "<td class='cart-unit-price'>Grand Total: </td>"
    //             + "<td class='cart-total'>$###.##</td>"
    //           + "</tr>");
    //}
    e.append("<tr>"
             + "<td class='cart-line-number'></td>"
             + "<td class='cart-description'></td>"
             + "<td class='cart-quantity'></td>"
             + "<td class='cart-unit-price'>Grand Total: </td>"
             + "<td class='cart-total'>$###.##</td>"
           + "</tr>");
};


/** \brief Snap eCommerceProduct constructor.
 *
 * For each of the product you add to the Cart, one of these
 * is created on the client system.
 *
 * \code
 * class eCommerceProduct
 * {
 * public:
 *      function eCommerceProduct(cart: eCommerceCart, path: string);
 *      final function getName() : string;
 *      final function wasModified(opt_recheck: boolean) : boolean;
 *      final function saving() : SaveData;
 *      final function saved(data: SaveData) : boolean;
 *      final function discard();
 *      function getCart() : eCommerceCart;
 *      function enable() : Void;
 *      function disable() : Void;
 *      final function getWidget() : jQuery;
 *      final function getWidgetContent() : jQuery;
 *      final function checkForBackgroundValue();
 *      static final function isEmptyBlock(html: string|jQuery) : boolean;
 *      function getValue() : string;
 *      function resetValue(changed: boolean) : void;
 *      function setValue(value: Object, changed: boolean) : void;
 *      function getProductType() : EditorWidgetTypeBase;
 *
 * private:
 *      var editorBase_: EditorBase = null;
 *      var editorForm_: EditorForm = null;
 *      var widget_: jQuery = null;
 *      var widgetContent_ : jQuery = null;
 *      var name_: string;
 *      var originalData_: string = "";
 *      var modified_: boolean = false;
 *      var widgetType_: EditorWidgetTypeBase = null;
 * };
 * \endcode
 *
 * @param {snapwebsites.EditorBase} editor_base  A reference to the editor base object.
 * @param {snapwebsites.EditorForm} editor_form  A reference to the editor form object that owns this widget.
 * @param {jQuery} widget  The jQuery object representing this editor toolbar.
 *
 * @return {snapwebsites.EditorWidget} The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.EditorWidget = function(editor_base, editor_form, widget)
{
    var type = snapwebsites.castToString(widget.attr("field_type"), "field_type attribute");

    this.editorBase_ = editor_base;
    this.editorForm_ = editor_form;
    this.widget_ = widget; // this is the jQuery widget (.snap-editor)
    this.widgetContent_ = widget.children(".editor-content");
    if(this.widgetContent_.length == 0)
    {
        alert("Widget must define a tag with class \"editor-content\"");
        throw new Error("Widget must define a tag with class \"editor-content\"");
    }
    this.name_ = snapwebsites.castToString(widget.attr("field_name"), "field_name attribute");
    // Moved to AFTER the [pre]initialization
    //this.originalData_ = snapwebsites.castToString(this.widgetContent_.html(), "widgetContent HTML in EditorWidget constructor for " + this.name_);
    this.widgetType_ = editor_base.getWidgetType(type);
    this.checkForBackgroundValue();

//#ifdef DEBUG
    // these should be caught on the server side too but this test
    // ensures that we catch invalid names of dynamically created widgets
    // (i.e. in most cases the widget names are "hard coded" in your XSLT
    // files and as such can be checked even before we add the file to the
    // layout table)
    if(this.name_.length > 0
    && this.name_[0] === "_")
    {
        throw new Error("The widget name \"" + this.name_ + "\" cannot start with an underscore as such names are reserved by the system.");
    }
//#endif

    // this should be last
    this.widgetType_.preInitializeWidget(this);

    return this;
};


/** \brief Mark EditorWidget as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.EditorWidget);


/** \brief The editor base object.
 *
 * The a reference back to the editor base object. This is pretty much
 * the same as the snapwebsites.EditorInstance reference.
 *
 * @type {snapwebsites.EditorBase}
 * @private
 */
snapwebsites.EditorWidget.prototype.editorBase_ = null;


/** \brief The form that owns this widget.
 *
 * This a reference back to the editor form object.
 *
 * @type {snapwebsites.EditorForm}
 * @private
 */
snapwebsites.EditorWidget.prototype.editorForm_ = null;


/** \brief The jQuery widget.
 *
 * This parameter represents the jQuery widget. The actual visible
 * editor widget.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorWidget.prototype.widget_ = null;


/** \brief A jQuery wait widget.
 *
 * This parameter represents a jQuery widget created when the
 * showWaitImage() is called.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorWidget.prototype.waitWidget_ = null;


/** \brief The timer identifier.
 *
 * This parameter is the identifier returned by the setInterval() function.
 *
 * @type {number}
 * @private
 */
snapwebsites.EditorWidget.prototype.waitWidgetTimerID_ = -1;


/** \brief A jQuery wait widget current rotation position.
 *
 * This parameter represents the position of the image in the wait
 * widget. The wait widget has a background which position is moved
 * from 0 to -704 so it looks as it was rotating.
 *
 * @type {number}
 * @private
 */
snapwebsites.EditorWidget.prototype.waitWidgetPosition_ = 0;


/** \brief The jQuery widget representing the content of the widget.
 *
 * This parameter is the DIV under the widget which is marked as the
 * "editor-content" (i.e. with the class named "editor-content").
 * This is the part that we manage in the different widget
 * implementations.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorWidget.prototype.widgetContent_ = null;


/** \brief The name of the widget.
 *
 * Whenever the widget is created this field is set to the name
 * of the widget (the "field_name" attribute.)
 *
 * @const
 * @type {!string}
 * @private
 */
snapwebsites.EditorWidget.prototype.name_;


/** \brief The original data of the widget.
 *
 * Until saved, this data is taken from the existing widget at
 * initialization time.
 *
 * @type {string}
 * @private
 */
snapwebsites.EditorWidget.prototype.originalData_ = "";


/** \brief Whether the system detected that the widget was modified.
 *
 * To avoid comparing possibly very large buffers (originalData_ and
 * the widget html() data) we save the last result in this variable.
 * Once it is true, we just return true when calling the wasModified()
 * function.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorWidget.prototype.modified_ = false;


/** \brief The type of this widget.
 *
 * The object representing the type of this widget. It is used to
 * finish the initialization of the widget (i.e. connect to the
 * signals, etc.)
 *
 * @type {snapwebsites.EditorWidgetTypeBase}
 * @private
 */
snapwebsites.EditorWidget.prototype.widgetType_ = null;


/** \brief Get the name of this widget.
 *
 * On creation the widget retrieves the attribute named "field_name"
 * which contains the name of the widget (used to communicate
 * with the server.) This function returns that name.
 *
 * @return {string}  The name of the widget.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.getName = function()
{
    return this.name_;
};


/** \brief Check whether the widget was modified.
 *
 * This function compares the widget old and current data to see
 * whether it changed.
 *
 * @param {?boolean} opt_recheck  Whether we want to force a check of
 *                                the current HTML data with the original.
 *
 * @return {boolean}  Whether the widget was modified (true) or not
 *                    (false).
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.wasModified = function(opt_recheck)
{
    if(opt_recheck || !this.modified_)
    {
        this.modified_ = this.originalData_ != this.widgetContent_.html();
    }
    return this.modified_;
};


/** \brief Get the data to be saved.
 *
 * This function is called whenever the widget is marked as modified
 * or the form as "always save all". It retrieves the data being
 * saved and the result to be sent to the server. The result may
 * be quite optimized, for example, in case of radio buttons, just
 * one (small) value is sent to the server.
 *
 * By default the data.result gets cleaned up so no starting or
 * ending blanks are kept. It also removes \<br\> tags that have
 * no attributes.
 *
 * This function cannot be overridden. There is no need because the
 * editor creates the EditorWidget objects anyway. Instead you want
 * to derive the saving() function of the widget type.
 *
 * @return {snapwebsites.EditorWidgetTypeBase.SaveData}  The data to be saved.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.saving = function()
{
    var value,          // in case the widget defines a value attribute
        data = {};      // the data to be returned

    data.html = snapwebsites.castToString(this.widgetContent_.html(), "widgetContent HTML in saving()");

    value = this.widgetContent_.attr("value");
    if(typeof value !== "undefined")
    {
        // some widget save their current state as the "value" attribute
        // so we can quickly retrieve that and return it as the result
        // (note that in this case we even skip calling the widget type
        // saving() function since the value is already the final result.)
        data.result = snapwebsites.castToString(value, "widgetContent value attribute");
    }
    else
    {
        // by default the result (what is sent to the server) is the
        // same as the HTML data, but widgets are free to chnage that
        // value (i.e. checkmark widgets set it to 0 or 1).
        //
        // TBD: should we also remove <br> tags with attributes?
        // TBD: should we remove <hr> tags at the start/end too?
        data.result = snapwebsites.castToString(
                            data.html.replace(/^(<br *\/?>| |\t|\n|\r|\v|\f|&nbsp;|&#160;|&#xA0;)+/, "")
                                     .replace(/(<br *\/?>| |\t|\n|\r|\v|\f|&nbsp;|&#160;|&#xA0;)+$/, ""),
                            "data html trimmed"
                      );

        this.widgetType_.saving(this, data);
    }

    return data;
};


/** \brief Call after a widget was saved.
 *
 * This function reset the original data and modified flags when called.
 * It is expected to be called on a successful save.
 *
 * @param {snapwebsites.EditorWidgetTypeBase.SaveData} data  The data that was
 *        saved (what saving() returned.)
 *
 * @return {boolean}  Whether the widget was modified (true) or not
 *                    (false).
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.saved = function(data)
{
    this.originalData_ = data.html;
    return this.wasModified(true);
};


/** \brief Generally called when you want to ignore changes in a widget.
 *
 * This function resets the original data from the current data. This is
 * useful to pretend that the widget was saved or you want to ignore the
 * changes.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.discard = function()
{
    this.originalData_ = snapwebsites.castToString(this.widgetContent_.html(), "widgetContent HTML in EditorWidget constructor for " + this.name_);
    this.wasModified(true);
};


/** \brief Retrieve the editor form object.
 *
 * This function returns the editor form object.
 *
 * @return {snapwebsites.EditorForm} The editor form object.
 */
snapwebsites.EditorWidget.prototype.getEditorForm = function()
{
    return this.editorForm_;
};


/** \brief Retrieve the editor base object.
 *
 * This function returns the editor base object as passed to
 * the constructor.
 *
 * @return {snapwebsites.EditorBase} The editor base object.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.getEditorBase = function()
{
    return this.editorBase_;
};


/** \brief Set the focus to this widget if possible.
 *
 * This function is used to set the focus to this widget.
 */
snapwebsites.EditorWidget.prototype.focus = function()
{
    var id = this.widgetContent_.focus();
};


/** \brief Show the widget.
 *
 * This function is used to show the widget. If there is a label defined
 * for that widget (i.e. with the for="..." attribute properly defined)
 * then the label gets hidden too.
 *
 * \todo
 * Add signatures as supported by jQuery (duration, complete), (options).
 */
snapwebsites.EditorWidget.prototype.show = function()
{
    var id = this.widget_.attr("field_name");

    this.widget_.show();

    if(id)
    {
        jQuery("label[for='" + id + "']").show();
    }
};


/** \brief Hide the widget.
 *
 * This function is used to hide the widget. If there is a label defined
 * for that widget (i.e. with the for="..." attribute properly defined)
 * then the label gets hidden too.
 *
 * \todo
 * Add signatures as supported by jQuery (duration, complete), (options).
 */
snapwebsites.EditorWidget.prototype.hide = function()
{
    var id = this.widget_.attr("field_name");

    this.widget_.hide();

    if(id)
    {
        jQuery("label[for='" + id + "']").hide();
    }
};


/** \brief Enable the widget.
 *
 * This function is used to enable the widget.
 *
 * The function simply adds or removes the "disabled" class.
 *
 * By default, if the \p state parameter is not specified, the function
 * enables the widget. If the state is specified and it is true, the
 * widget is also enabled. When the state is specified and is false,
 * then it adds the disabled class.
 *
 * \important
 * If the state is not of type Boolean, then it is ignored and the
 * widget is always enabled. You may convert any value to a Boolean
 * value using the logical not (!) operator.
 *
 * @param {boolean=} opt_state  The new state, if true (default) enable,
 *                              otherwise disable the widget.
 */
snapwebsites.EditorWidget.prototype.enable = function(opt_state)
{
    if(typeof opt_state === "boolean" && !opt_state)
    {
        this.widget_.addClass("disabled");
    }
    else
    {
        this.widget_.removeClass("disabled");
    }
};


/** \brief Disable the widget.
 *
 * This function is used to disable the widget.
 *
 * The function calls enable() with the value false as the state.
 *
 * Note that the disable function does not accept a parameter.
 */
snapwebsites.EditorWidget.prototype.disable = function()
{
    this.enable(false);
};


/** \brief Retrieve the jQuery widget.
 *
 * This function returns the jQuery widget attached to this
 * editor widget.
 *
 * @return {jQuery} The jQuery widget as passed to the constructor.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.getWidget = function()
{
    return this.widget_;
};


/** \brief Retrieve the jQuery widget content.
 *
 * This function returns the jQuery widget attached to this
 * editor widget which holds the content of the field represented
 * by this widget.
 *
 * This is the child of the widget with class "editor-content".
 *
 * @return {jQuery} The jQuery widget content.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.getWidgetContent = function()
{
    return this.widgetContent_;
};


/** \brief Check whether the background value should be shown.
 *
 * This function checks whether the background value of this widget
 * should be shown which happens when the widget is empty.
 *
 * @final
 */
snapwebsites.EditorWidget.prototype.checkForBackgroundValue = function()
{
    this.widget_.children(".snap-editor-background").toggle(snapwebsites.EditorWidget.isEmptyBlock(this.widgetContent_.html()));
};


/** \brief Check whether a block of HTML is empty or not.
 *
 * This function checks the HTML in the specified \p html parameter
 * and after cleaning up the string, returns true if it ends up
 * being empty.
 *
 * @param {string|jQuery} html
 *
 * @return {boolean}  true if the string can be considered empty.
 *
 * @final
 */
snapwebsites.EditorWidget.isEmptyBlock = function(html) // static
{
//#ifdef DEBUG
    if(typeof html !== "string")
    {
        throw new Error("snapwebsites.EditorBase.isEmptyBlock() called with a parameter which is not a string (" + (typeof html) + ")");
    }
//#endif

    //
    // replace all the nothingness by ""
    // and see whether the result is the empty string
    //
    // WARNING: in the following, if html is empty on entry then
    //          the result is still true but just a match against
    //          the regex would return false on the empty string
    //
    return html.replace(/^(<br *\/?>| |\t|\n|\r|&nbsp;)+$/, "").length === 0;
};


/** \brief Retrieve the current value of this widget.
 *
 * This function returns the current value of the widget.
 * Note that some widget do not really have a string as a value, you may
 * want to check out the widget type you are handling to know what it
 * returns.
 *
 * This is a helper function that is a replacement to the following:
 *
 * \code
 *      // get the value at once
 *      var value = widget.getValue();
 *
 *      // get the value and raw data
 *      var data = widget.saving();
 *      var value = data.result;
 * \endcode
 *
 * @return {string}  The value of this widget.
 */
snapwebsites.EditorWidget.prototype.getValue = function()
{
    var data = this.saving();
    return data.result;
};


/** \brief Reset the value of the widget.
 *
 * This function resets the value of this widget. This generally only
 * works with widgets that have a form of default value.
 *
 * If the parameters 'changed' is set to true then the widget is marked
 * as modified, otherwise the modified flag is not changed.
 *
 * @param {!boolean} changed  Whether to mark the widget as modified.
 *
 * @return {boolean}  true if the value got set.
 */
snapwebsites.EditorWidget.prototype.resetValue = function(changed)
{
    var data;

    if(!this.widgetType_.resetValue(this))
    {
        // could not even change the value
        return false;
    }
    if(changed)
    {
        this.wasModified(true);
    }
    else
    {
        // programmer views this change as a 'current status' so we
        // prevent the wasModified() from being true in this case
        data = this.saving();
        this.saved(data);
    }
    this.checkForBackgroundValue();

    // it was set, whether it was modified is not our concern when returning
    return true;
};


/** \brief Set the value of the widget.
 *
 * This function sets the specified value as the new value of the widget.
 *
 * If the parameters 'changed' is set to true and the new value is
 * different from the existing value, then the widget is marked as
 * modified, otherwise the modified flag is not changed (it may
 * actually get reset if you restore the value in this way.)
 *
 * @param {!Object} value  The new widget value.
 * @param {!boolean} changed  Whether to mark the widget as modified.
 *
 * @return {boolean}  true if the value got set.
 */
snapwebsites.EditorWidget.prototype.setValue = function(value, changed)
{
    var data;

    if(!this.widgetType_.setValue(this, value))
    {
        // could not even change the value
        return false;
    }
    if(changed)
    {
        this.wasModified(true);
    }
    else
    {
        // programmer views this change as a 'current status' so we
        // prevent the wasModified() from being true in this case
        data = this.saving();
        this.saved(data);
    }
    this.checkForBackgroundValue();

    // it was set, whether it was modified is not our concern when returning
    return true;
};


/** \brief Get the type of this widget.
 *
 * This function returns the reference to the editor widget type.
 *
 * @return {snapwebsites.EditorWidgetTypeBase}
 */
snapwebsites.EditorWidget.prototype.getWidgetType = function()
{
    return this.widgetType_;
};


/** \brief Show a wait image inside this widget.
 *
 * This function creates a wait image inside the widget and then shows
 * it rotating.
 *
 * Any widget that needs to do a computation that will take time or
 * communicate with the server should probably use this function.
 *
 * \note
 * If the wait image is already visible, then nothing happens.
 */
snapwebsites.EditorWidget.prototype.showWaitImage = function()
{
    var that = this,
        w = this.getWidget();

    if(!this.waitWidget_)
    {
        //wait_image = w.children(".widget-wait-image");
        w.prepend("<div class=\"widget-wait-image\"/>");
        this.waitWidget_ = w.children(".widget-wait-image");
    }

    if(this.waitWidgetTimerID_ === -1)
    {
        this.waitWidget_.fadeIn(1000);
        this.waitWidgetTimerID_ = setInterval(
            function()
            {
                that.rotateWaitImage_();
            },
            200);
    }

    //
    // The rotateWait_ works but uses 100% of the CPU, so no good.
    // (especially for a wait!) At this time make use of a GIF instead.
    // The rotate is still very cool for a one time animation of 1
    // second or less.
    //
    // /** \brief A RotateAnimation object.
    //  *
    //  * This object is used to rotate the wait image by 30 degrees every 200ms.
    //  *
    //  * It gets created whenever a file is dropped and a preview is being
    //  * generated.
    //  *
    //  * @type {snapwebsites.RotateAnimation}
    //  * @private
    //  */
    // snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.rotateWait_ = null;
    //
    //if(!this.rotateWait_)
    //{
    //    this.rotateWait_ = new snapwebsites.RotateAnimation(wait_image, 30);
    //}
    //this.rotateWait_.start();
};


/** \brief Once the wait is over, hide it.
 *
 * This function is used to hide the wait image in this widget. It also
 * makes sure to stop the animation.
 */
snapwebsites.EditorWidget.prototype.hideWaitImage = function()
{
    if(this.waitWidgetTimerID_ !== -1)
    {
        clearTimeout(this.waitWidgetTimerID_);
        this.waitWidgetTimerID_ = -1;
        this.waitWidget_.fadeOut(200);
    }
};


/** \brief Rotate the wait image by 30 degrees.
 *
 * Each time we rotate the wait image we turn it by 30 degrees. This
 * function increments a counter that wraps back to zero when it
 * reaches 12.
 *
 * The function changes the background position to give the effect that
 * the image rotates.
 *
 * On my computer this animation takes less than 7% of the CPU. So I think
 * it is still acceptable.
 *
 * @private
 */
snapwebsites.EditorWidget.prototype.rotateWaitImage_ = function()
{
    this.waitWidgetPosition_ = (this.waitWidgetPosition_ + 1) % 12;
    this.waitWidget_.css('background-position', (this.waitWidgetPosition_ * -64) + 'px 0');
};



/** \brief Snap EditorFormBase constructor.
 *
 * The EditorForm inherits the EditorFormBase.
 *
 * \code
 * class EditorFormBase : public ServerAccessCallbacks
 * {
 * public:
 *      function EditorFormBase(editor_base, session) : EditorFormBase;
 *      function getEditorBase() : EditorBase;
 *      function getFormWidget() : jQuery;
 *      virtual function saveData();
 *      virtual function serverAccessSuccess(result);
 *      virtual function serverAccessError(result);
 *      virtual function serverAccessComplete(result);
 *
 *      static SAVE_MODE_PUBLISH: string;
 *      static SAVE_MODE_SAVE: string;
 *      static SAVE_MODE_SAVE_NEW_BRANCH: string;
 *      static SAVE_MODE_SAVE_DRAFT: string;
 *
 * private:
 *      editorBase_: EditorBase;
 *      formWidget_: jQuery;
 * };
 * \endcode
 *
 * @param {snapwebsites.EditorBase} editor_base  The base editor object.
 * @param {jQuery} form_widget  The editor form DOM in a jQuery object.
 *
 * @return {!snapwebsites.EditorFormBase}  The newly created object.
 *
 * @extends {snapwebsites.ServerAccessCallbacks}
 * @constructor
 * @struct
 */
snapwebsites.EditorFormBase = function(editor_base, form_widget)
{
    snapwebsites.EditorFormBase.superClass_.constructor.call(this);

    this.editorBase_ = editor_base;
    this.formWidget_ = form_widget;

    return this;
};


/** \brief The EditorFormBase derives from ServerAccessCallbacks.
 *
 * This class inherits from the ServerAccessCallbacks so it can receive
 * callback events after a AJAX object was sent to the server.
 */
snapwebsites.inherits(snapwebsites.EditorFormBase, snapwebsites.ServerAccessCallbacks);


/** \brief Save the data and then publish it.
 *
 * When saving data to the database, it creates a new revision nearly
 * every single time (there is one case where it does not, but that's
 * probably a bug...)
 *
 * When the user publishes, that new revision becomes the current
 * revision. Otherwise the current revision does not change. This
 * allows for revisions to be checked and eventually corrected
 * before publication of the content.
 *
 * @type {string}
 * @const
 */
snapwebsites.EditorFormBase.SAVE_MODE_PUBLISH = "publish";


/** \brief Save the data in a new revision.
 *
 * Save the data entered in the page to a new revision entry. The
 * new entry is available for review, but it does not get shown
 * to people who could not edit the page in some way.
 *
 * Once everyone approaved of a page, the page can get published by
 * asking the system to show that specific revision.
 *
 * @type {string}
 * @const
 */
snapwebsites.EditorFormBase.SAVE_MODE_SAVE = "save";


/** \brief Create a new branch.
 *
 * This function saves the current data in a new branch. One can
 * create a new branch to make changes to the page such as adding
 * or removing tags and yet leave the current branch as the publicly
 * visible data.
 *
 * This gives the user time to work on his new content until he's
 * ready to publish it.
 *
 * @type {string}
 * @const
 */
snapwebsites.EditorFormBase.SAVE_MODE_SAVE_NEW_BRANCH = "save-new-branch";


/** \brief Save the data as a draft.
 *
 * Snap supports drafts to save the data being typed (in case the
 * browser or even the whole client computer crashes, the electricity
 * goes out, etc.)
 *
 * Drafts are special in that they get saved in a special location
 * instead of the normal revision.
 *
 * @type {string}
 * @const
 */
snapwebsites.EditorFormBase.SAVE_MODE_SAVE_DRAFT = "save-draft";


/** \brief A reference to the base editor object.
 *
 * This value is a reference to the base editor object so the
 * EditorForm objects can access it.
 *
 * @type {snapwebsites.EditorBase}
 * @private
 */
snapwebsites.EditorFormBase.prototype.editorBase_ = null;


/** \brief The jQuery object representing this form.
 *
 * This variable represents the DOM form as a jQuery object. It is
 * given to the EditorForm on creation.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorFormBase.prototype.formWidget_ = null;


/** \brief Retrieve the editor base object.
 *
 * This function returns the editor base object as passed to
 * the constructor.
 *
 * @return {snapwebsites.EditorBase} The editor base object.
 * @final
 */
snapwebsites.EditorFormBase.prototype.getEditorBase = function()
{
    return this.editorBase_;
};


/** \brief Retrieve the jQuery form widget.
 *
 * This function returns the form widget.
 *
 * @return {jQuery}
 * @final
 */
snapwebsites.EditorFormBase.prototype.getFormWidget = function()
{
    return this.formWidget_;
};


/*jslint unparam: true */
/** \brief Save the form data.
 *
 * This function is called to save the data. It generally happens in
 * two cases:
 *
 * \li When the user clicks on of the submit button (Publish, Save,
 *     Save New Branch, and Save Draft);
 * \li When a timer times out and the auto-save to draft feature is
 *     allowed.
 *
 * The supported modes are defined as constants:
 *
 * \li SAVE_MODE_PUBLISH
 * \li SAVE_MODE_SAVE
 * \li SAVE_MODE_SAVE_NEW_BRANCH
 * \li SAVE_MODE_SAVE_DRAFT
 *
 * @throws {Error} The function throws an error if the base class
 *                 version is called (i.e. not implemented.)
 *
 * @param {string} mode  The mode used to save the data.
 */
snapwebsites.EditorFormBase.prototype.saveData = function(mode) // virtual
{
    throw new Error("snapwebsites.EditorFormBase.saveData() was called which means it was not properly overridden");
};
/*jslint unparam: false */



/** \brief Dialog used to show different Save options.
 *
 * By default the editor offers a simple save dialog which includes
 * several buttons used to save the changes to a page. The offered
 * buttons are:
 *
 * \li Publish -- save the current data and make it an official revision
 * \li Save -- save the changes, but do not make it the official revision
 * \li Save New Branch -- save the changes in a new branch
 * \li Save Draft -- save the data as a draft (no revision in this case)
 *
 * Editor forms used as standard form most often override the default
 * DOM buttons with just a Save button which pretty much acts like
 * the Publish button.
 *
 * \code
 * class SaveEditorDialog
 * {
 * public:
 *      function setPopup(widget: Element|jQuery);
 *      function open();
 *      function close();
 *      function setStatus(new_status: boolean);
 *
 * private:
 *      var editorForm_: EditorFormBase;
 *      var saveDialogPopup_: jQuery;
 *      function create_();
 * };
 * \endcode
 *
 * \todo
 * The branch the user decided to edit (i.e. with the query string
 * ...?a=edit&revision=1.2) needs to be taken in account as well.
 *
 * @param {snapwebsites.EditorFormBase}  editor_form The editor form that
 *                                       created this editor save dialog.
 *
 * @constructor
 */
snapwebsites.EditorSaveDialog = function(editor_form)
{
    this.editorForm_ = editor_form;

    return this;
};


/** \brief Mark EditorBase as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.EditorSaveDialog);


/** \brief An editor form reference.
 *
 * This parameter holds the editor form that created this save dialog.
 * It is used to call the saveData() function on the correct editor
 * form.
 *
 * @type {snapwebsites.EditorFormBase}
 *
 * @private
 */
snapwebsites.EditorSaveDialog.prototype.editorForm_ = null;


/** \brief The jQuery of the DOM representing the save dialog.
 *
 * This parameter holds the jQuery object referencing the DOM representing
 * the save dialog.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorSaveDialog.prototype.saveDialogPopup_ = null;


/** \brief Create the default save dialog DOM.
 *
 * This function creates the default dialog DOM parameters. If you
 * call the setDialogDOM() function before the dialog needs to
 * be opened, then the default DOM will never get used.
 *
 * @private
 */
snapwebsites.EditorSaveDialog.prototype.create_ = function()
{
    var that = this,
        html = "<div id='snap_editor_save_dialog'>"
            + "<h3 class='title'>Editor</h3>"
            + "<div id='snap_editor_save_dialog_page'>"
            + "<p class='description'>You made changes to your page. Make sure to save your modifications.</p>"
            // this is wrong at this point because the current branch
            // management is more complicated...
            // (i.e. if you are editing a new branch that is not
            //       public then Publish would make that branch
            //       public and the Save would make that too?!)
            + "<p class='snap_editor_publish_p'><a class='button' id='snap_editor_publish' href='#'>Publish</a></p>"
            + "<p class='snap_editor_save_p'><a class='button' id='snap_editor_save' href='#'>Save</a></p>"
            + "<p class='snap_editor_save_new_branch_p'><a class='button' id='snap_editor_save_new_branch' href='#'>Save New Branch</a></p>"
            + "<p class='snap_editor_save_draft_p'><a class='button' id='snap_editor_save_draft' href='#'>Save Draft</a></p>"
            + "</div></div>";

    jQuery(html).appendTo("body");

    this.saveDialogPopup_ = jQuery("#snap_editor_save_dialog");

    // very simple positioning at this point
    this.saveDialogPopup_.css("left", jQuery(window).outerWidth(true) - 190);

    jQuery("#snap_editor_publish")
        .click(function(){
            that.editorForm_.saveData(snapwebsites.EditorFormBase.SAVE_MODE_PUBLISH);
        });
    jQuery("#snap_editor_save")
        .click(function(){
            that.editorForm_.saveData(snapwebsites.EditorFormBase.SAVE_MODE_SAVE);
        });
    jQuery("#snap_editor_save_new_branch")
        .click(function(){
            alert("Save New Branch! (to be implemented)");
        });
    jQuery("#snap_editor_save_draft")
        .click(function(){
            that.editorForm_.saveData(snapwebsites.EditorFormBase.SAVE_MODE_SAVE_DRAFT);
        });

    // while creating a new page, the page is kept under "admin/drafts"
    // and in that case "save" and "save new branch" do not make sense
    if(jQuery("meta[name='path']").attr("content") === "admin/drafts")
    {
        jQuery(".snap_editor_save_p").hide();
        jQuery(".snap_editor_save_new_branch_p").hide();
    }
};


/** \brief Define the save dialog popup to use for this dialog.
 *
 * In many cases you will define your own buttons for a dialog.
 * This function let you do that. Note that the editor expects those
 * to be hidden by default. It will show them whenever necessary
 * (i.e. when something changed.)
 *
 * \note
 * This function forces an immediate hide on the save popup dialog.
 *
 * \todo
 * Offer ways to just disable buttons (instead of hiding them)
 * and to keep the buttons "active" but generate errors if clicked
 * while currently saving.
 *
 * @param {Element|jQuery} widget  A DOM or jQuery widget.
 */
snapwebsites.EditorSaveDialog.prototype.setPopup = function(widget)
{
    this.saveDialogPopup_ = jQuery(widget);
    this.saveDialogPopup_.hide();
};


/** \brief Open the save dialog.
 *
 * This function is generally called whenever the user makes a change
 * that needs to be sent to the server.
 *
 * First the function makes sure that a dialog is defined, if not it
 * initializes the default dialog.
 *
 * Then it shows the dialog.
 *
 * \note
 * This function does not change the dialog buttons status. The
 * save function is expected to call the saveDialogStatus() function
 * for that purpose.
 *
 * \todo
 * The positioning of the dialog is done at creation time so it can be
 * a problem if it is expected (for example) to not overlay the area
 * being edited.
 */
snapwebsites.EditorSaveDialog.prototype.open = function()
{
    if(!this.saveDialogPopup_)
    {
        this.create_();
    }
    this.saveDialogPopup_.fadeIn(300).css("display", "block");
};


/** \brief Close the save dialog popup.
 *
 * Once a save completed, the editor form checks whether other
 * modifications were performed while saving, if not, then the
 * save dialog gets closed.
 *
 * \note
 * This function does not change the status of the dialog buttons.
 * The save function is expected to call the setStatus() function
 * for that purpose.
 */
snapwebsites.EditorSaveDialog.prototype.close = function()
{
    if(this.saveDialogPopup_)
    {
        this.saveDialogPopup_.fadeOut(300);
    }
    // else throw?
};


/** \brief Setup the save dialog status.
 *
 * When a user clicks on a save dialog button, you should call this
 * function to disable the dialog.
 *
 * \warning
 * The function throws if the popup is not yet defined. You should not
 * be able to save without the dialog having been created so that
 * should not happen. That being said, it means we cannot call this
 * function before we called the open() function.
 *
 * @param {boolean} new_status  Whether the widget is enabled (true)
 *                              or disabled (false).
 */
snapwebsites.EditorSaveDialog.prototype.setStatus = function(new_status)
{
//#ifdef DEBUG
    // dialog even exists?
    if(!this.saveDialogPopup_)
    {
        throw new Error("EditorSaveDialog.setStatus() called without a save dialog popup defined.");
    }
//#endif

    if(new_status)
    {
        this.saveDialogPopup_.parent().children("a").removeClass("disabled");
    }
    else
    {
        this.saveDialogPopup_.parent().children("a").addClass("disabled");
    }
};



/** \brief Snap EditorForm constructor.
 *
 * \code
 * class EditorForm extends EditorFormBase
 * {
 * public:
 *      function EditorForm() : EditorForm;
 *      function getName() : string;
 *      function getWidgetByName(name: string) : EditorWidget;
 *      function getSession() : string;
 *      function setInPopup(in_popup: boolean);
 *      function getToolbarAutoVisible() : boolean;
 *      function setToolbarAutoVisible(toolbar_auto_visible: boolean);
 *      virtual function saveData(mode: string);
 *      virtual function serverAccessSuccess(result: ResultData);
 *      virtual function serverAccessError(result: ResultData);
 *      virtual function serverAccessComplete(result: ResultData);
 *      function isSaving() : boolean;
 *      function setSaving(new_status: boolean, will_redirect: boolean);
 *      function changed();
 *      function getSaveDialog() : EditorSaveDialog;
 *      function newTypeRegistered();
 *      function wasModified(recheck: boolean) : boolean;
 *      function earlyClose() : boolean;
 *      static function titleToURI(title: string) : string;
 *
 * private:
 *      function readyWidgets_();
 *
 *      var usedTypes_: Object;                 // map of types necessary to open that form
 *      var session_: string;                   // session identifier
 *      var name_: string;                      // the name of the form
 *      var widgets_: jQuery;                   // the widgets of this form
 *      var editorWidgets_: Object;             // map of editor objects
 *      var widgetInitialized_: boolean;        // whether the form was initialized
 *      var saveDialog_: EditorSaveDialog;      // a reference to the save dialog
 *      var mode_: string;                      // general mode the form is used as
 *      var toolbarAutoVisible: boolean;        // whether to show the toolbar automatically
 *      var modified_: boolean;                 // one or more fields changed
 *      var saveFunctionOnSuccess_: function(editor_form: EditorForm, result: snapwebsites.ServerAccessCallbacks.ResultData);
 *                                              // function called in case the save succeeded
 *      var saveFunctionOnError_: function(editor_form: EditorForm, result: snapwebsites.ServerAccessCallbacks.ResultData);
 *                                              // function called in case the save failed
 *      var savedData_: Object;                 // a set of objects to know whether things changed while saving
 *      var serverAccess_: ServerAccess;        // a ServerAccess object to send the AJAX
 * };
 * \endcode
 *
 * \note
 * The Snap! EditorForm objects are created as required based on the DOM.
 * If the DOM is dynamically updated to add more forms, then it may require
 * special handling (TBD at this point) to make sure that the new forms
 * are handled by the editor.
 *
 * @param {snapwebsites.EditorBase} editor_base  The base editor object.
 * @param {jQuery} form_widget  The editor form DOM in a jQuery object.
 * @param {!string} name  The name of the editor form.
 * @param {!string} session  The form session identification.
 *
 * @return {!snapwebsites.EditorForm}  The newly created object.
 *
 * @extends {snapwebsites.EditorFormBase}
 * @constructor
 * @struct
 */
snapwebsites.EditorForm = function(editor_base, form_widget, name, session)
{
    var mode;

    snapwebsites.EditorForm.superClass_.constructor.call(this, editor_base, form_widget);

    this.editorWidgets_ = {};
    this.usedTypes_ = {};
    this.session_ = session;
    this.name_ = name;
    mode = form_widget.attr("mode");
    if(mode) // user specified?
    {
        this.mode_ = snapwebsites.castToString(mode, "casting the editor form mode");
    }
    else
    {
        this.mode_ = "default";
    }

    this.readyWidgets_();

    return this;
};


/** \brief EditorForm inherits from EditorFormBase.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorForm, snapwebsites.EditorFormBase);


/** \brief A map of widget types used by this form.
 *
 * This object represents a list of types that this form uses to handle
 * its widgets. Each type defines functions to handle that widget
 * as it needs to be handled and allows for easy extensions, etc.
 *
 * The map uses the name of the type as the index and true or false
 * as the value. true is used when the type is not known to be
 * provided by the system, false once it is known to be defined.
 *
 * @type {!Object}
 * @private
 */
snapwebsites.EditorForm.prototype.usedTypes_; // = {}; -- initialized in the constructor to avoid problems


/** \brief The session identification of this form.
 *
 * This is the session identification and random number for this editor
 * form.
 *
 * @type {!string}
 * @private
 */
snapwebsites.EditorForm.prototype.session_ = "";


/** \brief The name of this form.
 *
 * This is the name of this editor form object. The name is taken from the
 * "form_name" attribute which must exist on the "editor-form" DOM element.
 *
 * @type {!string}
 * @private
 */
snapwebsites.EditorForm.prototype.name_ = "";


/** \brief A jQuery array of widgets found in this form.
 *
 * This parameter is the jQuery array of widgets defined in this
 * form.
 *
 * \bug
 * At this time, dynamically adding or removing widgets is not supported.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorForm.prototype.widgets_ = null;


/** \brief A jQuery array of widgets found in this form.
 *
 * This parameter is the map of EditorWidget objects defined in this
 * form. The key used to define these widget is the name of the widget
 * (the parameter used as the name.) This is quite practical to retrieve
 * widgets which name is known.
 *
 * \bug
 * At this time, dynamically adding or removing widgets is not supported.
 *
 * @type {Object.<snapwebsites.EditorWidget>}
 * @private
 */
snapwebsites.EditorForm.prototype.editorWidgets_ = null;


/** \brief Flag to know whether the widgets got initialized.
 *
 * We initialize the widgets as soon as we know that all the widget
 * types were loaded. At that point we know that the entire form
 * can be properly initialized.
 *
 * Once the flag is true, the form is fully initialized and
 * functional.
 *
 * @type {!boolean}
 * @private
 */
snapwebsites.EditorForm.prototype.widgetInitialized_ = false;


/** \brief The popup dialog object.
 *
 * This member is the save dialog widget. It is an object
 * handling a few DOM widgets representing buttons used to
 * let the user send his work to the server.
 *
 * @type {snapwebsites.EditorSaveDialog}
 * @private
 */
snapwebsites.EditorForm.prototype.saveDialog_ = null;


/** \brief The mode used for this editor form.
 *
 * This parameter is taken from the attribute named "mode" of the
 * div representing the editor form. The mode can be set to one of
 * the following values:
 *
 * \li default -- process the form "as normal", you do not need to define
 *                this value as it is the default.
 * \li save-all -- save all the fields whether or not they were modified.
 *
 * @type {!string}
 * @private
 */
snapwebsites.EditorForm.prototype.mode_ = "";


/** \brief Whether the toolbar is shown immediately on focus.
 *
 * This flag is used to know whether the toolbar should be shown on
 * focus. If so, the value is true (the default). You may turn this
 * value off using the setToolbarAutoVisible() function.
 *
 * @type {!boolean}
 * @private
 */
snapwebsites.EditorForm.prototype.toolbarAutoVisible_ = true;


/** \brief Whether this form was modified.
 *
 * Whenever a form is first loaded and after a successful save,
 * this flag is set to false, meaning that the form was not
 * modified. Each time an event that may have modified the
 * form happens, the wasModified() function is called to check
 * all the fields. If one or more were modified, then the form
 * is marked as modified.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorForm.prototype.modified_ = false;


/** \brief A function to call on a successful save.
 *
 * This reference points to a function that gets called in the event
 * of a successful save. You may define one such function.
 *
 * @type {?function(snapwebsites.EditorForm, snapwebsites.ServerAccessCallbacks.ResultData)}
 * @private
 */
snapwebsites.EditorForm.prototype.saveFunctionOnSuccess_ = null;


/** \brief A function to call on a successful save.
 *
 * This reference points to a function that gets called in the event
 * of a failed save. You may define one such function.
 *
 * @type {?function(snapwebsites.EditorForm, snapwebsites.ServerAccessCallbacks.ResultData)}
 * @private
 */
snapwebsites.EditorForm.prototype.saveFunctionOnError_ = null;


/** \brief An object of the data as sent to the server.
 *
 * The user clicks Save, that may take a little time since the entier
 * block of data is sent to the server (maybe at some point we'll have
 * a JavaScript "diff" function...). While that happens, some pages do
 * not block the user who can continue to edit his work. When that
 * happens the behavior of the editor changes slightly. To know whether
 * such a change happens, the data is saved in this object for the time
 * it takes to process the save command.
 *
 * @type {Object}
 * @private
 */
snapwebsites.EditorForm.prototype.savedData_ = null;


/** \brief A server access object.
 *
 * Whenever the user wants to save his data, a ServerAccess object
 * is required so we create one and keep its reference here. It
 * will be reused on each subsequent save (instead of a new one
 * created each time.)
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.EditorForm.prototype.serverAccess_ = null;


/** \brief Retrieve the editor form name.
 *
 * This function returns the name of the editor form.
 *
 * @return {string} The editor form name.
 */
snapwebsites.EditorForm.prototype.getName = function()
{
    return this.name_;
};


/** \brief Retrieve a widget reference.
 *
 * This function returns the EditorWidget using the widget's name.
 *
 * @param {string} name  The name of the widget to retrieve.
 *
 * @return {snapwebsites.EditorWidget} A reference to the editor widget.
 */
snapwebsites.EditorForm.prototype.getWidgetByName = function(name)
{
    return this.editorWidgets_[name];
};


/** \brief Retrieve the session of this EditorForm object.
 *
 * This function returns the session one can use to communicate with the
 * server about this form.
 *
 * @return {string} A session string of the editor widget.
 */
snapwebsites.EditorForm.prototype.getSession = function()
{
    return this.session_;
};


/** \brief Return whether the toolbar should automatically be shown.
 *
 * When clicking in a widget, a form can automatically show the
 * toolbar corresponding to that widget. This is true by default.
 * The toolbar can be shown / hidden using the Ctlr-T key as well.
 *
 * \todo
 * We'll also want to add a close button at some point
 *
 * @return {boolean} true if the toolbar should be shown on focus.
 */
snapwebsites.EditorForm.prototype.getToolbarAutoVisible = function()
{
    return this.toolbarAutoVisible_;
};


/** \brief Change whether the toolbar should automatically be shown.
 *
 * Whenever a widget gets the focus we can automatically have the
 * toolbar popup. By default, this is true. It can be changed to
 * false using this function.
 *
 * @param {boolean} toolbar_auto_visible  Whether the toolbar should be
 *                                        shown on widget focus.
 */
snapwebsites.EditorForm.prototype.setToolbarAutoVisible = function(toolbar_auto_visible)
{
    this.toolbarAutoVisible_ = toolbar_auto_visible;
};


/** \brief Massage the title to make it a URI.
 *
 * This function transforms the characters in \p title so it can be
 * used as a segment of the URI of this page. This is quite important
 * since we use the URI to save the page.
 *
 * @param {string} title  The title to tweak.
 *
 * @return {string}  The tweaked title. It may be an empty string.
 */
snapwebsites.EditorForm.titleToURI = function(title) // static
{
    // force all lower case
    title = title.toLowerCase();
    // replace spaces with dashes
    title = title.replace(/ +/g, "-");
    // remove all characters other than letters and digits
    title = title.replace(/[^-a-z0-9_]+/g, "");
    // remove duplicate dashes
    title = title.replace(/--+/g, "-");
    // remove dashes at the start & end
    title = title.replace(/^-+/, "");
    title = title.replace(/-+$/, "");

    return title;
};


/** \brief A function to call on a successful save.
 *
 * If defined, this function is called when the Save function
 * returned a successful result and is not asked to redirect the
 * user.
 *
 * This is particularly useful if you used a form in a popup that
 * should be closed on success.
 *
 * @param {function(snapwebsites.EditorForm, snapwebsites.ServerAccessCallbacks.ResultData)} f  A function to call on success.
 */
snapwebsites.EditorForm.prototype.setSaveFunctionOnSuccess = function(f)
{
    this.saveFunctionOnSuccess_ = f;
};


/** \brief A function to call on an error while saving.
 *
 * If defined, this function is called when the Save function
 * returned with a failed result.
 *
 * This is particularly useful if your C++ function generated some data
 * in the result structure and you want to make use of it.
 *
 * @param {function(snapwebsites.EditorForm, snapwebsites.ServerAccessCallbacks.ResultData)} f  A function to call on failure.
 */
snapwebsites.EditorForm.prototype.setSaveFunctionOnError = function(f)
{
    this.saveFunctionOnError_ = f;
};


/** \brief Function called on success.
 *
 * This function is called if the remote access was successful. The
 * result object includes a reference to the XML document found in the
 * data sent back from the server.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.EditorForm.prototype.serverAccessSuccess = function(result) // virtual
{
    var key,                    // loop index
        modified = false;       // whether some data was modified while saving

    // success! so it was saved and now that's the new original value
    // and next "Save" doesn't do anything
    for(key in this.editorWidgets_)
    {
        if(this.editorWidgets_.hasOwnProperty(key))
        {
            if(this.savedData_[key])
            {
                if(this.editorWidgets_[key].saved(this.savedData_[key]))
                {
                    modified = true;
                }
            }
        }
    }

    // if not modified while processing the POST, hide the save buttons
    if(!modified)
    {
        this.getSaveDialog().close();
    }

    // in case the manager of the form wants to know that a save was
    // successful (but only if we're not going to redirect the user)
    if(!result.will_redirect && this.saveFunctionOnSuccess_)
    {
        this.saveFunctionOnSuccess_(this, result);
    }
};


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
snapwebsites.EditorForm.prototype.serverAccessError = function(result) // virtual
{
    // TODO: do we have to anything with this error message? We
    //       should always have messages in the AJAX, but we may reach
    //       this function for other reasons... messages are displayed
    //       by the serverAccess directly

    // in case the manager of the form wants to know that an error
    // occured and get the corresponding result information
    if(this.saveFunctionOnError_)
    {
        this.saveFunctionOnError_(this, result);
    }
};


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
snapwebsites.EditorForm.prototype.serverAccessComplete = function(result) // virtual
{
    // we do not need that data anymore, release it
    this.savedData_ = null;

    if(!result.will_redirect && result.messages && result.messages.length > 0)
    {
        this.setSaving(false, false);
        snapwebsites.OutputInstance.displayMessages(result.messages);
    }
    else
    {
        this.setSaving(false, result.will_redirect);
    }
};


/** \brief Save the data.
 *
 * This function checks whether the data changed or not. If it was
 * modified, or the form was setup to save all the widgets content
 * each time, then the function builds an object and sends the
 * data to the server using AJAX.
 *
 * @param {!string} mode  The mode used to save the data.
 * @param {Object=} opt_options  Additional options to be added to the query string.
 */
snapwebsites.EditorForm.prototype.saveData = function(mode, opt_options)
{
    var key,                                    // loop index
        w,                                      // widget being managed
        obj = {},                               // object to send via AJAX
        save_all = this.mode_ === "save-all";   // whether all fields are sent to the server

    // are we already saving? if so, generate an error
    if(this.isSaving())
    {
        // TODO: translation support
        alert("You already clicked one of these buttons. Please wait until the save is over.");
        return;
    }

    // mark the form as saving, it may use CSS to show the new status
    this.setSaving(true, false);

    this.savedData_ = {};
    for(key in this.editorWidgets_)
    {
        if(this.editorWidgets_.hasOwnProperty(key))
        {
            // note that widgets marked as "immediate-save" are ignored
            // here because those were saved as soon as they were modified
            // so there is no need for us to save them again here (plus
            // in some cases it would be impossible like for file upload)
            w = this.editorWidgets_[key];
            if((save_all || w.wasModified(true))
            && !w.getWidget().hasClass("immediate-save"))
            {
                this.savedData_[key] = w.saving();
                obj[key] = this.savedData_[key].result;
            }
        }
    }

    // this test is not 100% correct for the Publish or Create Branch
    // buttons...
    if(!jQuery.isEmptyObject(obj))
    {
        obj._editor_save_mode = mode;
        obj._editor_session = this.session_;
        if(this.editorWidgets_.hasOwnProperty("title"))
        {
            obj._editor_uri = snapwebsites.EditorForm.titleToURI(this.editorWidgets_.title.saving().result);
        }
        if(!this.serverAccess_)
        {
            this.serverAccess_ = new snapwebsites.ServerAccess(this);
        }
        this.serverAccess_.setURI(snapwebsites.castToString(jQuery("link[rel='canonical']").attr("href"), "casting href of the canonical link to a string in snapwebsites.EditorForm.saveData()"),
                                  opt_options);
        this.serverAccess_.setData(obj);
        this.serverAccess_.send();
    }
    else
    {
        this.setSaving(false, false);

        // the serverAccess callback would otherwise call the close
        this.getSaveDialog().close();
    }
};


/** \brief Check whether this form is currently saving data.
 *
 * To avoid problems we prevent the editor from saving more than once
 * simultaneously.
 *
 * @return {boolean}  true if the form is currently saving data.
 * @final
 */
snapwebsites.EditorForm.prototype.isSaving = function() // virtual
{
    return this.getFormWidget().is(".editor-saving");
};


/** \brief Set the current saving state of the form.
 *
 * This function set the saving state to "true" or "false". This is
 * marked using the "editor-saving" class in the editor form widget.
 * That can be used to show that the form is being saved by changing
 * a color or a border.
 *
 * @param {boolean} new_status  The saving status of this editor form.
 * @param {boolean} will_redirect  Whether we're going to be redirected just
 *                                 after this call (keep the darken page if
 *                                 true!)
 */
snapwebsites.EditorForm.prototype.setSaving = function(new_status, will_redirect)
{
    // TBD: use the will_redirect flag to know what else to do?
    //      (i.e. if will_redirect is true, we probably should not
    //      do anything here, what do you think?)
    //
    this.getFormWidget().toggleClass("editor-saving", new_status);
    this.saveDialog_.setStatus(!new_status);

    // TODO: add a condition coming from the DOM (i.e. we don't want
    //       to gray out the screen if the user is expected to be
    //       able to continue editing while saving)
    //       the class is nearly there (see header trying to assign body
    //       attributes), we will then need to test it here
    //       WARNING: this needs to be moved to the editor-form object
    //                instead of the body!
    if(!will_redirect)
    {
        snapwebsites.PopupInstance.darkenPage(new_status ? 150 : -150, new_status);
    }
};


/** \brief Let the form know that one of its widgets changed.
 *
 * This signal implementation is called whenever the form detects
 * that one of its widgets changed.
 *
 * In most cases it will show the "Save Dialog" although it may just
 * want to ignore the fact. (i.e. some forms do not care about changes
 * as their "save" button is always shown, for example, or it may
 * reacted on this event directly and not give the user a choice to
 * save the changes.)
 *
 * \todo
 * Support a way to tell others to react to this change.
 */
snapwebsites.EditorForm.prototype.changed = function()
{
    // tell others that something changed in the editor form
    var e = jQuery.Event("formchange",
        {
            form: this
        });
    this.getFormWidget().trigger(e);

    if(!this.getFormWidget().is(".no-save"))
    {
        this.getSaveDialog().open();
    }
};


/** \brief Retrieve the save dialog.
 *
 * This functio retrieves a reference to the save dialog. It is a
 * function because the save dialog doesn't get created until
 * requested.
 *
 * \note
 * Unfortunately, if you want to define your own save buttons,
 * then you'll be creating the save dialog up front.
 *
 * @return {snapwebsites.EditorSaveDialog}  The save dialog reference.
 */
snapwebsites.EditorForm.prototype.getSaveDialog = function()
{
    if(!this.saveDialog_)
    {
        this.saveDialog_ = new snapwebsites.EditorSaveDialog(this);
    }
    return this.saveDialog_;
};


/** \brief Ready the widget for attachment.
 *
 * This function goes through the widgets and prepare them to be
 * attached to their respective handlers. Because we want to have
 * an extensible editor, this means we need to allow for "callbacks"
 * to be defined on the extension and not on some internal object.
 * This works by getting the list of widgets, retrieving their types,
 * and looking for a class of that name. If the class does not exist
 * (yet) then the form is not yet complete and the loader goes on.
 * Once all the types are properly defined, the form is ready and
 * the user can start editing.
 *
 * \note
 * The widgets themselves are marked with ".editor-content". The
 * editor always encapsulate those inside a div marked with
 * ".snap-editor". So there is a one to one correlation between
 * both of those.
 *
 * @private
 */
snapwebsites.EditorForm.prototype.readyWidgets_ = function()
{
    var that = this;

    // retrieve the widgets defined in that form
    this.widgets_ = this.getFormWidget().find(".snap-editor");

    // retrieve the field types for all the widgets
    this.widgets_.each(function(idx, w)
        {
            var type = jQuery(w).attr("field_type");
            that.usedTypes_[type] = true;
        });

    // check whether all the types are available
    // if so then the function finishes the initialization of the form
    this.newTypeRegistered();

    // make labels focus the corresponding editable box
    this.getFormWidget().find("label[for!='']").click(function(e)
        {
            if(!(jQuery(e.target).is("a")))
            {
                // the default may recapture the focus, so avoid it!
                e.preventDefault();
                e.stopPropagation();

                jQuery("div[name='" + jQuery(this).attr("for") + "']").focus();
            }
        });
};


/** \brief Check whether all types were registered and if so initialize everything.
 *
 * The function checks whether all the types necessary to initialize
 * the widgets are available. If so, then all the form widgets get
 * initialized.
 *
 * \todo
 * Find a way to detect that a form initialization was never completed
 * because the system is missing a widget type.
 */
snapwebsites.EditorForm.prototype.newTypeRegistered = function()
{
    var that = this,
        key;

    if(this.widgetInitialized_)
    {
        return;
    }

    // check whether all the types are available
    for(key in this.usedTypes_)
    {
        if(this.usedTypes_.hasOwnProperty(key)
        && this.usedTypes_[key])
        {
            if(!this.editorBase_.hasWidgetType(key))
            {
                // some types are missing, we're not ready
                // (this happens to forms using external widgets)
                return;
            }

            // this one was defined
            this.usedTypes_[key] = false;
        }
    }

    // if we reach here, all the types are available so we can
    // properly initialize the form now
    this.widgets_.each(function(idx, w)
        {
            var widget_content = jQuery(w),
                name = widget_content.attr("field_name");

            that.editorWidgets_[name] = new snapwebsites.EditorWidget(that.editorBase_, that, widget_content);
        }
    ).each(function(idx, w)
        {
            var widget_content = jQuery(w),
                name = widget_content.attr("field_name"),
                widget = that.editorWidgets_[name];

            widget.getWidgetType().initializeWidget(widget);

            // auto-focus the widget if so required
            if(widget_content.is(".auto-focus"))
            {
                widget.focus();
            }
        }
    ).each(function(idx, w)
        {
            var widget_content = jQuery(w),
                name = widget_content.attr("field_name"),
                widget = that.editorWidgets_[name];

            // reset the originalData_ field
            // TBD: we may want (need) to move this in another loop instead
            widget.discard();
        });

    this.widgetInitialized_ = true;

    // composite widgets may change their children widgets in a way that
    // marks the editor as modified, clear the flag here so we do not get
    // a spurious popup
    this.modified_ = false;
};


/** \brief This function checks whether the form is to be saved.
 *
 * You may mark a form as a "no-save" form. In that case, the main form
 * \<div\> tag gets a class named "no-save". This function checks for
 * that class.
 *
 * If the class "no-save" is defined on the main form \<div\> tag then
 * this function returns false, otherwise it returns true.
 *
 * This indicates whether the form is expected to be saved. In most cases,
 * a search form or a locator form would be marked as a "no-save" form
 * because entering data and click on a button or another should not
 * generate a save request.
 *
 * \note
 * This function is called on the unload event. If it returns false,
 * then that form is ignored in the test on whether it was modified.
 *
 * @return {boolean} true if the form is to be saved.
 */
snapwebsites.EditorForm.prototype.toBeSaved = function()
{
    return !this.getFormWidget().hasClass("no-save");
};


/** \brief This function checks whether the form was modified.
 *
 * This function checks whether the form was modified. First it
 * checks whether the form is to be saved. If not then the function
 * just returns false.
 *
 * @param {boolean} recheck  Whether to check each widget for modification
 *                           instead of just the modified flags.
 *
 * @return {boolean} true if the form was modified.
 */
snapwebsites.EditorForm.prototype.wasModified = function(recheck)
{
    var key;                            // loop index

    if(!recheck && this.modified_)
    {
        return true;
    }

    for(key in this.editorWidgets_)
    {
        if(this.editorWidgets_.hasOwnProperty(key)
        && this.editorWidgets_[key].wasModified(recheck))
        {
            this.modified_ = true;
            return true;
        }
    }

    return false;
};


/** \brief Close this form early.
 *
 * This function handles the case when a form is to be closed before the
 * window as a whole is to be closed. You can choose whether the form
 * is to react with a confirmation dialog or not.
 *
 * This is particularly useful in the beforeClose() callback of a popup
 * when that popup is used with a form.
 *
 * The 'message' parameter defines a message to show to the client in case
 * there were modifications. If the message is undefined, then the system
 * just saves the changes and returns.
 *
 * If earlyClose() is to automatically call saveData() ('auto_save' or one
 * of the buttons named 'save' gets clicked,) then the save_mode is used
 * to call saveData(). In most cases it should be set to SAVE_MODE_PUBLISH
 * or SAVE_MODE_SAVE.
 *
 * @param {{save_mode: string,
 *          title: string,
 *          message: string,
 *          buttons: Object.<{name: string, label: string}>,
 *          callback: function(string)}} early_close  The message object with
 *    the message itself, buttons, and a callback function.
 */
snapwebsites.EditorForm.prototype.earlyClose = function(early_close)
{
    var org_callback = early_close.callback,
        that = this;

    if(this.toBeSaved()
    && this.wasModified(true))
    {
        if(!early_close.message)
        {
            this.saveData(early_close.save_mode);
        }
        else
        {
            // our specialized callback for the message box
            early_close.callback = function(name)
                {
                    var key;

                    if(name == "save")
                    {
                        if(early_close.save)
                        {
                            early_close.save(that);
                        }
                        else
                        {
                            that.saveData(early_close.save_mode);
                        }
                    }
                    else if(name == "discard")
                    {
                        // copy the data to make it look like it was saved
                        for(key in that.editorWidgets_)
                        {
                            if(that.editorWidgets_.hasOwnProperty(key))
                            {
                                that.editorWidgets_[key].discard();
                            }
                        }
                        that.getSaveDialog().close();
                    }
                    if(org_callback)
                    {
                        // call the user callback too
                        org_callback(name);
                    }
                };
            snapwebsites.PopupInstance.messageBox(early_close);
        }
    }
    else
    {
        // nothing needs to be saved
        if(early_close.callback)
        {
            early_close.callback("no-save");
        }
    }
};



/** \brief Snap Editor constructor.
 *
 * \note
 * The Snap! Editor is a singleton and should never be created by you. It
 * gets initialized automatically when this editor.js file gets included.
 *
 * \code
 * class Editor extends EditorBase
 * {
 * public:
 *      function Editor() : Editor;
 *      virtual function getToolbar() : EditorToolbar;
 *      virtual function checkModified();
 *      function getActiveEditorForm() : EditorForm;
 *      virtual function getLinkDialog() : LinkDialog;
 *      virtual function registerWidgetType(widget_type: EditorWidgetType);
 *
 * private:
 *      function attachToForms_();
 *      function initUnload_();
 *      function unload_() : string;
 *
 *      toolbar_: EditorToolbar;
 *      editorSession_: Array; // of strings
 *      editorForm_: Object; // map of editor forms
 *      unloadCalled_: boolean;
 *      linkDialog_: EditorLinkDialog;
 * };
 * \endcode
 *
 * \return The newly created object.
 *
 * @constructor
 * @extends {snapwebsites.EditorBase}
 * @struct
 */
snapwebsites.Editor = function()
{
    snapwebsites.Editor.superClass_.constructor.call(this);
    this.editorForms_ = {};
    this.editorFormsByName_ = {};
    this.initUnload_();
    this.attachToForms_();

    return this;
};


/** \brief Editor inherits from EditorBase.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.Editor, snapwebsites.EditorBase);


/** \brief The Editor instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * \@type {snapwebsites.Editor}
 */
snapwebsites.EditorInstance = null; // static


/** \brief The toolbar object.
 *
 * This variable represents the toolbar used by the editor.
 *
 * Note that this is the toolbar object, not the DOM. The DOM is
 * defined within the toolbar object and is considered private.
 *
 * @type {snapwebsites.EditorToolbar}
 * @private
 */
snapwebsites.Editor.prototype.toolbar_ = null;


/** \brief List of EditorForm objects.
 *
 * This variable member holds the map of EditorForm objects indexed by
 * their name.
 *
 * @type {Object.<snapwebsites.EditorForm>}
 * @private
 */
snapwebsites.Editor.prototype.editorFormsByName_; // = {} -- initialized in the constructor to avoid problems


/** \brief List of EditorForm objects.
 *
 * This variable member holds the map of EditorForm objects indexed by
 * their sessions number (although those numbers are managed as strings).
 *
 * @type {Object.<snapwebsites.EditorForm>}
 * @private
 */
snapwebsites.Editor.prototype.editorForms_; // = {}; -- initialized in the constructor to avoid problems


/** \brief Whether unload is being processed.
 *
 * There is a "bug" in Firefox and derivative browsers (a.k.a. SeaMonkey)
 * where the browser calls the unload function twice. According to the
 * developers, "it is normal". To avoid that "bug" we use this flag and
 * a timer. If this flag is true, we avoid showing a second prompt to the
 * users.
 *
 * @type {!boolean}
 * @private
 */
snapwebsites.Editor.prototype.unloadCalled_ = false;


/** \brief The link dialog.
 *
 * The get_link_dialog() function creates this link dialog the first
 * time it is called.
 *
 * @type {snapwebsites.EditorLinkDialog}
 * @private
 */
snapwebsites.Editor.prototype.linkDialog_ = null;


/** \brief Attach to the EditorForms defined in the DOM.
 *
 * This function attaches the editor to the existing editor forms
 * as defined in the DOM. Editor forms are detected by the fact
 * that a \<div\> tag has class ".editor-form".
 *
 * The function is expected to be called only once.
 *
 * @private
 */
snapwebsites.Editor.prototype.attachToForms_ = function()
{
    var that = this;

    // retrieve the list of forms using their sessions
    jQuery(".editor-form")
        .each(function(){
            // TBD: We may be able to drop the session map.
            var that_element = jQuery(this),
                session = snapwebsites.castToString(that_element.attr("session"), "editor form session attribute"),
                name = snapwebsites.castToString(that_element.attr("form_name"), "editor form name attribute");

            that.editorForms_[session] = new snapwebsites.EditorForm(that, that_element, name, session);
            that.editorFormsByName_[name] = that.editorForms_[session];
        });
};


/** \brief Retrieve a reference to one of the forms by name.
 *
 * This function takes the name of a form and returns a corresponding
 * reference to the EditorForm object.
 *
 * If the form does not exist, null is returned.
 *
 * @param {string} form_name  The name of the form.
 *
 * @return {snapwebsites.EditorForm}  The corresponding EditorForm object.
 */
snapwebsites.Editor.prototype.getFormByName = function(form_name)
{
    return this.editorFormsByName_[form_name];
};


/** \brief Capture the unload event.
 *
 * This function adds the necessary code to handle the unload event.
 * This is used to make sure that users will save their data before
 * they actually close their browser window or tab.
 *
 * @private
 */
snapwebsites.Editor.prototype.initUnload_ = function()
{
    var that = this;
    jQuery(window).bind("beforeunload", function()
        {
            return that.unload_();
        });
};


/** \brief Handle the unload event.
 *
 * This function is called whenever the user is about to close their
 * browser window or tab. The function checks whether anything needs
 * to be saved. If so, then make sure to ask the user whether he
 * wants to save his changes before closing the window.
 *
 * @return {null|string|undefined}  A message saying that something
 *          was not saved if it applies, null otherwise.
 *
 * @private
 */
snapwebsites.Editor.prototype.unload_ = function()
{
    var key,                // loop index
        that = this;        // this pointer in the closure function

    if(!this.unloadCalled_)
    {
        for(key in this.editorForms_)
        {
            if(this.editorForms_.hasOwnProperty(key))
            {
                if(this.editorForms_[key].toBeSaved()
                && this.editorForms_[key].wasModified(true))
                {
                    // add this flag and timeout to avoid a double
                    // "are you sure?" under Firefox browsers
                    this.unloadCalled_ = true;

                    // we create a function in a for() loop because
                    // right after that statement we return
                    setTimeout(function(){
                            that.unloadCalled_ = false;
                        }, 20);

                    // TODO: translation
                    //       (although it doesn't show up in FireFox based
                    //       browsers many others do show this message)
                    return "You made changes to this page! Click Cancel to avoid closing the window and Save your changes first.";
                }
            }
        }
    }

    return;
};


/** \brief Retrieve the toolbar object.
 *
 * This function returns a reference to the toolbar object.
 *
 * @return {snapwebsites.EditorToolbar} The toolbar object.
 * @override
 */
snapwebsites.Editor.prototype.getToolbar = function() // virtual
{
    if(!this.toolbar_)
    {
        this.toolbar_ = new snapwebsites.EditorToolbar(this);
    }
    return this.toolbar_;
};


/** \brief Check whether a field was modified.
 *
 * When something may have changed (a character may have been inserted
 * or deleted, or a text replaced) then you are expected to call this
 * function in order to see whether something was indeed modified.
 *
 * When the process detects that something was modified, it calls the
 * necessary functions to open the Save Dialog.
 *
 * As a side effect it also lets the toolbar know so if it needs to be
 * moved, it happens.
 *
 * @override
 */
snapwebsites.Editor.prototype.checkModified = function() // virtual
{
    var active_element,
        active_form = this.getActiveEditorForm(),
        widget_name,
        widget;

    // checkModified only applies to the active element so make sure
    // there is one when called
    if(active_form)
    {
        // allow the toolbar to adjust itself (move to a new location)
        if(this.toolbar_)
        {
            this.toolbar_.checkPosition();
        }

        // got the form in which this element is defined,
        // now call the wasModified() function on it
        if(active_form.wasModified(false))
        {
            active_form.changed();
        }

        // replace nothingness by "background" value
        active_element = this.getActiveElement();
        widget_name = snapwebsites.castToString(active_element.parent().attr("field_name"), "Editor active element \"field_name\" attribute");
        widget = active_form.getWidgetByName(widget_name);
        widget.checkForBackgroundValue();
    }
};


/** \brief Retrieve the active editor form.
 *
 * This function gets the editor form that includes the currently
 * active element.
 *
 * @return {snapwebsites.EditorForm}  The active editor form or null.
 */
snapwebsites.Editor.prototype.getActiveEditorForm = function()
{
    var active_element = this.getActiveElement(),
        editor_element,
        editor_form = null,
        session;

    if(active_element)
    {
        editor_element = active_element.parents(".editor-form");
        if(editor_element)
        {
            session = editor_element.attr("session");
            editor_form = this.editorForms_[session];
        }
//#ifdef DEBUG
        if(!editor_form)
        {
            // should not happen or it means we whacked the session element
            throw new Error("There is an active element but no corresponding editor form.");
        }
//#endif
    }

    return editor_form;
};


/** \brief Retrieve the link dialog.
 *
 * This function creates an instance of the link dialog and returns it.
 * If the function gets called more than once, then the same reference
 * is returned.
 *
 * The function takes settings defined in an object, the following are
 * the supported names:
 *
 * \li close -- The function called whenever the user clicks the OK button.
 *
 * \param[in] settings  An object defining settings.
 *
 * @return {snapwebsites.EditorLinkDialog} The link dialog reference.
 * @override
 */
snapwebsites.Editor.prototype.getLinkDialog = function()
{
    if(!this.linkDialog_)
    {
        this.linkDialog_ = new snapwebsites.EditorLinkDialog(this);
    }
    return this.linkDialog_;
};


/** \brief Capture the registration of widget types.
 *
 * THis function captures the registration of widget types so it
 * can produce an event on the editor forms which may need additional
 * types whenever they get created to be able to initialize all their
 * widgets.
 *
 * @param {snapwebsites.EditorWidgetType} widget_type  The widget type to register.
 */
snapwebsites.Editor.prototype.registerWidgetType = function(widget_type) // virtual
{
    // first make sure to call the super class function
    snapwebsites.Editor.superClass_.registerWidgetType.call(this, widget_type);

    var key;

    // let all the forms know we have a new type
    //
    // XXX -- this is not really the most efficient, but at this point it
    //        will do
    for(key in this.editorForms_)
    {
        if(this.editorForms_.hasOwnProperty(key))
        {
            this.editorForms_[key].newTypeRegistered();
        }
    }
};



/** \brief Snap EditorWidgetType constructor.
 *
 * The editor works with widgets that are based on this class. Each widget
 * is given a type such as "line-edit".
 *
 * To make it fully dynamic, we define a base class here and let other
 * programmers add new widget types in their own .js files by extending
 * this class.
 *
 * Classes must be registered with the EditorBase class function:
 *
 * \code
 *    snapwebsites.EditorInstance.registerWidgetType(your_widget_type);
 * \endcode
 *
 * This base class already implements a few things that are common to
 * all widgets.
 *
 * \code
 *  class EditorWidgetType extends EditorWidgetTypeBase
 *  {
 *  public:
 *      function EditorWidgetType();
 *      virtual function preInitializeWidget(widget: Object) : void;
 *      virtual function initializeWidget(widget: Object) : void;
 *      virtual function setupEditButton(editor_widget: snapwebsites.EditorWidget) : void;
 *      abstract function droppedImage(e: ProgressEvent, img: Image) : void;
 *      abstract function droppedAttachment(e: ProgressEvent) : void;
 *
 *  private:
 *      function droppedImageConvert_(e: ProgressEvent) : void;
 *      function droppedFile_(e: ProgressEvent) : void;
 *  };
 * \endcode
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeBase}
 * @struct
 */
snapwebsites.EditorWidgetType = function()
{
    snapwebsites.EditorWidgetType.superClass_.constructor.call(this);

    // TBD
    // Maybe at some point we'd want to create yet another layer
    // so we can have an auto-register, but I'm not totally sure
    // that would work...
    //snapwebsites.EditorBase.registerWidgetType(this);

    return this;
};


/** \brief EditorWidgetType inherits from EditorWidgetTypeBase.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetType, snapwebsites.EditorWidgetTypeBase);


/*jslint unparam: true */
/** \brief Initialize a widget of this type.
 *
 * This function is called from the EditorWidget constructor. The core
 * variables will all be initialized when called, but there may be other
 * parts that are not. This function should be used to further some
 * initialization of variable members that does not happen in core.
 *
 * \warning
 * This function is called right after a widget was created and all the
 * widgets may not yet be created when called. In most cases, you want
 * to overload the initializeWidget() function instead.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetType.prototype.preInitializeWidget = function(widget) // virtual
{
};
/*jslint unparam: false */


/** \brief Initialize a widget of this type.
 *
 * Setup various events to handle changes to the widget content and
 * focus.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetType.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        //w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent();

    this.setupEditButton(editor_widget); // allow overrides to an empty function

    // widget was possibly modified, so make sure we stay on top
    c.on("keyup bind cut copy paste", function()
        {
            editor_widget.getEditorBase().checkModified();
        });

    // widget gets the focus, make it the active widget
    c.focus(function()
        {
            editor_widget.getEditorBase().setActiveElement(c);

            if(!jQuery(this).is(".no-toolbar")
            && !jQuery(this).parent().is(".read-only"))
            {
                if(editor_widget.getEditorForm().getToolbarAutoVisible())
                {
                    editor_widget.getEditorBase().getToolbar().toggleToolbar(true);
                }
            }
        });

    // widget loses the focus, lose the toolbar in a few ms...
    //
    // (note: the activeElement_ parameter is NOT reset and this is important
    //        because the toolbar may use it to restore it once the command
    //        is processed.)
    //
    c.blur(function()
        {
            // don't blur the toolbar immediately because if the user just
            // clicked on it, it would break it
            editor_widget.getEditorBase().getToolbar().startToolbarHide();
        });

    // a key was pressed in the focused widget
    c.keydown(function(e)
        {
//console.log("ctrl "+(e.shiftKey?"+ shift ":"")+"= "+e.which+", idx = "+(snapwebsites.EditorInstance.keys_[e.which + (e.shiftKey ? 0x10000 : 0)]));
            if(editor_widget.getEditorBase().getToolbar().keydown(e))
            {
                // toolbar used that key stroke
                e.preventDefault();
                e.stopPropagation();
            }
        });

    // the user just moved over a widget while dragging something
    c.on("dragenter",function(e)
        {
            e.preventDefault();
            e.stopPropagation();

            // allows your CSS to change some things when areas are
            // being dragged over
            jQuery(this).parent().addClass("dragging-over");
        });

    // the user is dragging something over a widget
    c.on("dragover",function(e)
        {
            // TBD this is said to make things work better in some browsers...
            e.preventDefault();
            e.stopPropagation();
        });

    // the user just moved out a widget while dragging something
    c.on("dragleave",function(e)
        {
            e.preventDefault();
            e.stopPropagation();

            // remove the class when the mouse leaves
            jQuery(this).parent().removeClass("dragging-over");
        });

    // the user actually dropped a file on this widget
    //
    // Note: we handle the drop at this level, other widget types
    //       should only override the the droppedImage()
    //       and droppedAttachment() functions instead
    //
    c.on("drop",function(e)
        {
            var i,                      // loop index
                r,                      // file reader object
                accept_images,          // boolean, true if element accepts images
                accept_files,           // boolean, true if element accepts attachments
                that_element = c,       // this element as a jQuery object
                file_loaded;            // finalizing function

            //
            // TODO:
            // At this point this code breaks the normal behavior that
            // properly places the image where the user wants it; I'm
            // not too sure how we can follow up on the "go ahead and
            // do a normal instead" without propagating the event, but
            // I'll just ask on StackOverflow for now...
            //
            // http://stackoverflow.com/questions/22318243/how-to-apply-the-default-image-drop-behavior-after-testing-that-image-is-valid
            //
            // That said, I did not get any answer but thinking about
            // it, it seems pretty easy to me: the answer is to use
            // the jQuery().trigger() command which processes the event
            // as if nothing had happened. Then we just need to ignore
            // that event if it calls this "drop" event handler again.
            //

            // remove the dragging-over class on a drop because we
            // do not always get the dragleave event in that case
            that_element.parent().removeClass("dragging-over");

            // always prevent the default dropping mechanism
            // we handle the file manually all the way
            e.preventDefault();
            e.stopPropagation();

            // anything transferred on widget that accepts files?
            if(e.originalEvent.dataTransfer
            && e.originalEvent.dataTransfer.files.length)
            {
                accept_images = that_element.hasClass("image");
                accept_files = that_element.hasClass("attachment");
                if(accept_images || accept_files)
                {
                    // TODO: add a test, in case length > 1 and the destination
                    //       widget expects exactly 1 file, then generate an
                    //       error because we cannot know which file the user
                    //       really intended to drop (maybe we could offer a
                    //       selection, assuming we do not lose the necessary
                    //       info...) We could also just have a max. # of
                    //       possible drops and if `length > max` then err.
                    //
                    file_loaded = function(e)
                        {
                            that.droppedFile_(e);
                        };
                    for(i = 0; i < e.originalEvent.dataTransfer.files.length; ++i)
                    {
                        // For images we do not really care about that info, for uploads we will
                        // use it so I keep that here for now to not have to re-research it...
                        //console.log("  filename = [" + e.originalEvent.dataTransfer.files[i].name
                        //          + "] + size = " + e.originalEvent.dataTransfer.files[i].size
                        //          + " + type = " + e.originalEvent.dataTransfer.files[i].type
                        //          + "\n");

                        // read the image so we can make sure it is indeed an
                        // image and ignore any other type of files
                        r = new FileReader();
                        r.snapEditorWidget = editor_widget;
                        r.snapEditorFile = e.originalEvent.dataTransfer.files[i];
                        r.snapEditorIndex = i;
                        r.snapEditorAcceptImages = accept_images;
                        r.snapEditorAcceptFiles = accept_files;
                        r.onload = file_loaded;

                        //
                        // TBD: right now we only check the first few bytes
                        //      but we may want to increase that size later
                        //      to allow for JPEG that have the width and
                        //      height defined (much) further in the stream
                        //      (at times at the end!?)
                        //
                        r.readAsArrayBuffer(r.snapEditorFile.slice(0, 64));
                    }
                }
            }

            return false;
        });
};


// noempty: false -- not support in current version of jslint
/*jslint unparam: true */
/** \brief Add an "Edit" button to this widget.
 *
 * In general, forms will make use of the "immediate" class which
 * means that no "Edit" button is required. However, a standard page
 * will look pretty much normal except for that little popup that
 * appears when you hover an editable area. Clicking on that "Edit"
 * button in the popup enables the editing of the area.
 *
 * The main reason for doing this is because other functionalities
 * break when editing an area.
 *
 * Widgets that don't offer a "contenteditable" area (i.e.
 * checkmarks, radios, etc.) override this functions with a do
 * nothing function.
 *
 * \note
 * Although we generally expect the getEditButton() to be overriden, this
 * function can also be overridden. This gives you the ability to, for
 * example, generate multiple edit field (i.e. short, normal, and long
 * titles when editing the title of a page.)
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget being
 *                                                   initialized.
 */
snapwebsites.EditorWidgetType.prototype.setupEditButton = function(editor_widget) // virtual
{
};
/*jslint unparam: false */ // noempty: true -- not support in current version


/** \brief Got the content of a dropped file.
 *
 * This function analyze the dropped file content. If recognized then we
 * proceed with the onimagedrop or onattachmentdrop as required.
 *
 * @param {ProgressEvent} e  The event.
 *
 * @private
 * @final
 */
snapwebsites.EditorWidgetType.prototype.droppedFile_ = function(e)
{
    var that = this,
        r,
        a,
        blob;

    e.target.snapEditorMIME = snapwebsites.OutputInstance.bufferToMIME(e.target.result);
    if(e.target.snapEditorAcceptImages && e.target.snapEditorMIME.substr(0, 6) === "image/")
    {
        // Dropped an Image managed as such

        // It is an image, now convert the data to URI encoding
        // (i.e. base64 encoding) before saving the result in the
        // target element
        r = new FileReader();
        r.snapEditorWidget = e.target.snapEditorWidget;
        r.snapEditorFile = e.target.snapEditorFile;
        r.snapEditorIndex = e.target.snapEditorIndex;
        r.snapEditorAcceptImages = e.target.snapEditorAcceptImages;
        r.snapEditorAcceptFiles = e.target.snapEditorAcceptFiles;
        r.snapEditorMIME = e.target.snapEditorMIME;
        r.onload = function(e)
            {
                that.droppedImageConvert_(e);
            };

        a = [];
        a.push(e.target.snapEditorFile);
        blob = new Blob(a, { type: e.target.snapEditorMIME });
        r.readAsDataURL(blob);
    }
    else if(e.target.snapEditorAcceptFiles && e.target.snapEditorMIME)
    {
        // Dropped a file managed as an attachment
        this.droppedAttachment(e);
    }
    else
    {
        // generate an error
        //
        // TODO: we do not yet have code to dynamically generate errors
        //       (we can show messages when created by the server, and
        //       want the same thing with errors, but that's not yet
        //       available...)
        //       -- This is not correct anymore, we now do have a way to
        //          dynamically generate errors!
        //
    }
};


/*jslint eqeq: true */
/** \brief Save the resulting image in the target.
 *
 * This function receives the image as data that can readily be stick
 * in the 'src' attribute of an 'img' tag (i.e. data:image/fmt;base64=...).
 * It gets loaded in an Image object so we can verify the image width
 * and height before it gets presented to the end user.
 *
 * @param {ProgressEvent} e  The file information.
 * @private
 * @final
 */
snapwebsites.EditorWidgetType.prototype.droppedImageConvert_ = function(e)
{
    var that = this,
        img = new Image();

    // The image parameters (width/height) are only available after the
    // onload() event kicks in
    img.onload = function()
        {
            // keep this function here because it is a full closure (it
            // uses 'img' 'that', and even 'e')

            var sizes,
                limit_width = 0,
                limit_height = 0,
                w,
                h,
                nw,
                nh,
                max_sizes;

            // make sure we do it just once
            img.onload = null;

            w = img.width;
            h = img.height;

            if(e.target.snapEditorWidget.getWidgetContent().attr("min-sizes"))
            {
                sizes = e.target.snapEditorWidget.getWidgetContent().attr("min-sizes").split("x");
                if(w < sizes[0] || h < sizes[1])
                {
                    // image too small...
                    // TODO: fix alert with clean error popup
                    alert("This image is too small. Minimum required is "
                            + e.target.snapEditorWidget.getWidgetContent().attr("min-sizes")
                            + ". Please try with a larger image.");
                    return;
                }
            }
            if(e.target.snapEditorWidget.getWidgetContent().attr("max-sizes"))
            {
                sizes = e.target.snapEditorWidget.getWidgetContent().attr("max-sizes").split("x");
                if(w > sizes[0] || h > sizes[1])
                {
                    // image too large...
                    // TODO: fix alert with clean error popup
                    alert("This image is too large. Maximum allowed is "
                            + e.target.snapEditorWidget.getWidgetContent().attr("max-sizes")
                            + ". Please try with a smaller image.");
                    return;
                }
            }

            if(e.target.snapEditorWidget.getWidgetContent().attr("resize-sizes"))
            {
                max_sizes = e.target.snapEditorWidget.getWidgetContent().attr("resize-sizes").split("x");
                limit_width = max_sizes[0];
                limit_height = max_sizes[1];
            }

            if(limit_width > 0 && limit_height > 0)
            {
                if(w > limit_width || h > limit_height)
                {
                    // source image is too large
                    nw = Math.round(limit_height / h * w);
                    nh = Math.round(limit_width / w * h);
                    if(nw > limit_width && nh > limit_height)
                    {
                        // TBD can this happen?
                        alert("somehow we could not adjust the dimentions of the image properly!?");
                    }
                    if(nw > limit_width)
                    {
                        h = nh;
                        w = limit_width;
                    }
                    else
                    {
                        w = nw;
                        h = limit_height;
                    }
                }
            }
            jQuery(img)
                .attr("width", w)
                .attr("height", h)
                .attr("filename", e.target.snapEditorFile.name)
                .css({top: (limit_height - h) / 2, left: (limit_width - w) / 2, position: "relative"});

            that.droppedImage(e, img);
        };
    img.src = e.target.result;

    // TBD: still a valid test? img.readyState is expected to be a string!
    //
    // a fix for browsers that don't call onload() if the image is
    // already considered loaded by now
    if(img.complete || img.readyState == 4)
    {
        img.onload();
    }
};
/*jslint eqeq: false */


/*jslint unparam: true */
/** \brief Handle an image that was just dropped.
 *
 * This function handles an image as it was just dropped.
 *
 * The default function is abstract (to simulate the abstractness, we throw
 * here.) Only widgets with a type that overloads this function understand
 * images.
 *
 * @param {ProgressEvent} e  The reader data.
 * @param {Image} img  The image te user dropped.
 */
snapwebsites.EditorWidgetType.prototype.droppedImage = function(e, img) // abstract
{
    throw new Error("snapwebsites.EditorWidgetType.prototype.droppedImage() not overridden and thus it cannot handle the dropped image.");
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Handle an image that was just dropped.
 *
 * This function handles an image as it was just dropped.
 *
 * @param {ProgressEvent} e  The event.
 */
snapwebsites.EditorWidgetType.prototype.droppedAttachment = function(e) // abstract
{
    throw new Error("snapwebsites.EditorWidgetType.prototype.droppedAttachment() not overridden and thus it cannot handle the dropped attachment.");
};
/*jslint unparam: false */



/** \brief The constructor of the widget type Content Editable.
 *
 * This intermediate type offers an "Edit" button for the user to enter
 * editing mode on an area which by default won't be editable.
 *
 * Note that widgets can be marked as immediate in order to force the
 * editing capability of the widget and bypass this "Edit" button.
 *
 * \code
 *  class EditorWidgetTypeContentEditable extends EditorWidgetType
 *  {
 *  public:
 *      function EditorWidgetTypeContentEditable();
 *      function setupEditButton(editor_widget: snapwebsites.EditorWidget) : void;
 *      function getEditButton() : string;
 *  };
 * \endcode
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeContentEditable = function()
{
    snapwebsites.EditorWidgetTypeContentEditable.superClass_.constructor.call(this);

    return this;
};


/** \brief EditorWidgetTypeContentEditable inherits from EditorWidgetType.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeContentEditable, snapwebsites.EditorWidgetType);


/** \brief Initialize an "Edit" button.
 *
 * This function adds an "Edit" button to the specified \p editor_widget.
 * By default most widgets don't get an edit button because they are
 * automatically editable (if not disabled or marked as read-only.)
 *
 * This is especially necessary on editable text areas where links would
 * otherwise not react to clicks since editable areas accept focus instead.
 *
 * @param {snapwebsites.EditorWidget} editor_widget  The widget being
 *                                                   initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeContentEditable.prototype.setupEditButton = function(editor_widget)
{
    var w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        html,
        edit_button_popup;

    if(w.is(".immediate"))
    {
        // editor is immediately made available
        // (most usual in editor forms)
        c.attr("contenteditable", "true");
        return;
    }

    // get the HTML of the "Edit" button
    html = this.getEditButton();
    if(!html)
    {
        return;
    }

    jQuery(/** @type {string} */ (html)).prependTo(w);

    edit_button_popup = w.children(".editor-edit-button");

    // user has to click Edit to activate the editor
    edit_button_popup.children(".activate-editor").click(function()
        {
            // simulate a mouseleave so the edit button form gets hidden
            // then remove the hover events
            w.mouseleave().off("mouseenter mouseleave");

            // make the child editable and give it the focus
            // TODO: either select all or at least place the cursor at the
            //       end in some cases...
            c.attr("contenteditable", "true").focus();
        });

    // this adds the mouseenter and mouseleave events
    w.hover(
        function()  // mouseenter
        {
            edit_button_popup.fadeIn(150);
        },
        function()  // mouseleave
        {
            edit_button_popup.fadeOut(150);
        });
};


/** \brief The HTML representing the "Edit" button.
 *
 * This function returns the default "Edit" button HTML. It can be
 * overridden in order to change the button.
 *
 * \note
 * If the function returns an empty string or false then it is assumed
 * that the object does not want an edit button (i.e. a checkmark object.)
 *
 * @return {!string|!boolean}  A string representing the HTML of the "Edit"
 *                             button or false.
 */
snapwebsites.EditorWidgetTypeContentEditable.prototype.getEditButton = function() // virtual
{
    return "<div class='editor-edit-button'><a class='activate-editor' href='#'>Edit</a></div>";
};



/** \brief Editor widget type for Hidden widgets.
 *
 * This widget defines hidden elements in the editor forms. This is
 * an equivalent to the hidden input element of a standard form, although
 * our hidden widgets can include any type of text.
 *
 * \code
 *  class EditorWidgetTypeHidden extends EditorWidgetType
 *  {
 *  public:
 *      virtual function getType() : string;
 *      virtual function initializeWidget(widget: Object) : void;
 *  };
 * \endcode
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeHidden = function()
{
    snapwebsites.EditorWidgetTypeHidden.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeHidden, snapwebsites.EditorWidgetType);


/** \brief Return "hidden".
 *
 * Return the name of the hidden type.
 *
 * @return {string} The name of the hidden type.
 * @override
 */
snapwebsites.EditorWidgetTypeHidden.prototype.getType = function() // virtual
{
    return "hidden";
};


/** \brief Initialize the widget.
 *
 * This function initializes the hidden widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeHidden.prototype.initializeWidget = function(widget) // virtual
{
    snapwebsites.EditorWidgetTypeHidden.superClass_.initializeWidget.call(this, widget);
};



/** \brief Editor widget type for Text Edit widgets.
 *
 * This widget defines the full text edit in the editor forms. This is
 * an equivalent to the text area of a standard form.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeContentEditable}
 * @struct
 */
snapwebsites.EditorWidgetTypeTextEdit = function()
{
    snapwebsites.EditorWidgetTypeTextEdit.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeTextEdit, snapwebsites.EditorWidgetTypeContentEditable);


/** \brief Return "text-edit".
 *
 * Return the name of the text edit type.
 *
 * @return {string} The name of the text edit type.
 * @override
 */
snapwebsites.EditorWidgetTypeTextEdit.prototype.getType = function() // virtual
{
    return "text-edit";
};


/** \brief Initialize the widget.
 *
 * This function initializes the text-edit widget. It setups a keydown
 * event to prevent editing if the widget is marked as read-only or
 * disabled.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeTextEdit.prototype.initializeWidget = function(widget) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        c = editor_widget.getWidgetContent();

    snapwebsites.EditorWidgetTypeTextEdit.superClass_.initializeWidget.call(this, widget);

    c.keydown(function(e)
        {
            var widget = c.parent();

            // do not prevent the Tab and Enter
            if(e.which == 9 || e.which == 13)
            {
                return;
            }

            // TBD: we may need to allow various keys when the widget is
            //      marked as 'read-only' (i.e. Ctrl-C, arrows, etc.)
            if(widget.is(".read-only")
            || widget.is(".disabled"))
            {
                // no typing allowed
                e.preventDefault();
                e.stopPropagation();
            }
        });
};


/** \brief Save a new value in the specified editor widget.
 *
 * This function offers a way for programmers to dynamically change the
 * value of a widget. You should never call the editor widget type
 * function, instead use the setValue() function of the widget you
 * want to change the value of (it will make sure that the modified
 * flag is properly set.)
 *
 * Depending on the type, the value may be a string, a number, of some
 * other type, this is why here it is marked as an object.
 *
 * @param {!Object} widget  The concerned widget.
 * @param {!Object|string|number} value  The value to be saved.
 *
 * @return {boolean}  true if the value gets changed.
 */
snapwebsites.EditorWidgetTypeTextEdit.prototype.setValue = function(widget, value)
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        c = editor_widget.getWidgetContent();

    if(c.html() !== value)
    {
        c.empty();
        c.append(/** @type {string} */ (value));
        return true;
    }

    return false;
};



/** \brief Editor widget type for Text Edit widgets.
 *
 * This widget defines the full text edit in the editor forms. This is
 * an equivalent to the text area of a standard form.
 *
 * @return {!snapwebsites.EditorWidgetTypeLineEdit}
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeTextEdit}
 * @struct
 */
snapwebsites.EditorWidgetTypeLineEdit = function()
{
    snapwebsites.EditorWidgetTypeLineEdit.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeLineEdit, snapwebsites.EditorWidgetTypeTextEdit);


/** \brief Return "line-edit".
 *
 * Return the name of the line edit type.
 *
 * @return {string} The name of the line edit type.
 * @override
 */
snapwebsites.EditorWidgetTypeLineEdit.prototype.getType = function()
{
    return "line-edit";
};


/** \brief Initialize the widget.
 *
 * This function initializes the line-edit widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeLineEdit.prototype.initializeWidget = function(widget) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        //w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent();

    snapwebsites.EditorWidgetTypeLineEdit.superClass_.initializeWidget.call(this, widget);

    c.keydown(function(e)
        {
            if(e.which === 13)
            {
                // prevent enter from doing anything here
                //
                // TODO: actually we want return to apply the submit if
                //       there is one
                //
                e.preventDefault();
                e.stopPropagation();
            }
        });
};



/** \brief Editor widget type for Dropdown widgets.
 *
 * This widget defines a dropdown in the editor forms.
 *
 * \note
 * The EditorWidgetTypeDropdown holds a reference to the last dropdown
 * items that was opened so it can close it in the event any other
 * dropdown happened to get opened.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeLineEdit}
 * @struct
 */
snapwebsites.EditorWidgetTypeDropdown = function()
{
    snapwebsites.EditorWidgetTypeDropdown.superClass_.constructor.call(this);

    return this;
};


/** \brief EditorWidgetTypeDropdown inherits from EditorWidgetTypeLineEdit.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeDropdown, snapwebsites.EditorWidgetTypeLineEdit);


/** \brief The currently opened dropdown.
 *
 * This variable member holds the jQuery widget of the currently opened
 * dropdown. It is used to be able to close that widget whenever another
 * one gets opened or the widget loses focus.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.openDropdown_ = null;


/** \brief Whether the currently opened dropdown is a clone.
 *
 * This variable member is true when the openDropdown_ variable is
 * a clone of the dropdown elements. This means it will be deleted
 * after it gets closed.
 *
 * By default, this is false as it is assumed that your dropdown is
 * created in the top-most window.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.clonedDropdown_ = false;


/** \brief Return "dropdown".
 *
 * Return the name of the dropdown type.
 *
 * @return {string} The name of the dropdown type.
 * @override
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.getType = function() // virtual
{
    return "dropdown";
};


/** \brief Initialize the widget.
 *
 * This function initializes the dropdown widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.initializeWidget = function(widget) // virtual
{
    var that = this,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        d = w.children(".dropdown-items");

    snapwebsites.EditorWidgetTypeDropdown.superClass_.initializeWidget.call(this, widget);

    //
    // WARNING: we dynamically assign the "absolute" flag because otherwise
    //          the item doesn't get positioned right at the start (i.e. in
    //          this way we don't have to compute the position.)
    //
    d.css("position", "absolute");

    // Click hides currently opened dropdown if there is one.
    // Then shows the dropdown of the clicked widget (unless that's the one
    // we just closed.)
    w.click(function(e)
        {
            var that_element = jQuery(this),
                visible,
                z,
                pos,
                iframe_pos;

            // avoid default browser behavior
            e.preventDefault();
            e.stopPropagation();

            visible = that_element.is(".disabled") || d.is(":visible");

            // hide the dropdown AFTER we checked its visibility
            that.hideDropdown();

            if(!visible)
            {
                // test with 'window.' so it works in IE
                if(window.self != window.top)
                {
                    that.openDropdown_ = window.top.jQuery("<div class='top-window dropdown-items zordered' style='position: absolute;'>" + d.html() + "</div>").appendTo("body");
                    pos = c.offset();
                    iframe_pos = window.top.jQuery("#create-finball.snap-popup .popup-body iframe").offset();
                    pos.left += iframe_pos.left;
                    pos.top += iframe_pos.top + w.height();
                    that.openDropdown_.offset(pos);
                    that.clonedDropdown_ = true;

                    that.openDropdown_
                        .children(".dropdown-selection")
                        .children(".dropdown-item")
                        .click(function(e)
                            {
                                that.itemClicked(e, editor_widget);
                            });

                    // setup z-index
                    // (reset itself first so we do not just +1 each time)
                    that.openDropdown_.css("z-index", 0);
                    z = window.top.jQuery("div.zordered").maxZIndex() + 1;
                    that.openDropdown_.css("z-index", z);
                }
                else
                {
                    // the newly visible dropdown
                    that.openDropdown_ = d;

                    // setup z-index
                    // (reset itself first so we do not just +1 each time)
                    that.openDropdown_.css("z-index", 0);
                    z = jQuery("div.zordered").maxZIndex() + 1;
                    that.openDropdown_.css("z-index", z);
                }

                that.openDropdown_.fadeIn(150);
            }
        });

    d.children(".dropdown-selection")
        .children(".dropdown-item")
        .click(function(e)
            {
                that.itemClicked(e, editor_widget);
            });

    c.blur(function()
        {
            that.hideDropdown();
        });

    // TODO: we need to add support for the up/down arrow keys to change
    //       the selection
    //w.keydown(function(e)
    //    {
    //        // ...
    //    });
};


/** \brief Request that the current dropdown be closed.
 *
 * This function ensures that the current dropdown gets closed.
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.hideDropdown = function()
{
    var clone = null;

    if(this.openDropdown_)
    {
        // if we are in the top window, otherwise it is already hidden
        if(this.clonedDropdown_)
        {
            clone = this.openDropdown_;
        }
        this.openDropdown_.fadeOut(150, function()
            {
                if(clone)
                {
                    clone.remove();
                }
            });
        this.openDropdown_ = null;
    }
};


/** \brief Handle a click on a dropdown item.
 *
 * This function is called whenever the user clicks on a dropdown
 * item.
 *
 * @param {Event} e  The jQuery click event on this item.
 * @param {snapwebsites.EditorWidget} editor_widget  The widget representing the dropdown.
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.itemClicked = function(e, editor_widget)
{
    var w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        d = w.children(".dropdown-items"),
        that_element = jQuery(e.target),
        value,
        widget_change;

    // avoid default browser behavior
    e.preventDefault();
    e.stopPropagation();

    // hide the dropdown (we could use d.toggle() but that
    // would not update the openDropdon_ variable member)
    this.hideDropdown();

    // first select the new item
    d.find(".dropdown-item").removeClass("selected");
    that_element.addClass("selected");

    // then copy the item label to the "content" (line edit)
    c.empty();
    c.append(that_element.html());

    // finally, get the resulting value if there is one
    value = that_element.attr("value");
    if(value)
    {
        c.attr("value", snapwebsites.castToString(value, "dropdown item value attribute"));
    }
    else
    {
        // canonicalize the undefined value
        value = null; // TBD should it be value = c.text(); ?
        c.removeAttr("value");
    }

    // make that dropdown the currently active object
    c.focus();
    editor_widget.getEditorBase().setActiveElement(c);

    // send an event for each change because the user
    // make want to know even if the value was not actually
    // modified
    widget_change = jQuery.Event("widgetchange", {
            widget: editor_widget,
            value: value
        });
    w.trigger(widget_change);

    editor_widget.getEditorBase().checkModified();
};


/** \brief Reset the value of the specified editor widget.
 *
 * This function offers a way for programmers to dynamically reset the
 * value of a dropdown widget. You should never call the editor widget
 * type function, instead use the resetValue() function of the widget you
 * want to reset the value of (it will make sure that the modified
 * flag is properly set.)
 *
 * For a dropdown, it may reset the value back to the original as if the
 * user had never selected anything. This may be a value found in the list
 * of items, or it may be a string such as "Please select an option ...".
 * If the default value is not equal to any one of the items, then the
 * function makes sure to remove the "value" attribute of the widget.
 *
 * If the default value is an empty string, then that is used. If you do
 * not want that resolution, you may want to use the setValue()
 * function instead.
 *
 * \todo
 * We want to add support for the item marked as the default. At this
 * point we only support the default string used to ask the user to
 * "Please select an entry..."
 *
 * @param {!Object} widget  The concerned widget.
 *
 * @return {boolean}  true if the value gets changed.
 *
 * \sa setValue()
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.resetValue = function(widget)
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        d = w.children(".dropdown-items"),
        item = d.children(".dropdown-selection").children(".dropdown-item"),
        reset_value = w.children(".snap-editor-dropdown-reset-value"),
        widget_change,
        result;

    // at this point the reset value cannot be a value defined in
    // the widget;
    //
    // TODO: later add support for default values (pre-selected item)
    //       when the widget itself does not otherwise offer a full
    //       reset value...

    item.removeClass("selected");

    // check whether the value was defined
    result = !!c.attr("value");

    // reset the actual line edit
    c.empty();
    c.append(reset_value.html());

    if(result)
    {
        // if "value" is defined, reset all of that and emit an event
        c.removeAttr("value");
        c.focus();
        editor_widget.getEditorBase().setActiveElement(c);

        // send an event for each change because the user
        // make want to know even if the value was not actually
        // modified
        widget_change = jQuery.Event("widgetchange",
            {
                widget: editor_widget,
                value: null
            });
        w.trigger(widget_change);

        editor_widget.getEditorBase().checkModified();
    }

    // if the item had a value attribute, this returns true
    // (i.e. it was modified)
    return result;
};


/** \brief Save a new value in the specified editor widget.
 *
 * This function offers a way for programmers to dynamically change the
 * value of a dropdown widget. You should never call the editor widget
 * type function, instead use the setValue() function of the widget you
 * want to change the value of (it will make sure that the modified
 * flag is properly set.)
 *
 * For a dropdown, the value parameter is expected to be one of the strings
 * found in the list of items or a number. If it is a number, then that
 * item gets selected (we use floor() to round any number down). If it
 * is a string it searches the list of items for it and selects that item.
 * If two or more items have the same label, then the first one is selected.
 *
 * If the index number is too large or the specified string is not found
 * in the existing items, then nothing happens.
 *
 * @param {!Object} widget  The concerned widget.
 * @param {!Object|string|number} value  The value to be saved.
 *
 * @return {boolean}  true if the value gets changed.
 *
 * \sa resetValue()
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.setValue = function(widget, value)
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        d = w.children(".dropdown-items"),
        item = d.children(".dropdown-selection").children(".dropdown-item"),
        that_item,
        item_value,
        widget_change;

    if(typeof value === "number")
    {
        that_item = item.eq(value);
    }
    else
    {
        //
        // if not a number, search the list of items:
        //
        //   . first we check with the "value" attribute; the value
        //     attribute is authoritative so we do want to test that
        //     first; this is viewed as a strong selection since two
        //     items cannot (should not at time of writing, really)
        //     share the same value
        //
        //   . second we give the user a chance to select using the
        //     HTML of one of the items (this is a requirement since
        //     some entries may not use the "value" attribute); this
        //     is somewhat flaky since two items could have the exact
        //     same HTML and only the first one will be selected
        //

        // Note: the filter may return more than one item, in that case
        //       we simply ignore the extra items
        that_item = item.filter(function()
            {
                var e = jQuery(this),
                    attr = e.attr("value");

                // note: if the value attribute is "" (an empty string)
                //       then we algorithm fails here and anywhere else
                //       we use that technique.
                if(attr)
                {
                    // if we have an attribute, match that
                    return attr === value;
                }

                // not a match...
                return false;
            }).eq(0);

        if(!that_item.exists())
        {
            // Note: the filter may return more than one item, in that case
            //       we simply ignore the extra items
            that_item = item.filter(function()
                {
                    // no 'value' attribute, attempt to match the HTML data
                    return jQuery(this).html() === value;
                }).eq(0);
        }
    }

    // TODO: if this Dropdown supports editing (i.e. not just read-only)
    //       then this value has to become the new value anyway.

    // if already selected, in effect we are not changing anything
    if(that_item.exists() && !that_item.hasClass("selected"))
    {
        item.removeClass("selected");
        that_item.addClass("selected");

        // then copy the item label to the "content" (line edit)
        c.empty();
        c.append(that_item.html());

        // finally, get the resulting value if there is one
        item_value = that_item.attr("value");
        if(item_value)
        {
            c.attr("value", snapwebsites.castToString(value, "dropdown item value attribute"));
        }
        else
        {
            // canonicalize the undefined value
            item_value = null; // TBD should it be item_value = value; ?
            c.removeAttr("value");
        }

        // make that dropdown the currently active object
        c.focus();
        editor_widget.getEditorBase().setActiveElement(c);

        // send an event for each change because the user
        // may want to know even if the value was not actually
        // modified
        widget_change = jQuery.Event("widgetchange",
            {
                widget: editor_widget,
                value: item_value
            });
        w.trigger(widget_change);

        editor_widget.getEditorBase().checkModified();

        return true;
    }

    return false;
};



/** \brief Editor widget type for Checkmark widgets.
 *
 * This widget defines a checkmark in the editor forms.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeCheckmark = function()
{
    snapwebsites.EditorWidgetTypeCheckmark.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeCheckmark, snapwebsites.EditorWidgetType);


/** \brief Return "checkmark".
 *
 * Return the name of the checkmark type.
 *
 * @return {string} The name of the checkmark type.
 * @override
 */
snapwebsites.EditorWidgetTypeCheckmark.prototype.getType = function() // virtual
{
    return "checkmark";
};


/** \brief Initialize the widget.
 *
 * This function initializes the checkmark widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeCheckmark.prototype.initializeWidget = function(widget) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        toggle = function()
            {
                // toggle the current value
                var checkmark = c.find(".checkmark-area");
                checkmark.toggleClass("checked");
                c.attr("value", checkmark.hasClass("checked") ? 1 : 0);

                // TBD: necessary to avoid setting the focus anew?
                //      if not then we should remove the if() statement
                //
                // Note: from what I can tell, c is a jQuery and so we
                //       should do a get on it too; at this point, it seems
                //       that calling focus() everytime is just fine.
                //
                //if(editor_widget.getEditorBase().getActiveElement().get() != c)
                //{
                    c.focus();
                //}

                // tell the editor that something may have changed
                // TODO: call the widget function which in turn tells the
                //       editor instead of re-testing all the widgets?!
                editor_widget.getEditorBase().checkModified();
            };

    snapwebsites.EditorWidgetTypeCheckmark.superClass_.initializeWidget.call(this, widget);

    c.keydown(function(e)
        {
            if(e.which === 0x20) // spacebar
            {
                e.preventDefault();
                e.stopPropagation();

                toggle();
            }
        });

    w.click(function(e)
        {
            if(!(jQuery(e.target).is("a")))
            {
                // the default may do weird stuff, so avoid it!
                e.preventDefault();
                e.stopPropagation();

                toggle();
            }
        });
};


// This is not necessary anymore, but I want to keep it for documentation
// purposes.
//
// * brief Change the result to just 0 or 1.
// *
// * This function changes the result of a checkmark as the value 0 or 1
// * instead of the HTML of the sub-objects. This value represents the
// * current selection (0 -- not checked, or 1 -- checked.)
// *
// * param {!Object} editor_widget  The concerned widget
// * param {snapwebsites.EditorWidgetTypeBase.SaveData} data  The data object with the HTML and result parameters.
// *
//snapwebsites.EditorWidgetTypeCheckmark.prototype.saving = function(editor_widget, data) // virtual
//{
//    snapwebsites.EditorWidgetType.prototype.initializeWidget.apply(this, data);
//
//    data.result = edit_area.find(".checkmark-area").hasClass("checked") ? 1 : 0;
//}



/** \brief Editor widget type for Radio widgets.
 *
 * This widget defines a set of radio buttons in editor forms.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeRadio = function()
{
    snapwebsites.EditorWidgetTypeRadio.superClass_.constructor.call(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and its super.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeRadio, snapwebsites.EditorWidgetType);


/** \brief Return "radio".
 *
 * Return the name of the radio type.
 *
 * @return {string} The name of the radio type.
 * @override
 */
snapwebsites.EditorWidgetTypeRadio.prototype.getType = function() // virtual
{
    return "radio";
};


/** \brief Initialize the widget.
 *
 * This function initializes the checkmark widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeRadio.prototype.initializeWidget = function(widget) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        c = editor_widget.getWidgetContent(),
        select_radio = function(radio_button)
            {
                // select the current radio button
                var radio = jQuery(radio_button);
                if(!radio.hasClass("selected"))
                {
                    // remove all selected
                    radio.parent().children("li").removeClass("selected");
                    // then select this radio button
                    radio.addClass("selected");
                    c.attr("value", snapwebsites.castToString(radio.attr("value"), "casting radio value to a string"));

                    // tell the editor that something may have changed
                    // TODO: call the widget function which in turn tells the
                    //       editor instead of re-testing all the widgets?!
                    editor_widget.getEditorBase().checkModified();
                }
            };

    snapwebsites.EditorWidgetTypeRadio.superClass_.initializeWidget.call(this, widget);

    c.keydown(function(e)
        {
            if(e.which === 0x20) // spacebar
            {
                e.preventDefault();
                e.stopPropagation();

                select_radio(c.find(":focus"));
            }
        });

    w.find("li").click(function(e)
        {
            if(!(jQuery(e.target).is("a")))
            {
                // the default may do weird stuff, so avoid it!
                e.preventDefault();
                e.stopPropagation();

                select_radio(this);
            }
        });
};



/** \brief Editor widget type for Image Box widgets.
 *
 * This widget defines an image box in the editor forms. The whole widget
 * is just and only an image. You cannot type in data and when an image is
 * dragged and dropped over that widget, it replaces the previous version
 * of the image.
 *
 * If required, the widget is smart enough to use a proportional resize
 * so the image fits the widget area.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeImageBox = function()
{
    snapwebsites.EditorWidgetTypeImageBox.superClass_.constructor.call(this);

    return this;
};


/** \brief EditorWidgetTypeImageBox inherits from EditorWidgetType.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeImageBox, snapwebsites.EditorWidgetType);


/** \brief Return "image-box".
 *
 * Return the name of the image box type.
 *
 * @return {string} The name of the image box type.
 * @override
 */
snapwebsites.EditorWidgetTypeImageBox.prototype.getType = function() // virtual
{
    return "image-box";
};


/** \brief Initialize the widget.
 *
 * This function initializes the image box widget.
 *
 * \note
 * At this point the Image Box widget do not attach to any events since all
 * the drag and drop work is done at the EditorWidgetType level. However,
 * we have a droppedImage() function (see below) which finishes the work
 * of the drag and drop implementation.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeImageBox.prototype.initializeWidget = function(widget) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget),
        w = editor_widget.getWidget(),
        background = w.children(".snap-editor-background");

    snapwebsites.EditorWidgetTypeImageBox.superClass_.initializeWidget.call(this, widget);

    // backgrounds are positioned as "absolute" so their width
    // is "broken" and we cannot center them in their parent
    // which is something we want to do for image-box objects
    background.css("width", snapwebsites.castToNumber(w.width(), "ImageBox widget width"))
              .css("margin-top", (snapwebsites.castToNumber(w.height(), "ImageBox widget height")
                                      - snapwebsites.castToNumber(background.height(), "ImageBox background height")) / 2);
};


/** \brief Handle the dropped image.
 *
 * This function handles the dropped image by saving it in the target
 * element.
 *
 * @param {ProgressEvent} e  The element.
 * @param {Image} img  The image to insert in the destination widget.
 *
 * @override
 */
snapwebsites.EditorWidgetTypeImageBox.prototype.droppedImage = function(e, img)
{
    var saved_active_element, editor;

    e.target.snapEditorWidget.getWidgetContent().empty();
    jQuery(img).appendTo(e.target.snapEditorWidget.getWidgetContent());

    // now make sure the editor detects the change
    editor = snapwebsites.EditorInstance;
    saved_active_element = editor.getActiveElement();
    editor.setActiveElement(e.target.snapEditorWidget.getWidgetContent());
    editor.checkModified();
    editor.setActiveElement(saved_active_element);
};



/** \brief Editor widget type for Dropped Image with a Preview.
 *
 * This widget defines an image box in the editor forms. The image form
 * is used to display a preview of the file that gets dropped in it. This
 * is an extension of the EditorWidgetTypeImageBox which itself only
 * accepts recognized image files.
 *
 * Just like the image box widget, this widget does not allow for typing,
 * only to drag and drop a file in it. A new file dropped on this widget
 * replaces the previously attached file (it is not additive.)
 *
 * As required, the widget is smart enough to use a proportional resize
 * so the preview is made to fit the widget area.
 *
 * The widget accepts images just like the EditorWidgetTypeImageBox widget
 * does. It also can accept file formats that we can transform to a preview
 * (i.e. a PDF of which the first page will be transform in a preview.)
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeImageBox}
 * @struct
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview = function()
{
    snapwebsites.EditorWidgetTypeDroppedFileWithPreview.superClass_.constructor.call(this);

    return this;
};


/** \brief EditorWidgetTypeDroppedFileWithPreview inherits from EditorWidgetTypeImageBox.
 *
 * This call ensures proper inheritance between the two classes.
 */
snapwebsites.inherits(snapwebsites.EditorWidgetTypeDroppedFileWithPreview, snapwebsites.EditorWidgetTypeImageBox);


/** \brief A server access object.
 *
 * Whenever the user drops a file, we use this object to send it to
 * the server. In turn the server sends us a reply to know whether
 * the file was accepted or not. Later we will be able to check on
 * the server to know whether it has a preview for us to display.
 *
 * \note
 * This serverAccess_ object is shared between all the drop widget
 * of this type of attachments.
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.serverAccess_ = null;


/** \brief Return "dropped-file-with-preview".
 *
 * Return the name of the image box type.
 *
 * Note that this widget type has 2 sub-types named
 * "dropped-image-with-preview" and "dropped-any-with-preview". However,
 * these sub-types cannot be distinguished here. Instead you have to
 * check whether it has classes "attachment" and/or "image".
 *
 * @return {string} The name of the image box type.
 * @override
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.getType = function() // virtual
{
    return "dropped-file-with-preview";
};


/** \brief Initialize the widget.
 *
 * This function initializes the image box widget.
 *
 * \note
 * At this point the Image Box widget do not attach to any events since all
 * the drag and drop work is done at the EditorWidgetType level. However,
 * we have a droppedImage() function (see below) which finishes the work
 * of the drag and drop implementation.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.initializeWidget = function(widget) // virtual
{
    snapwebsites.EditorWidgetTypeDroppedFileWithPreview.superClass_.initializeWidget.call(this, widget);
};


/** \brief Handle the dropped file.
 *
 * This function handles the dropped image by saving it in the target
 * element.
 *
 * \todo
 * We may want to look into generating an MD5 checksum and check that first
 * because the file may already be available on the server. Calculating an
 * MD5 in JavaScript is not that hard...
 *
 * @param {ProgressEvent} e  The event.
 *
 * @override
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.droppedAttachment = function(e)
{
    var form_data,
        editor_widget = /** @type {snapwebsites.EditorWidget} */ (e.target.snapEditorWidget),
        editor_form = editor_widget.getEditorForm(),
        w = editor_widget.getWidget(),
        session = editor_form.getSession(),
        title_widget = editor_form.getWidgetByName("title"),
        name = editor_widget.getName(),
        wait_image,
        broken_icon = jQuery(".broken-attachment-icon"),
        icon_widget = w.children(".attachment-icon");

    // hide previous icons if the user is doing this a second time
    broken_icon.hide();
    icon_widget.hide();

    //
    // In case of an attachment, we send them to the server because the
    // browser cannot just magically generate a preview; so we create
    // a POST with the data and send that.
    //
    // (Note: the client "could" create a preview, it would just take
    // a day or two and loads of memory...)
    //
    // The data is attached to the session of this editor form. We do that
    // because we do not want to involve the main plugin responsible for
    // the form until a full POST of all the changes. We can still have a
    // hook on a callback if necessary.
    //
    // Note that in the end this means the data of an editor form come from
    // the client and the editor session system.
    //

    form_data = new FormData();
    form_data.append("_editor_session", session);
    form_data.append("_editor_save_mode", "attachment");
    if(title_widget)
    {
        form_data.append("_editor_uri", snapwebsites.EditorForm.titleToURI(title_widget.saving().result));
        //form_data.append("_editor_uri", snapwebsites.EditorForm.titleToURI(snapwebsites.castToString(jQuery("[field_name='title'] .editor-content").text(), "casting the field name title to a string")));
    }
    form_data.append("_editor_widget_names", name); // this field supports multiple names separated by commas
    form_data.append(name, e.target.snapEditorFile);

    // mark widget as processing (allows for CSS effects)
    w.addClass("processing-attachment");

    // show a waiting rotating image
    editor_widget.showWaitImage();

    if(!this.serverAccess_)
    {
        this.serverAccess_ = new snapwebsites.ServerAccess(this);
    }
    this.serverAccess_.setURI(snapwebsites.castToString(jQuery("link[rel='canonical']").attr("href"), "casting href of the canonical link to a string in snapwebsites.EditorWidgetTypeDroppedFileWithPreview.droppedAttachment()"));
    this.serverAccess_.setData(form_data);
    this.serverAccess_.send(e);
};


/*jslint unparam: true */
/** \brief Function called on AJAX success.
 *
 * This function is called if the remote access was successful. The
 * result object includes a reference to the XML document found in the
 * data sent back from the server.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.serverAccessSuccess = function(result) // virtual
{
    // the response was successful so responseXML is valid,
    // get the canonicalized path to the file we just sent
    // to the server (this is a full URI)
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (result.userdata.target.snapEditorWidget),
        w = editor_widget.getWidget(),
        xml_data = jQuery(result.jqxhr.responseXML),
        attachment_path = xml_data.find("data[name='attachment-path']").text(),
        attachment_icon = xml_data.find("data[name='attachment-icon']").text(),
        icon_widget,
        request,
        preview_uri = attachment_path + "/preview.jpg";

    snapwebsites.EditorWidgetTypeDroppedFileWithPreview.superClass_.serverAccessSuccess.call(this, result);

    // show the attachment icon
    icon_widget = w.children(".attachment-icon");
    if(icon_widget.length == 0)
    {
        w.prepend("<div class=\"attachment-icon\"><img src=\"" + attachment_icon + "\"/></div>");
        icon_widget = w.children(".attachment-icon");
    }
    else
    {
        icon_widget.show();
    }

    request = new snapwebsites.ListenerRequest(
        {
            uri: preview_uri,
            success: function(request)
                {
                    var preview_widget = editor_widget.getWidgetContent(),
                        editor = snapwebsites.EditorInstance,
                        saved_active_element = editor.getActiveElement();

                    preview_widget.empty();
                    jQuery("<img src=\"" + preview_uri + "\"/>").appendTo(preview_widget);

                    // now make sure the editor detects the change
                    // (even though we do not expect to re-save this widget)
                    editor.setActiveElement(preview_widget);
                    editor.checkModified();
                    editor.setActiveElement(saved_active_element);
                },
            error: function(request)
                {
                    var broken_icon;

                    // show a broken image in this case
//console.log("Editor Listener Request: FAILURE!");
                    broken_icon = jQuery(".broken-attachment-icon");
                    if(broken_icon.length == 0)
                    {
                        // TODO: fix the path with our sitekey
                        w.prepend("<div class=\"broken-attachment-icon\"><img src=\"/images/mimetype/file-broken-document.png\" width=\"48\" height=\"48\"/></div>");
                    }
                    else
                    {
                        broken_icon.show();
                    }
                },
            complete: function(request)
                {
//console.log("Editor Listener Request: COMPLETE!");
                    // not waiting anymore
                    editor_widget.hideWaitImage();
                    icon_widget.hide();
                }
        });
    request.setSpeeds([10, 3]);
    snapwebsites.ListenerInstance.addRequest(request);
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Function called on AJAX error.
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
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.serverAccessError = function(result) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (result.userdata.target.snapEditorWidget);

    snapwebsites.EditorWidgetTypeDroppedFileWithPreview.superClass_.serverAccessError.call(this, result);

    editor_widget.hideWaitImage();
};
/*jslint unparam: false */


/*jslint unparam: true */
/** \brief Function called on AJAX completion.
 *
 * This function is called once the whole process is over. It is most
 * often used to do some cleanup.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.EditorWidgetTypeDroppedFileWithPreview.prototype.serverAccessComplete = function(result) // virtual
{
    snapwebsites.EditorWidgetTypeDroppedFileWithPreview.superClass_.serverAccessComplete.call(this, result);

    // done processing
    result.userdata.target.snapEditorWidget.getWidget().removeClass("processing-attachment");
};
/*jslint unparam: false */


// auto-initialize
jQuery(document).ready(function()
    {
        snapwebsites.CartInstance = new snapwebsites.Cart();
        snapwebsites.CartInstance.registerProductType(new snapwebsites.ProductTypeBasic());
    });

// vim: ts=4 sw=4 et
