/*
 * Name: popup
 * Version: 0.0.18
 * Browsers: all
 * Copyright: Copyright 2014 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: output (0.0.5)
 * License: GPL 2.0
 */

/** \brief Snap Popup.
 *
 * This class initializes and handles the different popup features.
 *
 * \note
 * The Snap! Output handles a popup capability that can be used to open
 * any number of popups.
 *
 * @constructor
 */
snapwebsites.Popup = function()
{
};


snapwebsites.Popup.prototype = {
    _uniqueID: 0,
    _darkenPagePopup: null,

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
     * \param[in] show  If positive, show the darken page using fadeIn(show),
     *                  if negative, hide the darken page using fadeOut(-show).
     */
    darkenPage: function(show)
    {
        var html;

        if(!this._darkenPagePopup)
        {
            // create the full screen fixed div
            jQuery("<div id='darkenPage'></div>").appendTo("body");
            this._darkenPagePopup = jQuery("#darkenPage");
        }

        this._darkenPagePopup.css("z-index", 1);
        this._darkenPagePopup.css("z-index", jQuery("body").children().maxZIndex() + 1);

        if(show > 0)
        {
            this._darkenPagePopup.fadeIn(show);
        }
        else
        {
            this._darkenPagePopup.fadeOut(-show);
        }
    },

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
     *           identifier defined, it creates a new one
     * \li widget -- the jQuery object referencing the popup is saved in
     *               this variable member
     *
     * \param[in,out] popup  The settings to create the popup.
     */
    open: function(popup)
    {
        var i, b, f, t;

        if(!popup.id)
        {
            // user did not specific an id, generate a new one
            ++this._uniqueID;
            popup.id = "snapPopup" + this._uniqueID;
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
            snapwebsites.Popup.hide(popup);
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
            popup.widget.css("top", (jQuery("body").height() - popup.widget.height()) / 2);
        }
        if(popup.left)
        {
            popup.widget.css("left", popup.left);
        }
        else
        {
            popup.widget.css("left", (jQuery("body").width() - popup.widget.width()) / 2);
        }
        if(popup.position && popup.position == "absolute")
        {
            popup.widget.css("position", "absolute");
        }
        if(popup.path)
        {
            b.empty();
            b.append("<iframe class='popup-iframe' src='" + popup.path + "' frameborder='0' marginleft='0' marginright='0'></iframe>");
            f = b.children(".popup-iframe");
console.log("WxH = "+b.width()+"x"+b.height());
            f.attr("width", b.width());
            // the height of the body matches the height of the IFRAME so we
            // cannot use it here
            f.attr("height", popup.widget.offset().top + popup.widget.height() - b.offset().top);
        }
        if(popup.open)
        {
            popup.open(popup.widget);
        }
        popup.widget.hide();
        return popup.widget;
    },

    /** \brief Show the popup.
     *
     * This function shows a popup that was just created with the open()
     * function or got hidden with the hide() function.
     *
     * \param[in] popup  The popup to show.
     */
    show: function(popup)
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
    },

    /** \brief Hide the popup.
     *
     * This function hides a popup that was previously opened with the
     * open() function and shown with the show() function.
     *
     * This is often referenced as a soft-close. After this call the popup
     * is still available in the DOM, it is just hidden (display: none).
     *
     * \param[in] popup  The popup to hide.
     */
    hide: function(popup)
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
    },

    /** \brief Remove a popup from the DOM.
     *
     * This function removes the popup from the DOM. It doesn't first
     * fade out the popup. This function immediately removes the elements
     * from the DOM.
     *
     * \param[in,out] popup  The popup object to forget.
     */
    forget: function(popup)
    {
        if(popup.widget)
        {
            popup.widget.remove();
            popup.widget = null;
        }
    },

    /** \brief Initialize the popup library.
     *
     * This function initializes the popup library.
     */
    init: function()
    {
    }
};


// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.Popup = new snapwebsites.Popup();
        snapwebsites.Popup.init();
    }
);

// vim: ts=4 sw=4 et
