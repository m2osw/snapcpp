/* TLD library -- TLD, domain name, and sub-domain extraction
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
 * \brief Implementation of the TLD parser library.
 *
 * This file includes all the functions available in the C library
 * of libtld that pertain to the parsing of URIs and extraction of
 * TLDs.
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
#include <limits.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#define strncasecmp _strnicmp
#endif

/** \mainpage
 *
 * \section introduction The libtld Library
 *
 * The libtld project is a library that gives you the capability to
 * determine the TLD part of any Internet URI or email address.
 *
 * The main function of the library, tld(), takes a URI string and a
 * tld_info structure. From that information it computes the position
 * where the TLD starts in the URI. For email addresses (see the
 * tld_email_list C++ object, or the tld_email.cpp file for the C
 * functions,) it breaks down a full list of emails verifying the
 * syntax as defined in RFC 5822.
 *
 * \section c_programmers For C Programmers
 *
 * The C functions that you are expected to use are listed here:
 *
 * \li tld_version() -- return a string representing the TLD library version
 * \li tld() -- find the position of the TLD of any URI
 * \li tld_domain_to_lowercase() -- force lowercase on the domain name before
 *                                  calling other tld function
 * \li tld_check_uri() -- verify a full URI, with scheme, path, etc.
 * \li tld_clear_info() -- reset a tld_info structure for use with tld()
 * \li tld_email_alloc() -- allocate a tld_email_list object
 * \li tld_email_free() -- free a tld_email_list object
 * \li tld_email_parse() -- parse a list of email addresses
 * \li tld_email_count() -- number of emails found by tld_email_parse()
 * \li tld_email_rewind() -- go back at the start of the list of emails
 * \li tld_email_next() -- read the next email from the list of emails
 *
 * \section cpp_programmers For C++ Programmers
 *
 * For C++ users, please make use of these tld classes:
 *
 * \li tld_object
 * \li tld_email_list
 *
 * In C++, you may also make use of the tld_version() to check the current
 * version of the library.
 *
 * To check whether the version is valid for your tool, you may look at the
 * version handling of the libdebpackages library of the wpkg project. The
 * libtld version is always a Debian compatible version.
 *
 * http://windowspackager.org/documentation/implementation-details/debian-version-api
 *
 * \section php_programmers For PHP Programmers
 *
 * At this point I do not have a very good environment to recompile everything
 * for PHP. The main reason is because the library is being compiled with cmake
 * opposed to the automake toolchain that Zend expects.
 *
 * This being said, the php directory includes all you need to make use of the
 * library under PHP. It works like a charm for me and there should be no reason
 * for you not to be able to do the same with the library.
 *
 * The way I rebuild everything for PHP:
 *
 * \code
 * # from within the libtld directory:
 * mkdir ../BUILD
 * (cd ../BUILD; cmake ../libtld)
 * make -C ../BUILD
 * cd php
 * ./build
 * \endcode
 *
 * The build script will copy the resulting php_libtld.so file where it
 * needs to go using sudo. Your system (Red Hat, Mandrake, etc.) may use
 * su instead. Update the script as required.
 *
 * Note that the libtld will be linked statically inside the php_libtld.so
 * so you do not have to actually install the libtld environment to make
 * everything work as expected.
 *
 * The resulting functions added to PHP via this extension are:
 *
 * \li %check_tld()
 * \li %check_uri()
 * \li %check_email()
 *
 * For information about these functions, check out the php/php_libtld.c
 * file which describes each function, its parameters, and its results
 * in great details.
 *
 * \section not_linux Compiling on Other Platforms
 *
 * We can successfully compile the library under MS-Windows with cygwin
 * and the Microsoft IDE. To do so, we use the CMakeLists.txt file found
 * under the dev directory. Overwrite the CMakeLists.txt file in the
 * main directory before configuring and you'll get a library without
 * having to first compile Qt4.
 *
 * \code
 * cp dev/libtld-only-CMakeLists.txt CMakeListst.txt
 * \endcode
 *
 * At this point this configuration only compiles the library. It gives
 * you a shared (.DLL) and a static (.lib) version. With the IDE you may
 * create a debug and a release version.
 *
 * Later we'll look into having a single CMakeLists.txt so you do not
 * have to make this copy.
 *
 * \section example Example
 *
 * We offer a file named example.c that shows you how to use the
 * library in C. It is very simple, one main() function so it is
 * very easy to get started with libtld.
 *
 * For a C++ example, check out the src/validate_tld.cpp tool which was
 * created as a command line tool coming with the libtld library.
 *
 * \include example.c
 *
 * \section dev Programmers & Maintainers
 *
 * If you want to work on the library, there are certainly things to
 * enhance. We could for example offer more offsets in the info
 * string, or functions to clearly define each part of the URI.
 *
 * However, the most important part of this library is the XML file
 * which defines all the TLDs. Maintaining that file is what will
 * help the most. It includes all the TLDs known at this point
 * (as defined in different places such as Wikipedia and each
 * different authority in that area.) The file is easy to read so
 * you can easily find whether your extension is defined and if not
 * you can let us know.
 *
 * \section requirements Library Requirements
 *
 * \li Usage
 *
 * The library doesn't need anything special. It's a few C functions.
 *
 * The library also offers a C++ classes. You do not need a C++ compiler
 * to use the library, but if you do program in C++, you can use the
 * tld_object and tld_email_list instead of the C functions. It makes
 * things a lot easier!
 *
 * Also if you are programming using PHP, the library includes a PHP
 * extension so you can check URIs and emails directly from PHP without
 * trying to create crazy regular expressions (that most often do not work
 * right!)
 *
 * \li Compiling
 *
 * To compile the library, you'll need CMake, a C++ compiler for different
 * parts and the Qt library as we use the QtXml and QtCore (Qt4). The QtXml
 * library is used to parse the XML file (tld_data.xml) which defines all
 * the TLDs, worldwide.
 *
 * To regenerate the documentation we use Doxygen. It is optional, though.
 *
 * \li PHP
 *
 * In order to recompile the PHP extension the Zend environment is required.
 * Under a Debian or Ubuntu system you can install the php5-dev package.
 *
 * \section tests Tests Coming with the Library
 *
 * We have the following tests at this time:
 *
 * \li tld_test.c
 *
 * \par
 * This test checks the tld() function as end users of the
 * library. It checks all the existing TLDs, a few unknown TLDs,
 * and invalid TLDs.
 *
 * \li tld_test_object.cpp
 *
 * \par
 * This test verifies that the tld_object works as expected. It is not
 * exhaustive in regard to the tld library itself, only of the tld_object.
 *
 * \li tld_internal_test.c
 *
 * \par
 * This test includes the tld.c directly so it can check each
 * internal function directly. This test checks the cmp() and
 * search() functions, with full coverage.
 *
 * \li tld_test_domain_lowercase.c
 *
 * \par
 * This test runs 100% coverage of the tld_domain_to_lowercase() function.
 * This includes conversion of %XX encoded characters and UTF-8 to wide
 * characters that can be case folded and saved back as encoded %XX
 * characters. The test verifies that all characters are properly
 * supported and that errors are properly handled.
 *
 * \li tld_test_tld_names.cpp
 *
 * \par
 * The Mozilla foundation offers a file with a complete list of all the
 * domain names defined throughout the world. This test reads that list
 * and checks all the TLDs against the libtld system. Some TLDs may be
 * checked in multiple ways. We support the TLDs that start with an
 * asterisk (*) and those that start with an exclamation mark (!) which
 * means all the TLDs are now being checked out as expected.
 * This test reads the effective_tld_names.dat file which has to be
 * available in your current directory.
 *
 * \par
 * A copy of the Mozilla file is included with each version of the TLD
 * library. It is named tests/effective_tld_names.dat and should be
 * up to date when we produce a new version for download on
 * SourceForge.net.
 *
 * \li tld_test_full_uri.c
 *
 * \par
 * The library includes an advanced function that checks the validity
 * of complete URIs making it very simple to test such in any software.
 * The URI must include a scheme (often called protocol), fully qualified
 * domain (sub-domains, domain, TLD), an absolute path, variables (after
 * the question mark,) and an anchor. The test ensures that all the
 * checks the parser uses are working as expected and allow valid URIs
 * while it forbids any invalid URIs.
 *
 * \li tld_test_emails.cpp
 *
 * \par
 * The libtld supports verifying and breaking up emails in different
 * parts. This is done to make sure users enter valid emails (although
 * it doesn't mean that the email address exists, it at least allows
 * us to know when an email is definitively completely incorrect and
 * should be immediately rejected.) The test ensures that all the
 * different types of invalid emails are properly being caught (i.e.
 * emails with control characters, invalid domain name, missing parts,
 * etc.)
 *
 * \li tld_test_versions.c
 *
 * \par
 * This test checks that the versions in all the files (two
 * CMakeLists.txt and the changelog) are equal. If one of those
 * does not match, then the test fails.
 *
 * \li tld_test_xml.sh
 *
 * \par
 * Shell script to run against the tld_data.xml file to ensure its validity.
 * This is a good idea any time you make changes to the file. It runs with
 * the xmllint tool. If you do not have the tool, it won't work. The tool
 * is part of the libxml2-utils package under Ubuntu.
 */




