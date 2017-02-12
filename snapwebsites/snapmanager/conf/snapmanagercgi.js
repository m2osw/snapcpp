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
    jQuery("#accordion").accordion({
        animate: 300,
        collapsible: true,
        header: "h3",
        heightStyle: "content",
        icons: { "header": "ui-icon-triangle-1-e", "activeHeader": "ui-icon-triangle-1-s" }
    }); 
});

// vim: ts=4 sw=4 et
