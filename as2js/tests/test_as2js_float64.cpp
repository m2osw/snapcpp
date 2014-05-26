/* test_as2js_float64.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "test_as2js_float64.h"
#include    "test_as2js_main.h"

#include    "as2js/float64.h"
#include    "as2js/exceptions.h"

#include    <cstring>
#include    <algorithm>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsFloat64UnitTests );



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"


void As2JsFloat64UnitTests::test_float64()
{
    // default constructor gives us zero
    {
        as2js::Float64 zero;
        CPPUNIT_ASSERT(zero.get() == 0.0);
    }

    // float constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 64 bit number
        float s1(rand() & 1 ? -1 : 1);
        float n1(static_cast<float>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        float d1(static_cast<float>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        float r(n1 / d1 * s1);
        as2js::Float64 random(r);
        CPPUNIT_ASSERT(random.get() == r);
        CPPUNIT_ASSERT(!random.is_NaN());
        CPPUNIT_ASSERT(!random.is_infinity());
        CPPUNIT_ASSERT(!random.is_positive_infinity());
        CPPUNIT_ASSERT(!random.is_negative_infinity());
        CPPUNIT_ASSERT(random.classified_infinity() == 0);

        as2js::Float64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);
        CPPUNIT_ASSERT(!copy.is_NaN());
        CPPUNIT_ASSERT(!copy.is_infinity());
        CPPUNIT_ASSERT(!copy.is_positive_infinity());
        CPPUNIT_ASSERT(!copy.is_negative_infinity());
        CPPUNIT_ASSERT(copy.classified_infinity() == 0);

        float s2(rand() & 1 ? -1 : 1);
        float n2(static_cast<float>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        float d2(static_cast<float>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        float q(n2 / d2 * s2);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);
        CPPUNIT_ASSERT(!random.is_NaN());
        CPPUNIT_ASSERT(!random.is_infinity());
        CPPUNIT_ASSERT(!random.is_positive_infinity());
        CPPUNIT_ASSERT(!random.is_negative_infinity());
        CPPUNIT_ASSERT(random.classified_infinity() == 0);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::COMPARE_EQUAL);
        }

        float s3(rand() & 1 ? -1 : 1);
        float n3(static_cast<float>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        float d3(static_cast<float>((static_cast<int64_t>(rand()) << 48)
                                  ^ (static_cast<int64_t>(rand()) << 32)
                                  ^ (static_cast<int64_t>(rand()) << 16)
                                  ^ (static_cast<int64_t>(rand()) <<  0)));
        float p(n3 / d3 * s3);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
        CPPUNIT_ASSERT(!random.is_NaN());
        CPPUNIT_ASSERT(!random.is_infinity());
        CPPUNIT_ASSERT(!random.is_positive_infinity());
        CPPUNIT_ASSERT(!random.is_negative_infinity());
        CPPUNIT_ASSERT(random.classified_infinity() == 0);
    }

    // double constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 64 bit number
        double s1(rand() & 1 ? -1 : 1);
        double n1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                    ^ (static_cast<int64_t>(rand()) << 32)
                                    ^ (static_cast<int64_t>(rand()) << 16)
                                    ^ (static_cast<int64_t>(rand()) <<  0)));
        double d1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                    ^ (static_cast<int64_t>(rand()) << 32)
                                    ^ (static_cast<int64_t>(rand()) << 16)
                                    ^ (static_cast<int64_t>(rand()) <<  0)));
        double r(n1 / d1 * s1);
        as2js::Float64 random(r);
        CPPUNIT_ASSERT(random.get() == r);
        CPPUNIT_ASSERT(!random.is_NaN());
        CPPUNIT_ASSERT(!random.is_infinity());
        CPPUNIT_ASSERT(!random.is_positive_infinity());
        CPPUNIT_ASSERT(!random.is_negative_infinity());
        CPPUNIT_ASSERT(random.get() != std::numeric_limits<double>::quiet_NaN());
        CPPUNIT_ASSERT(random.classified_infinity() == 0);

        as2js::Float64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);
        CPPUNIT_ASSERT(!copy.is_NaN());
        CPPUNIT_ASSERT(!copy.is_infinity());
        CPPUNIT_ASSERT(!copy.is_positive_infinity());
        CPPUNIT_ASSERT(!copy.is_negative_infinity());
        CPPUNIT_ASSERT(copy.get() != std::numeric_limits<double>::quiet_NaN());
        CPPUNIT_ASSERT(copy.classified_infinity() == 0);

        double s2(rand() & 1 ? -1 : 1);
        double n2(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                    ^ (static_cast<int64_t>(rand()) << 32)
                                    ^ (static_cast<int64_t>(rand()) << 16)
                                    ^ (static_cast<int64_t>(rand()) <<  0)));
        double d2(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                    ^ (static_cast<int64_t>(rand()) << 32)
                                    ^ (static_cast<int64_t>(rand()) << 16)
                                    ^ (static_cast<int64_t>(rand()) <<  0)));
        double q(n2 / d2 * s2);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);
        CPPUNIT_ASSERT(!random.is_NaN());
        CPPUNIT_ASSERT(!random.is_infinity());
        CPPUNIT_ASSERT(!random.is_positive_infinity());
        CPPUNIT_ASSERT(!random.is_negative_infinity());
        CPPUNIT_ASSERT(random.get() != std::numeric_limits<double>::quiet_NaN());
        CPPUNIT_ASSERT(random.classified_infinity() == 0);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::COMPARE_EQUAL);
        }

        double s3(rand() & 1 ? -1 : 1);
        double n3(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                    ^ (static_cast<int64_t>(rand()) << 32)
                                    ^ (static_cast<int64_t>(rand()) << 16)
                                    ^ (static_cast<int64_t>(rand()) <<  0)));
        double d3(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                    ^ (static_cast<int64_t>(rand()) << 32)
                                    ^ (static_cast<int64_t>(rand()) << 16)
                                    ^ (static_cast<int64_t>(rand()) <<  0)));
        double p(n3 / d3 * s3);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
        CPPUNIT_ASSERT(!random.is_NaN());
        CPPUNIT_ASSERT(!random.is_infinity());
        CPPUNIT_ASSERT(!random.is_positive_infinity());
        CPPUNIT_ASSERT(!random.is_negative_infinity());
        CPPUNIT_ASSERT(random.get() != std::numeric_limits<double>::quiet_NaN());
        CPPUNIT_ASSERT(random.classified_infinity() == 0);
    }
}


void As2JsFloat64UnitTests::test_special_numbers()
{
    as2js::Float64 special;

    // start with zero
    CPPUNIT_ASSERT(special.get() == 0.0);

    // create a random number to compare with
    double s1(rand() & 1 ? -1 : 1);
    double n1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                ^ (static_cast<int64_t>(rand()) << 32)
                                ^ (static_cast<int64_t>(rand()) << 16)
                                ^ (static_cast<int64_t>(rand()) <<  0)));
    double d1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                ^ (static_cast<int64_t>(rand()) << 32)
                                ^ (static_cast<int64_t>(rand()) << 16)
                                ^ (static_cast<int64_t>(rand()) <<  0)));
    double p(n1 / d1 * s1);
    as2js::Float64 r(p);

    // test NaN
    special.set_NaN();
    CPPUNIT_ASSERT(special.is_NaN());
    CPPUNIT_ASSERT(!special.is_infinity());
    CPPUNIT_ASSERT(!special.is_positive_infinity());
    CPPUNIT_ASSERT(!special.is_negative_infinity());
    CPPUNIT_ASSERT(special.get() != 0.0);
    CPPUNIT_ASSERT(special.get() != p);
    CPPUNIT_ASSERT(!(special.get() == p));
    CPPUNIT_ASSERT(!(special.get() > p));
    CPPUNIT_ASSERT(!(special.get() >= p));
    CPPUNIT_ASSERT(!(special.get() < p));
    CPPUNIT_ASSERT(!(special.get() <= p));
    // We do not offer those yet
    //CPPUNIT_ASSERT(special != r);
    //CPPUNIT_ASSERT(!(special == r));
    //CPPUNIT_ASSERT(!(special > r));
    //CPPUNIT_ASSERT(!(special >= r));
    //CPPUNIT_ASSERT(!(special < r));
    //CPPUNIT_ASSERT(!(special <= r));
    CPPUNIT_ASSERT(special.get() != std::numeric_limits<double>::quiet_NaN());
    CPPUNIT_ASSERT(special.compare(p) == as2js::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(special.compare(r) == as2js::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(r.compare(special) == as2js::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(special.classified_infinity() == 0);

    // test Infinity
    special.set_infinity(); // +inf
    CPPUNIT_ASSERT(!special.is_NaN());
    CPPUNIT_ASSERT(special.is_infinity());
    CPPUNIT_ASSERT(special.is_positive_infinity());
    CPPUNIT_ASSERT(!special.is_negative_infinity());
    CPPUNIT_ASSERT(special.get() != 0.0);
    CPPUNIT_ASSERT(special.get() != p);
    CPPUNIT_ASSERT(!(special.get() == p));
    CPPUNIT_ASSERT(special.get() > p);
    CPPUNIT_ASSERT(special.get() >= p);
    CPPUNIT_ASSERT(!(special.get() < p));
    CPPUNIT_ASSERT(!(special.get() <= p));
    // We do not offer those yet
    //CPPUNIT_ASSERT(special != r);
    //CPPUNIT_ASSERT(!(special == r));
    //CPPUNIT_ASSERT(!(special > r));
    //CPPUNIT_ASSERT(!(special >= r));
    //CPPUNIT_ASSERT(!(special < r));
    //CPPUNIT_ASSERT(!(special <= r));
    CPPUNIT_ASSERT(special.get() != std::numeric_limits<double>::quiet_NaN());
    CPPUNIT_ASSERT(special.compare(p) == as2js::COMPARE_GREATER);
    CPPUNIT_ASSERT(special.compare(r) == as2js::COMPARE_GREATER);
    CPPUNIT_ASSERT(r.compare(special) == as2js::COMPARE_LESS);
    CPPUNIT_ASSERT(special.classified_infinity() == 1);

    as2js::Float64 pinf;
    pinf.set_infinity();
    CPPUNIT_ASSERT(pinf.compare(special) == as2js::COMPARE_EQUAL);
    CPPUNIT_ASSERT(special.compare(pinf) == as2js::COMPARE_EQUAL);

    special.set(-special.get()); // -inf
    CPPUNIT_ASSERT(!special.is_NaN());
    CPPUNIT_ASSERT(special.is_infinity());
    CPPUNIT_ASSERT(!special.is_positive_infinity());
    CPPUNIT_ASSERT(special.is_negative_infinity());
    CPPUNIT_ASSERT(special.get() != 0.0);
    CPPUNIT_ASSERT(special.get() != p);
    CPPUNIT_ASSERT(!(special.get() == p));
    CPPUNIT_ASSERT(!(special.get() > p));
    CPPUNIT_ASSERT(!(special.get() >= p));
    CPPUNIT_ASSERT(special.get() < p);
    CPPUNIT_ASSERT(special.get() <= p);
    // We do not offer those yet
    //CPPUNIT_ASSERT(special != r);
    //CPPUNIT_ASSERT(!(special == r));
    //CPPUNIT_ASSERT(!(special > r));
    //CPPUNIT_ASSERT(!(special >= r));
    //CPPUNIT_ASSERT(!(special < r));
    //CPPUNIT_ASSERT(!(special <= r));
    CPPUNIT_ASSERT(special.get() != std::numeric_limits<double>::quiet_NaN());
    CPPUNIT_ASSERT(special.compare(p) == as2js::COMPARE_LESS);
    CPPUNIT_ASSERT(special.compare(r) == as2js::COMPARE_LESS);
    CPPUNIT_ASSERT(r.compare(special) == as2js::COMPARE_GREATER);
    CPPUNIT_ASSERT(special.classified_infinity() == -1);

    CPPUNIT_ASSERT(pinf.compare(special) != as2js::COMPARE_EQUAL);
    CPPUNIT_ASSERT(special.compare(pinf) != as2js::COMPARE_EQUAL);
}


// vim: ts=4 sw=4 et
