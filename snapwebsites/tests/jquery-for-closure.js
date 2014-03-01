/*
 * Fake definitions to run the Google Closure Compiler against our code
 * to discover errors that their compile detects.
 */

/**
 * @param {string|Object} selector  The CSS like selector.
 * @param {Object=} opt_context  The context, defaults to document.
 */
function jQuery(selector, opt_context)
{
	//return new jQuery.prototype.init(selector, opt_context);
	return null;
}

jQuery.ajax = function(a, b) { };
jQuery.prototype = {
	addClass: function(a, b) { },
	animate: function(a, b) { },
	appendTo: function(a) { },
	attr: function(a, b) { },
	delay: function(a) { },
	each: function(a) { },
	fadeIn: function(a) { },
	fadeOut: function(a) { },
	hasClass: function(a) { },
	html: function() { },
	hover: function(a) { },
	init: function(selector, opt_context) { },
	keydown: function(a) { },
	mouseleave: function() { },
	off: function(a) { },
	on: function(a, b) { },
	prependTo: function(a) { },
	prop: function(a) { },
	ready: function(a) { },
	removeAttr: function(a) { },
	val: function() { }
};

