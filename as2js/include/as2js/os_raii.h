#ifndef AS2JS_OS_RAII_H
#define AS2JS_OS_RAII_H
/* os_raii.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2015 */

/*

Copyright (c) 2005-2015 Made to Order Software Corp.

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

#include    <controlled_vars/controlled_vars_need_enum_init.h>
#include    <controlled_vars/controlled_vars_ptr_need_init.h>

namespace as2js
{


class raii_stream_flags
{
public:
                            raii_stream_flags(std::ios_base& s);
                            ~raii_stream_flags();

    void                    restore();

private:
    typedef controlled_vars::ptr_need_init<std::ios_base>           mpstream_t;
    typedef controlled_vars::need_enum_init<std::ios_base::fmtflags>     mfmtflags_t;

    mpstream_t              f_stream;
    mfmtflags_t             f_flags;
};


}
// namespace as2js
#endif
// #ifndef AS2JS_OS_RAII_H

// vim: ts=4 sw=4 et
