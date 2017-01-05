/* test_as2js_string.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_string.h"
#include    "test_as2js_main.h"

#include    "as2js/string.h"
#include    "as2js/exceptions.h"

#include    <cstring>
#include    <algorithm>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsStringUnitTests );

namespace
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
#pragma GCC diagnostic ignored "-Woverflow"
char const iso8859_1_bad_start[] = { 0xA0, 0xA1, 0xA2, 0 };
char const iso8859_1_bom_and_bad_start[] = { 0xEF, 0xBB, 0xBF, 0xA0, 0xA1, 0xA2, 0 };
#pragma GCC diagnostic pop
wchar_t const utf16_to_append[] = { 0x1111, 0x2222, 0x3333, 0 };
as2js::as_char_t const utf32_to_append[] = { 0x101111, 0x5555, 0x103333, 0 };


bool compare_chars(char const *a, as2js::as_char_t const *b)
{
    for(; *a != '\0' && *b != '\0'; ++a, ++b)
    {
        if(static_cast<unsigned char>(*a) != *b)
        {
            return false;
        }
    }

    return static_cast<unsigned char>(*a) == *b;
}

int wctombs(char *mb, uint32_t wc)
{
    if(wc < 0x80)
    {
        /* this will also encode '\0'... */
        mb[0] = static_cast<char>(wc);
        mb[1] = '\0';
        return 1;
    }
    if(wc < 0x800)
    {
        mb[0] = static_cast<char>((wc >> 6) | 0xC0);
        mb[1] = (wc & 0x3F) | 0x80;
        mb[2] = '\0';
        return 2;
    }
    if(wc < 0x10000)
    {
        mb[0] = static_cast<char>((wc >> 12) | 0xE0);
        mb[1] = ((wc >> 6) & 0x3F) | 0x80;
        mb[2] = (wc & 0x3F) | 0x80;
        mb[3] = '\0';
        return 3;
    }
    if(wc < 0x200000)
    {
        mb[0] = static_cast<char>((wc >> 18) | 0xF0);
        mb[1] = ((wc >> 12) & 0x3F) | 0x80;
        mb[2] = ((wc >> 6) & 0x3F) | 0x80;
        mb[3] = (wc & 0x3F) | 0x80;
        mb[4] = '\0';
        return 4;
    }
    if(wc < 0x4000000)
    {
        mb[0] = (wc >> 24) | 0xF8;
        mb[1] = ((wc >> 18) & 0x3F) | 0x80;
        mb[2] = ((wc >> 12) & 0x3F) | 0x80;
        mb[3] = ((wc >> 6) & 0x3F) | 0x80;
        mb[4] = (wc & 0x3F) | 0x80;
        mb[5] = '\0';
        return 5;
    }
    if(static_cast<int32_t>(wc) > 0)    // <=> (uint32_t) wc < 0x80000000
    {
        mb[0] = (wc >> 30) | 0xFC;
        mb[1] = ((wc >> 24) & 0x3F) | 0x80;
        mb[2] = ((wc >> 18) & 0x3F) | 0x80;
        mb[3] = ((wc >> 12) & 0x3F) | 0x80;
        mb[4] = ((wc >> 6) & 0x3F) | 0x80;
        mb[5] = (wc & 0x3F) | 0x80;
        mb[6] = '\0';
        return 6;
    }

    /* an invalid wide character (negative!) simply not encoded */
    mb[0] = '\0';
    return 0;
}

std::string wcstombs(as2js::String const& wcs)
{
    std::string mbs;

    for(as2js::as_char_t const *s(wcs.c_str()); *s != '\0'; ++s)
    {
        char buf[8];
        wctombs(buf, *s);
        mbs += buf;
    }

    return mbs;
}

int mbstowc(uint32_t& wc, char const *& mb, size_t& len)
{
    // define a default output character of NUL
    wc = L'\0';

    // done?
    if(len <= 0)
    {
        return 0;
    }

    // we eat one character from the source minimum
    unsigned char c(*mb++);
    len--;

    if(c < 0x80)
    {
        wc = c;
        return 1;
    }

    // invalid stream?
    if((c >= 0x80 && c <= 0xBF) || c == 0xFE || c == 0xFF)
    {
        // this is bad UTF-8, skip all the invalid bytes
        while(len > 0 && (c = *mb, (c >= 0x80 && c < 0xBF) || c == 0xFE || c == 0xFF))
        {
            mb++;
            len--;
        }
        return -2;
    }

    // use a uint32_t because some wchar_t are not wide
    // enough; generate an error later if that's the case
    // (we are trying to go to UCS-4, not UTF-16, but MS-Windows
    // really only supports UCS-2.)
    uint32_t w(L'\0');
    size_t cnt;

    // note that in current versions of UTF-8 0xFC and 0xF8
    // are not considered valid because they accept a maximum
    // of 20 bites instead of 31
    if(c >= 0xFC)
    {
        w = c & 0x01;
        cnt = 5;
    }
    else if(c >= 0xF8)
    {
        w = c & 0x03;
        cnt = 4;
    }
    else if(c >= 0xF0)
    {
        w = c & 0x07;
        cnt = 3;
    }
    else if(c >= 0xE0)
    {
        w = c & 0x0F;
        cnt = 2;
    }
    else if(c >= 0xC0)
    {
        w = c & 0x1F;
        cnt = 1;
    }
    else
    {
        throw std::logic_error("c < 0xC0 when it should not be");
    }

    // enough data in the input? if not, that's an error
    if(len < cnt)
    {
        while(len > 0 && (c = *mb, c >= 0x80 && c <= 0xBF))
        {
            len--;
            mb++;
        }
        return -1;
    }
    len -= cnt;

    for(size_t l(cnt); l > 0; --l, mb++)
    {
        c = *mb;
        if(c < 0x80 || c > 0xBF)
        {
            // we got an invalid sequence!
            // restore whatever is left in len
            len += l;
            return -3;
        }
        w = (w << 6) | (c & 0x3F);
    }

    wc = w;

    return static_cast<int>(cnt + 1);
}

as2js::String mbstowcs(std::string const& mbs)
{
    as2js::String wcs;

    size_t len(mbs.length());
    char const *s(mbs.c_str());
    while(*s != '\0')
    {
        uint32_t wc;
        int const l(mbstowc(wc, s, len));
        if(l > 0)
        {
            wcs += static_cast<as2js::as_char_t>(wc);
        }
    }

    return wcs;
}

bool close_double(double a, double b, double epsilon)
{
    return a >= b - epsilon && a <= b + epsilon;
}

}
// no name namespace




