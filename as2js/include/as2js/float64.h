#ifndef AS2JS_FLOAT64_H
#define AS2JS_FLOAT64_H
/* float64.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/compare.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include    <controlled_vars/controlled_vars_fauto_init.h>
#pragma GCC diagnostic pop

#include    <limits>
#include    <cmath>


namespace as2js
{

class Float64
{
public:
    typedef double  float64_type;

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

    void            set_NaN()
                    {
                        f_float = std::numeric_limits<Float64::float64_type>::quiet_NaN();
                    }

    void            set_infinity()
                    {
                        f_float = std::numeric_limits<Float64::float64_type>::infinity();
                    }

    bool            is_NaN() const
                    {
                        return std::isnan(f_float);
                    }

    bool            is_infinity() const
                    {
                        return std::isinf(f_float);
                    }

    bool            is_positive_infinity() const
                    {
                        return std::isinf(f_float) && !std::signbit(f_float);
                    }

    bool            is_negative_infinity() const
                    {
                        return std::isinf(f_float) && std::signbit(f_float);
                    }

    int             classified_infinity() const
                    {
                        // if infinity, return -1 or +1
                        // if not infinity, return 0
                        return std::isinf(f_float)
                                ? (std::signbit(f_float) ? -1 : 1)
                                : 0;
                    }

    compare_t       compare(Float64 const& rhs) const
                    {
                        // if we got a NaN, it's not ordered
                        if(is_NaN() || rhs.is_NaN())
                        {
                            return compare_t::COMPARE_UNORDERED;
                        }

                        // comparing two floats properly handles infinity
                        // (at least in g++ on Intel processors)
                        return f_float == rhs.f_float ? compare_t::COMPARE_EQUAL
                             : (f_float < rhs.f_float ? compare_t::COMPARE_LESS
                                                      : compare_t::COMPARE_GREATER);
                    }

private:
    controlled_vars::fauto_init<float64_type>    f_float;
};


}
// namespace as2js
#endif
// #ifndef AS2JS_FLOAT64_H

// vim: ts=4 sw=4 et
