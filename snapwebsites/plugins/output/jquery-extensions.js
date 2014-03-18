/*
 * Name: jquery-extensions
 * Version: 1.0.0
 * Browsers: all
 * Copyright: Copyright 2014 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: jquery (1.10)
 * License: GPL 2.0
 */



/** \brief Objects that match any one of the selectors.
 *
 * Usage: $(":any(\<selectors\>)")
 *
 * This is particularly useful when you want to apply a function against
 * another, so for example you could apply a not to all the objects that
 * match the any:
 *
 * \code
 * $("span:any(\<selectors\>):not(\<selector\>)")
 * \endcode
 *
 * It makes is a little easier to handle the :not() without having to
 * repeat it many times over.
 *
 * \source
 * http://stackoverflow.com/questions/14525758/how-to-use-a-not-selector-with-multiple-span-classes
 *
 * \param[in] selector  A list of selectors, all matches get returned.
 *
 * \return The list of matched DOM tags.
 */
jQuery.expr[':'].any = jQuery.expr.createPseudo(function(selector){
    return function(el){
        return jQuery.find.matches(selector, [el]).length > 0;
    }
});

// vim: ts=4 sw=4 et
