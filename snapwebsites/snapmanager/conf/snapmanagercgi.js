/*
 * Name: snapmanagercgi.js
 * Layout: default
 * Version: 0.1
 * Browsers: all
 * Copyright: Copyright 2017 (c) Made to Order Software Inc.
 * License: GPLv2
 */

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
        var button_name = jQuery(this).attr("name");
        var parent_form = jQuery(this).parent();
        parent_form.data( "button_name", button_name );
    });

    setInterval( function() {
        var modified_tr = jQuery("tr[class='modified']");
        if( modified_tr.length > 0 )
        {
            console.log("modified trs found!");
            var parent_div  = jQuery(upTo(modified_tr.get(0),"div"));
            if( parent_div.data("ajax_pending") !== "true" )
            {
                console.log("Sending ajax...");
                var the_data  = parent_div.data("form_data");
                if( the_data )
                {
                    parent_div.data("ajax_pending", "true");
                    jQuery.ajax({
                                    url : "snapmanager",
                                    type: "POST",
                                    data: "hostname="+the_data.hostname
                                          + "&field_name="+the_data.field_name
                                          + "&plugin_name="+the_data.plugin_name
                                          + "&status=true"
                                })
                    .done( function(response) {
                        var modified_tr = jQuery("tr[class='modified']");
                        var parent_div  = jQuery(upTo(modified_tr.get(0),"div"));
                        parent_div.html(response);
                        parent_div.data("ajax_pending", "false");
                    })
                    .fail( function(xhr,status,errorThrown) {
                        server_fail(xhr,status,errorThrown);
                    });
                }
            }
        }
    }, 1000 );

    jQuery(".manager_form").submit( function( event )
    {
        event.preventDefault();

        var the_form = jQuery(this);

        var fields_array = {};
        var fields = the_form.serializeArray();
        jQuery.each( fields, function(i,element) {
            fields_array[element.name] = element.value;
        });
        the_form.data( "form_data", fields_array );

        var last_tr = upTo(this,"tr");
        jQuery(last_tr).addClass("modified");

        var button_name = the_form.data("button_name");
        jQuery.ajax({
            url : "snapmanager",
            type: "POST",
            data: the_form.serialize() + "&" + button_name + "="
        })
        .done( function(response) {
            var modified_tr = jQuery("tr[class='modified']");
            var the_form    = modified_tr.find("form");
            var the_data    = the_form.data("form_data");
            var parent_div  = jQuery(upTo(modified_tr.get(0),"div"));
            parent_div.html(response);
            parent_div.data("form_data",the_data);
        })
        .fail( function(xhr,status,errorThrown) {
            server_fail(xhr,status,errorThrown);
        });
    });
});

// vim: ts=4 sw=4 et
