/** @preserve
 * Name: popup
 * Version: 0.1.0.3
 * Browsers: all
 * Copyright: Copyright 2014 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: output (0.0.5)
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
 */
snapwebsites.Popup.prototype.darkenPage = function(show)
{
    if(!this.darkenPagePopup_)
    {
        // create the full screen fixed div
        jQuery("<div id='darkenPage'></div>").appendTo("body");
        this.darkenPagePopup_ = jQuery("#darkenPage");
    }

    this.darkenPagePopup_.css("z-index", 1);
    this.darkenPagePopup_.css("z-index", jQuery("body").children().maxZIndex() + 1);

    if(show > 0)
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
 *                 (doesn't scroll and default)
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
 * @param {{id: ?string, path: string, html: string,
 *          top: number, left: number, width: number, height: number,
 *          darken: number, position: string, title: string,
 *          open: function(Object), show: function(Object), hide: function(Object),
 *          widget: Object}} popup  The settings to create the popup.
 * @return {jQuery}
 */
snapwebsites.Popup.prototype.open = function(popup)
{
    var i, b, f, t, w, y;

    if(!popup.id)
    {
        // user did not specific an id, generate a new one
        ++this.uniqueID_;
        popup.id = "snapPopup" + this.uniqueID_;
    }
    // popup already exists?
    popup.widget = jQuery("#" + popup.id);
    if(popup.widget.length == 0)
    {
        jQuery("<div class='snap-popup' id='" + popup.id + "' style='position:fixed;display:none;'><div class='close-popup'></div><div class='inside-popup'><div class='popup-title'></div><div class='popup-body'></div></div></div>")
                .appendTo("body");
        popup.widget = jQuery("#" + popup.id);
    }
    popup.widget.show();
    popup.widget.children(".close-popup").click(function(){
        snapwebsites.PopupInstance.hide(popup);
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
        y = Math.floor((jQuery("body").height() - popup.widget.height()) / 2);
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
        popup.widget.css("left", Math.floor((jQuery("body").width() - popup.widget.width()) / 2));
    }
    if(popup.position && popup.position == "absolute")
    {
        popup.widget.css("position", "absolute");
    }
    if(popup.path)
    {
        b.empty();
        b.append("<iframe class='popup-iframe' src='" + popup.path + "' frameborder='0' marginheight='0' marginwidth='0'></iframe>");
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
 * @param {{id: ?string, path: string, html: string,
 *          top: number, left: number, width: number, height: number,
 *          darken: number, position: string, title: string,
 *          open: function(Object), show: function(Object), hide: function(Object),
 *          widget: Object}} popup  The popup to show.
 */
snapwebsites.Popup.prototype.show = function(popup)
{
    if(popup.id && popup.widget)
    {
        popup.widget = jQuery("#" + popup.id);
        if(popup.widget && !popup.widget.is(":visible"))
        {
            popup.widget.fadeIn(150);
            if(popup.show)
            {
                popup.show(popup.widget);
            }
            if(popup.darken > 0)
            {
                this.darkenPage(popup.darken);
            }
            popup.widget.css("z-index", 1);
            popup.widget.css("z-index", jQuery("body").children().maxZIndex() + 1);
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
 * @param {{id: ?string, path: string, html: string,
 *          top: number, left: number, width: number, height: number,
 *          darken: number, position: string, title: string,
 *          open: function(Object), show: function(Object), hide: function(Object),
 *          widget: Object}} popup  The popup to hide.
 */
snapwebsites.Popup.prototype.hide = function(popup)
{
    if(popup.id && popup.widget)
    {
        popup.widget = jQuery("#" + popup.id);
        if(popup.widget && popup.widget.is(":visible"))
        {
            popup.widget.fadeOut(150);
            if(popup.hide)
            {
                popup.hide(popup.widget);
            }
            if(popup.darken > 0)
            {
                this.darkenPage(-popup.darken);
            }
        }
    }
};


/** \brief Remove a popup from the DOM.
 *
 * This function removes the popup from the DOM. It doesn't first
 * fade out the popup. This function immediately removes the elements
 * from the DOM.
 *
 * @param {{id: ?string, path: string, html: string,
 *          top: number, left: number, width: number, height: number,
 *          darken: number, position: string, title: string,
 *          open: function(Object), show: function(Object), hide: function(Object),
 *          widget: Object}} popup  The popup object to forget.
 */
snapwebsites.Popup.prototype.forget = function(popup)
{
    if(popup.widget)
    {
        popup.widget.remove();
        popup.widget = null;
    }
};


// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.PopupInstance = new snapwebsites.Popup();
    }
);

// vim: ts=4 sw=4 et
