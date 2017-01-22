// Network Address -- classes functions to ease handling IP addresses
// Copyright (C) 2012-2017  Made to Order Software Corp.
//
// http://snapwebsites.org/project/libaddr
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
#pragma once

#include "libexcept/exception.h"

#include <arpa/inet.h>

#include <memory>
#include <cstring>

namespace addr
{


class addr_invalid_argument_exception : public libexcept::exception_t
{
public:
    addr_invalid_argument_exception(char const *        what_msg) : exception_t(what_msg) {}
    addr_invalid_argument_exception(std::string const & what_msg) : exception_t(what_msg) {}
};

class addr_invalid_state_exception : public libexcept::exception_t
{
public:
    addr_invalid_state_exception(char const *        what_msg) : exception_t(what_msg) {}
    addr_invalid_state_exception(std::string const & what_msg) : exception_t(what_msg) {}
};

class addr_invalid_structure_exception : public libexcept::logic_exception_t
{
public:
    addr_invalid_structure_exception(char const *        what_msg) : logic_exception_t(what_msg) {}
    addr_invalid_structure_exception(std::string const & what_msg) : logic_exception_t(what_msg) {}
};

class addr_invalid_parameter_exception : public libexcept::logic_exception_t
{
public:
    addr_invalid_parameter_exception(char const *        what_msg) : logic_exception_t(what_msg) {}
    addr_invalid_parameter_exception(std::string const & what_msg) : logic_exception_t(what_msg) {}
};



constexpr struct sockaddr_in6 init_in6()
{
    struct sockaddr_in6 in6 = sockaddr_in6();
    in6.sin6_family = AF_INET6;
    return in6;
}


class addr
{
public:
    enum class network_type_t
    {
        NETWORK_TYPE_UNDEFINED,
        NETWORK_TYPE_PRIVATE,
        NETWORK_TYPE_CARRIER,
        NETWORK_TYPE_LINK_LOCAL,
        NETWORK_TYPE_MULTICAST,
        NETWORK_TYPE_LOOPBACK,
        NETWORK_TYPE_ANY,
        NETWORK_TYPE_UNKNOWN,
        NETWORK_TYPE_PUBLIC = NETWORK_TYPE_UNKNOWN  // we currently do not distinguish public and unknown
    };

    enum class computer_interface_address_t
    {
        COMPUTER_INTERFACE_ADDRESS_ERROR = -1,
        COMPUTER_INTERFACE_ADDRESS_FALSE,
        COMPUTER_INTERFACE_ADDRESS_TRUE
    };

    enum class string_ip_t
    {
        STRING_IP_ONLY,
        STRING_IP_BRACKETS,         // IPv6 only
        STRING_IP_PORT,
        STRING_IP_MASK,
        STRING_IP_BRACKETS_MASK,    // IPv6 only
        STRING_IP_ALL
    };

    typedef std::shared_ptr<addr>   pointer_t;
    typedef std::vector<addr>       vector_t;

                                    addr();
                                    addr(struct sockaddr_in const & in);
                                    addr(struct sockaddr_in6 const & in6);

    static vector_t                 get_local_addresses();

    void                            set_from_socket(int s);
    void                            set_ipv4(struct sockaddr_in const & in);
    void                            set_ipv6(struct sockaddr_in6 const & in6);
    void                            set_port(int port);
    void                            set_protocol(char const * protocol);
    void                            set_protocol(int protocol);
    void                            set_mask(uint8_t const * mask);
    void                            apply_mask();

    bool                            is_ipv4() const;
    void                            get_ipv4(struct sockaddr_in & in) const;
    void                            get_ipv6(struct sockaddr_in6 & in6) const;
    std::string                     to_ipv4_string(string_ip_t mode) const;
    std::string                     to_ipv6_string(string_ip_t mode) const;
    std::string                     to_ipv4or6_string(string_ip_t mode) const;

    network_type_t                  get_network_type() const;
    std::string                     get_network_type_string() const;
    computer_interface_address_t    is_computer_interface_address() const;

    std::string                     get_iface_name() const;
    std::string                     get_name() const;
    std::string                     get_service() const;
    int                             get_port() const;
    int                             get_protocol() const;
    void                            get_mask(uint8_t * mask);

    bool                            match(addr const & ip) const;
    bool                            operator == (addr const & rhs) const;
    bool                            operator != (addr const & rhs) const;
    bool                            operator <  (addr const & rhs) const;
    bool                            operator <= (addr const & rhs) const;
    bool                            operator >  (addr const & rhs) const;
    bool                            operator >= (addr const & rhs) const;

private:
    void                            address_changed();

