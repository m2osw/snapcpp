// Snap Communicator -- classes to ease handling communication between processes
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

/** \file
 * \brief Implementation of the Snap Communicator class.
 *
 * This class wraps the C poll() interface in a C++ object with many types
 * of objects:
 *
 * \li Server Connections; for software that want to offer a port to
 *     which clients can connect to; the server will call accept()
 *     once a new client connection is ready; this results in a
 *     Server/Client connection object
 * \li Client Connections; for software that want to connect to
 *     a server; these expect the IP address and port to connect to
 * \li Server/Client Connections; for the server when it accepts a new
 *     connection; in this case the server gets a socket from accept()
 *     and creates one of these objects to handle the connection
 *
 * Using the poll() function is the easiest and allows us to listen
 * on pretty much any number of sockets (on my server it is limited
 * at 16,768 and frankly over 1,000 we probably will start to have
 * real slowness issues on small VPN servers.)
 */

// to get the POLLRDHUP definition
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "snap_communicator.h"

#include "log.h"
#include "not_reached.h"
#include "not_used.h"
#include "qstring_stream.h"

#include <sstream>
#include <limits>

#include <poll.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "poison.h"

namespace snap
{
namespace
{


snap_communicator::pointer_t                        g_instance;

/** \brief The array of signals handled by snap_signal objects.
 *
 * This function holds a list of signal handlers. Whenever a
 * signal is received a snap_signal object has to be marked
 * as active. Then the signal handler returns and the run()
 * loop wakes up, detects that the signal was received and
 * runs the process_signal() callback.
 */
QMap<int, snap_communicator::snap_signal::weak_t>   g_signal_handlers;


} // no name namespace




///////////////////////////////
// Snap Communicator Message //
///////////////////////////////


/** \brief Parse a message from the specified paremeter.
 *
 * This function transformed the input string in a set of message
 * fields.
 *
 * The message format supported is:
 *
 * \code
 *      ( service '/' )? command ' ' ( parameter_name '=' value ';' )*
 * \endcode
 *
 * The space after the command cannot be there unless parameters follow.
 * Parameters must be separated by semi-colons. No space is allowed anywhere
 * except between the command and first parameter. The value of a parameter
 * can be quoted if it includes a ';'. Quotes can be escaped inside the
 * value by adding a backslash in front of it. Newline characters (as well
 * as return carriage) are also escaped. Only values support any character.
 * All the other parameters are limited to the latin alphabet, digits,
 * and underscores ([A-Za-z0-9_]+). At the point, all commands are
 * always written in uppercase.
 *
 * \note
 * The input message is not saved as a cached version of the message
 * because we assume it may not be 100% optimized (canonicalized.)
 *
 * \param[in] message
 *
 * \return true if the message was succesfully parsed; false when an
 *         error occurs and in that case no parameters get modified.
 */
bool snap_communicator_message::from_message(QString const & message)
{
    QString service;
    QString command;
    parameters_t parameters;

    QChar const * m(message.constData());
    bool has_service(false);
    for(; !m->isNull() && m->unicode() != ' '; ++m)
    {
        if(m->unicode() == '/')
        {
            if(has_service
            || command.isEmpty())
            {
                // we cannot have more than one '/'
                // and the name cannot be empty if '/' is used
                return false;
            }
            has_service = true;
            service = command;
            command.clear();
        }
        else
        {
            command += *m;
        }
    }

    if(command.isEmpty())
    {
        // command is mandatory
        return false;
    }

    // if we have a space, we expect one or more parameters
    if(m->unicode() == ' ')
    {
        for(++m; !m->isNull();)
        {
            // first we have to read the parameter name (up to the '=')
            QString param_name;
            for(; !m->isNull() && m->unicode() != '='; ++m)
            {
                param_name += *m;
            }
            if(param_name.isEmpty())
            {
                // parameters must have a name
                return false;
            }
            try
            {
                verify_parameter_name(param_name);
            }
            catch(snap_communicator_invalid_message const &)
            {
                // name is not empty, but it has invalid characters in it
                return false;
            }

            if(m->isNull()
            || m->unicode() != '=')
            {
                // ?!?
                return false;
            }
            ++m;

            // retrieve the parameter name at first
            QString param_value;
            if(!m->isNull() && m->unicode() == '"')
            {
                // quoted parameter
                for(++m; !m->isNull() && m->unicode() != '"'; ++m)
                {
                    // restored escaped double quotes
                    if(m->unicode() == '\\' && !m[1].isNull() && m[1].unicode() == '"')
                    {
                        ++m;
                        param_value += *m;
                    }
                    else
                    {
                        // here the character may be ';'
                        param_value += *m;
                    }
                }
                if(m->isNull()
                || m->unicode() != '"')
                {
                    // closing quote (") is missing
                    return false;
                }
                ++m;

                // now we have to have the ';' if the string goes on
                if(!m->isNull()
                && m->unicode() != ';')
                {
                    return false;
                }
            }
            else
            {
                // parameter value is found as is
                for(; !m->isNull() && m->unicode() != ';'; ++m)
                {
                    param_value += *m;
                }
            }

            if(!m->isNull())
            {
                if(m->unicode() != ';')
                {
                    // this should neverh append
                    return false;
                }
                // skip the ';'
                ++m;
            }

            // also restore new lines if any
            param_value.replace("\\n", "\n")
                       .replace("\\r", "\r");

            // we got a valid parameter, add it
            parameters[param_name] = param_value;
        }
    }

    f_service = service;
    f_command = command;
    f_parameters.swap(parameters);
    f_cached_message.clear();

    return true;
}


/** \brief Transform all the message parameters in a string.
 *
 * This function transforms all the message parameters in a string
 * and returns the result. The string is a message we can send over
 * TCP/IP (if you make sure to add a "\n", note that the
 * send_message() does that automatically) or over UDP/IP.
 *
 * \note
 * The function caches the result so calling the function many times
 * will return the same string and thus the function is very fast
 * after the first time (assuming you do not modify the message on
 * each call to to_message().)
 *
 * \exception snap_communicator_invalid_message
 * This function raises an exception if the command was not defined
 * since a command is always mandatory.
 *
 * \return The converted message as a string.
 */
QString snap_communicator_message::to_message() const
{
    if(f_cached_message.isEmpty())
    {
        if(f_command.isEmpty())
        {
            throw snap_communicator_invalid_message("snap_communicator_message::to_message(): cannot build a valid message without at least a command.");
        }

        // <name>/
        f_cached_message = f_service;
        if(!f_cached_message.isEmpty())
        {
            f_cached_message += "/";
        }

        // [<name>/]command
        f_cached_message += f_command;

        // then add parameters
        bool first(true);
        for(auto p(f_parameters.begin());
                 p != f_parameters.end();
                 ++p, first = false)
        {
            f_cached_message += QString("%1%2=").arg(first ? " " : ";").arg(p.key());
            QString param(p.value());
            param.replace("\n", "\\n")   // newline needs to be escaped
                 .replace("\r", "\\r");  // this one is not important, but for completeness
            if(param.indexOf(";") >= 0
            || (!param.isEmpty() && param[0] == '\"'))
            {
                // escape the double quotes
                param.replace("\"", "\\\"");
                // quote the resulting parameter and save in f_cached_message
                f_cached_message += QString("\"%1\"").arg(param);
            }
            else
            {
                // no special handling necessary
                f_cached_message += param;
            }
        }
    }

    return f_cached_message;
}


QString const & snap_communicator_message::get_service() const
{
    return f_service;
}


void snap_communicator_message::set_service(QString const & service)
{
    if(f_service != service)
    {
        f_service = service;
        f_cached_message.clear();
    }
}


QString const & snap_communicator_message::get_command() const
{
    return f_command;
}


void snap_communicator_message::set_command(QString const & command)
{
    if(f_command != command)
    {
        f_command = command;
        f_cached_message.clear();
    }
}


void snap_communicator_message::add_parameter(QString const & name, QString const & value)
{
    verify_parameter_name(name);

    f_parameters[name] = value;
    f_cached_message.clear();
}


void snap_communicator_message::add_parameter(QString const & name, int64_t value)
{
    verify_parameter_name(name);

    f_parameters[name] = QString("%1").arg(value);
    f_cached_message.clear();
}


bool snap_communicator_message::has_parameter(QString const & name) const
{
    verify_parameter_name(name);

    return f_parameters.contains(name);
}


QString const snap_communicator_message::get_parameter(QString const & name) const
{
    verify_parameter_name(name);

    if(f_parameters.contains(name))
    {
        return f_parameters[name];
    }

    throw snap_communicator_invalid_message("snap_communicator_message::get_parameter(): parameter not defined, try has_parameter() before calling a get_..._parameter() function.");
}


int64_t snap_communicator_message::get_integer_parameter(QString const & name) const
{
    verify_parameter_name(name);

    if(f_parameters.contains(name))
    {
        bool ok;
        int64_t const r(f_parameters[name].toLongLong(&ok, 10));
        if(!ok)
        {
            throw snap_communicator_invalid_message("snap_communicator_message::get_integer_parameter(): message expected integer could not be converted.");
        }
        return r;
    }

    throw snap_communicator_invalid_message("snap_communicator_message::get_integer_parameter(): parameter not defined, try has_parameter() before calling a get_..._parameter() function.");
}


snap_communicator_message::parameters_t const & snap_communicator_message::get_all_parameters() const
{
    return f_parameters;
}


void snap_communicator_message::verify_parameter_name(QString const & name) const
{
    for(auto c : name)
    {
        if((c < 'a' || c > 'z')
        && (c < 'A' || c > 'Z')
        && (c < '0' || c > '9')
        && c != '_')
        {
            throw snap_communicator_invalid_message("snap_communicator_message::add_parameter(): parameter name must be composed of ASCII 'a'..'z', 'A'..'Z', '0'..'9', or '_' only.");
        }
    }
}








/////////////////////
// Snap Connection //
/////////////////////


/** \brief Initializes the client connection.
 *
 * This function initializes a client connection with the given
 * snap_communicator pointer.
 *
 * \param[in] communicator  The snap communicator controlling this connection.
 */
snap_communicator::snap_connection::snap_connection()
    //: f_name("")
    //, f_priority(0)
    //, f_timeout(-1)
{
}


/** \brief Proceed with the cleanup of the snap_connection.
 *
 * This function cleans up a snap_connection object.
 *
 * The main work on the function is to make sure that it can indeed
 * be cleaned (i.e. if the connection is still attached to an event_base
 * object, then it generates an error and terminates the software.)
 */
snap_communicator::snap_connection::~snap_connection()
{
}


/** \brief Remove this connection from the communicator it was added in.
 *
 * This function removes the connection from the communicator that
 * it was created in.
 *
 * This happens in several circumstances:
 *
 * \li When the connection is not necessary anymore
 * \li When the connection receives a message saying it should close
 * \li When the connection receives a Hang Up event
 * \li When the connection looks erroneous
 * \li When the connection looks invalid
 *
 * If the connection is not currently connected to a snap_communicator
 * object, then nothing happens.
 */
void snap_communicator::snap_connection::remove_from_communicator()
{
    snap_communicator::instance()->remove_connection(shared_from_this());
}


/** \brief Retrieve the name of the connection.
 *
 * When generating an error or a log the library makes use of this name
 * so we actually know which type of socket generated a problem.
 *
 * \return A constant reference to the connection name.
 */
QString const & snap_communicator::snap_connection::get_name() const
{
    return f_name;
}


/** \brief Change the name of the connection.
 *
 * A connection can be given a name. This is mainly for debug purposes.
 * We will be adding this name in errors and exceptions as they occur.
 *
 * The connection makes a copy of \p name.
 *
 * \param[in] name  The name to give this connection.
 */
void snap_communicator::snap_connection::set_name(QString const & name)
{
    f_name = name;
}


/** \brief Tell us whether this socket is a listener or not.
 *
 * By default a snap_connection object does not represent a listener
 * object.
 *
 * \return The base implementation returns false. Override this
 *         virtual function if your snap_connection is a listener.
 */
bool snap_communicator::snap_connection::is_listener() const
{
    return false;
}


/** \brief Tell us whether this connection is listening on a Unix signal.
 *
 * By default a snap_connection object does not represent a Unix signal.
 * See the snap_signal implementation for further information about
 * Unix signal handling in this library.
 *
 * \return The base implementation returns false.
 */
bool snap_communicator::snap_connection::is_signal() const
{
    return false;
}


/** \brief Tell us whether this socket is used to receive data.
 *
 * If you expect to receive data on this connection, then mark it
 * as a reader by returning true in an overridden version of this
 * function.
 *
 * \return By default this function returns false (nothing to read).
 */
bool snap_communicator::snap_connection::is_reader() const
{
    return false;
}


/** \brief Tell us whether this socket is used to send data.
 *
 * If you expect to send data on this connection, then mark it
 * as a writer by returning true in an overridden version of
 * this function.
 *
 * \return By default this function returns false (nothing to write).
 */
bool snap_communicator::snap_connection::is_writer() const
{
    return false;
}


/** \brief Check whether the socket is valid for this connection.
 *
 * Some connections do not make use of a socket so just checking
 * whether the socket is -1 is not a good way to know whether the
 * socket is valid.
 *
 * The default function assumes that a socket has to be 0 or more
 * to be valid. Other connection implementations may overload this
 * function to allow other values.
 *
 * \return true if the socket is valid.
 */
bool snap_communicator::snap_connection::valid_socket() const
{
    return get_socket() >= 0;
}


/** \brief Check whether this connection is enabled.
 *
 * It is possible to turn a connection ON or OFF using the set_enabled()
 * function. This function returns the current value. If true, which
 * is the default, the connection is considered enabled and will get
 * its callbacks called.
 *
 * \return true if the connection is currently enabled.
 */
bool snap_communicator::snap_connection::is_enabled() const
{
    return f_enabled;
}


/** \brief Change the status of a connection.
 *
 * This function let you change the status of a connection from
 * enabled (true) to disabled (false) and vice versa.
 *
 * A disabled connection is not listened on at all. This is similar
 * to returning false in all three functions is_listener(),
 * is_reader(), and is_writer().
 *
 * \param[in] enabled  The new status of the connection.
 */
void snap_communicator::snap_connection::set_enable(bool enabled)
{
    f_enabled = enabled;
}


/** \brief Define the priority of this connection object.
 *
 * By default snap_connection objets have a priority of 0. Since most
 * of the time a snap_communicator is setup to only support one priority
 * (i.e. priority of zero) then it makes sense to return 0 as the default.
 *
 * You may also use the set_priority() to change the priority of an
 * event. You have to do so before you call add_connection(). After
 * that the priority has no effect.
 */
int snap_communicator::snap_connection::get_priority() const
{
    return f_priority;
}


/** \brief Change this event priority.
 *
 * This function can be used to change the default priority (which is
 * zero) to a larger number. A larger number makes the event less
 * important.
 *
 * Note that the priority of an event can only be setup before
 * the event gets added (with the add_connection() function.)
 * The priority parameter must be valid for the snap_communicator
 * where it will be added.
 *
 * \exception snap_communicator_parameter_error
 * The priority of the event is out of range when this exception is raised.
 * The value must between between 0 and EVENT_MAX_PRIORITY. Any
 * other value raises this exception.
 *
 * \param[in] priority  Priority of the event.
 */
void snap_communicator::snap_connection::set_priority(priority_t priority)
{
    if(priority < 0 || priority > EVENT_MAX_PRIORITY)
    {
        std::stringstream ss;
        ss << "snap_communicator::set_priority(): priority out of range, this instance of snap_communicator accepts priorities between 0 and "
           << EVENT_MAX_PRIORITY
           << ".";
        throw snap_communicator_parameter_error(ss.str());
    }

    f_priority = priority;

    // make sure that the new order is calculated when we execute
    // the next loop
    //
    snap_communicator::instance()->f_force_sort = true;
}


/** \brief Return the delay between ticks when this connection times out.
 *
 * All connections can include a timeout delay in microseconds which is
 * used to know when the wait on that specific connection times out.
 *
 * By default connections do not time out. This function returns -1
 * to indicate that this connection does not ever time out. To
 * change the timeout delay use the set_timeout_delay() function.
 *
 * \return This function returns the current timeout delay.
 */
int64_t snap_communicator::snap_connection::get_timeout_delay() const
{
    return f_timeout_delay;
}


/** \brief Change the timeout of this connection.
 *
 * Each connection can be setup with a timeout in microseconds.
 * When that delay is past, the callback function of the connection
 * is called with the EVENT_TIMEOUT flag set (note that the callback
 * may happen along other events.)
 *
 * The current date when this function gets called is the starting
 * point for each following trigger. Because many other callbacks
 * get called, it is not very likely that you will be called
 * exactly on time, but the ticks are guaranteed to be requested
 * on a non moving schedule defined as:
 *
 * \f[
 * \large tick_i = start-time + k \times delay
 * \f]
 *
 * In other words the time and date when ticks happen does not slip
 * with time. However, this implementation may skip one or more
 * ticks at any time (especially if the delay is very small).
 *
 * When a tick triggers an EVENT_TIMEOUT, the snap_communicator::run()
 * function calls calculate_next_tick() to calculate the time when
 * the next tick will occur which will always be in the function.
 *
 * \param[in] timeout_us  The new time out in microseconds.
 */
void snap_communicator::snap_connection::set_timeout_delay(int64_t timeout_us)
{
    if(timeout_us < -1)
    {
        throw snap_communicator_parameter_error("snap_communicator::snap_connection::set_timeout_delay(): timeout_us parameter cannot be less than -1.");
    }

    f_timeout_delay = timeout_us;

    // immediately calculate the next timeout date
    f_timeout_next_date = get_current_date() + f_timeout_delay;
}


/** \brief Calculate when the next tick shall occur.
 *
 * This function calculates the date and time when the next tick
 * has to be triggered. This function is called after the
 * last time the EVENT_TIMEOUT callback was called.
 */
void snap_communicator::snap_connection::calculate_next_tick()
{
    if(f_timeout_delay == -1)
    {
        // no delay based timeout so forget about it
        return;
    }

    // what is now?
    int64_t const now(get_current_date());

    // gap between now and the last time we triggered this timeout
    int64_t const gap(now - f_timeout_next_date);
    if(gap < 0)
    {
        // someone we got called even though now is still larger
        // than f_timeout_next_date
        //
        SNAP_LOG_DEBUG("snap_communicator::snap_connection::calculate_next_tick() called even though the next date is still larger than 'now'.");
        return;
    }

    // number of ticks in that gap, rounded up
    int64_t const ticks((gap + f_timeout_delay - 1) / f_timeout_delay);

    // the next date may be equal to now, however, since it is very
    // unlikely that the tick has happened right on time, and took
    // less than 1ms, this is rather unlikely all around...
    //
    f_timeout_next_date += ticks * f_timeout_delay;
}


/** \brief Return when this connection times out.
 *
 * All connections can include a timeout in microseconds which is
 * used to know when the wait on that specific connection times out.
 *
 * By default connections do not time out. This function returns -1
 * to indicate that this connection does not ever time out. You
 * may overload this function to return a different value so your
 * version can time out.
 *
 * \return This function returns the timeout date.
 */
int64_t snap_communicator::snap_connection::get_timeout_date() const
{
    return f_timeout_date;
}


/** \brief Change the date at which you want a timeout event.
 *
 * This function can be used to setup one specific date and time
 * at which this connection should timeout. This specific date
 * is used internally to calculate the amount of time the poll()
 * will have to wait, not including the time it will take
 * to execute other callbacks if any need to be run (i.e. the
 * timeout is executed last, after all other events, and also
 * priority is used to know which other connections are parsed
 * first.)
 *
 * \param[in] date_us  The new time out in micro seconds.
 */
void snap_communicator::snap_connection::set_timeout_date(int64_t date_us)
{
    if(date_us < -1)
    {
        throw snap_communicator_parameter_error("snap_communicator::snap_connection::set_timeout_date(): date_us parameter cannot be less than -1.");
    }

    f_timeout_date = date_us;
}


/** \brief Return when this connection expects a timeout.
 *
 * All connections can include a timeout specification which is
 * either a specific day and time set with set_timeout_date()
 * or an repetitive timeout which is defined with the
 * set_timeout_delay().
 *
 * If neither timeout is set the function returns -1. Otherwise
 * the function will calculate when the connection is to time
 * out and return that date.
 *
 * If the date is already in the past then the callback
 * is called immediately with the EVENT_TIMEOUT flag set.
 *
 * \note
 * If the timeout date is triggered, then the loop calls
 * set_timeout_date(-1) because the date timeout is expected
 * to only be triggered once. This resetting is done before
 * calling the user callback which can in turn set a new
 * value back in the connection object.
 *
 * \return This function returns -1 when no timers are set
 *         or a timestamp in microseconds when the timer is
 *         expected to trigger.
 */
int64_t snap_communicator::snap_connection::get_timeout_timestamp() const
{
    if(f_timeout_date != -1)
    {
        // this one is easy, it is already defined as expected
        return f_timeout_date;
    }

    if(f_timeout_delay != -1)
    {
        // no timeout defined
        return f_timeout_next_date;
    }

    return -1;
}


/** \brief Save the timeout stamp just before calling poll().
 *
 * This function is called by the run() function before the poll()
 * gets called. It makes sure to save the timeout timestamp so
 * when we check the connections again after poll() returns and
 * any number of callbacks were called, the timeout does or does
 * not happen as expected.
 *
 * \return The timeout timestamp as returned by get_timeout_timestamp().
 *
 * \sa get_saved_timeout_timestamp()
 * \sa run()
 */
int64_t snap_communicator::snap_connection::save_timeout_timestamp()
{
    f_saved_timeout_stamp = get_timeout_timestamp();
    return f_saved_timeout_stamp;
}


/** \brief Get the saved timeout timestamp.
 *
 * This function returns the timeout as saved by the
 * save_timeout_timestamp() function. The timestamp returned by
 * this funtion was frozen so if the user calls various timeout
 * functions that could completely change the timeout stamp that
 * the get_timeout_timestamp() would return just at the time we
 * want to know whether th timeout callback needs to be called
 * will be ignored by the loop.
 *
 * \return The saved timeout stamp as returned by save_timeout_timestamp().
 *
 * \sa save_timeout_timestamp()
 * \sa run()
 */
int64_t snap_communicator::snap_connection::get_saved_timeout_timestamp() const
{
    return f_saved_timeout_stamp;
}


/** \brief Make this connection socket a non-blocking socket.
 *
 * For the read and write to work as expected we generally need
 * to make those sockets non-blocking.
 *
 * For accept(), you do just one call and return and it will not
 * block on you. It is important to not setup a socket you
 * listen on as non-blocking if you do not want to risk having the
 * accepted sockets non-blocking.
 */
void snap_communicator::snap_connection::non_blocking() const
{
    if(get_socket() >= 0)
    {
        int optval(1);
        ioctl(get_socket(), FIONBIO, &optval);
    }
}


/** \brief Ask the OS to keep the socket alive.
 *
 * This function marks the socket with the SO_KEEPALIVE flag. This means
 * the OS implementation of the network stack should regularly send
 * small messages over the network to keep the connection alive.
 *
 * The function returns whether the function works or not. If the function
 * fails, it logs a warning and returns.
 */
void snap_communicator::snap_connection::keep_alive() const
{
    if(get_socket() != -1)
    {
        int optval(1);
        socklen_t const optlen(sizeof(optval));
        if(setsockopt(get_socket(), SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) != 0)
        {
            SNAP_LOG_WARNING("snap_communicator::snap_tcp_server_client_connection::keep_alive(): an error occurred trying to mark socket with SO_KEEPALIVE.");
        }
    }
}


/** \brief Less than operator to sort connections by priority.
 *
 * This function is used to know whether a connection has a higher or lower
 * priority. This is used when one adds, removes, or change the priority
 * of a connection. The sorting itself happens in the
 * snap_communicator::run() which knows that something changed whenever
 * it checks the data.
 *
 * The result of the priority mechanism is that callbacks of items with
 * a smaller priorirty will be executed first.
 *
 * \param[in] rhs  The right hand side snap_connection.
 *
 * \return true if this snap_connection has a smaller priority than the
 *         right hand side snap_connection.
 */
bool snap_communicator::snap_connection::operator < (snap_connection const & rhs) const
{
    return f_priority < rhs.f_priority;
}


/** \brief This callback gets called whenever the connection times out.
 *
 * This function is called whenever a timeout is detected on this
 * connection. It is expected to be overwritten by your class if
 * you expect to use the timeout feature.
 *
 * The snap_timer class is expected to always have a timer (although
 * the connection can temporarily be disabled) which triggers this
 * callback on a given periodicity.
 */
void snap_communicator::snap_connection::process_timeout()
{
}


/** \brief This callback gets called whenever the signal happened.
 *
 * This function is called whenever a certain signal (as defined in
 * your snap_signal object) was detected while waiting for an
 * event.
 */
void snap_communicator::snap_connection::process_signal()
{
}


/** \brief This callback gets called whenever data can be read.
 *
 * This function is called whenever a socket has data that can be
 * read. For UDP, this means reading one packet. For TCP, it means
 * you can read at least one byte. To avoid blocking in TCP,
 * you must have called the non_blocking() function on that
 * connection, then you can attempt to read as much data as you
 * want.
 */
void snap_communicator::snap_connection::process_read()
{
}


/** \brief This callback gets called whenever data can be written.
 *
 * This function is called whenever a socket has space in its output
 * buffers to write data there.
 *
 * For UDP, this means writing one packet.
 *
 * For TCP, it means you can write at least one byte. To be able to
 * write as many bytes as you want, you must make sure to make the
 * socket non_blocking() first, then you can write as many bytes as
 * you want, although all those bytes may not get written in one
 * go (you may need to wait for the next call to this function to
 * finish up your write.)
 */
void snap_communicator::snap_connection::process_write()
{
}


/** \brief This callback gets called whenever a connection is made.
 *
 * A listening server receiving a new connection gets this function
 * called. The function is expected to create a new connection object
 * and add it to the communicator.
 *
 * \code
 *      // get the socket from the accept() function
 *      int const client_socket(accept());
 *      client_impl::pointer_t connection(new client_impl(get_communicator(), client_socket));
 *      connection->set_name("connection created by server on accept()");
 *      get_communicator()->add_connection(connection);
 * \endcode
 */
void snap_communicator::snap_connection::process_accept()
{
}


/** \brief This callback gets called whenever an error is detected.
 *
 * If an error is detected on a socket, this callback function gets
 * called. By default the function removes the connection from
 * the communicator because such errors are generally non-recoverable.
 *
 * The function also logs an error message.
 */
void snap_communicator::snap_connection::process_error()
{
    SNAP_LOG_ERROR("socket of connection \"")(f_name)("\" was marked as erroneous by the kernel.");

    remove_from_communicator();
}


/** \brief This callback gets called whenever a hang up is detected.
 *
 * When the remote connection (client or server) closes a socket
 * on their end, then the other end is signaled by getting this
 * callback called.
 *
 * Note that this callback will be called after the process_read()
 * and process_write() callbacks. The process_write() is unlikely
 * to work at all. However, the process_read() may be able to get
 * a few more bytes from the remove connection and act on it.
 *
 * By default a connection gets removed from the communicator
 * when the hang up even occurs.
 */
void snap_communicator::snap_connection::process_hup()
{
    remove_from_communicator();
}


/** \brief This callback gets called whenever an invalid socket is detected.
 *
 * I am not too sure at the moment when we are expected to really receive
 * this call. How does a socket become invalid (i.e. does it get closed
 * and then the user still attempts to use it)? In most cases, this should
 * probably never happen.
 *
 * By default a connection gets removed from the communicator
 * when the invalid even occurs.
 *
 * This function also logs the error.
 */
void snap_communicator::snap_connection::process_invalid()
{
    SNAP_LOG_ERROR("socket of connection \"")(f_name)("\" was marked as invalid by the kernel.");

    remove_from_communicator();
}







////////////////
// Snap Timer //
////////////////


/** \brief Initializes the timer object.
 *
 * This function initializes the timer object with the specified \p timeout
 * defined in microseconds.
 *
 * Note that by default all snap_connection objects are marked as persistent
 * since in most cases that is the type of connections you are interested
 * in. Therefore timers are also marked as persistent. This means if you
 * want a one time callback, you want to call the remove_connection()
 * function with your timer from your callback.
 *
 * \param[in] communicator  The snap communicator controlling this connection.
 * \param[in] timeout_us  The timeout in microseconds.
 */
snap_communicator::snap_timer::snap_timer(int64_t timeout_us)
{
    set_timeout_delay(timeout_us);
}


/** \brief Retrieve the socket of the timer object.
*
* Timer objects are never attached to a socket so this function always
 * returns -1.
 *
 * \note
 * You should not override this function since there is not other
 * value it can return.
 *
 * \return Always -1.
 */
int snap_communicator::snap_timer::get_socket() const
{
    return -1;
}


/** \brief Tell that the socket is always valid.
 *
 * This function always returns true since the timer never uses a socket.
 *
 * \return Always true.
 */
bool snap_communicator::snap_timer::valid_socket() const
{
    return true;
}







/////////////////
// Snap Signal //
/////////////////


/** \brief Initializes the signal object.
 *
 * This function initializes the signal object with the specified
 * \p posix_signal which represents a POSIX signal such as SIGHUP,
 * SIGTERM, SIGUSR1, SIGUSR2, etc.
 *
 * The poll() function can unblock a set of POSIX signals that
 * end up calling the process_signal() callback function.
 * We have not written any specialized code for this type of
 * connection at this point.
 *
 * Note that the snap_signal callback is called from the normal user
 * environment and not directly from the POSIX signal handler.
 * This means you can call any function from your callback.
 *
 * \note
 * IMPORTANT: Remember that POSIX signals stop your code at a 'breakable'
 * point which in many circumstances can create many problems unless
 * you make sure to mask signals while doing work. For example, you
 * could end up with a read() returning an error when the file you
 * are reading has absolutely no error but a dude decided to signal
 * you with a 'kill -HUP 123'...
 *
 * \code
 *      {
 *          // use an RAII masking mechanism
 *          mask_posix_signal mask();
 *
 *          // do your work (i.e. read/write/etc.)
 *          ...
 *      }
 * \endcode
 *
 * \par
 * The best way in our processes will be to block all signals except
 * while poll() is called (using ppoll() for the feat.)
 *
 * \param[in] communicator  The snap communicator controlling this connection.
 * \param[in] timeout  The timeout in microseconds.
 */
snap_communicator::snap_signal::snap_signal(int posix_signal)
    : f_signal(posix_signal)
{
    // TODO: implement the grabbing of that signal
    if(g_signal_handlers.contains(f_signal))
    {
        // this could be fixed, but probably not worth the trouble...
        throw snap_communicator_initialization_error("the same signal cannot be created more than once in your entire process.");
    }
    pointer_t sp(this);
    g_signal_handlers[f_signal] = sp; // TBD: is that assignment really correct?!

    // TODO: redesign that one with signalfd() instead, because that way
    //       we don't need to have a handler at all! Then we just use
    //       the read() to get the signal information...
    //
    f_sighandler = signal(f_signal, sighandler);
}


/** \brief Restore the signal as it was before you created a snap_signal.
 *
 * The destructor is expected to restore the signal to what it was
 * before you create this snap_signal. Of course, if you created
 * other signal handlers in between, it will not work right since
 * this function will destroy your handler pointer.
 *
 * To do it right, it has to be done in order (i.e. set handler 1, set
 * handler 2, set handler 3, remove handler 3, remove handler 2, remove
 * handler 1.) We do not guarantee anything at this level!
 */
snap_communicator::snap_signal::~snap_signal()
{
    // restore signal() handler as it was before
    signal(f_signal, f_sighandler);
    g_signal_handlers.remove(f_signal); // the weak pointer is already nullptr but it still exists in this map
}


/** \brief Tell that this connection is listening on a Unix signal.
 *
 * The snap_signal implements the signal listening feature. We use
 * a simple flag in the virtual table to avoid a more expansive
 * dynamic_cast<>() is a loop that goes over all the connections
 * you have defined.
 *
 * \return The base implementation returns false.
 */
bool snap_communicator::snap_signal::is_signal() const
{
    return true;
}


/** \brief Retrieve the "socket" of the signal object.
 *
 * Signal objects have a socket number that represents the POSIX
 * signal number.
 *
 * \note
 * You should not override this function since there is not other
 * value it can return.
 *
 * \return The POSIX signal number of this snap_signal object.
 */
int snap_communicator::snap_signal::get_socket() const
{
    return f_signal;
}


/** \brief Whether this signal is active or not.
 *
 * This function returns true if this signal is active, which means
 * that the signal handler was called.
 *
 * The flag gets set to true whenever the signal handler gets called,
 * and reset by the run() function once the process_signal() gets
 * called.
 */
bool snap_communicator::snap_signal::is_active() const
{
    return f_active;
}


/** \brief Mark the signal as active or not.
 *
 * This function is used to activate (\p active = true) or
 * deactivate (\p active = false) this signal. An active
 * signal is one for which the process_signal() callback
 * will be called.
 */
void snap_communicator::snap_signal::activate(bool active)
{
    f_active = active;
}


/** \brief Capture the signal.
 *
 * This signal handler is called by the kernel whenever a signal is
 * received with the identifier as the user defined when constructing
 * the corresponding snap_signal object.
 *
 * \todo
 * Add a flag to know whether the previous handler should be called
 * once we are done with our own work.
 */
void snap_communicator::snap_signal::sighandler(int sig)
{
    // make sure that signal was still valid
    if(g_signal_handlers.contains(sig))
    {
        pointer_t s(g_signal_handlers[sig].lock());
        if(s)
        {
            s->signal_received();
        }
    }
}


/** \brief Called when receiving the signal.
 *
 * This function is called from the signal handler because the
 * signal handled is a static function (so it cannot access
 * internal variables without first calling a function like
 * this one.)
 *
 * The function marks the signal as active.
 *
 * \todo
 * TBD: determine whether the previous signal should be called.
 */
void snap_communicator::snap_signal::signal_received()
{
    activate(true);

    // call the previous handler implementation?
    // (we may want to include a flag to know whether this should
    // be done... at this point we do not do that.)
    //
    if(f_sighandler != SIG_IGN
    && f_sighandler != SIG_DFL)
    {
        //f_sighandler(sig);
    }
}






////////////////////////////////
// Snap TCP Client Connection //
////////////////////////////////


/** \brief Initializes the client connection.
 *
 * This function creates a connection using the address, port, and mode
 * parameters. This is very similar to using the bio_client class to
 * create a connection, only the resulting connection can be used with
 * the snap_communicator object.
 *
 * \param[in] communicator  The snap communicator controlling this connection.
 * \param[in] addr  The address of the server to connect to.
 * \param[in] port  The port to connect to.
 * \param[in] mode  Type of connection: plain or secure.
 */
snap_communicator::snap_tcp_client_connection::snap_tcp_client_connection(std::string const & addr, int port, mode_t mode)
    : bio_client(addr, port, mode)
{
}


/** \brief Retrieve the socket of this client connection.
 *
 * This function retrieves the socket this client connection. In this case
 * the socket is defined in the bio_client class.
 *
 * \return The socket of this client connection.
 */
int snap_communicator::snap_tcp_client_connection::get_socket() const
{
    return bio_client::get_socket();
}


/** \brief Check whether this connection is a reader.
 *
 * We change the default to true since TCP sockets are generally
 * always readers. You can still overload this function and
 * return false if necessary.
 *
 * However, we do not overload the is_writer() because that is
 * much more dynamic (i.e. you do not want to advertise as
 * being a writer unless you have data to write to the
 * socket.)
 *
 * \return The events to listen to for this connection.
 */
bool snap_communicator::snap_tcp_client_connection::is_reader() const
{
    return true;
}






////////////////////////////////
// Snap TCP Buffer Connection //
////////////////////////////////

/** \brief Initialize a client socket.
 *
 * The client socket gets initialized with the specified 'socket'
 * parameter.
 *
 * This constructor creates a writer connection too. This gives you
 * a read/write connection. You can get the writer with the writer()
 * function. So you may write data with:
 *
 * \code
 *      my_reader.writer().write(buf, buf_size);
 * \endcode
 *
 * \param[in] communicator  The snap communicator controlling this connection.
 * \param[in] socket  The socket to be used for writing.
 */
snap_communicator::snap_tcp_client_buffer_connection::snap_tcp_client_buffer_connection(std::string const & addr, int port, mode_t mode)
    : snap_tcp_client_connection(addr, port, mode)
{
}


/** \brief Instantiation of process_read().
 *
 * This function reads incoming data from a socket.
 *
 * The function is what manages our low level TCP/IP connection protocol
 * which is to read one line of data (i.e. bytes up to the next '\n'
 * character; note that '\r' are not understood.)
 *
 * Once a complete line of data was read, it is converted to UTF-8 and
 * sent to the next layer using the process_line() function passing
 * the line it just read (without the '\n') to that callback.
 *
 * \sa process_write()
 * \sa process_line()
 */
void snap_communicator::snap_tcp_client_buffer_connection::process_read()
{
    // we read one character at a time until we get a '\n'
    // since we have a non-blocking socket we can read as
    // much as possible and then check for a '\n' and keep
    // any extra data in a cache.
    //
    std::vector<char> buffer;
    buffer.resize(1024);
    for(;;)
    {
        errno = 0;
        ssize_t const r(::read(get_socket(), &buffer[0], buffer.size()));
        if(r > 0)
        {
            for(ssize_t position(0); position < r; )
            {
                std::vector<char>::const_iterator it(std::find(buffer.begin() + position, buffer.begin() + r, '\n'));
                if(it == buffer.begin() + r)
                {
                    // no newline, just add the whole thing
                    f_line += std::string(&buffer[position], r - position);
                    break; // do not waste time, we know we are done
                }

                // retrieve the characters up to the newline
                // character and process the line
                //
                f_line += std::string(&buffer[position], it - buffer.begin() - position);
                process_line(QString::fromUtf8(f_line.c_str()));

                // done with that line
                f_line.clear();

                // we had a newline, we may still have some data
                // in that buffer; (+1 to skip the '\n' itself)
                //
                position = it - buffer.begin() + 1;
            }
        }
        else if(r == 0 || errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // no more data available at this time
            break;
        }
        else //if(r < 0)
        {
            // TODO: do something about the error
            SNAP_LOG_ERROR("an error occured while reading from socket.");
            remove_from_communicator();
            break;
        }
    }

    // process next level too
    snap_tcp_client_connection::process_read();
}


/** \brief Instantiation of process_write().
 *
 * This function writes outgoing data to a socket.
 *
 * This function manages our own internal cache, which we use to allow
 * for out of synchronization (non-blocking) output.
 *
 * \sa write()
 * \sa process_read()
 */
void snap_communicator::snap_tcp_client_buffer_connection::process_write()
{
    errno = 0;
    ssize_t const r(::write(get_socket(), &f_output[f_position], f_output.size() - f_position));
    if(r > 0)
    {
        // some data was written
        f_position += r;
        if(f_position >= f_output.size())
        {
            f_output.clear();
            f_position = 0;
        }
    }
    else if(r < 0 && errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        // TODO: deal with error, if we lost the connection
        //       remove the whole thing and log an event

        // connection is considered bad, get rid of it
        //
        SNAP_LOG_ERROR("an error occured while writing to socket.");
        remove_from_communicator();
    }

    // process next level too
    snap_tcp_client_connection::process_write();
}


/** \brief The hang up event occurred.
 *
 * This function closes the socket and then calls the previous level
 * hang up code which removes this connection from the snap_communicator
 * object it was last added in.
 */
void snap_communicator::snap_tcp_client_buffer_connection::process_hup()
{
    // this connection is dead...
    //
    close();

    // process next level too
    snap_tcp_client_connection::process_hup();
}


/** \brief Write data to the connection.
 *
 * This function can be used to send data to this TCP/IP connection.
 * The data is bufferized and as soon as the connection can WRITE
 * to the socket, it will wake up and send the data. In other words,
 * we cannot just sleep and wait for an answer. The transfer will
 * be asynchroneous.
 *
 * \todo
 * Optimization: look into writing the \p data buffer directly in
 * the socket if the f_output cache is empty. If that works then
 * we can completely bypass our intermediate cache. This works only
 * if we make sure that the socket is non-blocking, though.
 *
 * \todo
 * Determine whether we may end up with really large buffers that
 * grow for a long time. This function only inserts and the
 * process_signal() function only reads some of the bytes but it
 * does not reduce the size of the buffer until all the data was
 * sent.
 *
 * \param[in] data  The pointer to the buffer of data to be sent.
 * \param[out] length  The number of bytes to send.
 */
void snap_communicator::snap_tcp_client_buffer_connection::write(char const * data, size_t length)
{
    if(length > 0)
    {
        f_output.insert(f_output.end(), data, data + length);
    }
}


/** \brief The buffer is a writer when the output buffer is not empty.
 *
 * This function returns true as long as the output buffer of this
 * client connection is not empty.
 *
 * \return true if the output buffer is not empty, false otherwise.
 */
bool snap_communicator::snap_tcp_client_buffer_connection::is_writer() const
{
    return !f_output.empty();
}


/** \fn snap_communicator::snap_tcp_client_buffer_connection::process_line(QString const & line);
 * \brief Process a line of data.
 *
 * This is the default virtual class that can be overridden to implement
 * your own processing. By default this function does nothing.
 *
 * \note
 * At this point I implemented this function so one can instantiate
 * a snap_tcp_server_client_buffer_connection without having to
 * derive it, although I do not think that is 100% proper.
 *
 * \param[in] line  The line of data that was just read from the input
 *                  socket.
 */





///////////////////////////////////////////////
// Snap TCP Server Message Buffer Connection //
///////////////////////////////////////////////

/** \brief Initializes a client to read messages from a socket.
 *
 * This implementation creates a message in/out client.
 * This is the most useful client in our Snap! Communicator
 * as it directly sends and receives messages.
 *
 * \param[in] communicator  The communicator connected with this client.
 * \param[in] socket  The in/out socket.
 */
snap_communicator::snap_tcp_client_message_connection::snap_tcp_client_message_connection(std::string const & addr, int port, mode_t mode)
    : snap_tcp_client_buffer_connection(addr, port, mode)
{
}


/** \brief Process a line (string) just received.
 *
 * The function parses the line as a message (snap_communicator_message)
 * and then calls the process_message() function if the line was valid.
 *
 * \param[in] line  The line of text that was just read.
 */
void snap_communicator::snap_tcp_client_message_connection::process_line(QString const & line)
{
    if(line.isEmpty())
    {
        return;
    }

    snap_communicator_message message;
    if(message.from_message(line))
    {
        process_message(message);
    }
    else
    {
        // TODO: what to do here? This could because the version changed
        //       and the messages are not compatible anymore.
        //
        SNAP_LOG_ERROR("snap_communicator::snap_tcp_server_client_message_reader_connection::process_line() was asked to process an invalid message (")(line)(")");
    }
}


/** \brief Send a message.
 *
 * This function sends a message to the client on the other side
 * of this connection.
 *
 * \param[in] message  The message to be processed.
 */
void snap_communicator::snap_tcp_client_message_connection::send_message(snap_communicator_message const & message)
{
    // transform the message to a string and write to the socket
    // the writing is asynchronous so the message is saved in a cache
    // and transferred only later when the run() loop is hit again
    //
    QString const msg(message.to_message());
    QByteArray const utf8(msg.toUtf8());
    std::string buf(utf8.data(), utf8.size());
    buf += "\n";
    write(buf.c_str(), buf.length());
}








////////////////////////////////
// Snap TCP Server Connection //
////////////////////////////////


/** \brief Initialize a server connection.
 *
 * This function is used to initialize a server connection, a TCP/IP
 * listener which can accept() new connections.
 *
 * \param[in] communicator  The snap communicator controlling this connection.
 * \param[in] addr  The address to listen on. It may be set to "0.0.0.0".
 * \param[in] port  The port to listen on.
 * \param[in] max_connections  The number of connections to keep in the listen queue.
 * \param[in] reuse_addr  Whether to mark the socket with the SO_REUSEADDR flag.
 * \param[in] auto_close  Automatically close the client socket in accept and the destructor.
 */
snap_communicator::snap_tcp_server_connection::snap_tcp_server_connection(std::string const & addr, int port, int max_connections, bool reuse_addr, bool auto_close)
    : tcp_server(addr, port, max_connections, reuse_addr, auto_close)
{
}


/** \brief Reimplement the is_listener() for the snap_tcp_server_connection.
 *
 * A server connection is a listener socket. The library makes
 * use of a completely different callback when a "read" event occurs
 * on these connections.
 *
 * The callback is expected to create the new connection and add
 * it the communicator.
 *
 * \return This version of the function always returns true.
 */
bool snap_communicator::snap_tcp_server_connection::is_listener() const
{
    return true;
}


/** \brief Retrieve the socket of this server connection.
 *
 * This function retrieves the socket this server connection. In this case
 * the socket is defined in the tcp_server class.
 *
 * \return The socket of this client connection.
 */
int snap_communicator::snap_tcp_server_connection::get_socket() const
{
    return tcp_server::get_socket();
}







///////////////////////////////////////
// Snap TCP Server Client Connection //
///////////////////////////////////////


/** \brief Create a client connection created from an accept().
 *
 * This constructor initializes a client connection from a socket
 * that we received from an accept() call.
 *
 * The destructor will automatically close that socket on destruction.
 *
 * \param[in] communicator  The snap communicator controlling this connection.
 * \param[in] socket  The socket that acecpt() returned.
 */
snap_communicator::snap_tcp_server_client_connection::snap_tcp_server_client_connection(int socket)
    : f_socket(socket < 0 ? -1 : socket)
{
}


/** \brief Make sure the socket gets released once we are done witht he connection.
 *
 * This destructor makes sure that the socket gets closed.
 */
snap_communicator::snap_tcp_server_client_connection::~snap_tcp_server_client_connection()
{
    close();
}


/** \brief Close the socket of this connection.
 *
 * This function is automatically called whenever the object gets
 * destroyed (see destructor) or detects that the client closed
 * the network connection.
 *
 * Connections cannot be reopened.
 */
void snap_communicator::snap_tcp_server_client_connection::close()
{
    if(f_socket != -1)
    {
        if(::close(f_socket) != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("closing socket generated error: ")(e);
        }
        f_socket = -1;
    }
}


/** \brief Retrieve the socket of this connection.
 *
 * This function returns the socket defined in this connection.
 */
int snap_communicator::snap_tcp_server_client_connection::get_socket() const
{
    return f_socket;
}


/** \brief Tell that we are always a reader.
 *
 * This function always returns true meaning that the connection is
 * always of a reader. In most cases this is safe because if nothing
 * is being written to you then poll() never returns so you do not
 * waste much time in have a TCP connection always marked as a
 * reader.
 *
 * \return The events to listen to for this connection.
 */
bool snap_communicator::snap_tcp_server_client_connection::is_reader() const
{
    return true;
}


/** \brief The address of this client connection.
 *
 * This function saves the address of the client connection.
 *
 * There are times when we are given the address so we call this function
 * to save it instead of having to query it again later.
 *
 * \exception snap_communicator_parameter_error
 * The address pointer cannot be nullptr. The address cannot be larger
 * than struct sockaddr. If either parameter is wrong, then this
 * exception is raised.
 *
 * \param[in] address  The address of this client connection.
 * \param[in] length  The length of the address defined in addr.
 */
void snap_communicator::snap_tcp_server_client_connection::set_address(struct sockaddr * address, size_t length)
{
    if(address == nullptr
    || length > sizeof(f_address))
    {
        throw snap_communicator_parameter_error("snap_communicator::snap_tcp_server_client_connection::save_address(): the address received by evconnlistener is larger than our sockaddr.");
    }

    // keep a copy of the address
    memcpy(&f_address, address, length);
    if(length < sizeof(f_address))
    {
        // reset the rest of the structure, just in case
        memset(reinterpret_cast<char *>(&f_address) + length, 0, sizeof(f_address) - length);
    }

    f_length = length;
}


/** \brief Retrieve a copy of the client's address.
 *
 * This function makes a copy of the address of this client connection
 * to the \p address parameter and returns the length.
 *
 * \todo
 * To be compatible with the tcp implementation we need to return
 * a string here or have a get_addr() which converts the address
 * into a string.
 *
 * \param[in] address  The reference to an address variable where the
 *                     address gets copied.
 *
 * \return Return the length of the address which may be smaller than
 *         sizeof(struct sockaddr). If zero, then no address is defined.
 */
size_t snap_communicator::snap_tcp_server_client_connection::get_address(struct sockaddr & address) const
{
    address = f_address;
    return f_length;
}


/** \brief Save the address defined as a string.
 *
 * This function is used to transform the \p addr parameter to an address
 * and save that internally. This function is called when you get the
 * address as a string instead of a raw sockaddr structure.
 *
 * \param[in] addr  The address to save in this client connection.
 */
void snap_communicator::snap_tcp_server_client_connection::set_addr(std::string const & addr)
{
    inet_aton(addr.c_str(), reinterpret_cast<struct in_addr *>(&f_address));
}


/** \brief Retrieve the address in the form of a string.
 *
 * Like the get_addr() of the tcp client and server classes, this
 * function returns the address in the form of a string which can
 * easily be used to log information and other similar tasks.
 *
 * \return The client's address in the form of a string.
 */
std::string snap_communicator::snap_tcp_server_client_connection::get_addr() const
{
    char buf[INET_ADDRSTRLEN];
    char const * r;

    if(f_address.sa_family == AF_INET)
    {
        r = inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in const &>(f_address).sin_addr, buf, INET_ADDRSTRLEN);
    }
    else
    {
        r = inet_ntop(AF_INET6, &reinterpret_cast<struct sockaddr_in6 const &>(f_address).sin6_addr, buf, INET_ADDRSTRLEN);
    }

