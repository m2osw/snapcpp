#ifndef AS2JS_INT64_H
#define AS2JS_INT64_H
/* int64.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "compare.h"

#include    <cstdint>


namespace as2js
{

class Int64
{
public:
    typedef int64_t int64_type;

                    Int64()
                    {
                    }

                    Int64(int64_type const rhs)
                    {
                        f_int = rhs;
                    }

                    Int64(Int64 const& rhs)
                    {
                        f_int = rhs.f_int;
                    }

    Int64&          operator = (Int64 const& rhs)
                    {
                        f_int = rhs.f_int;
                        return *this;
                    }

    int64_type      get() const
                    {
                        return f_int;
                    }

    void            set(int64_type const new_int)
                    {
                        f_int = new_int;
                    }

    compare_t       compare(Int64 const& rhs) const
                    {
                        return f_int == rhs.f_int ? compare_t::COMPARE_EQUAL
                             : (f_int < rhs.f_int ? compare_t::COMPARE_LESS
                                                  : compare_t::COMPARE_GREATER);
                    }

private:
    int64_type      f_int = 0;
};


}
// namespace as2js
#endif
// #ifndef AS2JS_INT64_H

// vim: ts=4 sw=4 et
