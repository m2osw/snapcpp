/** @preserve
 * Version: 0.0.4
 * Browsers: all
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
snapwebsites.List = function()
{
    this.constructor = snapwebsites.List;

    return this;
};


/** \brief The list functions.
 *
 * @struct
 */
snapwebsites.List.prototype =
{
};

// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.ListInstance = new snapwebsites.List();
    }
);

// vim: ts=4 sw=4 et