void As2JsStringUnitTests::test_iso88591()
{
    // to know whether code checks for UTF-8 we should provide
    // invalid input

    // a little extra test, make sure a string is empty on
    // creation without anything
    {
        as2js::String str1;
        CPPUNIT_ASSERT(str1.empty());
        CPPUNIT_ASSERT(str1.length() == 0);
        CPPUNIT_ASSERT(str1.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str1);
        CPPUNIT_ASSERT(str1 == "");
        CPPUNIT_ASSERT(!("" != str1));
        CPPUNIT_ASSERT(!(str1 != ""));
        CPPUNIT_ASSERT(str1.valid());

        as2js::String str2("");
        CPPUNIT_ASSERT(str2.empty());
        CPPUNIT_ASSERT(str2.length() == 0);
        CPPUNIT_ASSERT(str2.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str2);
        CPPUNIT_ASSERT(str2 == "");
        CPPUNIT_ASSERT(!("" != str2));
        CPPUNIT_ASSERT(!(str2 != ""));
        CPPUNIT_ASSERT(str2.valid());

        as2js::String str3(str1); // and a copy
        CPPUNIT_ASSERT(str3.empty());
        CPPUNIT_ASSERT(str3.length() == 0);
        CPPUNIT_ASSERT(str3.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str3);
        CPPUNIT_ASSERT(str3 == "");
        CPPUNIT_ASSERT(!("" != str3));
        CPPUNIT_ASSERT(!(str3 != ""));
        CPPUNIT_ASSERT(str3.valid());

        std::string std_empty;
        as2js::String str4(std_empty); // and a copy from an std::string
        CPPUNIT_ASSERT(str4.empty());
        CPPUNIT_ASSERT(str4.length() == 0);
        CPPUNIT_ASSERT(str4.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str4);
        CPPUNIT_ASSERT(str4 == "");
        CPPUNIT_ASSERT(!("" != str4));
        CPPUNIT_ASSERT(!(str4 != ""));
        CPPUNIT_ASSERT(str4.valid());

        as2js::String str5;
        CPPUNIT_ASSERT(str5.from_char("ignored", 0) == as2js::String::conversion_result_t::STRING_GOOD);
        CPPUNIT_ASSERT(str5.empty());
        CPPUNIT_ASSERT(str5.length() == 0);
        CPPUNIT_ASSERT(str5.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str5);
        CPPUNIT_ASSERT(str5 == "");
        CPPUNIT_ASSERT(!("" != str5));
        CPPUNIT_ASSERT(!(str5 != ""));
        CPPUNIT_ASSERT(str5.valid());

        as2js::String str6;
        CPPUNIT_ASSERT(str6.from_char("", 5) == as2js::String::conversion_result_t::STRING_GOOD);
        CPPUNIT_ASSERT(str6.empty());
        CPPUNIT_ASSERT(str6.length() == 0);
        CPPUNIT_ASSERT(str6.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str6);
        CPPUNIT_ASSERT(str6 == "");
        CPPUNIT_ASSERT(!("" != str6));
        CPPUNIT_ASSERT(!(str6 != ""));
        CPPUNIT_ASSERT(str6.valid());

        as2js::String str7;
        CPPUNIT_ASSERT(str7.from_char("") == as2js::String::conversion_result_t::STRING_GOOD);
        CPPUNIT_ASSERT(str7.empty());
        CPPUNIT_ASSERT(str7.length() == 0);
        CPPUNIT_ASSERT(str7.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str7);
        CPPUNIT_ASSERT(str7 == "");
        CPPUNIT_ASSERT(!("" != str7));
        CPPUNIT_ASSERT(!(str7 != ""));
        CPPUNIT_ASSERT(str7.valid());

        as2js::String str8;
        CPPUNIT_ASSERT(&(str8 = "") == &str8);
        CPPUNIT_ASSERT(str8.empty());
        CPPUNIT_ASSERT(str8.length() == 0);
        CPPUNIT_ASSERT(str8.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str8);
        CPPUNIT_ASSERT(str8 == "");
        CPPUNIT_ASSERT(!("" != str8));
        CPPUNIT_ASSERT(!(str8 != ""));
        CPPUNIT_ASSERT(str8.valid());

        char const *null_char_ptr(nullptr);
        as2js::String str9(null_char_ptr, 4);
        CPPUNIT_ASSERT(&(str9 = "") == &str9);
        CPPUNIT_ASSERT(str9.empty());
        CPPUNIT_ASSERT(str9.length() == 0);
        CPPUNIT_ASSERT(str9.utf8_length() == 0);
        CPPUNIT_ASSERT(!("" != str9));
        CPPUNIT_ASSERT(!(str9 != ""));
        CPPUNIT_ASSERT(str9.valid());

        as2js::String str10;
        str10.from_char(null_char_ptr, 6);
        CPPUNIT_ASSERT(&(str10 = "") == &str10);
        CPPUNIT_ASSERT(str10.empty());
        CPPUNIT_ASSERT(str10.length() == 0);
        CPPUNIT_ASSERT(str10.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str10);
        CPPUNIT_ASSERT(str10 == "");
        CPPUNIT_ASSERT(!("" != str10));
        CPPUNIT_ASSERT(!(str10 != ""));
        CPPUNIT_ASSERT(str10.valid());
    }

    // characters between 0x80 and 0xBF are only to chain UTF-8
    // codes, so if they start a string and are accepted, we're
    // not in UTF-8
    {
        // constructor
        as2js::String str1(iso8859_1_bad_start);
        CPPUNIT_ASSERT(strlen(iso8859_1_bad_start) == str1.length());
        CPPUNIT_ASSERT(compare_chars(iso8859_1_bad_start, str1.c_str()));
        CPPUNIT_ASSERT(iso8859_1_bad_start == str1);
        CPPUNIT_ASSERT(!(iso8859_1_bad_start != str1));
        CPPUNIT_ASSERT(str1 == iso8859_1_bad_start);
        CPPUNIT_ASSERT(!(str1 != iso8859_1_bad_start));
        CPPUNIT_ASSERT(str1.valid());

        // then copy operator
        as2js::String str2(str1);
        CPPUNIT_ASSERT(strlen(iso8859_1_bad_start) == str2.length());
        CPPUNIT_ASSERT(compare_chars(iso8859_1_bad_start, str2.c_str()));
        CPPUNIT_ASSERT(iso8859_1_bad_start == str2);
        CPPUNIT_ASSERT(!(iso8859_1_bad_start != str2));
        CPPUNIT_ASSERT(str2 == iso8859_1_bad_start);
        CPPUNIT_ASSERT(!(str2 != iso8859_1_bad_start));
        CPPUNIT_ASSERT(str2.valid());

        // copy from std::string
        std::string std(iso8859_1_bad_start);
        as2js::String str3(std);
        CPPUNIT_ASSERT(strlen(iso8859_1_bad_start) == str3.length());
        CPPUNIT_ASSERT(compare_chars(iso8859_1_bad_start, str3.c_str()));
        CPPUNIT_ASSERT(iso8859_1_bad_start == str3);
        CPPUNIT_ASSERT(!(iso8859_1_bad_start != str3));
        CPPUNIT_ASSERT(str3 == iso8859_1_bad_start);
        CPPUNIT_ASSERT(!(str3 != iso8859_1_bad_start));
        CPPUNIT_ASSERT(str3.valid());

        as2js::String str4;
        CPPUNIT_ASSERT(str4.from_char(iso8859_1_bad_start) == as2js::String::conversion_result_t::STRING_GOOD);
        CPPUNIT_ASSERT(strlen(iso8859_1_bad_start) == str4.length());
        CPPUNIT_ASSERT(compare_chars(iso8859_1_bad_start, str4.c_str()));
        CPPUNIT_ASSERT(iso8859_1_bad_start == str4);
        CPPUNIT_ASSERT(!(iso8859_1_bad_start != str4));
        CPPUNIT_ASSERT(str4 == iso8859_1_bad_start);
        CPPUNIT_ASSERT(!(str4 != iso8859_1_bad_start));
        CPPUNIT_ASSERT(str4.valid());

        as2js::String str5;
        CPPUNIT_ASSERT(&(str5 = iso8859_1_bad_start) == &str5);
        CPPUNIT_ASSERT(strlen(iso8859_1_bad_start) == str5.length());
        CPPUNIT_ASSERT(iso8859_1_bad_start == str5);
        CPPUNIT_ASSERT(!(iso8859_1_bad_start != str5));
        CPPUNIT_ASSERT(str5 == iso8859_1_bad_start);
        CPPUNIT_ASSERT(!(str5 != iso8859_1_bad_start));
        CPPUNIT_ASSERT(str5.valid());
    }

    // make sure that the UTF-8 BOM does not change a thing
    {
        // constructor
        as2js::String str1(iso8859_1_bom_and_bad_start);
        CPPUNIT_ASSERT(strlen(iso8859_1_bom_and_bad_start) == str1.length());
        CPPUNIT_ASSERT(compare_chars(iso8859_1_bom_and_bad_start, str1.c_str()));
        CPPUNIT_ASSERT(iso8859_1_bom_and_bad_start == str1);
        CPPUNIT_ASSERT(!(iso8859_1_bom_and_bad_start != str1));
        CPPUNIT_ASSERT(str1 == iso8859_1_bom_and_bad_start);
        CPPUNIT_ASSERT(!(str1 != iso8859_1_bom_and_bad_start));
        CPPUNIT_ASSERT(str1.valid());

        // then copy operator
        as2js::String str2(str1);
        CPPUNIT_ASSERT(strlen(iso8859_1_bom_and_bad_start) == str2.length());
        CPPUNIT_ASSERT(compare_chars(iso8859_1_bom_and_bad_start, str2.c_str()));
        CPPUNIT_ASSERT(iso8859_1_bom_and_bad_start == str2);
        CPPUNIT_ASSERT(!(iso8859_1_bom_and_bad_start != str2));
        CPPUNIT_ASSERT(str2 == iso8859_1_bom_and_bad_start);
        CPPUNIT_ASSERT(!(str2 != iso8859_1_bom_and_bad_start));
        CPPUNIT_ASSERT(str2.valid());

        // copy from std::string
        std::string std(iso8859_1_bom_and_bad_start);
        as2js::String str3(std);
        CPPUNIT_ASSERT(strlen(iso8859_1_bom_and_bad_start) == str3.length());
        CPPUNIT_ASSERT(compare_chars(iso8859_1_bom_and_bad_start, str3.c_str()));
        CPPUNIT_ASSERT(iso8859_1_bom_and_bad_start == str3);
        CPPUNIT_ASSERT(!(iso8859_1_bom_and_bad_start != str3));
        CPPUNIT_ASSERT(str3 == iso8859_1_bom_and_bad_start);
        CPPUNIT_ASSERT(!(str3 != iso8859_1_bom_and_bad_start));
        CPPUNIT_ASSERT(str3.valid());

        as2js::String str4;
        CPPUNIT_ASSERT(str4.from_char(iso8859_1_bom_and_bad_start) == as2js::String::conversion_result_t::STRING_GOOD);
        CPPUNIT_ASSERT(strlen(iso8859_1_bom_and_bad_start) == str4.length());
        CPPUNIT_ASSERT(compare_chars(iso8859_1_bom_and_bad_start, str4.c_str()));
        CPPUNIT_ASSERT(iso8859_1_bom_and_bad_start == str4);
        CPPUNIT_ASSERT(!(str4 != iso8859_1_bom_and_bad_start));
        CPPUNIT_ASSERT(str4.valid());

        as2js::String str5;
        CPPUNIT_ASSERT(&(str5 = iso8859_1_bom_and_bad_start) == &str5);
        CPPUNIT_ASSERT(strlen(iso8859_1_bom_and_bad_start) == str5.length());
        CPPUNIT_ASSERT(compare_chars(iso8859_1_bom_and_bad_start, str5.c_str()));
        CPPUNIT_ASSERT(iso8859_1_bom_and_bad_start == str5);
        CPPUNIT_ASSERT(!(iso8859_1_bom_and_bad_start != str5));
        CPPUNIT_ASSERT(str5 == iso8859_1_bom_and_bad_start);
        CPPUNIT_ASSERT(!(str5 != iso8859_1_bom_and_bad_start));
        CPPUNIT_ASSERT(str5.valid());
    }

    // try with all possible bytes now, the order would totally break
    // UTF-8 in many places
    {
        char buf[256];
        for(int i(0); i < 255; ++i)
        {
            buf[i] = static_cast<char>(i + 1);
        }
        buf[255] = '\0';

        // constructor
        as2js::String str1(buf);
        CPPUNIT_ASSERT(strlen(buf) == str1.length());
        CPPUNIT_ASSERT(compare_chars(buf, str1.c_str()));
        CPPUNIT_ASSERT(buf == str1);
        CPPUNIT_ASSERT(str1 == buf);
        CPPUNIT_ASSERT(!(buf != str1));
        CPPUNIT_ASSERT(!(str1 != buf));
        CPPUNIT_ASSERT(str1.valid());

        // then copy operator
        as2js::String str2(str1);
        CPPUNIT_ASSERT(strlen(buf) == str2.length());
        CPPUNIT_ASSERT(compare_chars(buf, str1.c_str()));
        CPPUNIT_ASSERT(buf == str2);
        CPPUNIT_ASSERT(str2 == buf);
        CPPUNIT_ASSERT(!(buf != str2));
        CPPUNIT_ASSERT(!(str2 != buf));
        CPPUNIT_ASSERT(str2.valid());

        // copy from std::string
        std::string std(buf);
        as2js::String str3(std);
        CPPUNIT_ASSERT(strlen(buf) == str3.length());
        CPPUNIT_ASSERT(compare_chars(buf, str3.c_str()));
        CPPUNIT_ASSERT(buf == str3);
        CPPUNIT_ASSERT(str3 == buf);
        CPPUNIT_ASSERT(!(buf != str3));
        CPPUNIT_ASSERT(!(str3 != buf));
        CPPUNIT_ASSERT(str3.valid());

        as2js::String str4;
        CPPUNIT_ASSERT(str4.from_char(buf) == as2js::String::conversion_result_t::STRING_GOOD);
        CPPUNIT_ASSERT(strlen(buf) == str4.length());
        CPPUNIT_ASSERT(compare_chars(buf, str4.c_str()));
        CPPUNIT_ASSERT(buf == str4);
        CPPUNIT_ASSERT(str4 == buf);
        CPPUNIT_ASSERT(!(buf != str4));
        CPPUNIT_ASSERT(!(str4 != buf));
        CPPUNIT_ASSERT(str4.valid());
    }

    // try with random strings
    {
        char buf[64 * 1024];

        for(int i(0); i < 50; ++i)
        {
            if(!as2js_test::g_gui && i % 5 == 4)
            {
                std::cout << "." << std::flush;
            }

            size_t const max_size(rand() % (sizeof(buf) - 5));
            for(size_t j(0); j < max_size; ++j)
            {
                // generate a number from 1 to 255
                // (we do not support '\0' in our strings
                do
                {
                    buf[j] = static_cast<char>(rand());
                }
                while(buf[j] == '\0');
            }
            buf[max_size] = '\0';
            CPPUNIT_ASSERT(strlen(buf) == max_size); // just in case

            // constructor
            as2js::String str1(buf);
            CPPUNIT_ASSERT(strlen(buf) == str1.length());
            CPPUNIT_ASSERT(compare_chars(buf, str1.c_str()));
            CPPUNIT_ASSERT(buf == str1);
            CPPUNIT_ASSERT(str1 == buf);
            CPPUNIT_ASSERT(!(buf != str1));
            CPPUNIT_ASSERT(!(str1 != buf));
            CPPUNIT_ASSERT(str1.valid());

            {
                std::stringstream ss;
                ss << str1;
                as2js::String wcs(buf); // this is verified in different places
                std::string utf8(wcstombs(wcs));
                CPPUNIT_ASSERT(ss.str() == utf8);
            }

            // then copy operator
            as2js::String str2(str1);
            CPPUNIT_ASSERT(strlen(buf) == str2.length());
            CPPUNIT_ASSERT(compare_chars(buf, str1.c_str()));
            CPPUNIT_ASSERT(buf == str2);
            CPPUNIT_ASSERT(str2 == buf);
            CPPUNIT_ASSERT(!(buf != str2));
            CPPUNIT_ASSERT(!(str2 != buf));
            CPPUNIT_ASSERT(str2.valid());

            // copy from std::string
            std::string std(buf);
            as2js::String str3(std);
            CPPUNIT_ASSERT(strlen(buf) == str3.length());
            CPPUNIT_ASSERT(compare_chars(buf, str3.c_str()));
            CPPUNIT_ASSERT(buf == str3);
            CPPUNIT_ASSERT(str3 == buf);
            CPPUNIT_ASSERT(!(buf != str3));
            CPPUNIT_ASSERT(!(str3 != buf));
            CPPUNIT_ASSERT(str3.valid());

            // also test the from_char(), should get the same result
            as2js::String str4;
            CPPUNIT_ASSERT(str4.from_char(buf) == as2js::String::conversion_result_t::STRING_GOOD);
            CPPUNIT_ASSERT(strlen(buf) == str4.length());
            CPPUNIT_ASSERT(compare_chars(buf, str4.c_str()));
            CPPUNIT_ASSERT(buf == str4);
            CPPUNIT_ASSERT(str4 == buf);
            CPPUNIT_ASSERT(!(buf != str4));
            CPPUNIT_ASSERT(!(str4 != buf));
            CPPUNIT_ASSERT(str4.valid());

            // also test the from_char(), should get the same result
            as2js::String str5;
            CPPUNIT_ASSERT(&(str5 = std) == &str5);
            CPPUNIT_ASSERT(strlen(buf) == str5.length());
            CPPUNIT_ASSERT(compare_chars(buf, str5.c_str()));
            CPPUNIT_ASSERT(buf == str5);
            CPPUNIT_ASSERT(str5 == buf);
            CPPUNIT_ASSERT(!(buf != str5));
            CPPUNIT_ASSERT(!(str5 != buf));
            CPPUNIT_ASSERT(str5.valid());

            // try truncation the input string
            // note: copy operators do not offer a truncate capability
            for(int k(0); k < 20; ++k)
            {
                size_t const size(rand() % (max_size * 2));
                size_t const end(std::min(size, strlen(buf)));

                // constructor
                as2js::String str1_1(buf, size);
                CPPUNIT_ASSERT(end == str1_1.length());
                char save1_1(buf[end]);
                buf[end] = '\0';
                CPPUNIT_ASSERT(compare_chars(buf, str1_1.c_str()));
                CPPUNIT_ASSERT(buf == str1_1);
                CPPUNIT_ASSERT(str1_1 == buf);
                CPPUNIT_ASSERT(!(buf != str1_1));
                CPPUNIT_ASSERT(!(str1_1 != buf));
                buf[end] = save1_1;
                CPPUNIT_ASSERT(str1_1.valid());

                as2js::String str1_2;
                CPPUNIT_ASSERT(str1_2.from_char(buf, size) == as2js::String::conversion_result_t::STRING_GOOD);
                char save1_2(buf[end]);
                buf[end] = '\0';
                CPPUNIT_ASSERT(strlen(buf) == str1_2.length());
                CPPUNIT_ASSERT(compare_chars(buf, str1_2.c_str()));
                CPPUNIT_ASSERT(buf == str1_2);
                CPPUNIT_ASSERT(str1_2 == buf);
                CPPUNIT_ASSERT(!(buf != str1_2));
                CPPUNIT_ASSERT(!(str1_2 != buf));
                buf[end] = save1_2;
                CPPUNIT_ASSERT(str1_2.valid());
            }

            // now try a += char
            for(int k(0); k < 5; ++k)
            {
                char random(rand());
                while(random == '\0')
                {
                    random = rand();
                }
                size_t l(strlen(buf));
                buf[l] = random; // we have at least 10 bytes extra for this purpose
                buf[l + 1] = '\0';
                CPPUNIT_ASSERT(&(str1 += random) == &str1);
                CPPUNIT_ASSERT(strlen(buf) == str1.length());
                CPPUNIT_ASSERT(compare_chars(buf, str1.c_str()));
                CPPUNIT_ASSERT(buf == str1);
                CPPUNIT_ASSERT(str1 == buf);
                CPPUNIT_ASSERT(!(buf != str1));
                CPPUNIT_ASSERT(!(str1 != buf));
                CPPUNIT_ASSERT(str1.valid());

                char buf2_2[64 * 1024 + 10];
                strcpy(buf2_2, "foo: ");
                strcat(buf2_2, buf);
                as2js::String str2_2("foo: ");
                CPPUNIT_ASSERT(&(str2_2 += buf) == &str2_2);
                CPPUNIT_ASSERT(strlen(buf) + 5 == str2_2.length());
                CPPUNIT_ASSERT(strlen(buf2_2) == str2_2.length());
                CPPUNIT_ASSERT(compare_chars(buf2_2, str2_2.c_str()));
                CPPUNIT_ASSERT(buf2_2 == str2_2);
                CPPUNIT_ASSERT(str2_2 == buf2_2);
                CPPUNIT_ASSERT(!(buf2_2 != str2_2));
                CPPUNIT_ASSERT(!(str2_2 != buf2_2));
                CPPUNIT_ASSERT(str2_2.valid());

                as2js::String str2_3("foo: ");
                std::string lstd(buf);
                CPPUNIT_ASSERT(&(str2_3 += lstd) == &str2_3);
                CPPUNIT_ASSERT(strlen(buf) + 5 == str2_3.length());
                CPPUNIT_ASSERT(strlen(buf2_2) == str2_3.length());
                CPPUNIT_ASSERT(compare_chars(buf2_2, str2_3.c_str()));
                CPPUNIT_ASSERT(buf2_2 == str2_3);
                CPPUNIT_ASSERT(str2_3 == buf2_2);
                CPPUNIT_ASSERT(!(buf2_2 != str2_3));
                CPPUNIT_ASSERT(!(str2_3 != buf2_2));
                CPPUNIT_ASSERT(str2_3.valid());
            }
        }
    }
}


