/* TLD library -- TLD example
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
 * \brief Simple usage example of the C library.
 *
 * This file is an example of usage of the tld() function defined in the
 * libtld library. As another example, you can check the following file
 * in the source package:
 *
 * src/validate_tld.cpp
 *
 * Although it is written in C++, it shows the different ways to check
 * a URI and an email address.
 */

#include "libtld/tld.h"
#include <stdio.h>
#include <stdlib.h>


/** \brief Simple example of a call to the tld() function.
 *
 * The following is a very simple code sample used to present the libtld
 * library usage.
 *
 * To test different URIs, edit the file and change the uri variable
 * defined below.
 *
 * \include example.c
 *
 * \param[in] argc  Number of command line arguments passed in.
 * \param[in] argv  The arguments passed in.
 *
 * \return The function returns 0 on success, 1 otherwise.
 */
int main(int argc, char *argv[])
{
    const char *uri = "WWW.Example.Co.Uk";
    char *uri_lowercase;
    struct tld_info info;
    enum tld_result r;

    if(argc > 1)
    {
        uri = argv[1];
    }

    // if your input may include uppercase characters and you
    // do not have an easy way to compute the lowercase before
    // calling tld(), call the tld_domain_to_lowercase() function
    uri_lowercase = tld_domain_to_lowercase(uri);

    r = tld(uri_lowercase, &info);
    if(r == TLD_RESULT_SUCCESS)
    {
        const char *s = uri_lowercase + info.f_offset - 1;
        while(s > uri_lowercase)
        {
            if(*s == '.')
            {
                ++s;
                break;
            }
            --s;
        }
        // here uri_lowercase points to your sub-domains, the length is
        // "s - uri_lowercase"
        // if uri_lowercase == s then there are no sub-domains
        // s points to the domain name, the length is "info.f_tld - s"
        // and info.f_tld points to the TLD
        //
        // When TLD_RESULT_SUCCESS is returned the domain cannot be an
        // empty string; also the TLD cannot be empty, however, there
        // may be no sub-domains.
        printf("Sub-domain(s): \"%.*s\"\n", (int)(s - uri_lowercase), uri_lowercase);
        printf("Domain: \"%.*s\"\n", (int)(info.f_tld - s), s);
        printf("TLD: \"%s\"\n", info.f_tld);
        free(uri_lowercase);
        return 0;
    }

    free(uri_lowercase);
    return 1;
}

// vim: ts=4 sw=4 et
