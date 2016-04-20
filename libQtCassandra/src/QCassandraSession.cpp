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

using namespace CassTools;


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
 * \param[in] host      The host, defaults to "localhost" (an IP address,
 * computer
 *                      hostname, domain name, etc.)
 * \param[in] port      The connection port, defaults to 9042.
 * \param[in] password  Whether the connection makes use of encryption and a
 *                      password (if password is not an empty string).
 *
 * \return throws std::runtime_error on failure
 */
void QCassandraSession::connect( const QString& host, const int port )
{
    QStringList host_list;
    host_list << host;
    connect( host_list, port );
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
 *
 * \return throws std::runtime_error on failure
 */
void QCassandraSession::connect( const QStringList& host_list, const int port )
{
    // disconnect any existing connection
    disconnect();

    f_cluster.reset( cass_cluster_new(), clusterDeleter() );
    cass_cluster_set_contact_points( f_cluster.get(),
                                     host_list.join(",").toUtf8().data() );

    cass_cluster_set_port( f_cluster.get(), port );
    //
    f_session.reset( cass_session_new(), sessionDeleter() );
    f_connection.reset(
        cass_session_connect( f_session.get(), f_cluster.get() ),
        futureDeleter() );

    /* This operation will block until the result is ready */
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
        future_pointer_t result( cass_session_close( f_session.get() ),
                                 futureDeleter() );
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


/** \brief Return a smart pointer to the cassandra-cpp cluster object.
 */
cluster_pointer_t QCassandraSession::cluster() const
{
    return f_cluster;
}

/** \brief Return a smart pointer to the cassandra-cpp session object.
 */
session_pointer_t QCassandraSession::session() const
{
    return f_session;
}

/** \brief Return a smart pointer to the cassandra-cpp connection future object.
 */
future_pointer_t QCassandraSession::connection() const
{
    return f_connection;
}


} // namespace QtCassandra

// vim: ts=4 sw=4 et
