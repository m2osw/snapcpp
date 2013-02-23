/* TLD library -- TLD, domain name, and sub-domain extraction
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

/** \file
 * \brief Declaration of the C++ tld_object class.
 *
 * This source file is the declaration of all the functions of the C++
 * tld_object class.
 */

#include "libtld/tld.h"
#include <stdio.h>

/** \class tld_object
 * \brief Class used to ease the use o the tld() function in C++.
 *
 * The tld_object class allows you to query the tld library and then check
 * each part of the URI with simple calls instead of you having to determine
 * the location of each part.
 */

/** \brief Initialize a tld object with the specified domain.
 *
 * This function initializes a TLD object with the specified \p domain
 * name. This function accepts a null terminated C string pointer.
 * The pointer can be set to NULL or point to an empty string in which
 * case the constructor creates an empty TLD object. Note that an
 * empty TLD object is considered invalid and if called some
 * functions throw the invalid_domain exception.
 *
 * \note
 * The string is expected to be UTF-8.
 *
 * \param[in] domain_name  The domain to parse by this object.
 */
tld_object::tld_object(const char *domain_name)
{
    set_domain(domain_name);
}

/** \brief Initialize a tld object with the specified domain.
 *
 * This function initializes a TLD object with the specified \p domain
 * name. This function accepts standard C++ strings. The string can be
 * empty to create an empty TLD object. Note that an empty TLD object
 * is considered invalid and if called some functions throw the
 * invalid_domain exception.
 *
 * \note
 * The string is expected to be UTF-8.
 *
 * \param[in] domain_name  The domain to parse by this object.
 */
tld_object::tld_object(const std::string& domain_name)
{
    set_domain(domain_name);
}

/** \brief Change the domain of a tld object with the newly specified domain.
 *
 * This function initializes this TLD object with the specified \p domain
 * name. This function accepts a null terminated C string pointer.
 * The pointer can be set to NULL or point to an empty string in which
 * case the constructor creates an empty TLD object. Note that an
 * empty TLD object is considered invalid and if called some
 * functions throw the invalid_domain exception.
 *
 * \note
 * The string is expected to be UTF-8.
 *
 * \param[in] domain_name  The domain to parse by this object.
 */
void tld_object::set_domain(const char *domain_name)
{
    set_domain(std::string(domain_name == NULL ? "" : domain_name));
}

/** \brief Change the domain of a tld object with the newly specified domain.
 *
 * This function initializes a TLD object with the specified \p domain
 * name. This function accepts standard C++ strings. The string can be
 * empty to create an empty TLD object. Note that an empty TLD object
 * is considered invalid and if called some functions throw the
 * invalid_domain exception.
 *
 * \note
 * The string is expected to be UTF-8.
 *
 * \param[in] domain_name  The domain to parse by this object.
 */
void tld_object::set_domain(const std::string& domain_name)
{
    // tld() supports empty strings and initializes f_info appropriately
    f_domain = domain_name;
    f_result = tld(f_domain.c_str(), &f_info);
    // TBD -- should we clear f_domain on an invalid result?
}

/** \brief Check the result of the tld() command.
 *
 * This function returns the result that the tld() command produced
 * when called with the domain as specified in a constructor or
 * the set_domain() functions.
 *
 * Valid resutls are:
 *
 * \li TLD_RESULT_SUCCESS -- the URI is valid and all the tld_object functions can be called
 * \li TLD_RESULT_INVALID -- the TLD of this URI exists but the combination used is not acceptable
 * \li TLD_RESULT_NULL -- the domain name is the empty string or NULL
 * \li TLD_RESULT_NO_TLD -- the domain name does not even include one period
 * \li TLD_RESULT_BAD_URI -- URI parsing failed (i.e. two periods one after another)
 * \li TLD_RESULT_NOT_FOUND -- this domain TLD doesn't exist
 *
 * \return The last result of the tld() function.
 */
tld_result tld_object::result() const
{
    return f_result;
}

