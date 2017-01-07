/* TLD library -- test the TLD interface against the Public Suffix List
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
 * tld() function works as expected with valid and
 * invalid names.
 */

// Qt headers make use of long long which is not considered a valid type
#pragma GCC diagnostic ignored "-Wlong-long"

#include "libtld/tld.h"
#include <string>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <boost/algorithm/string.hpp>
#include <QtCore/QString>



int err_count = 0;
int verbose = 0;

/*
 * This test calls the tld() function with all the TLDs as defined
 * by Mozilla to determine whether we are up to date.
 *
 * extern enum tld_result tld(const char *uri, struct tld_info *info);
 */

typedef std::vector<std::string> string_vector_t;
string_vector_t tlds;


/** \brief Encode a URL.
 *
 * This function transforms the characters in a valid URI string.
 */
QString tld_encode(const QString& tld, int& level)
{
    QString result;
    level = 0;

    QByteArray utf8 = tld.toUtf8();
    int max(utf8.length());
    const char *p = utf8.data();
    for(int l = 0; l < max; ++l)
    {
        char c(p[l]);
        if(static_cast<unsigned char>(c) < 0x20)
        {
            fprintf(stderr, "error: controls characters (^%c) are not allowed in TLDs (%s).\n", c, p);
            exit(1);
        }
        if((c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '.' || c == '-')
        {
            // these are accepted as is; note that we already checked the
            // validty of the data w
            if(c == '.')
            {
                ++level;
            }
            result += c;
        }
        else
        {
            // add/remove as appropriate
            if(c == '/' || c == ':' || c == '&')
            {
                fprintf(stderr, "error: character (^%c) is not allowed in TLDs.\n", c);
                exit(1);
            }
            result += '%';
            QString v(QString("%1").arg(c & 255, 2, 16, QLatin1Char('0')));
            result += v[0];
            result += v[1];
        }
    }
    // at this time the maximum level we declared is 4 but there are cases
    // where countries defined 5 levels (which is definitively crazy!)
    // there is also one Amazon server using 6 levels
    if(level < 0 || level > 6)
    {
        fprintf(stderr, "error: level out of range (%d) in \"%s\"; if larger than the maximum limit, you may want to increase the limit.\n", level, p);
        exit(1);
    }

    return result;
}


/*
 * The function reads the public_suffix_list.dat file in memory.
 *
 * We call exit(1) if we find an error while reading the data.
 */
void test_load()
{
    FILE *f = fopen("public_suffix_list.dat", "r");
    if(f == NULL)
    {
        fprintf(stderr, "error: could not open the \"public_suffix_list.dat\" file; did you start the test in the source directory?\n");
        exit(1);
    }
    char buf[256];
    buf[sizeof(buf) -1] = '\0';
    int line(0);
    while(fgets(buf, sizeof(buf) - 1, f) != NULL)
    {
        ++line;
        int const l = strlen(buf);
        if(l == sizeof(buf) - 1)
        {
            // the fgets() failed in this case so forget it
            fprintf(stderr, "public_suffix_list.dat:%d:error: line too long.\n", line);
            ++err_count;
        }
        else
        {
            std::string s(buf);
            boost::algorithm::trim(s);
            if(s.length() == 1)
            {
                // all TLDs are at least 2 characters
                fprintf(stderr, "public_suffix_list.dat:%d:error: a TLD must be at least two characters.\n", line);
                ++err_count;
            }
            else if(s.length() > 1 && s[0] != '/' && s[1] != '/')
            {
                // this is not a comment and not an empty line, that's a TLD
                tlds.push_back(s);
//printf("found [%s]\n", s.c_str());
            }
        }
    }
    fclose(f);
    if(verbose)
    {
        printf("Found %d TLDs in the input file.\n", static_cast<int>(tlds.size()));
    }
}


/*
 * This test checks out URIs that end with an invalid TLD. This is
 * expected to return an error every single time.
 */
void test_tlds()
{
    for(string_vector_t::const_iterator it(tlds.begin()); it != tlds.end(); ++it)
    {
        tld_info info;

        // note: it is possible for the input to have an asterisk (*) anywhere
        //       in the name, although at this time it only appears at the
        //       start and we just handle it as a special case here
        //
        if(it->at(0) == '*'
        && it->at(1) == '.')
        {
            // as is (well, without the '*'), a '*.tld' must return INVALID
            // and status UNUSED
            //
            std::string base_tld(it->substr(2));
            if(base_tld.find('.') == std::string::npos)
            {
                // at least one '.', however for one such as '*.example.com'
                // we just want the 'example.com' part, no extra '.',
                // otherwise the test itself would fail.
                //
                base_tld = "." + base_tld;
            }
            tld_result r = tld(base_tld.c_str(), &info);
            if(r != TLD_RESULT_INVALID)
            {
                // we're good if invalid since that's what we expect in this case
                // any other result is an error
                fprintf(stderr, "error: tld(\"%s\", &info) for \"%s\" expected %d, got %d instead.\n",
                            base_tld.c_str(),
                            it->c_str(),
                            TLD_RESULT_INVALID,
                            r);
                ++err_count;
            }

            // then try with a sub-name, in most cases it is invalid
            // although it can be success (it depends on whether the
            // '*' has a few specific cases or none at all)
            //
            std::string url("we-want-to-test-just-one-domain-name");
            url += it->substr(1);
            r = tld(url.c_str(), &info);
            if(r == TLD_RESULT_SUCCESS)
            {
                // if it worked then we have a problem
                //
                fprintf(stderr,
                        "error: tld(\"%s\", &info) accepted when 2nd or 3rd level names are not accepted by public_suffix_list.dat.\n",
                        url.c_str());
                ++err_count;
            }
            else if(r != TLD_RESULT_INVALID)
            {
                // we're good if invalid since that's what we expect in this case
                // any other result is an error
                fprintf(stderr, "error: tld(\"%s\", &info) for \"%s\" failed with %d.\n",
                            url.c_str(), it->c_str(), r);
                ++err_count;
            }
        }
        else if(it->at(0) == '!')
        {
            std::string url;//("we-want-to-test-just-one-domain-name.");
            url += it->substr(1);
            tld_result r = tld(url.c_str(), &info);
            if(r != TLD_RESULT_SUCCESS)
            {
                // if it worked then we have a problem
                fprintf(stderr, "error: tld(\"%s\", &info) = %d failed with an exception that should have been accepted.\n",
                        it->c_str(), r);
                ++err_count;
            }
        }
        else if(it->at(0) != '!')
        {
            std::string url("www.this-is-a-long-domain-name-that-should-not-make-it-in-a-tld.");
            url += *it;
            int level;
            QString utf16(QString::fromUtf8(url.c_str()));
            QString u(tld_encode(utf16, level));
            QByteArray uri(u.toUtf8());
            tld_result r = tld(uri.data(), &info);
            if(r == TLD_RESULT_SUCCESS || r == TLD_RESULT_INVALID)
            {
                // it succeeded, but is it the right length?
                utf16 = QString::fromUtf8(it->c_str());
                u = tld_encode(utf16, level);
                if(strlen(info.f_tld) != static_cast<size_t>(u.size() + 1))
                {
                    fprintf(stderr, "error: tld(\"%s\", &info) length mismatch (\"%s\", %d/%d).\n",
                            uri.data(), info.f_tld,
                            static_cast<int>(strlen(info.f_tld)),
                            static_cast<int>((u.size() + 1)));
// s3-website.ap-northeast-2.amazonaws.com
QString s(QString::fromUtf8(it->c_str()));
fprintf(stderr, "%d> %s [%s] {%s} -> %d ", r, it->c_str(), u.toUtf8().data(), info.f_tld, s.length());
for(int i(0); i < s.length(); ++i) {
fprintf(stderr, "&#x%04X;", s.at(i).unicode());
}
fprintf(stderr, "\n");
                    ++err_count;
                }
            }
            else
            {
                //fprintf(stderr, "error: tld(\"%s\", &info) failed.\n", it->c_str());
QString s(QString::fromUtf8(it->c_str()));
printf("error: tld(\"%s\", &info) failed with %d [%s] -> %d ", it->c_str(), r, u.toUtf8().data(), s.length());
for(int i(0); i < s.length(); ++i) {
printf("&#x%04X;", s.at(i).unicode());
}
printf("\n");
                ++err_count;
            }
        }
    }
}




int main(int argc, char *argv[])
{
    printf("testing tld names version %s\n", tld_version());

    if(argc > 1)
    {
        if(strcmp(argv[1], "-v") == 0)
        {
            verbose = 1;
        }
    }

    /* call all the tests, one by one
     * failures are "recorded" in the err_count global variable
     * and the process stops with an error message and exit(1)
     * if err_count is not zero.
     */
    test_load();

    if(err_count == 0)
    {
        test_tlds();
    }

    if(err_count || verbose)
    {
        fprintf(stderr, "%d error%s occured.\n",
                    err_count, err_count != 1 ? "s" : "");
    }
    exit(err_count ? 1 : 0);
}

/* vim: ts=4 sw=4 et
 */