/** \brief Compare two strings, one of which is limited by length.
 * \internal
 *
 * This internal function was created to handle a simple string
 * (no locale) comparison with one string being limited in length.
 *
 * The comparison does not require locale since all characters are
 * ASCII (a URI with Unicode characters encode them in UTF-8 and
 * changes all those bytes with %XX.)
 *
 * The length applied to the string in \p b. This allows us to make
 * use of the input string all the way down to the cmp() function.
 * In other words, we avoid a copy of the string.
 *
 * The string in \p a is 'nul' (\0) terminated. This means \p a
 * may be longer or shorter than \p b. In other words, the function
 * is capable of returning the correct result with a single call.
 *
 * \param[in] a  The pointer in an f_tld field of the tld_descriptions.
 * \param[in] b  Pointer directly in referencing the user domain string.
 * \param[in] n  The number of characters that can be checked in \p b.
 *
 * \return -1 if a < b, 0 when a == b, and 1 when a > b
 */
static int cmp(const char *a, const char *b, int n)
{
    /* n represents the maximum number of characters to check in b */
    while(n > 0 && *a != '\0')
    {
        if(*a < *b)
        {
            return -1;
        }
        if(*a > *b)
        {
            return 1;
        }
        ++a;
        ++b;
        --n;
    }
    if(*a == '\0')
    {
        if(n > 0)
        {
            /* in this case n > 0 so b is larger */
            return -1;
        }
        return 0;
    }
    /* in this case n == 0 so a is larger */
    return 1;
}


/** \brief Search for the specified domain.
 * \internal
 *
 * This function executes one search for one domain. The
 * search is binary, which means the tld_descriptions are
 * expected to be 100% in order at all levels.
 *
 * The \p i and \p j parameters represent the boundaries
 * of the current level to be checked. Know that for a
 * given TLD, there is a start and end boundary that is
 * used to define \p i and \p j. So except for the top
 * level, the bounds are limited to one TLD, sub-TLD, etc.
 * (for example, .uk has a sub-layer with .co, .ac, etc.
 * and that ground is limited to the second level entries
 * accepted within the .uk TLD.)
 *
 * This search does one search at one level. If sub-levels
 * are available for that TLD, then it is the responsibility
 * of the caller to call the function again to find out whether
 * one of those sub-domain name is in use.
 *
 * When the TLD cannot be found, the function returns -1.
 *
 * \param[in] i  The start point of the search (included.)
 * \param[in] j  The end point of the search (excluded.)
 * \param[in] domain  The domain name to search.
 * \param[in] n  The length of the domain name.
 *
 * \return The offset of the domain found, or -1 when not found.
 */
int search(int i, int j, const char *domain, int n)
{
    int p, r;
    const struct tld_description *tld;

    while(i < j)
    {
        p = (j - i) / 2 + i;
        tld = tld_descriptions + p;
        r = cmp(tld->f_tld, domain, n);
        if(r < 0)
        {
            /* eliminate the first half */
            i = p + 1;
        }
        else if(r > 0)
        {
            /* eliminate the second half */
            j = p;
        }
        else
        {
            /* match */
            return p;
        }
    }

    return -1;
}