    if(r == nullptr)
    {
        throw snap_communicator_runtime_error("snap_tcp_server_client_connection::get_addr(): inet_ntop() could not convert IP address properly.");
    }

    return buf;
}







////////////////////////////////
// Snap TCP Buffer Connection //
////////////////////////////////

/** \brief Initialize a client socket.
 *
 * The client socket gets initialized with the specified 'socket'
 * parameter.
 *
 * If you are a pure client (opposed to a client that was just accepted)
 * you may want to consider using the snap_tcp_client_buffer_connection
 * instead. That gives you a way to open the socket from a set of address
 * and port definitions among other things.
 *
 * This initialization, so things work as expected in our environment,
 * the function marks the socket as non-blocking. This is important for
 * the reader and writer capabilities.
 *
 * \param[in] communicator  The snap communicator controlling this connection.
 * \param[in] socket  The socket to be used for reading and writing.
 */
snap_communicator::snap_tcp_server_client_buffer_connection::snap_tcp_server_client_buffer_connection(int socket)
    : snap_tcp_server_client_connection(socket)
{
    non_blocking();
}


/** \brief Read and process as much data as possible.
 *
 * This function reads as much incoming data as possible and processes
 * it.
 *
 * \todo
 * Look into a way, if possible to have a single instantiation since
 * as far as I know this code matches the one written in the
 * process_read() of the snap_tcp_client_buffer_connection class.
 */
