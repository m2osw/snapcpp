/*
 * Text:
 *      snapdbproxy_connection.cpp
 *
 * Description:
 *      Each connection is managed by a thread. This file implements that
 *      thread. The thread lasts as long as the connection. Once the connect
 *      gets closed by the client, the thread terminates.
 *
 *      TODO: we certainly want to look into reusing threads in a pool
 *            instead of having a onetime run like we have now.
 *
 * License:
 *      Copyright (c) 2016 Made to Order Software Corp.
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

// ourselves
//
#include "snapdbproxy.h"

// our lib
//
#include "log.h"
#include "not_used.h"
#include "qstring_stream.h"
#include "dbutils.h"

// 3rd party libs
//
#include <QtCore>
#include <QtCassandra/QCassandra.h>
#include <QtCassandra/QCassandraSchema.h>
#include <advgetopt/advgetopt.h>

// system (C++)
//
#include <algorithm>
#include <iostream>
#include <sstream>

// OS libs
//
#include <sys/ioctl.h>
#include <sys/syscall.h>


// a mutex to manage data common to all connections
//
snap::snap_thread::snap_mutex  g_connections_mutex;

// the DESCRIBE CLUSTER is very slow, this is a cached version which
// is reset once in a while when certain orders happen (i.e. create
// remove a context, table, or alter a context, table, column.)
//
QByteArray  g_cluster_description;


void signalfd_deleted(int s)
{
    close(s);
}


int64_t timeofday()
{
    struct timeval tv;

    // we ignore timezone as it can also generate an error
    if(gettimeofday( &tv, NULL ) != 0)
    {
        throw std::runtime_error("gettimeofday() failed.");
    }

    return static_cast<int64_t>( tv.tv_sec ) * 1000000 +
           static_cast<int64_t>( tv.tv_usec );
}


pid_t gettid()
{
    return syscall(SYS_gettid);
}





snapdbproxy_connection::snapdbproxy_connection(QtCassandra::QCassandraSession::pointer_t session, int & s, QString const & cassandra_host_list, int cassandra_port)
    : snap_runner("snapdbproxy_connection")
    //, f_proxy()
    , f_session(session)
    //, f_cursors() -- auto-init
    , f_socket(s)
    , f_cassandra_host_list(cassandra_host_list)
    , f_cassandra_port(cassandra_port)
{
    // the parent (main) thread will shutdown the socket if it receives
    // the STOP message from snapcommunicator
}


snapdbproxy_connection::~snapdbproxy_connection()
{
    // WARNING: do not close f_socket here, our parent (snapdbproxy_thread)
    //          takes care of that

    // TODO:
    // Note that on errors we still force close the socket, and we may
    // want to anyway revisit this scheme because the parent destructor
    // only happens when another connection comes in... and that means
    // we keep that socket opened for all that time, whether our thread
    // died a long time ago or not.
}


void snapdbproxy_connection::run()
{
    // let the other process push the whole order before moving forward
    //sched_yield();

    int const socket_on_entry(f_socket);
    SNAP_LOG_TRACE("starting new snapdbproxy connection thread (")(f_socket)(").");

    try
    {
        do
        {
            // wait for an order
            //
            QtCassandra::QCassandraOrder order(f_proxy.receiveOrder(*this));
            if(order.validOrder()
            && f_session->isConnected())
            // && !thread->is_stopping()) -- we do not have access to the thread
            //                               and the pthread_kill() should be more
            //                               than enough at this point
            {
                // order can be executed now
                //
//SNAP_LOG_WARNING("got an order: ")(static_cast<int>(order.get_type_of_result()))(", CQL \"")(order.cql())("\" (")(order.columnCount())(" columns).");
//std::cerr << "[" << getpid() << ":" << gettid() << "] got new order \"" << order.cql() << "\" in snapdbproxy at " << timeofday() << "\n";
                switch(order.get_type_of_result())
                {
                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_CLOSE:
                    close_cursor(order);
                    break;

                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_DECLARE:
                    declare_cursor(order);
                    break;

                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_DESCRIBE:
                    describe_cluster(order);
                    break;

                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_FETCH:
                    fetch_cursor(order);
                    break;

                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_ROWS:
                    read_data(order);
                    break;

                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_SUCCESS:
                    execute_command(order);
                    break;

                }

                // the order may include the flag telling us that the
                // cluster schema may have changed and if so we have
                // to clear our memory cache
                //
                if(order.clearClusterDescription())
                {
                    clear_cluster_description();
                }
            }
            else
            {
                // in most cases if the order is not valid the connect
                // was hung up; it could also be an invalid protocol
                // or some transmission error (although really, with
                // TCP/IP transmission errors rarely happen.)
                //
                if(order.validOrder())
                {
                    SNAP_LOG_TRACE("snapdbproxy connection socket is gone (")(f_socket)(").");
                }
                else
                {
                    SNAP_LOG_TRACE("snapdbproxy received an invalid order (")(f_socket)(").");
                }

                snap::NOTUSED(::close(f_socket));
                f_socket = -1;
            }
        }
        while(f_socket != -1);
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_WARNING("thread received std::exception \"")(e.what())("\"");
    }
    catch(...)
    {
        SNAP_LOG_WARNING("thread received an unknown exception");
    }
    // exit thread normally

    SNAP_LOG_TRACE("ending snapdbproxy connection thread (")(socket_on_entry)(").");
}


/** \brief Read count bytes to the specified buffer.
 *
 * This function reads \p count bytes from the socket managed by
 * this connection. The bytes are saved in the specified \p buf
 * buffer.
 *
 * If an error occurs before any data was read, the function returns
 * -1.
 *
 * If data was already read and an error occurs, the function returns
 * the number of bytes already read.
 *
 * If no error occurs, the function reads all the data from the socket
 * and returns \p count.
 *
 * \note
 * If the thread receives the SIGUSR1 signal, it is considered to be an
 * error. The function will stop immediately returning the number of
 * bytes already read. If nothing was read, then the function
 * returns -1 and sets errno to ECANCEL.
 *
 * \exception std::logic_error
 * This function raises this error if somehow it detects a size
 * mistmatch. This exception should never happen.
 *
 * \param[in] buf  A pointer to a buffer of data to send to the client.
 * \param[in] count  The number of bytes to write to the client.
 *
 * \return The number of bytes written or -1 if no bytes get written and
 *         an error occurs.
 */
