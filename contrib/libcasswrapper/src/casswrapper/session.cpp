/*
 * Text:
 *      src/session.cpp
 *
 * Description:
 *      Handling of the CQL interface.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2017 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
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

#include "casswrapper/session.h"
#include "casswrapper_impl.h"
#include "exception_impl.h"

#include <libexcept/exception.h>

#include "cassandra.h"

#include <iostream>
#include <stdexcept>
#include <sstream>

#include <QtCore>

/** \class session
 * \brief Creates and maintains a CQL session with the Cassandra server
 *
 * This class handles such things as the connection to the Cassandra
 * server and hiding all of the cassandra-cpp library interface.
 *
 * The interface does not seem to manage lifetimes of objects it creates,
 * so we put in many smart pointers with customer deleters to make sure
 * that object are returned to the free store upon destruction.
 *
 * Also, this class, in conjuction with query, provide a set of façades
 * to hide and encapuslate the details of the cassandra-cpp driver. This allows
 * us to use the CQL interface seemlessly, but without having to worry about
 * object lifetimes and garbage collection.
 *
 * \sa query
 */


namespace casswrapper
{


struct data
{
    std::unique_ptr<cluster>    f_cluster;
    std::unique_ptr<session>    f_session;
    std::unique_ptr<ssl>        f_ssl;
    std::unique_ptr<future>     f_connection;
};


/** \brief Initialize a session object
 *
 */
Session::Session()
    : f_data(std::make_unique<data>())
{
}

/** \brief Clean up the Session object.
 *
 */
Session::~Session()
{
    disconnect();
}


Session::pointer_t Session::create()
{
    // Can't use make_shared<> here
    //
    return pointer_t( new Session );
}


/** \brief Connect to a Cassandra Cluster.
 *
 * This function connects to a Cassandra Cluster. Which cluster is determined
 * by the host and port parameters.
 *
 * One cluster may include many database contexts (i.e. keyspaces.) Each
 * database
 * context (keyspace) has a set of parameters defining its duplication mechanism
 * among other things. Before working with a database context, one must call the
 * the setCurrentContext() function.
 *
 * The function first disconnects the existing connection when there is one.
 *
 * Many other functions require you to call this connect() function first. You
 * are likely to get a runtime exception if you don't.
 *
 * Note that the previous connection is lost whether or not the new one
 * succeeds.
 *
 * \param[in] host      The host, defaults to "localhost" (an IP address, computer
 *                      hostname, domain name, etc.)
 * \param[in] port      The connection port, defaults to 9042.
 * \param[in] password  Whether the connection makes use of encryption and a
 *                      password (if password is not an empty string).
 * \param[in] use_ssl   Turn on SSL for connections with Cassandra.
 *
 * \sa connect(), add_ssl_keys()
 *
 * \return throws std::runtime_error on failure
 */
void Session::connect( const QString& host, const int port, const bool use_ssl )
{
    QStringList host_list;
    host_list << host;
    connect( host_list, port, use_ssl );
}


/** \brief Connect to a Cassandra Cluster.
 *
 * This function connects to a Cassandra Cluster. Which cluster is determined
 * by the host and port parameters.
 *
 * One cluster may include many database contexts (i.e. keyspaces.) Each
 * database
 * context (keyspace) has a set of parameters defining its duplication mechanism
 * among other things. Before working with a database context, one must call the
 * the setCurrentContext() function.
 *
 * The function first disconnects the existing connection when there is one.
 *
 * Many other functions require you to call this connect() function first. You
 * are likely to get a runtime exception if you don't.
 *
 * Note that the previous connection is lost whether or not the new one
 * succeeds.
 *
 * \param[in] host_list The list of hosts, AKA contact points (IP addresses,
 *                      computer hostnames, domain names, etc.)
 * \param[in] port      The connection port, defaults to 9042.
 * \param[in] use_ssl   Turn on SSL for connections with Cassandra.
 *
 * \sa connect(), add_ssl_keys()
 *
 * \return throws std::runtime_error on failure
 */
void Session::connect( const QStringList& host_list, const int port, const bool use_ssl )
{
    // disconnect any existing connection
    //
    disconnect();

    // Make sure we add keys if we want SSL.
    //
    if( use_ssl )
    {
        add_ssl_keys();
    }

    // Create the cluster and specify settings.
    //
    f_data->f_cluster = std::make_unique<cluster>();
    f_data->f_cluster->set_contact_points( host_list.join(",") );

    f_data->f_cluster->set_port                        ( port              );
    f_data->f_cluster->set_request_timeout             ( f_timeout         );
    f_data->f_cluster->set_write_bytes_high_water_mark ( f_high_water_mark );
    f_data->f_cluster->set_write_bytes_low_water_mark  ( f_low_water_mark  );

    // Attach the SSL server trusted certificate if
    // it exists.
    //
    if( f_data->f_ssl )
    {
        f_data->f_cluster->set_ssl( *(f_data->f_ssl) );
    }

    // Create the session now, and create a connection.
    //
    f_data->f_session    = std::make_unique<session>();
    f_data->f_connection = std::make_unique<future>(*f_data->f_session, *f_data->f_cluster);

    // This operation will block until the result is ready
    //
    CassError rc = f_data->f_connection->get_error_code();
    if ( rc != CASS_OK )
    {
        QString const message( f_data->f_connection->get_error_message() );
        std::stringstream msg;
        msg << "Cannot connect to cassandra server! Reason=["
            << std::string( message.toUtf8().data() ) << "]";

        f_data->f_connection.reset();
        f_data->f_session.reset();
        f_data->f_cluster.reset();
        throw libexcept::exception_t( msg.str() );
    }
}


/** \brief Break the connection to Cassandra.
 *
 * This function breaks the connection to Cassandra.
 *
 * This function has the side effect of clearing the cluster name,
 * protocol version, and current context.
 *
 * The function does not clear the default consistency level or
 * the default time out used by the schema synchronization. Those
 * can be changed by calling their respective functions.
 */
void Session::disconnect()
{
    f_data->f_connection.reset();
    //
    if( f_data->f_session )
    {
        f_data->f_session->close().wait();
    }
    //
    f_data->f_session.reset();
    f_data->f_cluster.reset();
}

/** \brief Check whether the object is connected to the server.
 *
 * This function returns true when this object is connected to the
 * backend Cassandra server.
 *
 * The function is fast and does not actually verify that the TCP/IP
 * connection is still up.
 *
 * \return true if connect() was called and succeeded.
 */
bool Session::isConnected() const
{
    return f_data->f_connection.get() != nullptr;
}


/** \brief Reset the SSL key store by deleting the CassSsl object.
 *
 * Also, remove the ssl object from the cluster if the cluster is live.
 */
void Session::reset_ssl_keys()
{
    f_data->f_ssl.reset();
    if( f_data->f_cluster )
    {
        f_data->f_cluster->reset_ssl();
    }
}


/** \brief Add trusted certificate (public SSL key)
 *
 * This adds a certificate to the CassSsl object for the session.
 *
 * If the CassSsl object has not been created yet, then it is created
 * first. When the session is connected is when it is added into the
 * session.
 */
void Session::add_ssl_trusted_cert( const QString& cert )
{
    if( !f_data->f_ssl )
    {
        f_data->f_ssl = std::make_unique<ssl>();
    }

    // Add the trusted certificate (or chain) to the driver
    //
    f_data->f_ssl->add_trusted_cert( cert );
}


/** \brief Add trusted SSL cert file to SSL object.
 *
 * This adds a certificate from file to the CassSsl object for the session.
 *
 * If the CassSsl object has not been created yet, then it is created
 * first. When the session is connected is when it is added into the
 * session.
 *
 * /sa add_ssl_trusted_cert()
 */
void Session::add_ssl_cert_file( const QString& filename )
{
    QFile file( filename );
    if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        std::stringstream msg;
        msg << "Cannot open cert file '"
            << filename.toUtf8().data()
            << "'!";
        throw std::runtime_error( msg.str().c_str() );
    }

