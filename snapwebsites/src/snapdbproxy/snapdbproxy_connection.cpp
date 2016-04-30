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


void signalfd_deleted(int s)
{
    close(s);
}



snapdbproxy_connection::snapdbproxy_connection(QtCassandra::QCassandraSession::pointer_t session, int s)
    : snap_runner("snapdbproxy_connection")
    , f_session(session)
    //, f_cursors() -- auto-init
    , f_socket(s)
{
    int optval(1);
    ioctl(f_socket, FIONBIO, &optval);

    // the parent thread makes use a signal in order wake up a child
    // thread in case the parent wants to leave before all children
    // are done; here we setup a signal as a file descriptor signal
    // so that way we can poll on it
    //
    // the code here blocks the signal and create a file descriptor
    // that we will use in our poll() calls along the socket we were
    // just given
    //

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIGUSR1, &set, nullptr);
    f_signal = signalfd(-1, &set, SFD_NONBLOCK | SFD_CLOEXEC);
    if(f_signal == -1)
    {
        int const e(errno);
        SNAP_LOG_ERROR("signalfd() failed to create a signal listener for signal ")(f_signal)(" (errno: ")(e)(" -- ")(strerror(e))(")");
        throw snap::snap_communicator_runtime_error("signalfd() failed to create a signal listener.");
    }

    // initializes the poll() environment
    f_poll_fds[0].fd = f_socket;
    //f_poll_fds[0].events = POLLIN | POLLPRI | POLLRDHUP;
    //   or
    //f_poll_fds[0].events = POLLOUT;

    f_poll_fds[1].fd = f_signal;
    f_poll_fds[1].events = POLLIN;
}


snapdbproxy_connection::~snapdbproxy_connection()
{
    close(f_signal);

    // WARNING: do not close f_socket here, our parent (snapdbproxy_thread)
    //          takes care of that
}