void As2JsStringUnitTests::test_utf8()
{
    // all the other contructor tests verify that they do not support
    // UTF-8; there are no UTF-8 constructors actually, so here all
    // we can test is the from_utf8().

    {
        char const *null_char_ptr(nullptr);
        as2js::String str1;
        str1.from_utf8(null_char_ptr, 3);
        CPPUNIT_ASSERT(&(str1 = "") == &str1);
        CPPUNIT_ASSERT(str1.empty());
        CPPUNIT_ASSERT(str1.length() == 0);
        CPPUNIT_ASSERT(str1.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str1);
        CPPUNIT_ASSERT(str1 == "");
        CPPUNIT_ASSERT(!("" != str1));
        CPPUNIT_ASSERT(!(str1 != ""));
        CPPUNIT_ASSERT(str1.valid());
    }

    // first check a few small strings
    for(int i(0); i < 10; ++i)
    {
        // 5 to 9 character strings
        int32_t buf[10];
        size_t const max_chars(rand() % 5 + 5);
        for(size_t j(0); j < max_chars; ++j)
        {
            uint32_t wc(0);
            do
            {
                wc = rand() & 0x001FFFFF;
            }
            while(wc == 0 || wc > 0x0010FFFF || (wc >= 0xD800 && wc <= 0xDFFF));
            CPPUNIT_ASSERT(as2js::String::valid_character(wc));
            buf[j] = wc;
        }
        buf[max_chars] = '\0';
        as2js::String wcs(buf); // testing UTF-32 here!
        std::string mbs(wcstombs(wcs));

        {
            as2js::String str1;
            CPPUNIT_ASSERT(str1.from_utf8(mbs.c_str()) == as2js::String::conversion_result_t::STRING_GOOD);
            CPPUNIT_ASSERT(max_chars == str1.length());
            CPPUNIT_ASSERT(buf == str1);
            CPPUNIT_ASSERT(str1 == buf);
            CPPUNIT_ASSERT(!(buf != str1));
            CPPUNIT_ASSERT(!(str1 != buf));
            CPPUNIT_ASSERT(str1.valid());
            CPPUNIT_ASSERT(str1.utf8_length() == static_cast<ssize_t>(mbs.length()));
            CPPUNIT_ASSERT(mbs == str1.to_utf8());

            // try copies of larger characters
            as2js::String str2(str1);
            CPPUNIT_ASSERT(max_chars == str2.length());
            CPPUNIT_ASSERT(buf == str2);
            CPPUNIT_ASSERT(str2 == buf);
            CPPUNIT_ASSERT(!(buf != str2));
            CPPUNIT_ASSERT(!(str2 != buf));
            CPPUNIT_ASSERT(str2.valid());
            CPPUNIT_ASSERT(str2.utf8_length() == static_cast<ssize_t>(mbs.length()));
            CPPUNIT_ASSERT(mbs == str2.to_utf8());

            // test with a size (but that can break the UTF-8 encoding so
            // we have to be careful...)
            for(size_t k(1); k < mbs.length(); ++k)
            {
                // verify size
                char const *sub(mbs.c_str());
                size_t length(k);
                uint32_t wc;
                as2js::String out;
                int r(0);
                do
                {
                    r = mbstowc(wc, sub, length);
                    if(wc > 0)
                    {
                        out += static_cast<as2js::as_char_t>(wc);
                    }
                }
                while(r > 0);
                // all characters are good, but we may read the end early
                as2js::String::conversion_result_t const cr(
                        r == 0 ? as2js::String::conversion_result_t::STRING_GOOD
                               : as2js::String::conversion_result_t::STRING_END
                    );
                as2js::String str3;
                CPPUNIT_ASSERT(str3.from_utf8(mbs.c_str(), k) == cr);
                if(r == 0)
                {
                    CPPUNIT_ASSERT(out.length() == str3.length());
                    CPPUNIT_ASSERT(out == str3);
                    CPPUNIT_ASSERT(str3 == out);
                    CPPUNIT_ASSERT(!(out != str3));
                    CPPUNIT_ASSERT(!(str3 != out));
                    CPPUNIT_ASSERT(str3.valid());
                    CPPUNIT_ASSERT(str3.utf8_length() == static_cast<ssize_t>(k));
                    CPPUNIT_ASSERT(mbs.substr(0, k) == str3.to_utf8());
                }
                else
                {
                    // if an error occurs the destination remains unchanged
                    CPPUNIT_ASSERT(0 == str3.length());
                    CPPUNIT_ASSERT(compare_chars("", str3.c_str()));
                    CPPUNIT_ASSERT("" == str3);
                    CPPUNIT_ASSERT(str3 == "");
                    CPPUNIT_ASSERT(!("" != str3));
                    CPPUNIT_ASSERT(!(str3 != ""));
                    CPPUNIT_ASSERT(str3.valid());
                    CPPUNIT_ASSERT(str3.utf8_length() == 0);
                    CPPUNIT_ASSERT("" == str3.to_utf8());
                }
            }
        }
    }

    // then check all the characters (Except '\0')
    for(int i(1); i < 0x110000; ++i)
    {
        // skip the UTF-16 surrogate which are not considered valid
        // UTF-8
        if(i == 0x00D800)
        {
            i = 0x00DFFF;
            continue;
        }

        CPPUNIT_ASSERT(as2js::String::valid_character(i));

        if(!as2js_test::g_gui && (i & 0x00FFFF) == 0)
        {
            std::cout << "." << std::flush;
        }

        int32_t buf[2];
        buf[0] = i;
        buf[1] = '\0';
        as2js::String wcs(buf); // testing UTF-32 here!
        std::string mbs(wcstombs(wcs));

        {
            as2js::String str1;
            CPPUNIT_ASSERT(str1.from_utf8(mbs.c_str()) == as2js::String::conversion_result_t::STRING_GOOD);
            CPPUNIT_ASSERT(1 == str1.length());
            CPPUNIT_ASSERT(buf == str1);
            CPPUNIT_ASSERT(str1 == buf);
            CPPUNIT_ASSERT(!(buf != str1));
            CPPUNIT_ASSERT(!(str1 != buf));
            CPPUNIT_ASSERT(str1.valid());
            CPPUNIT_ASSERT(str1.utf8_length() == static_cast<ssize_t>(mbs.length()));
            CPPUNIT_ASSERT(mbs == str1.to_utf8());

            // try copies of larger characters
            as2js::String str2(str1);
            CPPUNIT_ASSERT(1 == str2.length());
            CPPUNIT_ASSERT(buf == str2);
            CPPUNIT_ASSERT(str2 == buf);
            CPPUNIT_ASSERT(!(buf != str2));
            CPPUNIT_ASSERT(!(str2 != buf));
            CPPUNIT_ASSERT(str2.valid());
            CPPUNIT_ASSERT(str2.utf8_length() == static_cast<ssize_t>(mbs.length()));
            CPPUNIT_ASSERT(mbs == str2.to_utf8());
        }
    }

    // test that the surrogate all crap out
    for(int i(0xD800); i < 0xE000; ++i)
    {
        CPPUNIT_ASSERT(!as2js::String::valid_character(i));

        // WARNING: cannot use the String to convert to wcs because
        //          that catches those invalid characters too!
        char buf[8];
        wctombs(buf, i);

        as2js::String str1;
        CPPUNIT_ASSERT(str1.from_utf8(buf) == as2js::String::conversion_result_t::STRING_INVALID);
        CPPUNIT_ASSERT(str1.empty()); // not modified

        as2js::String str2("old value");
        CPPUNIT_ASSERT(str2.from_utf8(buf) == as2js::String::conversion_result_t::STRING_INVALID);
        CPPUNIT_ASSERT(str2.length() == 9); // not modified
        CPPUNIT_ASSERT(str2 == "old value"); // not modified
    }

    // now to test bad encoding, generate random data and make
    // sure we detect it as incorrect then call the String
    // implementation
    // first test the sequences we expect to be wrong by themselves
    {
        char buf[16];
        for(int i(0x80); i < 0xC0; ++i)
        {
            buf[0] = i;
            buf[1] = '?';
            buf[2] = '\0';

            as2js::String str1;
            CPPUNIT_ASSERT(str1.from_utf8(buf) == as2js::String::conversion_result_t::STRING_BAD);
        }
        {
            buf[0] = static_cast<char>(0xFE);
            buf[1] = '?';
            buf[2] = '\0';

            as2js::String str1;
            CPPUNIT_ASSERT(str1.from_utf8(buf) == as2js::String::conversion_result_t::STRING_BAD);
        }
        {
            buf[0] = static_cast<char>(0xFF);
            buf[1] = '?';
            buf[2] = '\0';

            as2js::String str1;
            CPPUNIT_ASSERT(str1.from_utf8(buf) == as2js::String::conversion_result_t::STRING_BAD);
        }
        for(int i(0xC0); i < 0xFD; ++i)
        {
            // valid introducer
            buf[0] = i;
            do
            {
                // invalid continuation
                buf[1] = rand();
            }
            while(static_cast<unsigned char>(buf[1]) == '\0'
                || (static_cast<unsigned char>(buf[1]) >= 0x80 && static_cast<unsigned char>(buf[1]) <= 0xBF));
            buf[2] = '0';
            buf[3] = '1';
            buf[4] = '2';
            buf[5] = '3';
            buf[6] = '4';
            buf[7] = '\0';

            as2js::String str1;
            CPPUNIT_ASSERT(str1.from_utf8(buf) == as2js::String::conversion_result_t::STRING_BAD);
        }
    }
    // and now 10 random invalid strings
    for(int i(1); i < 10; ++i)
    {
        // verify size
        char buf[256];
        for(int j(0); j < 255; ++j)
        {
            do
            {
                buf[j] = rand();
            }
            while(buf[j] == '\0');
        }
        buf[255] = '\0';

        char const *sub(buf);
        size_t length(255);
        uint32_t wc;
        as2js::String out;
        int r(0);
        as2js::String::conversion_result_t result(as2js::String::conversion_result_t::STRING_BAD);
        do
        {
            r = mbstowc(wc, sub, length);
            if(r > 0 && !as2js::String::valid_character(wc))
            {
                result = as2js::String::conversion_result_t::STRING_INVALID;
                r = -2;
                break;
            }
        }
        while(r > 0);
        if(r != -2 && r != -3)
        {
            // a valid string?!
            continue;
        }
        // all characters are good, but we may read the end early
        as2js::String str3;
        CPPUNIT_ASSERT(str3.from_utf8(buf) == result);
        CPPUNIT_ASSERT(out.length() == str3.length());
    }

    // characters over 0x10FFFF are all invalid
    int counter(0);
    for(uint32_t i(0x110000); i < 0x80000000; i += rand() & 0x3FFF + 1, ++counter)
    {
        CPPUNIT_ASSERT(!as2js::String::valid_character(i));

        if(!as2js_test::g_gui && (counter & 0x00001FFF) == 0)
        {
            std::cout << "." << std::flush;
        }

        // WARNING: cannot use the String to convert to wcs because
        //          that catches those invalid characters too!
        char buf[8];
        wctombs(buf, i);

        as2js::String str1;
        CPPUNIT_ASSERT(str1.from_utf8(buf) == as2js::String::conversion_result_t::STRING_INVALID);
        CPPUNIT_ASSERT(str1.empty()); // not modified

        as2js::String str2("old value");
        CPPUNIT_ASSERT(str2.from_utf8(buf) == as2js::String::conversion_result_t::STRING_INVALID);
        CPPUNIT_ASSERT(str2.length() == 9); // not modified
        CPPUNIT_ASSERT(str2 == "old value"); // not modified
        CPPUNIT_ASSERT(!(str2 != "old value")); // not modified
    }

    // any value that represents a negative number (int32_t) is
    // so not valid that we cannot even encode it to test...
    //for(uint32_t i(0x80000000); i < 0xFFFFFFFF; ++i)
    //{
    //    // WARNING: cannot use the String to convert to wcs because
    //    //          that catches those invalid characters too!
    //    char buf[8];
    //    wctombs(buf, i);

    //    as2js::String str1;
    //    CPPUNIT_ASSERT(str1.from_utf8(buf) == as2js::String::conversion_result_t::STRING_INVALID);
    //    CPPUNIT_ASSERT(str1.empty()); // not modified

    //    as2js::String str2("old value");
    //    CPPUNIT_ASSERT(str2.from_utf8(buf) == as2js::String::conversion_result_t::STRING_INVALID);
    //    CPPUNIT_ASSERT(str2.length() == 9); // not modified
    //    CPPUNIT_ASSERT(str2 == "old value"); // not modified
    //}
}


