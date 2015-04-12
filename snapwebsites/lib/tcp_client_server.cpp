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

#include "tcp_client_server.h"

#include "not_reached.h"

#include <iostream>
#include <sstream>

#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include "poison.h"

namespace tcp_client_server
{


namespace
{

/** \brief Address info class to auto-free the structures.
 *
 * This class is used so we can auto-free the addrinfo structure(s)
 * because otherwise we find ourselves with many freeaddrinfo()
 * calls (and that's not safe in case you have exceptions.)
 */
class addrinfo_t
{
public:
    /** \brief Initialize the structure pointer to nullptr.
     *
     * The constructor ensures that the pointer is nullptr.
     */
    addrinfo_t()
        : f_addrinfo(nullptr)
    {
    }

    /** \brief Ensures that the structures get cleaned up.
     *
     * This function ensures that the addrinfo pointers get freed with a
     * call to the freeaddrinfo().
     *
     * If no structure was allocated, nothing happens.
     */
    ~addrinfo_t()
    {
        if(f_addrinfo != nullptr)
        {
            freeaddrinfo(f_addrinfo);
        }
    }

    /** \brief The address info.
     *
     * This is the address, it is public because we just use that internally
     * in the client and server constructors (see below.)
     */
    struct addrinfo *   f_addrinfo;
};


/** \brief Whether the bio_initialize() function was already called.
 *
 * This flag is used to know whether the bio_initialize() function was
 * already called. Only the bio_initialize() function is expected to
 * make use of this flag. Other functions should simply call the
 * bio_initialize() function (future versions may include addition
 * flags or use various bits in an integer instead.)
 */
bool g_bio_initialized = false;


/** \brief Initialize the BIO library.
 *
 * This function is called by the BIO implementations to initialize the
 * BIO library as required. It can be called any number of times. The
 * initialization will happen only once.
 */
void bio_initialize()
{
    // already initialized?
    if(g_bio_initialized)
    {
        return;
    }
    g_bio_initialized = true;

    // Make sure the SSL library gets initialized
    SSL_library_init();

    // TBD: should we call the load string functions only when we
    //      are about to generate an error?
    ERR_load_crypto_strings();
    ERR_load_SSL_strings();

    // TODO: define a way to only define safe algorithm
    //       (it looks like we can force TLSv1 below at least)
    OpenSSL_add_all_algorithms();

    // TBD: need a PRNG seeding before creating a new SSL context?
}



void bio_deleter(BIO *bio)
{
    BIO_free_all(bio);
}


void ssl_ctx_deleter(SSL_CTX *ssl_ctx)
{
    SSL_CTX_free(ssl_ctx);
}


}
// no name namespace



// ========================= CLIENT =========================


/** \class tcp_client
 * \brief Create a client socket and connect to a server.
 *
 * This class is a client socket implementation used to connect to a server.
 * The server is expected to be running at the time the client is created
 * otherwise it fails connecting.
 *
 * This class is not appropriate to connect to a server that may come and go
 * over time.
 */

/** \brief Contruct a tcp_client object.
 *
 * The tcp_client constructor initializes a TCP client object by connecting
 * to the specified server. The server is defined with the \p addr and
 * \p port specified as parameters.
 *
 * \exception tcp_client_server_parameter_error
 * This exception is raised if the \p port parameter is out of range or the
 * IP address is an empty string or otherwise an invalid address.
 *
 * \exception tcp_client_server_runtime_error
 * This exception is raised if the client cannot create the socket or it
 * cannot connect to the server.
 *
 * \param[in] addr  The address of the server to connect to. It must be valid.
 * \param[in] port  The port the server is listening on.
 */
tcp_client::tcp_client(std::string const& addr, int port)
    : f_socket(-1)
    , f_port(port)
    , f_addr(addr)
{
    if(f_port < 0 || f_port >= 65536)
    {
        throw tcp_client_server_parameter_error("invalid port for a client socket");
    }
    if(f_addr.empty())
    {
        throw tcp_client_server_parameter_error("an empty address is not valid for a client socket");
    }

    std::stringstream decimal_port;
    decimal_port << f_port;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo_t addr_info;
    std::string port_str(decimal_port.str());
    int r(getaddrinfo(addr.c_str(), port_str.c_str(), &hints, &addr_info.f_addrinfo));
    if(r != 0 || addr_info.f_addrinfo == nullptr)
    {
        throw tcp_client_server_runtime_error("invalid address or port: \"" + addr + ":" + port_str + "\"");
    }

    f_socket = socket(addr_info.f_addrinfo->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if(f_socket < 0)
    {
        throw tcp_client_server_runtime_error("could not create socket for client");
    }

    if(connect(f_socket, addr_info.f_addrinfo->ai_addr, addr_info.f_addrinfo->ai_addrlen) < 0)
    {
        close(f_socket);
        throw tcp_client_server_runtime_error("could not connect client socket to \"" + f_addr + "\"");
    }
}

/** \brief Clean up the TCP client object.
 *
 * This function cleans up the TCP client object by closing the attached socket.
 */
tcp_client::~tcp_client()
{
    close(f_socket);
}

/** \brief Get the socket descriptor.
 *
 * This function returns the TCP client socket descriptor. This can be
 * used to change the descriptor behavior (i.e. make it non-blocking for
 * example.)
 *
 * \return The socket descriptor.
 */
int tcp_client::get_socket() const
{
    return f_socket;
}

/** \brief Get the TCP client port.
 *
 * This function returns the port used when creating the TCP client.
 * Note that this is the port the server is listening to and not the port
 * the TCP client is currently connected to.
 *
 * \return The TCP client port.
 */
int tcp_client::get_port() const
{
    return f_port;
}

/** \brief Get the TCP server address.
 *
 * This function returns the address used when creating the TCP address as is.
 * Note that this is the address of the server where the client is connected
 * and not the address where the client is running (although it may be the
 * same.)
 *
 * Use the get_socket_name() function to retrieve the client's TCP address.
 *
 * \return The TCP client address.
 */
std::string tcp_client::get_addr() const
{
    return f_addr;
}

/** \brief Get the TCP client port.
 *
 * This function retrieve the port of the client (used on your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \return The port or -1 if it cannot be determined.
 */
int tcp_client::get_client_port() const
{
    struct sockaddr addr;
    socklen_t len(sizeof(addr));
    int r(getsockname(f_socket, &addr, &len));
    if(r != 0)
    {
        return -1;
    }
    // Note: I know the port is at the exact same location in both
    //       structures in Linux but it could change on other Unices
    if(addr.sa_family == AF_INET)
    {
        // IPv4
        return reinterpret_cast<sockaddr_in *>(&addr)->sin_port;
    }
    if(addr.sa_family == AF_INET6)
    {
        // IPv6
        return reinterpret_cast<sockaddr_in6 *>(&addr)->sin6_port;
    }
    return -1;
}

/** \brief Get the TCP client address.
 *
 * This function retrieve the IP address of the client (your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \return The IP address as a string.
 */
std::string tcp_client::get_client_addr() const
{
    struct sockaddr addr;
    socklen_t len(sizeof(addr));
    int const r(getsockname(f_socket, &addr, &len));
    if(r != 0)
    {
        throw tcp_client_server_runtime_error("address not available");
    }
    char buf[BUFSIZ];
    switch(addr.sa_family)
    {
    case AF_INET:
        inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in *>(&addr)->sin_addr, buf, sizeof(buf));
        break;

    case AF_INET6:
        inet_ntop(AF_INET6, &reinterpret_cast<struct sockaddr_in6 *>(&addr)->sin6_addr, buf, sizeof(buf));
        break;

    default:
        throw tcp_client_server_runtime_error("unknown address family");

    }
    return buf;
}

/** \brief Read data from the socket.
 *
 * A TCP socket is a stream type of socket and one can read data from it
 * as if it were a regular file. This function reads \p size bytes and
 * returns. The function returns early if the server closes the connection.
 *
 * If your socket is blocking, \p size should be exactly what you are
 * expecting or this function will block forever or until the server
 * closes the connection.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \param[in,out] buf  The buffer where the data is read.
 * \param[in] size  The size of the buffer.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 */
int tcp_client::read(char *buf, size_t size)
{
    return static_cast<int>(::read(f_socket, buf, size));
}


/** \brief Read one line.
 *
 * This function reads one line from the current location up to the next
 * '\\n' character. We do not have any special handling of the '\\r'
 * character.
 *
 * The function may return 0 in which case the server closed the connection.
 *
 * \param[out] line  The resulting line read from the server.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 *         If the function returns 0 or more, then the \p line parameter
 *         represents the characters read on the network.
 */
int tcp_client::read_line(std::string& line)
{
    line.clear();
    int len(0);
    for(;;)
    {
        char c;
        int r(read(&c, sizeof(c)));
        if(r <= 0)
        {
            return len == 0 && r < 0 ? -1 : len;
        }
        if(c == '\n')
        {
            return len;
        }
        ++len;
        line += c;
    }
}


/** \brief Write data to the socket.
 *
 * A TCP socket is a stream type of socket and one can write data to it
 * as if it were a regular file. This function writes \p size bytes to
 * the socket and then returns. This function returns early if the server
 * closes the connection.
 *
 * If your socket is not blocking, less than \p size bytes may be written
 * to the socket. In that case you are responsible for calling the function
 * again to write the remainder of the buffer until the function returns
 * a number of bytes written equal to \p size.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \param[in] buf  The buffer with the data to send over the socket.
 * \param[in] size  The number of bytes in buffer to send over the socket.
 *
 * \return The number of bytes that were actually accepted by the socket
 * or -1 if an error occurs.
 */
int tcp_client::write(const char *buf, size_t size)
{
    return static_cast<int>(::write(f_socket, buf, size));
}


// ========================= SERVER =========================

/** \brief Initialize the server and start listening for connections.
 *
 * The server constructor creates a socket, binds it, and then listen to it.
 *
 * By default the server accepts a maximum of \p max_connections (set to
 * -1 to get the default tcp_server::MAX_CONNECTIONS) in its waiting queue.
 * If you use the server and expect a low connection rate, you may want to
 * reduce the count to 5. Although some very busy servers use larger numbers.
 *
 * The address is made non-reusable (which is the default for TCP sockets.)
 * It is possible to mark the server address as immediately reusable by
 * setting the \p reuse_addr to true.
 *
 * By default the server is marked as "keepalive". You can turn it off
 * using the keepalive() function with false.
 *
 * \exception tcp_client_server_parameter_error
 * This exception is raised if the address parameter is an empty string or
 * otherwise an invalid IP address, or if the port is out of range.
 *
 * \exception tcp_client_server_runtime_error
 * This exception is raised if the socket cannot be created, bound to
 * the specified IP address and port, or listen() fails on the socket.
 *
 * \param[in] addr  The address to listen on. It may be set to "0.0.0.0".
 * \param[in] port  The port to listen on.
 * \param[in] max_connections  The number of connections to keep in the listen queue.
 * \param[in] reuse_addr  Whether to mark the socket with the SO_REUSEADDR flag.
 * \param[in] auto_close  Automatically close the the client socket in accept and the destructor.
 */
tcp_server::tcp_server(const std::string& addr, int port, int max_connections, bool reuse_addr, bool auto_close)
    : f_max_connections(max_connections < 1 ? MAX_CONNECTIONS : max_connections)
    , f_socket(-1)
    , f_port(port)
    , f_addr(addr)
    , f_accepted_socket(-1)
    , f_keepalive(true)
    , f_auto_close(auto_close)
{
    if(f_addr.empty())
    {
        throw tcp_client_server_parameter_error("the address cannot be an empty string");
    }
    if(f_port < 0 || f_port >= 65536)
    {
        throw tcp_client_server_parameter_error("invalid port for a client socket");
    }

    //char decimal_port[16];
    std::stringstream decimal_port;
    decimal_port << f_port;
    //snprintf(decimal_port, sizeof(decimal_port), "%d", f_port);
    //decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo_t addr_info;
    std::string port_str(decimal_port.str());
    int r(getaddrinfo(addr.c_str(), port_str.c_str(), &hints, &addr_info.f_addrinfo));
    if(r != 0 || addr_info.f_addrinfo == nullptr)
    {
        throw tcp_client_server_runtime_error("invalid address or port: \"" + addr + ":" + port_str + "\"");
    }

    f_socket = socket(addr_info.f_addrinfo->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if(f_socket < 0)
    {
        throw tcp_client_server_runtime_error("could not create socket for client");
    }

    // this should be optional as reusing an address for TCP/IP is not 100% safe
    if(reuse_addr)
    {
        // try to mark the socket address as immediately reusable
        // if this fails, we ignore the error (TODO log an INFO message)
        int optval(1);
        socklen_t const optlen(sizeof(optval));
        static_cast<void>(setsockopt(f_socket, SOL_SOCKET, SO_REUSEADDR, &optval, optlen));
    }

    if(bind(f_socket, addr_info.f_addrinfo->ai_addr, addr_info.f_addrinfo->ai_addrlen) < 0)
    {
        throw tcp_client_server_runtime_error("could not bind the socket to \"" + f_addr + "\"");
    }

    // start listening, we expect the caller to then call accept() to
    // acquire connections
    if(listen(f_socket, f_max_connections) < 0)
    {
        throw tcp_client_server_runtime_error("could not listen to the socket bound to \"" + f_addr + "\"");
    }
}


/** \brief Clean up the server sockets.
 *
 * This function ensures that the server sockets get cleaned up.
 *
 * If the \p auto_close parameter was set to true in the constructor, then
 * the last accepter socket gets closed by this function.
 */
tcp_server::~tcp_server()
{
    close(f_socket);
    if(f_auto_close && f_accepted_socket != -1)
    {
        close(f_accepted_socket);
    }
}


/** \brief Retrieve the socket descriptor.
 *
 * This function returns the socket descriptor. It can be used to
 * tweak things on the socket such as making it non-blocking or
 * directly accessing the data.
 *
 * \return The socket descriptor.
 */
int tcp_server::get_socket() const
{
    return f_socket;
}


/** \brief Retrieve the maximum number of connections.
 *
 * This function returns the maximum number of connections that can
 * be accepted by the socket. This was set by the constructor and
 * it cannot be changed later.
 *
 * \return The maximum number of incoming connections.
 */
int tcp_server::get_max_connections() const
{
    return f_max_connections;
}


/** \brief Return the server port.
 *
 * This function returns the port the server was created with. This port
 * is exactly what the server currently uses. It cannot be changed.
 *
 * \return The server port.
 */
int tcp_server::get_port() const
{
    return f_port;
}


/** \brief Retrieve the server IP address.
 *
 * This function returns the IP address used to bind the socket. This
 * is the address clients have to use to connect to the server unless
 * the address was set to all zeroes (0.0.0.0) in which case any user
 * can connect.
 *
 * The IP address cannot be changed.
 *
 * \return The server IP address.
 */
std::string tcp_server::get_addr() const
{
    return f_addr;
}


/** \brief Return the current status of the keepalive flag.
 *
 * This function returns the current status of the keepalive flag. This
 * flag is set to true by default (in the constructor.) It can be
 * changed with the keepalive() function.
 *
 * The flag is used to mark new connections with the SO_KEEPALIVE flag.
 * This is used whenever a service may take a little to long to answer
 * and avoid losing the TCP connection before the answer is sent to
 * the client.
 *
 * \return The current status of the keepalive flag.
 */
bool tcp_server::get_keepalive() const
{
    return f_keepalive;
}


/** \brief Set the keepalive flag.
 *
 * This function sets the keepalive flag to either true (i.e. mark connection
 * sockets with the SO_KEEPALIVE flag) or false. The default is true (as set
 * in the constructor,) because in most cases this is a feature people want.
 *
 * \param[in] yes  Whether to keep new connections alive even when no traffic
 * goes through.
 */
void tcp_server::keepalive(bool yes)
{
    f_keepalive = yes;
}


/** \brief Accept a connection.
 *
 * A TCP server accepts incoming connections. This call is a blocking call.
 * If no connections are available on the line, then the call blocks until
 * a connection becomes available.
 *
 * To prevent being blocked by this call you can either check the status of
 * the file descriptor (use the get_socket() function to retrieve the
 * descriptor and use an appropriate wait with 0 as a timeout,) or transform
 * the socket in a non-blocking socket (not tested, though.)
 *
 * This TCP socket implementation is expected to be used in one of two ways:
 *
 * (1) the main server accepts connections and then fork()'s to handle the
 * transaction with the client, in that case we want to set the \p auto_close
 * parameter of the constructor to true so the accept() function automatically
 * closes the last accepted socket.
 *
 * (2) the main server keeps a set of connections and handles them alongside
 * the main server connection. Although there are limits to what you can do
 * in this way, it is very efficient, but this also means the accept() call
 * cannot close the last accepted socket since the rest of the software may
 * still be working on it.
 *
 * The function returns a client/server socket. This is the socket one can
 * use to communicate with the client that just connected to the server. This
 * descriptor can be written to or read from.
 *
 * This function is the one that applies the keepalive flag to the
 * newly accepted socket.
 *
 * \note
 * If you prevent SIGCHLD from stopping your code, you may want to allow it
 * when calling this function (that is, if you're interested in getting that
 * information immediately, otherwise it is cleaner to always block those
 * signals.)
 *
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for a message. If set to -1 (the default), accept() will block indefintely.
 *
 * \return A client socket descriptor or -1 if an error occured, -2 if timeout and max_wait is set.
 */
int tcp_server::accept( int const max_wait_ms )
{
    // auto-close?
    if(f_auto_close && f_accepted_socket != -1)
    {
        // if the close is interrupted, make sure we try again otherwise
        // we could lose that stream until next restart (this could happen
        // if you have SIGCHLD)
        if(close(f_accepted_socket) == -1)
        {
            if(errno == EINTR)
            {
                close(f_accepted_socket);
            }
        }
    }
    f_accepted_socket = -1;

    if( max_wait_ms > -1 )
    {
        fd_set s;
        //
        FD_ZERO(&s);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        FD_SET(f_socket, &s);
#pragma GCC diagnostic pop
        //
        struct timeval timeout;
        timeout.tv_sec = max_wait_ms / 1000;
        timeout.tv_usec = (max_wait_ms % 1000) * 1000;
        int const retval = select(f_socket + 1, &s, NULL, &s, &timeout);
        //
        if( retval == -1 )
        {
            // error
            //
            return -1;
        }
        else if( retval == 0 )
        {
            // timeout
            //
            return -2;
        }
    }

    // accept the next connection
    struct sockaddr_in accepted_addr;
    socklen_t addr_len(sizeof(accepted_addr));
    memset(&accepted_addr, 0, sizeof(accepted_addr));
    f_accepted_socket = ::accept(f_socket, reinterpret_cast<struct sockaddr *>(&accepted_addr), &addr_len);

    // mark the new connection with the SO_KEEPALIVE flag
    if(f_accepted_socket != -1 && f_keepalive)
    {
        // if this fails, we ignore the error (TODO log an INFO message)
        int optval(1);
        socklen_t const optlen(sizeof(optval));
        static_cast<void>(setsockopt(f_accepted_socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen));
    }

    return f_accepted_socket;
}


/** \brief Retrieve the last accepted socket descriptor.
 *
 * This function returns the last accepted socket descriptor as retrieved by
 * accept(). If accept() was never called or failed, then this returns -1.
 *
 * Note that it is possible that the socket was closed in between in which
 * case this value is going to be an invalid socket.
 *
 * \return The last accepted socket descriptor.
 */
int tcp_server::get_last_accepted_socket() const
{
    return f_accepted_socket;
}




// ========================= BIO CLIENT =========================


/** \class bio_client
 * \brief Create a BIO client and connect to a server, eventually with TLS.
 *
 * This class is a client socket implementation used to connect to a server.
 * The server is expected to be running at the time the client is created
 * otherwise it fails connecting.
 *
 * This class is not appropriate to connect to a server that may come and go
 * over time.
 *
 * The BIO extension is from the OpenSSL library and it allows the client
 * to connect using SSL. At this time connections are either secure or
 * not secure. If a secure connection fails, you may attempt again without
 * TLS or other encryption mechanism.
 */


/** \brief Contruct a bio_client object.
 *
 * The bio_client constructor initializes a BIO connector and connects
 * to the specified server. The server is defined with the \p addr and
 * \p port specified as parameters. The connection tries to use TLS if
 * the \p mode parameter is set to MODE_SECURE. Note that you may force
 * a secure connection using MODE_SECURE_REQUIRED. With MODE_SECURE,
 * the connection to the server can be obtained even if a secure
 * connection could not be made to work.
 *
 * \exception tcp_client_server_parameter_error
 * This exception is raised if the \p port parameter is out of range or the
 * IP address is an empty string or otherwise an invalid address.
 *
 * \exception tcp_client_server_initialization_error
 * This exception is raised if the client cannot create the socket or it
 * cannot connect to the server.
 *
 * \param[in] addr  The address of the server to connect to. It must be valid.
 * \param[in] port  The port the server is listening on.
 * \param[in] mode  Whether to use SSL when connecting.
 */
bio_client::bio_client(std::string const& addr, int port, mode_t mode)
    //: f_bio(nullptr) -- auto-init
    //, f_ssl_ctx(nullptr) -- auto-init
    //, f_port(port)
    //, f_addr(addr)
{
    if(port < 0 || port >= 65536)
    {
        throw tcp_client_server_parameter_error("invalid port for a client socket");
    }
    if(addr.empty())
    {
        throw tcp_client_server_parameter_error("an empty address is not valid for a client socket");
    }

    bio_initialize();

    switch(mode)
    {
    case mode_t::MODE_SECURE:
    case mode_t::MODE_ALWAYS_SECURE:
        {
            // Use TLS v1 only as all versions of SSL are flawed...
            std::shared_ptr<SSL_CTX> ssl_ctx(SSL_CTX_new(TLSv1_client_method()), ssl_ctx_deleter);
            if(!ssl_ctx)
            {
                ERR_print_errors_fp(stderr);
                throw tcp_client_server_initialization_error("failed initializing an SSL_CTX object");
            }

            // load root certificates (correct path for Ubuntu?)
            if(SSL_CTX_load_verify_locations(ssl_ctx.get(), NULL, "/etc/ssl/certs") != 1)
            {
                ERR_print_errors_fp(stderr);
                throw tcp_client_server_initialization_error("failed loading verification certificates in an SSL_CTX object");
            }

            // create a BIO connected to SSL ciphers
            std::shared_ptr<BIO> bio(BIO_new_ssl_connect(ssl_ctx.get()), bio_deleter);
            if(!bio)
            {
                ERR_print_errors_fp(stderr);
                throw tcp_client_server_initialization_error("failed initializing a BIO object");
            }

            // verify that the connection worked
            SSL *ssl(nullptr);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_get_ssl(bio.get(), &ssl);
#pragma GCC diagnostic pop
            if(ssl == nullptr)
            {
                // TBD: does this mean we would have a plain connection?
                ERR_print_errors_fp(stderr);
                throw tcp_client_server_initialization_error("failed connecting BIO object with SSL_CTX object");
            }

            // allow automatic retries in case the connection somehow needs
            // an SSL renegotiation (maybe we should turn that off for cases
            // where we connect to a secure payment gateway?)
            SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

            // TODO: other SSL initialization?

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_set_conn_hostname(bio.get(), const_cast<char *>(addr.c_str()));
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
            BIO_set_conn_int_port(bio.get(), &port);
#pragma GCC diagnostic pop

            // connect to the server (open the socket)
            if(BIO_do_connect(bio.get()) <= 0)
            {
                ERR_print_errors_fp(stderr);
                throw tcp_client_server_initialization_error("failed connecting BIO object to server");
            }

            // encryption handshake
            if(BIO_do_handshake(bio.get()) != 1)
            {
                ERR_print_errors_fp(stderr);
                throw tcp_client_server_initialization_error("failed establishing a secure BIO connection with server");
            }

            // verify that the peer certificate was signed by a
            // recognized root authority
            if(SSL_get_peer_certificate(ssl) == nullptr)
            {
                ERR_print_errors_fp(stderr);
                throw tcp_client_server_initialization_error("peer failed presenting a certificate for security verification");
            }

            if(SSL_get_verify_result(ssl) != X509_V_OK)
            {
                ERR_print_errors_fp(stderr);
                throw tcp_client_server_initialization_error("peer certificate could not be verified");
            }

            // it worked, save the results
            f_ssl_ctx = ssl_ctx;
            f_bio = bio;

            // secure connection ready
        }
        break;

    case mode_t::MODE_PLAIN:
        {
            // create a plain BIO connection
            std::shared_ptr<BIO> bio(BIO_new(BIO_s_connect()), bio_deleter);
            if(!bio)
            {
                ERR_print_errors_fp(stderr);
                throw tcp_client_server_initialization_error("failed initializing a BIO object");
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_set_conn_hostname(bio.get(), const_cast<char *>(addr.c_str()));
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
            BIO_set_conn_int_port(bio.get(), &port);
#pragma GCC diagnostic pop

            // connect to the server (open the socket)
            if(BIO_do_connect(bio.get()) <= 0)
            {
                ERR_print_errors_fp(stderr);
                throw tcp_client_server_initialization_error("failed connecting BIO object to server");
            }

            // it worked, save the results
            f_bio = bio;

            // plain connection ready
        }
        break;

    }
}


/** \brief Clean up the BIO client object.
 *
 * This function cleans up the BIO client object by freeing the SSL_CTX
 * and the BIO objects.
 */
bio_client::~bio_client()
{
    // f_bio and f_ssl_ctx are allocated using shared pointers with
    // a deleter so we have nothing to do here.
}


/** \brief Get the socket descriptor.
 *
 * This function returns the TCP client socket descriptor. This can be
 * used to change the descriptor behavior (i.e. make it non-blocking for
 * example.)
 *
 * \warning
 * This socket is generally managed by the BIO library and thus it may
 * create unwanted side effects to change the socket under the feet of
 * the BIO library...
 *
 * \return The socket descriptor.
 */
int bio_client::get_socket() const
{
    int c;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    BIO_get_fd(f_bio.get(), &c);
#pragma GCC diagnostic pop
    return c;
}


/** \brief Get the TCP client port.
 *
 * This function returns the port used when creating the TCP client.
 * Note that this is the port the server is listening to and not the port
 * the TCP client is currently connected to.
 *
 * \return The TCP client port.
 */
int bio_client::get_port() const
{
    return BIO_get_conn_int_port(f_bio.get());
}


/** \brief Get the TCP server address.
 *
 * This function returns the address used when creating the TCP address as is.
 * Note that this is the address of the server where the client is connected
 * and not the address where the client is running (although it may be the
 * same.)
 *
 * Use the get_socket_name() function to retrieve the client's TCP address.
 *
 * \return The TCP client address.
 */
std::string bio_client::get_addr() const
{
    return BIO_get_conn_hostname(f_bio.get());
}


/** \brief Get the TCP client port.
 *
 * This function retrieve the port of the client (used on your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \return The port or -1 if it cannot be determined.
 */
int bio_client::get_client_port() const
{
    struct sockaddr addr;
    socklen_t len(sizeof(addr));
    int const r(getsockname(get_socket(), &addr, &len));
    if(r != 0)
    {
        return -1;
    }
    // Note: I know the port is at the exact same location in both
    //       structures in Linux but it could change on other Unices
    switch(addr.sa_family)
    {
    case AF_INET:
        // IPv4
        return reinterpret_cast<sockaddr_in *>(&addr)->sin_port;

    case AF_INET6:
        // IPv6
        return reinterpret_cast<sockaddr_in6 *>(&addr)->sin6_port;

    default:
        return -1;

    }
    snap::NOTREACHED();
}


/** \brief Get the TCP client address.
 *
 * This function retrieve the IP address of the client (your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \return The IP address as a string.
 */
std::string bio_client::get_client_addr() const
{
    struct sockaddr addr;
    socklen_t len(sizeof(addr));
    int const r(getsockname(get_socket(), &addr, &len));
    if(r != 0)
    {
        throw tcp_client_server_runtime_error("failed reading address");
    }
    char buf[BUFSIZ];
    switch(addr.sa_family)
    {
    case AF_INET:
        inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in *>(&addr)->sin_addr, buf, sizeof(buf));
        break;

    case AF_INET6:
        inet_ntop(AF_INET6, &reinterpret_cast<struct sockaddr_in6 *>(&addr)->sin6_addr, buf, sizeof(buf));
        break;

    default:
        throw tcp_client_server_runtime_error("unknown address family");

    }
    return buf;
}


/** \brief Read data from the socket.
 *
 * A TCP socket is a stream type of socket and one can read data from it
 * as if it were a regular file. This function reads \p size bytes and
 * returns. The function returns early if the server closes the connection.
 *
 * If your socket is blocking, \p size should be exactly what you are
 * expecting or this function will block forever or until the server
 * closes the connection.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \warning
 * When the function returns zero, it is likely that the server closed
 * the connection. It may also be that the buffer was empty and that
 * the BIO decided to return early. Since we use a blocking mechanism
 * by default, that should not happen.
 *
 * \todo
 * At this point, I do not know for sure whether errno is properly set
 * or not. It is not unlikely that the BIO library does not keep a clean
 * errno error since they have their own error management.
 *
 * \param[in,out] buf  The buffer where the data is read.
 * \param[in] size  The size of the buffer.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 *
 * \sa read_line()
 * \sa write()
 */
int bio_client::read(char *buf, size_t size)
{
    int r(static_cast<int>(BIO_read(f_bio.get(), buf, size)));
    if(r <= -2)
    {
        // the BIO is not implemented
        // XXX: do we have to set errno?
        ERR_print_errors_fp(stderr);
        return -1;
    }
    if(r == -1 || r == 0)
    {
        if(BIO_should_retry(f_bio.get()))
        {
            return 0;
        }
        // the BIO generated an error (TBD should we check BIO_eof() too?)
        // XXX: do we have to set errno?
        ERR_print_errors_fp(stderr);
        return -1;
    }
    return r;
}


/** \brief Read one line.
 *
 * This function reads one line from the current location up to the next
 * '\\n' character. We do not have any special handling of the '\\r'
 * character.
 *
 * The function may return 0 (an empty string) when the server closes
 * the connection.
 *
 * \warning
 * A return value of zero can mean "empty line" and not end of file. It
 * is up to you to know whether your protocol allows for empty lines or
 * not. If so, you may not be able to make use of this function.
 *
 * \param[out] line  The resulting line read from the server. The function
 *                   first clears the contents.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 *         If the function returns 0 or more, then the \p line parameter
 *         represents the characters read on the network.
 *
 * \sa read()
 */
int bio_client::read_line(std::string& line)
{
    line.clear();
    int len(0);
    for(;;)
    {
        char c;
        int r(read(&c, sizeof(c)));
        if(r <= 0)
        {
            return len == 0 && r < 0 ? -1 : len;
        }
        if(c == '\n')
        {
            return len;
        }
        ++len;
        line += c;
    }
}


/** \brief Write data to the socket.
 *
 * A BIO socket is a stream type of socket and one can write data to it
 * as if it were a regular file. This function writes \p size bytes to
 * the socket and then returns. This function returns early if the server
 * closes the connection.
 *
 * If your socket is not blocking, less than \p size bytes may be written
 * to the socket. In that case you are responsible for calling the function
 * again to write the remainder of the buffer until the function returns
 * a number of bytes written equal to \p size.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \todo
 * At this point, I do not know for sure whether errno is properly set
 * or not. It is not unlikely that the BIO library does not keep a clean
 * errno error since they have their own error management.
 *
 * \param[in] buf  The buffer with the data to send over the socket.
 * \param[in] size  The number of bytes in buffer to send over the socket.
 *
 * \return The number of bytes that were actually accepted by the socket
 * or -1 if an error occurs.
 *
 * \sa read()
 */
int bio_client::write(const char *buf, size_t size)
{
    int r(static_cast<int>(BIO_write(f_bio.get(), buf, size)));
    if(r <= -2)
    {
        // the BIO is not implemented
        // XXX: do we have to set errno?
        return -1;
    }
    if(r == -1 || r == 0)
    {
        if(BIO_should_retry(f_bio.get()))
        {
            return 0;
        }
        // the BIO generated an error (TBD should we check BIO_eof() too?)
        // XXX: do we have to set errno?
        return -1;
    }
    BIO_flush(f_bio.get());
    return r;
}


} // namespace tcp_client_server
// vim: ts=4 sw=4 et
