/*
 * Text:
 *      src/version.cpp
 *
 * Description:
 *      Return the version of the library when it was compiled.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
 * 
 *      https://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "cassvalue/version.h"


namespace cassvalue
{



/** \brief Return a string to the version of the library when it was compiled.
 *
 * This function returns the library version when it was compiled as a string.
 * It can be used to compare against a version you support (i.e. the version
 * your software was compiled against could be different.)
 *
 * \return A constant string that represents the library version.
 */
char const * version()
{
    return CASSVALUE_LIBRARY_VERSION_STRING;
}

/** \brief Return the major version number.
 *
 * This function returns the library major version number when it was compiled
 * as an integer. It can be used to compare against a version you support.
 *
 * \return The library major version number when it was compiled.
 */
int version_major()
{
    return CASSVALUE_LIBRARY_VERSION_MAJOR;
}

/** \brief Return the minor version number.
 *
 * This function returns the library minor version number when it was compiled
 * as an integer. It can be used to compare against a version you support.
 *
 * \return The library minor version number when it was compiled.
 */
int version_minor()
{
    return CASSVALUE_LIBRARY_VERSION_MINOR;
}

/** \brief Return the patch version number.
 *
 * This function returns the library patch version number when it was compiled
 * as an integer. It can be used to compare against a version you support.
 *
 * \return The library minor version number when it was compiled.
 */
int version_patch()
{
    return CASSVALUE_LIBRARY_VERSION_PATCH;
}


} // namespace cassvalue
// vim: ts=4 sw=4 et
