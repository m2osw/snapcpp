/* TLD library -- test the C++ TLD interface
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
 * \brief Test the tld() function through the C++ object.
 *
 * This file implements various test to verify that the
 * tld() function functions as expected in C++.
 */

#include "libtld/tld.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//#include <limits.h>

int err_count = 0;
int verbose = 0;


void error(const std::string& msg)
{
    fprintf(stderr, "%s\n", msg.c_str());
    ++err_count;
}


#define EXPECTED_THROW(f) \
    try \
    { \
        static_cast<void>(bad.f()); \
        error("error: bad." #f "() of \"\" did not throw an error."); \
    } \
    catch(const invalid_domain&) \
    { \
    }



void test_valid_uri(const char *uri, const char *tld, const char *domain, const char *sub_domains, tld_category category, const char *country)
{
    if(verbose)
    {
        printf("testing uri \"%s\"\n", uri);
    }

    // create object with a URI
    tld_object o(uri);

    if(!o.is_valid())
    {
        char r[32], s[32];
        snprintf(r, sizeof(r), "%d", o.result());
        snprintf(s, sizeof(s), "%d", o.status());
        error("error: o.is_valid() of \"" + std::string(uri) + "\" result is not true (result: " + r + ", status: " + s + ").");
        return;
    }

    if(o.result() != TLD_RESULT_SUCCESS)
    {
        error("error: o.result() of \"" + std::string(uri) + "\" result was not success.");
    }

    if(o.status() != TLD_STATUS_VALID)
    {
        error("error: o.status() of \"" + std::string(uri) + "\" result was not valid.");
    }

    if(!o.is_valid())
    {
        error("error: o.is_valid() of \"" + std::string(uri) + "\" result was not valid.");
    }

    if(o.domain() != uri)
    {
        error("error: o.domain() of \"" + std::string(uri) + "\" result was not valid.");
    }

    if(o.sub_domains() != sub_domains)
    {
        error("error: o.sub_domains() of \"" + std::string(uri) + "\" result was not valid.");
    }

    if(o.full_domain() != domain + std::string(tld))
    {
        error("error: o.full_domain() of \"" + std::string(uri) + "\" result was not valid.");
    }

    if(o.domain_only() != domain)
    {
        error("error: o.domain_only() of \"" + std::string(uri) + "\" result was not valid.");
    }

    if(o.tld_only() != tld)
    {
        error("error: o.tld_only() of \"" + std::string(uri) + "\" result was not valid.");
    }

    if(o.category() != category)
    {
        error("error: o.category() of \"" + std::string(uri) + "\" result was not valid.");
    }

    if(o.country() != (country == NULL ? "" : country))
    {
        error("error: o.country() of \"" + std::string(uri) + "\" result was not valid.");
    }
}



void test_invalid()
{
    {
        // when empty that's equivalent to NULL or ""
        if(verbose)
        {
            printf("testing NULL pointer\n");
        }

        tld_object bad;

        if(bad.is_valid())
        {
            char r[32], s[32];
            snprintf(r, sizeof(r), "%d", bad.result());
            snprintf(s, sizeof(s), "%d", bad.status());
            error(std::string("error:test_invalid(1): bad.is_valid() of \"\" result is true?! (result: ") + r + ", status: " + s + ").");
            return;
        }

        if(bad.status() != TLD_STATUS_UNDEFINED)
        {
            error("error: bad.status() of \"\" is not UNDEFINED.");
            return;
        }

        if(bad.result() != TLD_RESULT_NULL)
        {
            error("error: bad.status() of \"\" is not NULL.");
            return;
        }

        if(bad.domain() != "")
        {
            error("error:test_invalid(1): bad.domain() did not return \"\" as expected.");
        }

        EXPECTED_THROW(sub_domains);
        EXPECTED_THROW(full_domain);
        EXPECTED_THROW(domain_only);
        EXPECTED_THROW(tld_only);

        if(bad.category() != TLD_CATEGORY_UNDEFINED)
        {
            error("error: bad.category() did not return UNDEFINED as expected.");
        }

        if(bad.country() != "")
        {
            error("error: bad.country() did not return \"\" as expected, got \"" + bad.country() + "\" instead.");
        }
    }

    {
        // unknown TLD
        if(verbose)
        {
            printf("testing \"www.example.unknown\"\n");
        }

        std::string uri("www.example.unknown");
        tld_object bad(uri);

        if(bad.is_valid())
        {
            char r[32], s[32];
            snprintf(r, sizeof(r), "%d", bad.result());
            snprintf(s, sizeof(s), "%d", bad.status());
            error(std::string("error:test_invalid(2): bad.is_valid() of \"\" result is true?! (result: ") + r + ", status: " + s + ").");
            return;
        }

        if(bad.status() != TLD_STATUS_UNDEFINED)
        {
            error("error: bad.status() of \"\" is not UNDEFINED.");
            return;
        }

        if(bad.result() != TLD_RESULT_NOT_FOUND)
        {
            char r[32];
            snprintf(r, sizeof(r), "%d", bad.result());
            error(std::string("error: bad.result() of \"\" is ") + r + " instead of NOT_FOUND (5).");
            return;
        }

        // TBD: if we clear the f_domain then this would be ""
        if(bad.domain() != "www.example.unknown")
        {
            error("error:test_invalid(2): bad.domain() did not return \"\" as expected.");
        }

        EXPECTED_THROW(sub_domains);
        EXPECTED_THROW(full_domain);
        EXPECTED_THROW(domain_only);
        EXPECTED_THROW(tld_only);

        if(bad.category() != TLD_CATEGORY_UNDEFINED)
        {
            error("error: bad.category() did not return UNDEFINED as expected.");
        }

        if(bad.country() != "")
        {
            error("error: bad.country() did not return \"\" as expected, got \"" + bad.country() + "\" instead.");
        }
    }

    {
        if(verbose)
        {
            printf("testing \"el.eritrea.er\"\n");
        }

        // invalid TLD
        std::string uri("el.eritrea.er");
        tld_object bad(uri);

        if(bad.is_valid())
        {
            char r[32], s[32];
            snprintf(r, sizeof(r), "%d", bad.result());
            snprintf(s, sizeof(s), "%d", bad.status());
            error(std::string("error:test_invalid(3): bad.is_valid() of \"\" result is true?! (result: ") + r + ", status: " + s + ").");
            return;
        }

        if(bad.status() != TLD_STATUS_UNUSED)
        {
            error("error: bad.status() of \"\" is not UNUSED.");
            return;
        }

        if(bad.result() != TLD_RESULT_INVALID)
        {
            char r[32];
            snprintf(r, sizeof(r), "%d", bad.result());
            error(std::string("error: bad.result() of \"\" is ") + r + " instead of INVALID (1).");
            return;
        }

        // TBD: if we clear the f_domain then this would be ""
        if(bad.domain() != "el.eritrea.er")
        {
            error("error:test_invalid(3): bad.domain() did not return \"\" as expected.");
        }

        EXPECTED_THROW(sub_domains);
        EXPECTED_THROW(full_domain);
        EXPECTED_THROW(domain_only);
        EXPECTED_THROW(tld_only);

        if(bad.category() != TLD_CATEGORY_COUNTRY)
        {
            error("error: bad.category() did not return COUNTRY as expected.");
        }

        if(bad.country() != "Eritrea")
        {
            error("error: bad.country() did not return \"Eritrea\" as expected, got \"" + bad.country() + "\" instead.");
        }
    }
}



int main(int argc, char *argv[])
{
    printf("testing tld object version %s\n", tld_version());

    if(argc > 1)
    {
        if(strcmp(argv[1], "-v") == 0)
        {
            verbose = 1;
        }
    }

    /* Call all the tests, one by one.
     *
     * Failures are "recorded" in the err_count global variable
     * and the process stops with an error message and exit(1)
     * if err_count is not zero.
     *
     * Exceptions that should not occur are expected to also
     * be caught and reported as errors.
     */
    try
    {
        test_valid_uri("test-with-a-dash.mat.br", ".mat.br", "test-with-a-dash", "", TLD_CATEGORY_COUNTRY, "Brazil"); // no sub-domains
        test_valid_uri("www.m2osw.com", ".com", "m2osw", "www", TLD_CATEGORY_INTERNATIONAL, NULL); // one sub-domains (standard .com)
        test_valid_uri("test.valid.uri.domain.com.ac", ".com.ac", "domain", "test.valid.uri", TLD_CATEGORY_COUNTRY, "Ascension Island"); // many sub-domains
        test_valid_uri("sub-domain.www.ck", ".ck", "www", "sub-domain", TLD_CATEGORY_COUNTRY, "Cook Islands"); // exception test

        test_invalid();
    }
    catch(const invalid_domain&)
    {
        error("error: caught an exception when everything is expected to be valid.");
    }

    if(err_count)
    {
        fprintf(stderr, "%d error%s occured.\n",
                    err_count, err_count != 1 ? "s" : "");
    }
    exit(err_count ? 1 : 0);
}

/* vim: ts=4 sw=4 et
 */
