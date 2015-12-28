// Network Address -- classes functions to ease handling IP addresses
// Copyright (C) 2012-2015  Made to Order Software Corp.
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

#include "snap_exception.h"

#include <arpa/inet.h>

#include <string>
#include <memory>

namespace snap_addr
{


class addr_invalid_argument_exception : public snap::snap_exception
{
public:
    addr_invalid_argument_exception(char const *        what_msg) : snap_exception(what_msg) {}
    addr_invalid_argument_exception(std::string const & what_msg) : snap_exception(what_msg) {}
    addr_invalid_argument_exception(QString const &     what_msg) : snap_exception(what_msg) {}
};

class addr_invalid_structure_exception : public snap::snap_logic_exception
{
public:
    addr_invalid_structure_exception(char const *        what_msg) : snap_logic_exception(what_msg) {}
    addr_invalid_structure_exception(std::string const & what_msg) : snap_logic_exception(what_msg) {}
    addr_invalid_structure_exception(QString const &     what_msg) : snap_logic_exception(what_msg) {}
};

class addr_invalid_parameter_exception : public snap::snap_logic_exception
{
public:
    addr_invalid_parameter_exception(char const *        what_msg) : snap_logic_exception(what_msg) {}
    addr_invalid_parameter_exception(std::string const & what_msg) : snap_logic_exception(what_msg) {}
    addr_invalid_parameter_exception(QString const &     what_msg) : snap_logic_exception(what_msg) {}
};



class addr
{
public:
    typedef std::shared_ptr<addr>       pointer_t;
    typedef std::vector<addr>           vector_t;

                    addr();
                    addr(std::string const & ap, std::string const & default_address, int const default_port, char const * protocol);
                    addr(std::string const & ap, char const * protocol);
                    addr(struct sockaddr_in const & in);
                    addr(struct sockaddr_in6 const & in6);

    void            set_addr_port(std::string const & ap, std::string const & default_address, int const default_port, char const * protocol);
    void            set_from_socket(int s);
    void            set_ipv4(struct sockaddr_in const & in);
    void            set_ipv6(struct sockaddr_in6 const & in6);
    void            set_protocol(char const * protocol);

    bool            is_ipv4() const;
    void            get_ipv4(struct sockaddr_in & in) const;
    void            get_ipv6(struct sockaddr_in6 & in6) const;
    std::string     get_ipv4_string(bool include_port = false) const;
    std::string     get_ipv6_string(bool include_port = false, bool include_brackets = true) const;
    std::string     get_ipv4or6_string(bool include_port = false, bool include_brackets = true) const;

    int             get_port() const;
    int             get_protocol() const;

    bool            operator == (addr const & rhs) const;
    bool            operator < (addr const & rhs) const;

private:
    // either way, keep address in an IPv6 structure
    struct sockaddr_in6 f_address = sockaddr_in6();
    int                 f_protocol = IPPROTO_TCP;
};


}
// snap_addr namespace
// vim: ts=4 sw=4 et
