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

// ourselves
//
#include "process.h"

// snapwebsites lib
//
#include <snapwebsites/snap_communicator.h>
#include <snapwebsites/snap_string_list.h>

// Qt lib
//
#include <QDomElement>
#include <QFile>
#include <QString>

// C++ lib
//
#include <functional>
#include <map>
#include <memory>
#include <vector>

// C lib
//
#include <sys/resource.h>


namespace snapinit
{

// passed as the "parent" of each service
class snap_init;



/////////////////////////////////////////////////
// SERVICE (class declaration)                 //
/////////////////////////////////////////////////

class service
        : public snap::snap_communicator::snap_timer
{
public:
    typedef std::shared_ptr<service>        pointer_t;
    typedef std::vector<pointer_t>          vector_t;
    typedef std::weak_ptr<service>          weak_pointer_t;
    typedef std::vector<weak_pointer_t>     weak_vector_t;
    typedef std::map<QString, pointer_t>    map_t;

    static int64_t const        QUICK_RETRY_INTERVAL = 1000000LL;           // 1 second
    static int64_t const        SERVICE_STOP_DELAY = 120 * 1000000LL;       // 2 minutes
    static int64_t const        SERVICE_TERMINATE_DELAY = 30 * 1000000LL;   // 30 seconds
    static int const            DEFAULT_PRIORITY = 50;

                                service( std::shared_ptr<snap_init> si );
                                service() = delete;
                                service(service const & rhs) = delete;
    service &                   operator = (service const & rhs) = delete;


    void                        configure_as_snapinit();
    void                        configure(QDomElement e, QString const & binary_path, std::vector<QString> & common_options, bool const ignore_path_check);
    void                        finish_configuration(std::vector<QString> & common_options);

    // snap::snap_communicator::snap_timer implementation
    virtual void                process_timeout() override;

    bool                        is_cron_task() const;
    bool                        is_snapcommunicator() const;
    bool                        is_snapdbproxy() const;
    bool                        is_dependency_of( QString const & service_name );
    bool                        is_running() const;
    bool                        is_registered() const;
    bool                        is_weak_dependency( QString const & service_name );

    QString const &             get_service_name() const;
    std::string                 get_snapcommunicator_string() const;
    QString const &             get_snapcommunicator_addr() const;
    int                         get_snapcommunicator_port() const;
    std::string                 get_snapdbproxy_string() const;

    process &                       get_process();
    service::weak_vector_t const &  get_depends_list() const;

    void                        action_ready();
    void                        action_godown();
    void                        action_stop();

    void                        process_died();
    void                        process_pause();
    void                        process_status_changed();

    void                        set_service_index(int index);
    int                         get_service_index() const;

    bool                        operator < (service const & rhs) const;

protected:
    pointer_t                   shared_from_this() const;

private:
    // state of the service object
    enum class service_state_t
    {
        SERVICE_STATE_DISABLED,         // disabled (usually not added if disabled)
        SERVICE_STATE_READY,            // ready means that we want to get the process to run
        SERVICE_STATE_PAUSED,           // paused means that we are not running but crashed too many times in a row
        SERVICE_STATE_GOINGDOWN,        // goingdown means that we are trying to stop a pre-requirement which is still running
        SERVICE_STATE_STOPPING          // snapinit received a STOP message or equivalent, try to stop ASAP
    };

    // state of the killing of the process of the service
    enum class stopping_state_t
    {
        STOPPING_STATE_IDLE,            // we are not trying to stop (i.e. we are either running or stopped)
        STOPPING_STATE_STOP,            // request to stop the process
        STOPPING_STATE_TERMINATE,       // request to terminate the process (SIGTERM)
        STOPPING_STATE_KILL             // request to kill the process (SIGKILL)
    };

    struct dependency_t
    {
        typedef std::vector<dependency_t>   vector_t;

        enum class dependency_type_t
        {
            DEPENDENCY_TYPE_STRONG,         // a strong dependency must exist
            DEPENDENCY_TYPE_WEAK            // a weak dependency does not need to exist, if not there, ignore it without errors
        };

                            dependency_t();
                            dependency_t(QString const & service_name, dependency_type_t const type);

        QString             f_service_name;
        dependency_type_t   f_type = dependency_type_t::DEPENDENCY_TYPE_STRONG;
    };

    // state that can only be reached internally
    void                        action_idle();

    void                        process_ready();
    void                        process_stop();                 // select process_stop_...() as expected
    void                        process_stop_timeout();
    void                        process_stop_initiate();        // send STOP if possible
    void                        process_stop_terminate();       // send SIGTERM
    void                        process_stop_kill();            // send SIGKILL
    void                        process_wentdown();
    void                        process_prereqs_down();

    void                        init_prereqs_list();
    void                        init_depends_list();
    void                        start_pause_timer();
    void                        compute_next_tick(bool just_ran);
    std::shared_ptr<snap_init>  snap_init_ptr();

    static char const *         state_to_string( service_state_t const state );

    // parent object
    //
    std::weak_ptr<snap_init>    f_snap_init;

    // current states (we have two in the services!)
    //
    service_state_t             f_service_state = service_state_t::SERVICE_STATE_DISABLED;
    stopping_state_t            f_stopping_state = stopping_state_t::STOPPING_STATE_IDLE;

    // data from XML files (some also goes in the f_process object)
    //
    QString                     f_service_name;
    bool                        f_required = false;
    int                         f_wait_interval = 1;    // in seconds
    int                         f_recovery = 0;         // in seconds
    QString                     f_safe_message;
    int                         f_priority = DEFAULT_PRIORITY;
    QString                     f_snapcommunicator_addr;            // to connect with snapcommunicator
    int                         f_snapcommunicator_port = 4040;     // to connect with snapcommunicator
    QString                     f_snapdbproxy_addr;                 // to connect with snapdbproxy
    int                         f_snapdbproxy_port = 4042;          // to connect with snapdbproxy
    int                         f_cron = 0;                         // if 0, then off (i.e. not a cron task)
    dependency_t::vector_t      f_dep_name_list;

    // computed data
    //
    process                     f_process;
    service::weak_vector_t      f_prereqs_list;         // list of pre-required dependencies (they need us)
    service::weak_vector_t      f_depends_list;         // list of dependencies (we need those)

    int                         f_service_index = -1;  // used to generate the snapinit.dot file
};



} // namespace snapinit
// vim: ts=4 sw=4 et
