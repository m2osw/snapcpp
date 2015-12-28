/** @preserve
 * Name: popup
 * Version: 0.1.0.31
 * Browsers: all
 * Copyright: Copyright 2014-2015 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: output (0.1.5.71)
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
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false */



/** \brief Snap Popup.
 *
 * This class initializes and handles the different popup features.
 *
 * \note
 * The Snap! Output handles a popup capability that can be used to open
 * any number of popups.
 *
 * By default popups automatically include a close button.
 *
 * @return {!snapwebsites.Popup}
 *
 * @constructor
 * @struct
 */
snapwebsites.Popup = function()
{
    return this;
};


/** \brief Mark Popup as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.Popup);


/** \brief The type definition for the popup structure.
 *
 * The Popup class makes use of a popup structure called PopupData.
 * This is an object passed to most of the Popup object functions
 * that holds the current state of your popup.
 *
 * \li id -- the identifier of the popup DOM object; it has to be unique,
 *           if undefined, the Popup library generates one automatically.
 * \li path -- the path to a page to display in the popup; in this case
 *             the Popup object uses an IFRAME to display the content.
 * \li html -- The HTML to display in the IFRAME, this is exclusive from
 *             the path usage.
 * \li noClose -- do not create a close button for the popup; this is
 *                particularly useful for popup windows that ask a question
 *                and cannot really safely have a close button (because the
 *                meaning of that close button is not known).
 * \li top -- the top position; if undefined, the popup will be vertically
 *            centered.
 * \li left -- the left position; if undefined, the popup will be horizontally
 *             centered.
 * \li width -- the width to use for the popup; this is a required parameter.
 * \li height -- the height to use for the popup; this is a required parameter.
 * \li darken -- the amount of time it will take to darken the screen behind
 *               the popup; null or undefined prevents any darkening; zero
 *               makes the dark overlay appear instantely.
 * \li position -- "absolute" to get the popup to scroll with the window; this
 *                 is useful if you are to have very tall popups; you may also
 *                 use "fixed" to get a popup that doesn't scroll at all;
 *                 the default is "fixed".
 * \li title -- a title for the popup; if empty no popup.
 * \li open -- a callback called once the popup is open, it gets called with
 *             a reference to this popup.
 * \li show -- a callback called once the popup is fully visible (i.e. if
 *             fading in, this callback is called after the fade in is
 *             over); it has no parameters.
 * \li beforeHide -- a callback called before the popup gets hidden; this
 *                   gives your code a chance to cancel a click on the close
 *                   button or equivalent; the callback is called with a
 *                   PopupData object.
 * \li hide -- a callback called once the popup is fully hidden; which means
 *             after the fade out is over; it is called without any
 *             parameters.
 * \li hideNow -- a callback defined by the Popup class in case the
 *                beforeHide is used; that function should be called by
 *                the beforeHide callback in the event the user decided
 *                to anyway close the popup; it takes no parameters.
 * \li widget -- this parameter is defined by the system; it is set to the
 *               jQuery widget using the id parameter.
 *
 * \note
 * The width and height could be calculated with different euristic math
 * and propbably will at some point. The current version requires these
 * parameters to be defined if you want to get a valid popup.
 *
 * @typedef {{id: ?string,
 *            path: ?string,
 *            html: ?string,
 *            noClose: ?boolean,
 *            top: ?number,
 *            left: ?number,
 *            width: number,
 *            height: number,
 *            darken: ?number,
 *            position: string,
 *            title: ?string,
 *            open: ?function(Object),
 *            show: ?function(),
 *            beforeHide: ?function(Object),
 *            hide: ?function(),
 *            hideNow: ?function(),
 *            widget: ?Object}}
 */
snapwebsites.Popup.PopupData;


/** \brief The type used to create a message box.
 *
 * This type definition is used by the snapwebsites.Popup.messageBox()
 * function.
 *
 * \li id -- the identifier for the message box popup.
 *
 * @typedef {{id: string,
 *          title: string,
 *          message: string,
 *          buttons: Object.<{name: string, label: string}>,
 *          top: !number,
 *          left: !number,
 *          width: !number,
 *          height: !number,
 *          html: !string,
 *          callback: function(string)}}
 */
snapwebsites.Popup.PopupMessage;


/** \brief The Popup instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * \@type {snapwebsites.Popup}
 */
snapwebsites.PopupInstance = null; // static


/** \brief A unique identifier.
 *
 * Each time a new popup is created it is given a new unique
 * identifier. This parameter is used to compute that new
 * identifier. The value is saved in the settings passed to
 * the open() function, so on the next call with the same
 * settings, the identifier will already be defined and thus
 * it won't change.
 *
 * @type {number}
 * @private
 */
