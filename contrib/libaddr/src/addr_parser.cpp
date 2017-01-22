// Network Address -- classes functions to ease handling IP addresses
// Copyright (C) 2012-2017  Made to Order Software Corp.
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

// self
//
#include "libaddr/addr.h"

// C++ lib
//
#include <algorithm>
#include <sstream>
#include <iostream>

// C lib
//
#include <ifaddrs.h>
#include <netdb.h>



namespace addr
{


namespace
{


/** \brief Delete an addrinfo structure.
 *
 * This deleter is used to make sure all the addinfo get released when
 * an exception occurs or the function using such exists.
 *
 * \param[in] ai  The addrinfo structure to free.
 */
void addrinfo_deleter(struct addrinfo * ai)
{
    freeaddrinfo(ai);
}


}





void addr_parser::set_default_address(std::string const & addr)
{
    f_default_address = addr;
}


std::string const & addr_parser::get_default_address() const
{
    return f_default_address;
}


/** \brief Define the default port.
 *
 * This function is used to define the default port to use in the address
 * parser object. By default this is set to -1 meaning: no default port.
 *
 * This function accepts any port number from 0 to 65535. It also accepts
 * -1 to reset the port back to "no default".
 *
 * \param[in] port  The new default port.
 */
void addr_parser::set_default_port(int const port)
{
    if(port < -1
    || port > 65535)
    {
        throw addr_invalid_argument_exception("addr_parser::set_default_port(): port must be in range [-1..65535].");
    }

    f_default_port = port;
}


int addr_parser::get_default_port() const
{
    return f_default_port;
}


void addr_parser::set_default_mask(std::string const & mask)
{
    f_default_mask = mask;
}


std::string const & addr_parser::get_default_mask() const
{
    return f_default_mask;
}


void addr_parser::set_protocol(std::string const & protocol)
{
    if(protocol == "ip")
    {
        f_protocol = IPPROTO_IP;
    }
    else if(protocol == "tcp")
    {
        f_protocol = IPPROTO_TCP;
    }
    else if(protocol == "udp")
    {
        f_protocol = IPPROTO_UDP;
    }
    else
    {
        // not a protocol we support
        //
        throw addr_invalid_argument_exception(
                  std::string("unknown protocol \"")
                + protocol
                + "\", expected \"tcp\" or \"udp\".");
    }
}


void addr_parser::set_protocol(int const protocol)
{
    // make sure that's a protocol we support
    //
    switch(protocol)
    {
    case IPPROTO_IP:
    case IPPROTO_TCP:
    case IPPROTO_UDP:
        break;

    default:
        throw addr_invalid_argument_exception(
                  std::string("unknown protocol \"")
                + std::to_string(protocol)
                + "\", expected \"tcp\" or \"udp\".");

    }

    f_protocol = protocol;
}


/** \brief Use this function to reset the protocol back to "no default."
 *
 * This function sets the protocol to -1 (which is something you cannot
 * do by callingt he set_protocol() functions above.)
 *
 * The -1 special value means that the protocol is not defined, that
 * there is no default. In most cases this means all the addresses
 * that match, ignoring the protocol, will be returned by the parse()
 * function.
 */
void addr_parser::clear_protocol()
{
    f_protocol = -1;
}


int addr_parser::get_protocol() const
{
    return f_protocol;
}


/** \brief Set or clear allow flags in the parser.
 *
 * This parser has a set of flags it uses to know whether the input
 * string can include certain things such as a port or a mask.
 *
 * This function is used to allow or require certain parameters and
 * to disallow others.
 *
 * By default, the ADDRESS and PORT flags are set, meaning that an
 * address and a port can appear, but either or both are optinal.
 * If unspecified, then the default will be used. If not default
 * is defined, then the parser may fail in this situation.
 *
 * One problem is that we include contradictory syntatical features.
 * The parser supports lists of addresses separated by commas and
 * lists of ports separated by commas. Both are not supported
 * simultaneously. This means you want to allow multiple addresses
 * separated by commas, the function makes sure that the multiple
 * port separated by commas support is turned of.
 *
 * \li ADDRESS -- the IP address is allowed, but optional
 * \li REQUIRED_ADDRESS -- the IP address is mandatory
 * \li PORT -- the port is allowed, but optional
 * \li REQUIRED_PORT -- the port is mandatory
 * \li MASK -- the mask is allowed, but optional
 * \li MULTI_ADDRESSES_COMMAS -- the input can have multiple addresses
 * separated by commas, spaces are not allowed (prevents MULTI_PORTS_COMMAS)
 * \li MULTI_ADDRESSES_SPACES -- the input can have multiple addresses
 * separated by spaces
 * \li MULTI_ADDRESSES_COMMAS_AND_SPACES -- the input can have multiple
 * addresses separated by spaces and commas (prevents MULTI_PORTS_COMMAS)
 * \li MULTI_PORTS_SEMICOLONS -- the input can  have multiple ports
 * separated by semicolons NOT IMPLEMENTED YET
 * \li MULTI_PORTS_COMMAS -- the input can have multiple ports separated
 * by commas (prevents MULTI_ADDRESSES_COMMAS and
 * MULTI_ADDRESSES_COMMAS_AND_SPACES) NOT IMPLEMENTED YET
 * \li PORT_RANGE -- the input supports port ranges (p1-p2) NOT
 * IMPLEMENTED YET
 * \li ADDRESS_RANGE -- the input supports address ranges (addr-addr) NOT
 * IMPLEMENTED YET
 *
 * \param[in] flag  The flag to set or clear.
 * \param[in] allow  Whether to allow (true) or disallow (false).
 *
 * \sa get_allow()
 */
void addr_parser::set_allow(flag_t const flag, bool const allow)
{
    if(flag < static_cast<flag_t>(0)
    || flag >= flag_t::FLAG_max)
    {
        throw addr_invalid_argument_exception("addr_parser::set_allow(): flag has to be one of the valid flags.");
    }

    f_flags[static_cast<int>(flag)] = allow;

    // if we just set a certain flag, others may need to go to false
    //
    if(allow)
    {
        // we can only support one type of commas
        //
        switch(flag)
        {
        case flag_t::MULTI_ADDRESSES_COMMAS:
        case flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES:
            f_flags[static_cast<int>(flag_t::MULTI_PORTS_COMMAS)] = false;
            break;

        case flag_t::MULTI_PORTS_COMMAS:
            f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_COMMAS)] = false;
            f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES)] = false;
            break;

        default:
            break;

        }
    }
}


