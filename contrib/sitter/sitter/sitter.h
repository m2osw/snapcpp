// Copyright (c) 2013-2022  Made to Order Software Corp.  All Rights Reserved.
//
// https://snapwebsites.org/project/sitter
// contact@m2osw.com
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once

// eventdispatcher
//
#include "eventdispatcher/dispatcher.h"



namespace sitter
{

namespace watchdog
{
enum class name_t
{
    SNAP_NAME_WATCHDOG_ADMINISTRATOR_EMAIL,
    SNAP_NAME_WATCHDOG_CACHE_PATH,
    SNAP_NAME_WATCHDOG_DATA_PATH,
    SNAP_NAME_WATCHDOG_DEFAULT_LOG_PATH,
    SNAP_NAME_WATCHDOG_ERROR_REPORT_CRITICAL_PRIORITY,
    SNAP_NAME_WATCHDOG_ERROR_REPORT_LOW_PRIORITY,
    SNAP_NAME_WATCHDOG_ERROR_REPORT_MEDIUM_PRIORITY,
    SNAP_NAME_WATCHDOG_ERROR_REPORT_SETTLE_TIME,
    SNAP_NAME_WATCHDOG_FROM_EMAIL,
    SNAP_NAME_WATCHDOG_LOG_DEFINITIONS_PATH,
    SNAP_NAME_WATCHDOG_LOG_PATH,
    SNAP_NAME_WATCHDOG_SERVER_NAME,
    SNAP_NAME_WATCHDOG_SERVERSTATS,
    SNAP_NAME_WATCHDOG_STATISTICS_FREQUENCY,
    SNAP_NAME_WATCHDOG_STATISTICS_PERIOD,
    SNAP_NAME_WATCHDOG_STATISTICS_TTL,
    SNAP_NAME_WATCHDOG_USER_GROUP
};
char const * get_name(name_t name) __attribute__ ((const));
} // watchdog namespace


DECLARE_MAIN_EXCEPTION(snapwatchdog_exception);

DECLARE_EXCEPTION(snapwatchdog_exception, snapwatchdog_exception_invalid_parameters);



class watchdog_child;



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class watchdog_server
    : public server
    , public ed::connection_with_send_message
    , public ed::dispatcher<watchdog_server>
    , public std::enable_shared_from_this<watchdog_server>
{
public:
    typedef std::shared_ptr<watchdog_server>         pointer_t;

                        watchdog_server();

    static pointer_t    instance();
    void                watchdog();

    time_t              get_server_start_date() const;
    virtual void        show_version();
    int64_t             get_statistics_period() const { return f_statistics_period; }
    int64_t             get_statistics_ttl() const { return f_statistics_ttl; }
    void                ready(ed::message & message);
    void                stop(bool quitting);
    void                set_snapcommunicator_connected(bool status);
    void                set_snapcommunicator_disconnected(bool status);
    bool                get_snapcommunicator_is_connected() const;
    int64_t             get_snapcommunicator_connected_on() const;
    int64_t             get_snapcommunicator_disconnected_on() const;

    SNAP_SIGNAL_WITH_MODE(process_watch, (QDomDocument doc), (doc), NEITHER);

    // connection_with_send_message overloads
    //
    virtual bool        send_message(ed::message const & message, bool cache = false) override;

    // internal functions (these are NOT virtual)
    // 
    void                process_tick();
    void                process_sigchld();

    void                msg_nocassandra(ed::message & message);
    void                msg_cassandraready(ed::message & message);
    void                msg_rusage(ed::message & message);
    void                msg_reload_config(ed::message & message);

    int64_t             get_error_report_settle_time() const;
    int64_t             get_error_report_low_priority() const;
    int64_t             get_error_report_low_span() const;
    int64_t             get_error_report_medium_priority() const;
    int64_t             get_error_report_medium_span() const;
    int64_t             get_error_report_critical_priority() const;
    int64_t             get_error_report_critical_span() const;

private:
    void                define_server_name();
    void                init_parameters();
    void                run_watchdog_process();

    time_t const        f_server_start_date;
    int64_t             f_statistics_frequency = 0;
    int64_t             f_statistics_period = 0;
    int64_t             f_statistics_ttl = 0;
    int64_t             f_error_report_settle_time = 5 * 60;
    int64_t             f_error_report_low_priority = 10;
    int64_t             f_error_report_low_span = 86400 * 7;
    int64_t             f_error_report_medium_priority = 50;
    int64_t             f_error_report_medium_span = 86400 * 3;
    int64_t             f_error_report_critical_priority = 90;
    int64_t             f_error_report_critical_span = 86400 * 1;
    std::vector<std::shared_ptr<watchdog_child>>
                        f_processes = std::vector<std::shared_ptr<watchdog_child>>();
    bool                f_stopping = false;
    bool                f_force_restart = false;
    int64_t             f_snapcommunicator_connected = 0;
    int64_t             f_snapcommunicator_disconnected = 0;
};
#pragma GCC diagnostic pop


class watchdog_child
    : public snap_child
{
public:
    typedef std::shared_ptr<watchdog_child>     pointer_t;

                        watchdog_child(server_pointer_t s, bool tick);
    virtual             ~watchdog_child() override;

    bool                is_tick() const;
    bool                run_watchdog_plugins();
    bool                record_usage(ed::message const & message);
    virtual void        exit(int code) override;

    pid_t               get_child_pid() const;
    void                append_error(QDomDocument doc, QString const & plugin_name, QString const & message, int priority = 50);
    void                append_error(QDomDocument doc, QString const & plugin_name, std::string const & message, int priority = 50);
    void                append_error(QDomDocument doc, QString const & plugin_name, char const * message, int priority = 50);

    watchdog_server::pointer_t  get_server();
    QString             get_cache_path(QString const & filename);

private:
    pid_t               f_child_pid = -1;
    bool const          f_tick = true;
    bool                f_has_cassandra = false;
    std::string         f_cache_path = std::string();
};


} // namespace sitter
// vim: ts=4 sw=4 et