void snap_communicator::snap_tcp_server_client_buffer_connection::process_read()
{
    // we read one character at a time until we get a '\n'
    // since we have a non-blocking socket we can read as
    // much as possible and then check for a '\n' and keep
    // any extra data in a cache.
    //
    std::vector<char> buffer;
    buffer.resize(1024);
    for(;;)
    {
        errno = 0;
        ssize_t const r(::read(get_socket(), &buffer[0], buffer.size()));
        if(r > 0)
        {
            for(ssize_t position(0); position < r; )
            {
                std::vector<char>::const_iterator it(std::find(buffer.begin() + position, buffer.begin() + r, '\n'));
                if(it == buffer.begin() + r)
                {
                    // no newline, just add the whole thing
                    f_line += std::string(&buffer[position], r - position);
                    break; // do not waste time, we know we are done
                }

                // retrieve the characters up to the newline
                // character and process the line
                //
                f_line += std::string(&buffer[position], it - buffer.begin() - position);
                process_line(QString::fromUtf8(f_line.c_str()));

                // done with that line
                f_line.clear();

                // we had a newline, we may still have some data
                // in that buffer; (+1 to skip the '\n' itself)
                //
                position = it - buffer.begin() + 1;
            }
        }
        else if(r == 0 || errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // no more data available at this time
            break;
        }
        else //if(r < 0)
        {
            // TODO: do something about the error
            SNAP_LOG_ERROR("an error occured while reading from socket.");
            remove_from_communicator();
            break;
        }
    }
}