bool addr_parser::get_allow(flag_t const flag) const
{
    if(flag < static_cast<flag_t>(0)
    || flag >= flag_t::FLAG_max)
    {
        throw addr_invalid_argument_exception("addr_parser::get_allow(): flag has to be one of the valid flags.");
    }

    return f_flags[static_cast<int>(flag)];
}


bool addr_parser::has_errors() const
{
    return !f_error.empty();
}


void addr_parser::emit_error(std::string const & msg)
{
    f_error += msg;
    f_error += "\n";
    ++f_error_count;
}


std::string const & addr_parser::error_messages() const
{
    return f_error;
}


int addr_parser::error_count() const
{
    return f_error_count;
}


void addr_parser::clear_errors()
{
    f_error.clear();
    f_error_count = 0;
}


addr_range::vector_t addr_parser::parse(std::string const & in)
{
    addr_range::vector_t result;

    if(f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_COMMAS)]
    || f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_SPACES)])
    {
        char sep(f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_COMMAS)] ? ',' : ' ');
        std::string::size_type s(0);
        while(s < in.length())
        {
            std::string::size_type e(in.find(sep, s));
            if(e == std::string::npos)
            {
                e = in.length();
            }
            if(e > s)
            {
                parse_cidr(in.substr(s, e - s), result);
            }
            s = e + 1;
        }
    }
    else if(f_flags[static_cast<int>(flag_t::MULTI_ADDRESSES_COMMAS_AND_SPACES)])
    {
        std::string comma_space(", ");
        std::string::size_type s(0);
        while(s < in.length())
        {
            // since C++11 we have a way to search for a set of character
            // in a string with an algorithm!
            //
            auto const it(std::find_first_of(in.begin() + s, in.end(), comma_space.begin(), comma_space.end()));
            std::string::size_type const e(it == in.end() ? in.length() : it - in.begin());
            if(e > s)
            {
                parse_cidr(in.substr(s, e - s), result);
            }
            s = e + 1;
        }
    }
    else
    {
        parse_cidr(in, result);
    }

    return result;
}


