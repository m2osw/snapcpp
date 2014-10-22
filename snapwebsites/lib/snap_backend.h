// Snap Websites Servers -- snap websites child process hanlding
// Copyright (C) 2011-2014  Made to Order Software Corp.
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

#include "snap_child.h"
#include "snap_thread.h"

namespace snap
{

class snap_backend : public snap_child
{
public:
    typedef controlled_vars::ptr_auto_init<snap_backend>    zpsnap_backend_t;
    typedef std::string                                     message_t;

                snap_backend( server_pointer_t s );
    virtual     ~snap_backend();

    void        create_signal( const std::string& name );

    bool        is_message_pending() const;
    bool        pop_message( message_t& message, int const wait_secs = 1 );
    bool        get_error() const;
    bool        stop_received() const;

    void        run_backend();

private:
    typedef QSharedPointer<udp_client_server::udp_server>   udp_signal_t;

    class udp_monitor
        : public snap_thread::snap_runner
    {
    public:
        typedef controlled_vars::ptr_auto_init<udp_monitor>     zp_t;

        udp_monitor();

        // initialization
        udp_signal_t get_signal() const;
        void set_signal( udp_signal_t signal );
        void set_backend( zpsnap_backend_t backend );

        // current status
        bool get_error() const;
        bool stop_received() const;
        bool is_message_pending() const;
        bool pop_message( message_t& message, int const wait_secs );

        // from snap_thread::snap_runner
        virtual void run();

    private:
        mutable snap_thread::snap_mutex f_mutex;

        zpsnap_backend_t                    f_backend;
        udp_signal_t                        f_udp_signal;
        controlled_vars::flbool_t           f_error;
        controlled_vars::flbool_t           f_stop_received;
        snap_thread::snap_fifo<message_t>   f_message_fifo;
    };

    //mutable snap_thread::snap_mutex f_mutex;

    udp_monitor                 f_monitor;
    snap_thread                 f_thread;

    void                        process_backend_uri(QString const& uri);
    std::string                 get_signal_name_from_action(QString const& action);
};


} // namespace snap
// vim: ts=4 sw=4 et
