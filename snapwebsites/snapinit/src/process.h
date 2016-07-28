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
#include "common.h"

// Qt lib
//
#include <QString>

// C++ lib
//
#include <memory>

// C lib
//
#include <sys/resource.h>

namespace snapinit
{

class snap_init;
class service;


class process
{
public:
                            process(std::shared_ptr<snap_init> si, service * s);
                            process() = delete;
                            process(process const & rhs) = delete;
    process &               operator = (process const & rhs) = delete;

    void                    set_user(QString const & user);
    void                    set_group(QString const & groupd);
    void                    set_coredump_limit(rlim_t coredump_limit);
    void                    set_command(QString const & binary_path, QString const & command, bool const ignore_path_check);
    void                    set_config_filename(QString const & config_filename);
    void                    set_options(QString const & options);
    void                    set_common_options(std::vector<QString> const & options);
    void                    set_safe_message(QString const & safe_message);
    void                    set_nice(int const nice);

    void                    action_start();
    void                    action_died();
    void                    action_process_registered();
    void                    action_process_unregistered();
    void                    action_safe_message(QString const & message);

    bool                    is_running() const;
    bool                    is_registered() const;
    bool                    is_stopped() const;

    pid_t                   get_pid() const;
    QString const &         get_config_filename() const;

    bool                    kill_process(int signum);

private:
    enum class process_state_t
    {
        PROCESS_STATE_STOPPED,
        PROCESS_STATE_UNREGISTERED,
        PROCESS_STATE_REGISTERED,
        PROCESS_STATE_ERROR
    };

    // state that can only be reached internally
    void                        action_error(bool immediate_error);

    bool                        exists() const;
    void                        parse_options(std::vector<std::string> & args, char const * s);
    bool                        start_service_process();
    [[noreturn]] void           exec_child(pid_t parent_pid);
    std::shared_ptr<snap_init>  snap_init_ptr();

    static char const *         state_to_string( process_state_t const state );

    static int64_t const        MAX_START_INTERVAL = 60LL * common::SECONDS_TO_MICROSECONDS; // 1 minute in microseconds
    static int const            MAX_START_COUNT = 5;

    // parents
    //
    std::weak_ptr<snap_init>    f_snap_init;
    service *                   f_service = nullptr; // cannot be smart pointer, initialized from constructor and anyway 100% part of service class as a class variable member there

    // current state
    //
    process_state_t             f_state = process_state_t::PROCESS_STATE_STOPPED;
    int                         f_error_count = 0;

    // information to run the process
    //
    int64_t                     f_start_date = 0;       // in microseconds, to calculate an interval
    int64_t                     f_end_date = 0;         // in microseconds, to calculate an interval
    int                         f_nice = -1;
    pid_t                       f_pid = -1;
    rlim_t                      f_coredump_limit = 0;   // leave shell setup by default
    QString                     f_safe_message;
    QString                     f_user;
    QString                     f_group;
    QString                     f_command;
    QString                     f_full_path;
    QString                     f_config_filename;
    QString                     f_options;
    std::vector<QString>        f_common_options;
};



} // namespace snapinit
// vim: ts=4 sw=4 et
