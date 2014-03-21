/*
 * Name: output
 * Version: 0.0.5
 * Browsers: all
 * Copyright: Copyright 2014 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: jquery-extensions (1.0.1)
 * License: GPL 2.0
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
    /** \brief Helper function: generate hexadecimal number.
     *
     * This function transform byte \p c in a hexadecimal number of
     * exactly two digits.
     *
     * Note that \p c can be larger than a byte, only it should probably
     * not be negative.
     *
     * \param[in] c  The byte to transform (expected to be between 0 and 255)
     *
     * \return The hexadecimal representation of the number.
     */
    char2hex:function(c)
    {
        var a, b;

        a = c & 15;
        b = (c >> 4) & 15;
        return String.fromCharCode(b + (b >= 10 ? 55 : 48))
             + String.fromCharCode(a + (a >= 10 ? 55 : 48));
    },

    init: function()
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
        snapwebsites.OutputInstance.init();
    }
);

// vim: ts=4 sw=4 et
