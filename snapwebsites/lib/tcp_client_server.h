// TCP Client & Server -- classes to ease handling sockets
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

#include <stdexcept>
#include <memory>

#include <arpa/inet.h>

// BIO versions of the TCP client/server
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

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

class tcp_client_server_initialization_error : public tcp_client_server_runtime_error
{
public:
    tcp_client_server_initialization_error(std::string const& errmsg) : tcp_client_server_runtime_error(errmsg) {}
};



// TODO: assuming that bio_client with MODE_PLAIN works the same way
//       as a basic tcp_client, we should remove this class
class tcp_client
{
public:
    typedef std::shared_ptr<tcp_client>     pointer_t;

                        tcp_client(std::string const& addr, int port);
                        ~tcp_client();

    int                 get_socket() const;
    int                 get_port() const;
    int                 get_client_port() const;
    std::string         get_addr() const;
    std::string         get_client_addr() const;

    int                 read(char *buf, size_t size);
    int                 read_line(std::string& line);
    int                 write(char const *buf, size_t size);

private:
    int                 f_socket;
    int                 f_port;
    std::string         f_addr;
};


// TODO: implement a bio_server then like with the client remove
//       this basic tcp_server if it was like the bio version
class tcp_server
{
public:
    typedef std::shared_ptr<tcp_server>     pointer_t;

    static int const    MAX_CONNECTIONS = 50;

                        tcp_server(std::string const& addr, int port, int max_connections = -1, bool reuse_addr = false, bool auto_close = false);
                        ~tcp_server();

    int                 get_socket() const;
    int                 get_max_connections() const;
    int                 get_port() const;
    std::string         get_addr() const;
    bool                get_keepalive() const;

    void                keepalive(bool yes = true);
    int                 accept( int const max_wait_ms = -1 );
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


class bio_client
{
public:
    typedef std::shared_ptr<bio_client>     pointer_t;

    enum class mode_t
    {
        MODE_PLAIN,             // avoid SSL/TLS
        MODE_SECURE,            // WARNING: may return a non-secure connection
        MODE_ALWAYS_SECURE      // fails if cannot be secure
    };

                        bio_client(std::string const& addr, int port, mode_t mode = mode_t::MODE_PLAIN);
                        ~bio_client();

    int                 get_socket() const;
    int                 get_port() const;
    int                 get_client_port() const;
    std::string         get_addr() const;
    std::string         get_client_addr() const;

    int                 read(char *buf, size_t size);
    int                 read_line(std::string& line);
    int                 write(char const *buf, size_t size);

private:
    std::shared_ptr<BIO>        f_bio;
    std::shared_ptr<SSL_CTX>    f_ssl_ctx;
};


} // namespace tcp_client_server
// vim: ts=4 sw=4 et
