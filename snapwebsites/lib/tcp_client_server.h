// TCP Client & Server -- classes to ease handling sockets
// Copyright (C) 2012-2014  Made to Order Software Corp.
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

#include <stdexcept>

#include <arpa/inet.h>

namespace tcp_client_server
{

class tcp_client_server_logic_error : public std::logic_error
{
public:
    tcp_client_server_logic_error(std::string const& errmsg) : logic_error(errmsg) {}
};

class tcp_client_server_runtime_error : public std::runtime_error
{
public:
    tcp_client_server_runtime_error(std::string const& errmsg) : runtime_error(errmsg) {}
};

class tcp_client_server_parameter_error : public tcp_client_server_logic_error
{
public:
    tcp_client_server_parameter_error(std::string const& errmsg) : tcp_client_server_logic_error(errmsg) {}
};



class tcp_client
{
public:
                        tcp_client(std::string const& addr, int port);
                        ~tcp_client();

    int                 get_socket() const;
    int                 get_port() const;
    std::string         get_addr() const;

    int                 read(char *buf, size_t size);
    int                 read_line(std::string& line);
    int                 write(char const *buf, size_t size);

private:
    int                 f_socket;
    int                 f_port;
    std::string         f_addr;
};


class tcp_server
{
public:
    static int const    MAX_CONNECTIONS = 50;

                        tcp_server(std::string const& addr, int port, int max_connections = -1, bool reuse_addr = false, bool auto_close = false);
                        ~tcp_server();

    int                 get_socket() const;
    int                 get_max_connections() const;
    int                 get_port() const;
    std::string         get_addr() const;
    bool                get_keepalive() const;

    void                keepalive(bool yes = true);
    int                 accept( const int max_wait_ms = -1 );
    int                 get_last_accepted_socket() const;

private:
    int                 f_max_connections;
    int                 f_socket;
    int                 f_port;
    std::string         f_addr;
    int                 f_accepted_socket;
    struct sockaddr_in  f_accepted_addr;
    bool                f_keepalive;
    bool                f_auto_close;
};


} // namespace tcp_client_server
// vim: ts=4 sw=4 et