snapwebsites.Popup.prototype.uniqueID_ = 0;


/** \brief The jQuery DOM object representing the popup.
 *
 * This parameter represents the popup DOM object used to create the
 * popup window. Since there is only one darken page popup, this is
 * created once and reused as many times as required.
 *
 * Note, however, that it is not "safe" to open a popup from a popup
 * as that will reuse that same darkenpagepopup object which will
 * obstruct the view.
 *
 * @type {Object}
 * @private
 */
snapwebsites.Popup.prototype.darkenPagePopup_ = null;


/** \brief Create a DIV used to darken the background.
 *
 * This function creates a DIV tag used to darken the background of the
 * screen when a popup appears.
 *
 * The function creates a single DIV. If you call the function multiple
 * times, the existing DIV is reused over and over again.
 *
 * \todo
 * The function needs to retrieve the maximum z-index, without including
 * the popup that needs to appear over it, each time it gets called so
 * we can be sure it is always correct.
 *
 * \todo
 * Offer a debug version that verifies that show is never set to zero.
 *
 * @param {number} show  If positive, show the darken page using
 *                       fadeIn(show), if negative, hide the darken
 *                       page using fadeOut(-show).
 * @param {boolean} wait  Whether this is opened to represent a wait.
 */
snapwebsites.Popup.prototype.darkenPage = function(show, wait)
{
    if(!this.darkenPagePopup_)
    {
        // create the full screen fixed div
        jQuery("<div id='darkenPage' class='zordered'></div>").appendTo("body");
        this.darkenPagePopup_ = jQuery("#darkenPage");
    }

    this.darkenPagePopup_.css("z-index", 1);
    this.darkenPagePopup_.css("z-index", jQuery("body").children().maxZIndex() + 1);
    this.darkenPagePopup_.toggleClass("wait", wait);

    if(show >= 0)
    {
        this.darkenPagePopup_.fadeIn(show);
    }
    else
    {
        this.darkenPagePopup_.fadeOut(-show);
    }
};


/** \brief Create a new popup "window".
 *
 * This function creates a \<div\> tag with the settings as defined
 * in the \c popup object. Once created, the popup needs to be shown
 * with the show() function (see below).
 *
 * All popups will have an identifier. If you do not specify one, then
 * the next available identifier will be used. That identifier starts
 * with the name "snapPopup" and ends with a unique number.
 *
 * Note that the identifier cannot be anything other than a non-empty
 * string or a non-zero number since those are viewed as false and thus
 * the automatically generated unique identifier will be used in that
 * case.
 *
 * To get rid of a popup, make sure to call the forget() function with
 * the same \p popup parameter. However, popups that can be shared or
 * reused should not be forgotten.
 *
 * The settings of a \p popup may include any one of the following
 * parameters:
 *
 * \li id -- the identifier of the DIV, that way you can retrieve it
 *           again later by using the same identifier; note that the
 *           jQuery object is also saved in your popup parameter
 * \li path -- the path to the content to load in the popup using an IFRAME
 *             default is no IFRAME
 * \li html -- the HTML to put in the popup if no path was defined
 * \li top -- the top position of the popup, default center vertically
 * \li left -- the left position of the popup, default center horizontally
 * \li width -- the width of the popup, default 400 pixels
 * \li height -- the height of the popup, default 300 pixels
 * \li darken -- whether to create (> 0) an overlay to darken the
 *               rest of the screen, the default is to not darken
 *               if undefined (0)
 * \li position -- we accept absolute (scrolls up and down) and fixed
 *                 (does not scroll and default)
 * \li title -- a title shown at the top, may be HTML, added inside
 *              a \<div\> tag (i.e. if you want an header tag, you have
 *              to include it in the title)
 * \li open -- if defined, a function called once the popup was opened
 *             (in case of an IFRAME it generally happens BEFORE the
 *             IFRAME gets loaded.)
 * \li show -- if defined, a function called just before the popup
 *             window gets shown
 * \li hide -- if defined, a function called just before the popup
 *             window gets hidden
 *
 * The function creates the following parameters in \p popup as required:
 *
 * \li id -- if the input \p popup does not already have a valid
 *           identifier defined, this function assigns a new one
 * \li widget -- the jQuery object referencing the popup is saved in
 *               this variable member
 *
 * @param {snapwebsites.Popup.PopupData} popup  The settings to create
 *                                              the popup.
 * @return {jQuery}
 */
