// Snap Websites Server -- advanced handling of Unix thread
// Copyright (C) 2013  Made to Order Software Corp.
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
#ifndef SNAP_THREAD_H
#define SNAP_THREAD_H

#include "snap_exception.h"
#include <QString>
#include <controlled_vars/controlled_vars_auto_init.h>
#include <controlled_vars/controlled_vars_limited_auto_init.h>
#include <controlled_vars/controlled_vars_need_init.h>
#include <controlled_vars/controlled_vars_ptr_auto_init.h>
#include <controlled_vars/controlled_vars_ptr_need_init.h>

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
    controlled_vars::fbool_t    f_running;
    controlled_vars::fbool_t    f_started;
    controlled_vars::fbool_t    f_stopping;
    pthread_t                   f_thread;
    pthread_attr_t              f_thread_attr;
    std::exception_ptr          f_exception;
};

} // namespace snap
#endif
// SNAP_THREAD_H
// vim: ts=4 sw=4 et
