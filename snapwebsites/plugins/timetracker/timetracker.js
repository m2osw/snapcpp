/** @preserve
 * Name: timetracker
 * Version: 0.0.1.3
 * Browsers: all
 * Copyright: Copyright 2014-2016 (c) Made to Order Software Corporation  All rights reverved.
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
 * @return {snapwebsites.TimeTracker}  The newly created object.
 *
 * @constructor
 * @struct
 * @extends {snapwebsites.ServerAccessCallbacks}
 */
snapwebsites.TimeTracker = function()
{
    this.initTimeTrackerMainPage_();

    return this;
};


/** \brief TimeTracker derives from ServerAccessCallbacks.
 *
 * The TimeTracker class inherit the ServerAccessCallbacks class
 * so it knows when an AJAX request succeeded.
 */
snapwebsites.inherits(snapwebsites.TimeTracker, snapwebsites.ServerAccessCallbacks);


/** \brief The TimeTracker instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * \@type {snapwebsites.TimeTracker}
 */
snapwebsites.TimeTrackerInstance = null; // static


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
snapwebsites.TimeTracker.prototype.serverAccess_ = null;


/** \brief The Install or Remove button that was last clicked.
 *
 * This parameter holds the last Install or Remove button that
 * was last clicked.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.TimeTracker.prototype.clickedButton_ = null;


/** \brief The Time Tracker "Add User" popup window.
 *
 * This variable is used to describe the Time Tracker popup used when
 * the administrator wants to add a user to the Time Tracker system.
 *
 * \note
 * We have exactly ONE instance of this variable (i.e. is is static).
 * This means we cannot open two such popups simultaneously.
 *
 * @type {Object}
 * @private
 */
snapwebsites.TimeTracker.addUserPopup_ = // static
{
    id: "timetracker-popup",
    path: "/admin/settings/timetracker/add-user",
    darken: 150,
    width: 500
};


/** \brief Attach to the buttons.
 *
 * This function attaches the TimeTracker instance to the main page
 * buttons if any.
 *
 * The function is expected to be called only once by the constructor.
 *
 * @private
 */
snapwebsites.TimeTracker.prototype.initTimeTrackerMainPage_ = function()
{
    var that = this;

    // The "Add Self" buttons
    //
    jQuery(".button.time-tracker.add-self")
        .makeButton()
        .click(function(e){
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

            // user clicked the "Add Self" button, send the info to the
            // server with AJAX...
            //
            if(!that.serverAccess_)
            {
                that.serverAccess_ = new snapwebsites.ServerAccess(that);
            }
            that.serverAccess_.setURI("/timetracker");
            that.serverAccess_.setData({
                        action: "add-self"
                    });
            that.serverAccess_.send(e);
        });

    // The "Add User" buttons
    //
    jQuery(".button.time-tracker.add-user")
        .makeButton()
        .click(function(e){
            e.preventDefault();
            e.stopPropagation();

            // open and show popup where we can select a new user
            // and click "Add User" to actually add it
            //
            snapwebsites.PopupInstance.open(snapwebsites.TimeTracker.addUserPopup_);
            snapwebsites.PopupInstance.show(snapwebsites.TimeTracker.addUserPopup_);
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
snapwebsites.TimeTracker.prototype.serverAccessSuccess = function(result) // virtual
{
    snapwebsites.TimeTracker.superClass_.serverAccessSuccess.call(this, result);
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
snapwebsites.TimeTracker.prototype.serverAccessComplete = function(result) // virtual
{
    result.undarken = snapwebsites.ServerAccessCallbacks.UNDARKEN_ALWAYS;
    snapwebsites.TimeTracker.superClass_.serverAccessComplete.call(this, result);
    this.clickedButton_ = null;
};



// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.TimeTrackerInstance = new snapwebsites.TimeTracker();
    }
);

// vim: ts=4 sw=4 et