/** \brief Write to the connection's socket.
 *
 * This function writes as much data as possible to the
 * connection's socket.
 */
void snap_communicator::snap_tcp_server_client_buffer_connection::process_write()
{
    errno = 0;
    ssize_t r(::write(get_socket(), &f_output[f_position], f_output.size() - f_position));
    if(r > 0)
    {
        // some data was written
        f_position += r;
        if(f_position >= f_output.size())
        {
            f_output.clear();
            f_position = 0;
        }
    }
    else if(r != 0 && errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        // TODO: deal with error, if we lost the connection
        //       remove the whole thing and log an event

        // connection is considered bad, get rid of it
        //
        remove_from_communicator();
    }
}


/** \brief The remote used hanged up.
 *
 * This function makes sure that the connection gets closed properly.
 */
void snap_communicator::snap_tcp_server_client_buffer_connection::process_hup()
{
    // this connection is dead...
    //
    close();

    remove_from_communicator();
}


/** \brief Write data to the connection.
 *
 * This function can be used to send data to this TCP/IP connection.
 * The data is bufferized and as soon as the connection can WRITE
 * to the socket, it will wake up and send the data. In other words,
 * we cannot just sleep and wait for an answer. The transfer will
 * be asynchroneous.
 *
 * \todo
 * Determine whether we may end up with really large buffers that
 * grow for a long time. This function only inserts and the
 * process_signal() function only reads some of the bytes but it
 * does not reduce the size of the buffer until all the data was
 * sent.
 *
 * \param[in] data  The pointer to the buffer of data to be sent.
 * \param[out] length  The number of bytes to send.
 */
