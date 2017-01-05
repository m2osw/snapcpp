/* TLD library -- test converting domain names to lowercase
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
 * \brief Test the tld_domain_to_lowercase() function.
 *
 * This file implements various test to verify that the
 * tld_domain_to_lowercase() function works as expected.
 */

#include "libtld/tld.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <wctype.h>

int err_count = 0;
int verbose = 0;



void test_add_byte(char **out, int wc, int force_caps)
{
    if((wc >= 'A' && wc <= 'Z')
    || (wc >= 'a' && wc <= 'z')
    || (wc >= '0' && wc <= '9')
    || wc == '.'
    || wc == '-'
    || wc == '!'
    || wc == '~'
    || wc == '/'
    || wc == '_')
    {
        **out = wc;
        ++*out;
    }
    else
    {
        // add '%XX' where X represents an hexadecimal digit
        if(force_caps
        || (rand() & 1) != 0)
        {
            sprintf(*out, "%%%02X", wc);
        }
        else
        {
            sprintf(*out, "%%%02x", wc);
        }
        *out += 3;
    }
}


void test_to_utf8(char **out, int wc, int force_caps)
{
    if(wc < 0x80)
    {
        test_add_byte(out, wc, force_caps);
    }
    else if(wc < 0x800)
    {
        test_add_byte(out, ((wc >> 6) | 0xC0), force_caps);
        test_add_byte(out, ((wc & 0x3F) | 0x80), force_caps);
    }
    else if(wc < 0x10000)
    {
        test_add_byte(out, ((wc >> 12) | 0xE0), force_caps);
        test_add_byte(out, (((wc >> 6) & 0x3F) | 0x80), force_caps);
        test_add_byte(out, ((wc & 0x3F) | 0x80), force_caps);
    }
    else
    {
        test_add_byte(out, ((wc >> 18) | 0xF0), force_caps);
        test_add_byte(out, (((wc >> 12) & 0x3F) | 0x80), force_caps);
        test_add_byte(out, (((wc >> 6) & 0x3F) | 0x80), force_caps);
        test_add_byte(out, ((wc & 0x3F) | 0x80), force_caps);
    }
}


void test_all_characters()
{
    int  wc;
    char buf[256], *s, *r;

    for(wc = 1; wc < 0x110000; ++wc)
    {
        if((wc >= 0xD800 && wc <= 0xDFFF)  // UTF-16 stuff ignored
        || (wc & 0xFFFF) == 0xFFFE
        || (wc & 0xFFFF) == 0xFFFF
        || wc == '/')
        {
            // those code points must be ignored because they
            // really don't work in a domain name
            continue;
        }

        s = buf;
        test_to_utf8(&s, wc, 0);
        *s = '\0';

        r = tld_domain_to_lowercase(buf);

        s = buf;
        test_to_utf8(&s, towlower(wc), 1); // force caps in %XX notication
        *s = '\0';

        if(r == NULL)
        {
            fprintf(stderr, "error: character 0x%06X generated an error and tld_domain_to_lower() returned NULL (expected \"%s\")\n", wc, buf);
        }
        else
        {
            if(strcmp(r, buf) != 0)
            {
                fprintf(stderr, "error: character 0x%06X was not converted back and force as expected (expected \"%s\", received \"%s\")\n", wc, buf, r);
            }

            // done with the result
            free(r);
        }
    }
}


void test_empty()
{
    char *r;

    // NULL as input, returns NULL
    r = tld_domain_to_lowercase(NULL);
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(NULL) is expected to return NULL.\n");
    }

    // an empty string also returns NULL as result
    r = tld_domain_to_lowercase("");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"\") is expected to return NULL.\n");
    }
}


