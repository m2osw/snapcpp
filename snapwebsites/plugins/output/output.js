/** @preserve
 * Name: output
 * Version: 0.0.5
 * Browsers: all
 * Copyright: Copyright 2014 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: jquery-extensions (1.0.1)
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
// ==/ClosureCompiler==
//



/** \brief Defines the snapwebsites namespace in the JavaScript environment.
 *
 * All the JavaScript functions defined by Snap! plugins are defined inside
 * the snapwebsites namespace. For example, the WYSIWYG Snap! editor is
 * defined as:
 *
 * \code
 * snapwebsites.Editor
 * \endcode
 *
 * \note
 * Technically, this is an object.
 *
 * @type {Object}
 */
var snapwebsites = {};



/** \brief Snap Output Manipulations.
 *
 * This class initializes and handles the different output objects.
 *
 * \note
 * The Snap! Output is a singleton and should never be created by you. It
 * gets initialized automatically when this output.js file gets included.
 *
 * @constructor
 * @struct
 */
snapwebsites.Output = function()
{
    this._handleMessages();
    return this;
};


/** \brief Base definitions in the snapwebsites environment.
 *
 * The base prototype of the snapwebsites JavaScript environment is defined
 * here. These are functions used by all the code we provide as part of
 * Snap! C++.
 *
 * @struct
 */
snapwebsites.Output.prototype =
{
    /** \brief The constructor of this object.
     *
     * Make sure to declare the constructor for proper inheritance
     * support.
     *
     * @type {function()}
     */
    constructor: snapwebsites.Output,

    /** \brief Helper function: generate hexadecimal number.
     *
     * This function transform byte \p c in a hexadecimal number of
     * exactly two digits.
     *
     * Note that \p c can be larger than a byte, only it should probably
     * not be negative.
     *
     * @param {number} c  The byte to transform (expected to be between 0 and 255)
     *
     * @return {string}  The hexadecimal representation of the number.
     */
    char2hex: function(c)
    {
        var a, b;

        a = c & 15;
        b = (c >> 4) & 15;
        return String.fromCharCode(b + (b >= 10 ? 55 : 48))
             + String.fromCharCode(a + (a >= 10 ? 55 : 48));
    },

    /** \brief Internal function used to display the error messages.
     *
     * This function is used to display the error messages that occured
     * "recently" (in most cases, this last access, or the one before.)
     *
     * This function is called by the init() function and shows the
     * messages if any were added to the DOM.
     *
     * \note
     * This is here because the messages plugin cannot handle the output
     * of its own messages (it is too low level a plugin.)
     */
    _handleMessages: function()
    {
        jQuery("div.user-messages")
            .each(function(){
                var z;

                z = jQuery("div.zordered").maxZIndex() + 1;
                jQuery(this).css("z-index", z);
            })
            .delay(250).fadeIn(300) // put a little delay so we see the fadeIn(), eventually
            .click(function(){jQuery(this).fadeOut(300);});
    }
};

// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.OutputInstance = new snapwebsites.Output();
    }
);

// vim: ts=4 sw=4 et
