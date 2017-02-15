/*
 * Name: snapmanagercgi.js
 * Layout: default
 * Version: 0.1
 * Browsers: all
 * Copyright: Copyright 2017 (c) Made to Order Software Inc.
 * License: GPLv2
 */

var button_name;
var last_tr;
var last_serialized_data;

// Find first ancestor of el with tagName
// or undefined if not found
function upTo(el, tagName)
{
    tagName = tagName.toLowerCase();
    while (el && el.parentNode)
    {
        el = el.parentNode;
        if (el.tagName && el.tagName.toLowerCase() === tagName)
        {
            return el;
        }
    }
    return null;
}


jQuery(document).ready(function()
{
    jQuery("#tabs").tabs({
        heightStyle: "content",
    }); 

    jQuery("button").click( function() {
        button_name = jQuery(this).attr("name");
    });

    jQuery(".manager_form").submit( function( event )
    {
        event.preventDefault();

        /*
         * Not sure I will need this again, but leaving just in case
        var fields = jQuery(this).serializeArray();
        jQuery.each( fields, function(i,element) {
            if( element.name === "hostname" )
            {
                host_name = element.value;
            }
            else if( element.name === "plugin_name" )
            {
                plugin_name = element.value;
            }
        });
        */

        last_serialized_data = jQuery(this).serialize();
        last_tr = upTo(this,"tr");
        jQuery(last_tr).addClass( "modified" );

        jQuery.ajax({
            url : "snapmanager",
            type: "POST",
            data: last_serialized_data + "&" + button_name + "="
        })
        .done( function(response) {
            jQuery(last_tr).removeClass("modified");
        })
        .fail( function(xhr,status,errorThrown) {
            server_fail(xhr,status,errorThrown);
        });
    });
});

// vim: ts=4 sw=4 et