snapwebsites.Popup.prototype.open = function(popup)
{
    var i, b, f, p, t, w, y, that = this;

    if(!popup.id)
    {
        // user did not specific an id, generate a new one
        ++this.uniqueID_;
        popup.id = "snapPopup" + this.uniqueID_;
    }
    // popup already exists?
    popup.widget = jQuery("#" + popup.id);
    if(popup.widget.length === 0)
    {
        jQuery("<div class='snap-popup zordered' id='" + popup.id + "' style='position:fixed;display:none;'>"
             + (popup.noClose ? "" : "<div class='close-popup'></div>")
             + "<div class='inside-popup'><div class='popup-title'></div><div class='popup-body'></div></div></div>")
                .appendTo("body");
        popup.widget = jQuery("#" + popup.id);
    }
    popup.widget.show();

    // here we cannot be sure whether the click() event was still here
    // so we unbind() it first
    popup.widget.children(".close-popup").unbind('click').click(function()
        {
            that.hide(popup);
        });

    i = popup.widget.children(".inside-popup");
    t = i.children(".popup-title");
    b = i.children(".popup-body");
    if(popup.title)
    {
        t.empty();
        t.prepend(popup.title);
    }
    if(popup.width)
    {
        popup.widget.css("width", popup.width);
    }
    else
    {
        popup.widget.css("width", 400);
    }
    if(popup.height)
    {
        popup.widget.css("height", popup.height);
    }
    else
    {
        popup.widget.css("height", 300);
    }
    if(popup.top)
    {
        popup.widget.css("top", popup.top);
    }
    else
    {
        // We use jQuery("body") instead of jQuery(window) because the
        // body may have padding which needs to be taken in account
        y = Math.floor((jQuery("body").height() - popup.widget.outerHeight()) / 2);
        if(y < 0)
        {
            y = 0;
        }
        popup.widget.css("top", y);
    }
    if(popup.left)
    {
        popup.widget.css("left", popup.left);
    }
    else
    {
        w = Math.floor((jQuery("body").width() - popup.widget.outerWidth()) / 2);
        if(w < 0)
        {
            w = 0;
        }
        popup.widget.css("left", w);
    }
    if(popup.position && popup.position === "absolute")
    {
        popup.widget.css("position", "absolute");
    }
    if(popup.path)
    {
        // make sure to mark this popup window as coming from an IFRAME
        p = popup.path + (popup.path.indexOf("?") === -1 ? "?" : "&") + "iframe=true";
        b.empty();
        b.append("<iframe class='popup-iframe' src='" + p + "' frameborder='0' marginheight='0' marginwidth='0'></iframe>");
        f = b.children(".popup-iframe");

        // 'b.width()' may return a jQuery object so we have to force the
        // type to a number to call f.attr().
        f.attr("width", snapwebsites.castToNumber(b.width(), "snapwebsites.Popup.open() retrieving popup body height"));

        // the height of the body matches the height of the IFRAME so we
        // cannot use it here

//console.log("WxH = "+b.width()+"x"+b.height()
//          +" widget top = "+popup.widget.offset().top
//          +", widget height = "+popup.widget.height()
//          +", body top = "+b.offset().top);

        f.attr("height", popup.widget.offset().top
                       + snapwebsites.castToNumber(popup.widget.height(), "snapwebsites.Popup.open() retrieving popup window height")
                       - b.offset().top);
    }
    else if(popup.html)
    {
        b.empty();
        b.append(popup.html);
    }
    //else -- leave the user's body alone (on a second call, it may not be
    //        empty and already have what the user expects.)
    if(popup.open)
    {
        popup.open(popup.widget);
    }
    popup.widget.hide();
    return popup.widget;
};


/** \brief Show the popup.
 *
 * This function shows a popup that was just created with the open()
 * function or got hidden with the hide() function.
 *
 * @param {snapwebsites.Popup.PopupData} popup  The settings to create
 *                                              the popup.
 */
snapwebsites.Popup.prototype.show = function(popup)
{
    if(popup.id)
    {
        popup.widget = jQuery("#" + popup.id);
        if(popup.widget && !popup.widget.is(":visible"))
        {
            popup.widget.fadeIn(
                {
                    duration: 150,
                    complete: popup.show
                });
            if(popup.darken > 0)
            {
                this.darkenPage(/** @type {number} */ (popup.darken), false);
            }
            popup.widget.css("z-index", 1);
            popup.widget.css("z-index", jQuery("body").children().maxZIndex() + 1);

            // user make have asked for one of the widget to get focus
            // in most cases this is used for anchors (forms have their
            // own auto-focus feature)
            popup.widget.find(".popup-auto-focus").focus();
        }
    }
};


