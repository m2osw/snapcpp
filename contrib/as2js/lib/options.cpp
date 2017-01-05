/* options.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

/*

Copyright (c) 2005-2017 Made to Order Software Corp.

http://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "as2js/options.h"


/** \file
 * \brief Implementation of the Options object.
 *
 * The Options object is used to carry all the options around the
 * entire set of functions used to compile AlexScript.
 */


namespace as2js
{


/** \enum Options::option_t
 * \brief The available options.
 *
 * This enumeration defines all the options available in the compiler.
 * Additional options may be available in the command line tool, but
 * these are all those supported in the code via the 'use' keyword
 * (i.e. pragma) and expected to be command line options to the
 * command line tool.
 */


/** \var Options::OPTION_UNKNOWN
 * \brief Unknown option, used as a fallback in different situations.
 *
 * This is not really an option. It is used as a fallback in a few
 * situations where some otion is required, but none is really
 * available to do the job.
 */


/** \var Options::OPTION_ALLOW_WITH
 * \brief Whether the 'with' keyword can be used.
 *
 * By default the 'with' keyword is forbidden by as2js. You must
 * explicitly allow it to use it with:
 *
 * \code
 *      use allow_with(1);
 * \endcode
 *
 * In general, the 'with' statement is considered as deprecated and
 * it should not be used because the order in which objects are checked
 * for a field found inside a with statement is undefined. Because of
 * that, it is not possible to make sure that code within a 'with'
 * statement does what it is expected to do.
 *
 * In other words, it is not secure.
 *
 * Note that if the compiler is set in strict mode, the 'with' statement
 * is not allowed, whether or not this flag is turned on.
 */


/** \var Options::OPTION_BINARY
 * \brief Whether binary numbers are allowed.
 *
 * By default, binary numbers are not supported. If this option
 * is turned on, then the compiler recognizes binary numbers.
 *
 * The syntax of a binary number is: ('0' 'b' ['0'-'1']+)
 *
 * For example, 170 in binary:
 *
 * \code
 *    a := 0b10101010;
 * \endcode
 *
 * JavaScript does not support the binary notation at all. However, the
 * introducer '0b' is not compatible is a welformed JavaScript source file.
 *
 * \note
 * The maximum number of digits allowed is 64. Note, however, that
 * JavaScript does not support 64 bit decimal numbers so some bits
 * will be lost on such large numbers.
 */


/** \var Options::OPTION_COVERAGE
 * \brief Whether coverage is requested.
 *
 * AlexScript includes the necessary support to generate coverage code.
 * This is used to know whether your tests are thorough and really test
 * all the code.
 *
 * It is possible that your code becomes very slow because of this option.
 * Code that you use in a loop or that generally needs to be fast may
 * require that you surround that code with pragmas to temporarilly turn
 * the coverage off:
 *
 * \code
 *      use coverage(0);
 *      function slow_stuff()
 *      {
 *         ...
 *      }
 *      use coverage(1);
 * \endcode
 *
 * A future version will allow you to push/pop the current status so you
 * do not have to delete the pragmas once done running your tests.
 *
 * \note
 * This has to be used with some AJAX code to retrieve the counters so
 * just turning on the coverage option is not enough.
 */


/** \var Options::OPTION_DEBUG
 * \brief Turn on debug features.
 *
 * AlexScript supports a set of debug features, which still need to
 * be implemented, to help you find problems in your code.
 *
 * The debug option turns on those features and autmoatically adds
 * debug code in the output so you can easily find problems in your
 * source.
 */


/** \var Options::OPTION_EXTENDED_ESCAPE_SEQUENCES
 * \brief Accept additional escape sequences in strings.
 *
 * This options authorizes the compiler to transform escape sequences that
 * are otherwise forbidden in JavaScript. The compiler will automatically
 * transform those to a valid supported value in the final output.
 *
 * The extensions are:
 *
 * \li "\UXXXXXXXX" -- support any Unicode character from 0 to 0x10FFFF.
 * Generate two \\uXXXX of encoded UTF-16 in the output.
 *
 * \li "\e" -- insert an escape character in the string (code 033).
 *
 * \li "\0XX" to "\7XX" -- support any octal character. JavaScript may
 * accept those. We transform them to \\uXXXX. Note that only ISO-8869-1
 * characters are accept as the number of digits is limited to 3 (i.e. to
 * a number between 0 and 255.)
 *
 * Note that "\0" is always accepted and it represents the NUL character.
 */


/** \var Options::OPTION_EXTENDED_OPERATORS
 * \brief Accept additional operators.
 *
 * AlexScript offers additional operators to cover some operations that
 * are fairly common in programming and most often annoying to write
 * by hand.
 *
 * When this option is turned off, those extended operators are
 * recognized so everything continues to compile, but the parser
 * generates errors on each one found. When this option is set to
 * a value other than zero, the extended operators are accepted.
 *
 * The supported extensions are:
 *
 * \li '**' and '**=' -- power and power assignment operators; these are
 * transforms to Math.pow(l, r);.
 *
 * \li '`...`' -- allow regular expressions to be written without the
 * use of the standard /.../ syntax which can be confused when used
 * improperly (i.e. the '/' is used by comments, divisions, assignments...)
 *
 * \li '::' -- the scope operator which may be removed at some point,
 * although there are still some cases where this operator can be used
 * and not the member (.) operator... (TBD)
 *
 * \li '~~', '~=' and '!~' -- the smart match, match, and not match
 * operators that are used to match a value (left hand side) to a
 * regular expression (right hand side). These are taken from perl,
 * although the '~=' is inverted because '=~' could legally also
 * mean 'a = ~b'.
 *
 * \li '<%', '<%=', '>%', and '>%=' -- rotate to the left and rotate to
 * the right. This are just convenient in AlexScript. Remember that
 * bitwise operations only work on 32 bits.
 *
 * \li 'in ...' -- test whether the left hand side fits in a specified range.
 * This is similar to the longer syntax (a >= min && a <= max).
 *
 * \li '<>' -- this is another way to write '!='
 *
 * \li ':=' -- this is another way to write '=', although we offer a way to
 * force the code to use ':=' only.
 *
 * \li '<=>' -- this operator is taken from perl, it returns -1, 0, or 1
 * as a comparison result (if l < r, return -1; if l == r, return 0; and
 * if l > r, return 1).
 *
 * \li '^^' and '^^=' -- the logical XOR operator and corresponding
 * assignment. This is similar to the && and ||, but somehow they do
 * not offer that operator in C/C++/JavaScript/Java, etc. Not too sure
 * why since it is a rather common operation to perform.
 *
 * \li '&&=' and '||=' -- the logical AND and OR assignments, which are
 * not offered in C/C++/JavaScript/Java... I think the main reason to
 * not offer such is because it could be viewed as being confusing. But
 * there is no reason not to have those operations.
 *
 * \li '<?', '<?=', '>?', and '>?=' -- the minimum and maximum operators,
 * as they were defined by g++ at some point.
 *
 * This option has an extended feature which is to use flag 1 as a mean
 * to authorize (0) or forbid (1) the use of the '=' character as the
 * assignment operator. In other words, the following code is normal
 * AlexScript code:
 *
 * \code
 *      a = 123;
 * \endcode
 *
 * which fails with an error when bit 1 of the OPTION_EXTENDED_OPERATORS
 * value is set. So the following will NOT compile:
 *
 * \code
 *      use extended_operators(3);
 *      a = 123;   // error here
 * \endcode
 *
 * There is another extended operator which is the Pascal assignment
 * operator: ':=', which must be used to replace the '=' operator.
 * This the statment we've just see can be rewritten as:
 *
 * \code
 *      use extended_operators(3);
 *      a := 123;   // that works!
 * \endcode
 *
 * The idea is simple: it is much less likely that you would mistakingly
 * use ':=' instead of '=='. Also, at some point we want to completely
 * forbid assignments in expressions and instead only allow them as
 * statements (as done in Ada and Pascal). This way you avoid potential
 * problems with lines of code such as:
 *
 * \code
 *      if(a = 3)
 *      {
 *          // do something
 *          b += 4;
 *          ...
 *      }
 * \endcode
 *
 * which is the same as:
 *
 * \code
 *      a = 3;
 *      b += 4;
 *      ...
 * \endcode
 *
 * since (a = 3) returns 3 and that is viewed as always true and
 * it is probably a bug. In other words, the programmer probably meant:
 *
 * \code
 *      if(a == 3)
 *      {
 *          // do somthing
 *          b += 4;
 *          ...
 *      }
 * \endcode
 *
 * With a feature saying that assignments have to be writte as statements,
 * the <code>if(a = 3)</code> statement is not legal because in that case
 * the assignment is in an expression.
 *
 * \note
 * At this point, use 2 or 3 as the option value has the same effect.
 */


/** \var Options::OPTION_EXTENDED_STATEMENTS
 * \brief Accept additional statement structures.
 *
 * AlexScript offers additional capabilities while parsing your code.
 *
 * The following is the list of statements that are available to you
 * when the extended statement option is set:
 *
 * \li 'case <expr> ... <expr>' -- support a range in a case. This is
 * the same as gcc/g++. The left and right expressions are inclusive.
 *
 * \li 'switch(<expr>) with(<expr>) ...' -- support the specification
 * of the switch operator using the 'with' keyword. Note that this use
 * of the 'with' keyword has nothing to do with the 'with' statement
 * and thus the OPTION_ALLOW_WITH and OPTION_STRICT have no effect
 * over this usage.
 *
 * This option can also have bit 1 set to trigger the 'force a block
 * of statements' here feature. When writing code, one can use one
 * liners such as:
 *
 * \code
 *      if(a)
 *          b = a.somefield;
 * \endcode
 *
 * This is all good until the programmer notices that he needs a
 * second line of code and writes:
 *
 * \code
 *      if(a)
 *          b = a.somefield;
 *          c = a.something_else;
 * \endcode
 *
 * As we can see, the second statement does not depend on the if()
 * BUT if you do not pay close attention, the fact that the curvly
 * brackets are missing may not be obvious. Bit 1 of the extended
 * statement option is used to force the programmer to use such
 * statements with curvly brackets, ALWAYS.
 *
 * Note that if you do not put the curvly brackets to save space
 * in the final JavaScript code, know that our assembler will do
 * that for you automatically. So you do not have to worry about
 * that in your AlexScript source code.
 *
 * \code
 *      // fine, the extension is turned off
 *      use extended_statements(0);
 *      if(a)
 *          b = a.somefield;
 *
 *      // error, the extension is turned on
 *      use extended_statements(2);
 *      if(a)
 *          b = a.somefield;
 *
 *      // fine, the programmer used the { ... }
 *      use extended_statements(2);
 *      if(a)
 *      {
 *          b = a.somefield;
 *      }
 * \endcode
 *
 * \note
 * At this point, use 2 or 3 as the option value has the same effect.
 */


/** \var Options::OPTION_JSON
 * \brief Change the lexer to read data for our JSON implementation.
 *
 * The js2as library includes a JSON parser. It will force this
 * option to 1 when using the lexer from that parser. This tells
 * the lexer that a certain number of characters (such as 0x2028)
 * are to be viewed as standard characters instead of specialized
 * characters.
 *
 * \todo
 * This is not yet fully implemented.
 */


/** \var Options::OPTION_OCTAL
 * \brief Whether octal numbers are allowed.
 *
 * By default, octal numbers are not supported. If this option
 * is turned on, then the compiler recognizes octal numbers.
 *
 * The syntax of an octal number is: ('0' ['0'-'7']+)
 *
 * For example, 170 in octal:
 *
 * \code
 *    a := 0252;
 * \endcode
 *
 * JavaScript does supports the octal notation. However, it is forbidden
 * in strict mode and it is not considered safe so we only use decimal
 * numbers in the output.
 *
 * \note
 * AlexScript accepts up to 22 digits in octal numbers. This allows you
 * to write a 64 bit decimal number. Remember, though, that JavaScript
 * does not support 64 bit decimals. Some of the digits will be lost.
 */


/** \var Options::OPTION_STRICT
 * \brief Whether strict mode is turned on.
 *
 * By defaut, just like JavaScript, the compiler accepts 'weak' code
 * practices. This option turns on the strict mode of AlexScript.
 *
 * Note that by default AlexScript is already way strictier than the
 * standard JavaScript. For example, semi-colons are required everywhere.
 * On the other hand, AlexScript allows break and return statements to
 * be written on multiple lines.
 *
 * The following is legal code, even in strict mode:
 *
 * \code
 *      return
 *          a
 *          *
 *          3
 *      ;
 *
 *      break
 *          outside
 *      ;
 * \endcode
 *
 * We may look into fixing these, although the only reason for JavaScript
 * to disallow this syntax is to allow missing semi-colons so it is not
 * really important. We will always output semi-colons after each line
 * in our output.
 */


/** \var Options::OPTION_TRACE
 * \brief Turn on trace mode.
 *
 * This option requests that trace mode be turned on.
 *
 * Trace mode is a feature from AlexScript which adds code to trace
 * each "statement" just before it gets executed. We put statement between
 * quotes because in many cases it is more like a line of code (i.e. an
 * if(...) statement includes the block of statements, the else and the
 * blocks following the else, and in that case the trace only shows the
 * if() and its expression.)
 *
 * The trace feature makes use the Trace package which can be tweaked
 * to your liking. The Trace object records the information from the
 * original source file such as the line number, file name, options,
 * etc.
 */


/** \var Options::OPTION_UNSAFE_MATH
 * \brief Tell the optimizer whether to apply unsafe mathematical optimizations.
 *
 * Many operations in JavaScript look like they can be optimized. For example:
 *
 * \code
 *      a := a | 0;
 *      a := a & -1;
 *      a := ~~a;
 *      a := a * 1;
 *      a := a + '';
 * \endcode
 *
 * All of these operations have side effects. The bitwise operations
 * first transform the contents of variable 'a' to 32 bits integer,
 * then apply the operation, and finally save the result back in a
 * double. In other words, the bitwise operations presented here are
 * equivalent to <code>a &= 0xFFFFFFFF;</code>.
 *
 * The multiplication first transforms 'a' into a number, then multiplies
 * by 1, and finally saves the result in 'a'. If 'a' was a string, it
 * applies a conversion which may be important when using 'a' later
 * (i.e. some functions you call may check whether 'a' is a number.)
 *
 * The concatenation in the last statement transforms the variable 'a'
 * to a string. You could also have used the <code>a.toString()</code>
 * function.
 *
 * When the unsafe math option is turned on, many of these operations
 * will get optimized anyway. In all the examples shown above, the
 * optimizer would simply remove the entire statement. The result
 * is different and thus it may not work right, this is why we offer
 * this option because at times you may want to turn off the pragma.
 *
 * In most cases, it is not recommended to use this option.
 */


/** \var Options::OPTION_max
 * \brief Gives the number of options defined.
 *
 * This is not an option. It is used to define arrays of options as all
 * options are numbered from 0 to OPTION_max - 1.
 */




/** \typedef Options::option_value_t
 * \brief The type used by options.
 *
 * Each option is an integer of this type. The type is at least 32 bits.
 *
 * Most options just use 0 (false/not set) or 1 (true/set). However, some
 * options may use other values. In most cases the value will be used as
 * a set of flags:
 *
 * \li OPTION_EXTENDED_STATEMENTS uses bit 1 to force programmers to use
 * blocks with 'if', 'while', 'for', etc.
 *
 * \li OPTION_EXTENDED_OPERATORS uses bit 1 to force programmers to use
 * the ':=' operator instead of the '=' operator.
 */


/** \typedef Options::pointer_t
 * \brief The smart pointer used when creating an Options object.
 *
 * All Options objects are created and saved in this type of pointer.
 * This allows us to ensure no memory leaks ensue and that the pointer
 * can be shared between all parties.
 */


/** \typedef Options::zoption_value_t
 * \brief A controlled version of the option values.
 *
 * When using the zoption_value_t typedef in your classes, you make
 * sure that the option value is initialized to the default value
 * which is zero (0).
 *
 * The Options class uses this type to create its vector of options.
 */


/** \var Options::f_options
 * \brief The array of options in the Options class.
 *
 * When creating an Options object, you get a set of options defined
 * in this f_options variable member.
 *
 * By default all the options are considered to be set to zero. So if
 * no option object was created, you may assume that all the values
 * are set to zero.
 *
 * If you then want to modify an option to a value other than zero (0)
 * then you must allocate an Options object and use the set_option()
 * function to set the value accordingly.
 */




/** \brief Initialize an options object.
 *
 * The default constructor initializes the options array to the maximum
 * number of options. The options are all set to the default, zero (0).
 *
 * To change the option value, use the set_option() function. At this
 * point pretty much all the options accept either 0 or 1 as their
 * value, although any number of than 0 is considered to represent "set".
 */
Options::Options()
    : f_options(static_cast<size_t>(option_t::OPTION_max))
{
}


/** \brief Set an option to the specified value.
 *
 * This function sets the option to the specified value.
 *
 * At this point, all options expect the value to be 0 or 1, although
 * the system does not enforce that at this point. Any value is thus
 * accepted.
 *
 * Options make use of an int64_t so any 64 bit values works.
 *
 * \param[in] option  The option to set.
 * \param[in] value  The new 64 bit value for this option.
 */
void Options::set_option(option_t option, option_value_t value)
{
    f_options[static_cast<size_t>(option)] = value;
}


/** \brief Retrieve the current value of an option.
 *
 * This function is used to retrieve the current value of an option.
 * At this point, all options are expected to be zero (0), the default,
 * or one (1). It is possible to set options to other values, though.
 *
 * \param[in] option  The option to retrieve.
 *
 * \return The current value of this option.
 */
Options::option_value_t Options::get_option(option_t option)
{
    return f_options[static_cast<size_t>(option)];
}


}
// namespace as2js

// vim: ts=4 sw=4 et
