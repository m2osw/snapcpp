/* TLD library -- PHP extension to call tld() and tld_check_uri() from PHP
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
 * \brief To directly use the libtld library within PHP.
 *
 * This file declares the necessary functions to interface the libtld
 * library in the PHP environment.
 *
 * Some of the code is based on information found in various locations.
 * However, the Zend information for creating PHP variables is pretty
 * well explained on this page:
 *
 * http://devzone.zend.com/317/extension-writing-part-ii-parameters-arrays-and-zvals/
 *
 * \section check_tld check_tld($domain);
 *
 * \sa tld()
 *
 * This PHP function takes one string representing a domain name including a
 * valid TLD (top level domain.)
 *
 * The result of the function is an associative array with the following fields:
 *
 * \li $result["result"]
 *
 * The result returned by the check_tld() function. This is one of the
 * TLD_RESULT_... values as defined in the php_libtld.php "header" file.
 * Note that a result other than TLD_RESULT_SUCCESS does not mean that
 * the TLD wasn't found. However, if found, it either is not in use or
 * you are missing a sub-TLD name (for example .uk cannot be used by
 * itself except with a very few domains; most domains must use .co.uk,
 * .ac.uk, etc.).
 *
 * \li $result["category"]
 *
 * The result category. This is a number as defined in the php_libtld.php
 * "header" file. Most TLDs have the TLD_CATEGORY_COUNTRY value. This is
 * useful information, but in most cases useless to make use of the
 * domain name itself.
 *
 * \li $result["status"]
 *
 * The status of this TLD as defined by IANA and other similar organization.
 * The status is a number as defined in the php_libtld.php "header" file.
 * Only TLDs that are marked as TLD_STATUS_VALID should be used. Other TLDs
 * should either not be used or a user intervention should apply. For
 * example, the TLD_STATUS_PROPOSED should not be used until the TLD is in
 * effect, but some precursors may want to allow such as they test that TLD.
 *
 * \li $result["offset"]
 *
 * This value is the offset where the TLD starts in the unmodified input
 * string. You can therefore extract the result using this offset in
 * a substr() call:
 *
 * \code
 * $domain = "some.tld.here.com";
 * $result = check_tld($domain);
 * // IMPORTANT: here test that $result is valid... do not use the offset otherwise
 * $tld = substr($domain, $result["offset"]);
 * $domain_name = substr($domain, 0, $result["offset"] - 1);
 * \endcode
 *
 * \li $result["country"] (optional)
 *
 * If the category was set to TLD_CATEGORY_COUNTRY, then this entry defines
 * the country that owns this TLD. For example, ".fr" is owned by France.
 *
 * Note that this field is NOT defined if the category is not country.
 *
 * \li $result["tld"] (optional)
 *
 * If the function succeeded and a TLD is available, then this variable is
 * the TLD part of the domain. This is similar to taking that TLD using the
 * substr() function as shown in the $result["offset"].
 *
 * \section check_uri check_uri($uri, $protocols, $flags);
 *
 * \sa tld_check_uri()
 *
 * The check_uri() function verifies that the specified \p $uri is indeed a
 * valid URI. The \p $protocols is a list of schemes that the check_uri()
 * should accept. Any scheme that is not listed in that parameter are
 * viewed as invalid (unknown.) The flags define a few different ways to
 * behave.
 *
 * The check process is to detect the scheme, verify that it is a supported
 * (acceptable) scheme, process the sub-domains, domain, and TLD, then
 * parse the path, variables, and anchor if present.
 *
 * The result of the function is an associative array that matches the array
 * returned by the earlier %check_tld() function.
 *
 * \section check_email check_email($emails, $flags);
 *
 * \sa tld_email_parse()
 *
 * This function parses a string of emails and returns an array with the
 * results. The array is composed of one sub-array per email found
 * in the \p $emails parameter and indexed numerically starting at 0.
 * The main array also includes the result of the tld_email_parse()
 * function as an integer associated under the name "result".
 *
 * \code
 * $r = check_email("blah@m2osw.com", 0);
 * if ($r["result"] == TLD_RESULT_SUCCESS)
 * {
 *   // the email is valid check more about it
 *   $max = $r["count"];
 *   for($i = 0; $i < $max; ++$i)
 *   {
 *      // get email $i
 *      $e = $r[$i];
 *      // now $e is one of the emails, you can work on it...
 *      $canonical = $e["canonicalized_email"];
 *   }
 * }
 * \endcode
 *
 * If the result is not TLD_RESULT_SUCCESS, then the list of emails will
 * be empty (not even defined) and the "count" parameter will be zero.
 * Otherwise, each email is an array with the following fields:
 *
 * \li $e["group"] (optional)
 *
 * The name of the group. This is rarely used and in most cases it will
 * be empty. Note that when a group is defined, one entry appears
 * representing the group itself. That special entry has the group parameter
 * defined and all the other parameters are the empty string.
 *
 * Note that because the group is rarely used it was marked as optional,
 * so if not group was defined this parameter is not defined.
 *
 * \li $e["original_email"]
 *
 * The email as found in the original string, with all white spaces,
 * new lines, and carriage returns stripped from the start and end of
 * the email.
 *
 * This entry may still include comments and it may mix atom syntax
 * with quoted strings and domain literal when those could be simplified.
 *
 * \li $e["fullname"]
 *
 * The display name is called fullname in our library. In most cases, this
 * is used to name the owner of the mailbox; for example "Alexis Wilke" or
 * "Wilke, Alexis".
 *
 * This string may be empty since the display name is not required.
 *
 * \li $e["username"]
 *
 * The username part of the email address. This is the atom that appears
 * before the @ character. In most cases, these are atom characters, although
 * when the name is written between double quotes, many more characters
 * can appear in a username (For example, "\\"" represents a user named ".)
 *
 * \li $e["domain"]
 *
 * The domain part of the email address. This is the atom that appears
 * after the @ character. As with the username, the domain names are
 * generally limited to atom characters but it is possible to make use
 * of other characters when written between square brackets.
 *
 * Note that before returning the tld_check_uri() function verifies that the
 * domain is valid so in all cases this domain information is likely correct.
 * Although note that we do not check whether the domain is currently
 * registered (no whois...)
 *
 * The check is done against the TLD. So if you are validating an email
 * entered by a user and, for example, they put an extra m at the end as
 * in [m2osw.comm], then the parser fails and you do not get any information
 * about this email.
 *
 * \li $e["email_only"]
 *
 * The email-only field includes just and only the email. That is the part
 * with the user name, the @ character, and the domain name. The user name
 * and domain are protected as required (in the event it was necessary.)
 *
 * In a way this is a canonicalized version of the email address.
 *
 * \li $e["canonicalized_email"]
 *
 * The canonicalized email is the display name (fullname), the user name,
 * and the domain name all in one as we most often see in complete email
 * addresses.
 *
 * This email does not include any comments, group, or useless spaces. It
 * is as short as possible. Also if one of the parts needs quotation, it
 * is used, but if no quotations are necessary, then none are used.
 */

