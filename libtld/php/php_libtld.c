/* TLD library -- PHP extension to call tld() and tld_check_uri() from PHP
 * Copyright (C) 2013  Made to Order Software Corp.
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

/** \file
 * \brief To directly use the libtld library within PHP.
 *
 * This file declares the necessary functions to interface the libtld
 * library in the PHP environment.
 */

#include <php.h>
#include "../../BUILD/libtld/tld.h"

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

/* {{{ pgsql_functions[]
 */
const zend_function_entry libtld_functions[] = {
    PHP_FE(check_tld, arginfo_check_tld)
    PHP_FE(check_uri, arginfo_check_uri)
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