/** \brief Clear the info structure.
 *
 * This function initializes the info structure with defaults.
 * The different TLD functions that make use of this structure
 * will generally call this function first to represent a
 * failure case.
 *
 * Note that by default the category and status are set to
 * undefined (TLD_CATEGORY_UNDEFINED and TLD_STATUS_UNDEFINED).
 * Also the country and tld pointer are set to NULL and thus
 * they cannot be used as strings.
 *
 * \param[out] info  The tld_info structure to clear.
 */
void tld_clear_info(struct tld_info *info)
{
    info->f_category = TLD_CATEGORY_UNDEFINED;
    info->f_status = TLD_STATUS_UNDEFINED;
    info->f_country = (const char *) 0;
    info->f_tld = (const char *) 0;
    info->f_offset = -1;
}


/** \brief Get information about the TLD for the specified URI.
 *
 * The tld() function searches for the specified URI in the TLD
 * descriptions. The results are saved in the info parameter for
 * later interpretetation (i.e. extraction of the domain name,
 * sub-domains and the exact TLD.)
 *
 * The function extracts the last \em extension of the URI. For
 * example, in the following:
 *
 * \code
 * example.co.uk
 * \endcode
 *
 * the function first extracts ".uk". With that \em extension, it
 * searches the list of official TLDs. If not found, an error is
 * returned and the info parameter is set to \em unknown.
 *
 * When found, the function checks whether that TLD (".uk" in our
 * previous example) accepts sub-TLDs (second, third, forth and
 * fifth level TLDs.) If so, it extracts the next TLD entry (the
 * ".co" in our previous example) and searches for that second
 * level TLD. If found, it again tries with the third level, etc.
 * until all the possible TLDs were exhausted. At that point, it
 * returns the last TLD it found. In case of ".co.uk", it returns
 * the information of the ".co" TLD, second-level domain name.
 *
 * All the comparisons are done in lowercase. This is because
 * all the data is saved in lowercase and we expect the input
 * of the tld() function to already be in lowercase. If you
 * have a doubt and your input may actually be in uppercase,
 * make sure to call the tld_domain_to_lowercase() function
 * first. That function makes a duplicate of your domain name
 * in lowercase. It understands the %XX characters (since the
 * URI is expected to still be encoded) and properly handles
 * UTF-8 characters in order to define the lowercase characters
 * of the input. Note that the function returns a newly
 * allocated pointer that you are responsible to free once
 * you are done with it.
 *
 * \warning
 * If you call tld() with the pointer return by
 * tld_domain_to_lowercase(), keep in mind that the tld()
 * function saves pointers of the input string directly in
 * the tld_info structure. In other words, you want to free()
 * that string AFTER you are done with the tld_info structure.
 *
 * The \p info structure includes:
 *
 * \li f_category -- the category of TLD, unless set to
 * TLD_CATEGORY_UNDEFINED, it is considered valid
 * \li f_status -- the status of the TLD, unless set to
 * TLD_STATUS_UNDEFINED, it was defined from the tld_data.xml file;
 * however, only those marked as TLD_STATUS_VALID are considered to
 * currently be in use, all the other statuses can be used by your
 * software, one way or another, but it should not be accepted as
 * valid in a URI
 * \li f_country -- if the category is set to TLD_CATEGORY_COUNTRY
 * then this pointer is set to the name of the country
 * \li f_tld -- is set to the full TLD of your domain name; this is
 * a pointer WITHIN your uri string so make sure you keep your URI
 * string valid if you intend to use this f_tld string
 * \li f_offset -- the offset to the first period within the domain
 * name TLD (i.e. in our previous example, it would be the offset to
 * the first period in ".co.uk", so in "example.co.uk" the offset would
 * be 7. Assuming you prepend "www." to have the URI "www.example.co.uk"
 * then the offset would be 11.)
 *
 * \note
 * In our previous example, the ".uk" TLD is properly used: it includes
 * a second level domain name (".co".) The URI "example.uk" should have
 * returned TLD_RESULT_INVALID since .uk by itself was not supposed to be
 * acceptable. This changed a few years ago. The good thing is that it
 * resolves some problems as some companies were given a simple ".uk"
 * TLD and these were exceptions the library does not need to support
 * anymore. There are still some countries, such as ".bd", which do not
 * accept second level names, so "example.bd" does return
 * an \em error (TLD_RESULT_INVALID).
 *
 * Assuming that you always get valid URIs, you should get one of those
 * results:
 *
 * \li TLD_RESULT_SUCCESS -- success! the URI is valid and the TLD was
 * properly determined; use the f_tld or f_offset to extract the TLD
 * domain and sub-domains
 * \li TLD_RESULT_INVALID -- known TLD, but not currently valid; this
 * result is returned when we know that the TLD is not to be accepted
 *
 * Other results are returned when the input string is considered invalid.
 *
 * \note
 * The function only accepts a bare URI, in other words: no protocol, no
 * path, no anchor, no query string, and still URI encoded. Also, it
 * should not start and/or end with a period or you are likely to get
 * an invalid response. (i.e. don't use any of ".example.co.uk.",
 * "example.co.uk.", nor ".example.co.uk")
 *
 * \include example.c
 *
 * \param[in] uri  The URI to be checked.
 * \param[out] info  A pointer to a tld_info structure to save the result.
 *
 * \return One of the TLD_RESULT_... enumeration values.
 */
enum tld_result tld(const char *uri, struct tld_info *info)
{
    const char *end = uri;
    const char **level_ptr;
    int level = 0, start_level, i, r, p;
    enum tld_result result;

    /* set defaults in the info structure */
    tld_clear_info(info);

    if(uri == (const char *) 0 || uri[0] == '\0')
    {
        return TLD_RESULT_NULL;
    }

    level_ptr = malloc(sizeof(const char *) * tld_max_level);

    while(*end != '\0')
    {
        if(*end == '.')
        {
            if(level >= tld_max_level)
            {
                /* At this point the maximum number of levels in the
                 * TLDs is 5
                 */
                for(i = 1; i < tld_max_level; ++i)
                {
                    level_ptr[i - 1] = level_ptr[i];
                }
                level_ptr[tld_max_level - 1] = end;
            }
            else
            {
                level_ptr[level] = end;
                ++level;
            }
            if(level >= 2 && level_ptr[level - 2] + 1 == level_ptr[level - 1])
            {
                /* two periods one after another */
                free(level_ptr);
                return TLD_RESULT_BAD_URI;
            }
        }
        ++end;
    }
    /* if level is not at least 1 then there are no period */
    if(level == 0)
    {
        /* no TLD */
        free(level_ptr);
        return TLD_RESULT_NO_TLD;
    }

