#ifndef AS2JS_FLOAT64_H
#define AS2JS_FLOAT64_H
/* float64.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/compare.h"

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

    float64_type    get() const
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                        return f_float == rhs.f_float ? compare_t::COMPARE_EQUAL
                             : (f_float < rhs.f_float ? compare_t::COMPARE_LESS
                                                      : compare_t::COMPARE_GREATER);
#pragma GCC diagnostic pop
                    }

    static float64_type default_epsilon()
                    {
                        return 0.00001;
                    }

    bool            nearly_equal(Float64 const& rhs, float64_type epsilon = default_epsilon())
                    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                        // already equal?
                        if(f_float == rhs.f_float)
                        {
                            return true;
                        }

                        float64_type const diff = fabs(f_float - rhs.f_float);
                        if(f_float == 0.0
                        || rhs.f_float == 0.0
                        || diff < std::numeric_limits<float64_type>::min())
                        {
                            return diff < (epsilon * std::numeric_limits<float64_type>::min());
                        }
#pragma GCC diagnostic pop

                        return diff / (fabs(f_float) + fabs(rhs.f_float)) < epsilon;
                    }
 

private:
    float64_type    f_float = 0.0;
};


}
// namespace as2js
#endif
// #ifndef AS2JS_FLOAT64_H

// vim: ts=4 sw=4 et