/** \brief Hide the popup.
 *
 * This function hides a popup that was previously opened with the
 * open() function and shown with the show() function.
 *
 * This is often referenced as a soft-close. After this call the popup
 * is still available in the DOM, it is just hidden (display: none).
 *
 * If your popup gets hidden, then the hide() function gets called.
 * Note that the hide() function is called after the popup is completely
 * hidden so depending on the animation timing it may take more or less
 * time.
 *
 * Note that the hide() function is called if the popup is already
 * hidden to make sure that your code knows that it is indeed hidden.
 * However, if the popup object defines a beforeHide() function which
 * never calls the call back (the hideNow() function) then the
 * hide() function will never be called.
 *
 * @param {snapwebsites.Popup.PopupData} popup  The settings to create
 *                                              the popup.
 */
snapwebsites.Popup.prototype.hide = function(popup)
{
    var that = this;

    if(popup.id)
    {
        popup.widget = jQuery("#" + popup.id);
        if(popup.widget && popup.widget.is(":visible"))
        {
            if(popup.beforeHide)
            {
                // the beforeHide() function does not return immediately
                // as it may open a confirmation popup and thus need to
                // offer a callback to process the confirmation
                popup.hideNow = function() { that.hideNow_(popup); };
                popup.beforeHide(popup);
                return;
            }
            // no beforeHide(), immediate close
            that.hideNow_(popup);
            return;
        }
    }

    // it was already hidden, make sure the hide() function gets called
    if(popup.hide)
    {
        popup.hide();
    }
};


/** \brief Actual hides the popup.
 *
 * This function actually hides the Popup. It is separate from the hide()
 * function so one can make use of a beforeHide() which requires a callback
 * If you have a beforeHide() you must call the popup.hideNow() function
 * once your are done and if you want to proceed with hiding
 * the popup.
 *
 * @param {snapwebsites.Popup.PopupData} popup  The settings to create
 *                                              the popup.
 * @private
 */
snapwebsites.Popup.prototype.hideNow_ = function(popup)
{
    popup.widget.fadeOut(
        {
            duration: 150,
            complete: popup.hide
        });
    if(popup.darken > 0)
    {
        this.darkenPage(-popup.darken, false);
    }
};


/** \brief Remove a popup from the DOM.
 *
 * This function removes the popup from the DOM. It does not first
 * fade out the popup. This function immediately removes the elements
 * from the DOM. You may want to first close then forget.
 *
 * @param {snapwebsites.Popup.PopupData} popup  The settings to create
 *                                              the popup.
 */
snapwebsites.Popup.prototype.forget = function(popup)
{
    if(popup.widget)
    {
        popup.widget.remove();
        popup.widget = null;
    }
};


/** \brief Create a message box.
 *
 * This function is used to create an HTML message box. It expects a
 * structure that includes a message (in HTML) and any number of
 * buttons in an array. A click on any button closes the box and
 * calls your callback with the name of that button.
 *
 * A button has a label and a name.
 *
 * The callback is a function that should accept the name of the button
 * as its parameter. If you have a single button, it will generally not
 * be necessary to accept the name.
 *
 * The popup has a Close button which is always viewed as a button in
 * that dialog. When clicked, the callback function is called with the
 * string parameter set to "close_button".
 *
 * @param {snapwebsites.Popup.PopupMessage} msg  The message object with
 *    the message itself, buttons, and a callback function.
 */
snapwebsites.Popup.prototype.messageBox = function(msg)
{
    var popup = /** @type {snapwebsites.Popup.PopupData} */ ({}),
        that = this,
        key;

    popup.id = msg.id;
    popup.noClose = true;
    popup.top = msg.top > 0 ? msg.top : 10;
    if(msg.left > 0)
    {
        popup.left = msg.left;
    }
    popup.width = msg.width > 0 ? msg.width : 450;
    popup.height = msg.height > 0 ? msg.height : 250;
    popup.darken = 150;
    popup.title = msg.title;
    popup.html = "<div class=\"message-box\">" + msg.message + "</div><div class=\"message-box-buttons\">";
    for(key in msg.buttons)
    {
        if(msg.buttons.hasOwnProperty(key))
        {
            popup.html += "<a class=\"message-button\" name=\""
                    + msg.buttons[key].name
                    + "\" href=\"#\">" + msg.buttons[key].label + "</a>";
        }
    }
    popup.html += "</div>";

    this.open(popup);
    this.show(popup);

    popup.widget.find(".message-button").click(function()
        {
            var button = jQuery(this),
                name = /** @type {string} */ (button.attr("name"));

            // always hide that popup once a button was clicked
            that.hide(popup);
            msg.callback(name);
        });
};


// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.PopupInstance = new snapwebsites.Popup();
    }
);

// vim: ts=4 sw=4 et
