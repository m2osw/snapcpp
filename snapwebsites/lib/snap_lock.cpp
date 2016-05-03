// Snap Lock -- class to handle inter-process and inter-computer locks
// Copyright (C) 2016  Made to Order Software Corp.
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

#include "snap_lock.h"

#include "log.h"
#include "not_reached.h"
#include "not_used.h"
#include "qstring_stream.h"

#include <iostream>

#include <unistd.h>
#include <sys/syscall.h>

#include "poison.h"

/** \file
 * \brief Implementation of the Snap Lock class.
 *
 * This class is used to create an inter-process lock within a entire
 * Snap cluster.
 *
 * The class uses a blocking socket to communicate to a snaplock
 * instance and wait for the LOCKED event. Once received, it
 * let you run your code.
 *
 * If instead we receive a LOCKFAILED or UNLOCKED (on a timeout we may
 * get an UNLOCKED event) message as a response, then the class throws
 * since the lock was not obtained.
 */

namespace snap
{

namespace
{


/** \brief The default time to live of a lock.
 *
 * By default the inter-process locks are kept for only five seconds.
 * You may change the default using the snaplock::initialize() function.
 *
 * You can specify how long a lock should be kept around by setting its
 * duration at the time you create it (see the snap_lock constructor.)
 */
int g_timeout = snap_lock::SNAP_LOCK_DEFAULT_TIMEOUT;


/** \brief The default snapcommunicator address.
 *
 * This variable holds the snapcommunicator IP address used to create
 * locks by connecting (Sending messages) to snaplock processes.
 *
 * It can be changed using the snaplock::initialize() function.
 */
std::string g_snapcommunicator_address = "127.0.0.1";


/** \brief The default snapcommunicator port.
 *
 * This variable holds the snapcommunicator port used to create
 * locks by connecting (Sending messages) to snaplock processes.
 *
 * It can be changed using the snaplock::initialize() function.
 */
int g_snapcommunicator_port = 4040;


/** \brief The default snapcommunicator mode.
 *
 * This variable holds the snapcommunicator mode used to create
 * locks by connecting (Sending messages) to snaplock processes.
 *
 * It can be changed using the snaplock::initialize() function.
 */
tcp_client_server::bio_client::mode_t g_snapcommunicator_mode = tcp_client_server::bio_client::mode_t::MODE_PLAIN;


/** \brief A unique number used to name the lock service.
 *
 * Each time we create a new lock service we need to have a new unique name
 * because otherwise we could receive replies from a previous lock and
 * there is not other way for us to distinguish them. This is important
 * only if the user is trying to lock the same object several times in
 * a row.
 */
int g_unique_number = 0;


/** \brief Retrieve the current thread identifier.
 *
 * This function retrieves the current thread identifier.
 *
 * \return The thread identifier, which is a pid_t specific to each thread
 *         of a process.
 */
pid_t gettid()
{
    return syscall(SYS_gettid);
}

}
// no name namespace



/** \brief The actual implementation of snap_lock.
 *
 * This class is the actual implementation of snap_lock which is completely
 * hidden from the users of snap_lock. (i.e. implementation details.)
 */
class lock_connection
        : public snap_communicator::snap_tcp_blocking_client_message_connection
{
public:
                        lock_connection(QString const & object_name, int timeout);
                        ~lock_connection();

    void                unlock();
    time_t              get_timeout_date() const;

    void                process_message(snap_communicator_message const & message);

private:
    QString const       f_service_name;
    QString const       f_object_name;
    int const           f_timeout_date;
    bool                f_locked = false;
};





/** \brief Initiate a LOCK.
 *
 * This constructor is used to obtain an inter-process lock.
 *
 * The lock will be effective on all the computers that have access to
 * the running snaplock instances you can connect to via snapcommunicator.
 *
 * The LOCK and other messages are sent to the snaplock daemon using
 * snapcommunicator.
 *
 * Note that the constructor creates a "lock service" which is given a
 * name which is "lock" and the current thread identifier and a unique
 * number. We use an additional unique number to make sure messages
 * sent to our old instance do not make it to a newer instance, which
 * could be confusing and break the lock mechanism in case the user
 * was trying to get a lock multiple times on the same object.
 *
 * \param[in] object_name  The name of the object being locked.
 * \param[in] timeout  The total number of seconds this specific lock will
 *                     last for or -1 to use the default.
 */
lock_connection::lock_connection(QString const & object_name, int timeout)
    : snap_tcp_blocking_client_message_connection(g_snapcommunicator_address, g_snapcommunicator_port, g_snapcommunicator_mode)
    , f_service_name(QString("lock_%1_%2").arg(gettid()).arg(++g_unique_number))
    , f_object_name(object_name)
    , f_timeout_date((timeout == -1 ? g_timeout : timeout) + time(nullptr))
{
    // tell the lower level when the lock will time out
    // that one is in microseconds
    //
    set_timeout_date(f_timeout_date * 1000000L);

    // need to register with snap communicator
    //
    snap::snap_communicator_message register_message;
    register_message.set_command("REGISTER");
    register_message.add_parameter("service", f_service_name);
    register_message.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_message);

