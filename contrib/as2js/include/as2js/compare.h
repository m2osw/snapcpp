#ifndef AS2JS_COMPARE_H
#define AS2JS_COMPARE_H
/* compare.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


namespace as2js
{


enum class compare_t
{
	COMPARE_EQUAL = 0,
	COMPARE_GREATER = 1,
	COMPARE_LESS = -1,
	COMPARE_UNORDERED = 2,
	COMPARE_ERROR = -2,
	COMPARE_UNDEFINED = -3		// not yet compared
};


namespace compare_utils
{
inline bool is_ordered(compare_t const c)
{
    return c == compare_t::COMPARE_EQUAL || c == compare_t::COMPARE_GREATER || c == compare_t::COMPARE_LESS;
}
}
// namespace compare

}
// namespace as2js
#endif
// #ifndef AS2JS_COMPARE_H

// vim: ts=4 sw=4 et
