/*
 * Text:
 *      QCassandraQuery.cpp
 *
 * Description:
 *      Handling of the CQL interface.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2016 Made to Order Software Corp.
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

#include "QtCassandra/QCassandraSession.h"
#include "CassTools.h"

#include "cassandra.h"

#include <iostream>
#include <stdexcept>
#include <sstream>

#include <QtCore>

/** \class QCassandraSession
 * \brief Creates and maintains a CQL session with the Cassandra server
 *
 * This class handles such things as the connection to the Cassandra
 * server and hiding all of the cassandra-cpp library interface.
 *
 * The interface does not seem to manage lifetimes of objects it creates,
 * so we put in many smart pointers with customer deleters to make sure
 * that object are returned to the free store upon destruction.
 *
 * Also, this class, in conjuction with QCassandraQuery, provide a set of façades
 * to hide and encapuslate the details of the cassandra-cpp driver. This allows
 * us to use the CQL interface seemlessly, but without having to worry about
 * object lifetimes and garbage collection.
 *
 * \sa QCassandraQuery
 */


namespace QtCassandra
{



/** \brief Initialize a QCassandraSession object
 *
 */
QCassandraSession::QCassandraSession()
    // f_cluster
    // f_session
    // f_connection
{
}

/** \brief Clean up the QCassandraSession object.
 *
 */
QCassandraSession::~QCassandraSession()
{
    disconnect();
}


QCassandraSession::pointer_t QCassandraSession::create()
{
    // Can't use make_shared<> here
    //
    return pointer_t( new QCassandraSession );
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
void QCassandraSession::connect( const QString& host, const int port, const bool use_ssl )
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
void QCassandraSession::connect( const QStringList& host_list, const int port, const bool use_ssl )
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
    f_cluster.reset( cass_cluster_new(), CassTools::clusterDeleter() );
    cass_cluster_set_contact_points( f_cluster.get(),
                                     host_list.join(",").toUtf8().data() );

    cass_cluster_set_port                        ( f_cluster.get(), port );
    cass_cluster_set_request_timeout             ( f_cluster.get(), static_cast<unsigned>(f_timeout) );
    cass_cluster_set_write_bytes_high_water_mark ( f_cluster.get(), f_high_water_mark );
    cass_cluster_set_write_bytes_low_water_mark  ( f_cluster.get(), f_low_water_mark  );

    // Attach the SSL server trusted certificate if
    // it exists.
    //
    if( f_ssl )
    {
        cass_cluster_set_ssl( f_cluster.get(), f_ssl.get() );
    }

    // Create the session now, and create a connection.
    //
    f_session.reset( cass_session_new(), CassTools::sessionDeleter() );
    f_connection.reset(
        cass_session_connect( f_session.get(), f_cluster.get() ),
        CassTools::futureDeleter() );

    // This operation will block until the result is ready
    //
    CassError rc = cass_future_error_code( f_connection.get() );
    if ( rc != CASS_OK )
    {
        const char *message;
        size_t message_length;
        cass_future_error_message( f_connection.get(), &message,
                                   &message_length );
        std::stringstream msg;
        msg << "Cannot connect to cassandra server! Reason=["
            << std::string( message ) << "]";

        f_connection.reset();
        f_session.reset();
        f_cluster.reset();
        throw std::runtime_error( msg.str().c_str() );
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
void QCassandraSession::disconnect()
{
    f_connection.reset();
    //
    if ( f_session )
    {
        CassTools::future_pointer_t result( cass_session_close( f_session.get() ),
                                 CassTools::futureDeleter() );
        cass_future_wait( result.get() );
    }
    //
    f_session.reset();
    f_cluster.reset();
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
bool QCassandraSession::isConnected() const
{
    return f_connection && f_session && f_cluster;
}


/** \brief Reset the SSL key store by deleting the CassSsl object.
 *
 * Also, remove the ssl object from the cluster if the cluster is live.
 */
void QCassandraSession::reset_ssl_keys()
{
    f_ssl.reset();
    //
    if( f_cluster )
    {
        cass_cluster_set_ssl( f_cluster.get(), nullptr );
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
void QCassandraSession::add_ssl_trusted_cert( const QString& cert )
{
    if( !f_ssl )
    {
        f_ssl.reset( cass_ssl_new(), CassTools::sslDeleter() );
        cass_ssl_set_verify_flags( f_ssl.get(), CASS_SSL_VERIFY_PEER_CERT | CASS_SSL_VERIFY_PEER_IDENTITY );
    }

    // Add the trusted certificate (or chain) to the driver
    //
    CassError rc = cass_ssl_add_trusted_cert_n
        ( f_ssl.get()
        , cert.toUtf8().data()
        , cert.size()
        );
    if( rc != CASS_OK )
    {
        std::stringstream msg;
        msg << "Error loading SSL certificate: "
            << cass_error_desc(rc);
        throw std::runtime_error( msg.str().c_str() );
    }
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
void QCassandraSession::add_ssl_cert_file( const QString& filename )
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
QString const& QCassandraSession::get_keys_path() const
{
    return f_keys_path;
}


/** \brief Set the path from where the SSL keys are to be read.
 *
 * \sa get_keys_path(), add_ssl_keys()
 */
void QCassandraSession::set_keys_path( QString const& path )
{
    f_keys_path = path;
}


/** \brief Add each trusted certificate available to the CassSsl object.
 *
 * Iterates the keys path and adds each ".pem" file found.
 *
 * \sa set_keys_path()
 */
void QCassandraSession::add_ssl_keys()
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
CassTools::cluster_pointer_t QCassandraSession::cluster() const
{
    return f_cluster;
}

/** \brief Return a smart pointer to the cassandra-cpp session object.
 */
CassTools::session_pointer_t QCassandraSession::session() const
{
    return f_session;
}

/** \brief Return a smart pointer to the cassandra-cpp connection future object.
 */
CassTools::future_pointer_t QCassandraSession::connection() const
{
    return f_connection;
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
CassTools::timeout_t QCassandraSession::timeout() const
{
    return f_timeout;
}


/** \brief Change the current timeout of CQL requests.
 *
 * WARNING: In the snapdbproxy the request timeout is only implemented
 *          for QtCassandra::QCassandraOrder::TYPE_OF_RESULT_SUCCESS.
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
 *          QtCassandra::QCassandraRequestTimeout safe_timeout(session, 5 * 60 * 1000); // 5 minutes
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
 *      session = QtCassandra::QCassandraSession::create();
 *      {
 *          guard lock(some_mutex);
 *
 *          QtCassandra::QCassandraRequestTimeout safe_timeout(session, 5 * 60 * 1000);
 *          session->connect(...);
 *      }
 * \endcode
 *
 * \param[in] timeout_ms  The new timeout in milliseconds.
 *
 * \return The old timeout.
 */
int64_t QCassandraSession::setTimeout(CassTools::timeout_t timeout_ms)
{
    CassTools::timeout_t const old_timeout(f_timeout);
    f_timeout = timeout_ms;
    // the cluster may not yet have been allocated
    if(f_cluster.get())
    {
//std::cerr << "*** setting cluster " << f_cluster.get() << " timeout to: " << f_timeout << "\n";
        cass_cluster_set_request_timeout(f_cluster.get(), static_cast<unsigned>(f_timeout));
    }
    return old_timeout;
}


uint32_t QCassandraSession::highWaterMark() const
{
    return f_high_water_mark;
}


uint32_t QCassandraSession::lowWaterMark() const
{
    return f_low_water_mark;
}

void QCassandraSession::setHighWaterMark( uint32_t val )
{
    f_high_water_mark = val;
    if( f_cluster )
    {
        cass_cluster_set_write_bytes_high_water_mark( f_cluster.get(), f_high_water_mark );
    }
}


void QCassandraSession::setLowWaterMark( uint32_t val )
{
    f_low_water_mark = val;
    if( f_cluster )
    {
        cass_cluster_set_write_bytes_low_water_mark( f_cluster.get(), f_low_water_mark );
    }
}


} // namespace QtCassandra

// vim: ts=4 sw=4 et