    // now wait for the READY and HELP replies
    //
    run();
}


/** \brief Send the UNLOCK message to snaplock to terminate the lock.
 *
 * The destructor makes sure that the lock is released.
 */
lock_connection::~lock_connection()
{
    unlock();
}


/** \brief Send the UNLOCK early (before the destructor is called).
 *
 * This function releases the lock obtained by the constructor.
 *
 * It is safe to call the function multiple times. It will send the
 * UNLOCK only the first time.
 *
 * Note that it is not possible to re-obtain the lock once unlocked.
 * You will have to create a new snap_lock object to do so.
 */
void lock_connection::unlock()
{
    if(f_locked)
    {
        f_locked = false;

        // done
        //
        // explicitly send the UNLOCK message and then make sure to unregister
        // from snapcommunicator; note that we do not wait for a reply to the
        // UNLOCK message, since to us it does not matter much as long as the
        // message was sent...
        //
        snap_communicator_message unlock_message;
        unlock_message.set_command("UNLOCK");
        unlock_message.set_service("snaplock");
        unlock_message.add_parameter("object_name", f_object_name);
        unlock_message.add_parameter("pid", gettid());
        send_message(unlock_message);

        snap_communicator_message unregister_message;
        unregister_message.set_command("UNREGISTER");
        unregister_message.add_parameter("service", f_service_name);
        send_message(unregister_message);
    }
}


/** \brief Retrieve the cutoff time for this lock.
 *
 * This lock will time out when this date is reached.
 */
time_t lock_connection::get_timeout_date() const
{
    return f_timeout_date;
}


/** \brief Process messages as we receive them.
 *
 * This function is called whenever a complete message is read from
 * the snapcommunicator.
 *
 * In a perfect world, the following shows what happens message wise
 * as far as the client is concerned. The snaplock sends many more
 * messages in order to obtain the lock (See src/snaplock/snaplock_ticket.cpp
 * for details about those events.)
 *
 * \note
 * The REGISTER message is sent from the constructor to initiate the
 * process. This function starts by receiving the READY message.
 *
 * \msc
 *    client,snapcommunicator,snaplock;
 *
 *    client->snapcommunicator [label="REGISTER"];
 *    snapcommunicator->client [label="READY"];
 *    snapcommunicator->client [label="HELP"];
 *    client->snapcommunicator [label="COMMANDS"];
 *    client->snapcommunicator [label="LOCK"];
 *    snapcommunicator->snaplock [label="LOCK"];
 *    snaplock->snapcommunicator [label="LOCKED"];
 *    snapcommunicator->client [label="LOCKED"];
 *    ...;
 *    client->snapcommunicator [label="UNLOCK"];
 *    snapcommunicator->snaplock [label="UNLOCK"];
 *    client->snapcommunicator [label="UNREGISTER"];
 * \endmsc
 *
 * If somehow the lock fails, we may also receive LOCKFAILED or UNLOCKED.
 *
 * \param[in] message  The message we just received.
 */