#include <php.h>
#include "libtld/tld.h"

/// \brief Declaration of the check_tld() function in PHP.
PHP_FUNCTION(check_tld);
ZEND_BEGIN_ARG_INFO_EX(arginfo_check_tld, 0, 0, 1)
    ZEND_ARG_INFO(0, uri)
ZEND_END_ARG_INFO()

/// \brief Declaration of the check_uri() function in PHP.
PHP_FUNCTION(check_uri);
ZEND_BEGIN_ARG_INFO_EX(arginfo_check_uri, 0, 0, 1)
    ZEND_ARG_INFO(0, uri)
ZEND_END_ARG_INFO()

/// \brief Declaration of the check_email() function in PHP.
PHP_FUNCTION(check_email);
ZEND_BEGIN_ARG_INFO_EX(arginfo_check_email, 0, 0, 1)
    ZEND_ARG_INFO(0, email)
ZEND_END_ARG_INFO()

/* {{{ pgsql_functions[]
 */
const zend_function_entry libtld_functions[] = {
    PHP_FE(check_tld,   arginfo_check_tld)
    PHP_FE(check_uri,   arginfo_check_uri)
    PHP_FE(check_email, arginfo_check_email)
    PHP_FE_END
};
/* }}} */

/* {{{ pgsql_module_entry
 */
zend_module_entry libtld_module_entry = {
    STANDARD_MODULE_HEADER,
    "libtld",
    libtld_functions,
    NULL, //PHP_MINIT(libtld),
    NULL, //PHP_MSHUTDOWN(libtld),
    NULL, //PHP_RINIT(libtld),
    NULL, //PHP_RSHUTDOWN(libtld),
    NULL, //PHP_MINFO(libtld),
    NO_VERSION_YET,
    0, //PHP_MODULE_GLOBALS(libtld),
    NULL, //PHP_GINIT(libtld),
    NULL, // Global constructor
    NULL, // Global destructor
    NULL, // Post deactivate
    STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

zend_module_entry *get_module(void)
{
    return &libtld_module_entry;
}


/* {{{ proto mixed check_tld(string uri)
   Check a URI and return the result or FALSE */
PHP_FUNCTION(check_tld)
{
    char *query;
    int query_len;
    int argc = ZEND_NUM_ARGS();
    struct tld_info info;
    enum tld_result r;

    if (argc != 1)
    {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &query, &query_len) == FAILURE)
    {
        RETURN_FALSE;
    }

    r = tld(query, &info);

    array_init(return_value);
    add_assoc_long(return_value, "result", r);
    add_assoc_long(return_value, "category", info.f_category);
    add_assoc_long(return_value, "status", info.f_status);
    add_assoc_long(return_value, "offset", info.f_offset);
    if(info.f_country != NULL)
    {
        add_assoc_string(return_value, "country", (char *) info.f_country, 1);
    }
    if(info.f_tld != NULL)
    {
        add_assoc_string(return_value, "tld", (char *) info.f_tld, 1);
    }
}
/* }}} */