void snap_communicator::snap_tcp_server_client_buffer_connection::write(char const * data, size_t length)
{
    if(length > 0)
    {
        f_output.insert(f_output.end(), data, data + length);
    }
}


/** \brief Tells that this connection is a writer when we have data to write.
 *
 * This function checks to know whether there is data to be writen to
 * this connection socket. If so then the function returns true. Otherwise
 * it just returns false.
 *
 * This happens whenever you called the write() function and our cache
 * is not empty yet.
 *
 * \return true if there is data to write to the socket, false otherwise.
 */
bool snap_communicator::snap_tcp_server_client_buffer_connection::is_writer() const
{
    return !f_output.empty();
}







////////////////////////////////////////
// Snap TCP Server Message Connection //
////////////////////////////////////////

/** \brief Initializes a client to read messages from a socket.
 *
 * This implementation creates a message in/out client.
 * This is the most useful client in our Snap! Communicator
 * as it directly sends and receives messages.
 *
 * \param[in] communicator  The communicator connected with this client.
 * \param[in] socket  The in/out socket.
 */
snap_communicator::snap_tcp_server_client_message_connection::snap_tcp_server_client_message_connection(int socket)
    : snap_tcp_server_client_buffer_connection(socket)
{
}


/** \brief Process a line (string) just received.
 *
 * The function parses the line as a message (snap_communicator_message)
 * and then calls the process_message() function if the line was valid.
 *
 * \param[in] line  The line of text that was just read.
 */