ssize_t snapdbproxy_connection::read(void * buf, size_t count)
{
    if(f_socket == -1)
    {
        return -1L;
    }

    if(count == 0)
    {
        return 0;
    }

    // we are supposed to have a blocking socket, but with large amounts
    // of data the read() may return less than count bytes, for this
    // reason we have to have a loop
    //
    size_t size(0);
    for(;;)
    {
        ssize_t const r(::read(f_socket, buf, count));
        if(r < 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("snapdbproxy_connection::read() returned with ")(r)(", errno ")(e)(", ")(strerror(e));
            // TBD: we could "return size > 0 ? size : -1L;" too...
            return -1L;
        }
        if(r > 0)
        {
            count -= r;
            size += r;
            if(count == 0)
            {
                return size;
            }
            buf = reinterpret_cast<char *>(buf) + r;
            SNAP_LOG_TRACE("snapdbproxy_connection::read() needs more than one call (")(count)("/")(size)(").");
        }
        else
        {
            // wait a bit and try again
            // (this is when we receive a return value of 0)
            //
            struct pollfd fd;
            fd.fd = f_socket;
            fd.events = POLLIN | POLLPRI | POLLRDHUP | POLLHUP;
            snap::NOTUSED(poll(&fd, 1, 0));
            if((fd.revents & (POLLHUP | POLLRDHUP)) != 0)
            {
                // this happens all the time so we just use a trace on it
                // (at first it was an error)
                SNAP_LOG_TRACE("snapdbproxy_connection::read() attempted to read from a socket that is closed.");
                return -1L;
            }
        }
    }
}


/** \brief Write the count bytes of the specified buffer.
 *
 * This function writes the specified buffer to the socket managed by
 * this connection. The number of bytes written to the socket is
 * specified by count.
 *
 * If an error occurs before any data was written, the function returns
 * -1.
 *
 * If data was already written and an error occurs, the function returns
 * the number of bytes already written.
 *
 * If no error occurs, the function writes all the data to the socket
 * and returns \p count.
 *
 * \note
 * If the thread receives the SIGUSR1 signal, it is considered to be an
 * error. The function will stop immediately returning the number of
 * bytes already written. If nothing was written, then the function
 * returns -1 and sets errno to ECANCEL.
 *
 * \exception std::logic_error
 * This function raises this error if somehow it detects a size
 * mistmatch. This exception should never happen.
 *
 * \param[in] buf  A pointer to a buffer of data to send to the client.
 * \param[in] count  The number of bytes to write to the client.
 *
 * \return The number of bytes written or -1 if no bytes get written and
 *         an error occurs.
 */
