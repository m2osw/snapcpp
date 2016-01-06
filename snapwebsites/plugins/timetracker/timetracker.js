/** @preserve
 * Name: timetracker
 * Version: 0.0.1.27
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



/** \brief Time Tracker constructor.
 *
 * The Time Tracker is a singleton handling the Time Tracker
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
    this.serverAccess_ = new snapwebsites.ServerAccess(this);
    this.initTimeTrackerMainPage_();
    this.initTimeTrackerCalendar_();

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
            that.serverAccess_.setURI("/timetracker");
            that.serverAccess_.setData({
                        operation: "add-self"
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


/** \brief Attach to the Calendar buttons.
 *
 * This function attaches the TimeTracker instance to the calendar
 * buttons. This includes changing month or year and creating/editing a
 * day.
 *
 * The function is expected to be called once by the constructor and
 * then each time a new calendar is received.
 *
 * @private
 */
snapwebsites.TimeTracker.prototype.initTimeTrackerCalendar_ = function()
{
    var that = this;

    // The "Previous Month" button
    //
    jQuery("div.calendar table.calendar-table .buttons a[href='#previous-month']")
        .makeButton()
        .click(function(e){
            that.nextMonth(e, jQuery(this), -1);
        });

    // The "Next Month" button
    //
    jQuery("div.calendar table.calendar-table .buttons a[href='#next-month']")
        .makeButton()
        .click(function(e){
            that.nextMonth(e, jQuery(this), 1);
        });

    // The "Previous Year" button
    //
    jQuery("div.calendar table.calendar-table .buttons a[href='#previous-year']")
        .makeButton()
        .click(function(e){
            that.nextMonth(e, jQuery(this), -12);
        });

    // The "Next Year" button
    //
    jQuery("div.calendar table.calendar-table .buttons a[href='#next-year']")
        .makeButton()
        .click(function(e){
            that.nextMonth(e, jQuery(this), 12);
        });
};


/** \brief Go to the next month.
 *
 * This function moves the calendar to the next, previous month or year.
 *
 * The offset can only be set to one of the supported values which
 * currently are:
 *
 * \li -1 -- previous month
 * \li 1 -- next month
 * \li -12 -- previous year
 * \li 12 -- next year
 * \li 0 -- reset to current month
 *
 * @param {Event} e  The event that triggered this move or null.
 * @param {jQuery} button  The button just clicked as a jQuery object.
 * @param {integer} offset  The offset to be added to the month.
 */
snapwebsites.TimeTracker.prototype.nextMonth = function(e, button, offset)
{
    var year = this.getYear_(),
        month = this.getMonth_();

    if(e)
    {
        e.preventDefault();
        e.stopPropagation();
    }

    if(this.clickedButton_)
    {
        // already working
        return;
    }

    this.clickedButton_ = jQuery(e.target);
    if(this.clickedButton_.hasClass("disabled"))
    {
        // disabled means no reaction
        this.clickedButton_ = null;
        return;
    }

    snapwebsites.PopupInstance.darkenPage(0, true);

    switch(offset)
    {
    case 0:
        // server is in charge of calculating the current month/year in this case
        month = 0;
        year = 0;
        break;

    case -1:
        if(month != 1)
        {
            --month;
        }
        else
        {
            month = 12;
            --year;
        }
        break;

    case 1:
        if(month != 12)
        {
            ++month;
        }
        else
        {
            month = 1;
            ++year;
        }
        break;

    case -12:
        --year;
        break;

    case 12:
        ++year;
        break;

    default:
        throw new Error("invalid offset (" + offset + ") in timeTracker.nextMonth().");

    }

    // user clicked a next/previous month/year, request the new calendar
    //
    this.serverAccess_.setURI("/timetracker");
    this.serverAccess_.setData({
                operation: "calendar",
                year: year,
                month: month
            });
    this.serverAccess_.send(e);
};


/** \brief Retrieve the current calendar year.
 *
 * This function returns the current calendar year. This we search starting
 * on the div that encompasses the table representing the calendar, it
 * should always get updated whenever we go to a different month/year.
 *
 * @return {integer}  The year the calendar displays.
 * @private
 */
snapwebsites.TimeTracker.prototype.getYear_ = function()
{
    return jQuery("div.calendar table.calendar-table th.month").data("year");
};


/** \brief Retrieve the current calendar month.
 *
 * This function returns the current calendar month. This we search starting
 * on the div that encompasses the table representing the calendar, it
 * should always get updated whenever we go to a different month/year.
 *
 * @return {integer}  The month the calendar displays.
 * @private
 */
snapwebsites.TimeTracker.prototype.getMonth_ = function()
{
    return jQuery("div.calendar table.calendar-table th.month").data("month");
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
    var button_name = this.clickedButton_.get(0).hash,
        result_xml = result.jqxhr.responseXML,
        new_calendar;

    snapwebsites.TimeTracker.superClass_.serverAccessSuccess.call(this, result);

//console.log("hash is [" + button_name + "]");
    if(button_name == "#previous-month"
    || button_name == "#next-month"
    || button_name == "#previous-year"
    || button_name == "#next-year")
    {
        // on success we get a new calendar, save it
        //
        a = jQuery("data[name='calendar']", result_xml).text();
        new_calendar = jQuery.parseHTML(a);
        //new_calendar = jQuery.parseHTML(jQuery("data[name='calendar']", result_xml));
        jQuery("div.calendar").html(jQuery("div.calendar table.calendar-table", new_calendar));
        this.initTimeTrackerCalendar_();
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