/* {{{ proto mixed check_uri(string uri, string protocols, int flags)
   Check a complete URI for validity */
PHP_FUNCTION(check_uri)
{
    char *query, *protocols;
    int query_len, *protocols_len;
    int argc = ZEND_NUM_ARGS();
    long flags = 0;
    struct tld_info info;
    enum tld_result r;

    if (argc != 3)
    {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &query, &query_len, &protocols, &protocols_len, &flags) == FAILURE)
    {
        RETURN_FALSE;
    }

    r = tld_check_uri(query, &info, protocols, flags);

    array_init(return_value);
    add_assoc_long(return_value, "result", r);
    add_assoc_long(return_value, "category", info.f_category);
    add_assoc_long(return_value, "status", info.f_status);
    add_assoc_long(return_value, "offset", info.f_offset);
    if(info.f_country != NULL)
    {
        add_assoc_string(return_value, "country", (char *) info.f_country, 1);
    }
    if(info.f_tld != NULL)
    {
        add_assoc_string(return_value, "tld", (char *) info.f_tld, 1);
    }
}
/* }}} */

/* {{{ proto mixed check_email(string eamil, int flags)
   Check a string of emails as defined in an email field like the From: or To:
   fields found in the email header. */
PHP_FUNCTION(check_email)
{
    char *emails;
    int emails_len;
    int argc = ZEND_NUM_ARGS();
    int idx;
    long flags = 0;
    struct tld_email_list *list;
    struct tld_email email;
    zval *email_arr;
    enum tld_result r;

    if (argc != 2)
    {
        WRONG_PARAM_COUNT;
    }

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &emails, &emails_len, &flags) == FAILURE)
    {
        RETURN_FALSE;
    }

    list = tld_email_alloc();
    if(list == NULL)
    {
        RETURN_FALSE;
    }

    array_init(return_value);

    r = tld_email_parse(list, emails, flags);

    idx = 0;
    if(r == TLD_RESULT_SUCCESS)
    {
        while(tld_email_next(list, &email))
        {
            ALLOC_INIT_ZVAL(email_arr);
            array_init(email_arr);
            // only the group is optional because it is so rarely used
            if(*email.f_group != '\0')
            {
                add_assoc_string(email_arr, "group",         (char *) email.f_group, 1);
            }
            add_assoc_string(email_arr, "original_email",    (char *) email.f_original_email, 1);
            add_assoc_string(email_arr, "fullname",          (char *) email.f_fullname, 1);
            add_assoc_string(email_arr, "username",          (char *) email.f_username, 1);
            add_assoc_string(email_arr, "domain",            (char *) email.f_domain, 1);
            add_assoc_string(email_arr, "email_only",        (char *) email.f_email_only, 1);
            add_assoc_string(email_arr, "canonicalized_email", (char *) email.f_canonicalized_email, 1);

            add_index_zval(return_value, idx, email_arr);
            ++idx;
        }
    }
    add_assoc_long(return_value, "count", idx);

    // also return the result of the call
    add_assoc_long(return_value, "result", r);

    tld_email_free(list);
}
/* }}} */

/* additional Doxygen documentation to not interfer with the PHP documentation. */

/** \fn get_module()
 * \brief Function called to retrieve the module information.
 *
 * This global function is used to retrieve the module definition of this
 * PHP extension. That definition includes all the necessary declarations
 * for PHP to understand our extension.
 *
 * \return The pointer to the module structure.
 */

/** \var libtld_functions
 * \brief The list of functions we offer to PHP.
 *
 * This table is the list of functions offered to the PHP interpreter
 * from our library. The list is null terminated.
 */

/** \var libtld_module_entry
 * \brief The module definition.
 *
 * This structure is the libtld module definition to interface with PHP.
 */

/* vim: ts=4 sw=4 et
 */
