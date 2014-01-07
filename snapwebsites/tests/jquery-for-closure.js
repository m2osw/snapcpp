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

jQuery.prototype = {
	selector: ".",
	context: null,

	contructor: jQuery,
	init: function(selector, opt_context)
	{
		this.selector = selector;
		this.context = opt_context === undefined ? document : opt_context;
	},

	animate: function(a, b) { },
	appendTo: function(a) { },
	attr: function(a, b) { },
	fadeIn: function(a) { },
	fadeOut: function(a) { },
	html: function() { },
	keydown: function(a) { },
	prop: function(a) { },
	ready: function(a) { },
	removeAttr: function(a) { },
	val: function() { }
};

