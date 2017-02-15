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


function replace_div( response, save_form_data )
{
    var the_data;
    var modified_tr = jQuery("tr[class='modified']");

    if( save_form_data === "true" )
    {
        var the_form = modified_tr.find("form");
        the_data     = the_form.data("form_data");
    }

    var parent_div  = jQuery(upTo(modified_tr.get(0),"div"));
    parent_div.html(response);

    if( save_form_data === "true" )
    {
        parent_div.data("form_data",the_data);
    }
    else
    {
        parent_div.data("ajax_pending", "false");
    }
}


function server_fail( xhr, the_status, errorThrown )
{
    console.log( "Failed to connect to server!"  );
    console.log( "xhr   : [" + xhr         + "]" );
    console.log( "status: [" + status      + "]" );
    console.log( "error : [" + errorThrown + "]" );
}


function hook_up_form_events()
{
    jQuery("button").click( function() {
        var button_name = jQuery(this).attr("name");
        var parent_form = jQuery(this).parent();
        parent_form.data( "button_name", button_name );
    });

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

        console.log("Sending data POST...");
        jQuery.ajax(
        {
            url : "snapmanager",
            type: "POST",
            data: the_form.serialize() + "&" + button_name + "="
        })
        .done( function(response)
        {
            replace_div( response, "true" );
            hook_up_form_events();
        })
        .fail( function( xhr, the_status, errorThrown )
        {
            server_fail( xhr, the_status, errorThrown );
        });
    });
}


jQuery(document).ready(function()
{
    jQuery("#tabs").tabs(
    {
        heightStyle: "content",
    }); 

    setInterval( function()
    {
        var modified_tr = jQuery("tr[class='modified']");
        if( modified_tr.length > 0 )
        {
            console.log("modified trs found!");
            var parent_div  = jQuery(upTo(modified_tr.get(0),"div"));
            if( parent_div.data("ajax_pending") !== "true" )
            {
                console.log("Sending status POST...");
                var the_data  = parent_div.data("form_data");
                if( the_data )
                {
                    parent_div.data("ajax_pending", "true");
                    jQuery.ajax(
                    {
                        url : "snapmanager",
                        type: "POST",
                        data: "hostname="+the_data.hostname
                            + "&field_name="+the_data.field_name
                            + "&plugin_name="+the_data.plugin_name
                            + "&status=true"
                    })
                    .done( function(response)
                    {
                        replace_div( response, "false" );
                        hook_up_form_events();
                    })
                    .fail( function( xhr, the_status, errorThrown )
                    {
                        server_fail( xhr, the_status, errorThrown );
                    });
                }
            }
        }
    }, 1000 );

    hook_up_form_events();
});

// vim: ts=4 sw=4 et