    start_level = level;
    --level;
    r = search(tld_start_offset, tld_end_offset,
                level_ptr[level] + 1, (int) (end - level_ptr[level] - 1));
    if(r == -1)
    {
        /* unknown */
        free(level_ptr);
        return TLD_RESULT_NOT_FOUND;
    }

    /* check for the next level if there is one */
    p = r;
    while(level > 0 && tld_descriptions[r].f_start_offset != USHRT_MAX)
    {
        r = search(tld_descriptions[r].f_start_offset,
                tld_descriptions[r].f_end_offset,
                level_ptr[level - 1] + 1,
                (int) (level_ptr[level] - level_ptr[level - 1] - 1));
        if(r == -1)
        {
            /* we are done, return the previous level */
            break;
        }
        p = r;
        --level;
    }

    /* if there are exceptions we may need to search those now if level is 0 */
    if(level == 0)
    {
        r = search(tld_descriptions[p].f_start_offset,
                tld_descriptions[p].f_end_offset,
                uri,
                (int) (level_ptr[0] - uri));
        if(r != -1)
        {
            p = r;
        }
    }

    info->f_status = tld_descriptions[p].f_status;
    result = info->f_status == TLD_STATUS_VALID
                ? TLD_RESULT_SUCCESS
                : TLD_RESULT_INVALID;

    /* did we hit an exception? */
    if(tld_descriptions[p].f_status == TLD_STATUS_EXCEPTION)
    {
        /* return the actual TLD and not the exception */
        p = tld_descriptions[p].f_exception_apply_to;
        level = start_level - tld_descriptions[p].f_exception_level;
        info->f_status = TLD_STATUS_VALID;
        result = TLD_RESULT_SUCCESS;
    }

    /* return a valid result */
    info->f_category = tld_descriptions[p].f_category;
    info->f_country = tld_descriptions[p].f_country;
    info->f_tld = level_ptr[level];
    info->f_offset = (int) (level_ptr[level] - uri);

    free(level_ptr);

    return result;
}


/** \brief Internal function used to transform %XX values.
 *
 * This function transforms an hexadecimal (h) character to (2) a
 * decimal number (d).
 *
 * \param[in] c  The hexadecimal character to transform
 *
 * \return The number the hexadecimal character represents (0 to 15)
 */
static int h2d(int c)
{
    if(c >= 'a')
    {
        return c - 'a' + 10;
    }
    if(c >= 'A')
    {
        return c - 'A' + 10;
    }
    return c - '0';
}


/** \brief Check that a URI is valid.
 *
 * This function very quickly parses a URI to determine whether it
 * is valid.
 *
 * Note that it does not (currently) support local naming conventions
 * which means that a host such as "localhost" will fail the test.
 *
 * The \p protocols variable can be set to a list of protocol names
 * that are considered valid. For example, for HTTP protocol one
 * could use "http,https". To accept any protocol use an asterisk
 * as in: "*". The protocol must be only characters, digits, or
 * underscores ([0-9A-Za-z_]+) and it must be at least one character.
 *
 * The flags can be set to the following values, or them to set multiple
 * flags at the same time:
 *
 * \li VALID_URI_ASCII_ONLY -- refuse characters that are not in the
 * first 127 range (we expect the URI to be UTF-8 encoded and any
 * byte with bit 7 set is considered invalid if this flag is set,
 * including encoded bytes such as %A0)
 * \li VALID_URI_NO_SPACES -- refuse spaces whether they are encoded
 * with + or %20 or verbatim.
 *
 * The return value is generally TLD_RESULT_BAD_URI when an invalid
 * character is found in the URI string. The TLD_RESULT_NULL is
 * returned if the URI is a NULL pointer or an empty string.
 * Other results may be returned by the tld() function. If a result
 * other than TLD_RESULT_SUCCESS is returned then the info structure
 * may or may not be updated.
 *
 * \param[in] uri  The URI which validity is being checked.
 * \param[out] info  The resulting information about the URI domain and TLD.
 * \param[in] protocols  List of comma separated protocols accepted.
 * \param[in] flags  A set of flags to tell the function what is valid/invalid.
 *
 * \return The result of the operation, TLD_RESULT_SUCCESS if the URI is
 * valid.
 *
 * \sa tld()
 */
enum tld_result tld_check_uri(const char *uri, struct tld_info *info, const char *protocols, int flags)
{
    const char      *p, *q, *username, *password, *host, *port, *n, *a, *query_string;
    char            domain[256];
    int             protocol_length, length, valid, c, i, j, anchor;
    enum tld_result result;

    /* set defaults in the info structure */
    tld_clear_info(info);

    if(uri == NULL || uri[0] == '\0')
    {
        return TLD_RESULT_NULL;
    }

    /* check the protocol: [0-9A-Za-z_]+ */
    for(p = uri; *uri != '\0' && *uri != ':'; ++uri)
    {
        if((*uri < 'a' || *uri > 'z')
        && (*uri < 'A' || *uri > 'Z')
        && (*uri < '0' || *uri > '9')
        && *uri != '_')
        {
            return TLD_RESULT_BAD_URI;
        }
    }
    valid = 0;
    protocol_length = (int) (uri - p);
    c = tolower(*p);
    for(q = protocols; *q != '\0';)
    {
        if(q[0] == '*' && (q[1] == '\0' || q[1] == ','))
        {
            valid = 1;
            break;
        }
        if(tolower(*q) == c)
        {
            if(strncasecmp(p, q, protocol_length) == 0
            && (q[protocol_length] == '\0' || q[protocol_length] == ','))
            {
                valid = 1;
                break;
            }
        }
        /* move to the next protocol */
        for(; *q != '\0' && *q != ','; ++q);
        for(; *q == ','; ++q);
    }
    if(valid == 0)
    {
        return TLD_RESULT_BAD_URI;
    }
    if(uri[1] != '/' || uri[2] != '/')
    {
        return TLD_RESULT_BAD_URI;
    }
    uri += 3; /* skip the '://' */