void snap_communicator::snap_tcp_server_client_message_connection::process_line(QString const & line)
{
    // empty lines should not occur, but just in case, just ignore
    if(line.isEmpty())
    {
        return;
    }

    snap_communicator_message message;
    if(message.from_message(line))
    {
        process_message(message);
    }
    else
    {
        // TODO: what to do here? This could because the version changed
        //       and the messages are not compatible anymore.
        //
        SNAP_LOG_ERROR("snap_communicator::snap_tcp_server_client_message_connection::process_line() was asked to process an invalid message (")(line)(")");
    }
}


/** \brief Send a message.
 *
 * This function sends a message to the client on the other side
 * of this connection.
 *
 * \param[in] message  The message to be processed.
 */
void snap_communicator::snap_tcp_server_client_message_connection::send_message(snap_communicator_message const & message)
{
    // transform the message to a string and write to the socket
    // the writing is asynchronous so the message is saved in a cache
    // and transferred only later when the run() loop is hit again
    //
    QString const msg(message.to_message());
    QByteArray const utf8(msg.toUtf8());
    std::string buf(utf8.data(), utf8.size());
    buf += "\n";
    write(buf.c_str(), buf.length());
}




////////////////////////////////
// Snap UDP Server Connection //
////////////////////////////////


/** \brief Initialize a UDP listener.
 *
 * This function is used to initialize a server connection, a UDP/IP
 * listener which wakes up whenever a send() is sent to this listener
 * address and port.
 *
 * \param[in] communicator  The snap communicator controlling this connection.
 * \param[in] addr  The address to listen on. It may be set to "0.0.0.0".
 * \param[in] port  The port to listen on.
 */
