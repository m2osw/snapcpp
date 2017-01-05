/* test_as2js_stream.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_stream.h"
#include    "test_as2js_main.h"

#include    "as2js/stream.h"
#include    "as2js/exceptions.h"

#include    <cstring>
#include    <algorithm>
#include    <sstream>

#include    <unistd.h>
#include    <fcntl.h>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsStreamUnitTests );


namespace
{

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

}
// no name namespace




void As2JsStreamUnitTests::test_filter_iso88591()
{
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterISO88591);
        for(int c(1); c < 256; ++c)
        {
            filter->putc(c);
            CPPUNIT_ASSERT(filter->getc() == c);
        }
        // check EOF and make sure it remains that way
        for(int c(0); c < 256; ++c)
        {
            CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        }
    }
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterISO88591);
        for(int c(1); c < 256; ++c)
        {
            filter->putc(c);
        }
        for(int c(1); c < 256; ++c)
        {
            CPPUNIT_ASSERT(filter->getc() == c);
        }
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);

        // then try with random data
        int buf[256];
        for(int c(0); c < 256; ++c)
        {
            CPPUNIT_ASSERT(static_cast<size_t>(c) < sizeof(buf) / sizeof(buf[0]));
            do
            {
                buf[c] = rand() & 0xFF;
            }
            while(buf[c] == 0);
            filter->putc(buf[c]);
        }
        for(int c(0); c < 256; ++c)
        {
            CPPUNIT_ASSERT(filter->getc() == buf[c]);
        }
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }
}


void As2JsStreamUnitTests::test_filter_utf8()
{
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterUTF8);

        // The Stream reimplements its own UTF-8 conversion so we want to
        // test all the characters here...
        for(as2js::as_char_t wc(1); wc < 0x1FFFFF; ++wc)
        {
            if((wc & 0xFFFF) == 0)
            {
                std::cout << "." << std::flush;
            }

            bool err(wc >= 0xD800 && wc <= 0xDFFF || wc > 0x10FFFF);

            // 1 to 7 character strings
            char buf[10];
            wctombs(buf, wc);

            for(size_t idx(0); buf[idx] != '\0'; ++idx)
            {
                filter->putc(buf[idx]);
                if(buf[idx + 1] == '\0')
                {
                    if(err)
                    {
                        // invalid characters must return an error
                        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
                    }
                    else //if(wc != as2js::String::STRING_BOM)
                    {
                        as2js::as_char_t get_wc(filter->getc());
//std::cerr << "got " << get_wc << ", expected " << wc << "\n";
                        CPPUNIT_ASSERT(get_wc == wc);
                    }
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
                }
                else
                {
                    // NAC remains any number of times until we add
                    // enough bytes to the input
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                }
            }
        }
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // now check sending many characters with putc() and reading them back later
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterUTF8);

        as2js::String result;
        for(int c(0); c < 256; ++c)
        {
            // 5 to 9 character strings
            int32_t wc;
            do
            {
                // generate a random Unicode character
                wc = ((rand() << 16) ^ rand()) & 0x1FFFFF;
            }
            while((wc >= 0xD800 && wc <= 0xDFFF) || wc >= 0x110000);
            char buf[10];
            wctombs(buf, wc);
            for(int idx(0); buf[idx] != '\0'; ++idx)
            {
                filter->putc(buf[idx]);
            }
            //if(wc != as2js::String::STRING_BOM)
            {
                result += wc;
            }
        }

        for(size_t idx(0); idx < result.length(); ++idx)
        {
            as2js::as_char_t get_wc(filter->getc());
//std::cerr << get_wc << " vs. " << result[idx];
            CPPUNIT_ASSERT(get_wc == result[idx]);
        }
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // bytes F8 to FF generate errors immedately
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterUTF8);

        for(int idx(0xF8); idx < 0x100; ++idx)
        {
            filter->putc(idx);
            CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
            CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        }
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // invalid continue bytes test
//std::cerr << "\n";
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterUTF8);

        for(int idx(0xC0); idx < 0xF8; ++idx)
        {
//std::cerr << std::hex << "testing " << idx;
            filter->putc(idx);
            as2js::as_char_t bad, extra1, extra2, extra3(0);
            do
            {
                bad = rand() & 0xFF;
            }
            while(bad >= 0x80 && bad <= 0xBF); // skip valid bytes
//std::cerr << std::hex << " + " << bad;
            filter->putc(bad);
            if(idx >= 0xE0)
            {
                do
                {
                    extra1 = rand() & 0x7F;
                }
                while(extra1 == 0);
                filter->putc(extra1);
//std::cerr << std::hex << " + " << extra1;
            }
            else
            {
                extra1 = 0;
            }
            if(idx >= 0xF0)
            {
                do
                {
                    extra2 = rand() & 0x7F;
                }
                while(extra2 == 0);
                filter->putc(extra2);
//std::cerr << std::hex << " + " << extra2;
            }
            else
            {
                extra2 = 0;
            }
//std::cerr << "\n";
            CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
            // the bad byte is still there, check it...
            if(bad < 0x80)
            {
                // load a normal ISO-8859-1 character
                CPPUNIT_ASSERT(filter->getc() == bad);
                if(extra1 != 0)
                {
                    CPPUNIT_ASSERT(filter->getc() == extra1);
                }
                if(extra2 != 0)
                {
                    CPPUNIT_ASSERT(filter->getc() == extra2);
                }
            }
            else if(bad >= 0xC0 && bad < 0xE0)
            {
                // do something to reset the state
                if(extra1 == 0)
                {
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);

                    extra1 = rand() & 0x7F;
                    filter->putc(extra1);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
                    CPPUNIT_ASSERT(filter->getc() == extra1);
                }
                else
                {
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
                    CPPUNIT_ASSERT(filter->getc() == extra1);
                    if(extra2 != 0)
                    {
                        CPPUNIT_ASSERT(filter->getc() == extra2);
                    }
                }
            }
            else if(bad >= 0xE0 && bad < 0xF0)
            {
                if(extra1 == 0)
                {
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                    extra1 = rand() & 0x7F;
                    filter->putc(extra1);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                    extra2 = rand() & 0x7F;
                    filter->putc(extra2);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
                    CPPUNIT_ASSERT(filter->getc() == extra1);
                    CPPUNIT_ASSERT(filter->getc() == extra2);
                }
                else if(extra2 == 0)
                {
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                    extra2 = rand() & 0x7F;
                    filter->putc(extra2);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
                    CPPUNIT_ASSERT(filter->getc() == extra1);
                    CPPUNIT_ASSERT(filter->getc() == extra2);
                }
                else
                {
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
                    CPPUNIT_ASSERT(filter->getc() == extra1);
                    CPPUNIT_ASSERT(filter->getc() == extra2);
                }
            }
            else if(bad >= 0xF0 && bad < 0xF8)
            {
                if(extra1 == 0)
                {
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                    extra1 = rand() & 0x7F;
                    filter->putc(extra1);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                    extra2 = rand() & 0x7F;
                    filter->putc(extra2);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                    extra3 = rand() & 0x7F;
                    filter->putc(extra3);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
                    CPPUNIT_ASSERT(filter->getc() == extra1);
                    CPPUNIT_ASSERT(filter->getc() == extra2);
                    CPPUNIT_ASSERT(filter->getc() == extra3);
                }
                else if(extra2 == 0)
                {
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                    extra2 = rand() & 0x7F;
                    filter->putc(extra2);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                    extra3 = rand() & 0x7F;
                    filter->putc(extra3);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
                    CPPUNIT_ASSERT(filter->getc() == extra1);
                    CPPUNIT_ASSERT(filter->getc() == extra2);
                    CPPUNIT_ASSERT(filter->getc() == extra3);
                }
                else
                {
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                    extra3 = rand() & 0x7F;
                    filter->putc(extra3);
                    CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
                    CPPUNIT_ASSERT(filter->getc() == extra1);
                    CPPUNIT_ASSERT(filter->getc() == extra2);
                    CPPUNIT_ASSERT(filter->getc() == extra3);
                }
            }
            else
            {
                CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_ERR);
                if(extra1 != 0)
                {
                    CPPUNIT_ASSERT(filter->getc() == extra1);
                }
                if(extra2 != 0)
                {
                    CPPUNIT_ASSERT(filter->getc() == extra2);
                }
            }
            // make sure the buffer is empty
            CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        }
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }
}


void As2JsStreamUnitTests::test_filter_utf16()
{
    {
        as2js::DecodingFilter *filter_be(new as2js::DecodingFilterUTF16BE);
        as2js::DecodingFilter *filter_le(new as2js::DecodingFilterUTF16LE);

        // The Stream reimplements its own UTF-16 conversion so we want to
        // test all the characters here... Also we have a BE and an LE
        // version so we check both at the same time; just valid characters
        for(as2js::as_char_t wc(1); wc < 0x110000; ++wc)
        {
            if((wc & 0xFFFF) == 0)
            {
                std::cout << "." << std::flush;
            }

            if(wc >= 0xD800 && wc <= 0xDFFF)
            {
                continue;
            }

            // putc() accepts bytes only, so we need to break down all those
            // character into bytes as expected by the respective filter
            if(wc > 0xFFFF)
            {
                // in this case we need to send 2x uint16_t values
                uint16_t lead((((wc - 0x10000) >> 10) & 0x03FF) | 0xD800);
                uint16_t trail(((wc - 0x10000) & 0x03FF) | 0xDC00);

                filter_be->putc(lead >> 8);
                filter_be->putc(lead & 255);
                filter_be->putc(trail >> 8);
                filter_be->putc(trail & 255);
                CPPUNIT_ASSERT(filter_be->getc() == wc);

                // little endian swaps bytes, not the lead & trail
                filter_le->putc(lead & 255);
                filter_le->putc(lead >> 8);
                filter_le->putc(trail & 255);
                filter_le->putc(trail >> 8);
                CPPUNIT_ASSERT(filter_le->getc() == wc);
            }
            //else if(wc == as2js::String::STRING_BOM)
            //{
            //    // the BOM is never returned
            //    filter_be->putc(wc >> 8);
            //    filter_be->putc(wc & 255);
            //    CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);

            //    filter_le->putc(wc & 255);
            //    filter_le->putc(wc >> 8);
            //    CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
            //}
            else
            {
                filter_be->putc(wc >> 8);
                filter_be->putc(wc & 255);
                as2js::as_char_t get_wc(filter_be->getc());
//std::cerr << "wc " << wc << " got " << get_wc << "\n";
                CPPUNIT_ASSERT(get_wc == wc);

                filter_le->putc(wc & 255);
                filter_le->putc(wc >> 8);
                CPPUNIT_ASSERT(filter_le->getc() == wc);
            }
        }
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
    }

    // do it again, this time try all the NAC
    {
        as2js::DecodingFilter *filter_be(new as2js::DecodingFilterUTF16BE);
        as2js::DecodingFilter *filter_le(new as2js::DecodingFilterUTF16LE);

        // The Stream reimplements its own UTF-16 conversion so we want to
        // test all the characters here... Also we have a BE and an LE
        // version so we check both at the same time; just valid characters
        for(as2js::as_char_t wc(1); wc < 0x110000; ++wc)
        {
            if((wc & 0xFFFF) == 0)
            {
                std::cout << "." << std::flush;
            }

            if(wc >= 0xD800 && wc <= 0xDFFF)
            {
                continue;
            }

            // putc() accepts bytes only, so we need to break down all those
            // character into bytes as expected by the respective filter
            if(wc > 0xFFFF)
            {
                // in this case we need to send 2x uint16_t values
                uint16_t lead((((wc - 0x10000) >> 10) & 0x03FF) | 0xD800);
                uint16_t trail(((wc - 0x10000) & 0x03FF) | 0xDC00);

                filter_be->putc(lead >> 8);
                CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
                filter_be->putc(lead & 255);
                CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
                filter_be->putc(trail >> 8);
                CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
                filter_be->putc(trail & 255);
                CPPUNIT_ASSERT(filter_be->getc() == wc);

                // little endian swaps bytes, not the lead & trail
                filter_le->putc(lead & 255);
                CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
                filter_le->putc(lead >> 8);
                CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
                filter_le->putc(trail & 255);
                CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
                filter_le->putc(trail >> 8);
                CPPUNIT_ASSERT(filter_le->getc() == wc);
            }
            //else if(wc == as2js::String::STRING_BOM)
            //{
            //    // the BOM is never returned
            //    filter_be->putc(wc >> 8);
            //    CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
            //    filter_be->putc(wc & 255);
            //    CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);

            //    filter_le->putc(wc & 255);
            //    CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
            //    filter_le->putc(wc >> 8);
            //    CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
            //}
            else
            {
                filter_be->putc(wc >> 8);
                CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
                filter_be->putc(wc & 255);
                as2js::as_char_t get_wc(filter_be->getc());
//std::cerr << "wc " << wc << " got " << get_wc << "\n";
                CPPUNIT_ASSERT(get_wc == wc);

                filter_le->putc(wc & 255);
                CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
                filter_le->putc(wc >> 8);
                CPPUNIT_ASSERT(filter_le->getc() == wc);
            }
        }
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
    }

    // invalid surrogates
    // (1) trail surrogate without a lead
    std::cout << "." << std::flush;
    {
        as2js::DecodingFilter *filter_be(new as2js::DecodingFilterUTF16BE);
        as2js::DecodingFilter *filter_le(new as2js::DecodingFilterUTF16LE);

        // The Stream reimplements its own UTF-16 conversion so we want to
        // test all the characters here... Also we have a BE and an LE
        // version so we check both at the same time; just valid characters
        for(as2js::as_char_t wc(0xDC00); wc < 0xE000; ++wc)
        {
            filter_be->putc(wc >> 8);
            CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
            filter_be->putc(wc & 255);
            CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_ERR);

            filter_le->putc(wc & 255);
            CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
            filter_le->putc(wc >> 8);
            CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_ERR);
        }
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
    }

    // invalid surrogates
    // (2) lead surrogate without a trail
    std::cout << "." << std::flush;
    {
        as2js::DecodingFilter *filter_be(new as2js::DecodingFilterUTF16BE);
        as2js::DecodingFilter *filter_le(new as2js::DecodingFilterUTF16LE);

        // The Stream reimplements its own UTF-16 conversion so we want to
        // test all the characters here... Also we have a BE and an LE
        // version so we check both at the same time; just valid characters
        for(as2js::as_char_t wc(0xD800); wc < 0xDC00; ++wc)
        {
            // get another character which is not a surrogate
            as2js::as_char_t extra1;
            do
            {
                extra1 = rand() & 0x0FFFF;
            }
            while(extra1 >= 0xD800 && extra1 <= 0xDFFF);

            filter_be->putc(wc >> 8);
            CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
            filter_be->putc(wc & 255);
            CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
            filter_be->putc(extra1 >> 8);
            CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
            filter_be->putc(extra1 & 255);
            CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_ERR);
            as2js::as_char_t get_wc(filter_be->getc());
//std::cerr << "got " << get_wc << ", expected " << extra1 << "\n";
            //if(extra1 == as2js::String::STRING_BOM)
            //{
            //    CPPUNIT_ASSERT(get_wc == as2js::Input::INPUT_EOF);
            //}
            //else
            {
                CPPUNIT_ASSERT(get_wc == extra1);
            }

            filter_le->putc(wc & 255);
            CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
            filter_le->putc(wc >> 8);
            CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
            filter_le->putc(extra1 & 255);
            CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
            filter_le->putc(extra1 >> 8);
            CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_ERR);
            //if(extra1 == as2js::String::STRING_BOM)
            //{
            //    CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
            //}
            //else
            {
                CPPUNIT_ASSERT(filter_le->getc() == extra1);
            }
        }
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
    }

}


void As2JsStreamUnitTests::test_filter_utf32()
{
    {
        as2js::DecodingFilter *filter_be(new as2js::DecodingFilterUTF32BE);
        as2js::DecodingFilter *filter_le(new as2js::DecodingFilterUTF32LE);

        // The Stream reimplements its own UTF-16 conversion so we want to
        // test all the characters here... Also we have a BE and an LE
        // version so we check both at the same time; just valid characters
        for(as2js::as_char_t wc(1); wc < 0x1FFFFF; ++wc)
        {
            if((wc & 0xFFFF) == 0)
            {
                std::cout << "." << std::flush;
            }

            bool const err((wc >= 0xD800 && wc <= 0xDFFF) || wc > 0x10FFFF);

            // putc() accepts bytes only, so we need to break down all those
            // character into bytes as expected by the respective filter

            // in this case we need to send 2x uint16_t values

            filter_be->putc((wc >> 24) & 255);
            CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
            filter_be->putc((wc >> 16) & 255);
            CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
            filter_be->putc((wc >>  8) & 255);
            CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_NAC);
            filter_be->putc((wc >>  0) & 255);
            //if(wc == as2js::String::STRING_BOM)
            //{
            //    CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
            //}
            //else
            {
                CPPUNIT_ASSERT(filter_be->getc() == (err ? as2js::Input::INPUT_ERR : wc));
            }

            // little endian swaps bytes, not the lead & trail
            filter_le->putc((wc >>  0) & 255);
            CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
            filter_le->putc((wc >>  8) & 255);
            CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
            filter_le->putc((wc >> 16) & 255);
            CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_NAC);
            filter_le->putc((wc >> 24) & 255);
            //if(wc == as2js::String::STRING_BOM)
            //{
            //    CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
            //}
            //else
            {
                CPPUNIT_ASSERT(filter_le->getc() == (err ? as2js::Input::INPUT_ERR : wc));
            }
        }
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
    }

    {
        as2js::DecodingFilter *filter_be(new as2js::DecodingFilterUTF32BE);
        as2js::DecodingFilter *filter_le(new as2js::DecodingFilterUTF32LE);

        // The Stream reimplements its own UTF-16 conversion so we want to
        // test all the characters here... Also we have a BE and an LE
        // version so we check both at the same time; just valid characters
        std::cout << "-" << std::flush;
        std::vector<as2js::as_char_t> result;
        for(as2js::as_char_t idx(0); idx < 256; ++idx)
        {
            as2js::as_char_t wc(((rand() << 16) ^ rand()) & 0x1FFFFF);

            //if(wc != as2js::String::STRING_BOM)
            {
                result.push_back(wc);
            }

            // putc() accepts bytes only, so we need to break down all those
            // character into bytes as expected by the respective filter

            // in this case we need to send 2x uint16_t values

            filter_be->putc((wc >> 24) & 255);
            filter_be->putc((wc >> 16) & 255);
            filter_be->putc((wc >>  8) & 255);
            filter_be->putc((wc >>  0) & 255);

            filter_le->putc((wc >>  0) & 255);
            filter_le->putc((wc >>  8) & 255);
            filter_le->putc((wc >> 16) & 255);
            filter_le->putc((wc >> 24) & 255);
        }
        std::cout << "+" << std::flush;
        for(size_t idx(0); idx < result.size(); ++idx)
        {
            as2js::as_char_t wc(result[idx]);

            bool const err((wc >= 0xD800 && wc <= 0xDFFF) || wc > 0x10FFFF);

            CPPUNIT_ASSERT(filter_be->getc() == (err ? as2js::Input::INPUT_ERR : wc));
            CPPUNIT_ASSERT(filter_le->getc() == (err ? as2js::Input::INPUT_ERR : wc));
        }
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_be->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
        CPPUNIT_ASSERT(filter_le->getc() == as2js::Input::INPUT_EOF);
    }
}


void As2JsStreamUnitTests::test_filter_detect()
{
    // test UTF32BE
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // BOM + 0x10203
        filter->putc(0);
        filter->putc(0);
        filter->putc(0xFE);
        filter->putc(0xFF);
        filter->putc(0);
        filter->putc(1);
        filter->putc(2);
        filter->putc(3);

        as2js::as_char_t wc(filter->getc());
        CPPUNIT_ASSERT(wc == 0x10203);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test UTF32LE
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // BOM + 0x10203
        filter->putc(0xFF);
        filter->putc(0xFE);
        filter->putc(0);
        filter->putc(0);
        filter->putc(3);
        filter->putc(2);
        filter->putc(1);
        filter->putc(0);

        as2js::as_char_t wc(filter->getc());
        CPPUNIT_ASSERT(wc == 0x10203);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test UTF16BE
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // BOM + 0x102
        filter->putc(0xFE);
        filter->putc(0xFF);
        filter->putc(1);
        filter->putc(2);

        as2js::as_char_t wc(filter->getc());
        CPPUNIT_ASSERT(wc == 0x102);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test UTF16LE
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // BOM + 0x102
        filter->putc(0xFF);
        filter->putc(0xFE);
        filter->putc(2);
        filter->putc(1);

        as2js::as_char_t wc(filter->getc());
        CPPUNIT_ASSERT(wc == 0x102);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test UTF8 with BOM
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // BOM + 0x10203
        as2js::String wstr;
        wstr += 0x0000FEFF;     // BOM
        wstr += 0x00010203;     // 0x10203
        std::string utf8(wstr.to_utf8());
        for(size_t idx(0); idx < utf8.size(); ++idx)
        {
            filter->putc(utf8[idx]);
        }

        as2js::as_char_t wc(filter->getc());
        CPPUNIT_ASSERT(wc == 0x10203);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test UTF8 without BOM
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // 0x10203 and 0x30201
        as2js::String wstr;
        wstr += 0x00010203;
        wstr += 0x00030201;
        std::string utf8(wstr.to_utf8());
        for(size_t idx(0); idx < utf8.size(); ++idx)
        {
            filter->putc(utf8[idx]);
        }

        as2js::as_char_t wc(filter->getc());
        CPPUNIT_ASSERT(wc == 0x10203);
        wc = filter->getc();
        CPPUNIT_ASSERT(wc == 0x30201);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test ISO-8859-1 (fallback)
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // invalid UTF-8 on the first 4 bytes
        filter->putc(0xFF);
        filter->putc(0x01);
        filter->putc(0x02);
        filter->putc(0x03);

        CPPUNIT_ASSERT(filter->getc() == 0xFF);
        CPPUNIT_ASSERT(filter->getc() == 0x01);
        CPPUNIT_ASSERT(filter->getc() == 0x02);
        CPPUNIT_ASSERT(filter->getc() == 0x03);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test UTF32BE with NAC tests
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // BOM + 0x10203
        filter->putc(0);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0xFE);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0xFF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        filter->putc(0);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(1);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(2);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(3);
        CPPUNIT_ASSERT(filter->getc() == 0x10203);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test UTF32LE
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // BOM + 0x10203
        filter->putc(0xFF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0xFE);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
        filter->putc(3);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(2);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(1);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0);
        CPPUNIT_ASSERT(filter->getc() == 0x10203);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test UTF16BE
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // BOM + 0x102
        filter->putc(0xFE);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0xFF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(1);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(2);
        CPPUNIT_ASSERT(filter->getc() == 0x102);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test UTF16LE
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // BOM + 0x102
        filter->putc(0xFF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0xFE);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(2);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(1);
        CPPUNIT_ASSERT(filter->getc() == 0x102);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test UTF8 with BOM
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // BOM + 0x10203
        as2js::String wstr;
        wstr += 0x0000FEFF;     // BOM
        wstr += 0x00010203;     // 0x10203
        std::string utf8(wstr.to_utf8());
        for(size_t idx(0); idx < utf8.size(); ++idx)
        {
            filter->putc(utf8[idx]);
            switch(idx)
            {
            case 0:
            case 1:
            case 2:
            case 3: // at 3 bytes the BOM is available but not detected yet...
            case 4: // at 4 bytes we got the BOM + 1 byte of the next character so we get a NAC again...
            case 5:
                CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                break;

            case 6:
                CPPUNIT_ASSERT(filter->getc() == 0x10203);
                break;

            default:
                CPPUNIT_ASSERT(false);
                break;

            }
        }
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_EOF);
    }

    // test UTF8 without BOM
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // 0x10203 and 0x30201
        as2js::String wstr;
        wstr += 0x00010203;
        wstr += 0x00030201;
        std::string utf8(wstr.to_utf8());
        for(size_t idx(0); idx < utf8.size(); ++idx)
        {
            filter->putc(utf8[idx]);
            switch(idx)
            {
            case 0:
            case 1:
            case 2:
            case 4:
            case 5:
            case 6:
                CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
                break;

            case 3:
                CPPUNIT_ASSERT(filter->getc() == 0x10203);
                break;

            case 7:
                CPPUNIT_ASSERT(filter->getc() == 0x30201);
                break;

            default:
                CPPUNIT_ASSERT(false);
                break;

            }
        }
    }

    // test ISO-8859-1 (fallback)
    {
        as2js::DecodingFilter::pointer_t filter(new as2js::DecodingFilterDetect);

        // invalid UTF-8 on the first 4 bytes
        filter->putc(0xFF);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0x01);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0x02);
        CPPUNIT_ASSERT(filter->getc() == as2js::Input::INPUT_NAC);
        filter->putc(0x03);

        CPPUNIT_ASSERT(filter->getc() == 0xFF);
        CPPUNIT_ASSERT(filter->getc() == 0x01);
        CPPUNIT_ASSERT(filter->getc() == 0x02);
        CPPUNIT_ASSERT(filter->getc() == 0x03);
    }
}


void As2JsStreamUnitTests::test_string_input()
{
    {
        as2js::String input_data("This is\nthe\ninput data\n");
        as2js::Input::pointer_t str_input(new as2js::StringInput(input_data));

        CPPUNIT_ASSERT(str_input->get_position().get_line() == 1);
        for(size_t idx(0); idx < input_data.length(); ++idx)
        {
            as2js::Input::char_t c(str_input->getc());
            CPPUNIT_ASSERT(c == input_data[idx]);
            // the input does not know anything about the position
            // so it does not change a bit
            CPPUNIT_ASSERT(str_input->get_position().get_line() == 1);
        }
    }

    {
        as2js::String input_data("Here we have another string\n");
        as2js::Input::pointer_t str_input(new as2js::StringInput(input_data));

        CPPUNIT_ASSERT(str_input->get_position().get_line() == 1);
        for(size_t idx(0); idx < input_data.length(); ++idx)
        {
            as2js::Input::char_t c(str_input->getc());
            CPPUNIT_ASSERT(c == input_data[idx]);

            // unget and re-get
            str_input->ungetc(c);
            as2js::Input::char_t bad;
            do
            {
                bad = (rand() << 16) ^ rand();
            }
            while(bad > 0 && bad < 0x110000);
            str_input->ungetc(bad); // this will be ignored
            str_input->ungetc(0); // and this too (0 is rather unlikely otherwise
            CPPUNIT_ASSERT(str_input->getc() == input_data[idx]);

            // the input does not know anything about the position
            // so it does not change a bit
            CPPUNIT_ASSERT(static_cast<as2js::Input const *>(str_input.get())->get_position().get_line() == 1);
        }
    }

    {
        as2js::String input_data("This is\nthe\ninput data\n");
        as2js::Input::pointer_t str_input(new as2js::StringInput(input_data));

        CPPUNIT_ASSERT(str_input->get_position().get_line() == 1);
        int line(1);
        for(size_t idx(0); idx < input_data.length(); ++idx)
        {
            as2js::Input::char_t c(str_input->getc());
            CPPUNIT_ASSERT(c == input_data[idx]);
            if(c == '\n')
            {
                ++line;
                str_input->get_position().new_line();
            }
            // we handle the '\n' so the line no. increases in this one
            CPPUNIT_ASSERT(static_cast<as2js::Input const *>(str_input.get())->get_position().get_line() == line);
        }
    }
}


void As2JsStreamUnitTests::test_stdin()
{
    {
        // 1. create a file with some test in it
        char filename[256];
        strncpy(filename, "/tmp/testXXXXXX.js", sizeof(filename));
        int fd(mkstemps(filename, 3));
        char const *input_data("This is\nthe\ninput data\nfor std::cin\n");
        size_t const len(strlen(input_data));
        if(write(fd, input_data, len) != static_cast<ssize_t>(len))
        {
            CPPUNIT_ASSERT(!"write failed");
            return;
        }
        close(fd);

        // 2. put that in std::cin
        if(freopen(filename, "r", stdin) == nullptr)
        {
            std::cerr << "error: failed to set stdin to a file" << std::endl;
            CPPUNIT_ASSERT(!"freopen(..., stdin) failed");
            return;
        }

        as2js::Input::pointer_t str_input(new as2js::StandardInput);

        // The filename for the StandardInput is set to "-" by default
        CPPUNIT_ASSERT(str_input->get_position().get_filename() == "-");

        CPPUNIT_ASSERT(str_input->get_position().get_line() == 1);
        as2js::String input_data_str(input_data);
        for(size_t idx(0); idx < input_data_str.length(); ++idx)
        {
            as2js::Input::char_t c(str_input->getc());
            CPPUNIT_ASSERT(c == input_data_str[idx]);
            // the input does not know anything about the position
            // so it does not change a bit
            CPPUNIT_ASSERT(str_input->get_position().get_line() == 1);
        }

        unlink(filename);
    }
}


void As2JsStreamUnitTests::test_file()
{
    {
        // 1. create a file with some test in it
        char filename[256];
        strncpy(filename, "/tmp/testXXXXXX.js", sizeof(filename));
        int fd(mkstemps(filename, 3));
        char const *input_data("This is\nthe\ninput data\nfor the file\n");
        size_t const len(strlen(input_data));
        if(write(fd, input_data, len) != static_cast<ssize_t>(len))
        {
            CPPUNIT_ASSERT(!"write failed");
            return;
        }
        close(fd);

        // 2. put that in a file
        as2js::FileInput::pointer_t str_input(new as2js::FileInput);

        // test a filename that does not exist
        CPPUNIT_ASSERT(!str_input->open("I'm pretty sure that this will not work although a funky programmer may end up creating such a file..."));
        // filename not modified if open fails
        CPPUNIT_ASSERT(str_input->get_position().get_filename() == "");

        // expect this open to work
        CPPUNIT_ASSERT(str_input->open(filename));

        // The filename for the StandardInput is set to "-" by default
        CPPUNIT_ASSERT(str_input->get_position().get_filename() == filename);

        CPPUNIT_ASSERT(str_input->get_position().get_line() == 1);
        as2js::String input_data_str(input_data);
        for(size_t idx(0); idx < input_data_str.length(); ++idx)
        {
            as2js::Input::char_t c(str_input->getc());
            CPPUNIT_ASSERT(c == input_data_str[idx]);
            // the input does not know anything about the position
            // so it does not change a bit
            CPPUNIT_ASSERT(str_input->get_position().get_line() == 1);
        }

        // if already open, we get a throw
        CPPUNIT_ASSERT_THROW(str_input->open("This is yet another filename..."), as2js::exception_file_already_open);
        CPPUNIT_ASSERT(str_input->get_position().get_filename() == filename);

        unlink(filename);
    }
}


void As2JsStreamUnitTests::test_bad_impl()
{
    {
        class BadImpl : public as2js::Input
        {
        public:
            // no overloading of either virtual function is a problem!
        };

        as2js::Input::pointer_t str_input(new BadImpl);
        CPPUNIT_ASSERT_THROW(str_input->getc(), as2js::exception_internal_error);
    }
}


void As2JsStreamUnitTests::test_stdout()
{
    {
        // 1. create an empty file
        char filename[256];
        strncpy(filename, "/tmp/testXXXXXX.js", sizeof(filename));
        int fd(mkostemps(filename, 3, O_WRONLY));

        // 2. put that in std::cout
        if(freopen(filename, "a", stdout) == nullptr)
        {
            CPPUNIT_ASSERT(!"freopen() of stdout failed");
            return;
        }

        // 3. generate some data in the file
        as2js::String str("This is some test to send to stdout\n");
        bool assert0(false);
        bool assert1(false);
        bool assert2(false);
        {
            as2js::Output::pointer_t output(new as2js::StandardOutput);

            // at the start the position is expected to be 1

            // The filename for the StandardOutput is set to "-" by default
            assert0 = output->get_position().get_filename() == "-";

            assert1 = output->get_position().get_line() == 1;

            output->write(str);

            // the write() has no effect over the position
            assert2 = static_cast<as2js::Output const *>(output.get())->get_position().get_line() == 1;
        }
        // now StandardOutput is closed, verify the contents of the file

        // 4. reassign the output
        //
        //    This freopen() works under Linux, on other systems, you may
        //    have to fiddle with the code; see:
        //    http://stackoverflow.com/questions/1908687/how-to-redirect-the-output-back-to-the-screen-after-freopenout-txt-a-stdo
        if(freopen("/dev/tty", "a", stdout) == nullptr)
        {
            CPPUNIT_ASSERT(!"freopen() to restore stdout failed");
            return;
        }

        CPPUNIT_ASSERT(assert0 && assert1 && assert2); // these are here because stdout is now restored

        lseek(fd, 0, SEEK_SET);

        char buf[256];
        ssize_t l(read(fd, buf, sizeof(buf)));
        CPPUNIT_ASSERT(l == str.utf8_length());
        CPPUNIT_ASSERT(static_cast<size_t>(l) < sizeof(buf)); // too large, we cannot continue
        buf[l] = '\0';
        CPPUNIT_ASSERT(str == buf);

        close(fd);
        unlink(filename);
    }
}


void As2JsStreamUnitTests::test_stdout_destructive()
{
    if(as2js_test::g_run_stdout_destructive)
    {
        // 1. create an empty file
        char filename[256];
        strncpy(filename, "/tmp/testXXXXXX.js", sizeof(filename));
        int fd(mkostemps(filename, 3, O_WRONLY));
        close(fd);

        // 2. put that in std::cout
        if(freopen(filename, "a", stdout) == nullptr)
        {
            CPPUNIT_ASSERT(!"freopen() of stdout failed");
            return;
        }
        setbuf(stdout, nullptr); // with a buffer the write would not fail!

        // 3. generate some data in the file
        as2js::String str("This is some test to send to stdout\n");
        {
            as2js::Output::pointer_t output(new as2js::StandardOutput);

            // close stdout before writing to it so we get an error
            close(fileno(stdout));

            CPPUNIT_ASSERT_THROW(output->write(str), as2js::exception_exit);
        }
        // now StandardOutput is closed, verify the contents of the file

        // 4. reassign the output
        //
        //    This freopen() fails with error 22 (EINVAL).
        //    have to fiddle with the code; see:
        //    http://stackoverflow.com/questions/1908687/how-to-redirect-the-output-back-to-the-screen-after-freopenout-txt-a-stdo
        if(freopen("/dev/tty", "a+", stdout) == nullptr)
        {
            CPPUNIT_ASSERT(!"freopen() of stdout failed");
            return;
        }

        unlink(filename);
    }
    else
    {
        std::cout << " --- test_stdout_destructive() not run, use --destructive on the command line to not bypass this test --- ";
    }
}


void As2JsStreamUnitTests::test_output()
{
    {
        // 1. create an empty file
        char const *filename("/tmp/test123456.js");

        // 2. generate some data in the file
        as2js::String str("This is\nsome test\nto send\nto \"filename\".\n");
        {
            as2js::FileOutput::pointer_t output(new as2js::FileOutput);

            CPPUNIT_ASSERT(!output->open("/first/we/want/to/test/with/an/invalid/filename!"));

            CPPUNIT_ASSERT(output->open(filename));

            CPPUNIT_ASSERT_THROW(output->open("another one"), as2js::exception_file_already_open);

            // at the start the position is expected to be 1
            CPPUNIT_ASSERT(output->get_position().get_line() == 1);

            // The filename for the FileOutput is set to the file filename as passed to open()
            CPPUNIT_ASSERT(output->get_position().get_filename() == filename);

            CPPUNIT_ASSERT(output->get_position().get_line() == 1);

            output->write(str);

            // the write() has no effect over the position
            CPPUNIT_ASSERT(static_cast<as2js::Output const *>(output.get())->get_position().get_line() == 1);
        }
        // now FileOutput is closed, verify the contents of the file

        int fd(open(filename, O_RDONLY));

        char buf[256];
        ssize_t l(read(fd, buf, sizeof(buf)));
        CPPUNIT_ASSERT(l == str.utf8_length());
        CPPUNIT_ASSERT(static_cast<size_t>(l) < sizeof(buf)); // too large, we cannot continue
        buf[l] = '\0';
        CPPUNIT_ASSERT(str == buf);

        close(fd);
        unlink(filename);
    }

    {
        // 1. create an empty file
        char const *filename("/tmp/test789012.js");

        // 2. determine the current fd
        //    (see use of fd_to_close further below)
        char const *find_fd("/tmp/test345678.js");
        int fd_to_close(open(find_fd, O_RDWR|O_CREAT, 0600));
        CPPUNIT_ASSERT(fd_to_close >= 0);
        close(fd_to_close);
        unlink(find_fd);

        // 3. generate some data in the file
        as2js::String str("This is\nsome test\nto send\nto \"filename\".\n");
        while(str.length() < 64 * 1024) // we should get the size from a file created with fopen()... ?
        {
            str += "This string is too short to make sure we get a flush and a write error...";
        }
        {
            as2js::FileOutput::pointer_t output(new as2js::FileOutput);

            CPPUNIT_ASSERT(!output->open("/first/we/want/to/test/with/an/invalid/filename!"));

            CPPUNIT_ASSERT(output->open(filename));

            CPPUNIT_ASSERT_THROW(output->open("another one"), as2js::exception_file_already_open);

            // at the start the position is expected to be 1
            CPPUNIT_ASSERT(output->get_position().get_line() == 1);

            // The filename for the StandardOutput is set to "-" by default
            CPPUNIT_ASSERT(output->get_position().get_filename() == filename);

            CPPUNIT_ASSERT(output->get_position().get_line() == 1);

            // close so we can generate an error...
            close(fd_to_close);

            CPPUNIT_ASSERT_THROW(output->write(str), as2js::exception_exit);

            // the write() has no effect over the position
            CPPUNIT_ASSERT(static_cast<as2js::Output const *>(output.get())->get_position().get_line() == 1);
        }
        // now FileOutput is closed, verify the contents of the file

        int fd(open(filename, O_RDONLY));

        char buf[256];
        ssize_t l(read(fd, buf, sizeof(buf)));
        CPPUNIT_ASSERT(l == 0); // must be empty since it was closed before the write

        close(fd);
        unlink(filename);
    }
}


void As2JsStreamUnitTests::test_string_output()
{
    {
        as2js::String str("This is\nsome test\nto send\nto \"filename\".\n");

        as2js::StringOutput::pointer_t output(new as2js::StringOutput);

        // at the start the position is expected to be 1
        CPPUNIT_ASSERT(output->get_position().get_line() == 1);

        // The filename for the StringOutput is always ""
        CPPUNIT_ASSERT(output->get_position().get_filename() == "");

        CPPUNIT_ASSERT(output->get_position().get_line() == 1);

        output->write(str);

        // the write() has no effect over the position
        CPPUNIT_ASSERT(static_cast<as2js::Output const *>(output.get())->get_position().get_line() == 1);

        CPPUNIT_ASSERT(output->get_string() == str);

        output->write(str);
        CPPUNIT_ASSERT(output->get_string() == str + str);
    }
}


// vim: ts=4 sw=4 et