    /* extract the complete domain name with sub-domains, etc. */
    username = NULL;
    host = uri;
    for(; *uri != '/' && *uri != '\0'; ++uri)
    {
        if((unsigned char) *uri < ' ')
        {
            /* forbid control characters in domain name */
            return TLD_RESULT_BAD_URI;
        }
        if(*uri == '@')
        {
            if(username != NULL)
            {
                /* two '@' signs is not possible */
                return TLD_RESULT_BAD_URI;
            }
            username = host;
            host = uri + 1;
        }
        else if(*uri & 0x80)
        {
            if(flags & VALID_URI_ASCII_ONLY)
            {
                /* only ASCII allowed by caller */
                return TLD_RESULT_BAD_URI;
            }
        }
        else if(*uri == ' ' || *uri == '+')
        {
            /* spaces not allowed in domain name */
            return TLD_RESULT_BAD_URI;
        }
        else if(*uri == '%')
        {
            /* the next two digits must be hex
             * note that the first digit must be at least 2 because
             * we do not allow control characters
             */
            if(((uri[1] < '2' || uri[1] > '9')
             && (uri[1] < 'a' || uri[1] > 'f')
             && (uri[1] < 'A' || uri[1] > 'F'))
            || ((uri[2] < '0' || uri[2] > '9')
             && (uri[2] < 'a' || uri[2] > 'f')
             && (uri[2] < 'A' || uri[2] > 'F')))
            {
                return TLD_RESULT_BAD_URI;
            }
            if(uri[1] == '2' && uri[2] == '0')
            {
                /* spaces not allowed in domain name */
                return TLD_RESULT_BAD_URI;
            }
            if(uri[1] >= '8' && (flags & VALID_URI_ASCII_ONLY))
            {
                /* only ASCII allowed by caller */
                return TLD_RESULT_BAD_URI;
            }
            /* skip the two digits right away */
            uri += 2;
        }
    }
    if(username != NULL)
    {
        password = username;
        for(; *password != '@' && *password != ':'; ++password);
        if(*password == ':')
        {
            if((host - 1) - (password + 1) <= 0)
            {
                /* empty password are not acceptable */
                return TLD_RESULT_BAD_URI;
            }
        }
        if(password - username - 1 <= 0)
        {
            /* username cannot be empty */
            return TLD_RESULT_BAD_URI;
        }
    }
    for(port = host; *port != ':' && port < uri; ++port);
    if(*port == ':')
    {
        /* we have a port, it must be digits [0-9]+ */
        for(n = port + 1; *n >= '0' && *n <= '9'; ++n);
        if(n != uri || n == port + 1)
        {
            /* port is empty or includes invalid characters */
            return TLD_RESULT_BAD_URI;
        }
    }

    /* check the address really quick */
    query_string = NULL;
    anchor = 0;
    for(a = uri; *a != '\0'; ++a)
    {
        if((unsigned char) *a < ' ')
        {
            /* no control characters allowed */
            return TLD_RESULT_BAD_URI;
        }
        else if(*a == '+' || *a == ' ') /* old space encoding */
        {
            if(flags & VALID_URI_NO_SPACES)
            {
                /* spaces not allowed by caller */
                return TLD_RESULT_BAD_URI;
            }
        }
        else if(*a == '?')
        {
            query_string = a + 1;
        }
        else if(*a == '&' && anchor == 0)
        {
            if(query_string == NULL)
            {
                /* & must be encoded if used before ? */
                return TLD_RESULT_BAD_URI;
            }
            query_string = a + 1;
        }
        else if(*a == '=')
        {
            if(query_string != NULL && a - query_string == 0)
            {
                /* a query string variable name cannot be empty */
                return TLD_RESULT_BAD_URI;
            }
        }
        else if(*a == '#')
        {
            query_string = NULL;
            anchor = 1;
        }
        else if(*a == '%')
        {
            /* the next two digits must be hex
             * note that the first digit must be at least 2 because
             * we do not allow control characters
             */
            if(((a[1] < '2' || a[1] > '9')
             && (a[1] < 'a' || a[1] > 'f')
             && (a[1] < 'A' || a[1] > 'F'))
            || ((a[2] < '0' || a[2] > '9')
             && (a[2] < 'a' || a[2] > 'f')
             && (a[2] < 'A' || a[2] > 'F')))
            {
                return TLD_RESULT_BAD_URI;
            }
            if(a[1] == '2' && a[2] == '0' && (flags & VALID_URI_NO_SPACES))
            {
                /* spaces not allowed by caller */
                return TLD_RESULT_BAD_URI;
            }
            if(a[1] >= '8' && (flags & VALID_URI_ASCII_ONLY))
            {
                /* only ASCII allowed by caller */
                return TLD_RESULT_BAD_URI;
            }
            /* skip the two digits right away */
            a += 2;
        }
        else if(*a & 0x80)
        {
            if(flags & VALID_URI_ASCII_ONLY)
            {
                /* only ASCII allowed by caller */
                return TLD_RESULT_BAD_URI;
            }
        }
    }

    /* check the domain */

/** \todo
 * The following is WRONG:
 * \li the domain \%XX are not being checked properly, as it stands the
 *     characters following % can be anything!
 * \li the tld() function must be called with the characters still
 *     encoded; if you look at the data, you will see that I kept
 *     the data encoded (i.e. with the \%XX characters)
 * \li what could be checked (which I guess could be for the entire
 *     domain name) is whether the entire string represents valid
 *     UTF-8; I don't think I'm currently doing so here. (I have
 *     such functions in the tld_domain_to_lowercase() now)
 */

    length = (int) (port - host);
    if(length >= (int) (sizeof(domain) / sizeof(domain[0])))
    {
        /* sub-domains + domain + TLD is more than 255 characters?!
         * note that the host main include many %XX characters but
         * we ignore the fact here at this time; we could move this
         * test in the for() loop below though.
         */
        return TLD_RESULT_BAD_URI;
    }
    if(length == 0)
    {
        /* although we could return TLD_RESULT_NULL it would not be
         * valid here because "http:///blah.com" is invalid, not NULL
         */
        return TLD_RESULT_BAD_URI;
    }
    for(i = 0, j = 0; i < length; ++i, ++j)
    {
        if(host[i] == '%') {
            domain[j] = (char) (h2d(host[i + 1]) * 16 + h2d(host[i + 2]));
            i += 2; /* skip the 2 digits */
        }
        else
        {
            domain[j] = host[i];
        }
        /* TODO: check that characters are acceptable in a domain name */
    }
    domain[j] = '\0';
    result = tld(domain, info);
    if(info->f_tld != NULL)
    {
        /* define the TLD inside the source string which "unfortunately"
         * is not null terminated by '\0'; also fix the offset since in
         * the complete URI the TLD is a bit further away
         */
        info->f_tld = host + info->f_offset;
        info->f_offset = (int) (info->f_tld - p);
    }
    return result;
}