/** \brief Check one address.
 *
 * This function checks one address, although if it is a name, it could
 * represent multiple IP addresses.
 *
 * This function separate the address:port from the mask if the mask is
 * allowed. Then it parses the address:port part and the mask separately.
 *
 * \param[in] in  The address to parse.
 * \param[in] result  The resulting list of addresses.
 */
void addr_parser::parse_cidr(std::string const & in, addr_range::vector_t & result)
{
    if(f_flags[static_cast<int>(flag_t::MASK)])
    {
        // check whether there is a mask
        //
        std::string mask(f_default_mask);

        std::string address;
        std::string::size_type const p(in.find('/'));
        if(p != std::string::npos)
        {
            address = in.substr(0, p);
            mask = in.substr(p + 1);
        }
        else
        {
            address = in;
        }
        if(mask.empty())
        {
            // mask not found, do as if none defined
            //
            parse_address(address, result);
        }
        else
        {
            int const errcnt(f_error_count);

            // handle the address first
            //
            addr_range::vector_t addr_mask;
            parse_address(address, addr_mask);

            // now check for the mask
            //
            for(auto & am : addr_mask)
            {
                parse_mask(mask, am.get_from());
            }

            // now append the list to the result if no errors occurred
            //
            if(errcnt == f_error_count)
            {
                result.insert(result.end(), addr_mask.begin(), addr_mask.end());
            }
        }
    }
    else
    {
        // no mask allowed, if there is one, this call will fail
        //
        parse_address(in, result);
    }
}


void addr_parser::parse_address(std::string const & in, addr_range::vector_t & result)
{
    // With our only supportted format, ipv6 addresses must be between square
    // brackets. The address may just be a mask in which case the '[' may
    // not be at the very start (i.e. "/[ffff:ffff::]")
    //
    if(in.find('[') != std::string::npos)
    {
        // IPv6 parsing
        //
        parse_address6(in, result);
    }
    else
    {
        parse_address4(in, result);
    }
}


void addr_parser::parse_address4(std::string const & in, addr_range::vector_t & result)
{
    std::string port_str(f_default_port == -1 ? std::string() : std::to_string(f_default_port));
    std::string address(f_default_address);

    std::string::size_type const p(in.find(':'));

    if(f_flags[static_cast<int>(flag_t::PORT)]
    || f_flags[static_cast<int>(flag_t::REQUIRED_PORT)])
    {
        // the address can include a port
        //
        if(p != std::string::npos)
        {
            // get the address only if not empty (otherwise we want to
            // keep the default)
            //
            if(p > 0)
            {
                address = in.substr(0, p);
            }

            // get the port only if not empty (otherwise we want to
            // keep the default)
            //
            if(p + 1 < in.length())
            {
                port_str = in.substr(p + 1);
            }
        }
        else if(!in.empty())
        {
            address = in;
        }
    }
    else
    {
        if(p != std::string::npos
        && !f_flags[static_cast<int>(flag_t::PORT)]
        && !f_flags[static_cast<int>(flag_t::REQUIRED_PORT)])
        {
            emit_error("Port not allowed (" + in + ").");
            return;
        }

        if(!in.empty())
        {
            address = in;
        }
    }

    parse_address_port(address, port_str, result);
}