void snapdbproxy_connection::run()
{
SNAP_LOG_WARNING("got inside the connection::run() function!");
    try
    {
        do
        {
            // wait for an order
            //
            QtCassandra::QCassandraOrder order(f_proxy.receiveOrder(*this));
            if(order.validOrder())
            // && !thread->is_stopping()) -- we do not have access to the thread
            //                               and the pthread_kill() should be more
            //                               than enough at this point
            {
                // order can be executed now
                //
SNAP_LOG_WARNING("got an order: ")(static_cast<int>(order.get_type_of_result()))(", CQL \"")(order.cql())("\" (")(order.columnCount())(" columns).");
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
            }
            else
            {
                // in most cases if the order is not valid the connect
                // was hung up; it could also be an invalid protocol
                // or some transmission error (although really, with
                // TCP/IP transmission errors rarely happen.)
                //
SNAP_LOG_WARNING("socket is gone or order was invalid... ")(f_socket);
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

    // TODO: support some kind of timeout so we can close the connect
    //       after too long a pause?
    //
    f_poll_fds[0].events = POLLIN | POLLPRI | POLLRDHUP;
    ssize_t size(-1);
    for(;;)
    {
        int const r(poll(f_poll_fds, sizeof(f_poll_fds) / sizeof(f_poll_fds[0]), -1));

        if(r < 0)
        {
            // poll() returned an error (r < 0)
            //
            return size;
        }
        if(f_poll_fds[1].revents != 0)
        {
            // we received the SIGUSR1 signal
            //
            // Note: we should be reading from f_signal, but since we
            //       are leaving, the descriptor will get closed shortly
            //
            errno = ECANCELED;
            return size;
        }

        if(f_poll_fds[0].revents != 0)
        {
            // did the client hang up?
            //
            if((f_poll_fds[0].revents & POLLRDHUP) != 0)
            {
                return size;
            }

            ssize_t const sz(::read(f_socket, buf, count));
            if(sz < 0)
            {
                // the write failed, it could be a hangup
                //
                if(errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    return size;
                }
            }
            else if(sz > 0)
            {
                if(static_cast<size_t>(sz) > count)
                {
                    throw std::logic_error("there is a size mismatch while writing to a socket in snapdbproxy");
                }
                if(size == -1L)
                {
                    size = 0;
                }
                size += sz;
                count -= sz;
                if(count == 0)
                {
                    return size;
                }
                buf = reinterpret_cast<char *>(buf) + sz;
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

    // TODO: support some kind of timeout so we can close the connect
    //       after too long a pause?
    //
    f_poll_fds[0].events = POLLOUT;
    ssize_t size(-1L);
    for(;;)
    {
        int const r(poll(f_poll_fds, sizeof(f_poll_fds) / sizeof(f_poll_fds[0]), -1));

        if(r < 0)
        {
            // poll() returned an error (r < 0)
            //
            return size;
        }
        if(f_poll_fds[1].revents != 0)
        {
            // we received the SIGUSR1 signal
            //
            // Note: we should be reading from f_signal, but since we
            //       are leaving, the descriptor will get closed shortly
            //
            errno = ECANCELED;
            return size;
        }

        if(f_poll_fds[0].revents != 0)
        {
            ssize_t const sz(::write(f_socket, buf, count));
            if(sz < 0)
            {
                // the write failed, it could be a hangup
                //
                if(errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    return size;
                }
            }
            else if(sz > 0)
            {
                if(static_cast<size_t>(sz) > count)
                {
                    throw std::logic_error("there is a size mismatch while writing to a socket in snapdbproxy");
                }
                if(size == -1L)
                {
                    size = 0;
                }
                size += sz;
                count -= sz;
                if(count == 0)
                {
                    return size;
                }
                buf = reinterpret_cast<char const *>(buf) + sz;
            }
        }
    }
}


void snapdbproxy_connection::send_order(QtCassandra::QCassandraQuery * q, QtCassandra::QCassandraOrder const & order)
{
    size_t const count(order.parameterCount());

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
    //
    // TBD: will the timeout be effective even if set just before start()
    //      instead of before the allocation of this query? (if so create
    //      another sub-function to run the query and call it with or
    //      without the QCassandraRequestTimeout setup.)
    //
    if(order.timeout() != 0)
    {
        QtCassandra::QCassandraRequestTimeout request_timeout(f_session, order.timeout());
        q->start();
    }
    else
    {
        q->start();
    }
}


void snapdbproxy_connection::declare_cursor(QtCassandra::QCassandraOrder const & order)
{
    cursor_t cursor;
    cursor.f_query = std::make_shared<QtCassandra::QCassandraQuery>(f_session);
    cursor.f_column_count = order.columnCount();

    // in this case we have to keep the query so we allocate it
    //
    send_order(cursor.f_query.get(), order);

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
        f_socket = -1;
    }
}


void snapdbproxy_connection::describe_cluster(QtCassandra::QCassandraOrder const & order)
{
    snap::NOTUSED(order);

    // load the meta data
    QtCassandra::QCassandraSchema::SessionMeta::pointer_t session_meta( QtCassandra::QCassandraSchema::SessionMeta::create(f_session) );
    session_meta->loadSchema();

    // convert the meta data to a blob and send it over the wire
    QtCassandra::QCassandraOrderResult result;
    result.addResult( session_meta->encodeSessionMeta() );
    result.setSucceeded(true);

    if(!f_proxy.sendResult(*this, result))
    {
        f_socket = -1;
    }
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
    QtCassandra::QCassandraQuery q( f_session );
    send_order(&q, order);

    QtCassandra::QCassandraOrderResult result;

    if( q.nextRow() )
    {
        // the list of columns may vary so we get the count
        int max_columns(order.columnCount());
        for(int idx(0); idx < max_columns; ++idx)
        {
            result.addResult(q.getByteArrayColumn( idx ));
        }
    }

    result.setSucceeded(true);
    if(!f_proxy.sendResult(*this, result))
    {
        f_socket = -1;
    }
}


void snapdbproxy_connection::execute_command(QtCassandra::QCassandraOrder const & order)
{
    QtCassandra::QCassandraQuery q( f_session );
    send_order(&q, order);

    // success
    QtCassandra::QCassandraOrderResult result;
    result.setSucceeded(true);
    if(!f_proxy.sendResult(*this, result))
    {
        f_socket = -1;
    }
}


// vim: ts=4 sw=4 et
