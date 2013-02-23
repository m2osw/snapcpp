/* TLD library -- test the TLD interface
 * Copyright (C) 2011-2013  Made to Order Software Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "libtld/tld.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

/* we get access to the table with all the TLDs so we can go through them all
 * the library does not give direct access by default... (although maybe we
 * could give users access to the data)
 */
#include "tld_data.h"
extern const struct tld_description tld_descriptions[];
extern unsigned short tld_start_offset;
extern unsigned short tld_end_offset;

int err_count = 0;


void error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}


/// \brief Structure used to define many tests to run against the tld_check_uri() function
struct test_info
{
    /// The URI to be tested
    const char *        f_uri;
    /// The valid protocols for that test
    const char *        f_protocols;
    /// Flags used to call the tld_check_uri() function
    int                 f_flags;
    /// The expected result
    enum tld_result     f_result;
    /// The expected info values
    struct tld_info     f_info;
};

/* WARNING: the first and last protocols need to be tested for full
 *          validity of the library; if you add more, add them in the
 *          middle or update the list of entries below to include the
 *          new start/end protocols.
 */
#define PROTOCOLS "ftp,sftp,http,https,gopher"

const struct test_info test_info_entries[] =
{
    /* NULL entries */
    {
      NULL,
      PROTOCOLS,
      0,
      TLD_RESULT_NULL,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "",
      PROTOCOLS,
      0,
      TLD_RESULT_NULL,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    /* invalid entries */
    {
      "bad://www.m2osw.th/bad-protocol",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "b!d://www.m2osw.cg/bad-protocol/character",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "https:/www.m2osw.sx.cn/missing/slash",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "http:q/www.m2osw.lezajsk.pl/q/instead/of/slash",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "https::/www.m2osw.museum.mv/two/colons",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "http:///www.m2osw.rec.co/3/slashes",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "https://@www.m2osw.co.zm/bad?user=info",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "https://@www.m2osw.mil.py@/bad?user=info",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "https://:password1@www.m2osw.net.tn/bad?user=info",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "https://alexis:@www.m2osw.int.rw/bad?user=info",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "https://:@www.m2osw.l.lc/bad?user=info",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "https://www.m2osw.mil.sh:/bad?port=empty",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "https://www.m2osw.krasnoyarsk.ru:abc/bad?port=invalid&p=number",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "sftp://www.m2osw.cat.tt:-33/bad?port=unexpected number",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "https://www.m2osw.cat.tt/bad/variable/name?=value",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "http://www.m2osw.edu.pt/bar%xx/percent",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "http://www.m2osw.tjeldsund.no/con\trol/character",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "http://www.m2osw.edu.pt/con%09rol/character", /* %09 = \t */
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "http://www.m2osw.other.nf/space character",
      PROTOCOLS,
      VALID_URI_NO_SPACES,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "http://www.m2osw.co.mp/space+character",
      PROTOCOLS,
      VALID_URI_NO_SPACES,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "http://www.m2osw.bv.nl/space%20character",
      PROTOCOLS,
      VALID_URI_NO_SPACES,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "http://www.m2osw.gov.pk/bad/variable/name?good=value&=at&the=beginning",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "http://www.m2osw.rakpetroleum.om/bad/variable/name?good=value&at=the&=beginning",
      PROTOCOLS,
      0,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    {
      "http://www.m2osw.and\xF8y.no/forbid/special-characters",
      PROTOCOLS,
      VALID_URI_ASCII_ONLY,
      TLD_RESULT_BAD_URI,
      { TLD_CATEGORY_UNDEFINED, TLD_STATUS_UNDEFINED, NULL, NULL, -1 }
    },
    /* valid entries */
    {
      "https://www.m2osw.co.uk/simple",
      PROTOCOLS,
      0,
      TLD_RESULT_SUCCESS,
      { TLD_CATEGORY_COUNTRY, TLD_STATUS_VALID, "United Kingdom", ".co.uk/", 17 }
    },
    {
      "ftp://www.m2osw.ltd%2Euk/encoded/period", /* still encoded */
      PROTOCOLS,
      0,
      TLD_RESULT_SUCCESS,
      { TLD_CATEGORY_COUNTRY, TLD_STATUS_VALID, "United Kingdom", ".ltd%2Euk/", 15 }
    },
    {
      "http://m2osw.org.nr/encoded/period?#&=55", /* looks like an empty variable name */
      PROTOCOLS,
      0,
      TLD_RESULT_SUCCESS,
      { TLD_CATEGORY_COUNTRY, TLD_STATUS_VALID, "Nauru", ".org.nr/", 12 }
    },
    {
      "http://snap.m2osw.blogspot.mx:888/longer/address",
      PROTOCOLS,
      0,
      TLD_RESULT_SUCCESS,
      { TLD_CATEGORY_ENTREPRENEURIAL, TLD_STATUS_VALID, NULL, ".blogspot.mx:", 17 }
    },
    {
      "HTTP://alexis:password1@www.m2osw.com:888/correct/uri?with=variable",
      PROTOCOLS,
      0,
      TLD_RESULT_SUCCESS,
      { TLD_CATEGORY_INTERNATIONAL, TLD_STATUS_VALID, NULL, ".com:", 33 }
    },
    {
      "ftp://ftp.m2osw.me/freeware/download/snap/snap-1.0.tar.gz",
      PROTOCOLS,
      0,
      TLD_RESULT_SUCCESS,
      { TLD_CATEGORY_COUNTRY, TLD_STATUS_VALID, "Montenegro", ".me/", 15 }
    },
    {
      "gopher://news.m2osw.ca.us:71/protocols",
      PROTOCOLS,
      0,
      TLD_RESULT_SUCCESS,
      { TLD_CATEGORY_COUNTRY, TLD_STATUS_VALID, "United States", ".ca.us:", 19 }
    },
    {
      "random://news.m2osw.edu.mm/asterisk/protocols",
      "*",
      0,
      TLD_RESULT_SUCCESS,
      { TLD_CATEGORY_COUNTRY, TLD_STATUS_VALID, "Republic of the Union of Myanmar", ".edu.mm/", 19 }
    },
};
#define    test_info_entries_length (sizeof(test_info_entries) / sizeof(test_info_entries[0]))


void test_uri()
{
    struct tld_info info;
    enum tld_result result;
    int i;

    for(i = 0; i < test_info_entries_length; ++i)
    {
        //fprintf(stderr, "testing [%s]\n", test_info_entries[i].f_uri);
        result = tld_check_uri(test_info_entries[i].f_uri, &info, test_info_entries[i].f_protocols, test_info_entries[i].f_flags);
        if(result != test_info_entries[i].f_result)
        {
            fprintf(stderr, "error:%s: URI failed with the wrong result: %d\n", test_info_entries[i].f_uri, result);
            ++err_count;
        }

        if(test_info_entries[i].f_info.f_tld == NULL)
        {
            if(info.f_tld != NULL)
            {
                fprintf(stderr, "error:%s: TLD was expected to be NULL, got \"%s\" instead.\n", test_info_entries[i].f_uri, info.f_tld);
                ++err_count;
            }
        }
        else
        {
            if(info.f_tld == NULL)
            {
                fprintf(stderr, "error:%s: TLD was not expected to be NULL but \"%s\" instead.\n", test_info_entries[i].f_uri, test_info_entries[i].f_info.f_tld);
                ++err_count;
            }
            else if(strncmp(test_info_entries[i].f_info.f_tld, info.f_tld, strlen(test_info_entries[i].f_info.f_tld)) != 0)
            {
                fprintf(stderr, "error:%s: TLD was not properly extracted: \"%s\".\n", test_info_entries[i].f_uri, info.f_tld);
                ++err_count;
            }
        }

        if(info.f_category != test_info_entries[i].f_info.f_category)
        {
            fprintf(stderr, "error:%s: category was not properly extracted.\n", test_info_entries[i].f_uri);
            ++err_count;
        }

        if(info.f_status != test_info_entries[i].f_info.f_status)
        {
            fprintf(stderr, "error:%s: category was not properly extracted.\n", test_info_entries[i].f_uri);
            ++err_count;
        }

        if(test_info_entries[i].f_info.f_country == NULL)
        {
            if(info.f_country != NULL)
            {
                fprintf(stderr, "error:%s: country was expected to be NULL.\n", test_info_entries[i].f_uri);
                ++err_count;
            }
        }
        else
        {
            if(info.f_country == NULL)
            {
                fprintf(stderr, "error:%s: country is NULL when it was expected to be \"%s\".\n", test_info_entries[i].f_uri, test_info_entries[i].f_info.f_country);
                ++err_count;
            }
            else if(strcmp(info.f_country, test_info_entries[i].f_info.f_country) != 0)
            {
                fprintf(stderr, "error:%s: country was not properly extracted.\n", test_info_entries[i].f_uri);
                ++err_count;
            }
        }

        if(info.f_offset != test_info_entries[i].f_info.f_offset)
        {
            fprintf(stderr, "error:%s: TLD offset is not the expected value (%d).\n", test_info_entries[i].f_uri, info.f_offset);
            ++err_count;
        }
    }
}









int main(int argc, char *argv[])
{
    fprintf(stderr, "testing tld version %s: tld_valid_uri() function\n", tld_version());

    /* call all the tests, one by one
     * failures are "recorded" in the err_count global variable
     * and the process stops with an error message and exit(1)
     * if err_count is not zero.
     */
    test_uri();

    if(err_count)
    {
        fprintf(stderr, "%d error%s occured.\n",
                    err_count, err_count != 1 ? "s" : "");
    }
    exit(err_count ? 1 : 0);
}

/* vim: ts=4 sw=4 et
 */