/** \brief Return the version of the library.
 *
 * This functino returns the version of this library. The version
 * is defined with three numbers: \<major>.\<minor>.\<patch>.
 *
 * You should be able to use the libversion to compare different
 * libtld versions and know which one is the newest version.
 *
 * \return A constant string with the version of the library.
 */
const char *tld_version()
{
    return LIBTLD_VERSION;
}


/** \def LIBTLD_EXPORT
 * \brief The export API used by MS-Windows DLLs.
 *
 * This definition is used to mark functions and classes as exported
 * from the library. This allows other programs to automatically use
 * functions defined in the library.
 *
 * The LIBTLD_EXPORT may be set to dllexport or dllimport depending
 * on whether you compile the library or you intend to link against it.
 */

/** \def LIBTLD_VERSION
 * \brief The version of the library as a string.
 *
 * This definition represents the version of the libtld header you
 * are compiling against. You can compare it to the returned value
 * of the tld_version() function to make sure that everything is
 * compatible (i.e. if the version is not the same, then the
 * tld_info structure may have changed.)
 */

/** \def LIBTLD_VERSION_MAJOR
 * \brief The major version as a number.
 *
 * This definition represents the major version of the libtld header
 * you are compiling against.
 */

/** \def LIBTLD_VERSION_MINOR
 * \brief The minor version as a number.
 *
 * This definition represents the minor version of the libtld header
 * you are compiling against.
 */

/** \def LIBTLD_VERSION_PATCH
 * \brief The patch version as a number.
 *
 * This definition represents the patch version of the libtld header
 * you are compiling against. Some people call this number the release
 * number.
 */

/** \def VALID_URI_ASCII_ONLY
 * \brief Whether to check that the URI only includes ASCII.
 *
 * By default the tld_check_uri() function accepts any extended character
 * (i.e. characters over 0x80). This flag can be used to refuse such
 * characters.
 */

/** \def VALID_URI_NO_SPACES
 * \brief Whether to check that the URI do not include any spaces.
 *
 * By default the tld_check_uri() function accepts spaces as valid
 * characters in a URI (whether they are explicit " ", or written as
 * "+" or "%20".) This flag can be used to refuse all spaces (i.e.
 * this means the "+" and "%20" are also refused.)
 */

/** \enum tld_category
 * \brief The list of categories for the different TLDs.
 *
 * Defines the category of the TLD. The most well known categories
 * are International TLDs (such as .com and .info) and the countries
 * TLDs (such as .us, .uk, .fr, etc.)
 *
 * IANA offers and is working on other extensions such as .pro for
 * profesionals, and .arpa for their internal infrastructure.
 */

/** \var TLD_CATEGORY_INTERNATIONAL
 * \brief International TLDs
 *
 * This category represents TLDs that can be used by anyone anywhere
 * in the world. In some cases, these have some limits (i.e. only a
 * museum can register a .museum TLD.) However, the most well known
 * international extension is .com and this one has absolutely no
 * restrictions.
 */

/** \var TLD_CATEGORY_PROFESSIONALS
 * \brief Professional TLDs
 *
 * This category is offered to professionals. Some countries already
 * offer second-level domain name registrations for professionals and
 * either way they are not used very much. These are reserved for people
 * such as accountants, attorneys, and doctors.
 *
 * Only people who have a lisence with a government can register a .pro
 * domain name.
 */

/** \var TLD_CATEGORY_LANGUAGE
 * \brief Language specific TLDs
 *
 * At time of writing, there is one language extension: .cat for the
 * Catalan language. The idea of the language extensions is to offer
 * a language, rather than a country, a way to have a website that
 * all the people on the Earth can read in their language.
 */

/** \var TLD_CATEGORY_GROUPS
 * \brief Groups specific TLDs
 *
 * The concept of groups is similar to the language grouping, but in
 * this case it may reference to a specific group of people (but not
 * based on anything such as etnicity.)
 *
 * Examples of groups are Kids, Gay people, Ecologists, etc. This is
 * only proposed at this point.
 */

/** \var TLD_CATEGORY_REGION
 * \brief Region specific TLDs
 *
 * It has been proposed, like the .eu, to have extensions based on
 * well defined regions such as .asia for all of Asia. We currently
 * also have .aq for Antartique. Some proposed regions are .africa
 * and city names such as .paris and .wien.
 *
 * Old TLDs that were for countries but are not assigned to those
 * because the country \em disappeared (i.e. in general was split in
 * two and both new countries have different names,) and future
 * regions appear in this category.
 *
 * We keep old TLDs because it is not unlikely that such will be
 * used every now and then and they can, in this way, cleanly be
 * refused by your software.
 */

/** \var TLD_CATEGORY_TECHNICAL
 * \brief Technical extensions are considered internal.
 *
 * These are likely valid (i.e. the .arpa is valid) but are used for
 * technical reasons and not for regular URIs. So they are present
 * but must certainly be ignored by your software.
 *
 * To avoid returning TLD_RESULT_SUCCESS when a TLD with such a
 * category is found, we mark these with the
 * TLD_STATUS_INFRASTRUCTURE.
 */

/** \var TLD_CATEGORY_COUNTRY
 * \brief A country extension.
 *
 * Most of the extensions are country extensions. Country extensions
 * are generally further broken down with second-level domain names.
 * Some countries even have third, forth, and fifth level domain
 * names.
 */

/** \var TLD_CATEGORY_ENTREPRENEURIAL
 * \brief A private extension.
 *
 * Some private companies and individuals purchased domains that they
 * then use as a TLD reselling sub-domains from that main domain name.
 *
 * For example, the ".blogspot.com" domain is offered by blogspot as
 * a TLD to their users. This gives the users the capability to
 * define a cookie at the ".blogspot.com" level but not directly
 * under ".com". In other words, two distinct site such as:
 *
 * \li "a.blogspot.com", and
 * \li "b.blogspot.com"
 *
 * cannot share their cookies. Yet, ".com" by itself is also a
 * top-level domain name that anyone can use.
 */

