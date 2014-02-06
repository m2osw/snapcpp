/*
 * Version: 0.0.4
 * Browsers: all
 */

/** \brief Defines the snapwebsites namespace in the JavaScript environment.
 *
 * All the JavaScript functions defined by Snap! plugins are defined inside
 * the snapwebsites namespace. For example, the WYSIWYG Snap! editor is
 * defined as:
 *
 * \code
 * snapwebsites.editor
 * \endcode
 *
 * \note
 * Technically, this is an object.
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
 */
snapwebsites.Output = function()
{
};


snapwebsites.Output.prototype = {
    init: function()
    {
        jQuery("div.user-messages")
            .delay(250).fadeIn(300) // put a little delay so we see the fadeIn(), eventually
            .click(function(){jQuery(this).fadeOut(300);});
    }
};

// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.OutputInstance = new snapwebsites.Output();
        snapwebsites.OutputInstance.init();
    }
);

// vim: ts=4 sw=4 et