ssize_t snapdbproxy_connection::write(void const * buf, size_t count)
{
    if(f_socket == -1)
    {
        return -1L;
    }

    if(count == 0)
    {
        return 0;
    }

    // we are supposed to have a blocking socket, but with large amounts
    // of data the write() may accept less than count bytes, for this
    // reason we have to have a loop
    //
    size_t size(0);
    for(;;)
    {
        ssize_t const r(::write(f_socket, buf, count));
        if(r < 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("snapdbproxy_connection::write() returned with ")(r)(", errno ")(e)(", ")(strerror(e));
            // TBD: we could "return size > 0 ? size : -1L;" too...
            return -1L;
        }
        if(r > 0)
        {
            count -= r;
            size += r;
            if(count == 0)
            {
                return size;
            }
            buf = reinterpret_cast<char const *>(buf) + r;
            SNAP_LOG_TRACE("snapdbproxy_connection::write() needs more than one call (")(count)("/")(size)(").");
        }
        else
        {
            // wait a bit and try again
            // (this is when we receive a return value of 0)
            //
            struct pollfd fd;
            fd.fd = f_socket;
            fd.events = POLLOUT | POLLRDHUP | POLLHUP;
            snap::NOTUSED(poll(&fd, 1, 0));
            if((fd.revents & (POLLHUP | POLLRDHUP)) != 0)
            {
                SNAP_LOG_ERROR("snapdbproxy_connection::write() attempted to write to a socket that is closed.");
                return -1L;
            }
        }
    }
}


void snapdbproxy_connection::kill()
{
    // parent thread wants to quit, tell the child to exit ASAP
    // by partially shutting down the socket
    //
    snap::NOTUSED(::shutdown(f_socket, SHUT_RD));
}


void snapdbproxy_connection::send_order(QtCassandra::QCassandraQuery::pointer_t q, QtCassandra::QCassandraOrder const & order)
{
    size_t const count(order.parameterCount());

    if( order.consistencyLevel() != QtCassandra::CONSISTENCY_LEVEL_QUORUM)
    {
        SNAP_LOG_WARNING("Consistency ")
                        (order.consistencyLevel())
                        (" instead of the usually expected QUORUM for [")
                        (order.cql())
                        ("]");
    }

    // CQL order
    q->query( order.cql(), count );

    // Parameters
    for(size_t idx(0); idx < count; ++idx)
    {
        q->bindByteArray( idx, order.parameter(idx) );
    }

    // Consistency Level
    q->setConsistencyLevel( order.consistencyLevel() );

    // Timestamp
    q->setTimestamp(order.timestamp());

    // Paging Size
    int32_t const paging_size(order.pagingSize());
    if(paging_size > 0)
    {
        q->setPagingSize(paging_size);
    }

    // run the CQL order
    q->start();
}


void snapdbproxy_connection::declare_cursor(QtCassandra::QCassandraOrder const & order)
{
    cursor_t cursor;
    cursor.f_query = QtCassandra::QCassandraQuery::create(f_session);
    cursor.f_column_count = order.columnCount();

    // in this case we have to keep the query so we allocate it
    //
    send_order(cursor.f_query, order);

    QtCassandra::QCassandraOrderResult result;
    QByteArray cursor_index;
    QtCassandra::appendUInt32Value(cursor_index, f_cursors.size());
    result.addResult(cursor_index);

    while(cursor.f_query->nextRow())
    {
        for(int idx(0); idx < cursor.f_column_count; ++idx)
        {
            result.addResult( cursor.f_query->getByteArrayColumn( idx ) );
        }
    }

    f_cursors.push_back(cursor);

    result.setSucceeded(true);
    if(!f_proxy.sendResult(*this, result))
    {
        snap::NOTUSED(::close(f_socket));
        f_socket = -1;
    }
}


void snapdbproxy_connection::describe_cluster(QtCassandra::QCassandraOrder const & order)
{
    snap::NOTUSED(order);

    QtCassandra::QCassandraOrderResult result;

    {
        snap::snap_thread::snap_lock lock(g_connections_mutex);

        if(g_cluster_description.isEmpty())
        {
            // load the meta data
            QtCassandra::QCassandraSchema::SessionMeta::pointer_t session_meta( QtCassandra::QCassandraSchema::SessionMeta::create(f_session) );
            session_meta->loadSchema();
            g_cluster_description = session_meta->encodeSessionMeta();
        }

        // convert the meta data to a blob and send it over the wire
        result.addResult( g_cluster_description );
    }

    result.setSucceeded(true);

    if(!f_proxy.sendResult(*this, result))
    {
        snap::NOTUSED(::close(f_socket));
        f_socket = -1;
    }
}