void As2JsStringUnitTests::test_utf16()
{
    {
        wchar_t const *null_wchar_ptr(nullptr);
        as2js::String str1(null_wchar_ptr, 4);
        CPPUNIT_ASSERT(&(str1 = "") == &str1);
        CPPUNIT_ASSERT(str1.empty());
        CPPUNIT_ASSERT(str1.length() == 0);
        CPPUNIT_ASSERT(str1.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str1);
        CPPUNIT_ASSERT(str1 == "");
        CPPUNIT_ASSERT(!("" != str1));
        CPPUNIT_ASSERT(!(str1 != ""));
        CPPUNIT_ASSERT(str1.valid());

        as2js::String str2;
        str2.from_wchar(null_wchar_ptr, 6);
        CPPUNIT_ASSERT(&(str2 = "") == &str2);
        CPPUNIT_ASSERT(str2.empty());
        CPPUNIT_ASSERT(str2.length() == 0);
        CPPUNIT_ASSERT(str2.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str2);
        CPPUNIT_ASSERT(str2 == "");
        CPPUNIT_ASSERT(!("" != str2));
        CPPUNIT_ASSERT(!(str2 != ""));
        CPPUNIT_ASSERT(str2.valid());
    }

    // check all the characters (Except '\0' and surrogates)
    for(int i(1); i < 0x110000; ++i)
    {
        // skip the surrogate which we want to encode from other characters
        // rather than use as is...
        if(i == 0x00D800)
        {
            i = 0x00DFFF;
            continue;
        }

        CPPUNIT_ASSERT(as2js::String::valid_character(i));

        if(!as2js_test::g_gui && (i & 0x001FFF) == 0)
        {
            std::cout << "." << std::flush;
        }

        // Note: although wchar_t is 32 bits under Linux, we manage these
        //       strings as if they were 16 bits... (although we'll
        //       accept characters larger than 0x00FFFF as a UTF-32
        //       character.)
        wchar_t buf[10];
        if(i >= 0x10000)
        {
            buf[0] = ((i - 0x10000) >> 10) | 0xD800;    // lead
            buf[1] = ((i - 0x10000) & 0x3FF) | 0xDC00;  // trail
            buf[2] = '\0';
        }
        else
        {
            buf[0] = i;
            buf[1] = '\0';
        }

        {
            as2js::String str1;
            CPPUNIT_ASSERT(str1.from_wchar(buf) == as2js::String::conversion_result_t::STRING_GOOD);
            CPPUNIT_ASSERT(1 == str1.length());
            //CPPUNIT_ASSERT(buf == str1); -- we do not support those
            //CPPUNIT_ASSERT(str1 == buf);
            CPPUNIT_ASSERT(str1.valid());

            // try copies of strings created from wchar_t characters
            as2js::String str2(str1);
            CPPUNIT_ASSERT(1 == str2.length());
            CPPUNIT_ASSERT(str1 == str2);
            CPPUNIT_ASSERT(!(str1 != str2));
            CPPUNIT_ASSERT(str2.valid());

            // now test the += of a wchar_t
            // TODO under MS-Windows we cannot test this += with
            //      characters larger than 0x0FFFF
            CPPUNIT_ASSERT(&(str1 += static_cast<wchar_t>(i)) == &str1);
            CPPUNIT_ASSERT(2 == str1.length());
            CPPUNIT_ASSERT(str1.valid());

            CPPUNIT_ASSERT(&(str1 += utf16_to_append) == &str1);
            CPPUNIT_ASSERT(5 == str1.length());
            CPPUNIT_ASSERT(str1.valid());
            CPPUNIT_ASSERT(str1[2] == 0x1111);
            CPPUNIT_ASSERT(str1[3] == 0x2222);
            CPPUNIT_ASSERT(str1[4] == 0x3333);

            // try copies of strings created from wchar_t characters
            as2js::String str6;
            CPPUNIT_ASSERT(&(str6 = str2) == &str6);
            CPPUNIT_ASSERT(1 == str6.length());
            CPPUNIT_ASSERT(str2 == str6);
            CPPUNIT_ASSERT(!(str2 != str6));
            CPPUNIT_ASSERT(str6.valid());
        }

        // just in case, try without the surrogate if wchar_t is > 2
        if(sizeof(wchar_t) > 2 && i > 0xFFFF)
        {
            buf[0] = i;
            buf[1] = '\0';

            {
                as2js::String str3;
                CPPUNIT_ASSERT(str3.from_wchar(buf) == as2js::String::conversion_result_t::STRING_GOOD);
                CPPUNIT_ASSERT(1 == str3.length());
                CPPUNIT_ASSERT(str3.valid());

                // try copies of strings created from wchar_t characters
                as2js::String str4(str3);
                CPPUNIT_ASSERT(1 == str4.length());
                CPPUNIT_ASSERT(str3 == str4);
                CPPUNIT_ASSERT(!(str3 != str4));
                CPPUNIT_ASSERT(str4.valid());

                std::wstring wstr(utf16_to_append);
                CPPUNIT_ASSERT(&(str3 += wstr) == &str3);
                CPPUNIT_ASSERT(4 == str3.length());
                CPPUNIT_ASSERT(str3.valid());
                CPPUNIT_ASSERT(str3[1] == 0x1111);
                CPPUNIT_ASSERT(str3[2] == 0x2222);
                CPPUNIT_ASSERT(str3[3] == 0x3333);

                CPPUNIT_ASSERT(&(str3 = wstr) == &str3);
                CPPUNIT_ASSERT(3 == str3.length());
                CPPUNIT_ASSERT(str3.valid());
                CPPUNIT_ASSERT(str3[0] == 0x1111);
                CPPUNIT_ASSERT(str3[1] == 0x2222);
                CPPUNIT_ASSERT(str3[2] == 0x3333);
            }
        }

        // try with a string of a respectful size (really small though)
        // and the operator = (wchar_t const *) function
        {
            // repeat 5 times
            for(int j(0); j < 5; ++j)
            {
                for(int k(0); k < 8; ++k)
                {
                    if(k == 4)
                    {
                        if(i >= 0x10000)
                        {
                            buf[k] = ((i - 0x10000) >> 10) | 0xD800;    // lead
                            ++k;
                            buf[k] = ((i - 0x10000) & 0x3FF) | 0xDC00;  // trail
                        }
                        else
                        {
                            buf[k] = i;
                        }
                    }
                    else
                    {
                        // if not offset 4, get a random character
                        // in BMP 0 which are not '\0' nor a surrogate
                        do
                        {
                            buf[k] = rand() & 0x00FFFF;
                        }
                        while(buf[k] == '\0' || (buf[k] >= 0xD800 && buf[k] <= 0xDFFF));
                    }
                }
                buf[8] = '\0';

                // we verify the constructor, so we know it works...
                as2js::String str_cmp(buf);
                as2js::String str9("original");
                CPPUNIT_ASSERT(&(str9 = buf) == &str9);
                CPPUNIT_ASSERT((i >= 0x10000 ? 7 : 8) == str9.length());
                CPPUNIT_ASSERT(str9 == str9);
                CPPUNIT_ASSERT(str9 == str_cmp);
                CPPUNIT_ASSERT(str_cmp == str_cmp);
                CPPUNIT_ASSERT(!(str9 != str9));
                CPPUNIT_ASSERT(!(str9 != str_cmp));
                CPPUNIT_ASSERT(!(str_cmp != str_cmp));
                CPPUNIT_ASSERT(str9.valid());

                std::wstring wstd(buf);
                as2js::String str10("original");
                CPPUNIT_ASSERT(&(str10 = buf) == &str10);
                CPPUNIT_ASSERT((i >= 0x10000 ? 7 : 8) == str10.length());
                CPPUNIT_ASSERT(str10 == str10);
                CPPUNIT_ASSERT(str10 == str_cmp);
                CPPUNIT_ASSERT(str_cmp == str_cmp);
                CPPUNIT_ASSERT(!(str10 != str10));
                CPPUNIT_ASSERT(!(str10 != str_cmp));
                CPPUNIT_ASSERT(!(str_cmp != str_cmp));
                CPPUNIT_ASSERT(str10.valid());
            }
        }

        // test that we detect lead without trail surrogates
        if(i >= 0x10000)
        {
            // inverted, oops!
            for(int j(0); j < 5; ++j)
            {
                do
                {
                    // generate a random character in the first spot
                    buf[j] = rand() & 0x00FFFF;
                }
                while(buf[j] == '\0' || (buf[j] >= 0xD800 && buf[j] <= 0xDFFF));
            }
            buf[5] = ((i - 0x10000) >> 10) | 0xD800;    // lead
            buf[6] = '\0';

            as2js::String str7("original");
            CPPUNIT_ASSERT(str7.from_wchar(buf) == as2js::String::conversion_result_t::STRING_END);
            CPPUNIT_ASSERT(8 == str7.length());
            CPPUNIT_ASSERT("original" == str7);
            CPPUNIT_ASSERT(!("original" != str7));
            CPPUNIT_ASSERT(str7.valid());

            as2js::String str8("original");
            CPPUNIT_ASSERT(str8.from_wchar(buf, 6) == as2js::String::conversion_result_t::STRING_END);
            CPPUNIT_ASSERT(8 == str8.length());
            CPPUNIT_ASSERT("original" == str8);
            CPPUNIT_ASSERT(!("original" != str8));
            CPPUNIT_ASSERT(str8.valid());
        }

        // test that we detect inverted surrogates
        if(i >= 0x10000)
        {
            // inverted, oops!
            buf[0] = ((i - 0x10000) & 0x3FF) | 0xDC00;  // trail
            buf[1] = ((i - 0x10000) >> 10) | 0xD800;    // lead
            buf[2] = '\0';

            as2js::String str7("original");
            CPPUNIT_ASSERT(str7.from_wchar(buf) == as2js::String::conversion_result_t::STRING_BAD);
            CPPUNIT_ASSERT(8 == str7.length());
            CPPUNIT_ASSERT("original" == str7);
            CPPUNIT_ASSERT(!("original" != str7));
            CPPUNIT_ASSERT(str7.valid());

            buf[2] = rand();
            buf[3] = rand();
            buf[4] = rand();
            buf[5] = rand();
            buf[6] = rand();
            buf[7] = rand();
            buf[8] = rand();
            buf[9] = rand();
            as2js::String str11("original");
            CPPUNIT_ASSERT(str11.from_wchar(buf, rand() % 8 + 2) == as2js::String::conversion_result_t::STRING_BAD);
            CPPUNIT_ASSERT(8 == str11.length());
            CPPUNIT_ASSERT("original" == str11);
            CPPUNIT_ASSERT(!("original" != str11));
            CPPUNIT_ASSERT(str11.valid());
        }
    }
}


