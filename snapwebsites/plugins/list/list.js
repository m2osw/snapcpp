/*
 * Version: 0.0.4
 * Browsers: all
 */

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
snapwebsites.List = function()
{
};


snapwebsites.List.prototype = {
    init: function()
    {
    }
};

// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.ListInstance = new snapwebsites.List();
        snapwebsites.ListInstance.init();
    }
);

// vim: ts=4 sw=4 et