void test_invalid_xx()
{
    char *r;
    char buf[256];
    int i;

    r = tld_domain_to_lowercase("%AZ");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"%%AZ\") is expected to return NULL.\n");
    }

    r = tld_domain_to_lowercase("%ZA");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"%%ZA\") is expected to return NULL.\n");
    }

    // these are 3 x a with an acute accent (as used in Spanish)
    r = tld_domain_to_lowercase("\xC3\xA1\xC3\xA1\xC3\xA1");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"\xC3\xA1\xC3\xA1\xC3\xA1\") is expected to return NULL because of an overflow.\n");
    }

    // these are 2 x a with an acute accent followed by "ab"
    // this time the overflow happens when the 'a' is hit
    r = tld_domain_to_lowercase("\xC3\xA1\xC3\xA1\x61\x62");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"\xC3\xA1\xC3\xA1\x61\x62\") is expected to return NULL because of an overflow.\n");
    }

    // these are 3 x 0x0911 (Devanagari letter candra o)
    r = tld_domain_to_lowercase("\xE0\xA4\x91\xE0\xA4\x91\xE0\xA4\x91");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"\xE0\xA4\x91\xE0\xA4\x91\xE0\xA4\x91\") is expected to return NULL because of an overflow.\n");
    }

    // these are 2 x 0x0911 and a # in between (Devanagari letter candra o)
    r = tld_domain_to_lowercase("\xE0\xA4\x91#\xE0\xA4\x91");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"\xE0\xA4\x91#\xE0\xA4\x91\") is expected to return NULL because of an overflow.\n");
    }

    // these are 2 x 0x0911 and a q in between (Devanagari letter candra o)
    r = tld_domain_to_lowercase("\xE0\xA4\x91q\xE0\xA4\x91");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"\xE0\xA4\x91q\xE0\xA4\x91\") is expected to return NULL because of an overflow.\n");
    }

    // these are 3 x 0x13F0B (Miao letter da)
    r = tld_domain_to_lowercase("\xF0\x96\xBC\x8B\xF0\x96\xBC\x8B\xF0\x96\xBC\x8B");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"\xF0\x96\xBC\x8B\xF0\x96\xBC\x8B\xF0\x96\xBC\x8B\") is expected to return NULL because of an overflow.\n");
    }

    // these are 2 x 0x13F0B with # in between (Miao letter da)
    r = tld_domain_to_lowercase("\xF0\x96\xBC\x8B#\xF0\x96\xBC\x8B");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"\xF0\x96\xBC\x8B#\xF0\x96\xBC\x8B\") is expected to return NULL because of an overflow.\n");
    }

    // these are 2 x 0x13F0B with q in between (Miao letter da)
    r = tld_domain_to_lowercase("\xF0\x96\xBC\x8Bq\xF0\x96\xBC\x8B");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"\xF0\x96\xBC\x8Bq\xF0\x96\xBC\x8B\") is expected to return NULL because of an overflow.\n");
    }

    // these are 2 x 0x13F0B with qq in between (Miao letter da)
    r = tld_domain_to_lowercase("\xF0\x96\xBC\x8Bqq\xF0\x96\xBC\x8B");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"\xF0\x96\xBC\x8Bqq\xF0\x96\xBC\x8B\") is expected to return NULL because of an overflow.\n");
    }

    // these are 2 x 0x13F0B with qqq in between (Miao letter da)
    r = tld_domain_to_lowercase("\xF0\x96\xBC\x8Bqqq\xF0\x96\xBC\x8B");
    if(r != NULL)
    {
        ++err_count;
        fprintf(stderr, "error: tld_domain_to_lowercase(\"\xF0\x96\xBC\x8Bqqq\xF0\x96\xBC\x8B\") is expected to return NULL because of an overflow.\n");
    }

    for(i = 0xF8; i <= 0xFF; ++i)
    {
        snprintf(buf, sizeof(buf), "+%%%02X+", i);

        r = tld_domain_to_lowercase(buf);
        if(r != NULL)
        {
            ++err_count;
            fprintf(stderr, "error: tld_domain_to_lowercase(\"%s\") is expected to return NULL because of the invalid byte (introduction byte).\n", buf);
        }
    }

    for(i = 0x80; i <= 0xBF; ++i)
    {
        snprintf(buf, sizeof(buf), "+%%%02X+", i);

        r = tld_domain_to_lowercase(buf);
        if(r != NULL)
        {
            ++err_count;
            fprintf(stderr, "error: tld_domain_to_lowercase(\"%s\") is expected to return NULL because of the invalid byte (continuation byte).\n", buf);
        }
    }

    // byte missing (end of string found before end of UTF-8 character)
    for(i = 0xC0; i <= 0xF7; ++i)
    {
        buf[0] = i;
        buf[1] = '\0';
        r = tld_domain_to_lowercase(buf);
        if(r != NULL)
        {
            ++err_count;
            fprintf(stderr, "error: tld_domain_to_lowercase(\"%s\") is expected to return NULL because of the invalid UTF-8 sequence (end of string found too early).\n", buf);
        }
    }

    // continuation byte out of range
    for(i = 0x00; i <= 0xFF; ++i)
    {
        if(i >= 0x80 && i <= 0xBF)
        {
            // that's a valid continuation
            continue;
        }
        buf[0] = rand() % (0xF8 - 0xC0) + 0xC0;
        buf[1] = i;
        buf[2] = '\0';
        r = tld_domain_to_lowercase(buf);
        if(r != NULL)
        {
            ++err_count;
            fprintf(stderr, "error: tld_domain_to_lowercase(\"%s\") is expected to return NULL because of the invalid UTF-8 sequence (continuation byte out of range).\n", buf);
        }
    }

    for(i = 1; i < 0x110000; ++i)
    {
        if((i >= 0xD800 && i <= 0xDFFF)  // UTF-16 stuff ignored
        || (i & 0xFFFF) == 0xFFFE
        || (i & 0xFFFF) == 0xFFFF)
        {
            r = buf;
            test_to_utf8(&r, i, rand() & 1);
            *r = '\0';

            r = tld_domain_to_lowercase(buf);
            if(r != NULL)
            {
                ++err_count;
                fprintf(stderr, "error: tld_domain_to_lowercase(\"%s\") is expected to return NULL because of the invalid UTF-8 sequence (continuation byte out of range).\n", buf);
            }
        }
    }

    for(i = 0x110000;; ++i)
    {
        r = buf;
        test_to_utf8(&r, i, rand() & 1);
        *r = '\0';

        // we only save up to 4 bytes, so to check overflow, we expect %F0
        // as the first byte...
        if(strncmp(buf, "%f0", 3) == 0
        || strncmp(buf, "%F0", 3) == 0)
        {
            // no need to test further, we hit the case of 0xF8 or more in
            // the first byte which is checked somewhere else
            break;
        }

        r = tld_domain_to_lowercase(buf);
        if(r != NULL)
        {
            ++err_count;
            fprintf(stderr, "error: tld_domain_to_lowercase(\"%s\") is expected to return NULL because of the invalid Unicode character. Got \"%s\" instead.\n", buf, r);
        }
    }
}


int main(int argc, char *argv[])
{
    int i;
    int seed = time(NULL);

    for(i = 1; i < argc; ++i)
    {
        if(strcmp(argv[i], "-v") == 0)
        {
            verbose = 1;
        }
        else if(strcmp(argv[i], "--seed") == 0)
        {
            if(i + 1 >= argc)
            {
                fprintf(stderr, "error: --seed expect a value.\n");
                exit(1);
            }
            seed = atol(argv[i + 1]);
        }
    }

    printf("testing tld test domain lowercase version %s with seed %d\n", tld_version(), seed);

    srand(seed);

    test_empty();
    test_all_characters();
    test_invalid_xx();

    exit(err_count ? 1 : 0);
}

/* vim: ts=4 sw=4 et
 */

