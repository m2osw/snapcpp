/* TLD library -- encrypted domain name case folding
 * Copyright (C) 2011-2017  Made to Order Software Corp.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/** \file
 * \brief Force lowercase for all characters in the domain name.
 *
 * This file includes the functions used to convert a domain name
 * from whatever case it comes in as to lowercase only. The input
 * domain name is expected to still be URL encoded and be valid
 * UTF-8.
 */

#include "libtld/tld.h"
#include "tld_data.h"
#if defined(MO_DARWIN)
#   include <malloc/malloc.h>
#endif
#if !defined(MO_DARWIN) && !defined(MO_FREEBSD)
#include <malloc.h>
#endif
#include <stdlib.h>
//#include <limits.h>
#include <string.h>
//#include <ctype.h>
#include <wctype.h>


/** \brief Transform an hexadecimal digit to a number.
 * \internal
 *
 * This function transforms the specified character \p c to a number from
 * 0 to 15.
 *
 * The function supports upper and lower case.
 *
 * \param[in] c  An hexadecimal character to transform to a number.
 *
 * \return The number corresponding to the hexadecimal character or -1 if
 *         the character is not 0-9, A-F, nor a-f.
 */
static int tld_hex2dec(char c)
{
    if(c >= '0' && c <= '9')
    {
        return c - '0';
    }

    if(c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }

    if(c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }

    return -1;
}


/** \brief Transform an hexadecimal digit to a number.
 * \internal
 *
 * This function transforms the specified character \p c to a number from
 * 0 to 15.
 *
 * The function supports upper and lower case.
 *
 * \param[in] d  An hexadecimal character to transform to a number.
 *
 * \return The number corresponding to the hexadecimal character or -1 if
 *         the character is not 0-9, A-F, nor a-f.
 */
static int tld_dec2hex(int d)
{
    if(d < 10)
    {
        return d + '0';
    }
    /* the spec says we should use an uppercase character */
    return d - 10 + 'A';
}


/** \brief Read one byte of data.
 * \internal
 *
 * The tld_byte_in() function reads one byte. The byte may either be
 * a %XX or a plain byte. The input may be UTF-8 characters.
 *
 * The input pointer (\p s) get incremented automatically as required.
 *
 * \param[in] s  The pointer to a string pointer where the byte the read is.
 *
 * \return The byte or -1 if an error occurs.
 */
static int tld_byte_in(const char **s)
{
    int c, h, l;

    c = (unsigned char) **s;
    if(c == '\0')
    {
        /* EOF reached; avoid the ++ on the string pointer */
        return '\0';
    }

    ++*s;

    if(c == '%')
    {
        h = tld_hex2dec(**s);
        if(h == -1)
        {
            return -1;
        }
        ++*s;

        l = tld_hex2dec(**s);
        if(l == -1)
        {
            return -1;
        }
        ++*s;

        return h * 16 + l;
    }

    return c;
}


/** \brief The tld_byte_out() outputs a character.
 * \internal
 *
 * This function ensures that the byte being output is properly
 * defined according to URI encoding rules. This means all
 * the characters get converted to %XX except the \em few that
 * can be encoded as is (i.e. some of the ASCII characters.)
 *
 * \param[in,out] s  The output string where the character is saved.
 * \param[in,out] max_length  The length of s, adjusted each time s
 *                            is incremented.
 * \param[in] byte  The byte to output in s.
 *
 * \return 0 if no error occurs, -1 on buffer overflow.
 */
static int tld_byte_out(char **s, int *max_length, char byte)
{
    int convert;

    switch(byte)
    {
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '.':
    case '-':
    case '/':
    case '_':
    case '~':
    case '!':
        convert = 0;
        break;

    default:
        convert = 1;
        break;

    }

    if(convert)
    {
        if(*max_length < 3)
        {
            return -1;
        }
        *max_length -= 3;

        **s = '%';
        ++*s;
        **s = tld_dec2hex(((unsigned char) byte) >> 4);
        ++*s;
        **s = tld_dec2hex(byte & 15);
        ++*s;
    }
    else
    {
        if(*max_length < 1)
        {
            return -1;
        }
        *max_length -= 1;

        **s = byte;
        ++*s;
    }

    return 0;
}


/** \brief Transform a multi-byte UTF-8 character to a wide character.
 * \internal
 *
 * This function transforms a UTF-8 encoded character, which may use 1
 * to 4 bytes, to a wide character (31 bit).
 *
 * \bug
 * This function transforms letters to lowercase on the fly (one by
 * one) which may not always be correct in Unicode (some languages
 * make use of multiple characters to properly calculate various
 * things such as uppercase and lowercase characters.)
 *
 * \param[in] s  A pointer to string with possible UTF-8 bytes.
 *
 * \return The corresponding UTF-32 character in lowercase, NUL
 *         character ('\0' when the end of the string is reached,
 *         or -1 if the input is invalid.
 */
static wint_t tld_mbtowc(const char **s)
{
    wint_t wc;
    int cnt;
    int c;

    c = tld_byte_in(s);
    if(c < 0x80)
    {
        /* ASCII is the same in UTF-8
         * (this also returns -1 if the byte could not be read properly)
         */
        if(c >= 'A' && c <= 'Z')
        {
            /* return upper ASCII characters as lowercase characters
             * (no need for complex tolower() in this case)
             */
            return c | 0x20;
        }
        /* return '\0' once end of string is reached */
        return c;
    }

    if(c >= 0xF0)
    {
        if(c >= 0xF8)
        {
            return -1;
        }
        wc = c & 0x07;
        cnt = 3;
    }
    else if(c >= 0xE0)
    {
        wc = c & 0x0F;
        cnt = 2;
    }
    else if(c >= 0xC0)
    {
        wc = c & 0x1F;
        cnt = 1;
    }
    else
    {
        return -1;
    }

    for(; cnt > 0; --cnt)
    {
        /* retrieve next byte */
        c = tld_byte_in(s);
        if(c == '\0')
        {
            return -1;
        }
        if(c < 0x80 || c > 0xBF)
        {
            return -1;
        }
        wc = (wc << 6) | (c & 0x3F);
    }

    return towlower(wc);
}