void As2JsStringUnitTests::test_utf32()
{
    {
        as2js::as_char_t const *null_char32_ptr(nullptr);
        as2js::String str1(null_char32_ptr, 9);
        CPPUNIT_ASSERT(&(str1 = "") == &str1);
        CPPUNIT_ASSERT(str1.empty());
        CPPUNIT_ASSERT(str1.length() == 0);
        CPPUNIT_ASSERT(str1.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str1);
        CPPUNIT_ASSERT(str1 == "");
        CPPUNIT_ASSERT(!("" != str1));
        CPPUNIT_ASSERT(!(str1 != ""));
        CPPUNIT_ASSERT(str1.valid());

        as2js::String str2;
        CPPUNIT_ASSERT(str2.from_as_char(null_char32_ptr, 6) == as2js::String::conversion_result_t::STRING_GOOD);
        CPPUNIT_ASSERT(&(str2 = "") == &str2);
        CPPUNIT_ASSERT(str2.empty());
        CPPUNIT_ASSERT(str2.length() == 0);
        CPPUNIT_ASSERT(str2.utf8_length() == 0);
        CPPUNIT_ASSERT("" == str2);
        CPPUNIT_ASSERT(str2 == "");
        CPPUNIT_ASSERT(!("" != str2));
        CPPUNIT_ASSERT(!(str2 != ""));
        CPPUNIT_ASSERT(str2.valid());
    }

    // check all the characters (Except '\0' and surrogates)
    for(int i(1); i < 0x120000; ++i)
    {
        // Note: although wchar_t is 32 bits under Linux, we manage these
        //       strings as if they were 16 bits... (although we'll
        //       accept characters larger than 0x00FFFF as a UTF-32
        //       character.)
        as2js::as_char_t buf[2];
        buf[0] = i;
        buf[1] = '\0';

        // skip the surrogate which we want to encode from other characters
        // rather than use as is...
        if((i >= 0x00D800 && i <= 0x00DFFF)
        || i >= 0x110000)
        {
            // creating a string with a surrogate will generate an exception
            try
            {
                as2js::String str(buf);
                CPPUNIT_ASSERT(!"we wanted the exception and did not get it");
            }
            catch(as2js::exception_internal_error const &)
            {
                // it worked as expected
            }
            continue;
        }

        if(!as2js_test::g_gui && (i & 0x00FFFF) == 0)
        {
            std::cout << "." << std::flush;
        }

        {
            as2js::String str1;
            CPPUNIT_ASSERT(str1.from_as_char(buf) == as2js::String::conversion_result_t::STRING_GOOD);
            CPPUNIT_ASSERT(1 == str1.length());
            CPPUNIT_ASSERT(buf == str1);
            CPPUNIT_ASSERT(str1 == buf);
            CPPUNIT_ASSERT(!(str1 != buf));
            CPPUNIT_ASSERT(str1.valid());

            // try copies of strings created from wchar_t characters
            as2js::String str2(str1);
            CPPUNIT_ASSERT(1 == str2.length());
            CPPUNIT_ASSERT(str1 == str2);
            CPPUNIT_ASSERT(!(str1 != str2));
            CPPUNIT_ASSERT(str2.valid());

            // now test the += of a wchar_t
            // TODO under MS-Windows we cannot test this += with
            //      characters larger than 0x0FFFF
            CPPUNIT_ASSERT(&(str1 += static_cast<as2js::as_char_t>(i)) == &str1);
            CPPUNIT_ASSERT(2 == str1.length());
            CPPUNIT_ASSERT(str1.valid());
            CPPUNIT_ASSERT(str1[0] == i);
            CPPUNIT_ASSERT(str1[1] == i);

            CPPUNIT_ASSERT(&(str1 += utf32_to_append) == &str1);
            CPPUNIT_ASSERT(5 == str1.length());
            CPPUNIT_ASSERT(str1.valid());
            CPPUNIT_ASSERT(str1[0] == i);
            CPPUNIT_ASSERT(str1[1] == i);
            CPPUNIT_ASSERT(str1[2] == 0x101111);
            CPPUNIT_ASSERT(str1[3] == 0x5555);
            CPPUNIT_ASSERT(str1[4] == 0x103333);
        }
    }

    // some random strings to test the length on the constructor
    for(int i(0); i < 50; ++i)
    {
        as2js::as_char_t buf[256];
        for(int j(0); j < 255; ++j)
        {
            do
            {
                buf[j] = rand() & 0x001FFFFF;
            }
            while(buf[j] == '\0' || buf[j] > 0x0010FFFF || (buf[j] >= 0xD800 && buf[j] <= 0xDFFF));
        }
        buf[255] = '\0';

        // the whole string first
        as2js::String str1(buf);
        CPPUNIT_ASSERT(255 == str1.length());
        CPPUNIT_ASSERT(buf == str1);
        CPPUNIT_ASSERT(str1 == buf);
        CPPUNIT_ASSERT(!(buf != str1));
        CPPUNIT_ASSERT(!(str1 != buf));
        CPPUNIT_ASSERT(str1.valid());

        // try again with the from_as_char()
        CPPUNIT_ASSERT(str1.from_as_char(buf) == as2js::String::conversion_result_t::STRING_GOOD);
        CPPUNIT_ASSERT(255 == str1.length());
        CPPUNIT_ASSERT(buf == str1);
        CPPUNIT_ASSERT(str1 == buf);
        CPPUNIT_ASSERT(!(buf != str1));
        CPPUNIT_ASSERT(!(str1 != buf));
        CPPUNIT_ASSERT(str1.valid());

        // now test different sizes
        for(int j(0); j < 50; ++j)
        {
            size_t size(rand() % 250 + 2);

            // the whole string first
            as2js::String str2(buf, size);
            CPPUNIT_ASSERT(size == str2.length());
            as2js::as_char_t save_a(buf[size]);
            buf[size] = '\0';
            CPPUNIT_ASSERT(buf == str2);
            CPPUNIT_ASSERT(str2 == buf);
            CPPUNIT_ASSERT(!(buf != str2));
            CPPUNIT_ASSERT(!(str2 != buf));
            CPPUNIT_ASSERT(str2.valid());
            buf[size] = save_a;

            // try again with the from_as_char()
            CPPUNIT_ASSERT(str2.from_as_char(buf, size) == as2js::String::conversion_result_t::STRING_GOOD);
            CPPUNIT_ASSERT(size == str2.length());
            as2js::as_char_t save_b(buf[size]);
            buf[size] = '\0';
            CPPUNIT_ASSERT(buf == str2);
            CPPUNIT_ASSERT(str2 == buf);
            CPPUNIT_ASSERT(!(buf != str2));
            CPPUNIT_ASSERT(!(str2 != buf));
            CPPUNIT_ASSERT(str2.valid());
            buf[size] = save_b;

            // this should not have changed
            CPPUNIT_ASSERT(255 == str1.length());

            // take a minute to test str1 += str2
            {
                // make a copy otherwise str1 += str2 becomes cumulative
                as2js::String str3(str1);

                as2js::as_char_t buf2[512];
                memcpy(buf2, buf, sizeof(buf[0]) * 255);
                memcpy(buf2 + 255, buf, sizeof(buf[0]) * size);  // then what was copied in str2
                buf2[255 + size] = '\0';
                str3 += str2;
                CPPUNIT_ASSERT(size + 255 == str3.length());
                CPPUNIT_ASSERT(buf2 == str3);
                CPPUNIT_ASSERT(str3 == buf2);
                CPPUNIT_ASSERT(!(buf2 != str3));
                CPPUNIT_ASSERT(!(str3 != buf2));
                CPPUNIT_ASSERT(str3.valid());

                // and make sure that str2 was indeed untouched
                CPPUNIT_ASSERT(size == str2.length());
                as2js::as_char_t save_c(buf[size]);
                buf[size] = '\0';
                CPPUNIT_ASSERT(buf == str2);
                CPPUNIT_ASSERT(str2 == buf);
                CPPUNIT_ASSERT(!(buf != str2));
                CPPUNIT_ASSERT(!(str2 != buf));
                CPPUNIT_ASSERT(str2.valid());
                buf[size] = save_c;
            }

            // try again with the from_as_char()
            int const bad_pos(size / 2);
            as2js::as_char_t save_d(buf[bad_pos]);
            do
            {
                buf[bad_pos] = rand();
            }
            while((buf[bad_pos] > 0 && buf[bad_pos] < 0xD800)
               || (buf[bad_pos] > 0xDFFF && buf[bad_pos] < 0x110000));
            as2js::String str4;
            CPPUNIT_ASSERT(str4.from_as_char(buf, size) == as2js::String::conversion_result_t::STRING_INVALID);
            CPPUNIT_ASSERT(0 == str4.length());
            CPPUNIT_ASSERT(str4.empty());
            CPPUNIT_ASSERT("" == str4);
            CPPUNIT_ASSERT(str4 == "");
            CPPUNIT_ASSERT(!("" != str4));
            CPPUNIT_ASSERT(!(str4 != ""));
            CPPUNIT_ASSERT(str4.valid());
            buf[bad_pos] = save_d;

            // test a copy of str1 with one invalid character
            as2js::String str5(str1);
            do
            {
                // testing that indeed the [] operator does not check the
                // validity of UTF-32 characters...
                // std::basic_string<as_char_t> operator []()
                str5[bad_pos] = rand();
            }
            while((str5[bad_pos] > 0 && str5[bad_pos] < 0xD800)
               || (str5[bad_pos] > 0xDFFF && str5[bad_pos] < 0x110000));
            CPPUNIT_ASSERT(!str5.valid());
            // if invalid the UTF-8 length is always -1
            CPPUNIT_ASSERT(str5.utf8_length() == -1);
        }

        //
    }

    // test that the surrogate all crap out
    for(int i(0xD800); i < 0xE000; ++i)
    {
        as2js::as_char_t buf[2];
        buf[0] = i;
        buf[1] = '\0';

        as2js::String str1;
        CPPUNIT_ASSERT(str1.from_as_char(buf) == as2js::String::conversion_result_t::STRING_INVALID);
        CPPUNIT_ASSERT(str1.empty()); // not modified

        as2js::String str2("old value");
        CPPUNIT_ASSERT(str2.from_as_char(buf) == as2js::String::conversion_result_t::STRING_INVALID);
        CPPUNIT_ASSERT(str2.length() == 9); // not modified
        CPPUNIT_ASSERT(str2 == "old value"); // not modified
        CPPUNIT_ASSERT(!(str2 != "old value")); // not modified
        CPPUNIT_ASSERT(str2 != "new value");

        as2js::String str3;
        CPPUNIT_ASSERT(str3.from_as_char(buf, 1) == as2js::String::conversion_result_t::STRING_INVALID);
        CPPUNIT_ASSERT(str3.empty()); // not modified

        as2js::String str4("old value");
        CPPUNIT_ASSERT(str4.from_as_char(buf, 1) == as2js::String::conversion_result_t::STRING_INVALID);
        CPPUNIT_ASSERT(str4.length() == 9); // not modified
        CPPUNIT_ASSERT(str4 == "old value"); // not modified
        CPPUNIT_ASSERT(!(str4 != "old value")); // not modified
        CPPUNIT_ASSERT(str4 != "new value");
    }

    // characters over 0x10FFFF are all invalid
    //
    // NOTE: In this case the loop index (i) will wrap around and we
    //       catch that using that wierd test you see below
    //
    int counter(0);
    for(uint32_t i(0x110000); i >= 0x00110000; i += rand() & 0x3FFF + 1, ++counter)
    {
        // test this one because it may not have been tested yet
        CPPUNIT_ASSERT(!as2js::String::valid_character(i));

        if(!as2js_test::g_gui && (counter & 0x00001FFF) == 0)
        {
            std::cout << "." << std::flush;
        }

        // WARNING: cannot use the String to convert to wcs because
        //          that catches those invalid characters too!
        as2js::as_char_t buf[8];
        buf[0] = i;
        buf[1] = '\0';

        as2js::String str1;
        CPPUNIT_ASSERT(str1.from_as_char(buf) == as2js::String::conversion_result_t::STRING_INVALID);
        CPPUNIT_ASSERT(str1.empty()); // not modified

        as2js::String str2("old value");
        CPPUNIT_ASSERT(str2.from_as_char(buf) == as2js::String::conversion_result_t::STRING_INVALID);
        CPPUNIT_ASSERT(str2.length() == 9); // not modified
        CPPUNIT_ASSERT(str2 == "old value"); // not modified
        CPPUNIT_ASSERT(!(str2 != "old value")); // not modified
        CPPUNIT_ASSERT(str2 != "new value");
    }
}


