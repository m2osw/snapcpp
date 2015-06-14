// CSS Preprocessor
// Copyright (C) 2015  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// Documentation only file

/** \page selectors_rules CSS Preprocessor Reference -- Selectors
 * \tableofcontents
 *
 * CSS Preprocessor parses all the selectors it finds in all the source
 * files it parses. This ensures that only valid selectors are output
 * making it easier to find potential errors in your source CSS files.
 *
 * The supported selectors are all the selected supported in CSS 3 and
 * the %<name> selector which is used to allow for optional rules
 * defined in CSS libraries.
 *
 * \section asterisk Select All (*)
 *
 * The asterisk (*) character can be used to select any tag.
 *
 * For example the following says any 'a' tag which appears in a tag
 * defined inside a 'div' tag. (i.e. \<div>\<at least one other tag>\<a>).
 *
 * \code
 *      div * a { color: orange; }
 * \endcode
 *
 * \section dash_match Dash Match (|=)
 *
 * The dash match, written pipe (|) and equal (=), no spaces in between,
 * is used to check a language in the hreflang attribute of an anchor
 * tag. It is very unlikely that you will ever need this matching
 * operator unless you are in the academic world or have a website similar
 * to Wikipedia with translations of your many pages.
 *
 * http://www.rfc-editor.org/rfc/bcp/bcp47.txt
 *
 * \section grammar Grammar used to parse the selectors
 *
 * The definition of the grammar appears in CSS 3, the selectors:
 *
 * http://www.w3.org/TR/selectors/
 *
 * There is a more yacc like grammar definition:
 *
 * \code
 * selector-list: selector
 *              | selector-list ',' selector
 *
 * selector: term
 *         | selector WHITESPACE '>' WHITESPACE term
 *         | selector WHITESPACE '+' WHITESPACE term
 *         | selector WHITESPACE '~' WHITESPACE term
 *         | selector WHITESPACE term
 *         | selector term
 *
 * term: simple-term
 *     | PLACEHOLDER
 *     | REFERENCE
 *     | ':' FUNCTION (="not") component-value-list ')'
 *     | ':' ':' IDENTIFIER
 *
 * simple-term: universal-selector
 *            | qualified-name
 *            | HASH
 *            | ':' IDENTIFIER
 *            | ':' FUNCTION (!="not") component-value-list ')'
 *            | '.' IDENTIFIER
 *            | '[' WHITESPACE attribute-check WHITESPACE ']'
 *
 * universal-selector: IDENTIFIER '|' '*'
 *                   | '*' '|' '*'
 *                   | '|' '*'
 *                   | '*'
 *
 * qualified-name: IDENTIFIER '|' IDENTIFIER
 *               | '*' '|' IDENTIFIER
 *               | '|' IDENTIFIER
 *               | IDENTIFIER
 *
 * attribute-check: qualified-name
 *                | qualified-name WHITESPACE attribute-operator WHITESPACE attribute-value
 *
 * attribute-operator: '='
 *                   | '~='
 *                   | '^='
 *                   | '$='
 *                   | '*='
 *                   | '|='
 *
 * attribute-value: IDENTIFIER
 *                | INTEGER
 *                | DECIMAL_NUMBER
 *                | STRING
 * \endcode
 *
 * All operators have the same priority and all selections are always going
 * from left to right.
 *
 * The FUNCTION parsing changes for all n-th functions to re-read the input
 * data as an A+Bn which generates a new token as expected.
 *
 * Further we detect whether the same HASH appears more than once. Something
 * like:
 *
 * \code
 *      #my-div .super-class #my-div { ... }
 * \endcode
 *
 * is not going to work (assuming that the document respects the idea that
 * 'my-div' cannot be used more than once since identifiers are expected
 * to be unique.)
 */

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