/** \var TLD_CATEGORY_BRAND
 * \brief The TLD is owned and represents a brand.
 *
 * This category is used to mark top level domain names that are
 * specific to one company. Note that certain TLDs are owned by
 * companies now, but they are not automatically marked as a
 * brand (i.e. ".lol").
 */

/** \var TLD_CATEGORY_UNDEFINED
 * \brief The TLD was not found.
 *
 * This category is used to initialize the information structure and
 * is used to show that the TLD was not found.
 */

/** \enum tld_status
 * \brief Defines the current status of the TLD.
 *
 * Each TLD has a status. By default, it is generally considered valid,
 * however, many TLDs are either proposed or deprecated.
 *
 * Proposed TLDs are not yet officially accepted by the official entities
 * taking care of those TLDs. They should be refused, but may become
 * available later.
 *
 * Deprecated TLDs were in use before but got dropped. They may be dropped
 * because a country doesn't follow up on their Internet TLD, or because
 * the extension is found to be \em boycotted.
 */

/** \var TLD_STATUS_VALID
 * \brief The TLD is currently valid.
 *
 * This status represents a TLD that is currently fully valid and supported
 * by the owners.
 *
 * These can be part of URIs representing valid resources.
 */

/** \var TLD_STATUS_PROPOSED
 * \brief The TLD was proposed but not yet accepted.
 *
 * The TLD is nearly considered valid, at least it is in the process to get
 * accepted. The TLD will not work until officially accepted.
 *
 * No valid URIs can include this TLD until it becomes TLD_STATUS_VALID.
 */

/** \var TLD_STATUS_DEPRECATED
 * \brief The TLD was once in use.
 *
 * This status is used by TLDs that were valid (TLD_STATUS_VALID) at some point
 * in time and was changed to another TLD rendering that one useless (or
 * \em incorrect in the case of a country name change.)
 *
 * This status means such URIs are not to be considered valid. However, it may
 * be possible to emit a 301 (in terms of HTTP protocol) to fix the problem.
 */

/** \var TLD_STATUS_UNUSED
 * \brief The TLD was officially assigned but not put to use.
 *
 * This special status is used for all the TLDs that were assigned to a specific
 * entity, but never actually put to use. Many smaller countries (especially
 * islands) are assigned this status.
 *
 * Unused TLDs are not valid in any URI until marked valid.
 */

/** \var TLD_STATUS_RESERVED
 * \brief The TLD is reserved so no one can use it.
 *
 * This special case forces the specified TLDs into a "do not use" list. Seeing
 * such TLDs may happen by people who whish it were official, but it is not
 * considered \em legal.
 *
 * A reserved TLD may represent a second TLD that was assigned to a specific
 * country or other category. It may be possible to do a transfer from that
 * TLD to the official TLD (i.e. Great Britain was assigned .gb, but instead
 * uses .uk; URIs with .gb could be transformed with .uk and checked for
 * validity.)
 */

/** \var TLD_STATUS_INFRASTRUCTURE
 * \brief These TLDs are reserved for the Internet infrastructure.
 *
 * These TLDs cannot be used with standard URIs. These are used to make the
 * Internet functional instead.
 *
 * All URIs for standard resources must refuse these URIs.
 */

/** \var TLD_STATUS_UNDEFINED
 * \brief Special status to indicate we did not find the TLD.
 *
 * The info structure is returned with an \em undefined status whenever the
 * TLD could not be found in the list of existing TLDs. This means the URI
 * is completely invalid. (The only exception would be if you support some
 * internal TLDs.)
 *
 * URI what cannot get a TLD_STATUS_VALID should all be considered invalid.
 * But those marked as TLD_STATUS_UNDEFINED are completely invalid. This
 * being said, you may want to make sure you passed the correct string.
 * The URI must be just and only the set of sub-domains, the domain, and
 * the TLDs. No protocol, slashes, colons, paths, query strings, anchors
 * are accepted in the URI.
 */

/** \var TLD_STATUS_EXCEPTION
 * \brief Special status to indicate an exception which is not directly a TLD.
 *
 * When a NIC decides to change their setup it can generate exceptions. For
 * example, the UK first made use of .uk and as such offered a few customers
 * to use .uk. Later they decided to only offer second level domain names
 * such as the .co.uk and .ac.uk. This generates a few exceptions on the .uk
 * domain name. For example, the police.uk domain is still in use and thus
 * it is an exception. We reference it as ".police.uk" in our XML data file
 * yet the TLD in that case is just ".uk".
 */


/** \enum tld_result
 * \brief The result returned by tld().
 *
 * This enumeration defines all the possible results of the tld() function.
 *
 * Only the TLD_RESULT_SUCCESS is considered to represent a valid result.
 *
 * The TLD_RESULT_INVALID represents a TLD that was found but is not currently
 * marked as valid (it may be deprecated or proposed, for example.)
 */

/** \var TLD_RESULT_SUCCESS
 * \brief Success! The TLD of the specified URI is valid.
 *
 * This result is returned when the URI includes a valid TLD. The function
 * further includes valid results in the tld_info structure.
 *
 * You can accept this URI as valid.
 */

/** \var TLD_RESULT_INVALID
 * \brief The TLD was found, but it is marked as invalid.
 *
 * This result represents a TLD that is not valid as is for a URI, but it
 * was defined in the TLD data. The function includes further information
 * in the tld_info structure. There you can check the category, status,
 * and other parameters to determine what the TLD really represents.
 *
 * It may be possible to use such a TLD, although as far as web addresses
 * are concerned, these are not considered valid. As mentioned in the
 * statuses, some may mean that the TLD can be changed for another and
 * work (i.e. a country name that changed.)
 */

/** \var TLD_RESULT_NULL
 * \brief The input URI is empty.
 *
 * The tld() function returns this value whenever the input URI pointer is
 * NULL or the empty string (""). Obviously, no TLD is found in this case.
 */

/** \var TLD_RESULT_NO_TLD
 * \brief The input URI has no TLD defined.
 *
 * Whenever the URI does not include at least one period (.), this error
 * is returned. Local URIs are considered valid and don't generally include
 * a period (i.e. "localhost", "my-computer", "johns-computer", etc.) We
 * expect that the tld() function would not be called with such URIs.
 *
 * A valid Internet URI must include a TLD.
 */

