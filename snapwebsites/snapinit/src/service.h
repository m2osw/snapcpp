/////////////////////////////////////////////////////////////////////////////////
// Snap Init Server -- snap initialization server

// Copyright (C) 2011-2016  Made to Order Software Corp.
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
//
// This server reads in a configuration file and keeps specified services running.
// When signaled, it will terminate those services cleanly.
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/snap_string_list.h>

#include <QDomElement>
#include <QFile>
#include <QString>

#include <sys/resource.h>

#include <functional>
#include <map>
#include <memory>
#include <vector>

// passed as the "parent" of each service
class snap_init;


namespace common
{
    bool is_a_tty();
    void fatal_error(QString msg) __attribute__ ((noreturn));
}


/////////////////////////////////////////////////
// SERVICE (class declaration)                 //
/////////////////////////////////////////////////

class service
        : public snap::snap_communicator::snap_timer
{
public:
    enum class state_t
    {
        not_started,
        cron_running,
        starting,
        starting_without_listener,
        starting_with_listener,
        started,
        running,
        stopping,
        stopped,
        dead,
        failing,
        failed
    };

    typedef std::shared_ptr<service>        pointer_t;
    typedef std::vector<pointer_t>          vector_t;
    typedef std::map<QString, pointer_t>    map_t;

    static int const            MAX_START_COUNT = 5;
    static int64_t const        MAX_START_INTERVAL = 60LL * 1000000LL; // 1 minute in microseconds
    static int const            DEFAULT_PRIORITY = 50;

                                service( std::shared_ptr<snap_init> si );

    void                        configure(QDomElement e, QString const & binary_path, bool const debug, bool const ignore_path_check);

    // snap::snap_communicator::snap_timer implementation
    virtual void                process_timeout() override;

    bool                        exists() const;
    bool                        is_service_required();

    state_t                     current_state() const   { return f_current_state;  }
    state_t                     previous_state() const  { return f_previous_state; }

    void                        set_starting();
    bool                        is_running() const;

    void                        set_stopping();
    bool                        is_stopping() const;
    bool                        has_stopped() const;

    void                        set_failing();
    bool                        is_failing() const;
    bool                        failed() const;

    bool                        is_dead() const;

    bool                        cron_task() const;

    bool                        service_may_have_died() const;
    void                        mark_service_as_dead();

    bool                        is_connection_required() const;
    bool                        is_snapdbproxy() const;
    bool                        is_registered() const;
    void                        set_registered( const bool val );

    std::string                 get_connect_string() const;
    std::string                 get_snapdbproxy_string() const;
    bool                        is_safe_required() const;
    QString const &             get_safe_message() const;
    QString const &             get_config_filename() const;
    QString const &             get_service_name() const;
    pid_t                       get_old_pid() const;
    int                         get_wait_interval() const;
    int                         get_recovery() const;
    int                         get_priority() const;
    bool                        is_dependency_of( const QString& service_name );
    void                        clear_queue();

    bool                        operator < (service const & rhs) const;

    static QString              state_to_string( state_t const state );

protected:
    pointer_t                   shared_from_this() const;

private:
    void                        init_functions();
    void                        push_state( const state_t state );
    bool                        run();
    bool                        has_cron_elapsed();
    void                        compute_next_tick(bool just_ran);
    void                        get_prereqs_list();
    void                        get_depends_list();
    void                        mark_process_as_stopped( const bool from_set_stopping );
    void                        mark_process_as_dead();
    bool                        has_failed();
    bool                        kill_process();

    std::weak_ptr<snap_init>    f_snap_init;
    QString                     f_full_path;
    QString                     f_config_filename;
    QString                     f_service_name;
    QString                     f_command;
    QString                     f_options;
    QString                     f_user;
    QString                     f_group;
    snap::snap_string_list      f_dep_name_list;
    service::vector_t           f_prereqs_list;
    service::vector_t           f_depends_list;
    pid_t                       f_pid = 0;
    pid_t                       f_old_pid = 0;
    int                         f_short_run_count = 0;
    int64_t                     f_start_date = 0;       // in microseconds, to calculate an interval
    int                         f_wait_interval = 0;    // in seconds
    int                         f_recovery = 0;         // in seconds
    QString                     f_safe_message;
    rlim_t                      f_coredump_limit = 0;   // avoid core dump files by default
    bool                        f_started = false;
    bool                        f_failed = false;
    bool                        f_debug = false;
    bool                        f_required = false;
    int                         f_stopping_signal = 0;
    QString                     f_snapcommunicator_addr;            // to connect with snapcommunicator
    int                         f_snapcommunicator_port = 4040;     // to connect with snapcommunicator
    QString                     f_snapdbproxy_addr;                 // to connect with snapdbproxy
    int                         f_snapdbproxy_port = 4042;          // to connect with snapdbproxy
    int                         f_priority = DEFAULT_PRIORITY;
    int                         f_cron = 0;                         // if 0, then off (i.e. not a cron task)
    bool                        f_registered = false;               // set to true when service is registered with snapcomm
    bool                        f_restart_requested = false;
    int64_t                     f_timestamp = 0LL;

    int                         f_timeout_count = 0;

    // For future refactoring. Not yet implemented.
    //
    typedef std::function<void()>                   func_t;
    typedef std::map<state_t,func_t>                func_map_t;
    typedef std::queue<std::pair<state_t,func_t>>   func_queue_t;

    func_map_t                  f_func_map;
    func_queue_t                f_queue;
    state_t                     f_current_state  = state_t::not_started;
    state_t                     f_previous_state = state_t::not_started;
};

// vim: ts=4 sw=4 et