void snapdbproxy_connection::clear_cluster_description()
{
    snap::snap_thread::snap_lock lock(g_connections_mutex);

    g_cluster_description.clear();
}


void snapdbproxy_connection::fetch_cursor(QtCassandra::QCassandraOrder const & order)
{
    int const cursor_index(order.cursorIndex());
    if(static_cast<size_t>(cursor_index) >= f_cursors.size())
    {
        throw snap::snapwebsites_exception_invalid_parameters("cursor index is out of bounds, it may already have been closed.");
    }
    QtCassandra::QCassandraQuery::pointer_t q(f_cursors[cursor_index].f_query);
    if(!q)
    {
        throw snap::snapwebsites_exception_invalid_parameters("cursor was already closed.");
    }

    QtCassandra::QCassandraOrderResult result;

    if(q->nextPage())
    {
        // TBD: add the cursor_index on a fetch? probably not required...
        //result.addResult(...);

        int const column_count(f_cursors[cursor_index].f_column_count);
        while(q->nextRow())
        {
            for(int idx(0); idx < column_count; ++idx)
            {
                result.addResult( q->getByteArrayColumn( idx ) );
            }
        }
    }

    // send the following or an empty set (an empty set means we reached
    // the last page!)
    //
    result.setSucceeded(true);
    if(!f_proxy.sendResult(*this, result))
    {
        snap::NOTUSED(::close(f_socket));
        f_socket = -1;
    }
}


void snapdbproxy_connection::close_cursor(QtCassandra::QCassandraOrder const & order)
{
    // verify that the specified index is considered valid on this side
    //
    int const cursor_index(order.cursorIndex());
    if(static_cast<size_t>(cursor_index) >= f_cursors.size())
    {
        throw snap::snapwebsites_exception_invalid_parameters("cursor index is out of bounds.");
    }

    // send an empty, successful reply in this case
    //
    QtCassandra::QCassandraOrderResult result;
    result.setSucceeded(true);
    if(!f_proxy.sendResult(*this, result))
    {
        snap::NOTUSED(::close(f_socket));
        f_socket = -1;
    }

    // now actually do the clean up
    // (we can do that after we sent the reply since we are one separate
    // process, yet the process is fully synchronized on the TCP/IP socket)
    //
    f_cursors[cursor_index].f_query.reset();

    // remove all the cursors that were closed if possible so the
    // vector does not grow indefinitly
    //
    while(!f_cursors.empty() && !f_cursors.rbegin()->f_query)
    {
        f_cursors.pop_back();
    }
}


void snapdbproxy_connection::read_data(QtCassandra::QCassandraOrder const & order)
{
    auto q( QtCassandra::QCassandraQuery::create( f_session ) );
    send_order(q, order);

    QtCassandra::QCassandraOrderResult result;

    if( q->nextRow() )
    {
        // the list of columns may vary so we get the count
        int const max_columns(order.columnCount());
        for(int idx(0); idx < max_columns; ++idx)
        {
            result.addResult(q->getByteArrayColumn( idx ));
        }
    }

    result.setSucceeded(true);
    if(!f_proxy.sendResult(*this, result))
    {
        snap::NOTUSED(::close(f_socket));
        f_socket = -1;
    }
}


void snapdbproxy_connection::execute_command(QtCassandra::QCassandraOrder const & order)
{
    QtCassandra::QCassandraSession::pointer_t order_session;

    if(order.timeout() > 0)
    {
        // unfortunately, the request timeout cannot be changed in an
        // existing session (a connected session, to be precise); the
        // only way to get that to work is to change the timeout (in
        // the cluster config_) and then create a new session connection...
        //
        // see: https://datastax-oss.atlassian.net/browse/CPP-362
        //      https://datastax-oss.atlassian.net/browse/CPP-300
        //
        //SNAP_LOG_INFO("creating sub-Cassandra-session for this slow order (")(order.cql())(")");
        order_session = QtCassandra::QCassandraSession::create();
        {
            snap::snap_thread::snap_lock lock(g_connections_mutex);

            QtCassandra::QCassandraRequestTimeout request_timeout(order_session, order.timeout());
            order_session->connect( f_cassandra_host_list, f_cassandra_port ); // throws on failure!
        }
    }
    else
    {
        order_session = f_session;
    }

    auto q( QtCassandra::QCassandraQuery::create( order_session ) );
    send_order(q, order);

    // success
    QtCassandra::QCassandraOrderResult result;
    result.setSucceeded(true);
    if(!f_proxy.sendResult(*this, result))
    {
        snap::NOTUSED(::close(f_socket));
        f_socket = -1;
    }
}


// vim: ts=4 sw=4 et