void As2JsStringUnitTests::test_number()
{
    {
        // empty is a special case that represents 0 or 0.0
        as2js::String str1;

        CPPUNIT_ASSERT(str1.is_int64());
        CPPUNIT_ASSERT(str1.is_float64());
        CPPUNIT_ASSERT(str1.is_number());
        CPPUNIT_ASSERT(str1.to_int64() == 0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        CPPUNIT_ASSERT(str1.to_float64() == 0.0);
#pragma GCC diagnostic pop
        CPPUNIT_ASSERT(!str1.is_true());

        // "0x" or "0X" are not valid hexadecimal numbers
        as2js::String str2;
        CPPUNIT_ASSERT(&(str2 = "0x") == &str2);
        CPPUNIT_ASSERT(!str2.is_int64());
        CPPUNIT_ASSERT(!str2.is_float64());
        CPPUNIT_ASSERT(!str2.is_number());
        CPPUNIT_ASSERT_THROW(str2.to_int64(), as2js::exception_internal_error);
        CPPUNIT_ASSERT(std::isnan(str2.to_float64()));
        CPPUNIT_ASSERT(str2.is_true());

        as2js::String str3;
        CPPUNIT_ASSERT(&(str3 = "0X") == &str3);
        CPPUNIT_ASSERT(!str3.is_int64());
        CPPUNIT_ASSERT(!str3.is_float64());
        CPPUNIT_ASSERT(!str3.is_number());
        CPPUNIT_ASSERT_THROW(str3.to_int64(), as2js::exception_internal_error);
        CPPUNIT_ASSERT(std::isnan(str3.to_float64()));
        CPPUNIT_ASSERT(str3.is_true());
    }

    for(int64_t i(-100000); i <= 100000; ++i)
    {
        // decimal
        {
            std::stringstream str;
            str << (i >= 0 && (rand() & 1) ? "+" : "") << i;
            as2js::String str1(str.str());
            CPPUNIT_ASSERT(str1.is_int64());
            CPPUNIT_ASSERT(str1.is_float64());
            CPPUNIT_ASSERT(str1.is_number());
            CPPUNIT_ASSERT(str1.to_int64() == i);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            CPPUNIT_ASSERT(str1.to_float64() == static_cast<double>(i));
#pragma GCC diagnostic pop
            CPPUNIT_ASSERT(str1.is_true());
        }
        // hexadecimal
        {
            std::stringstream str;
            str << (i < 0 ? "-" : (rand() & 1 ? "+" : "")) << "0" << (rand() & 1 ? "x" : "X") << std::hex << labs(i);
            as2js::String str1(str.str());
            CPPUNIT_ASSERT(str1.is_int64());
            CPPUNIT_ASSERT(!str1.is_float64());
            CPPUNIT_ASSERT(str1.is_number());
            CPPUNIT_ASSERT(str1.to_int64() == i);
            CPPUNIT_ASSERT(std::isnan(str1.to_float64()));
            CPPUNIT_ASSERT(str1.is_true());
        }
    }

    for(double i(-1000.00); i <= 1000.00; i += (rand() % 120) / 100.0)
    {
        std::stringstream str;
        str << i;
        if(str.str().find('e') != std::string::npos
        || str.str().find('E') != std::string::npos)
        {
            // this happens with numbers very close to zero and the
            // system decides to write them as '1e-12' for example
            continue;
        }
        std::string value1(str.str());
        as2js::String str1(value1);
        int64_t integer1(lrint(i));
        bool is_integer1(std::find(value1.begin(), value1.end(), '.') == value1.end());
        CPPUNIT_ASSERT(str1.is_int64() ^ !is_integer1);
        CPPUNIT_ASSERT(str1.is_float64());
        CPPUNIT_ASSERT(str1.is_number());
        if(is_integer1)
        {
            CPPUNIT_ASSERT(str1.to_int64() == integer1);
        }
        else
        {
            CPPUNIT_ASSERT_THROW(str1.to_int64(), as2js::exception_internal_error);
        }
        CPPUNIT_ASSERT(close_double(str1.to_float64(), i, 0.01));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        CPPUNIT_ASSERT(str1.is_true());
#pragma GCC diagnostic pop

        // add x 1000 as an exponent
        str << "e" << (rand() & 1 ? "+" : "") << "3";
        std::string value2(str.str());
        as2js::String str2(value2);
        // the 'e' "breaks" the integer test in JavaScript
        CPPUNIT_ASSERT(!str2.is_int64());
        CPPUNIT_ASSERT(str2.is_float64());
        CPPUNIT_ASSERT(str2.is_number());
        CPPUNIT_ASSERT_THROW(str2.to_int64(), as2js::exception_internal_error);
        CPPUNIT_ASSERT(close_double(str2.to_float64(), i * 1000.0, 0.01));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        CPPUNIT_ASSERT(str2.is_true());
#pragma GCC diagnostic pop

        // add x 1000 as an exponent
        str.str(""); // reset the string
        str << value1 << "e-3";
        std::string value3(str.str());
        as2js::String str3(value3);
        // the 'e' "breaks" the integer test in JavaScript
        CPPUNIT_ASSERT(!str3.is_int64());
        CPPUNIT_ASSERT(str3.is_float64());
        CPPUNIT_ASSERT(str3.is_number());
        CPPUNIT_ASSERT_THROW(str3.to_int64(), as2js::exception_internal_error);
        CPPUNIT_ASSERT(close_double(str3.to_float64(), i / 1000.0, 0.00001));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        CPPUNIT_ASSERT(str3.is_true());
#pragma GCC diagnostic pop
    }

    // a few more using random
    for(int i(0); i < 100000; ++i)
    {
        // rand generally returns 31 bit values
        int64_t value((rand() | (static_cast<uint64_t>(rand()) << 32)) ^ (static_cast<uint64_t>(rand()) << 16));
        std::stringstream str;
        str << value;
        as2js::String str1(str.str());
        CPPUNIT_ASSERT(str1.is_int64());
        CPPUNIT_ASSERT(str1.is_float64());
        CPPUNIT_ASSERT(str1.is_number());
        CPPUNIT_ASSERT(str1.to_int64() == value);
        as2js::Float64 flt1(str1.to_float64());
        as2js::Float64 flt2(static_cast<double>(value));
        CPPUNIT_ASSERT(flt1.nearly_equal(flt2, 0.0001));
        CPPUNIT_ASSERT(str1.is_true());
    }

    // test a few non-hexadecimal numbers
    for(int i(0); i < 100; ++i)
    {
        // get a character which is not a valid hex digit and not '\0'
        char c;
        do
        {
            c = static_cast<char>(rand());
        }
        while(c == '\0'
          || (c >= '0' && c <= '9')
          || (c >= 'a' && c <= 'f')
          || (c >= 'A' && c <= 'F'));

        // bad character is right at the beginning of the hex number
        std::stringstream ss1;
        ss1 << "0" << (rand() & 1 ? "x" : "X") << c << "123ABC";
        as2js::String str1(ss1.str());
        CPPUNIT_ASSERT(!str1.is_int64());
        CPPUNIT_ASSERT(!str1.is_float64());
        CPPUNIT_ASSERT(!str1.is_number());
        CPPUNIT_ASSERT_THROW(str1.to_int64(), as2js::exception_internal_error);
        CPPUNIT_ASSERT(std::isnan(str1.to_float64()));
        CPPUNIT_ASSERT(str1.is_true());

        // invalid character is in the middle of the hex number
        std::stringstream ss2;
        ss2 << "0" << (rand() & 1 ? "x" : "X") << "123" << c << "ABC";
        as2js::String str2(ss2.str());
        CPPUNIT_ASSERT(!str2.is_int64());
        CPPUNIT_ASSERT(!str2.is_float64());
        CPPUNIT_ASSERT(!str2.is_number());
        CPPUNIT_ASSERT_THROW(str2.to_int64(), as2js::exception_internal_error);
        CPPUNIT_ASSERT(std::isnan(str2.to_float64()));
        CPPUNIT_ASSERT(str2.is_true());
    }
}


void As2JsStringUnitTests::test_concatenation()
{
    // this test allows us to hit the basic_string<as_char_t> constructor
    // and copy operator

    as2js::String str1("blah");
    as2js::String str2("foo");

    as2js::String str3(str1 + str2); // here!
    CPPUNIT_ASSERT(str3.length() == 7);
    CPPUNIT_ASSERT(str3 == "blahfoo");
    CPPUNIT_ASSERT(!(str3 != "blahfoo"));
    CPPUNIT_ASSERT(str3 != "blah");
    CPPUNIT_ASSERT(str3 != "foo");
    CPPUNIT_ASSERT(str3 == str1 + str2);
    CPPUNIT_ASSERT(!(str3 != str1 + str2));

    as2js::String str4;
    str4 = str2 + str1;
    CPPUNIT_ASSERT(str4.length() == 7);
    CPPUNIT_ASSERT(str4 == "fooblah");
    CPPUNIT_ASSERT(!(str4 != "fooblah"));
    CPPUNIT_ASSERT(str4 != "foo");
    CPPUNIT_ASSERT(str4 != "blah");
    CPPUNIT_ASSERT(str4 == str2 + str1);
}


void As2JsStringUnitTests::test_simplified()
{
    // remove spaces at the start
    {
        as2js::String str("    blah");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "blah");
    }

    // remove spaces at the end
    {
        as2js::String str("blah    ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "blah");
    }

    // remove spaces at the start and end
    {
        as2js::String str("    blah    ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "blah");
    }

    // simplify spaces inside
    {
        as2js::String str("blah    foo");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "blah foo");
    }

    // simplify all spaces inside
    {
        as2js::String str("    blah    foo    ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "blah foo");
    }

    // simplify spaces inside, including newlines
    {
        as2js::String str("blah  \n  foo");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "blah foo");
    }

    // empty strings become zero
    {
        as2js::String str("");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "0");
    }
    {
        as2js::String str("     ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "0");
    }

    // simplify to the number: just spaces around
    {
        as2js::String str("  3.14159  ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "3.14159");
        CPPUNIT_ASSERT(simplified.is_float64());
        CPPUNIT_ASSERT(simplified.is_number());
        CPPUNIT_ASSERT(as2js::Float64(simplified.to_float64()).nearly_equal(3.14159, 1e-8));
    }

    // simplify to the number: spaces and left over
    {
        as2js::String str("  3.14159 ignore that part  ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "3.14159");
        CPPUNIT_ASSERT(simplified.is_float64());
        CPPUNIT_ASSERT(simplified.is_number());
        CPPUNIT_ASSERT(as2js::Float64(simplified.to_float64()).nearly_equal(3.14159, 1e-8));
    }

    // simplify to the number: sign, spaces and left over
    {
        as2js::String str("  +3.14159 ignore that part  ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "+3.14159");
        CPPUNIT_ASSERT(simplified.is_float64());
        CPPUNIT_ASSERT(simplified.is_number());
        CPPUNIT_ASSERT(as2js::Float64(simplified.to_float64()).nearly_equal(3.14159, 1e-8));
    }
    {
        as2js::String str("  -314159 ignore that part  ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "-314159");
        CPPUNIT_ASSERT(simplified.is_int64());
        CPPUNIT_ASSERT(simplified.to_int64() == -314159);
        CPPUNIT_ASSERT(simplified.is_float64());
        CPPUNIT_ASSERT(simplified.is_number());
        CPPUNIT_ASSERT(as2js::Float64(simplified.to_float64()).nearly_equal(-314159, 1e-8));
    }

    // simplify to the number: sign, exponent, spaces and left over
    {
        as2js::String str("  +0.00314159e3 ignore that part  ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "+0.00314159e3");
        CPPUNIT_ASSERT(simplified.is_float64());
        CPPUNIT_ASSERT(simplified.is_number());
        CPPUNIT_ASSERT(as2js::Float64(simplified.to_float64()).nearly_equal(3.14159, 1e-8));
    }
    {
        as2js::String str("  +0.00314159e+3 ignore that part  ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "+0.00314159e+3");
        CPPUNIT_ASSERT(simplified.is_float64());
        CPPUNIT_ASSERT(simplified.is_number());
        CPPUNIT_ASSERT(as2js::Float64(simplified.to_float64()).nearly_equal(3.14159, 1e-8));
    }
    {
        as2js::String str("  -314159e-5 ignore that part  ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "-314159");
        CPPUNIT_ASSERT(simplified.is_int64());
        CPPUNIT_ASSERT(simplified.to_int64() == -314159);
        CPPUNIT_ASSERT(simplified.is_float64());
        CPPUNIT_ASSERT(simplified.is_number());
        CPPUNIT_ASSERT(as2js::Float64(simplified.to_float64()).nearly_equal(-314159, 1e-8));
    }
    {
        as2js::String str("  -314159.e-5 ignore that part  ");
        as2js::String simplified(str.simplified());
        CPPUNIT_ASSERT(simplified == "-314159.e-5");
        CPPUNIT_ASSERT(simplified.is_float64());
        CPPUNIT_ASSERT(simplified.is_number());
        CPPUNIT_ASSERT(as2js::Float64(simplified.to_float64()).nearly_equal(-3.14159, 1e-8));
    }
}



// vim: ts=4 sw=4 et
