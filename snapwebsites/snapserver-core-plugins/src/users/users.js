/** @preserve
 * Name: users
 * Version: 0.0.1.11
 * Browsers: all
 * Depends: output (>= 0.1.5)
 * Copyright: Copyright 2012-2016 (c) Made to Order Software Corporation  All rights reverved.
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
// @externs plugins/users/externs/users-externs.js
// @js plugins/output/output.js
// @js plugins/output/popup.js
// @js plugins/server_access/server-access.js
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false */



/** \brief User session timeout.
 *
 * This class initialize a timer that allows the client (browser) to
 * "close" the window on a session timeout. That way the user does
 * not start using his account when it is already timed out (especially
 * by starting to type new data and not being able to save it.)
 *
 * The timer can be set to display a warning to the end user. By default
 * it just resets the current page if it is public, otherwise it sends
 * the user to another public page such as the home page.
 *
 * \todo
 * Add a timer that goes and check the server once in a while to make
 * sure that the session is not killed in between. Like for instance
 * once every 4h or so. Only that check must be such that we avoid
 * updating the dates in the existing session.
 *
 * @return {!snapwebsites.Users}  This object reference.
 *
 * @constructor
 * @struct
 */
snapwebsites.Users = function()
{
    // start the auto-logout timer
    //
    this.startAutoLogout();

    // see todo -- add another timer to periodically check the session
    //             on the server because it could get obliterated...

    return this;
};


/** \brief Mark Users as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.Users);


/** \brief The Users instance.
 *
 * This class is a singleton and as such it makes use of a static
 * reference to itself. It gets created on load.
 *
 * @type {snapwebsites.Users}
 */
snapwebsites.UsersInstance = null; // static


/** \brief The variable holding the timeout identifier.
 *
 * This variable holds the timer identifier which is eventually used to
 * cancel the timer when it was not triggered before some other event.
 *
 * @type {number}
 *
 * @private
 */
snapwebsites.Users.prototype.auto_logout_id_ = NaN;


/** \brief Start the timer of the auto-logout feature.
 *
 * This function starts or restarts the auto-logout timer.
 *
 * The function may call itself in case the timeout is too far in the
 * future (more than 24.8 days!) so as to make sure we time out on the
 * day we have to time out.
 *
 * In most cases, you call this function after you called the
 * preventAutoLogout()
 *
 * \note
 * Yes. We expect the user to close his browsers way before this timer
 * happens. But we are thourough...
 *
 * \sa preventAutoLogout()
 */
snapwebsites.Users.prototype.startAutoLogout = function()
{
    // TODO: remove the 20 seconds once we have a valid query string
    //       option to force a log out of the user on a timeout (actually
    //       we may even want to reduce the timeout by 1 minute or so...)
    //
    // we add 20 seconds to the time limit to make sure that we
    // timeout AFTER the session is dead;
    //
    var delay = (users__session_time_limit + 20) * 1000 - Date.now(),
        that = this;

    // we may get called again before the existing timeout was triggered
    // so if the ID is not NaN, we call the clearTimeout() on it
    //
    if(!isNaN(this.auto_logout_id_))
    {
        clearTimeout(this.auto_logout_id_);
        this.auto_logout_id_ = NaN;
    }

    // A JavaScript timer is limited to 2^31 - 1 which is about 24.8 days.
    // So here we want to make sure that we do not break the limit if
    // that ever happens.
    //
    // Note: I checked the Firefox implementation of window.setTimeout()
    //       and it takes the interval at he time of the call to calculate
    //       the date when it will be triggered, so even if 24.8 days later
    //       it will still be correct and execute within 50ms or so
    //
    if(delay > 0x7FFFFFFF)
    {
        this.auto_logout_id_ = setTimeout(function()
            {
                that.startAutoLogout();
            },
            0x7FFFFFFF);
    }
    else
    {
        this.auto_logout_id_ = setTimeout(function()
            {
                that.autoLogout_();
            },
            delay);
    }
};


/** \brief Stop the auto-logout timer.
 *
 * Once in a while, it may be useful (probably not, though) to
 * stop the auto-logout timer. This could help in avoiding the
 * redirect that happens on the timeout while we do some other
 * work.
 *
 * To restart the auto-logout timer, use the restartTimer()
 * function.
 *
 * \sa restartTimer()
 */
snapwebsites.Users.prototype.preventAutoLogout = function()
{
    if(!isNaN(this.auto_logout_id_))
    {
        clearTimeout(this.auto_logout_id_);
        this.auto_logout_id_ = NaN;
    }
};


/** \brief Function called once the auto-logout times out.
 *
 * After a certain amount of time, a user login session times out.
 * When that happens we can do one of several things
 *
 * \warning
 * Although we do not yet have it at the time of writing, we
 * expect that the logout timer will fire way after the last
 * auto-save of whatever changes the user has made so far.
 * This means that we do not have to worry about obliterating
 * the user's data.
 *
 * @private
 */
snapwebsites.Users.prototype.autoLogout_ = function()
{
    var doc = document,
        redirect_uri = doc.location.toString();

    // note: since the timer was triggered, the ID is not valid anymore
    this.auto_logout_id_ = NaN;

    // reload the page without the 'edit' action (we probably should
    // remove any type of action, not just the edit action!)
    // since the user is now logged out, then the reload will send
    // him to another page if this page requires the user to be logged
    // in to view this page
    //
    redirect_uri = redirect_uri.replace(/\?a=edit$/, "")
                               .replace(/\?a=edit&/, "?")
                               .replace(/&a=edit&/, "&");
    redirect_uri = snapwebsites.ServerAccess.appendQueryString(redirect_uri, { hit: "transparent" });
    doc.location = redirect_uri;
};



// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.UsersInstance = new snapwebsites.Users();
    }
);
// vim: ts=4 sw=4 et