/** \brief Convert a wide character to UTF-8.
 * \internal
 *
 * This function quickly transforms a wide character to UTF-8.
 * The output buffer is pointed by s and has max_length byte
 * left for output.
 *
 * The function returns -1 if the character cannot be converted.
 * There are the main reasons for failure:
 *
 * \li the input wide character is not valid (out of bounds)
 * \li the input wide character represents a UTF-16 encoding value
 * \li the output buffer is full
 * \li the character ends with 0xFFFE or 0xFFFF
 *
 * The function automatically adjusts the output buffer and
 * max_length parameters.
 *
 * \param[in] wc  The wide character to convert
 * \param[in,out] s  The pointer to the output string pointer.
 * \param[in,out] max_length  The size of the output string buffer.
 *
 * \return Zero on success, -1 on error.
 */
static int tld_wctomb(wint_t wc, char **s, int *max_length)
{
    // cast because wint_t is expected to be unsigned
    if((int) wc < 0)
    {
        return -1; // LCOV_EXCL_LINE
    }

    if(wc < 0x80)
    {
        return tld_byte_out(s, max_length, (char) wc);
    }
    if(wc < 0x800)
    {
        if(tld_byte_out(s, max_length, (char) ((wc >> 6) | 0xC0)) != 0)
        {
            return -1;
        }
        return tld_byte_out(s, max_length, (char) ((wc & 0x3F) | 0x80));
    }
    if(wc < 0x10000)
    {
        if((wc >= 0xD800 && wc <= 0xDFFF)
        || wc == 0xFFFE
        || wc == 0xFFFF)
        {
            return -1;
        }

        if(tld_byte_out(s, max_length, (char) ((wc >> 12) | 0xE0)) != 0)
        {
            return -1;
        }
        if(tld_byte_out(s, max_length, (char) (((wc >> 6) & 0x3F) | 0x80)) != 0)
        {
            return -1;
        }
        return tld_byte_out(s, max_length, (char) ((wc & 0x3F) | 0x80));
    }
    if(wc < 0x110000)
    {
        if((wc & 0xFFFF) == 0xFFFE
        || (wc & 0xFFFF) == 0xFFFF)
        {
            return -1;
        }

        if(tld_byte_out(s, max_length, (char) ((wc >> 18) | 0xF0)) != 0)
        {
            return -1;
        }
        if(tld_byte_out(s, max_length, (char) (((wc >> 12) & 0x3F) | 0x80)) != 0)
        {
            return -1;
        }
        if(tld_byte_out(s, max_length, (char) (((wc >> 6) & 0x3F) | 0x80)) != 0)
        {
            return -1;
        }
        return tld_byte_out(s, max_length, (char) ((wc & 0x3F) | 0x80));
    }

    // internally, this should never happen.
    return -1;
}


/** \brief Transform a domain with a TLD to lowercase before processing.
 *
 * This function will transform the input domain name to lowercase.
 * You should call this function before you call the tld() function
 * to make sure that the input data is in lowercase.
 *
 * This function interprets the %XX input data and transforms that
 * to characters. The function further converts UTF-8 characters to
 * wide characters to be able to determine the lowercase version.
 *
 * \warning
 * The function allocates a new buffer to save the result in it.
 * You are responsible for freeing that buffer. So the following
 * code is wrong:
 *
 * \code
 *      struct tld_info info;
 *      tld(tld_domain_to_lowercase(domain), &info);
 *      // WRONG: tld_domain_to_lowercase() leaked a heap buffer
 * \endcode
 *
 * In C++ you may use an std::unique_ptr<> with free as the deleter
 * to not have to both with the call by hand (especially if you
 * have possible exceptions in your code):
 *
 * \code
        std::unique_ptr<char, void(*)(char *)> lowercase_domain(tld_domain_to_lowercase(domain.c_str()), reinterpret_cast<void(*)(char *)>(&::free));
 * \endcode
 *
 * \param[in] domain  The input domain to convert to lowercase.
 *
 * \return A pointer to the resulting conversion, NULL if the buffer
 *         cannot be allocated or the input data is considered invalid.
 */
char *tld_domain_to_lowercase(const char *domain)
{
    int len = (domain == (const char *) 0 ? 0 : strlen(domain) * 2);
    wint_t wc;
    char *result;
    char *output;

    if(len == 0)
    {
        return (char *) 0;
    }

    // we cannot change the input buffer, plus our result may be longer
    // than the input...
    result = malloc(len + 1);
    if(result == (char *) 0)
    {
        return (char *) 0; // LCOV_EXCL_LINE
    }

    output = result;
    for(;;)
    {
        wc = tld_mbtowc(&domain);
        // wint_t is expected to be unsigned so we need a cast here
        if((int) wc == -1)
        {
            free(result);
            return (char *) 0;
        }
        if(wc == L'\0')
        {
            *output = '\0';
            return result;
        }
        if(tld_wctomb(wc, &output, &len) != 0)
        {
            // could not encode; buffer is probably full
            free(result);
            return (char *) 0;
        }
    }
    /*NOTREACHED*/
}

/* vim: ts=4 sw=4 et
 */
