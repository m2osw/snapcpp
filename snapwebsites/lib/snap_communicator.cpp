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
 * This class wraps the libevent in a C++ object with three types
 * of objects:
 *
 * \li Client Connections; for software that want to connect to
 *     a server
 * \li Server Connections; for software that want to offer a port to
 *     which clients can connect to; the server will call accept()
 *     once the socket is ready
 * \li Server Client Connections; for the server when it accepts a new
 *     connection; in this case the server gets a socket from accept()
 *     and creates one of these objects to handle the connection
 *
 * The libevent library is well documented on this page:
 *
 * http://www.wangafu.net/~nickm/libevent-book/
 *
 * The library home page is here:
 *
 * http://libevent.org/
 */

#include "snap_communicator.h"

#include "log.h"
#include "not_reached.h"
#include "not_used.h"

#include <sstream>

#include <event2/event.h>
#include <event2/listener.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "poison.h"

namespace snap
{
namespace
{


/** \brief Delete an event base defined in a Unique pointer.
 *
 * This function is used as the deleter of an event_base in a
 * unique_ptr<>() object. This way we do not have to deal with
 * deleting an event_base object.
 *
 * The library defines a private typedef: unique_event_base_t.
 *
 * \param[in] eb  The event base object to delete.
 */
void delete_event_base(void * eb)
{
    event_base_free(static_cast<struct event_base *>(eb));
}


/** \brief Delete the event configuration structure.
 *
 * This function is used with the Unique pointer to automatically
 * delete an event_config structure once done with it.
 *
 * \param[in] ec  The event configuration structure to delete.
 */
void delete_event_config(void * ec)
{
    event_config_free(static_cast<struct event_config *>(ec));
}


/** \brief Delete the event structure.
 *
 * This function is used with the Unique pointer to automatically
 * delete an event structure once done with it.
 *
 * \param[in] evt  The event structure to delete.
 */
void delete_event(void * evt)
{
    event_free(static_cast<struct event *>(evt));
}


/** \brief Delete the listener structure.
 *
 * This function is used with the Unique pointer to automatically
 * delete a listener structure once done with it.
 *
 * \param[in] evt  The listener structure to delete.
 */
void delete_listener(void * listener)
{
    evconnlistener_free(static_cast<struct evconnlistener *>(listener));
}


/** \brief Convert a libevent what parameter to a snap_communicator one.
 *
 * This function converts the set of events we receive from the libevent
 * library to a set of events one can use with the snap communictor
 * environment.
 *
 * \param[in] what  A libevent set of event flags.
 *
 * \return A set of snap communicator event flags.
 */
snap_communicator::what_event_t what_to_what_event(int what)
{
    return ((what & EV_TIMEOUT) == 0 ? 0 : snap_communicator::EVENT_TIMEOUT)
         | ((what & EV_READ   ) == 0 ? 0 : snap_communicator::EVENT_READ)
         | ((what & EV_WRITE  ) == 0 ? 0 : snap_communicator::EVENT_WRITE)
         | ((what & EV_SIGNAL ) == 0 ? 0 : snap_communicator::EVENT_SIGNAL)
         //| ((what & EV_PERSIST) == 0 ? 0 : snap_communicator::EVENT_PERSIST)
         //| ((what & EV_ET     ) == 0 ? 0 : snap_communicator::EVENT_ET)
    ;
}


/** \brief Convert a snap_communicator set of events to a libevent one.
 *
 * This function converts the set of snap communicator events to one
 * we can provide the libevent library.
 *
 * \param[in] we  A snap communicator set of event flags.
 *
 * \return A set of libevent flags.
 */
int what_event_to_what(snap_communicator::what_event_t we)
{
    return ((we & snap_communicator::EVENT_TIMEOUT) == 0 ? 0 : EV_TIMEOUT)
         | ((we & snap_communicator::EVENT_READ   ) == 0 ? 0 : EV_READ)
         | ((we & snap_communicator::EVENT_WRITE  ) == 0 ? 0 : EV_WRITE)
         | ((we & snap_communicator::EVENT_SIGNAL ) == 0 ? 0 : EV_SIGNAL)
         //| ((we & snap_communicator::EVENT_PERSIST) == 0 ? 0 : EV_PERSIST)
         //| ((we & snap_communicator::EVENT_ET     ) == 0 ? 0 : EV_ET)
    ;
}


/** \brief Callback used whenever a new connection is accepted.
 *
 * The libevent library has a special set of functions to handle
 * a server listening on a socket. This is called an evconnlistener.
 * This special set of functions include a special callback that
 * gets called whenever a new connection is accepted on the socket.
 *
 * \param[in] listener  The concerned listener.
 * \param[in] s  The new socket.
 * \param[in] addr  The client address.
 * \param[in] len  The length of the client address.
 * \param[in] ctx  The context, for use it is a snap_connection object.
 */
void snap_accept_callback(struct evconnlistener * listener, evutil_socket_t s, struct sockaddr * addr, int len, void * ctx)
{
    NOTUSED(listener);

    snap_communicator::snap_connection * sc(reinterpret_cast<snap_communicator::snap_connection *>(ctx));

    // create a client
    snap_communicator::snap_connection::pointer_t client(sc->create_new_connection(s));
    if(!client)
    {
        // allocation failed
        throw snap_communicator_initialization_error("snap_accept_callback(): creating of a new client connection from an accept failed.");
    }
    snap_communicator::snap_tcp_server_client_connection * server_client(dynamic_cast<snap_communicator::snap_tcp_server_client_connection *>(client.get()));
    if(server_client == nullptr)
    {
        // invalid type
        throw snap_communicator_initialization_error("snap_accept_callback(): new client connection created from an accept did not create a compatible connection type.");
    }
    server_client->set_address(addr, len);

    // process the signal (we call the same function as for others, not
    // too sure with libevent decided on a completely different scheme
    // on this one...)
    //
    sc->process_signal(snap_communicator::EVENT_ACCEPT, client);
}


/** \brief Callback used whenever an event is received.
 *
 * This function gets called any time the event library receives an
 * event and dispatches it.
 *
 * \param[in] s  The socket that triggered an event.
 * \param[in] what  The set of events that were triggered.
 * \param[in] arg  The snap_connection (bare) pointer.
 */
void snap_event_callback(evutil_socket_t s, short what, void * arg)
{
    NOTUSED(s);

    // convert the argument to a snap connection pointer
    snap_communicator::snap_connection * sc(reinterpret_cast<snap_communicator::snap_connection *>(arg));

    // call the C++ snap connection callback
    sc->process_signal(what_to_what_event(what), snap_communicator::snap_connection::pointer_t());
}


} // no name namespace




///////////////////////////////
// Snap Communicator Message //
///////////////////////////////


bool snap_communicator_message::from_message(QString const & message)
{
    QString command;
    QString name;
    parameters_t parameters;

    QChar const * m(message.constData());
    bool has_name(false);
    for(; !m->isNull() && m->unicode() == ' '; ++m)
    {
        if(m->unicode() == '/')
        {
            if(has_name
            || command.isEmpty())
            {
                // we cannot have more than one '/'
                // and the name cannot be empty if '/' is used
                return false;
            }
            has_name = true;
            name = command;
            command.clear();
        }
        else
        {
            command += *m;
        }
    }

    if(command.isEmpty())
    {
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
                verify_parameter_name(name);
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
                if(m->unicode() == ';')
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

    f_name = name;
    f_command = command;
    f_parameters.swap(parameters);

    return true;
}


QString snap_communicator_message::to_message() const
{
    if(f_command.isEmpty())
    {
        throw snap_communicator_invalid_message("snap_communicator_message::to_message(): cannot build a valid message without at least a command.");
    }

    // <name>/
    QString result(f_name);
    if(!result.isEmpty())
    {
        result += "/";
    }

    // [<name>/]command
    result += f_command;

    // then add parameters
    bool first(true);
    for(auto p(f_parameters.begin());
             p != f_parameters.end();
             ++p)
    {
        result += QString("%1%2=").arg(first ? " " : ";").arg(p.key());
        QString param(p.value());
        param.replace("\n", "\\n")   // newline needs to be escaped
             .replace("\r", "\\r");  // this one is not important, but for completeness
        if(param.indexOf(";") >= 0
        || (!param.isEmpty() && param[0] == '\"'))
        {
            // escape the double quotes
            param.replace("\"", "\\\"");
            // quote the resulting parameter and save in result
            result += QString("\"%1\"").arg(param);
        }
        else
        {
            // no special handling necessary
            result += param;
        }
    }

    return result;
}


QString snap_communicator_message::get_name() const
{
    return f_name;
}


void snap_communicator_message::set_name(QString const & name)
{
    f_name = name;
}


QString snap_communicator_message::get_command() const
{
    return f_command;
}


void snap_communicator_message::set_command(QString const & command)
{
    f_command = command;
}


void snap_communicator_message::add_parameter(QString const & name, QString const & value)
{
    verify_parameter_name(name);

    f_parameters[name] = value;
}


bool snap_communicator_message::has_parameter(QString const & name) const
{
    verify_parameter_name(name);

    return f_parameters.contains(name);
}


QString snap_communicator_message::get_parameter(QString const & name)
{
    verify_parameter_name(name);

    if(f_parameters.contains(name))
    {
        return f_parameters[name];
    }

    throw snap_communicator_invalid_message("snap_communicator_message::get_parameter(): parameter not defined, try has_parameter() before calling a get_..._parameter() function.");
}


int64_t snap_communicator_message::get_integer_parameter(QString const & name)
{
    verify_parameter_name(name);

    if(f_parameters.contains(name))
    {
        bool ok;
        int64_t r(f_parameters[name].toLongLong(&ok, 10));
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





///////////////////////////////////
// Snap Connection Implementaion //
///////////////////////////////////


struct snap_communicator::snap_connection::snap_connection_impl
{
    typedef std::unique_ptr<struct event, void (*)(void *)>             unique_event_t;
    typedef std::unique_ptr<struct evconnlistener, void (*)(void *)>    unique_listener_t;

    snap_connection_impl()
        : f_event(nullptr, &delete_event)
        , f_listener(nullptr, &delete_listener)
        //, f_created(false) -- class initialized
        //, f_attached(false) -- class initialized
        //, f_has_timer(false) -- class initialized
    {
    }

    ~snap_connection_impl()
    {
        if(f_attached)
        {
            // this should NEVER happen since connections are saved as
            // shared pointer in a snap_communicator object
            std::cerr << "*** ERROR: snap_connection() \"" << f_name << "\" is being destroyed while still attached to an event_base object.\n";
            std::terminate();
        }
    }

    void set_name(std::string const & name)
    {
        f_name = name;
    }

    void create_event(struct event_base * event_base, snap_connection::pointer_t connection)
    {
        // we cannot be attached more than once
        if(f_created)
        {
            throw snap_communicator_initialization_error("snap_connection_impl::create_event(): connection \"" + f_name + "\" is already attached.");
        }

        if(connection->is_listener())
        {
            // TODO: it should be created disabled, unforunately that
            //       is in libevent 2.1.1+ which is in pure dev. now
            //
            f_listener.reset(evconnlistener_new(
                        event_base,
                        snap_accept_callback,
                        connection.get(),
                        LEV_OPT_LEAVE_SOCKETS_BLOCKING /* | LEV_OPT_DISABLED */,
                        0, // backlog -- leave our settings alone
                        connection->get_socket()
                    ));
            if(!f_listener)
            {
                throw snap_communicator_initialization_error("snap_connection_impl::create_event(): event could not be allocated for \"" + f_name + "\".");
            }

            // no priority?!
        }
        else
        {
            // create the event
            //
            // Note: I am not using event_assign() because according to the
            //       documentation, its support is not forward compatible
            //       (meaning that running against a newer version of the
            //       libevent library may break snap_communicator unless you
            //       recompile everything)
            //
            f_event.reset(event_new(
                        event_base,
                        connection->get_socket(),
                        (what_event_to_what(connection->get_events()) | EV_PERSIST) & ~(EV_TIMEOUT | EV_ET),
                        snap_event_callback,
                        connection.get()
                    ));
            if(!f_event)
            {
                throw snap_communicator_initialization_error("snap_connection_impl::create_event(): event could not be allocated for \"" + f_name + "\".");
            }

            event_priority_set(f_event.get(), connection->get_priority());
        }

        // it worked, we are now created
        // (note that there is no destruction of an event...
        // I am not too sure how they get unallocated!)
        f_created = true;
    }

    void attach_event(int64_t const timeout_us)
    {
        // we must be attached to get destroyed
        if(!f_created)
        {
            throw snap_communicator_initialization_error("snap_connection_impl::attach_event(): connection \"" + f_name + "\" was not yet created.");
        }

        if(f_event)
        {
            if(!f_has_timer && timeout_us < 0)
            {
                // in case there is a timer, remove it
                //
                // TODO: update using this function once available
                //       and remove the f_has_timer flag for the purpose
                //       (it looks like that's not in 2.0.14 yet)
                //
                //event_remove_timer(f_event.get());

                // never timeout
                event_add(f_event.get(), nullptr);
            }
            else
            {
                // TODO: when we remove the f_has_timer, we also will not
                //       need the following special case
                struct timeval tv;
                if(timeout_us < 0)
                {
                    // sleep for at least 50 years (close to 60, but not taking
                    // bissextile years in account...); which should be similar
                    // to removing the timer
                    //
                    tv.tv_sec = 60LL * 365LL * 24LL * 60LL * 60LL;
                    tv.tv_usec = 999999;
                }
                else
                {
                    tv.tv_sec = timeout_us / 1000000;
                    tv.tv_usec = timeout_us % 1000000;
                }

                // WARNING: the libevent library makes use of a timeval
                //          structure, but uses it as an interval
                //
                event_add(f_event.get(), &tv);

                f_has_timer = true;
            }
        }
        else if(f_listener)
        {
            // Once we have version 2.1.1+ this will really make sense
            // since our listener will start in a disabled state
            evconnlistener_enable(f_listener.get());
        }
        else
        {
            throw snap_communicator_parameter_error("snap_connection_impl::attach_event(): connection \"" + f_name + "\" called with no event and no listener.");
        }

        f_attached = true;
    }

    void detach_event()
    {
        // we must be attached to get destroyed
        if(!f_attached)
        {
            throw snap_communicator_initialization_error("snap_connection_impl::detach_event(): connection \"" + f_name + "\" event is not attached.");
        }

        if(f_event)
        {
            // "delete" the event (i.e. this just detaches the event, it does
            // not invalidate it so you can add it right back; to free an
            // event we use the event_free() in the deleter of the f_event
            // unique_ptr field)
            //
            int const r(event_del(f_event.get()));
            if(r != 0)
            {
                throw snap_communicator_initialization_error("snap_connection_impl::detach_event(): connection \"" + f_name + "\" event could not be deleted.");
            }
        }
        else if(f_listener)
        {
            evconnlistener_disable(f_listener.get());
        }
        else
        {
            throw snap_communicator_parameter_error("snap_connection_impl::detach_event(): connection \"" + f_name + "\" called with no event and no listener.");
        }

        // it worked, we are now attached
        f_attached = false;
    }

    unique_event_t          f_event;
    unique_listener_t       f_listener;
    std::string             f_name;
    bool                    f_created = false;
    bool                    f_attached = false;
    bool                    f_has_timer = false; // to palliate from the fact we do not yet have event_remove_timer() support
};


struct snap_communicator::snap_communicator_impl
{
    typedef std::unique_ptr<struct event_base, void (*)(void *)>   unique_event_base_t;

    snap_communicator_impl(priority_t const & priority)
        : f_event_base(nullptr, &delete_event_base)
    {
        typedef std::unique_ptr<struct event_config, void (*)(void *)>   unique_event_config_t;

        // initialize the event_base object
        {
            unique_event_config_t event_cfg(event_config_new(), &delete_event_config);
            if(!event_cfg)
            {
                throw snap_communicator_initialization_error("snap_communicator::snap_communicator(): libevent could not allocate an event_config object");
            }
            if(event_config_set_flag(event_cfg.get(), EVENT_BASE_FLAG_NOLOCK) != 0)
            {
                throw snap_communicator_initialization_error("snap_communicator::snap_communicator(): libevent could not set the event_config object to NOLOCK");
            }
            setup_config(event_cfg.get(), priority);
            f_event_base.reset(event_base_new_with_config(event_cfg.get()));
        }
        if(!f_event_base)
        {
            throw snap_communicator_initialization_error("snap_communicator::snap_communicator(): libevent could not be initialized.");
        }

        // I would imagine that setting this value to 1 (the default) will
        // have no detrimenal side effects
        {
            int const r(event_base_priority_init(f_event_base.get(), priority.get_priorities()));
            if(r != 0)
            {
                throw snap_communicator_initialization_error("snap_communicator::snap_communicator(): libevent did not accept the number of priorities.");
            }
        }
    }

    /** \brief Transform our priority_t object in an event_config priority.
     *
     * This function validates this priority_t object and then converts its
     * parameters to pass them to the specified \p event_cfg object.
     *
     * If the number of priorities defined in the priority_t is still 1
     * (the default) then nothing happens.
     *
     * \param[in,out] event_cfg  The configuration to be updated with
     *                           these priority_t information.
     */
    void setup_config(struct event_config * event_cfg, priority_t const & priority)
    {
        // first make sure our priority_t object is valid
        priority.validate();

        if(priority.get_priorities() == 1)
        {
            // no priorities, use defaults
            return;
        }

#if LIBEVENT_VERSION_NUMBER >= 0x02010100
        // event_config_set_max_dispatch_interval() is not yet available
        // in 2.0.x... so we just ignore this whole piece of code for now
        //
        int64_t const timeout(priority.get_timeout());

        // setup has some valid values
        struct timeval tv;
        struct timeval * tvp(nullptr);
        if(timeout != -1)
        {
            tv.tv_sec  = timeout / 1000000;
            tv.tv_usec = timeout % 1000000;
            tvp = &tv;
        }
        event_config_set_max_dispatch_interval(
                    event_cfg,
                    tvp,
                    priority.get_max_callbacks(),
                    priority.get_min_priority()
                );
#else
        // that means event_cfg does not get used...
        snap::NOTUSED(event_cfg);
#endif
    }

    void reinit()
    {
        int const r(event_reinit(f_event_base.get()));
        if(r != 0)
        {
            throw snap_communicator_initialization_error("snap_communicator::snap_communicator(): libevent could not re-initialize after a fork().");
        }
    }

    unique_event_base_t         f_event_base;
};




//////////////
// Priority //
//////////////


/** \brief The maximum number of priorities offered by libevent.
 *
 * This function returns the maximum number of priorities offered
 * by libevent.
 *
 * \note
 * This number is determine at compile time so it may
 * not always be correct at runtime.
 *
 * \return The maximum number you can pass to set_priorities().
 */
int snap_communicator::priority_t::get_maximum_number_of_priorities()
{
    return EVENT_MAX_PRIORITIES;
}


/** \brief Get the current number of priorities.
 *
 * This function returns the number of priorities that the snap_communicator
 * events will support.
 *
 * By default this number is 1.
 *
 * \return The current number of priorities.
 */
int snap_communicator::priority_t::get_priorities() const
{
    return f_priorities;
}


/** \brief Set the new number of priorities.
 *
 * This function sets the number of priorities that the snap_communicator
 * events will support.
 *
 * By default this number is 1.
 *
 * You want to keep this number as small as possible. If the priorities
 * are allocated dynamically, you may want to use the maximum, but
 * otherwise use as small a number you can such as 3 or even 2.
 *
 * Valid event priorities are between 0 and \p n_priorities - 1.
 *
 * \param[in] n_priorities  The number of priorities you will have available
 *                          for your events.
 */
void snap_communicator::priority_t::set_priorities(int n_priorities)
{
    if(n_priorities <= 0 || n_priorities > EVENT_MAX_PRIORITIES)
    {
        throw snap_communicator_parameter_error("snap_communicator::set_priorities(): n_priority is out of bounds, it must be at least 1 and at most EVENT_MAX_PRIORITIES.");
    }

    f_priorities = n_priorities;
}


/** \brief Time after which events are checked again.
 *
 * This is the amount of time after which your events are checked again.
 *
 * If set to -1 (the default,) then it is ignored.
 *
 * \return The current timeout of this priority object.
 */
int64_t snap_communicator::priority_t::get_timeout() const
{
    return f_timeout;
}


/** \brief Time after which events are checked again.
 *
 * When running the event loop, the libevent will check all the events
 * but only executes those with the smallest priorities (depending on
 * the set_min_priority() parameter.)
 *
 * Events that have a priority equal or larger than the minimum
 * priority will not be executed unless a check for new events
 * returns false and no more events with lower priorities are
 * present in the list of active events.
 *
 * Note with a timeout, some callbacks of events with low priorities
 * may not run either if the previous callbacks took too long to
 * process their event.
 *
 * \note
 * I have not tested, but I would imagine that using a very small
 * time out is probably not a good idea.
 *
 * \param[in] timeout_us  The new timeout in microseconds.
 */
void snap_communicator::priority_t::set_timeout(int64_t timeout_us)
{
    if(timeout_us < 0)
    {
        f_timeout = -1;
    }
    else
    {
        f_timeout = timeout_us;
    }
}


/** \brief Get the maximum number of callbacks in one go.
 *
 * Again, the process will first execute events with the lower priority.
 * If max_callbacks get called, then the process checks for new events
 * before executing more callbacks. This gives events with a lower
 * priority to run ahead of other events for as long as they have work
 * to do.
 *
 * \return The current maximum number of callbacks to execute.
 */
int64_t snap_communicator::priority_t::get_max_callbacks() const
{
    return f_max_callbacks;
}


/** \brief Change the maximum number of callbacks to call in one go.
 *
 * The process of events depends on their priority. Events with
 * lower priorities are executed first, but only up to \p max_callbacks
 * of them get executed. After that, libevent will check for
 * more pending events and reschedule based on the new events
 * it found.
 *
 * \param[in] max_callbacks  The maximum number of callbacks to call
 *                           before check for new events.
 */
void snap_communicator::priority_t::set_max_callbacks(int max_callbacks)
{
    if(max_callbacks < 0)
    {
        max_callbacks = -1;
    }
    else
    {
        f_max_callbacks = max_callbacks;
    }
}


/** \brief Get the threshold of high and low priority events.
 *
 * New events are checked if the next active event has a priority
 * equal or larger than the minimum priority value.
 *
 * \return The minimum priority value.
 */
int64_t snap_communicator::priority_t::get_min_priority() const
{
    return f_min_priority;
}


/** \brief Change the threshold of high and low priority events.
 *
 * Events are grouped in two categories: low priorities which run first
 * and high priorities which are ignored as long as low priority events
 * exist.
 *
 * To avoid the effect of the minimum priority, you may use +1 when
 * setting your total number of priorities and then set the minimum
 * priority to that maximum number minus one. This way all events
 * are considered low priority (assuming you never assign that
 * maximum priority - 1 to any event, of course.)
 *
 * \return The minimum priority value.
 */
void snap_communicator::priority_t::set_min_priority(int min_priority)
{
    if(min_priority < 0 || min_priority >= EVENT_MAX_PRIORITIES)
    {
        throw snap_communicator_parameter_error("snap_communicator::set_min_priority(): min_priority is out of bounds, it must be between 0 and EVENT_MAX_PRIORITIES - 1, also it has to be smaller than f_priorities in the end.");
    }

    f_min_priority = min_priority;
}


/** \brief Validate that the priorities make sense.
 *
 * This may not be useful because the libevent library itself makes
 * such a check. However, it may make it easier to understand what is
 * wrong in your setup since we throw whereas the libevent library
 * just returns -1.
 *
 * This function is called by the setup_config() function just
 * before using the priority information.
 */
void snap_communicator::priority_t::validate() const
{
    if(f_priorities == 1
    && (f_min_priority != 0 || f_timeout >= 0 || f_max_callbacks >= 0))
    {
        throw snap_communicator_parameter_error("snap_communicator::validate(): f_priorities has to be larger than 1 to allow any priority features to work.");
    }

    if(f_min_priority >= f_priorities)
    {
        throw snap_communicator_parameter_error("snap_communicator::validate(): f_min_priority is out of bounds, it must be at least 1 and smaller than f_priorities.");
    }

    if(f_max_callbacks == 0)
    {
        throw snap_communicator_parameter_error("snap_communicator::validate(): f_max_callbacks cannot be set to 0, try with -1 if you do not want to take that parameter in account.");
    }

    if(f_timeout == 0)
    {
        throw snap_communicator_parameter_error("snap_communicator::validate(): f_timeout cannot be set to 0, try with -1 if you do not want to take that parameter in account.");
    }
}







/////////////////////
// Snap Connection //
/////////////////////


/** \brief Initializes the client connection.
 *
 * This function creates a connection using the address, port, and mode
 * parameters. This is very similar to using the bio_client class to
 * create a connection, only the resulting connection can be used with
 * the snap_communicator object.
 *
 * \param[in] addr  The address of the server to connect to.
 * \param[in] port  The port to connect to.
 * \param[in] mode  Type of connection: plain or secure.
 */
snap_communicator::snap_connection::snap_connection()
    : f_impl(new snap_connection_impl)
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
 * The value must between between 0 and EVENT_MAX_PRIORITIES - 1. Any
 * other value raises this exception.
 *
 * \param[in] priority  Priority of the event.
 */
void snap_communicator::snap_connection::set_priority(int priority)
{
    if(priority < 0 || priority >= EVENT_MAX_PRIORITIES)
    {
        std::stringstream ss;
        ss << "snap_communicator::set_priority(): priority out of range, this instance of snap_communicator accepts priorities between 0 and "
           << EVENT_MAX_PRIORITIES
           << ".";
        throw snap_communicator_parameter_error(ss.str());
    }

    f_priority = priority;
}


/** \brief Retrieve the name of the connection.
 *
 * When generating an error or a log the library makes use of this name
 * so we actually know which type of socket generated a problem.
 *
 * \return A constant reference to the connection name.
 */
std::string const & snap_communicator::snap_connection::get_name() const
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
void snap_communicator::snap_connection::set_name(std::string const & name)
{
    f_impl->set_name(name);
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


/** \brief Function used as a factory to create new connections on accept().
 *
 * A server which is a listener needs to accept new connections and thus
 * it needs to create new snap_connection objects. Unfortunately we
 * cannot do that internally because we do not have an internal callback
 * for that new object. Instead we want to leave it to the owner of the
 * server to create its own client objects.
 *
 * This function is like a factory that is expected to create new
 * snap_connection compatible objects. The implementation in the
 * snap_connection base class throws an error.
 *
 * \param[in] socket  The socket we just received from the accept() function.
 *
 * \return A pointer to a snap_connection object.
 */
snap_communicator::snap_connection::pointer_t snap_communicator::snap_connection::create_new_connection(int socket)
{
    NOTUSED(socket);

    // you should never reach this line of code because you should have
    // an implementation of this virtual function in your server class
    //
    throw snap_communicator_parameter_error("snap_communicator::snap_connection::create_new_connection() called, it has to be implemented in your snap_tcp_server_connection class.");
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
 * \return This function always returns -1.
 */
int64_t snap_communicator::snap_connection::get_timeout() const
{
    return f_timeout;
}


/** \brief Change the timeout of this timer.
 *
 * This function attempts to change the timer timeout value.
 *
 * \warning
 * The libevent library does not support removing a timeout.
 * If you want to wait forever, use a very long time out
 * such as one whole year (unless you foresee your systems
 * running for period of times that are even longer, but
 * you see the picture, just use a really large number.)
 *
 * \param[in] timeout_us  The new time out in micro seconds.
 */
void snap_communicator::snap_connection::set_timeout(int64_t timeout_us)
{
    f_timeout = timeout_us;
    if(f_impl->f_created)
    {
        f_impl->attach_event(timeout_us);
    }
}


/** \brief Make this connection socket a non-blocking socket.
 *
 * Many sockets have to be marked as non-blocking in order to work
 * properly with libevent. This function can be called for the purpose
 * of changing the socket of a connection non-blocking.
 *
 * Remember that non-blocking socket may end up returning EAGAIN
 * as an "error" message.
 */
void snap_communicator::snap_connection::non_blocking()
{
    if(get_socket() >= 0)
    {
        evutil_make_socket_nonblocking(get_socket());
    }
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
* \param[in] timeout  The timeout in microseconds.
*/
snap_communicator::snap_timer::snap_timer(int64_t timeout_us)
{
    set_timeout(timeout_us);
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


/** \brief Retrieve the set of events a timer listens to: none.
 *
 * This function returns zero (0) since timers listen to nothing.
 * (It could not since it returns -1 as the socket identifier).
 *
 * \note
 * You should not override this function since there is not other
 * value it can return.
 *
 * \return Always 0.
 */
int snap_communicator::snap_timer::get_events() const
{
    return 0;
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
 * SIGTERM, etc.
 *
 * The libevent also supports POSIX signals so we offer such an
 * object. We have not written any specialized code for this one.
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
 * \param[in] timeout  The timeout in microseconds.
 */
snap_communicator::snap_signal::snap_signal(int posix_signal)
    : f_signal(posix_signal)
{
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


/** \brief Retrieve the set of flags to use with a signal event.
 *
 * This function returns SIGNAL as its set of flags.
 *
 * \note
 * You should not override this function since there is not other
 * value it can return.
 *
 * \return Always EVENT_SIGNAL.
 */
int snap_communicator::snap_signal::get_events() const
{
    return EVENT_SIGNAL;
}








////////////////////////////
// Snap Client Connection //
////////////////////////////


/** \brief Initializes the client connection.
 *
 * This function creates a connection using the address, port, and mode
 * parameters. This is very similar to using the bio_client class to
 * create a connection, only the resulting connection can be used with
 * the snap_communicator object.
 *
 * \param[in] addr  The address of the server to connect to.
 * \param[in] port  The port to connect to.
 * \param[in] mode  Type of connection: plain or secure.
 */
snap_communicator::snap_client_connection::snap_client_connection(std::string const & addr, int port, mode_t mode)
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
int snap_communicator::snap_client_connection::get_socket() const
{
    return bio_client::get_socket();
}


/** \brief Retrieve the set of events a client connection listens to.
 *
 * This function returns a set of events that a client connection socket
 * is expected to listen to.
 *
 * By default this is set to READ and WRITE only.
 *
 * \note
 * Generally you do not need to override this call unless you want
 * to specifically listen to only READ or only WRITE events.
 *
 * \return The events to listen to for this connection.
 */
int snap_communicator::snap_client_connection::get_events() const
{
    return EVENT_READ | EVENT_WRITE;
}






////////////////////////////////
// Snap TCP Server Connection //
////////////////////////////////


/** \brief Initialize a server connection.
 *
 * This function is used to initialize a server connection, a TCP/IP
 * listener which can accept() new connections.
 *
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
 * A server connection is a listener socket. The libevent library makes
 * use of a completely different object (struct evconnlistener) in order
 * to handle listeners. Their callback includes all the information
 * about the new client connection.
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


/** \brief Retrieve the set of events a client connection listens to.
 *
 * This function returns a set of events that a client connection socket
 * is expected to listen to.
 *
 * By default this is set to READ and WRITE only.
 *
 * \note
 * Generally you do not need to override this call unless you want
 * to specifically listen to only READ or only WRITE events.
 *
 * \return The events to listen to for this connection.
 */
int snap_communicator::snap_tcp_server_connection::get_events() const
{
    return EVENT_READ | EVENT_WRITE;
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
    // at this point, we should never get a socket with -1, but just in
    // case that could happen later
    //
    if(f_socket != -1)
    {
        close(f_socket);
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


/** \brief Retrieve the set of events a client connection listens to.
 *
 * This function returns a set of events that a client connection socket
 * is expected to listen to.
 *
 * By default this is set to READ and WRITE only.
 *
 * \note
 * Generally you do not need to override this call unless you want
 * to specifically listen to only READ or only WRITE events.
 *
 * \return The events to listen to for this connection.
 */
int snap_communicator::snap_tcp_server_client_connection::get_events() const
{
    return EVENT_READ | EVENT_WRITE;
}


/** \brief The address of this client connection.
 *
 * This function saves the address of the client connection as received
 * by the libevent callback. This way we avoid having to query it
 * ourselves (although it is probably very much the same.)
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


/** \brief Ask the OS to keep the socket alive.
 *
 * This function marks the socket with the SO_KEEPALIVE flag. This means
 * the OS implementation of the network stack should regularly send
 * small messages over the network to keep the connection alive.
 *
 * The function returns whether the function works or not. If the function
 * fails, it logs a warning and returns.
 */
void snap_communicator::snap_tcp_server_client_connection::keep_alive() const
{
    if(f_socket != -1)
    {
        int optval(1);
        socklen_t const optlen(sizeof(optval));
        if(setsockopt(f_socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) != 0)
        {
            SNAP_LOG_WARNING("snap_communicator::snap_tcp_server_client_connection::keep_alive(): an error occurred trying to mark socket with SO_KEEPALIVE.");
        }
    }
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


/** \brief Retrieve the set of events a client connection listens to.
 *
 * This function returns a set of events that a client connection socket
 * is expected to listen to.
 *
 * By default this is set to READ and WRITE only.
 *
 * \note
 * Generally you do not need to override this call unless you want
 * to specifically listen to only READ or only WRITE events.
 *
 * \return The events to listen to for this connection.
 */
int snap_communicator::snap_udp_server_connection::get_events() const
{
    return EVENT_READ | EVENT_WRITE;
}






///////////////////////
// Snap Communicator //
///////////////////////


/** \brief Initialize the snap communicator.
 *
 * This function initializes the libevent library so it is ready for use.
 * (this is probably mostly to initialize the SOCK library under MS-Windows
 * but the library may also initialize a few other things and discover
 * what interface it will be using.)
 *
 * The priority of various events can be managed using a priority
 * object and changing the defaults. If no priority is setup, then
 * all events have the same priority (i.e. zero.) See the priority_t
 * class for more information.
 *
 * \exception snap_communicator_initialization_error
 * If the library cannot be initialized, then this exception is raised.
 *
 * \param[in] priority  The priority setup for this instance.
 */
snap_communicator::snap_communicator(priority_t const & priority)
    : f_impl(new snap_communicator_impl(priority))
    //, f_connections() -- auto-init
    , f_priority(priority)
{
}


/** \brief Reinitialize the library after a fork().
 *
 * This function makes sure that the libevent library continues to
 * function as expected after a fork().
 *
 * It is up to you to properly make a call to this function.
 *
 * \todo
 * There are currently no safeguards to know that you are calling this
 * function only once and only in the child process.
 */
void snap_communicator::reinit()
{
    f_impl->reinit();
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

    int const priority(connection->get_priority());
    if(priority < 0 || priority >= f_priority.get_priorities())
    {
        std::stringstream ss;
        ss << "snap_communicator::add_connecton(): priority out of range, this instance of snap_communicator accepts priorities between 0 and "
           << f_priority.get_priorities()
           << ".";
        throw snap_communicator_parameter_error(ss.str());
    }

    // create the libevent event, we save it in the connection object
    // which is a friend (argh!)
    //
    // At this time we do not support Edge Triggered events (EV_ET)
    //
    if(!connection->f_impl->f_created)
    {
        connection->f_impl->create_event(f_impl->f_event_base.get(), connection);
    }
    connection->f_impl->attach_event(connection->get_timeout());

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

    connection->f_impl->detach_event();

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
    int const r(event_base_dispatch(f_impl->f_event_base.get()));
    if(r != 0 && r != 1)
    {
        // event loop exited because of something else than an empty set
        // of events in the event_base
        throw snap_communicator_runtime_error("snap_communicator.cpp: an error occurred in the event dispatch loop.");
    }

    return r == 1;
}










} // namespace snap
// vim: ts=4 sw=4 et
