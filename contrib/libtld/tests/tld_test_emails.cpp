/* TLD library -- test the TLD interface for emails
 * Copyright (C) 2013-2017  Made to Order Software Corp.
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
 * \brief Test the tld_email_list class.
 *
 * This file implements various tests to verify that the
 * tld_email_list functions as expected.
 */

#include "libtld/tld.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

/// The number of errors encountered before exiting.
int err_count = 0;

/// Whether to be verbose, turned off by default.
int verbose = 0;


/** \brief Print an error.
 *
 * This function prints the specified \p msg in stderr and increases
 * the error counter by one.
 *
 * \param[in] msg  The message to be printed.
 */
void error(const std::string& msg)
{
    fprintf(stderr, "%s\n", msg.c_str());
    ++err_count;
}


/// Macro to check that exceptions are raised without having to write the try/catch each time.
#define EXPECTED_THROW(s, e) \
    try \
    { \
        static_cast<void>(s); \
        error("error: bad." #s "() of \"\" did not throw an error."); \
    } \
    catch(const e&) \
    { \
    }


/** \brief Define a valid email string.
 *
 * This structure is used to define a valid email string. The string may
 * include any number of emails as defined by the \p f_count field. Note
 * that the count is increased by 1 for each group definition in the list
 * defined in the \p f_input_email string.
 *
 * This structure is used to validate many different types of email
 * addresses to make sure that our parser works properly.
 */
struct valid_email
{
    /// The valid emails to be parsed.
    const char *        f_input_email;
    /// The number of emails returned on f_input_email was parsed, plus one per group.
    int                 f_count;
};

//const char *        f_group;
//const char *        f_original_email;
//const char *        f_fullname;
//const char *        f_username;
//const char *        f_domain;
//const char *        f_email_only;
//const char *        f_canonicalized_email;

/// List of results to verify all the fields of the parser output. There is one entry per group and email.
const tld_email list_of_results[] =
{
    { "", "alexis@m2osw.com",
      "", "alexis", "m2osw.com", "alexis@m2osw.com", "alexis@m2osw.com" },
    { "", "a@m2osw.com",
      "", "a", "m2osw.com", "a@m2osw.com", "a@m2osw.com" },
    { "", "b@c.com",
      "", "b", "c.com", "b@c.com", "b@c.com" },
#if 1
    { "", "alexis@m2osw.com",
      "", "alexis", "m2osw.com", "alexis@m2osw.com", "alexis@m2osw.com" },
    { "", "\"Wilke, Alexis\" <alexis@m2osw.com>",
      "Wilke, Alexis", "alexis", "m2osw.com", "alexis@m2osw.com", "\"Wilke, Alexis\" <alexis@m2osw.com>" },
    { "", "(* Pascal Comments *) \t alexis@m2osw.com\n (Just (kidding) he! he!)",
      "", "alexis", "m2osw.com", "alexis@m2osw.com", "alexis@m2osw.com" },
    { "", "(Start-Comment)alexis@ \t [ \t m2osw.com \t ] \n (More (comment) here)",
      "", "alexis", "m2osw.com", "alexis@m2osw.com", "alexis@m2osw.com" },
    { "", "(Test with dots in user name) al.ex.is@ \t [ \t m2osw.com \t ] \n (More (comments) there)",
      "", "al.ex.is", "m2osw.com", "al.ex.is@m2osw.com", "al.ex.is@m2osw.com" },
    { "", "< (Test with dots in user name) al.ex.is@ \t [ \t m2osw.com \t ] \n (More (comments) there) >",
      "", "al.ex.is", "m2osw.com", "al.ex.is@m2osw.com", "al.ex.is@m2osw.com" },
    { "", "(With full name) Alexis Wilke < (Test with dots in user name) al.ex.is@ \t [ \t m2osw.com \t ] \n (More (comments) there) >",
      "Alexis Wilke", "al.ex.is", "m2osw.com", "al.ex.is@m2osw.com", "Alexis Wilke <al.ex.is@m2osw.com>" },
    { "This Group", "",
      "", "", "", "", "" },
    { "This Group", "(With full name) Alexis Wilke < \n alexis \t @ \t [ \t m2osw.com \t ] \n (Less) >",
      "Alexis Wilke", "alexis", "m2osw.com", "alexis@m2osw.com", "Alexis Wilke <alexis@m2osw.com>" },
    { "People", "",
      "", "", "", "", "" },
    { "People", "Alexis Wilke <alexis@m2osw.com>",
      "Alexis Wilke", "alexis", "m2osw.com", "alexis@m2osw.com", "Alexis Wilke <alexis@m2osw.com>" },
    { "People", "John Smith <john@m2osw.com>",
      "John Smith", "john", "m2osw.com", "john@m2osw.com", "John Smith <john@m2osw.com>" },
    { "Lists", "",
      "", "", "", "", "" },
    { "Lists", "Contact <contact@m2osw.com>",
      "Contact", "contact", "m2osw.com", "contact@m2osw.com", "Contact <contact@m2osw.com>" },
    { "Lists", "Resume <resume@m2osw.com>",
      "Resume", "resume", "m2osw.com", "resume@m2osw.com", "Resume <resume@m2osw.com>" },
    { "", "normal@m2osw.com",
      "", "normal", "m2osw.com", "normal@m2osw.com", "normal@m2osw.com" },
    { "No-Reply", "",
      "", "", "", "", "" },
    { "No-Reply", "no-reply@m2osw.com",
      "", "no-reply", "m2osw.com", "no-reply@m2osw.com", "no-reply@m2osw.com" },
    { "", "\"Complex <name> for !a! \\\"USER\\\"\" <user@example.co.uk>",
      "Complex <name> for !a! \"USER\"", "user", "example.co.uk", "user@example.co.uk", "\"Complex <name> for !a! \\\"USER\\\"\" <user@example.co.uk>" },
    { "", "(Comment \n New-Line) alexis@m2osw.com",
      "", "alexis", "m2osw.com", "alexis@m2osw.com", "alexis@m2osw.com" },
    { "", "(Comment (Sub-Comment (Sub-Sub-Comment (Sub-Sub-Sub-Comment \\) This is still the Sub-Sub-Sub-Comment!!!)))) alexis@m2osw.com",
      "", "alexis", "m2osw.com", "alexis@m2osw.com", "alexis@m2osw.com" },
    { "Group with  some sub-comments", "",
      "", "", "", "", "" },
    { "Group with  some sub-comments", "alexis@m2osw.com",
      "", "alexis", "m2osw.com", "alexis@m2osw.com", "alexis@m2osw.com" },
    // TBD: since the colons get canonicalized to %3A we do not need the '[' and ']' in the canonicalized version
    { "", "\"Wilke, Alexis\" <\"alexis,wilke\"@[:special:.m2osw.com]>",
      "Wilke, Alexis", "alexis,wilke", ":special:.m2osw.com", "\"alexis,wilke\"@[:special:.m2osw.com]", "\"Wilke, Alexis\" <\"alexis,wilke\"@%3Aspecial%3A.m2osw.com>" },
    { "", "alexis@m2osw.com (Simple Comment)",
      "", "alexis", "m2osw.com", "alexis@m2osw.com", "alexis@m2osw.com" },
#endif

    { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

/// The list of valid emails used to check the parser out.
const valid_email list_of_valid_emails[] =
{
    { "alexis@m2osw.com", 1 },
    { "a@m2osw.com", 1 },
    { "b@c.com", 1 },
#if 1
    { " \t alexis@m2osw.com\n \t", 1 },
    { "\"Wilke, Alexis\" <alexis@m2osw.com>", 1 },
    { " (* Pascal Comments *) \t alexis@m2osw.com\n (Just (kidding) he! he!) \t", 1 },
    { "(Start-Comment)alexis@ \t [ \t m2osw.com \t ] \n (More (comment) here) \r\n\t", 1 },
    { "(Test with dots in user name) al.ex.is@ \t [ \t m2osw.com \t ] \n (More (comments) there) \r\n\t", 1 },
    { "< (Test with dots in user name) al.ex.is@ \t [ \t m2osw.com \t ] \n (More (comments) there) > \r\n\t", 1 },
    { "(With full name) Alexis Wilke < (Test with dots in user name) al.ex.is@ \t [ \t m2osw.com \t ] \n (More (comments) there) > \r\n\t", 1 },
    { "  (Now a group:) This Group: (With full name) Alexis Wilke < \n alexis \t @ \t [ \t m2osw.com \t ] \n (Less) >; \r\n\t", 2 },
    { "People: Alexis Wilke <alexis@m2osw.com>, John Smith <john@m2osw.com>; Lists: Contact <contact@m2osw.com>, Resume <resume@m2osw.com>; normal@m2osw.com, No-Reply: no-reply@m2osw.com;", 9 },
    { "\"Complex <name> for !a! \\\"USER\\\"\" <user@example.co.uk>", 1 },
    { "(Comment \n New-Line) alexis@m2osw.com", 1 },
    { "(Comment (Sub-Comment (Sub-Sub-Comment (Sub-Sub-Sub-Comment \\) This is still the Sub-Sub-Sub-Comment!!!)))) alexis@m2osw.com", 1 },
    { "Group with (Comment (Sub-Comment (Sub-Sub-Comment (Sub-Sub-Sub-Comment \\) This is still the Sub-Sub-Sub-Comment!!!)))) some sub-comments \t : alexis@m2osw.com;", 2 },
    { "\"Wilke, Alexis\" <\"alexis,wilke\"@[:special:.m2osw.com]>", 1 },
    { "alexis@m2osw.com (Simple Comment)", 1 },
#endif

    // end of list
    { NULL, 0 }
};


/** \brief Transform an email string in a C-like string.
 *
 * This function transforms the characters in \p e into a set of C-like
 * escape characters so it can safely be printed in the console.
 *
 * For example, the character 0x09 is transformed to the character \\t.
 *
 * \param[in] e  The email to be transformed.
 *
 * \return The transformed email.
 */
std::string email_to_vstring(const std::string& e)
{
    std::string result;
    char buf[3];

    for(const char *s(e.c_str()); *s != '\0'; ++s)
    {
        if(static_cast<unsigned char>(*s) < ' ')
        {
            switch(*s)
            {
            case '\a': result += "\\a"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            case '\v': result += "\\v"; break;
            default:
                buf[0] = '^';
                buf[1] = *s + '@';
                buf[2] = '\0';
                result += buf;
                break;

           }
        }
        else if(*s == 0x7F)
        {
            result += "<DEL>";
        }
        else if(static_cast<unsigned char>(*s) > 0x80)
        {
            static const char *hc = "0123456789ABCDEF";
            result += "\\x";
            buf[0] = hc[*s >> 4];
            buf[1] = hc[*s & 15];
            buf[2] = '\0';
            result += buf;
        }
        else
        {
            result += *s;
        }
    }

    return result;
}


void test_valid_emails()
{
    const tld_email *results(list_of_results);
    for(const valid_email *v(list_of_valid_emails); v->f_input_email != NULL; ++v)
    {
        if(verbose)
        {
            printf("*** testing email \"%s\", start with C++ test\n", email_to_vstring(v->f_input_email).c_str());
            fflush(stdout);
        }

        const tld_email * const cresults(results);

        // C++ test
        {
            tld_email_list list;
            tld_result r(list.parse(v->f_input_email, 0));
            int max(v->f_count);
            if(r != TLD_RESULT_SUCCESS)
            {
                fprintf(stderr, "return value is %d instead of %d with [%s]\n", r, TLD_RESULT_SUCCESS, v->f_input_email);
                error("error: unexpected return value.");
                results += max;
            }
            else if(list.count() != max)
            {
                fprintf(stderr, "parse() returned %d as count, expected %d\n", list.count(), max);
                error("error: unexpected count");
                results += max;
            }
            else
            {
                // test the C++ function first
                {
                    tld_email_list::tld_email_t e;
                    for(int i(0); i < max; ++i, ++results)
                    {
                        if(results->f_group == NULL)
                        {
                            error("error: end of results array reached before completion of the test.\n");
                            return;
                        }

                        if(!list.next(e))
                        {
                            error("error: next() returned false too soon.");
                        }
                        if(e.f_group != results->f_group)
                        {
                            error("error: next() returned the wrong group. Got \"" + e.f_group + "\" instead of \"" + results->f_group + "\".");
                        }
                        if(e.f_original_email != results->f_original_email)
                        {
                            error("error: next() returned the wrong original email. Got \"" + e.f_original_email + "\" instead of \"" + results->f_original_email + "\".");
                        }
                        if(e.f_fullname != results->f_fullname)
                        {
                            error("error: next() returned the wrong fullname. Got \"" + e.f_fullname + "\" instead of \"" + results->f_fullname + "\".");
                        }
                        if(e.f_username != results->f_username)
                        {
                            error("error: next() returned the wrong username. Got \"" + e.f_username + "\" instead of \"" + results->f_username + "\".");
                        }
                        if(e.f_domain != results->f_domain)
                        {
                            error("error: next() returned the wrong username. Got \"" + e.f_domain + "\" instead of \"" + results->f_domain + "\".");
                        }
                        if(e.f_email_only != results->f_email_only)
                        {
                            error("error: next() returned the wrong email only. Got \"" + e.f_email_only + "\" instead of \"" + results->f_email_only + "\".");
                        }
                        if(e.f_canonicalized_email != results->f_canonicalized_email)
                        {
                            error("error: next() returned the wrong canonicalized email. Got \"" + e.f_canonicalized_email + "\" instead of \"" + results->f_canonicalized_email + "\".");
                        }
                    }
                    if(list.next(e))
                    {
                        error("error: next(e) returned the wrong result, it should be false after the whole set of emails were read.");
                    }
                }
                // try the C function which also allows us to test the rewind()
                list.rewind();
                {
                    results = cresults;
                    tld_email e;
                    for(int i(0); i < max; ++i, ++results)
                    {
                        if(!list.next(&e))
                        {
                            error("error: next() returned false too soon.");
                        }
                        if(strcmp(e.f_group, results->f_group) != 0)
                        {
                            error("error: next() returned the wrong group. Got \"" + std::string(e.f_group) + "\" from \"" + results->f_group + "\".");
                        }
                        if(strcmp(e.f_original_email, results->f_original_email) != 0)
                        {
                            error("error: next() returned the wrong original email. Got \"" + std::string(e.f_original_email) + "\" instead of \"" + results->f_original_email + "\".");
                        }
                        if(strcmp(e.f_fullname, results->f_fullname) != 0)
                        {
                            error("error: next() returned the wrong fullname.");
                        }
                        if(strcmp(e.f_username, results->f_username) != 0)
                        {
                            error("error: next() returned the wrong username.");
                        }
                        if(strcmp(e.f_domain, results->f_domain) != 0)
                        {
                            error("error: next() returned the wrong username.");
                        }
                        if(strcmp(e.f_email_only, results->f_email_only) != 0)
                        {
                            error("error: next() returned the wrong email only.");
                        }
                        if(strcmp(e.f_canonicalized_email, results->f_canonicalized_email) != 0)
                        {
                            error("error: next() returned the wrong canonicalized email.");
                        }
                    }
                    if(list.next(&e))
                    {
                        error("error: next(&e) returned the wrong result, it should be false after the whole set of emails were read.");
                    }
                }
            }
        }

        if(verbose)
        {
            printf("*** C test now\n");
            fflush(stdout);
        }
        // C test
        {
            tld_email_list *list;
            list = tld_email_alloc();
            tld_result r = tld_email_parse(list, v->f_input_email, 0);
            const int max(v->f_count);
            if(r != TLD_RESULT_SUCCESS)
            {
                fprintf(stderr, "return value is %d instead of %d for \"%s\"\n", r, TLD_RESULT_SUCCESS, v->f_input_email);
                error("error: unexpected return value.");
            }
            else if(tld_email_count(list) != max)
            {
                fprintf(stderr, "parse() returned %d as count, expected %d\n", tld_email_count(list), max);
                error("error: unexpected count");
            }
            else
            {
                // test the C++ function first
                for(int repeat(0); repeat < 2; ++repeat)
                {
                    results = cresults;
                    struct tld_email e;
                    for(int i(0); i < max; ++i, ++results)
                    {
                        if(results->f_group == NULL)
                        {
                            error("error: end of results array reached before completion of the test.\n");
                            return;
                        }

                        if(tld_email_next(list, &e) != 1)
                        {
                            error("error: next() returned false too soon.");
                        }
                        if(strcmp(e.f_group, results->f_group) != 0)
                        {
                            error("error: next() returned the wrong group. Got \"" + std::string(e.f_group) + "\" from \"" + results->f_group + "\".");
                        }
                        if(strcmp(e.f_original_email, results->f_original_email) != 0)
                        {
                            error("error: next() returned the wrong original email. Got \"" + std::string(e.f_original_email) + "\" instead of \"" + results->f_original_email + "\".");
                        }
                        if(strcmp(e.f_fullname, results->f_fullname) != 0)
                        {
                            error("error: next() returned the wrong fullname.");
                        }
                        if(strcmp(e.f_username, results->f_username) != 0)
                        {
                            error("error: next() returned the wrong username.");
                        }
                        if(strcmp(e.f_domain, results->f_domain) != 0)
                        {
                            error("error: next() returned the wrong username.");
                        }
                        if(strcmp(e.f_email_only, results->f_email_only) != 0)
                        {
                            error("error: next() returned the wrong email only.");
                        }
                        if(strcmp(e.f_canonicalized_email, results->f_canonicalized_email) != 0)
                        {
                            error("error: next() returned the wrong canonicalized email.");
                        }
                    }
                    if(tld_email_next(list, &e) != 0)
                    {
                        error("error: next(&e) returned the wrong result, it should be false after the whole set of emails were read.");
                    }
                    // try again
                    tld_email_rewind(list);
                }
            }
            tld_email_free(list);
        }
    }

    {
        // all valid atom characters
        const char valid_chars[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "!#$%&'*+-/=?^_`{|}~" // here there is a NUL
        ;
        for(size_t i(0); i < sizeof(valid_chars) / sizeof(valid_chars[0]) - 1; ++i)
        {
            tld_email_list list;
            std::string e("abc");
            e += valid_chars[i];
            e += "def@m2osw.com";
            if(verbose)
            {
                printf("*** testing all atom characters with email \"%s\"\n", email_to_vstring(e).c_str());
                fflush(stdout);
            }
            tld_result r(list.parse(e, 0));
            if(r != TLD_RESULT_SUCCESS)
            {
                error("error: unexpected return value.");
            }
        }
    }

    {
        // all valid quoted characters: " " to "\x7E" except the " and \ characters
        if(sizeof(int) < 4)
        {
            error("error: the ctrl variable needs to be at least 32 bits");
            return;
        }
        const int ctrl(1 << '\t');
        for(size_t i(1); i <= 126; ++i)
        {
            switch(i)
            {
            case '"':
            case '\\':
            case 0x7F:  // not included in the loop anyway
                break;

            default:
                if(i >= ' ' || (ctrl & (1 << i)) != 0)
                {
                    tld_email_list list;
                    std::string e("\"abc");
                    e += static_cast<char>(i);
                    e += "def\"@m2osw.com";
                    if(verbose)
                    {
                        printf("*** testing all atom characters with email \"%s\"\n", email_to_vstring(e).c_str());
                        fflush(stdout);
                    }
                    tld_result r(list.parse(e, 0));
                    if(r != TLD_RESULT_SUCCESS)
                    {
                        error("error: unexpected return value.");
                    }
                }
                break;

            }
        }
    }

    {
        // all valid quoted pair: '\t' and " " to "\x7E"
        for(size_t i(31); i <= 126; ++i)
        {
            tld_email_list list;
            std::string e("\"abc\\");
            if(i == 31)
            {
                e += static_cast<char>('\t');
            }
            else
            {
                e += static_cast<char>(i);
            }
            e += "def\"@m2osw.com";
            if(verbose)
            {
                printf("*** testing all atom characters with email \"%s\"\n", email_to_vstring(e).c_str());
                fflush(stdout);
            }
            tld_result r(list.parse(e, 0));
            if(r != TLD_RESULT_SUCCESS)
            {
                error("error: unexpected return value.");
            }
        }
    }

    {
        // all valid comment characters: " " to "\x7E" except the " and \ characters
        if(sizeof(int) < 4)
        {
            error("error: the ctrl variable needs to be at least 32 bits");
            return;
        }
        const int ctrl((1 << '\t') | (1 << '\r') | (1 << '\n'));
        for(size_t i(1); i <= 126; ++i)
        {
            // we skip all the special characters in a comment since
            // those are already tested somewhere else
            switch(i)
            {
            case '(': // avoid a sub-comment
            case ')': // avoid closing the comment mid-way
            case '\\':  // tested somewhere else
            case 0x7F:  // not included in the loop anyway
                break;

            default:
                if(i >= ' ' || (ctrl & (1 << i)) != 0)
                {
                    tld_email_list list;
                    std::string e("(Comment \"");
                    e += static_cast<char>(i);
                    e += "\" char.) alexis@m2osw.com";
                    if(verbose)
                    {
                        printf("*** testing all atom characters with email \"%s\"\n", email_to_vstring(e).c_str());
                        fflush(stdout);
                    }
                    tld_result r(list.parse(e, 0));
                    if(r != TLD_RESULT_SUCCESS)
                    {
                        error("error: unexpected return value.");
                    }
                }
                break;

            }
        }
    }

    {
        // all valid domain characters: "!" to "\x7E" except the [, ], and \ characters
        for(size_t i('!'); i <= 126; ++i)
        {
            // a dot is valid but we cannot test it between two other dots
            if(i == '[' || i == ']' || i == '\\' || i == '.')
            {
                continue;
            }
            tld_email_list list;
            std::string e("alexis@[ m2osw.");
            e += static_cast<char>(i);
            if(i == '%')
            {
                e += "25";
            }
            e += ".com\t]";
            if(verbose)
            {
                printf("*** testing all atom characters with email \"%s\"\n", email_to_vstring(e).c_str());
                fflush(stdout);
            }
            tld_result r(list.parse(e, 0));
            if(r != TLD_RESULT_SUCCESS)
            {
                error("error: unexpected return value while testing a domain with special character \"" + e + "\"");
            }
        }
    }

    {
        if(tld_email_list::quote_string("Test quoting a simple comment", '(') != "(Test quoting a simple comment)")
        {
            error("error: unexpected return value when testing a simple comment quotation");
        }
        if(tld_email_list::quote_string("Test (quoting) a complex )comment(", '(') != "(Test \\(quoting\\) a complex \\)comment\\()")
        {
            error("error: unexpected return value when testing a complex comment quotation");
        }
    }
}




/** \brief Define an invalid email.
 *
 * This structure is used to list invalid emails in order to test that such
 * emails are not accepted by the parser. The structure includes the expected
 * result as well as a string pointer to the invalid email.
 */
struct invalid_email
{
    /// The expected reslut, if the call does not return this exact value the test fails
    tld_result          f_result;
    /// The pointer to the invalid email to be tested
    const char *        f_input_email;
};

const invalid_email list_of_invalid_emails[] =
{
    { TLD_RESULT_NULL, "alexism2osw.com (missing @)" },
    { TLD_RESULT_INVALID, " \v alexis@m2osw.com\n \t (bad control)" },
    { TLD_RESULT_INVALID, " (* Pascal Comments *) \t alexis@m2osw.com\n (missing closing parenthesis\\)" },
    { TLD_RESULT_INVALID, "(Start-Comment)alexis@ \t [ \t m2osw.com \t ] \n (extra after domain done) \"more\tdata\" \r\n\t" },
    { TLD_RESULT_INVALID, "(Test with dots in user name) al.ex.is@ \t(missing closing bracket ]) [ \t m2osw.com \t " },
    { TLD_RESULT_NULL, "< (Test with dots in user name) al.ex.is@ \t [ \t m2osw.com \t ] \n (Missing >) \r\n\t" },
    { TLD_RESULT_INVALID, "(Full name with period) Alexis.Wilke < (Test with dots in user name) al.ex.is@ \t [ \t m2osw.com \t ] \n (More (comments) there) > \r\n\t" },
    { TLD_RESULT_INVALID, "  (Now a group:) This Group: (With full name) Alexis Wilke < \n alexis \t @ \t [ \t m2osw.com \t ] \n (missing ;) > \r\n\t" },
    { TLD_RESULT_INVALID, "Good Group: alexis@m2osw.com, bad-group: test@example.com;" },
    { TLD_RESULT_INVALID, "(No Group Name): alexis@m2osw.com;" },
    { TLD_RESULT_INVALID, " (No Group Name) : alexis@m2osw.com;" },
    { TLD_RESULT_INVALID, ": alexis@m2osw.com;" },
    { TLD_RESULT_INVALID, "(Group with CTRL) Group \v Unexpected: alexis@m2osw.com;" },
    { TLD_RESULT_INVALID, "\"alexis@m2osw.com;" },
    { TLD_RESULT_INVALID, "\"alexis@m2osw.com;\v\"" },
    { TLD_RESULT_INVALID, "\"Alexis Wilke\\" },  // \ followed by NUL
    { TLD_RESULT_INVALID, "(Comment with \\\\ followed by NUL: \\" },
    { TLD_RESULT_INVALID, "(Test Errors Once Done) \"Wilke, Alexis\" <alexis@m2osw.com> \"Bad\"" },
    { TLD_RESULT_INVALID, "(Comment with CTRL \b) \"Wilke, Alexis\" <alexis@m2osw.com>" },
    { TLD_RESULT_INVALID, "[m2osw.com]" },
    { TLD_RESULT_INVALID, "good@[bad-slash\\.com]" },
    { TLD_RESULT_INVALID, "good@[bad[reopen.com]" },
    { TLD_RESULT_INVALID, "(Test Errors Once Done) \"Wilke, Alexis\" <alexis@m2osw.com> [Bad]" },
    { TLD_RESULT_INVALID, "(Test Errors Once Done) alexis@start[Bad]" },
    { TLD_RESULT_INVALID, "(Test Errors Once Done) alexis@[first][Bad]" },
    { TLD_RESULT_INVALID, "(Test Errors Once Done) alexis@[control:\v]" },
    { TLD_RESULT_NULL, "(Test Errors Once Done) alexis@[ spaces BAD]" },
    { TLD_RESULT_INVALID, "(Spurious Angle) alexis>@m2osw.com" },
    { TLD_RESULT_INVALID, "(Spurious Angle) alexis@m2osw.com>" },
    { TLD_RESULT_INVALID, "(Double Angle) <alexis@m2osw.com>>" },
    { TLD_RESULT_NULL, "(Missing domain) <alexis@>" },
    { TLD_RESULT_NULL, "(Missing domain) alexis@" },
    { TLD_RESULT_INVALID, "(2 domains) <alexis@[m2osw.com]bad>" },
    { TLD_RESULT_INVALID, "(Double @) <alexis@m2osw.com> @" },
    { TLD_RESULT_INVALID, "(Double @) alexis@m2osw.com@" },
    { TLD_RESULT_INVALID, "(Extra Chars) <alexis@m2osw.com> bad" },
    { TLD_RESULT_NULL, "(Empty username within brackets) <@m2osw.com>" },
    { TLD_RESULT_NULL, "(Empty User Name) @m2osw.com" },
    { TLD_RESULT_INVALID, "(Cannot start with a dot) .alexis@m2osw.com" },
    { TLD_RESULT_INVALID, "(Cannot start with a dot) <.alexis@m2osw.com>" },
    { TLD_RESULT_INVALID, "(Cannot end with a dot) alexis.@m2osw.com" },
    { TLD_RESULT_INVALID, "(Cannot end with a dot) <alexis.@m2osw.com>" },
    { TLD_RESULT_INVALID, "(Cannot include double dots) ale..xis@m2osw.com" },
    //{ TLD_RESULT_INVALID, "(End domain with dot not considered valid!) alexis@m2osw.com." }, viewed as valid! (that bad?)
    { TLD_RESULT_INVALID, "(End domain with dot not considered valid!) <alexis@m2osw.com.>" },
    { TLD_RESULT_NULL, "(Bad Emails) alexis,m2osw.com" },
    { TLD_RESULT_INVALID, "(Bad Char) alexis@m2osw\001com" },
    { TLD_RESULT_NOT_FOUND, "(Bad Extension) alexis@m2osw.comm" },
    { TLD_RESULT_INVALID, "(Bad Extension) alexis@m2osw.ar" },
    { TLD_RESULT_INVALID, "(Bad Extension) alexis@m2osw.nom.ar" },
    { TLD_RESULT_NO_TLD, "(Bad Extension) alexis@m2osw" },
    { TLD_RESULT_BAD_URI, "(Bad Extension) alexis@[m2osw..com]" },

    // end of list
    { TLD_RESULT_SUCCESS, NULL }
};

void test_invalid_emails()
{
    for(const invalid_email *v(list_of_invalid_emails); v->f_input_email != NULL; ++v)
    {
        if(verbose)
        {
            printf("+++ testing email \"%s\"\n", email_to_vstring(v->f_input_email).c_str());
        }

        // C++ test
        {
            tld_email_list list;
            tld_result r(list.parse(v->f_input_email, 0));
            if(r != v->f_result)
            {
                std::stringstream ss;
                ss << "error: unexpected return value. Got " << static_cast<int>(r) << ", expected " << static_cast<int>(v->f_result) << " for \"" << v->f_input_email << "\" (C++)";
                error(ss.str());
            }
        }

        // C test
        {
            tld_email_list *list;
            list = tld_email_alloc();
            tld_result r = tld_email_parse(list, v->f_input_email, 0);
            if(r != v->f_result)
            {
                std::stringstream ss;
                ss << "error: unexpected return value. Got " << static_cast<int>(r) << ", expected " << static_cast<int>(v->f_result) << " for \"" << v->f_input_email << "\" (C)";
                error(ss.str());
            }
            tld_email_free(list);
            list = NULL;
        }
    }
}


void contract_furfilled(tld_email_list::tld_email_t& e)
{
    if(!e.f_group.empty()
    || !e.f_original_email.empty()
    || !e.f_fullname.empty()
    || !e.f_username.empty()
    || !e.f_domain.empty()
    || !e.f_email_only.empty()
    || !e.f_canonicalized_email.empty())
    {
        error("error: one of the structure parameters was modified on error!");
    }
}


void test_direct_email()
{
    tld_email_list::tld_email_t email;

    ////////////// EMAILS
    // missing closing \"
    EXPECTED_THROW(email.parse("\"blah alexis@m2osw.com"), std::logic_error);
    contract_furfilled(email);

    // missing closing )
    EXPECTED_THROW(email.parse("(comment alexis@m2osw.com"), std::logic_error);
    contract_furfilled(email);

    // use of \ at the end of the comment
    EXPECTED_THROW(email.parse("(comment\\"), std::logic_error);
    contract_furfilled(email);

    // missing closing ]
    EXPECTED_THROW(email.parse("alexis@[m2osw.com"), std::logic_error);
    contract_furfilled(email);

    ////////////// GROUP
    // missing closing )
    EXPECTED_THROW(email.parse_group("Group (comment"), std::logic_error);
    contract_furfilled(email);

    // use of \ at the end of the comment
    EXPECTED_THROW(email.parse_group("Group (comment \\"), std::logic_error);
    contract_furfilled(email);
}



/** \brief Structure used to define a set of fields to test.
 *
 * This structure is used in this test to define a list of fields
 * to test against the library.
 */
struct email_field_types
{
    const char *            f_field;
    tld_email_field_type    f_type;
};

/** \var email_field_types::f_field
 * \brief The name of the field to be tested.
 */
/** \var email_field_types::f_type
 * \brief The type we expect the library to return for that field.
 */

const email_field_types list_of_email_field_types[] =
{
    // make sure case does not have side effects
    { "to", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "To", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "tO", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "TO", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },

    // check all fields that are expected to include emails
    { "from", TLD_EMAIL_FIELD_TYPE_MAILBOX_LIST },
    { "resent-from", TLD_EMAIL_FIELD_TYPE_MAILBOX_LIST },
    { "sender", TLD_EMAIL_FIELD_TYPE_MAILBOX },
    { "resent-sender", TLD_EMAIL_FIELD_TYPE_MAILBOX },
    { "to", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "cc", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "reply-to", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "resent-to", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "resent-cc", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "bcc", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST_OPT },
    { "resent-bcc", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST_OPT },

    // check all fields with a colon
    { "from: someone", TLD_EMAIL_FIELD_TYPE_MAILBOX_LIST },
    { "resent-from: someone", TLD_EMAIL_FIELD_TYPE_MAILBOX_LIST },
    { "sender: someone", TLD_EMAIL_FIELD_TYPE_MAILBOX },
    { "resent-sender: someone", TLD_EMAIL_FIELD_TYPE_MAILBOX },
    { "to: someone", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "cc: someone", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "reply-to: someone", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "resent-to: someone", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "resent-cc: someone", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST },
    { "bcc: someone", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST_OPT },
    { "resent-bcc: someone", TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST_OPT },

    // check other fields
    { "message-id", TLD_EMAIL_FIELD_TYPE_UNKNOWN },
    { "date", TLD_EMAIL_FIELD_TYPE_UNKNOWN },
    { "subject", TLD_EMAIL_FIELD_TYPE_UNKNOWN },
    { "x-extension", TLD_EMAIL_FIELD_TYPE_UNKNOWN },

    // check other fields with a colon
    { "message-id: something", TLD_EMAIL_FIELD_TYPE_UNKNOWN },
    { "date: something", TLD_EMAIL_FIELD_TYPE_UNKNOWN },
    { "subject: something", TLD_EMAIL_FIELD_TYPE_UNKNOWN },
    { "x-extension: something", TLD_EMAIL_FIELD_TYPE_UNKNOWN },

    // check for invalid field names
    { "s\xfc\x62ject", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "subj\xe9\x63t", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "-bad-dash", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "0bad-digit", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "1bad-digit", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "2bad-digit", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "3bad-digit", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "4bad-digit", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "5bad-digit", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "6bad-digit", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "7bad-digit", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "8bad-digit", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "9bad-digit", TLD_EMAIL_FIELD_TYPE_INVALID },
    { "" /*empty*/, TLD_EMAIL_FIELD_TYPE_INVALID },
};

void test_email_field_types()
{
    for(size_t i(0); i < sizeof(list_of_email_field_types) / sizeof(list_of_email_field_types[0]); ++i)
    {
        tld_email_field_type type(tld_email_list::email_field_type(list_of_email_field_types[i].f_field));
        if(type != list_of_email_field_types[i].f_type)
        {
            std::stringstream ss;
            ss << "error: email type mismatch for \"" << list_of_email_field_types[i].f_field
                << "\", expected " << static_cast<int>(list_of_email_field_types[i].f_type)
                << ", got " << static_cast<int>(type) << " instead.";
            error(ss.str());
        }
    }
}



int main(int argc, char *argv[])
{
    printf("testing tld emails version %s\n", tld_version());

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
        test_valid_emails();
        test_invalid_emails();
        test_direct_email();
        test_email_field_types();
    }
    catch(const invalid_domain&)
    {
        error("error: caught an exception when all emails are expected to be valid.");
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
