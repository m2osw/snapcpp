/*
 * Name: form
 * Version: 0.0.1.3
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
    init: function()
    {
        jQuery("input[type='text']:not([data-background-value='']),input[type='password']:not([data-background-value=''])")
            .focus(function(){
                // TODO: ameliorate with 2 colors
                if(jQuery(this).val() === jQuery(this).data("background-value")
                && !jQuery(this).data("editor"))
                {
                    jQuery(this).val("")
                        .removeClass('input-with-background-value');
                }
            })
            .change(function(){
                jQuery(this).data("edited", this.value.length > 0);
            })
            .blur(function(){
                if(jQuery(this).val().length === 0)
                {
                    jQuery(this).val(jQuery(this).data("background-value"))
                        .addClass('input-with-background-value');
                }
            });
    }
};

// auto-initialize
jQuery(document).ready(function(){snapwebsites.FormInstance = new snapwebsites.Form();snapwebsites.FormInstance.init();});
// vim: ts=4 sw=4 et