void addr_parser::parse_address6(std::string const & in, addr_range::vector_t & result)
{
    std::string::size_type p(0);

    std::string address(f_default_address);
    std::string port_str(f_default_port == -1 ? std::string() : std::to_string(f_default_port));

    // if there is an address extract it otherwise put the default
    //
    if(!in.empty()
    && in[0] == '[')
    {
        p = in.find(']');

        if(p == std::string::npos)
        {
            emit_error("IPv6 is missing the ']' (" + in + ").");
            return;
        }

        if(p != 1)
        {
            // get the address only if not empty (otherwise we want to
            // keep the default) -- so we actually support "[]" to
            // represent "use the default address if defined".
            //
            address = in.substr(1, p - 1);
        }
    }

    // on entry 'p' is either 0 or the position of the ']' character
    //
    p = in.find(':', p);

    if(p != std::string::npos)
    {
        if(f_flags[static_cast<int>(flag_t::PORT)]
        || f_flags[static_cast<int>(flag_t::REQUIRED_PORT)])
        {
            // there is also a port, extract it
            //
            port_str = in.substr(p + 1);
        }
        else if(!f_flags[static_cast<int>(flag_t::PORT)]
             && !f_flags[static_cast<int>(flag_t::REQUIRED_PORT)])
        {
            emit_error("Port not allowed (" + in + ").");
            return;
        }
    }

    parse_address_port(address, port_str, result);
}


void addr_parser::parse_address_port(std::string const & address, std::string const & port_str, addr_range::vector_t & result)
{
    // make sure the port is good
    //
    if(port_str.empty()
    && f_flags[static_cast<int>(flag_t::REQUIRED_PORT)])
    {
        emit_error("Required port is missing.");
        return;
    }

    // make sure the address is good
    //
    if(address.empty()
    && f_flags[static_cast<int>(flag_t::REQUIRED_ADDRESS)])
    {
        emit_error("Required address is missing.");
        return;
    }

    // prepare hints for the the getaddrinfo() function
    //
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG | AI_V4MAPPED;
    hints.ai_family = AF_UNSPEC;

    switch(f_protocol)
    {
    case IPPROTO_TCP:
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        break;

    case IPPROTO_UDP:
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
        break;

    }

    // convert address to binary
    //
    struct addrinfo * addrlist(nullptr);
    {
        errno = 0;
        int const r(getaddrinfo(address.c_str(), port_str.c_str(), &hints, &addrlist));
        if(r != 0)
        {
            // break on invalid addresses
            //
            int const e(errno); // if r == EAI_SYSTEM, then 'errno' is consistent here
            emit_error("Invalid address in \""
                     + address
                     + (port_str.empty() ? "" : ":")
                     + port_str
                     + "\" error "
                     + std::to_string(r)
                     + " -- "
                     + gai_strerror(r)
                     + " (errno: "
                     + std::to_string(e)
                     + " -- "
                     + strerror(e)
                     + ").");
            return;
        }
    }
    std::shared_ptr<struct addrinfo> ai(addrlist, addrinfo_deleter);

    bool first(true);
    while(addrlist != nullptr)
    {
        // go through the addresses and create ranges and save that in the result
        //
        if(addrlist->ai_family == AF_INET)
        {
            if(addrlist->ai_addrlen != sizeof(struct sockaddr_in))
            {
                emit_error("Unsupported address size ("                  // LCOV_EXCL_LINE
                         + std::to_string(addrlist->ai_addrlen)          // LCOV_EXCL_LINE
                         + ", expected"                                  // LCOV_EXCL_LINE
                         + std::to_string(sizeof(struct sockaddr_in))    // LCOV_EXCL_LINE
                         + ").");                                        // LCOV_EXCL_LINE
            }
            else
            {
                addr a(*reinterpret_cast<struct sockaddr_in *>(addrlist->ai_addr));
                // in most cases we do not get a protocol from
                // the getaddrinfo() function...
                if(addrlist->ai_protocol != 0)
                {
                    a.set_protocol(addrlist->ai_protocol);
                }
                addr_range r;
                r.set_from(a);
                result.push_back(r);
            }
        }
        else if(addrlist->ai_family == AF_INET6)
        {
            if(addrlist->ai_addrlen != sizeof(struct sockaddr_in6))
            {
                emit_error("Unsupported address size ("                  // LCOV_EXCL_LINE
                         + std::to_string(addrlist->ai_addrlen)          // LCOV_EXCL_LINE
                         + ", expected "                                 // LCOV_EXCL_LINE
                         + std::to_string(sizeof(struct sockaddr_in6))   // LCOV_EXCL_LINE
                         + ").");                                        // LCOV_EXCL_LINE
            }
            else
            {
                addr a(*reinterpret_cast<struct sockaddr_in6 *>(addrlist->ai_addr));
                a.set_protocol(addrlist->ai_protocol);
                addr_range r;
                r.set_from(a);
                result.push_back(r);
            }
        }
        else if(first)                                                  // LCOV_EXCL_LINE
        {
            // ignore errors from further addresses
            //
            emit_error("Unsupported address family "                     // LCOV_EXCL_LINE
                     + std::to_string(addrlist->ai_family)               // LCOV_EXCL_LINE
                     + ".");                                             // LCOV_EXCL_LINE
        }

        first = false;

        addrlist = addrlist->ai_next;
    }
}