    QTextStream in(&file);
    add_ssl_trusted_cert( in.readAll() );
}


/** \brief Return the current value of the path to the SSL keys.
 *
 * \sa set_keys_path()
 */
QString const& Session::get_keys_path() const
{
    return f_keys_path;
}


/** \brief Set the path from where the SSL keys are to be read.
 *
 * \sa get_keys_path(), add_ssl_keys()
 */
void Session::set_keys_path( QString const& path )
{
    f_keys_path = path;
}


/** \brief Add each trusted certificate available to the CassSsl object.
 *
 * Iterates the keys path and adds each ".pem" file found.
 *
 * \sa set_keys_path()
 */
void Session::add_ssl_keys()
{
    reset_ssl_keys();

    QDir keys_path;
    keys_path.setPath( f_keys_path );
    keys_path.setNameFilters( { "*.pem" } );
    keys_path.setSorting( QDir::Name );
    keys_path.setFilter( QDir::Files );

    for( QFileInfo const &info : keys_path.entryInfoList() )
    {
        try
        {
            add_ssl_cert_file( info.filePath() );
        }
        catch( const std::exception& x )
        {
            std::cerr << "snapdb: could not add ssl keys! Error: " << x.what() << std::endl;
            throw;
        }
    }
}


/** \brief Return a smart pointer to the cassandra-cpp cluster object.
 */