/** \var TLD_RESULT_BAD_URI
 * \brief The URI includes characters that are not accepted by the function.
 *
 * This value is returned if a character is found to be incompatible or a
 * sequence of characters is found incompatible.
 *
 * At this time, tld() returns this error if two periods (.) are found one
 * after another. The errors will be increased with time to detect invalid
 * characters (anything outside of [-a-zA-Z0-9.%].)
 *
 * Note that the URI should not start or end with a period. This error will
 * also be returned (at some point) when the function detects such problems.
 */

/** \var TLD_RESULT_NOT_FOUND
 * \brief The URI has a TLD that could not be determined.
 *
 * The TLD of the URI was searched in the TLD data and could not be found
 * there. This means the TLD is not a valid Internet TLD.
 */


/** \struct tld_info
 * \brief Set of information returned by the tld() function.
 *
 * This structure is used by the tld() function to define the results to
 * return to the caller.
 *
 * Remember that this is a C structure. By default, the fields are undefined.
 * The tld() function will first defined these fields, before returning any
 * result.
 *
 * It is acceptable to clear the structure before calling the tld() function
 * but it is not required.
 */

/** \var enum tld_category tld_info::f_category;
 * \brief The category of the TLD.
 *
 * This represents the category of the TLD. One of the tld_category enumeration
 * values can be found in this field.
 *
 * \sa enum tld_category
 */

/** \var enum tld_status tld_info::f_status;
 * \brief The status of the TLD.
 *
 * This value defines the current status of the TLD. Most of the TLDs we define
 * are valid, but some are either deprecated, unused, or proposed.
 *
 * Only a TLD marked as TLD_STATUS_VALID should be considered valid, although
 * otherwise may be accepted in some circumstances.
 *
 * \sa enum tld_status
 */

/** \var const char *tld_info::f_country;
 * \brief The country where this TLD is used.
 *
 * When the f_category is set to TLD_CATEGORY_COUNTRY then this field is a
 * pointer to the name of the country in English (although some may include
 * accents, the strings are in UTF-8.)
 *
 * This field is set to NULL if the category is not Country or the TLD was
 * not found.
 *
 * \sa tld_info::f_category
 * \sa enum tld_category
 */

/** \var const char *tld_info::f_tld;
 * \brief Pointer to the TLD in the URI string you supplied.
 *
 * This is a pointer to the TLD section that the tld() function found in
 * your URI. Note that it is valid only as long as your URI string pointer.
 *
 * It is also possible to make use of the tld_info::f_offset value to
 * extract the TLD, domain, or sub-domains.
 *
 * If the TLD is not found, this field is NULL.
 */

/** \var int tld_info::f_offset;
 * \brief The offset to the TLD in the URI string you supplied.
 *
 * This offset, when added to the URI string pointer, gets you to the
 * TLD of that URI. The offset can also be used to start searching
 * for the beginning of the domain name by searching for the previous
 * period from that offset minus one. In effect, this gives you a
 * way to determine the list of sub-domain.
 */

/** \struct tld_description
 * \brief [internal] The description of one TLD.
 * \internal
 *
 * The XML data is transformed in an array of TLD description saved in this
 * structure.
 *
 * This structure is internal to the database. You never are given direct
 * access to it. However, some of the constant pointers (i.e. country names)
 * do point to that data.
 */

/** \var tld_description::f_category
 * \brief The category of this entry.
 *
 * The XML data must defined the different TLDs inside catageorized area
 * tags. This variable represents that category.
 */

/** \var tld_description::f_country
 * \brief The name of the country owning this TLD.
 *
 * The name of the country owning this entry. Many TLDs do not have a
 * country attached to it (i.e. .com and .info, for example, do not have
 * a country attached to them) in which case this pointer is NULL.
 */

/** \var tld_description::f_start_offset
 * \brief The first offset of a list of TLDs.
 *
 * This offset represents the start of a list of TLDs. The start offset is
 * inclusive so that very offset IS included in the list.
 *
 * The TLDs being referenced from this TLD are those between f_start_offset
 * and f_end_offset - 1 also writte:
 *
 * [f_start_offset, f_end_offset)
 */

/** \var tld_description::f_end_offset
 * \brief The last offset of a list of TLDs.
 *
 * This offset represents the end of a list of TLDs. The end offset is
 * exclusive so that very offset is NOT included in the list.
 *
 * The TLDs being referenced from this TLD are those between f_start_offset
 * and f_end_offset - 1 also writte:
 *
 * [f_start_offset, f_end_offset)
 */

/** \var tld_description::f_exception_apply_to
 * \brief This TLD is an exception of the "apply to" TLD.
 *
 * With time, some TLDs were expected to have or not have certain sub-domains
 * and when removal of those was partial (i.e. did not force existing owners
 * to lose their domain) then we have exceptions. This variable holds the
 * necessary information to support such exceptions.
 *
 * The "apply to" is only defined if the entry is an exception (see f_status.)
 * The f_exception_apply_to value is an offset to the very TLD we want to
 * return when we get this exception.
 */

/** \var tld_description::f_exception_level
 * \brief This entry is an exception representing a TLD at this specified level.
 *
 * When we find an exception, it may be more than 1 level below the TLD it uses
 * (a.b.c.d may be viewed as part of TLD .d thus .a has to be bumped 3 levels
 * up.) In most cases, this is equal to this TLD level - 1.
 */

/** \var tld_description::f_status
 * \brief The status of this TLD.
 *
 * The status of a TLD is TLD_STATUS_VALID by default. Using the different
 * tags available in the XML file we can defined other statuses such as the
 * TLD_STATUS_DEPRECATED status.
 *
 * In the TLD table the status can be TLD_STATUS_EXCEPTION.
 */

/** \var tld_description::f_tld
 * \brief The actual TLD of this entry.
 *
 * In this table, the TLD is actually just one name and no period. Other
 * parts of a multi-part TLD are found at the [f_start_offset, f_end_offset).
 *
 * The TLD is built by starting a search at the top level which is defined as 
 * [tld_start_offset, tld_end_offset). These offsets are global variables defined
 * in the tld_data.c file.
 */

/* vim: ts=4 sw=4 et
 */
