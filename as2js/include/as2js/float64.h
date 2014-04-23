#ifndef AS2JS_FLOAT64_H
#define AS2JS_FLOAT64_H
/* as.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

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

#include    <controlled_vars/controlled_vars_fauto_init.h>

namespace as2js
{

class Float64
{
public:
    typedef double      float64_type;

                    Float64()
                        // : f_float(0.0) auto-init
                    {
                    }

                    Float64(float64_type const rhs)
                    {
                        f_float = rhs;
                    }

                    Float64(Float64 const& rhs)
                    {
                        f_float = rhs.f_float;
                    }

    Float64&        operator = (Float64 const& rhs)
                    {
                        f_float = rhs.f_float;
                        return *this;
                    }

    double          get() const
                    {
                        return f_float;
                    }

    void            set(float64_type const new_float)
                    {
                        f_float = new_float;
                    }

private:
    controlled_vars::fauto_init<float64_type>    f_float;
};


}
// namespace as2js
#endif
// #ifndef AS2JS_FLOAT64_H

// vim: ts=4 sw=4 et