cluster Session::getCluster() const
{
    if( !f_data->f_cluster )
    {
        throw libexcept::exception_t( "The cluster is not connected!" );
    }

    return *(f_data->f_cluster);
}

/** \brief Return a smart pointer to the cassandra-cpp session object.
 */
session Session::getSession() const
{
    if( !f_data->f_session )
    {
        throw libexcept::exception_t( "The session is not connected!" );
    }

    return *(f_data->f_session);
}

/** \brief Return a smart pointer to the cassandra-cpp connection future object.
 */
future Session::getConnection() const
{
    if( !isConnected() )
    {
        throw libexcept::exception_t( "The cluster/session is not connected!" );
    }

    return *(f_data->f_connection);
}


/** \brief Return the current request timeout.
 *
 * This function returns the timeout for the next CQL requests.
 *
 * The setTimeout() function manages the timeout in such a way that
 * only the largest one is kept while running. Others are kept around,
 * but they do not apply until the largest one is removed and they
 * eventually themselves become the largest one.
 *
 * It is very much possible that some setTimeout() call never have
 * any effect since a larger setTimeout() is in effect while they
 * themselves run.
 *
 * \note
 * At this time we only use this mechanism when we create a table
 * so there should be no clash, except that multiple threads could
 * all attempt to create a table and RAII would not be enough to
 * know whether to keep the largest timeout or put the default
 * back in.
 *
 * \return The timeout amount.
 */
casswrapper::timeout_t Session::timeout() const
{
    return f_timeout;
}


/** \brief Change the current timeout of CQL requests.
 *
 * WARNING: In the snapdbproxy the request timeout is only implemented
 *          for casswrapper::QCassandraOrder::TYPE_OF_RESULT_SUCCESS.
 *          If you are using sessions directly, make sure to create
 *          a new session after this change!
 *
 * This function changes the timeout for the next CQL requests.
 *
 * Because the timeout is shared between all requests and all threads
 * that currently run against the Cassandra C++ driver, the function
 * makes sure to use the largest value that has been specified so far.
 *
 * You may "remove" your timeout amount by calling the function again
 * with timeout_ms set to -1. For example:
 *
 * \code
 *      CassTools::timeout_t const old_timeout(session->setTimeout(5 * 60 * 1000)); // 5 minutes
 *      ... do some work ...
 *      session->setTimeout(old_timeout); // restore
 * \endcode
 *
 * It is strongly advised that you make use of the QCassandraRequestTimeout
 * class in order to do such changes to make sure that your timeout is
 * always removed once you are done with your work (i.e. RAII, exception
 * safe code):
 *
 * \code
 *      {
 *          casswrapper::QCassandraRequestTimeout safe_timeout(session, 5 * 60 * 1000); // 5 minutes
 *          ... do some work ...
 *      }
 * \endcode
 *
 * \warning
 * This value is not multi-thread protected. Since you need to change it
 * just for the time you connect a session, you can protect that part
 * if you are using threads:
 *
 * \code
 *      session = casswrapper::Session::create();
 *      {
 *          guard lock(some_mutex);
 *
 *          casswrapper::QCassandraRequestTimeout safe_timeout(session, 5 * 60 * 1000);
 *          session->connect(...);
 *      }
 * \endcode
 *
 * \param[in] timeout_ms  The new timeout in milliseconds.
 *
 * \return The old timeout.
 */
int64_t Session::setTimeout(timeout_t timeout_ms)
{
    timeout_t const old_timeout(f_timeout);
    f_timeout = timeout_ms;

    // the cluster may not yet have been allocated
    //
    if( f_data->f_cluster )
    {
        f_data->f_cluster->set_request_timeout( f_timeout );
    }
    return old_timeout;
}


uint32_t Session::highWaterMark() const
{
    return f_high_water_mark;
}


uint32_t Session::lowWaterMark() const
{
    return f_low_water_mark;
}

void Session::setHighWaterMark( uint32_t val )
{
    f_high_water_mark = val;

    if( f_data->f_cluster )
    {
        f_data->f_cluster->set_write_bytes_high_water_mark( f_high_water_mark );
    }
}


void Session::setLowWaterMark( uint32_t val )
{
    f_low_water_mark = val;

    if( f_data->f_cluster )
    {
        f_data->f_cluster->set_write_bytes_low_water_mark( f_low_water_mark );
    }
    
}


} // namespace casswrapper

// vim: ts=4 sw=4 et
