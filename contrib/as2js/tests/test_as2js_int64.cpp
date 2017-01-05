/* test_as2js_int64.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_int64.h"
#include    "test_as2js_main.h"

#include    "as2js/int64.h"
#include    "as2js/exceptions.h"

#include    <cstring>
#include    <algorithm>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsInt64UnitTests );





void As2JsInt64UnitTests::test_int64()
{
    // default constructor gives us zero
    {
        as2js::Int64   zero;
        CPPUNIT_ASSERT(zero.get() == 0);
    }

    // int8_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 16 bit number
        int8_t r(rand());

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        int8_t q;
        q = static_cast<int8_t>(rand());

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        int8_t p;
        p = static_cast<int8_t>(rand());

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // uint8_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 16 bit number
        uint8_t r(rand());

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        uint8_t q;
        q = static_cast<uint8_t>(rand());

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        uint8_t p;
        p = static_cast<uint8_t>(rand());

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // int16_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 16 bit number
        int16_t r(rand());

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        int16_t q;
        q = (static_cast<int16_t>(rand()) << 16)
          ^ (static_cast<int16_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        int16_t p;
        p = (static_cast<int16_t>(rand()) << 16)
          ^ (static_cast<int16_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // uint16_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 16 bit number
        uint16_t r(rand());

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        uint16_t q;
        q = (static_cast<uint16_t>(rand()) << 16)
          ^ (static_cast<uint16_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        uint16_t p;
        p = (static_cast<uint16_t>(rand()) << 16)
          ^ (static_cast<uint16_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // int32_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 32 bit number
        int32_t r;
        r = (static_cast<int32_t>(rand()) << 16)
          ^ (static_cast<int32_t>(rand()) <<  0);

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        int32_t q;
        q = (static_cast<int32_t>(rand()) << 16)
          ^ (static_cast<int32_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        int32_t p;
        p = (static_cast<int32_t>(rand()) << 16)
          ^ (static_cast<int32_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // uint32_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 32 bit number
        uint32_t r;
        r = (static_cast<uint32_t>(rand()) << 16)
          ^ (static_cast<uint32_t>(rand()) <<  0);

        // sign extends properly?
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        // copy works as expected?
        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        uint32_t q;
        q = (static_cast<uint32_t>(rand()) << 16)
          ^ (static_cast<uint32_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        uint32_t p;
        p = (static_cast<uint32_t>(rand()) << 16)
          ^ (static_cast<uint32_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // int64_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 64 bit number
        int64_t r;
        r = (static_cast<int64_t>(rand()) << 48)
          ^ (static_cast<int64_t>(rand()) << 32)
          ^ (static_cast<int64_t>(rand()) << 16)
          ^ (static_cast<int64_t>(rand()) <<  0);
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == r);

        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == r);

        int64_t q;
        q = (static_cast<int64_t>(rand()) << 48)
          ^ (static_cast<int64_t>(rand()) << 32)
          ^ (static_cast<int64_t>(rand()) << 16)
          ^ (static_cast<int64_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == q);

        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(q < r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(q > r)
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        int64_t p;
        p = (static_cast<int64_t>(rand()) << 48)
          ^ (static_cast<int64_t>(rand()) << 32)
          ^ (static_cast<int64_t>(rand()) << 16)
          ^ (static_cast<int64_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == p);
    }

    // uint64_t constructor, copy constructor, copy assignment
    for(int i(0); i < 1000; ++i)
    {
        // generate a random 64 bit number
        uint64_t r;
        r = (static_cast<uint64_t>(rand()) << 48)
          ^ (static_cast<uint64_t>(rand()) << 32)
          ^ (static_cast<uint64_t>(rand()) << 16)
          ^ (static_cast<uint64_t>(rand()) <<  0);
        as2js::Int64 random(r);
        CPPUNIT_ASSERT(random.get() == static_cast<int64_t>(r));

        as2js::Int64 copy(random);
        CPPUNIT_ASSERT(copy.get() == static_cast<int64_t>(r));

        uint64_t q;
        q = (static_cast<uint64_t>(rand()) << 48)
          ^ (static_cast<uint64_t>(rand()) << 32)
          ^ (static_cast<uint64_t>(rand()) << 16)
          ^ (static_cast<uint64_t>(rand()) <<  0);

        random = q;
        CPPUNIT_ASSERT(random.get() == static_cast<int64_t>(q));

        // Here the compare expects the signed int64_t...
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(random.compare(copy)));
        CPPUNIT_ASSERT(as2js::compare_utils::is_ordered(copy.compare(random)));
        if(static_cast<int64_t>(q) < static_cast<int64_t>(r))
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_LESS);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_GREATER);
        }
        else if(static_cast<int64_t>(q) > static_cast<int64_t>(r))
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_GREATER);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_LESS);
        }
        else
        {
            CPPUNIT_ASSERT(random.compare(copy) == as2js::compare_t::COMPARE_EQUAL);
            CPPUNIT_ASSERT(copy.compare(random) == as2js::compare_t::COMPARE_EQUAL);
        }

        uint64_t p;
        p = (static_cast<uint64_t>(rand()) << 48)
          ^ (static_cast<uint64_t>(rand()) << 32)
          ^ (static_cast<uint64_t>(rand()) << 16)
          ^ (static_cast<uint64_t>(rand()) <<  0);

        random.set(p);
        CPPUNIT_ASSERT(random.get() == static_cast<int64_t>(p));
    }
}




// vim: ts=4 sw=4 et
