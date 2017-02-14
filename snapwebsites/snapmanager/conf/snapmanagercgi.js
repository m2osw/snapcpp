/*
 * Name: snapmanagercgi.js
 * Layout: default
 * Version: 0.1
 * Browsers: all
 * Copyright: Copyright 2017 (c) Made to Order Software Inc.
 * License: GPLv2
 */

jQuery(document).ready(function()
{
    var button_name;

    jQuery("#tabs").tabs({
        heightStyle: "content",
    }); 

    jQuery("button").click( function() {
        button_name = jQuery(this).attr("name");
        console.log("button clicked: " + button_name);
    });

    jQuery(".manager_form").submit( function( event ) {
        var field_objs  = jQuery(this).serializeArray();
        var my_fields   = {};
        jQuery.each( field_objs, function( index, field ) {
            my_fields[field.name] = field.value;
        });
        var data = jQuery(this).serialize() + "&" + button_name + "=";
        event.preventDefault();
        jQuery.ajax({
            url : "snapmanager",
            type: "POST",
            data: data
        })
        .done( function(response) {
            //alert( "success! " + response );
            location.reload();
        })
        .fail( function( xhr, status, errorThrown ) {
            alert( "Sorry, there was a problem! See console log for details." );
            console.log( "Error: " + errorThrown );
            console.log( "Status: " + status );
            console.dir( xhr );
        });
    });
});

// vim: ts=4 sw=4 et