/** \brief Retrieve the current status of the TLD.
 *
 * This function returns the status that the last tld() call generated. status() along with
 * result() are used to determine whether a call to the TLD succeeded or not. See the
 * is_valid() function too.
 *
 * This function can be used to know why a domain name failed when parsed by the tld() function.
 *
 * \li TLD_STATUS_VALID -- This URI is valid and can be queried further.
 * \li TLD_STATUS_PROPOSED -- This TLD was proposed but is not yet in used.
 * \li TLD_STATUS_DEPRECATED -- This TLD was used and was deprecated.
 * \li TLD_STATUS_UNUSED -- This TLD is simply not used.
 * \li TLD_STATUS_RESERVED -- This TLD is currently reserved.
 * \li TLD_STATUS_INFRASTRUCTURE -- This TLD represents an infrastructure object (.arpa)
 * \li TLD_STATUS_UNDEFINED -- The status is undefined if the TLD cannot be found.
 *
 * \return The status generated by the last tld() function call.
 */
tld_status tld_object::status() const
{
    return f_info.f_status;
}

/** \brief Check whether this TLD object is valid.
 *
 * This function checks the result and status returned by the last call to
 * the tld() function. This object is considered valid if and only if the
 * result is TLD_RESULT_SUCCESS and the status is TLD_STATUS_VALID. At this
 * point, any other result returns invalid and that prevents you from checking
 * the object further (i.e. call the tld_only() function to retrieve the TLD
 * of the specified URI.)
 *
 * \return true if the result and status say this TLD object is valid.
 */
bool tld_object::is_valid() const
{
    return f_result == TLD_RESULT_SUCCESS && f_info.f_status == TLD_STATUS_VALID;
}

/** \brief Retrieve the domain name of this TLD object.
 *
 * The TLD object keeps a copy of the domain name as specified with the
 * constructor. This copy can be retrieved by this function. This is an
 * exact copy of the input (i.e. no canonalization.)
 *
 * \return The domain as specified to the constructor or the set_domain() functions.
 */
std::string tld_object::domain() const
{
    return f_domain;
}

/** \brief Retrieve the sub-domains of the URI.
 *
 * This function returns the sub-domains found in the URI. This may be
 * the empty string.
 *
 * \exception invalid_domain
 * This exception is raised when this function is called with an invalid
 * TLD object. This happens whenever you create the object or call
 * set_domain() with an invalid URI. You should call is_valid() and if
 * false, avoid calling this function.
 *
 * \return All the sub-domains found in the URI.
 */
std::string tld_object::sub_domains() const
{
    if(!is_valid())
    {
        throw invalid_domain();
    }
    const char *domain_name(f_info.f_tld);
    const char *start(f_domain.c_str());
    for(; domain_name > start && domain_name[-1] != '.'; --domain_name);
    if(domain_name == start)
    {
        return "";
    }
    // no not return the period
    return std::string(start, domain_name - start - 1);
}

/** \brief Full domain name: domain and TLD.
 *
 * This function returns the domain name and the TLD as a string.
 *
 * The result includes the domain name but no sub-domains.
 *
 * To get the domain name with the sub-domains, call the domain()
 * function instead. That function returns the URI as passed to
 * this function.
 *
 * \exception invalid_domain
 * This exception is raised when this function is called with an invalid
 * TLD object. This happens whenever you create the object or call
 * set_domain() with an invalid URI. You should call is_valid() and if
 * false, avoid calling this function.
 *
 * \return The fully qualified domain name.
 */
std::string tld_object::full_domain() const
{
    if(!is_valid())
    {
        throw invalid_domain();
    }
    const char *domain_name(f_info.f_tld);
    for(const char *start(f_domain.c_str()); domain_name > start && domain_name[-1] != '.'; --domain_name);
    return domain_name;
}

/** \brief Retrieve the domain name only.
 *
 * This function returns the domain name without the TLD nor any sub-domains.
 *
 * A domain name never includes any period.
 *
 * \exception invalid_domain
 * This exception is raised when this function is called with an invalid
 * TLD object. This happens whenever you create the object or call
 * set_domain() with an invalid URI. You should call is_valid() and if
 * false, avoid calling this function.
 *
 * \return The domain name without TLD or sub-domains.
 */
std::string tld_object::domain_only() const
{
    if(!is_valid())
    {
        throw invalid_domain();
    }
    const char *end(f_info.f_tld);
    const char *domain_name(end);
    for(const char *start(f_domain.c_str()); domain_name > start && domain_name[-1] != '.'; --domain_name);
    return std::string(domain_name, end - domain_name);
}