snap_communicator::snap_udp_server_connection::snap_udp_server_connection(std::string const & addr, int port)
    : udp_server(addr, port)
{
}


/** \brief Retrieve the socket of this server connection.
 *
 * This function retrieves the socket this server connection. In this case
 * the socket is defined in the udp_server class.
 *
 * \return The socket of this client connection.
 */
int snap_communicator::snap_udp_server_connection::get_socket() const
{
    return udp_server::get_socket();
}


/** \brief Check to know whether this UDP connection is a reader.
 *
 * This function returns true to say that this UDP connection is
 * indeed a reader.
 *
 * \return This function already returns true as we are likely to
 *         always want a UDP socket to be listening for incoming
 *         packets.
 */
bool snap_communicator::snap_udp_server_connection::is_reader() const
{
    return true;
}



////////////////////////////////
// Snap UDP Server Connection //
////////////////////////////////


/** \brief Initialize a UDP server to send and receive messages.
 *
 * This function initialises a UDP server as a Snap UDP server
 * connection attached to the specified address and port.
 *
 * It is expected to be used to send and receive UDP messages.
 *
 * Note that to send messages, you need the address and port
 * of the destination. In effect, we do not use this server
 * when sending. Instead we create a client that we immediately
 * destruct once the message was sent.
 *
 * \param[in] addr  The address to listen on.
 * \param[in] port  The port to listen on.
 */
snap_communicator::snap_udp_server_message_connection::snap_udp_server_message_connection(std::string const & addr, int port)
    : snap_udp_server_connection(addr, port)
{
    // allow for looping over all the messages in one go
    //
    non_blocking();
}


/** \brief Send a UDP message.
 *
 * This function offers you to send a UDP message to the specified
 * address and port. The message should be small enough to fit in
 * on UDP packet or the call will fail.
 *
 * \note
 * The function return true when the message was successfully sent.
 * This does not mean it was received.
 *
 * \param[in] addr  The destination address for the message.
 * \param[in] port  The destination port for the message.
 * \param[in] message  The message to send to the destination.
 *
 * \return true when the message was sent, false otherwise.
 */
bool snap_communicator::snap_udp_server_message_connection::send_message(std::string const & addr, int port, snap_communicator_message const & message)
{
    // Note: contrary to the TCP version, a UDP message does not
    //       need to include the '\n' character since it is sent
    //       in one UDP packet.
    //
    udp_client_server::udp_client client(addr, port);
    QString const msg(message.to_message());
    QByteArray const utf8(msg.toUtf8());
    if(static_cast<size_t>(utf8.size()) > DATAGRAM_MAX_SIZE)
    {
        // packet too large for our buffers
        throw snap_communicator_invalid_message("message too large for a UDP server");
    }
    if(client.send(utf8.data(), utf8.size()) != utf8.size()) // we do not send the '\0'
    {
        SNAP_LOG_ERROR("snap_udp_server_message_connection::send_message(): could not send UDP message.");
        return false;
    }

    return true;
}


/** \brief Implementation of the process_read() callback.
 *
 * This function reads the datagram we just received using the
 * recv() function. The size of the datagram cannot be more than
 * DATAGRAM_MAX_SIZE (1Kb at time of writing.)
 *
 * The message is then parsed and further processing is expected
 * to be accomplished in your implementation of process_message().
 *
 * The function actually reads as many pending datagrams as it can.
 */
void snap_communicator::snap_udp_server_message_connection::process_read()
{
    char buf[DATAGRAM_MAX_SIZE];
    for(;;)
    {
        ssize_t const r(recv(buf, sizeof(buf) / sizeof(buf[0]) - 1));
        if(r <= 0)
        {
            break;
        }
        buf[r] = '\0';
        QString const udp_message(QString::fromUtf8(buf));
        snap::snap_communicator_message message;
        if(message.from_message(udp_message))
        {
            // we received a valid message, process it
            process_message(message);
        }
    }
}




///////////////////////
// Snap Communicator //
///////////////////////


/** \brief Initialize a snap communicator object.
 *
 * This function initializes the snap_communicator object.
 */
snap_communicator::snap_communicator()
    //: f_connections() -- auto-init
    //, f_force_sort(true) -- auto-init
{
}


/** \brief Retrieve the instance() of the snap_communicator.
 *
 * This function returns the instance of the snap_communicator.
 * There is really no reason and it could also create all sorts
 * of problems to have more than one instance hence we created
 * the communicator as a singleton. It also means you cannot
 * actually delete the communicator.
 */
snap_communicator::pointer_t snap_communicator::instance()
{
    if(!g_instance)
    {
        g_instance.reset(new snap_communicator);
    }

    return g_instance;
}


/** \brief Retrieve a reference to the vector of connections.
 *
 * This function returns a reference to all the connections that are
 * currently attached to the snap_communicator system.
 *
 * This is useful to search the array.
 *
 * \return The vector of connections.
 */
snap_communicator::snap_connection::vector_t const & snap_communicator::get_connections() const
{
    return f_connections;
}


/** \brief Attach a connection to the communicator.
 *
 * This function attaches a connection to the communicator. This allows
 * us to execute code for that connection by having the process_signal()
 * function called.
 *
 * Connections are kept in the order in which they are added. This may
 * change the order in which connection callbacks are called. However,
 * events are received asynchronously so do not expect callbacks to be
 * called in any specific order.
 *
 * \note
 * A connection can only be added once to a snap_communicator object.
 * Also it cannot be shared between multiple communicator objects.
 *
 * \param[in] connection  The connection being added.
 *
 * \return true if the connection was added, false if the connection
 *         was already present in the communicator list of connections.
 */
