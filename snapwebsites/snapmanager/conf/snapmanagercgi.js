/*
 * Name: snapmanagercgi.js
 * Layout: default
 * Version: 0.1
 * Browsers: all
 * Copyright: Copyright 2017 (c) Made to Order Software Inc.
 * License: GPLv2
 */

// Find first ancestor of element with tagName
// or undefined if not found
//
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


var div_map = {};

function FieldDiv( the_form )
{
    jq_form             = jQuery(the_form);
    this.parent_tr      = upTo(the_form,"tr");
    this.parent_div     = jQuery( upTo(this.parent_tr,"div") );
    this.button_name    = jq_form.data("button_name");
    this.form_post_data = jq_form.serialize();
    this.form_data      = {}
    this.ajax_pending   = true; // Set to true when we are waiting for an ajax response.

    var that = this;
    jQuery.each( jq_form.serializeArray(), function(i,element) {
        that.form_data[element.name] = element.value;
    });

    jQuery(this.parent_tr).addClass("modified");

    // Replace the div which contiains the "modified" tr.
    // If you set save_form_data to 'true', it will first
    // save the embedded form data, useful for POSTs.
    //
    this.replace_div = function( response )
    {
        this.parent_div.html(response);
        this.ajax_pending = false;
    }

    this.get_field_id = function()
    {
        return this.form_data["plugin_name"] + "::" + this.form_data["field_name"];
    }

    this.is_modified = function()
    {
        return this.parent_div.find("tr[class='modified']").length > 0;
    }

    this.get_post_data = function()
    {
        return this.form_post_data + "&" + this.button_name + "=";
    }

    this.get_query_data = function()
    {
        return    "hostname="     + this.form_data.hostname
                + "&field_name="  + this.form_data.field_name
                + "&plugin_name=" + this.form_data.plugin_name
                + "&status=true";
    }
}


// Common failure function.
//
function server_fail( xhr, the_status, errorThrown )
{
    console.log( "Failed to connect to server!"  );
    console.log( "xhr   : [" + xhr         + "]" );
    console.log( "status: [" + status      + "]" );
    console.log( "error : [" + errorThrown + "]" );
}


function check_for_modified_divs()
{
    var pending = false;
    jQuery.each( div_map, function( index, div_object )
    {
        if( div_object.is_modified() )
        {
            pending = true;
        }
    });
    return pending;
}


// Hook up form events.
//
// For new DOM objects we injected, this is imperative.
//
function hook_up_form_events()
{
    jQuery("button").click( function(event)
    {
        if( check_for_modified_divs() )
        {
            //alert( "Operation pending, try again later." );
            // Ignore button hit if operation pending.
            event.preventDefault();
            return;
        }

        var button_name = jQuery(this).attr("name");
        var parent_form = jQuery(this).parent();
        parent_form.data( "button_name", button_name );
    });

    jQuery(".manager_form").submit( function( event )
    {
        event.preventDefault();

        var div_object = new FieldDiv( this );
        div_map[div_object.get_field_id()] = div_object;

        jQuery.ajax(
        {
            url : "snapmanager",
            type: "POST",
            data: div_object.get_post_data()
        })
        .done( function(response)
        {
            div_object.replace_div( response );
            hook_up_form_events();
        })
        .fail( function( xhr, the_status, errorThrown )
        {
            server_fail( xhr, the_status, errorThrown );
        });
    });
}


// Start monitor interval that runs forever, but only
// gets activated if a div_map is modified.
//
function start_monitor()
{
    setInterval( function()
    {
        jQuery.each( div_map, function( index, div_object )
        {
            if( !div_object.is_modified() || div_object.ajax_pending ) return;

            div_object.ajax_pending = true;
            jQuery.ajax(
            {
                url : "snapmanager",
                type: "POST",
                data: div_object.get_query_data()
            })
            .done( function(response)
            {
                div_object.replace_div( response );
                hook_up_form_events();
            })
            .fail( function( xhr, the_status, errorThrown )
            {
                server_fail( xhr, the_status, errorThrown );
            });
        });
    }, 1000 );
}


// When the document is ready, move the ul and divs into jQuery tabs.
//
// Set the interval once per second to send queries for modified divs
// which we are waiting on.
//
jQuery(document).ready(function()
{
    jQuery("#tabs").tabs(
    {
        heightStyle: "content",
    }); 

    start_monitor();

    hook_up_form_events();
});

// vim: ts=4 sw=4 et