    // always keep address in an IPv6 structure
    //
    struct sockaddr_in6             f_address = init_in6();
    uint8_t                         f_mask[16] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
    std::string                     f_iface_name;
    int                             f_protocol = IPPROTO_TCP;
    mutable network_type_t          f_private_network_defined = network_type_t::NETWORK_TYPE_UNDEFINED;
};


class addr_range
{
public:
    typedef std::shared_ptr<addr_range>     pointer_t;
    typedef std::vector<addr_range>         vector_t;

    bool                            has_from() const;
    bool                            has_to() const;
    bool                            is_range() const;
    bool                            is_empty() const;
    bool                            is_in(addr const & rhs) const;

    void                            set_from(addr const & from);
    addr &                          get_from();
    addr const &                    get_from() const;
    void                            set_to(addr const & to);
    addr &                          get_to();
    addr const &                    get_to() const;

    addr_range                      intersection(addr_range const & rhs) const;

private:
    bool                            f_has_from = false;
    bool                            f_has_to = false;
    addr                            f_from;
    addr                            f_to;
};


class addr_parser
{
public:
    enum class flag_t
    {
        ADDRESS,                            // address (IP)
        REQUIRED_ADDRESS,                   // address cannot be empty
        PORT,                               // port
        REQUIRED_PORT,                      // port cannot be empty
        MASK,                               // mask
        MULTI_ADDRESSES_COMMAS,             // IP:port/mask,IP:port/mask,...
        MULTI_ADDRESSES_SPACES,             // IP:port/mask IP:port/mask ...
        MULTI_ADDRESSES_COMMAS_AND_SPACES,  // IP:port/mask, IP:port/mask, ...

        // the following are not yet implemented
        MULTI_PORTS_SEMICOLONS,             // port1;port2;...
        MULTI_PORTS_COMMAS,                 // port1,port2,...
        PORT_RANGE,                         // port1-port2
        ADDRESS_RANGE,                      // IP-IP

        FLAG_max
    };

    void                    set_default_address(std::string const & addr);
    std::string const &     get_default_address() const;
    void                    set_default_port(int const port);
    int                     get_default_port() const;
    void                    set_default_mask(std::string const & mask);
    std::string const &     get_default_mask() const;
    void                    set_protocol(std::string const & protocol);
    void                    set_protocol(int const protocol);
    void                    clear_protocol();
    int                     get_protocol() const;

    void                    set_allow(flag_t const flag, bool const allow);
    bool                    get_allow(flag_t const flag) const;

    bool                    has_errors() const;
    void                    emit_error(std::string const & msg);
    std::string const &     error_messages() const;
    int                     error_count() const;
    void                    clear_errors();
    addr_range::vector_t    parse(std::string const & in);

private:
    void                    parse_cidr(std::string const & in, addr_range::vector_t & result);
    void                    parse_address(std::string const & in, addr_range::vector_t & result);
    void                    parse_address4(std::string const & in, addr_range::vector_t & result);
    void                    parse_address6(std::string const & in, addr_range::vector_t & result);
    void                    parse_address_port(std::string const & address, std::string const & port_str, addr_range::vector_t & result);
    void                    parse_mask(std::string const & mask, addr & cidr);

    bool                    f_flags[static_cast<int>(flag_t::FLAG_max)] = { true, false, true, false, false, false, false, false, false, false, false, false };
    std::string             f_default_address;
    std::string             f_default_mask;
    int                     f_protocol = -1;
    int                     f_default_port = -1;
    std::string             f_error;
    int                     f_error_count = 0;
};



} // addr namespace


inline bool operator == (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) == 0;
}


inline bool operator != (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) != 0;
}


inline bool operator < (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) < 0;
}


inline bool operator <= (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) <= 0;
}


inline bool operator > (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) > 0;
}


inline bool operator >= (struct sockaddr_in6 const & a, struct sockaddr_in6 const & b)
{
    return memcmp(&a, &b, sizeof(struct sockaddr_in6)) >= 0;
}


inline bool operator == (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) == 0;
}


inline bool operator != (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) != 0;
}


inline bool operator < (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) < 0;
}


inline bool operator <= (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) <= 0;
}


inline bool operator > (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) > 0;
}


inline bool operator >= (in6_addr const & a, in6_addr const & b)
{
    return memcmp(&a, &b, sizeof(in6_addr)) >= 0;
}


// vim: ts=4 sw=4 et
