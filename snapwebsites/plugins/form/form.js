/*
 * Name: form
 * Version: 0.0.1.6
 * Browsers: all
 * Copyright: Copyright 2012-2014 (c) Made to Order Software Corporation  All rights reverved.
 * License: GPL 2.0
 */

/** \brief Snap Form Manipulations.
 *
 * This class initializes and handles the different form inputs.
 *
 * \note
 * The Snap! Form is a singleton and should never be created by you. It
 * gets initialized automatically when this form.js file gets included.
 *
 * @constructor
 */
snapwebsites.Form = function()
{
};


snapwebsites.Form.prototype = {
    _focus: function(widget)
    {
        // TODO: ameliorate with 2 colors until change() happens
        if(jQuery(widget).val() === jQuery(widget).data("background-value")
        && !jQuery(widget).data("editor"))
        {
            jQuery(widget).val("");
        }

        // we force the removal of the class
        // (just in case something else went wrong)
        jQuery(widget).removeClass('input-with-background-value');

        if(jQuery(widget).hasClass('password-input'))
        {
            jQuery(widget).attr('type', 'password');
        }
    },

    _change: function(widget)
    {
        jQuery(widget).data("edited", jQuery(widget).val() != "");
    },

    _blur: function(widget)
    {
        if(!jQuery(widget).is(":focus"))
        {
            if(jQuery(widget).val().length === 0)
            {
                jQuery(widget).val(jQuery(widget).data("background-value"))
                    .addClass('input-with-background-value');

                if(jQuery(widget).hasClass('password-input'))
                {
                    jQuery(widget).attr('type', 'text');
                }
            }
        }
    },

    init: function()
    {
        jQuery("input[type='text']:not([data-background-value='']),input[type='password']:not([data-background-value=''])")
            .focus(function(){snapwebsites.FormInstance.prototype._focus(this);})
            .change(function(){snapwebsites.FormInstance.prototype._change(this);})
            .blur(function(){snapwebsites.FormInstance.prototype._blur(this);})
            .each(function(i,e){snapwebsites.FormInstance.prototype._blur(e);});
    }
};

// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.FormInstance = new snapwebsites.Form();
        snapwebsites.FormInstance.init();
    }
);
// vim: ts=4 sw=4 et
