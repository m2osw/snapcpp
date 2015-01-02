// Snap Websites Server -- advanced handling of Unix thread
// Copyright (C) 2013-2015  Made to Order Software Corp.
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
#pragma once

#include "snap_exception.h"

#include "not_reached.h"

#include <controlled_vars/controlled_vars_auto_init.h>
#include <controlled_vars/controlled_vars_limited_auto_init.h>
#include <controlled_vars/controlled_vars_limited_auto_enum_init.h>
#include <controlled_vars/controlled_vars_need_init.h>
#include <controlled_vars/controlled_vars_ptr_auto_init.h>
#include <controlled_vars/controlled_vars_ptr_need_init.h>

#include <sys/time.h>

#include <deque>


namespace snap
{

class snap_thread_exception : public snap_exception
{
public:
    snap_thread_exception(const char *whatmsg) : snap_exception("snap_thread", whatmsg) {}
    snap_thread_exception(const std::string& whatmsg) : snap_exception("snap_thread", whatmsg) {}
    snap_thread_exception(const QString& whatmsg) : snap_exception("snap_thread", whatmsg) {}
};

class snap_thread_exception_in_use_error : public snap_thread_exception
{
public:
    snap_thread_exception_in_use_error(const char *whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_in_use_error(const std::string& whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_in_use_error(const QString& whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_not_locked_error : public snap_thread_exception
{
public:
    snap_thread_exception_not_locked_error(const char *whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_not_locked_error(const std::string& whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_not_locked_error(const QString& whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_not_locked_once_error : public snap_thread_exception
{
public:
    snap_thread_exception_not_locked_once_error(const char *whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_not_locked_once_error(const std::string& whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_not_locked_once_error(const QString& whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_mutex_failed_error : public snap_thread_exception
{
public:
    snap_thread_exception_mutex_failed_error(const char *whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_mutex_failed_error(const std::string& whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_mutex_failed_error(const QString& whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_invalid_error : public snap_thread_exception
{
public:
    snap_thread_exception_invalid_error(const char *whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_invalid_error(const std::string& whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_invalid_error(const QString& whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_system_error : public snap_thread_exception
{
public:
    snap_thread_exception_system_error(const char *whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_system_error(const std::string& whatmsg) : snap_thread_exception(whatmsg) {}
    snap_thread_exception_system_error(const QString& whatmsg) : snap_thread_exception(whatmsg) {}
};


class snap_thread
{
public:
    typedef controlled_vars::ptr_auto_init<snap_thread> zpthread_t;
    class snap_condition; // forward declaration

    // a mutex to ensure single threaded work
    class snap_mutex
    {
    public:
                            snap_mutex();
                            ~snap_mutex();

        void                lock();
        bool                try_lock();
        void                unlock();
        void                wait();
        bool                timed_wait(uint64_t usec);
        bool                dated_wait(uint64_t msec);
        void                signal();

    private:
        friend class snap_condition;

        controlled_vars::zuint32_t      f_reference_count;
        pthread_mutex_t                 f_mutex;
        pthread_cond_t                  f_condition;
    };
    typedef controlled_vars::ptr_auto_init<snap_mutex> zpsnap_mutex_t;

    class snap_lock
    {
    public:
                            snap_lock(snap_mutex& mutex);
                            ~snap_lock();

        void                unlock();

    private:
        zpsnap_mutex_t      f_mutex;
    };

    // this is the actual thread because we cannot use the main thread
    // object destructor to properly kill a thread in a C++ environment
    class snap_runner
    {
    public:
                            snap_runner(const QString& name);
        virtual             ~snap_runner();

        virtual bool        is_ready() const;
        virtual bool        continue_running() const;
        virtual void        run() = 0;
        snap_thread *       get_thread() const;

    protected:
        mutable snap_mutex  f_mutex;

    private:
        friend class snap_thread;
        zpthread_t          f_thread;
        const QString       f_name;
    };
    typedef controlled_vars::ptr_auto_init<snap_runner> zprunner_t;

    class snap_condition
    {
    public:
        /** \brief Initializes a pthread condition.
         *
         * This function initializes a condition that can be used to
         * synchronize different threads and allowing one to wait on
         * another.
         */
        snap_condition()
        {
            pthread_cond_init(&f_condition, nullptr);
        }

        /** \brief Destroys the FIFO.
         *
         * This function destroys the FIFO and makes sure that the condition
         * is destroyed.
         *
         * \note
         * If the condition is still in use when the snap_condition gets
         * destroyed, then the results are undefined. (We are not expected
         * to throw in a destructor so if the condition is still busy,
         * there is nothing we can do here.)
         */
        ~snap_condition()
        {
            pthread_cond_destroy(&f_condition);
        }

        /** \brief Wait for some incoming data.
         *
         * This function waits for incoming data for as long as \p secs
         * seconds.
         *
         * The \p secs parameter can be set to variable values as follow:
         *
         * \li -2 or less -- do nothing
         * \li -1 -- wait until data arrives
         * \li 0 -- do not wait at all
         * \li +1 or more -- wait up to that many seconds
         *
         * \param[in] msecs  The number of milliseconds to wait for incoming data.
         */
        void wait(int msecs)
        {
            if(msecs == -1)
            {
                pthread_cond_wait(&f_condition, &f_mutex.f_mutex);
            }
            else if(msecs > 0)
            {
                struct timeval tod;
                gettimeofday(&tod, nullptr);
                struct timespec ts;
                ts.tv_sec = tod.tv_sec + msecs / 1000;
                ts.tv_nsec = tod.tv_usec * 1000 + (msecs % 1000) * 1000000L;
                ts.tv_sec += ts.tv_nsec / 1000000000L;
                ts.tv_nsec = ts.tv_nsec % 1000000000L;

                // TODO: test the return value; if not zero or ETIMEOUT
                //       we may want to react...
                pthread_cond_timedwait(&f_condition, &f_mutex.f_mutex, &ts);
            }
        }

        /** \brief Wake up waiting threads.
         *
         * This function signals the threads currently waiting on
         * this condition.
         *
         * If the \p broadcast parameter is set to true (The default)
         * then all the listening waiting threads get the signal,
         * otherwise only one of the threads receives the signal.
         *
         * \note
         * Although this function does not require the mutex
         */
        void signal(bool const broadcast = true)
        {
            if(broadcast)
            {
                pthread_cond_broadcast(&f_condition);
            }
            else
            {
                pthread_cond_signal(&f_condition);
            }
        }

        /** \brief Retrieve the condition mutex.
         *
         * A condition requires a mutex and only one mutex should be
         * used with one condition so we placed both together in this
         * object. However, you may want to lock the mutex once in
         * a while for your own purpose, and you MUST lock the mutex
         * whenever you want to call the wait function (before the
         * call.)
         *
         * \code
         *      snap_thread::snap_condition condition;
         *
         *      ...snip...
         *      {
         *          snap_lock lock(connection.get_mutex());
         *          // test something
         *          condition.wait();
         *      }
         *      ...snip...
         * \endcode
         *
         * \return A reference to the mutex handled by this condition.
         */
        snap_mutex& get_mutex() const
        {
            return f_mutex;
        }

    private:
        pthread_cond_t      f_condition;
        mutable snap_mutex  f_mutex;
    };

    template<class T>
    class snap_fifo
    {
    public:
        typedef std::deque<T>       items_t;

        /** \brief Push data on this FIFO.
         *
         * This function appends data on the FIFO queue. The function
         * has the side effect to wake up another thread if such is
         * currently waiting for data on the same FIFO.
         *
         * \param[in] v  The value to be pushed on the FIFO queue.
         */
        void push_back(T v)
        {
            snap_lock lock(f_condition.get_mutex());
            f_stack.push_back(v);
            f_condition.signal();
        }

        /** \brief Retrieve one value from the FIFO.
         *
         * This function retrieves one value from the thread FIFO.
         * If necessary, the function can wait for a value to be
         * received. The wait works as defined in the semaphore
         * wait() function:
         *
         * \li -1 -- wait forever (use with caution as this prevents
         *           the STOP event from working.)
         * \li 0 -- do not wait if there is no data, return immediately
         * \li +1 and more -- wait that many seconds
         *
         * If the function works (returns true,) then \p v is set
         * to the value being popped. Otherwise v is not modified
         * and the function returns false.
         *
         * \note
         * Because of the way the pthread conditions are implemented
         * it is possible that the condition was already raised
         * when you call this function. This means the wait, even if
         * you used a value of -1 or 1 or more, will not happen.
         *
         * \param[out] v  The value read.
         * \param[in] msecs  The number of milliseconds to wait.
         *
         * \return true if a value was popped, false otherwise.
         */
        bool pop_front(T& v, int const msecs)
        {
            snap_lock lock(f_condition.get_mutex());
            if(f_stack.empty())
            {
                // empty... wait a bit if possible and try
                // again
                f_condition.wait(msecs);
                if(f_stack.empty())
                {
                    return false;
                }
            }
            v = f_stack.front();
            f_stack.pop_front();
            return true;
        }


        /** \brief Test whether the FIFO is empty.
         *
         * This function checks whether the FIFO is empty and if so
         * returns true, otherwise it returns false.
         *
         * The function does not check the semaphore. Instead it
         * checks the size of the FIFO itself.
         *
         * \return true if the FIFO is empty.
         */
        bool empty() const
        {
            snap_lock lock(f_condition.get_mutex());
            return f_stack.empty();
        }

    private:
        snap_condition      f_condition;
        items_t             f_stack;
    };

    class snap_thread_life
    {
    public:
        snap_thread_life( snap_thread* thread )
            : f_thread(thread)
        {
            if(!f_thread)
            {
                throw snap_logic_exception("snap_thread_life pointer is nullptr");
            }
            f_thread->start();
        }

        ~snap_thread_life()
        {
            //SNAP_LOG_TRACE() << "stopping snap_thread_life...";
            f_thread->stop();
            //SNAP_LOG_TRACE() << "snap_thread_life stopped!";
        }

    private:
        snap_thread* f_thread;
    };

                                snap_thread(const QString& name, snap_runner *runner);
                                ~snap_thread();

    const QString&              get_name() const;
    bool                        is_running() const;
    bool                        is_stopping() const;
    bool                        start();
    void                        stop();

private:
    // prevent copies
                                snap_thread(const snap_thread& rhs);
                                snap_thread& operator = (const snap_thread& rhs);

    // internal function to start the runner
    friend void *               func_internal_start(void *thread);
    void                        internal_run();

    const QString               f_name;
    snap_runner *               f_runner;
    mutable snap_mutex          f_mutex;
    controlled_vars::flbool_t   f_running;
    controlled_vars::flbool_t   f_started;
    controlled_vars::flbool_t   f_stopping;
    pthread_t                   f_thread;
    pthread_attr_t              f_thread_attr;
    std::exception_ptr          f_exception;
};

} // namespace snap
// vim: ts=4 sw=4 et
