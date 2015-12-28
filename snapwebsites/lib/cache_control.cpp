// Snap Websites Server -- handle Cache-Control settings
// Copyright (C) 2011-2015  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "cache_control.h"

#include "http_strings.h"
#include "log.h"
#include "not_reached.h"


namespace snap
{


/** \brief Initialize a cache control object with defaults.
 *
 * This function initialize this cache control object with the
 * defaults and returns.
 *
 * You may later apply various changes to the cache control data
 * using the set_...() functions and the set_cache_info() if you
 * have cache control data in the form of a standard HTTP string.
 */
cache_control_settings::cache_control_settings()
    //: f_max_age(0) -- auto-init
    //, f_max_stale(-1) -- auto-init
    //, f_min_fresh(-1) -- auto-init
    //, f_must_revalidate(false) -- auto-init
    //, f_no_cache(false) -- auto-init
    //, f_no_store(false) -- auto-init
    //, f_no_transform(false) -- auto-init
    //, f_only_if_cached(false) -- auto-init
    //, f_private(false) -- auto-init
    //, f_public(false) -- auto-init
    //, f_s_maxage(-1) -- auto-init
{
}


/** \brief Initializes a cache control object with the specified info.
 *
 * This function initialize this cache control object with the defaults
 * and then applies the \p info parameters to the controls.
 *
 * \param[in] info  The new cache control information to assign to this object.
 * \param[in] internal_setup  Whether to allow our special '!' syntax.
 */
cache_control_settings::cache_control_settings(QString const & info, bool const internal_setup)
    //: f_max_age(0) -- auto-init
    //, f_max_stale(-1) -- auto-init
    //, f_min_fresh(-1) -- auto-init
    //, f_must_revalidate(false) -- auto-init
    //, f_no_cache(false) -- auto-init
    //, f_no_store(false) -- auto-init
    //, f_no_transform(false) -- auto-init
    //, f_only_if_cached(false) -- auto-init
    //, f_private(false) -- auto-init
    //, f_public(false) -- auto-init
    //, f_s_maxage(-1) -- auto-init
{
    set_cache_info(info, internal_setup);
}


/** \brief Reset all the cache information to their defaults.
 *
 * This function resets all the cache information to their defaults so
 * the object looks as if it just got initialized.
 */
void cache_control_settings::reset_cache_info()
{
    f_max_age = 0;
    f_max_stale = -1;
    f_min_fresh = -1;
    f_must_revalidate = true;
    f_no_cache = false;
    f_no_store = true;
    f_no_transform = false;
    f_only_if_cached = false;
    f_private = false;
    f_public = false;
    f_s_maxage = -1;
}


/** \brief Set the cache information parsed from the \p info parameter.
 *
 * This function parses the \p info string for new cache definitions.
 * The \p info string can be empty in which case nothing is modified.
 *
 * If you want to start from scratch, you may call reset_cache_info()
 * first.
 *
 * The 'must-revalidate' and 'no-store' are set by default. Unfortunately
 * that would mean the page setup capability would not be able to ever
 * clear those two flags. That would mean you could never use a full
 * permanent cache definition. Instead we offer an extension to the
 * flags and allow one to add a '!' in front of the names as in:
 * '!no-cache'. This way you can force the 'no-cache' flag to false
 * instead of the default of true.
 *
 * \todo
 * Determine whether any error in the field should be considered fatal and
 * thus react by ignoring the entire \p info parameter. This is not a bad
 * idea, only if there are not parameters that we do not understand,
 * it could mean that we skip on others that we do understand.
 *
 * \todo
 * Determine whether we should accept certain parameters only once.
 * Especially those that include values (i.e. max-age=123) because
 * the current implementation only takes the last one in account when
 * we probably should remember the first one (within one \p info string).
 *
 * \todo
 * Add support for private and no-cache parameters (server side only).
 *
 * \param[in] info  The new cache control information to assign to this object.
 * \param[in] internal_setup  Whether to allow our special '!' syntax.
 */
void cache_control_settings::set_cache_info(QString const & info, bool const internal_setup)
{
    // TODO: we want the weighted HTTP strings to understand
    //       parameters with values assigned and more than the
    //       q=xxx then we can update this code to better test
    //       what we are dealing with...

    // parse the data with the weighted HTTP string implementation
    http_strings::WeightedHttpString client_cache_control(info);

    // no go through the list of parts and handle them appropriately
    http_strings::WeightedHttpString::part_t::vector_t const & cache_control_parts(client_cache_control.get_parts());
    for(auto c : cache_control_parts)
    {
        // get the part name
        QString name(c.get_name());
        if(name.isEmpty())
        {
            continue;
        }

        bool negate(false);
        if(internal_setup)
        {
            negate = name[0].unicode() == '!';
            if(negate)
            {
                // remove the negate flag
                name = name.mid(1);
            }
        }

        // TODO: add code to check whether 'negate' (!) was used with
        //       an item that does not support it

        // do a switch first to save a tad bit of time
        switch(name[0].unicode())
        {
        case 'm':
            if(name.startsWith("max-age="))
            {
                set_max_age(name.mid(8));
            }
            else if(name == "max-stale")
            {
                // any good ol' stale data can be returned
                set_max_stale(0);
            }
            else if(name.startsWith("max-stale="))
            {
                set_max_stale(name.mid(10));
            }
            else if(name.startsWith("min-fresh="))
            {
                set_min_fresh(name.mid(10));
            }
            else if(name == "must-revalidate")
            {
                set_must_revalidate(!negate);
            }
            break;

        case 'n':
            if(name == "no-cache")
            {
                // TODO: add support for field specific caching selection
                //       (i.e. no-cache=secret-key)
                set_no_cache(!negate);
            }
            else if(name == "no-store")
            {
                set_no_store(!negate);
            }
            else if(name == "no-transform")
            {
                set_no_transform(!negate);
            }
            break;

        case 'o':
            if(name == "only-if-cached")
            {
                set_only_if_cached(!negate);
            }
            break;

        case 'p':
            if(name == "private")
            {
                // TODO: add support for field specific caching selection
                //       (i.e. private=secret-key)
                set_private(!negate);
            }
            else if(name == "proxy-revalidate")
            {
                set_proxy_revalidate(!negate);
            }
            else if(name == "public")
            {
                set_public(!negate);
            }
            break;

        case 's':
            if(name.startsWith("s-maxage="))
            {
                set_s_maxage(name.mid(9));
            }
            break;

        }
    }
}


/** \brief Set the must-revalidate to true or false.
 *
 * This function should only be called with 'true' to request
 * that the client revalidate the data each time it wants to
 * access it.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \param[in] must_revalidate  Whether the client should revalidate that
 *                             specific content.
 */
void cache_control_settings::set_must_revalidate(bool must_revalidate)
{
    f_must_revalidate = must_revalidate;
}


/** \brief Get the current value of the must-revalidate flag.
 *
 * This function returns the current value of the must-revalidate
 * flag.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \return true if the must-revalidate flag is to be specified in the
 *         Cache-Control field.
 */
bool cache_control_settings::get_must_revalidate() const
{
    return f_must_revalidate;
}


/** \brief Set the 'private' flag to true or false.
 *
 * Any page that is private, and thus should not be saved in a
 * shared cache, must be assigned the private flag, so this function
 * must be called with true.
 *
 * Note that does not encrypt the data in any way. It just adds
 * the private flag to the Cache-Control header. If you need to
 * encrypt the data, make sure to enforce HTTPS before return a
 * reply with secret data.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \param[in] private_cache  Whether the client should revalidate that
 *                           specific content.
 */
void cache_control_settings::set_private(bool private_cache)
{
    f_private = private_cache;
}


/** \brief Get the current value of the 'private' flag.
 *
 * This function returns the current value of the 'private'
 * flag.
 *
 * Note that 'private' has priority over 'public'. So if 'private'
 * is true, 'public' is ignored. For this reason you should only
 * set those flags to true and never attempt to reset them to
 * false. Similarly, the 'no-cache' and 'no-store' have priority
 * over the 'private' flag.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \return true if the must-revalidate flag is to be specified in the
 *         Cache-Control field.
 */
bool cache_control_settings::get_private() const
{
    return f_private;
}


/** \brief Set the proxy-revalidate to true or false.
 *
 * This function should only be called with 'true' to request
 * that proxy caches revalidate the data each time a client
 * asks for the data.
 *
 * You may instead want to use the 's-maxage' field.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \param[in] proxy_revalidate  Whether the client should revalidate that
 *                              specific content.
 */
void cache_control_settings::set_proxy_revalidate(bool proxy_revalidate)
{
    f_proxy_revalidate = proxy_revalidate;
}


/** \brief Get the current value of the proxy-revalidate flag.
 *
 * This function returns the current value of the proxy-revalidate
 * flag.
 *
 * Note that 'must-revalidate' has priority and if specified, the
 * 'proxy-revalidate' is ignored since the proxy-cache should
 * honor the 'must-revalidate' anyway.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \return true if the proxy-revalidate flag is to be specified in the
 *         Cache-Control field.
 */
bool cache_control_settings::get_proxy_revalidate() const
{
    return f_proxy_revalidate;
}


/** \brief Set the 'public' flag to true or false.
 *
 * Any page that is public, and thus can be saved in a public shared
 * cache.
 *
 * Snap! detects whether a page is accessible by a visitor, if so
 * it sets the public flag automatically. So you should not have
 * to set this flag unless somehow your page is public and the Snap!
 * test could fail or you know that your pages are always public
 * and thus you could avoid having to check the permissions.
 *
 * Note that if the 'private' flag is set to true, then the 'public'
 * flag is ignored. Further, if the 'no-cache' or 'no-store' flags
 * are set, then 'public' and 'private' are ignored.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \param[in] public_cache  Whether the client should revalidate that
 *                             specific content.
 */
void cache_control_settings::set_public(bool public_cache)
{
    f_public = public_cache;
}


/** \brief Get the current value of the 'public' flag.
 *
 * This function returns the current value of the 'public'
 * flag.
 *
 * Note that 'private' has priority over 'public'. So if 'private'
 * is true, 'public' is ignored. Similarly, the 'no-cache' and
 * 'no-store' have priority over the 'private' flag.
 *
 * \note
 * This flag may appear in the server response.
 *
 * \return true if the must-revalidate flag is to be specified in the
 *         Cache-Control field.
 */
bool cache_control_settings::get_public() const
{
    return f_public;
}


/** \brief Set the maximum number of seconds to cache this data.
 *
 * This function requests for the specified data to be cached
 * for that many seconds.
 *
 * The special value of -1 will be taken as: use the maximum
 * amount of time that can specified in 'max-age' (which for
 * HTTP/1.1 is one year).
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \param[in] max_age  The maximum age the data can have before checking
 *                     the data again.
 */
void cache_control_settings::set_max_age(int64_t max_age)
{
    if(max_age == -1 || max_age > AGE_MAXIMUM)
    {
        // 1 year in seconds
        f_max_age = AGE_MAXIMUM;
    }
    else if(max_age < 0)
    {
        f_max_age = -1;
    }
    else
    {
        f_max_age = max_age;
    }
}


/** \brief Set the 'max-age' field value from a string.
 *
 * This function accepts a string as input to setup the 'max-age'
 * field.
 */
void cache_control_settings::set_max_age(QString const & max_age)
{
    // in this case -1 is what we want in case of an error
    // and not 1 year in seconds... no other negative values
    // are possible so we are fine
    f_max_age = string_to_seconds(max_age);
}


/** \brief Update the maximum number of seconds to cache this data.
 *
 * This function keeps the smaller maximum of the existing setup and
 * the new value specified to this function.
 *
 * If you set \p max_age to -1 then the maximum age is used. Any
 * other negative value are ignored.
 *
 * \param[in] max_age  The maximum age the data can have before checking
 *                     the data again.
 */
void cache_control_settings::update_max_age(int64_t max_age)
{
    if(max_age == -1 || max_age > AGE_MAXIMUM)
    {
        // 1 year in seconds
        max_age = AGE_MAXIMUM;
    }
    if(max_age >= 0
    && (f_max_age == -1 || max_age < f_max_age))
    {
        f_max_age = max_age;
    }
}


/** \brief Retrieve the current max-age field.
 *
 * The Cache-Control can specify how long the data being returned
 * can be cached for. The 'max-age' field defines that duration
 * in seconds.
 *
 * By default the data is marked as 'do not cache' (i.e. max-age
 * is set to zero.)
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \return The number of seconds to cache the data, or -1 to use
 *         the maximum possible value of max-age.
 */
int64_t cache_control_settings::get_max_age() const
{
    return f_max_age;
}


/** \brief Set the no-cache to true or false.
 *
 * This function should only be called with 'true' to request
 * that the client and intermediate caches do not cache any
 * of the data. This does not prevent the client to store
 * the data.
 *
 * When the client sets this field to true, it means that we
 * should regenerate the specified page data.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \param[in] no_cache  Whether the client and intermediate caches
 *                      must not cache the data.
 */
void cache_control_settings::set_no_cache(bool no_cache)
{
    f_no_cache = no_cache;
}


/** \brief Retrieve the no-cache flag.
 *
 * This function returns the current value of the no-cache flag.
 *
 * The system ignores the 'public' and 'private' flags when the
 * no-cache flag is true.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \return true if the no-cache flag is to be specified in the
 *         Cache-Control field.
 */
bool cache_control_settings::get_no_cache() const
{
    return f_no_cache;
}


/** \brief Set the no-store field to true or false.
 *
 * This function sets the no-store field to true or false. This
 * flag means that any of the data in that request needs to be
 * transferred only and not stored anywhere except in temporary
 * buffers on the client's machine.
 *
 * Further, shared caches should clear all the data bufferized
 * to process this request as soon as it is done with it.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \param[in] no_store  Whether the data can be stored or not.
 */
void cache_control_settings::set_no_store(bool no_store)
{
    f_no_store = no_store;
}


/** \brief Retrieve the no-store flag.
 *
 * This function returns the current no-store flag status.
 *
 * In most cases, this flag is not required. It should be true
 * only on pages that include extremely secure content such
 * as a page recording the settings of an electronic payment
 * (i.e. the e-payment Paypal page allows you to enter your
 * Paypal identifiers and those should not be stored anywhere.)
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \return true if the data in this request should not be stored
 *         anywhere.
 */
bool cache_control_settings::get_no_store() const
{
    return f_no_store;
}


/** \brief Set whether the data can be transformed.
 *
 * The 'no-transform' flag can be used to make sure that caches do
 * not transform the data. This can also appear in the request from
 * the client in which case an exact original is required.
 *
 * This is generally important only for document files that may be
 * converted to a lousy format such as images that could be saved
 * as JPEG images.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \param[in] no_transform  Set to true to retrieve the original document.
 */
void cache_control_settings::set_no_transform(bool no_transform)
{
    f_no_transform = no_transform;
}


/** \brief Retrieve whether the data can be transformed.
 *
 * Check whether the client or the server are requesting that the data
 * not be transformed. If true, then the original data should be
 * transferred.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \return true i the 'no-transform' flag is set.
 */
bool cache_control_settings::get_no_transform() const
{
    return f_no_transform;
}


/** \brief Set the number of seconds to cache this data in shared caches.
 *
 * This function requests for the specified data to be cached
 * for that many seconds in any shared caches between the client
 * and the server. The client ignores that information.
 *
 * The special value of -1 will be taken as: use the maximum
 * amount of time that can specified in 's-maxage' (which for
 * HTTP/1.1 is one year).
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \param[in] s_maxage  The maximum age the data can have before checking
 *                      the source again.
 */
void cache_control_settings::set_s_maxage(int64_t s_maxage)
{
    if(s_maxage == -1 || s_maxage > AGE_MAXIMUM)
    {
        f_s_maxage = AGE_MAXIMUM;
    }
    else if(s_maxage < 0)
    {
        f_s_maxage = -1;
    }
    else
    {
        f_s_maxage = s_maxage;
    }
}


/** \brief Update the maximum number of seconds to cache this data on proxies.
 *
 * This function keeps the smaller maximum of the existing setup and
 * the new value specified to this function.
 *
 * If you set \p s_maxage to -1 then the maximum age is used. Any
 * other negative value are ignored.
 *
 * \param[in] s_maxage  The maximum age the data can have before checking
 *                      the data again from shared proxy cache.
 */
void cache_control_settings::update_s_maxage(int64_t s_maxage)
{
    if(s_maxage == -1 || s_maxage > AGE_MAXIMUM)
    {
        // 1 year in seconds
        s_maxage = AGE_MAXIMUM;
    }
    if(s_maxage >= 0
    && (f_s_maxage == -1 || s_maxage < f_s_maxage))
    {
        f_s_maxage = s_maxage;
    }
}


/** \brief Set the 's-maxage' field value from a string.
 *
 * This function accepts a string as input to setup the 's-maxage'
 * field.
 */
void cache_control_settings::set_s_maxage(QString const & s_maxage)
{
    // in this case -1 is what we want in case of an error
    // and not 1 year in seconds... no other negative values
    // are possible so we are fine
    f_s_maxage = string_to_seconds(s_maxage);
}


/** \brief Retrieve the current 's-maxage' field.
 *
 * The Cache-Control can specify how long the data being returned
 * can be cached for in a shared cache. The 's-maxage' field defines
 * that duration in seconds.
 *
 * By default shared caches are expected to use the 'max-age' parameter
 * when the 's-maxage' parameter is not defined. So if the value is
 * the same, you do not have to specified 's-maxage'.
 *
 * \note
 * This flag may appear in the client request or the server response.
 *
 * \return The number of seconds to cache the data, or -1 to use
 *         the maximum possible value of s-maxage.
 */
int64_t cache_control_settings::get_s_maxage() const
{
    return f_s_maxage;
}


/** \brief How long of a stale is accepted by the client.
 *
 * The client may asks for data that is stale. Assuming that
 * a cache may keep data after it stale, the client may
 * retrieve that data if they specified the stale parameter.
 *
 * A value of zero means that any stale data is acceptable.
 * A greate value specifies the number of seconds after the
 * normal cache threshold the data can be to be considered
 * okay to be returned to the client.
 *
 * In general, this is for cache systems and not the server
 * so our server generally ignores that data.
 *
 * \note
 * This flag may appear in the client request.
 *
 * \param[in] max_stale  The maximum number of seconds for the stale
 *                       data to be before the request is forwarded
 *                       to the server.
 */
void cache_control_settings::set_max_stale(int64_t max_stale)
{
    if(max_stale > AGE_MAXIMUM)
    {
        f_max_stale = AGE_MAXIMUM;
    }
    else if(max_stale < 0)
    {
        f_max_stale = -1;
    }
    else
    {
        f_max_stale = max_stale;
    }
}


/** \brief Set the 'max-stale' field value from a string.
 *
 * This function accepts a string as input to setup the 'max-stale'
 * field.
 */
void cache_control_settings::set_max_stale(QString const & max_stale)
{
    set_max_stale(string_to_seconds(max_stale));
}


/** \brief Retrieve the current maximum stale value.
 *
 * This function returns the maximum number of seconds the client is
 * will to accept after the cache expiration date.
 *
 * So if your cache expires at 14:30:00 and the user makes a new
 * request on 14:32:50 with a 'max-stale' value of 3600, then you
 * may return the stale cache instead of regenerating it.
 *
 * The stale value may be set to zero in which case the cache is
 * always returned if available.
 *
 * \note
 * This flag may appear in the client request.
 *
 * \return The number of seconds the cache may be stale by before
 *         regenerating it.
 */
int64_t cache_control_settings::get_max_stale() const
{
    return f_max_stale;
}


/** \brief Set the number of seconds of freshness required by the client.
 *
 * The freshness is the amount of seconds before the cache goes stale.
 * So if the cache goes stale in 60 seconds and the freshness query
 * is 3600, then the cache is ignored.
 *
 * Note that freshness cannot always be satisfied since a page cache
 * duration ('max-age') may always be smaller than the specified
 * freshness amount.
 *
 * \note
 * This flag may appear in the client request.
 *
 * \param[in] min_fresh  The minimum freshness in seconds.
 */
void cache_control_settings::set_min_fresh(int64_t min_fresh)
{
    if(min_fresh > AGE_MAXIMUM)
    {
        f_min_fresh = AGE_MAXIMUM;
    }
    else if(min_fresh < 0)
    {
        f_min_fresh = -1;
    }
    else
    {
        f_min_fresh = min_fresh;
    }
}


/** \brief Set the 'min-fresh' field value from a string.
 *
 * This function accepts a string as input to setup the 'min-fresh'
 * field.
 */
void cache_control_settings::set_min_fresh(QString const & min_fresh)
{
    set_min_fresh(string_to_seconds(min_fresh));
}


/** \brief Retrieve the 'min-fresh' value from the Cache-Control.
 *
 * If the cache is to get stale within less than 'min-fresh' then
 * the server is expected to recalculate the page.
 *
 * Pages that are given a 'max-age' of less than what 'min-fresh'
 * is set at will react as fully dynamic pages (i.e. as if no
 * caches were available.)
 *
 * \note
 * This flag may appear in the client request.
 *
 * \return The number of seconds of freshness that are required.
 */
int64_t cache_control_settings::get_min_fresh() const
{
    return f_min_fresh;
}


/** \brief Set the 'only-if-cached' flag.
 *
 * The 'only-if-cached' flag is used by clients with poor network
 * connectivity to request any available data from any cache instead
 * of gettings newer data.
 *
 * The server ignores that flag since the user connected to the server
 * it would not make sense to not return a valid response from that
 * point.
 *
 * \note
 * This flag may appear in the client request.
 *
 * \param[in] only_if_cached  The new flag value.
 */
void cache_control_settings::set_only_if_cached(bool only_if_cached)
{
    f_only_if_cached = only_if_cached;
}


/** \brief Retrieve the 'only-if-cached' flag.
 *
 * This function returns the current status of the 'only-if-cached' flag.
 *
 * The server ignores this flag since it is generally used so a client
 * can request all in between caches to return any data they have
 * available, instead of trying to reconnect to the server. However,
 * it may check the flag and if true change the behavior. Yet, that
 * would mean the cache behavior would change for all clients.
 *
 * Note that caches still don't return stale data unless the client
 * also specifies the max-stale parameter.
 *
 * \code
 *      Cache-Control: max-stale=0,only-if-cached
 * \endcode
 *
 * \return Return true if the only-if-cached flag was specified in the
 *         request.
 */
bool cache_control_settings::get_only_if_cached() const
{
    return f_only_if_cached;
}


/** \brief Convert a string to a number.
 *
 * This function returns a number as defined in a string. The input
 * string must exclusively be composed of decimal digits.
 *
 * No minus sign is allowed. If any character is not valid, then the
 * function returns -1.
 *
 * \return The number of seconds the string represents or -1 on errors.
 */
int64_t cache_control_settings::string_to_seconds(QString const & max_age)
{
    if(max_age.isEmpty())
    {
        // undefined / invalid
        return -1;
    }

    char const * s(max_age.toUtf8().data());
    char const * start(s);
    int64_t result(0);
    for(; *s != '\0'; ++s)
    {
        if(*s < '0'
        || *s > '9'
        || s - start > 9) // too large! (for 1 year in seconds we need 8 digits maximum)
        {
            // invalid
            return -1;
        }
        result = result * 10 + *s - '0';
    }

    return result > AGE_MAXIMUM ? AGE_MAXIMUM : result;
}


/** \brief Retrieve the smallest value of two.
 *
 * This special minimum function returns the smallest of two values,
 * only if one of those values is -1, then it is ignored.
 *
 * \note
 * This function is expected to be used with the 'max-age' and
 * 's-maxage' numbers. These numbers are expected to be defined
 * between -1 and AGE_MAXIMUM. Although -1 is <em>ignored</em>.
 *
 * \param[in] a  The left hand side value.
 * \param[in] b  The right hand side value.
 *
 * \return The smallest of a and b, ignoring either if -1.
 */
int64_t cache_control_settings::minimum(int64_t a, int64_t b)
{
    // if a or b are -1, then return the other
    if(a == -1)
    {
        // in this case the value may return -1.
        return b;
    }
    if(b == -1)
    {
        return a;
    }

    // normal std::min() otherwise
    return std::min(a, b);
}


} // namespace snap

// vim: ts=4 sw=4 et
