/*
 * Text:
 *      libcasswrapper/src/casswrapper/session.h
 *
 * Description:
 *      Handling of the connection to the Cassandra database via the
 *      cassandra-cpp-driver API.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
 * 
 *      https://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <map>
#include <memory>
#include <string>

#include <QString>
#include <QByteArray>

#include <unistd.h>

#include "casswrapper/exception.h"

namespace casswrapper
{


struct data;
typedef int64_t timeout_t;

class cluster;
class future;
class session;


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class Session
    : public std::enable_shared_from_this<Session>
{
public:
    typedef std::shared_ptr<Session> pointer_t;

    static timeout_t const  DEFAULT_TIMEOUT = 12 * 1000; // 12s

    static pointer_t        create();
                            ~Session();

    void                    connect( const QString & host = "localhost"
                                   , const int port = 9042
                                   , const bool use_ssl = true );
    void                    connect( const QStringList & host_list
                                   , const int port = 9042
                                   , const bool use_ssl = true );
    void                    disconnect();
    bool                    isConnected() const;

    QString const &         get_keys_path() const;
    void                    set_keys_path( QString const& path );

    void                    add_ssl_trusted_cert( const QString& cert     );
    void                    add_ssl_cert_file   ( const QString& filename );

    cluster                 getCluster()    const;
    session                 getSession()    const;
    future                  getConnection() const;

    timeout_t               timeout() const;
    timeout_t               setTimeout(timeout_t timeout_ms);

private:
                            Session();

    void                    reset_ssl_keys();
    void                    add_ssl_keys();

    std::unique_ptr<data>   f_data;
    //
    timeout_t               f_timeout         = DEFAULT_TIMEOUT;                         // 12s
    QString                 f_keys_path       = QString("/var/lib/snapwebsites/cassandra-keys/");
};
#pragma GCC diagnostic pop


class request_timeout
{
public:
    typedef std::shared_ptr<request_timeout> pointer_t;

    request_timeout(Session::pointer_t session, timeout_t timeout_ms)
        : f_session(session)
        , f_old_timeout(f_session->setTimeout(timeout_ms))
    {
    }

    ~request_timeout()
    {
        f_session->setTimeout(f_old_timeout);
    }

private:
    Session::pointer_t    f_session     = Session::pointer_t();
    timeout_t             f_old_timeout = Session::DEFAULT_TIMEOUT;
};


}
// namespace casswrapper

// vim: ts=4 sw=4 et
