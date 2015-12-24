/** @preserve
 * Name: plugin-selection
 * Version: 0.0.1.8
 * Browsers: all
 * Copyright: Copyright 2014-2015 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: output (0.1.5)
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

/*jslint nomen: true, todo: true, devel: true */
/*global jQuery: false, Uint8Array: true */



/** \brief Plugin Selection constructor.
 *
 * The Plugin Selection is a singleton handling the Plugin Selection
 * window.
 *
 * @return {snapwebsites.PluginSelection}  The newly created object.
 *
 * @constructor
 * @struct
 * @extends {snapwebsites.ServerAccessCallbacks}
 */
snapwebsites.PluginSelection = function()
{
    this.initPluginSelections_();

    return this;
};


/** \brief PluginSelection derives from ServerAccessCallbacks.
 *
 * The PluginSelection class inherit the ServerAccessCallbacks class
 * so it knows when an AJAX request succeeded.
 */
snapwebsites.inherits(snapwebsites.PluginSelection, snapwebsites.ServerAccessCallbacks);


/** \brief The PluginSelection instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * \@type {snapwebsites.PluginSelection}
 */
snapwebsites.PluginSelectionInstance = null; // static


/** \brief The toolbar object.
 *
 * This variable represents the toolbar used by the editor.
 *
 * Note that this is the toolbar object, not the DOM. The DOM is
 * defined within the toolbar object and is considered private.
 *
 * @type {snapwebsites.ServerAccess}
 * @private
 */
snapwebsites.PluginSelection.prototype.serverAccess_ = null;


/** \brief The Install or Remove button that was last clicked.
 *
 * This parameter holds the last Install or Remove button that
 * was last clicked.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.PluginSelection.prototype.clickedButton_ = null;


/** \brief Attach to the buttons.
 *
 * This function attaches the PluginSelection instance to the form buttons if
 * any.
 *
 * The function is expected to be called only once by the constructor.
 *
 * @private
 */
snapwebsites.PluginSelection.prototype.initPluginSelections_ = function()
{
    var that = this;

    // The "Install" buttons
    //
    jQuery(".button.install")
        .makeButton()
        .click(function(e){
            var plugin_name;

            e.preventDefault();
            e.stopPropagation();

            if(that.clickedButton_)
            {
                // already working
                return;
            }

            that.clickedButton_ = jQuery(this);
            if(that.clickedButton_.hasClass("disabled"))
            {
                // disabled means no reaction
                that.clickedButton_ = null;
                return;
            }

            snapwebsites.PopupInstance.darkenPage(0, true);

            plugin_name = that.clickedButton_.data("plugin-name");

            // user clicked the "Install" button, send the info to the
            // server with AJAX...
            //
            if(!that.serverAccess_)
            {
                that.serverAccess_ = new snapwebsites.ServerAccess(that);
            }
            that.serverAccess_.setURI("/admin/plugins/install/" + plugin_name);
            that.serverAccess_.send(e);
        });

    // The "Remove" buttons
    //
    jQuery(".button.remove")
        .makeButton()
        .click(function(e){
            var plugin_name;

            e.preventDefault();
            e.stopPropagation();

            if(that.clickedButton_)
            {
                // already working
                return;
            }

            that.clickedButton_ = jQuery(this);
            if(that.clickedButton_.hasClass("disabled"))
            {
                // disabled means no reaction
                that.clickedButton_ = null;
                return;
            }

            snapwebsites.PopupInstance.darkenPage(0, true);

            plugin_name = that.clickedButton_.data("plugin-name");

            // user clicked the "Remove" button, send the info to the
            // server with AJAX...
            //
            if(!that.serverAccess_)
            {
                that.serverAccess_ = new snapwebsites.ServerAccess(that);
            }
            that.serverAccess_.setURI("/admin/plugins/remove/" + plugin_name);
            that.serverAccess_.send(e);
        });

    // The "Settings" buttons
    //
    jQuery(".button.settings")
        .makeButton()
        .click(function(e){
            var this_button = jQuery(this);

            if(this_button.hasClass("disabled"))
            {
                // disabled means no reaction
                e.preventDefault();
                e.stopPropagation();

                return;
            }
        });
};


/** \brief Function called on AJAX success.
 *
 * This function is called if the install or remove of a plugin succeeded.
 *
 * Here we make sure to change the interface accordingly.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.PluginSelection.prototype.serverAccessSuccess = function(result) // virtual
{
    snapwebsites.PluginSelection.superClass_.serverAccessSuccess.call(this, result);

    if(this.clickedButton_)
    {
        this.clickedButton_.addClass("disabled");
        if(this.clickedButton_.hasClass("install"))
        {
            this.clickedButton_.parent().parent().find(".remove").removeClass("disabled");
            this.clickedButton_.parent().parent().find("a.settings").removeClass("disabled");
        }
        else if(this.clickedButton_.hasClass("remove"))
        {
            this.clickedButton_.parent().parent().find(".install").removeClass("disabled");
            this.clickedButton_.parent().parent().find(".settings").addClass("disabled");
        }
    }
};


/** \brief Function called on AJAX success.
 *
 * This function is called if the install or remove of a plugin succeeded.
 *
 * Here we make sure to change the interface accordingly.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.PluginSelection.prototype.serverAccessComplete = function(result) // virtual
{
    result.undarken = snapwebsites.ServerAccessCallbacks.UNDARKEN_ALWAYS;
    snapwebsites.PluginSelection.superClass_.serverAccessComplete.call(this, result);
    this.clickedButton_ = null;
};



// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.PluginSelectionInstance = new snapwebsites.PluginSelection();
    }
);

// vim: ts=4 sw=4 et