/** \brief Return the TLD of the URI.
 *
 * This function returns the TLD part of the URI specified in the constructor
 * or the set_domain() function.
 *
 * The TLD is the part that represents a country, a region, a general TLD, etc.
 * Generic TLDs have one period (.com, .info,) but in general you must expect TLDs with
 * several period characters (.ca.us, .indiana.museum, .yawatahama.ehime.jp).
 *
 * \exception invalid_domain
 * This exception is raised when this function is called with an invalid
 * TLD object. This happens whenever you create the object or call
 * set_domain() with an invalid URI. You should call is_valid() and if
 * false, avoid calling this function.
 *
 * \return the TLD part of the URI specified in this TLD object.
 */
std::string tld_object::tld_only() const
{
    if(!is_valid())
    {
        throw invalid_domain();
    }
    return f_info.f_tld;
}

/** \brief Retrieve the category of this URI.
 *
 * This function is used to retrieve the category of the URI. The category is
 * just informative and has no special bearing on the TLD, domain, and sub-domain
 * parts.
 *
 * The existing categories are:
 *
 * \li TLD_CATEGORY_INTERNATIONAL -- TLD names that can be used by anyone in the world
 * \li TLD_CATEGORY_PROFESSIONALS -- TLD names reserved to professionals
 * \li TLD_CATEGORY_LANGUAGE -- language based TLD
 * \li TLD_CATEGORY_GROUPS -- group based TLD
 * \li TLD_CATEGORY_REGION -- TLD representing a region (usually within a country)
 * \li TLD_CATEGORY_TECHNICAL -- technical TLD names used to make it all work
 * \li TLD_CATEGORY_COUNTRY -- country based TLD
 * \li TLD_CATEGORY_ENTREPRENEURIAL -- TLD spawned of other official TLD names
 * \li TLD_CATEGORY_UNDEFINED -- this value means the TLD was not defined
 *
 * \return The category of the current URI or TLD_CATEGORY_UNDEFINED.
 */
tld_category tld_object::category() const
{
    return f_info.f_category;
}

/** \brief The name of the country linked to that TLD.
 *
 * This TLD represents a country and this is its name.
 *
 * If the TLD does not represent a country then this function returns an
 * empty string. If category() returns TLD_CATEGORY_COUNTRY then this
 * function should always return a valid name.
 *
 * \note
 * At a later time we may also include other names such as the language, group, or
 * region that the TLD represents. At that time we'll certainly rename the function
 * and field.
 *
 * \return The name of the country or "" if undefined.
 */
std::string tld_object::country() const
{
    // std::string doesn't like NULL as a parameter
    if(f_info.f_country == NULL)
    {
        return "";
    }
    return f_info.f_country;
}


/** \var tld_object::f_domain
 * \brief The domain or URI as specified in the constructor or set_domain() function.
 *
 * This variable holds the original domain (URI) as passed to the tld_object
 * constructor or set_domain() function.
 *
 * You can retrieve that value with the domain() function. The tld_object never
 * modifies that string.
 *
 * Note that it can be an empty string.
 *
 * \sa tld_object()
 * \sa set_domain()
 * \sa domain()
 */

/** \var tld_object::f_info
 * \brief The information of the domain of this tld_object.
 *
 * This variable holds the information as defined by a call to the tld()
 * function. It holds information whether or not the domain is valid,
 * empty, etc.
 *
 * The structure gets reinitialized each time a call to set_domain() is
 * made and those values are considered cached.
 */

/** \var tld_object::f_result
 * \brief The result of the tld() function call.
 *
 * This variable caches the result of the last tld() call with the URI
 * as defined in the f_domain variable. The f_info also corresponds to
 * this f_result.
 *
 * The result is always initialized to a value or another by constructors
 * and set_domain() methods.
 */


/** \class invalid_domain
 * \brief Exception thrown when querying for data of an invalid domain.
 *
 * This exception is raised when a certain set of functions are called in a
 * tld_object which URI is not valid.
 *
 * Instead of catching this error, you should call the is_valid() function
 * before a function that may otherwise raise this exception and prpoerly
 * handle the case when it returns false.
 */

/** \fn invalid_domain::invalid_domain(const char *what_str)
 * \brief Initialized the invalid_domain exception.
 *
 * This function initializes the invalid_domain exception with the specified
 * \p what_str parameter as the what() string.
 *
 * \param[in] what_str  A string representing the content of the what() string of the exception.
 */


/* vim: ts=4 sw=4 et
 */