bool snap_communicator::add_connection(snap_connection::pointer_t connection)
{
    if(!connection->valid_socket())
    {
        throw snap_communicator_parameter_error("snap_communicator::add_connection(): connection without a socket cannot be added to a snap_communicator object.");
    }

    auto const it(std::find(f_connections.begin(), f_connections.end(), connection));
    if(it != f_connections.end())
    {
        // already added, can be added only once but we allow multiple
        // calls (however, we do not count those calls, so first call
        // to the remove_connection() does remove it!)
        return false;
    }

    f_connections.push_back(connection);

    return true;
}


/** \brief Remove a connection from a snap_communicator object.
 *
 * This function removes a connection from this snap_communicator object.
 * Note that any one connection can only be added once.
 *
 * \param[in] connection  The connection to remove from this snap_communicator.
 *
 * \return true if the connection was removed, false if it was not found.
 */
bool snap_communicator::remove_connection(snap_connection::pointer_t connection)
{
    auto it(std::find(f_connections.begin(), f_connections.end(), connection));
    if(it == f_connections.end())
    {
        return false;
    }

    f_connections.erase(it);

    return true;
}


/** \brief Run until all connections are removed.
 *
 * This function "blocks" until all the events added to this
 * snap_communicator instance are removed. Until then, it
 * wakes up and run callback functions whenever an event occurs.
 *
 * In other words, you want to add_connection() before you call
 * this function otherwise the function returns immediately.
 *
 * Note that you can include timeout events so if you need to
 * run some code once in a while, you may just use a timeout
 * event and process your repetitive events that way.
 *
 * \return true if the loop exits because the list of connections is empty.
 */
bool snap_communicator::run()
{
    // the loop promises to exit once the even_base object has no
    // more connections attached to it
    //
    std::vector<struct pollfd> fds;
    f_force_sort = true;
    for(;;)
    {
        // any connections?
        if(f_connections.empty())
        {
            return true;
        }

        if(f_force_sort)
        {
            // sort the connections by priority
            //
            sort(f_connections.begin(), f_connections.end());
            f_force_sort = false;
        }

        // make a copy because the callbacks may end up making
        // changes to the main list and we would have problems
        // with that here...
        //
        snap_connection::vector_t connections(f_connections);
        size_t max_connections(connections.size());

        // timeout is do not time out by default
        //
        int64_t next_timeout_timestamp(std::numeric_limits<int64_t>::max());

        fds.clear(); // this is not supposed to delete the buffer
        fds.reserve(max_connections); // avoid more than 1 allocation
        for(auto c : connections)
        {
            // is the connection enabled?
            if(!c->is_enabled())
            {
                continue;
            }

            int64_t const timestamp(c->save_timeout_timestamp());
            if(timestamp != -1)
            {
                // the timeout event gives us a time when to tick
                //
                if(timestamp < next_timeout_timestamp)
                {
                    next_timeout_timestamp = timestamp;
                }
            }

            // is there any events to listen on?
            int e(0);
            if(c->is_listener())
            {
                e |= POLLIN;
            }
            if(c->is_reader())
            {
                e |= POLLIN | POLLPRI | POLLRDHUP;
            }
            if(c->is_writer())
            {
                e |= POLLOUT | POLLRDHUP;
            }
            if(e == 0)
            {
                continue;
            }

            // do we have a currently valid socket (i.e. the connection
            // may have been closed or we may be handling a timer or
            // signal object)
            //
            if(c->get_socket() < 0)
            {
                continue;
            }

            // this is considered valid, add this connection to the list
            //
            // save the position since we may skip some entries...
            // (otherwise we would have to use -1 as the socket to
            // allow for such dead entries, but avoiding such entries
            // saves time)
            //
            c->f_fds_position = fds.size();

            struct pollfd fd;
            fd.fd = c->get_socket();
            fd.events = e;
            fd.revents = 0; // probably useless... (kernel should clear those)
            fds.push_back(fd);
        }

        if(fds.size() == 0)
        {
            // TODO: add support for timeout and signal only situations...
            //
            SNAP_LOG_FATAL("snap_communicator::run(): nothing to poll() on. All file connections are disabled or you only have timer and signal \"connections\" which is not yet supported.");
            return false;
        }

        // compute the right timeout
        int64_t timeout(-1);
        if(next_timeout_timestamp != std::numeric_limits<int64_t>::max())
        {
            int64_t const now(get_current_date());
            timeout = next_timeout_timestamp - now;
            if(timeout < 0)
            {
                // timeout is in the past so timeout immediately, but
                // still check for events if any
                timeout = 0;
            }
            else
            {
                // convert microseconds to milliseconds for poll()
                timeout /= 1000;
                if(timeout == 0)
                {
                    // less than one is a waste of time (CPU intessive
                    // until the time is reached, we can be 1 ms off
                    // instead...)
                    timeout = 1;
                }
            }
        }
//std::cerr << QString("%1: timeout %2 (next was: %3, current ~ %4)\n").arg(getpid()).arg(timeout).arg(next_timeout_timestamp).arg(get_current_date());

        // TODO: add support for ppoll() so we can support signals cleanly
        //       with nearly no additional work from us
        //
        errno = 0;
        int const r(poll(&fds[0], fds.size(), timeout));
        if(r >= 0)
        {
            // quick sanity check
            //
            if(static_cast<size_t>(r) > connections.size())
            {
                throw snap_communicator_runtime_error("poll() returned a number larger than the input");
            }

            // check each connection one by one for:
            //
            // 1) signals
            // 2) fds events
            // 3) timeouts
            //
            // and execute the corresponding callbacks
            //
            for(size_t idx(0); idx < connections.size(); ++idx)
            {
                snap_connection::pointer_t c(connections[idx]);
                struct pollfd * fd(&fds[c->f_fds_position]);

                // we consider that signals have the greater priority
                // and thus handle them first
                //
                if(c->is_signal())
                {
                    snap_signal *ss(dynamic_cast<snap_signal *>(c.get()));
                    if(ss && ss->is_active())
                    {
                        ss->activate(false);
                        c->process_signal();
                    }
                }

                // if any events were found by poll(), process them now
                //
                if(fd->revents != 0)
                {
                    // an event happened on this one
                    //
                    if((fd->revents & (POLLIN | POLLPRI)) != 0)
                    {
                        if(c->is_listener())
                        {
                            // a listener is a special case and we want
                            // to call process_accept() instead
                            //
                            c->process_accept();
                        }
                        else
                        {
                            c->process_read();
                        }
                    }
                    if((fd->revents & POLLOUT) != 0)
                    {
                        c->process_write();
                    }
                    if((fd->revents & POLLERR) != 0)
                    {
                        c->process_error();
                    }
                    if((fd->revents & (POLLHUP | POLLRDHUP)) != 0)
                    {
                        c->process_hup();
                    }
                    if((fd->revents & POLLNVAL) != 0)
                    {
                        c->process_invalid();
                    }
                }

                // now check whether we have a timeout on this connection
                //
                int64_t const timestamp(c->get_saved_timeout_timestamp());
                if(timestamp != -1)
                {
                    int64_t const now(get_current_date());
                    if(now >= timestamp)
                    {
                        // move the timeout as required first
                        c->calculate_next_tick();
                        c->set_timeout_date(-1);

                        // then run the callback
                        c->process_timeout();
                    }
                }
            }
        }
        else
        {
            // r < 0 means an error occurred
            //
            if(errno == EINTR)
            {
                // TODO: interrupt happened, check for a signal connection
                //       this probably require us to use ppoll() instead of
                //       just poll()... right now we do not support but we
                //       probably want to use SIGHUP and a few other signals
                //       in various servers (i.e. SIGUSR1, SIGUSR2...)
                //
                throw snap_communicator_runtime_error("EINTR occurred while in poll() -- interrupts are not supported yet though");
            }
            if(errno == EFAULT)
            {
                throw snap_communicator_parameter_error("buffer was moved out of our address space?");
            }
            if(errno == EINVAL)
            {
                // if this is really because nfds is too large then it may be
                // a "soft" error that can be fixed; that being said, my
                // current version is 16K files which frankly when we reach
                // that level we have a problem...
                //
                struct rlimit rl;
                getrlimit(RLIMIT_NOFILE, &rl);
                throw snap_communicator_parameter_error(QString("too many file fds for poll, limit is currently %1, your kernel top limit is %2")
                            .arg(rl.rlim_cur)
                            .arg(rl.rlim_max).toStdString());
            }
            if(errno == ENOMEM)
            {
                throw snap_communicator_runtime_error("poll() failed because of memory");
            }
            int const e(errno);
            throw snap_communicator_runtime_error(QString("poll() failed with error %1").arg(e).toStdString());
        }
    }
}





/** \brief Get the current date.
 *
 * This function retrieves the current date and time with a precision
 * to the microseconds.
 *
 * \todo
 * This is also defined in snap_child::get_current_date() so we should
 * unify that in some way...
 */
int64_t snap_communicator::get_current_date()
{
    struct timeval tv;
    if(gettimeofday(&tv, nullptr) != 0)
    {
        int const err(errno);
        SNAP_LOG_FATAL("gettimeofday() failed with errno: ")(err);
        throw std::runtime_error("gettimeofday() failed");
    }
    return static_cast<int64_t>(tv.tv_sec) * static_cast<int64_t>(1000000)
         + static_cast<int64_t>(tv.tv_usec);
}




} // namespace snap
// vim: ts=4 sw=4 et