void lock_connection::process_message(snap_communicator_message const & message)
{
    QString const command(message.get_command());

std::cerr << "lock connection received: [" << message.to_message() << "]\n";
    switch(command[0].unicode())
    {
    case 'H':
        if(command == "HELP")
        {
            // snapcommunicator wants us to tell it what commands
            // we accept
            snap_communicator_message commands_message;
            commands_message.set_command("COMMANDS");
            commands_message.add_parameter("list", "HELP,LOCKED,LOCKFAILED,QUITTING,READY,STOP,UNKNOWN,UNLOCKED");
            send_message(commands_message);

            // no reply expected from the COMMANDS message,
            // so send the LOCK now which really initiate the locking
            snap_communicator_message lock_message;
            lock_message.set_command("LOCK");
            lock_message.set_service("snaplock");
            lock_message.add_parameter("object_name", f_object_name);
            lock_message.add_parameter("pid", gettid());
            lock_message.add_parameter("timeout", f_timeout_date);
            send_message(lock_message);

            return;
        }
        break;

    case 'L':
        if(command == "LOCKED")
        {
            // the lock worked
            f_locked = message.get_parameter("object_name") == f_object_name;
            if(!f_locked)
            {
                // somehow we received the LOCKED message with the wrong object name
                //
                throw snap_lock_failed_exception(QString("received lock confirmation for object \"%1\" instead of \"%2\" (LOCKED).")
                                .arg(message.get_parameter("object_name"))
                                .arg(f_object_name));
            }

            // release hand back to the user while lock is still active
            //
            done();
            return;
        }
        else if(command == "LOCKFAILED")
        {
            if(message.get_parameter("object_name") == f_object_name)
            {
                throw snap_lock_failed_exception(QString("lock for object \"%1\" failed (LOCKEDFAILED).")
                                .arg(message.get_parameter("object_name")));
            }
            throw snap_lock_failed_exception(QString("object \"%1\" just for a lock failure reported and we received its message while trying to lock \"%2\" (LOCKEDFAILED).")
                            .arg(message.get_parameter("object_name"))
                            .arg(f_object_name));
        }
        break;

    case 'Q':
        if(command == "QUITTING")
        {
            SNAP_LOG_FATAL("we received the QUITTING command.");
            throw snap_lock_failed_exception(QString("lock object \"%1\" received the QUITTING command, so the lock failed.")
                            .arg(f_object_name));
        }
        break;

    case 'R':
        if(command == "READY")
        {
            // the REGISTER worked, wait for the HELP message
            return;
        }
        break;

    case 'S':
        if(command == "STOP")
        {
            SNAP_LOG_FATAL("we received the STOP command.");
            throw snap_lock_failed_exception(QString("lock object \"%1\" received the STOP command, so the lock failed.")
                            .arg(f_object_name));
        }
        break;

    case 'U':
        if(command == "UNKNOWN")
        {
            // we sent a command that Snap! Communicator did not understand
            //
            SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
            return;
        }
        else if(command == "UNLOCKED")
        {
            // we should not receive the UNLOCKED before the LOCKED
            // and thus this should never be accessed; however, if
            // the LOCKED message got lost, we could very well receive
            // this one instead
            //
            if(message.get_parameter("object_name") == f_object_name)
            {
                throw snap_lock_failed_exception(QString("lock for object \"%1\" failed (UNLOCKED).")
                                .arg(message.get_parameter("object_name")));
            }
            throw snap_lock_failed_exception(QString("object \"%1\" just got unlocked and we received its message while trying to lock \"%2\" (UNLOCKED).")
                                .arg(message.get_parameter("object_name"))
                                .arg(f_object_name));
        }
        break;

    }

    // unknown command is reported and process goes on
    //
    {
        SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received by snap_lock on the connection with Snap! Communicator.");

        snap::snap_communicator_message unknown_message;
        unknown_message.set_command("UNKNOWN");
        unknown_message.add_parameter("command", command);
        send_message(unknown_message);
    }
}








/** \brief Create an inter-process lock.
 *
 * This constructor blocks until you obtained an inter-process lock
 * named \p object_name.
 *
 * \note
 * The timeout of this function is a "Time To Live" in seconds. So if you
 * want to keep your lock for 1 hour, use 3600.
 *
 * \param[in] object_name  The name of the lock.
 * \param[in] timeout  The total number of seconds this specific lock will
 *                     last for or -1 to use the default.
 *
 * \sa snap_lock::initialize()
 */
snap_lock::snap_lock(QString const & object_name, int timeout)
    : f_lock_connection(std::make_shared<lock_connection>(object_name, timeout))
{
}


/** \brief Set the default timeout of the inter-process lock.
 *
 * This function let you set the timeout to "Time To Live" in seconds.
 * So if you want to keep your locks for 1 hour, use 3600.
 *
 * \warning
 * This function is not thread safe.
 *
 * \param[in] timeout  The total number of seconds this specific lock will last for.
 */
void snap_lock::initialize_timeout(int timeout)
{
    g_timeout = timeout;
}


/** \brief Initialize the snapcommunicator details.
 *
 * This function must be called before any lock can be obtained in order
 * to define the address and port to use to connect to the snapcommunicator
 * process.
 *
 * \warning
 * This function is not thread safe.
 *
 * \param[in] addr  The address of snapcommunicator.
 * \param[in] port  The port used to connect to snapcommunicator.
 * \param[in] mode  The mode used to open the connection (i.e. plain or secure.)
 */
void snap_lock::initialize_snapcommunicator(std::string const & addr, int port, tcp_client_server::bio_client::mode_t mode)
{
    g_snapcommunicator_address = addr;
    g_snapcommunicator_port = port;
    g_snapcommunicator_mode = mode;
}


/** \brief Release the inter-process lock early.
 *
 * This function releases this inter-process lock early.
 */
void snap_lock::unlock()
{
    f_lock_connection->unlock();
}


/** \brief Get the exact time when the lock times out.
 *
 * This function can be used to check when the lock will be considerd
 * out of date and thus when you should stop doing whatever requires
 * said lock.
 *
 * The time is in second and you can compare it against time(nullptr) to
 * know whether it timed out already or how long you still have:
 *
 * \code
 *      int64_t const diff(lock.get_timeout_date() - time(nullptr));
 *      if(diff <= 0)
 *      {
 *          // locked already timed out
 *          ...
 *      }
 *      else
 *      {
 *          // you have another 'diff' seconds to work on your stuff
 *          ...
 *      }
 * \endcode
 *
 * Remember that this exact date was sent to the snaplock system but you may
 * have a clock with a one second or so difference between various computers
 * so if the amount is really small (1 or 2) you should probably already
 * considered that the locked has timed out.
 */
time_t snap_lock::get_timeout_date() const
{
    return f_lock_connection->get_timeout_date();
}


} // namespace snap
// vim: ts=4 sw=4 et
