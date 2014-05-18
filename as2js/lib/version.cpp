/* version.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

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

#include    "as2js/as2js.h"

#include    <iostream>

namespace as2js
{

/** \brief Return the library version.
 *
 * This function can be used to get the current version of the as2js
 * library. It returns a string of the form:
 *
 * \code
 * <major>.<minor>.<release>
 * \endcode
 *
 * where each entry is a number (only numerics are used.)
 *
 * Note that this is different from using the AS2JS_VERSION macro
 * (defined in as2js.h) in that the macro defines the version you
 * are compiling against and not automatically the version that
 * your code will run against.
 *
 * \todo
 * Add another function that checks whether your code is compatible
 * with this library.
 *
 * \return The version of the loaded library.
 */
char const *as2js_library_version()
{
	return AS2JS_VERSION;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
