#ifndef AS2JS_OS_RAII_H
#define AS2JS_OS_RAII_H
/* os_raii.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    <ios>

namespace as2js
{


class raii_stream_flags
{
public:
                            raii_stream_flags(std::ios_base & s);
                            ~raii_stream_flags();

    void                    restore();

private:
    std::ios_base *         f_stream = nullptr;
    std::ios_base::fmtflags f_flags = std::ios_base::fmtflags();
    std::streamsize         f_precision = 0;
    std::streamsize         f_width = 0;
};


}
// namespace as2js
#endif
// #ifndef AS2JS_OS_RAII_H

// vim: ts=4 sw=4 et