/** \brief Parse a mask.
 *
 * If the input string is a decimal number, then use that as the
 * number of bits to clear.
 *
 * If the mask is not just one decimal number, try to convert it
 * as an address.
 *
 * If the string is neither a decimal number nor a valid IP address
 * then the parser adds an error string to the f_error variable.
 *
 * \param[in] mask  The mask to transform to binary.
 * \param[in,out] cidr  The address to which the mask will be added.
 */
void addr_parser::parse_mask(std::string const & mask, addr & cidr)
{
    // no mask?
    //
    if(mask.empty())
    {
        // in the current implementation this cannot happen since we
        // do not call this function when mask is empty
        //
        // hwoever, the algorithm below expect that 'mask' is not
        // empty (otherwise we get the case of 0 even though it
        // may not be correct.)
        //
        return;  // LCOV_EXCL_LINE
    }

    // the mask may be a decimal number or an address, if just one number
    // then it's not an address, so test that first
    //
    uint8_t mask_bits[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };

    // convert the mask to an integer, if possible
    //
    int mask_count(0);
    {
        for(char const * s(mask.c_str()); *s != '\0'; ++s)
        {
            if(*s >= '0' && *s <= '9')
            {
                mask_count = mask_count * 10 + *s - '0';
                if(mask_count > 1000)
                {
                    emit_error("Mask number too large ("
                             + mask
                             + ", expected a maximum of 128).");
                    return;
                }
            }
            else
            {
                mask_count = -1;
                break;
            }
        }
    }

    // the conversion to an integer worked if mask_count != -1
    //
    if(mask_count != -1)
    {
        if(cidr.is_ipv4())
        {
            if(mask_count > 32)
            {
                emit_error("Unsupported mask size ("
                         + std::to_string(mask_count)
                         + ", expected 32 at the most for an IPv4).");
                return;
            }
            mask_count = 32 - mask_count;
        }
        else
        {
            if(mask_count > 128)
            {
                emit_error("Unsupported mask size ("
                         + std::to_string(mask_count)
                         + ", expected 128 at the most for an IPv6).");
                return;
            }
            mask_count = 128 - mask_count;
        }

        // clear a few bits at the bottom of mask_bits
        //
        int idx(15);
        for(; mask_count > 8; mask_count -= 8, --idx)
        {
            mask_bits[idx] = 0;
        }
        mask_bits[idx] = 255 << mask_count;
    }
    else //if(mask_count < 0)
    {
        // prepare hints for the the getaddrinfo() function
        //
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_ADDRCONFIG | AI_V4MAPPED;
        hints.ai_family = AF_UNSPEC;

        switch(cidr.get_protocol())
        {
        case IPPROTO_TCP:
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            break;

        case IPPROTO_UDP:
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_protocol = IPPROTO_UDP;
            break;

        }

        std::string const port_str(std::to_string(cidr.get_port()));

        // if the mask is an IPv6, then it has to have the '[...]'
        std::string m(mask);
        if(cidr.is_ipv4())
        {
            if(mask[0] == '[')
            {
                emit_error("The address uses the IPv4 syntax, the mask cannot use IPv6.");
                return;
            }
        }
        else //if(!cidr.is_ipv4())
        {
            if(mask[0] != '[')
            {
                emit_error("The address uses the IPv6 syntax, the mask cannot use IPv4.");
                return;
            }
            if(mask.back() != ']')
            {
                emit_error("The IPv6 mask is missing the ']' (" + mask + ").");
                return;
            }

            // note that we know that mask.length() >= 2 here since
            // we at least have a '[' and ']'
            //
            m = mask.substr(1, mask.length() - 2);
            if(m.empty())
            {
                // an empty mask is valid, it just means keep the default
                // (getaddrinfo() fails on an empty string)
                //
                return;
            }
        }

        // if negative, we may have a full address here, so call the
        // getaddrinfo() on this other string
        //
        struct addrinfo * masklist(nullptr);
        errno = 0;
        int const r(getaddrinfo(m.c_str(), port_str.c_str(), &hints, &masklist));
        if(r != 0)
        {
            // break on invalid addresses
            //
            int const e(errno); // if r == EAI_SYSTEM, then 'errno' is consistent here
            emit_error("Invalid mask in \"/"
                     + mask
                     + "\", error "
                     + std::to_string(r)
                     + " -- "
                     + gai_strerror(r)
                     + " (errno: "
                     + std::to_string(e)
                     + " -- "
                     + strerror(e)
                     + ").");
            return;
        }
        std::shared_ptr<struct addrinfo> mask_ai(masklist, addrinfo_deleter);

        if(cidr.is_ipv4())
        {
            if(masklist->ai_family != AF_INET)
            {
                // this one happens when the user does not put the '[...]'
                // around an IPv6 address
                //
                emit_error("Incompatible address between the address and"
                          " mask address (first was an IPv4 second an IPv6).");
                return;
            }
            if(masklist->ai_addrlen != sizeof(struct sockaddr_in))
            {
                emit_error("Unsupported address size ("                 // LCOV_EXCL_LINE
                        + std::to_string(masklist->ai_addrlen)          // LCOV_EXCL_LINE
                        + ", expected"                                  // LCOV_EXCL_LINE
                        + std::to_string(sizeof(struct sockaddr_in))    // LCOV_EXCL_LINE
                        + ").");                                        // LCOV_EXCL_LINE
                return;                                                 // LCOV_EXCL_LINE
            }
            memcpy(mask_bits + 12, &reinterpret_cast<struct sockaddr_in *>(masklist->ai_addr)->sin_addr.s_addr, 4); // last 4 bytes are the IPv4 address, keep the rest as 1s
        }
        else //if(!cidr.is_ipv4())
        {
            if(masklist->ai_family != AF_INET6)
            {
                // this one happens if the user puts the '[...]'
                // around an IPv4 address
                //
                emit_error("Incompatible address between the address"
                          " and mask address (first was an IPv6 second an IPv4).");
                return;
            }
            if(masklist->ai_addrlen != sizeof(struct sockaddr_in6))
            {
                emit_error("Unsupported address size ("                 // LCOV_EXCL_LINE
                         + std::to_string(masklist->ai_addrlen)         // LCOV_EXCL_LINE
                         + ", expected "                                // LCOV_EXCL_LINE
                         + std::to_string(sizeof(struct sockaddr_in6))  // LCOV_EXCL_LINE
                         + ").");                                       // LCOV_EXCL_LINE
                return;                                                 // LCOV_EXCL_LINE
            }
            memcpy(mask_bits, &reinterpret_cast<struct sockaddr_in6 *>(masklist->ai_addr)->sin6_addr.s6_addr, 16);
        }
    }

    cidr.set_mask(mask_bits);
}








}
// snap_addr namespace
// vim: ts=4 sw=4 et
