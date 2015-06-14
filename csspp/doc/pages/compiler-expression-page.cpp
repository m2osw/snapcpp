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

/** \page expression_page CSS Preprocessor Reference -- Expressions
 * \tableofcontents
 *
 * The CSS Preprocessor is capable of compiling simple expressions.
 * These are used with @-keywords and field values.
 *
 * Since expressions can appear in lists of values of a field, an
 * expression may stop mid-stream.
 *
 * \section expression CSS Preprocessor Expression
 *
 * \code
 *  expression: conditional
 * \endcode
 *
 * \section expression_list CSS Preprocessor Condition Expression
 *
 * \code
 *  expression_list: assignment
 *                 | expression_list ',' assignment
 * \endcode
 *
 * \section assignment CSS Preprocessor Condition Expression
 *
 * \code
 *  assignment: conditional
 *            | IDENTIFIER ':=' conditional
 * \endcode
 *
 * \section conditional CSS Preprocessor Conditional Expression
 *
 * \code
 *  conditional: logical_or
 *             | conditional '?' expression_list ':' logical_or
 * \endcode
 *
 * \section logical_or CSS Preprocessor Logical OR Expression
 *
 * \code
 * logical_or: logical_and
 *           | logical_or IDENTIFIER (='or') logical_and
 *           | logical_or '||' logical_and
 * \endcode
 *
 * \section logical_and CSS Preprocessor Logical AND Expression
 *
 * \code
 * logical_and: equality
 *            | logical_and IDENTIFIER (='and') equality
 *            | logical_and '&&' equality
 * \endcode
 *
 * \section equality CSS Preprocess Equality Expression
 *
 * \code
 * equality: relational
 *         | equality '=' relational
 *         | equality '!=' relational
 *         | equality '~=' relational
 *         | equality '^=' relational
 *         | equality '$=' relational
 *         | equality '*=' relational
 *         | equality '|=' relational
 * \endcode
 *
 * \section relational CSS Preprocessor Relational Expression
 *
 * \code
 * relational: additive
 *           | relational '<' additive
 *           | relational '<=' additive
 *           | relational '>' additive
 *           | relational '>=' additive
 * \endcode
 *
 * \section additive CSS Preprocessor Additive Expression
 *
 * \code
 *  additive: multiplicative
 *          | additive '+' multiplicative
 *          | additive '-' multiplicative
 * \endcode
 *
 * \section multiplicative CSS Preprocessor Multiplicative Expression
 *
 * \code
 *  multiplicative: power
 *                | multiplicative '*' power
 *                | multiplicative '/' power
 *                | multiplicative '%' power
 * \endcode
 *
 * \section power CSS Preprocessor Power Expression
 *
 * \code
 *  power: post
 *       | post '**' post
 * \endcode
 *
 * \section post CSS Preprocessor Post Expression
 *
 * \code
 *  post: unary
 *      | post '[' expression_list ']'
 *      | post '.' IDENTIFIER
 * \endcode
 *
 * \section unary Unary CSS Preprocessor Unary Expression Expression
 *
 * \code
 *  unary: IDENTIFIER
 *       | INTEGER
 *       | DECIMAL_NUMBER
 *       | STRING
 *       | PERCENT
 *       | FUNCTION expression_list ')'
 *       | '(' expression_list ')'
 *       | '+' power
 *       | '-' power
 *       | '!' power
 * \endcode
 */

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
