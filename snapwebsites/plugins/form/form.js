/** @preserve
 * Name: form
 * Version: 0.0.1.13
 * Browsers: all
 * Depends: output (>= 0.0.5), jquery-extensions (>= 1.0.1)
 * Copyright: Copyright 2012-2014 (c) Made to Order Software Corporation  All rights reverved.
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



/** \brief Snap Form Manipulations.
 *
 * This class initializes and handles the different form inputs.
 *
 * \note
 * The Snap! Form is a singleton and should never be created by you. It
 * gets initialized automatically when this form.js file gets included.
 *
 * @return {!snapwebsites.Form}  This object reference.
 *
 * @constructor
 * @struct
 */
snapwebsites.Form = function()
{
    var that = this;

    jQuery(":any(input[type='text'] input[type='password']):not([data-background-value=''])")
        .focus(function()
            {
                that._focus(this);
            })
        .change(function()
            {
                that._change(this);
            })
        .blur(function()
            {
                that._blur(this);
            })
        .each(function(i, e)
            {
                that._blur(e);
            });

    return this;
};


/** \brief Mark Form as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.Form);


/** \brief This function is called whenever a widget gets the focus.
 *
 * The function makes sure that the background value is removed
 * when the widget gets focused, if the background value was there.
 *
 * Also the function sets the type attribute to "password" on
 * password widgets whenever the background value is removed.
 *
 * @param {Element} widget  The widget that was just focused.
 */
snapwebsites.Form.prototype._focus = function(widget)
{
    var w = jQuery(widget);

    // TODO: ameliorate with 2 colors until change() happens
    if(w.val() === w.data('background-value')
    && !w.data('editor'))
    {
        w.val('');
    }

    // we force the removal of the class
    // (just in case something else went wrong)
    w.removeClass('input-with-background-value');

    if(w.hasClass('password-input'))
    {
        w.attr('type', 'password');
    }
};


/** \brief This function is called whenever a widget is modified.
 *
 * The function is called whenever the widget is changed in some
 * way. The function is used to set the value of the edited
 * data so we know whether to show the background value when
 * the widget gets blurred.
 *
 * @param {Element} widget  The widget that just changed.
 */
snapwebsites.Form.prototype._change = function(widget)
{
    var w = jQuery(widget);

    w.data("edited", w.val() != "");
};


/** \brief This function is called whenever a widget is blurred.
 *
 * The function is an event executed when the blur event is received.
 * The function restores the background value if there is one and
 * the widget is empty.
 *
 * Note that in case of a password the type of the widget is changed
 * to "text" when it is empty and we want to show the background
 * value (otherwise the background value would show up as stars.)
 *
 * @param {Element} widget  The widget that just lost focus.
 */
snapwebsites.Form.prototype._blur = function(widget)
{
    var w = jQuery(widget);

    if(!w.is(":focus"))
    {
        if(w.val().length === 0)
        {
            w.val( /** @type string */ (w.data("background-value")))
                .addClass('input-with-background-value');

            if(w.hasClass('password-input'))
            {
                w.attr('type', 'text');
            }
        }
    }
};


/** \brief Get the current value of the widget.
 *
 * It is important for you to use this function to retrieve the
 * value of a widget because if it is the background value, this
 * function makes sure to return the empty string.
 *
 * @param {Element} widget  The widget element from which you want to
 *                          retrieve the value.
 *
 * @return {string}  The current value as a string.
 */
snapwebsites.Form.prototype.getVal = function(widget)
{
    var w = jQuery(widget);

    if(w.hasClass('input-with-background-value'))
    {
        return '';
    }
    return snapwebsites.Output.castToString(w.val());
};


/** \brief Set the value of the widget.
 *
 * It is important for you to use this function to set the value
 * of a widget because if it is the empty string, the background
 * value is used instead.
 *
 * @param {Element} widget  The widget element from which you want to
 *                          retrieve the value.
 * @param {string|number} value  The value of the text to set the
 *                               input widget.
 */
snapwebsites.Form.prototype.setVal = function(widget, value)
{
    var w = jQuery(widget);

    if(value || w.is(":focus"))
    {
        if(w.hasClass('password-input'))
        {
            w.attr('type', 'password');
        }
        w.val( /** @type {string} */ (value))
            .removeClass('input-with-background-value');
    }
    else
    {
        w.val( /** @type string */ (w.data("background-value")))
            .addClass('input-with-background-value');
        if(w.hasClass('password-input'))
        {
            w.attr('type', 'text');
        }
    }
};

// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.FormInstance = new snapwebsites.Form();
    }
);
// vim: ts=4 sw=4 et
