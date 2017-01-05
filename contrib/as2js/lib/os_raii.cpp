/* os_raii.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/os_raii.h"


namespace as2js
{


/** \class raii_stream_flags
 * \brief A class used to safely handle stream flags, width, and precision.
 *
 * Create an object of this type on your stack, and the flags, width,
 * and precision of your standard streams will be safe-guarded.
 *
 * See the constructor for an example.
 */


/** \brief Save the current format flags, width, and precision of a stream.
 *
 * This function saves the flags, precision, and width of a
 * stream inside this object so as to restore them later.
 *
 * The destructor will automatically restore the flags. The
 * restore() function can also be called early, although that
 * will eventually break the RAII feature since restore only
 * restores the flags once. Further calls to the restore()
 * function do nothing.
 *
 * To use:
 *
 * \code
 *   {
 *      as2js::raii_stream_flags stream_flags(std::cout);
 *      ...
 *      // this changes the flags to write numbers in hexadecimal
 *      std::cout << std::hex << 123 << ...;
 *      ...
 *   } // here all flags, width, precision get restored automatically
 * \endcode
 *
 * \bug
 * This class does not know about the fill character.
 *
 * \param[in] s  The stream of which flags are to be saved.
 */
raii_stream_flags::raii_stream_flags(std::ios_base & s)
    : f_stream(&s)
    , f_flags(s.flags())
    , f_precision(s.precision())
    , f_width(s.width())
{
}


/** \brief Restore the flags, width, and precision of a stream.
 *
 * The destructor automatically restores the stream flags, width,
 * and precision when called. Putting such an object on the stack
 * is the safest way to make sure that your function does not leak
 * the stream flags, width, and precision.
 *
 * This function calls the restore() function. Note that restore()
 * has no effect when called more than once.
 */
raii_stream_flags::~raii_stream_flags()
{
    restore();
}


/** \brief The restore function copies the flags, width, and precision
 *         back in the stream.
 *
 * This function restores the flags, width, and precision of the stream
 * as they were when the object was passed to the constructor of this
 * object.
 *
 * The function can be called any number of time, however, it only
 * restores the flags, width, and precision the first time it is called.
 *
 * In most cases, you want to let your raii_stream_flags object
 * destructor call this restore() function automatically, although
 * you may need to restore the format early once in a while.
 */
void raii_stream_flags::restore()
{
    if(f_stream)
    {
        f_stream->flags(f_flags);
        f_stream->precision(f_precision);
        f_stream->width(f_width);
        f_stream = nullptr;
    }
}


}
// namespace as2js

// vim: ts=4 sw=4 et
