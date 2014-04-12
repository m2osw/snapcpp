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

    snap_backend( server_pointer_t s );
    ~snap_backend();

    void        create_signal( const std::string& name );

    bool        is_message_pending() const;
    bool        pop_message( std::string& message, const bool wait_secs = 1 );
    void        push_message( const std::string& msg );
    bool        get_error() const;

    void        run_backend();

private:
    typedef QSharedPointer<udp_client_server::udp_server> udp_signal_t;

    class udp_monitor
        : public snap_thread::snap_runner
    {
    public:
        typedef controlled_vars::ptr_auto_init<udp_monitor>    zp_t;
        udp_monitor();

        void set_signal( udp_signal_t signal );
        void set_backend( zpsnap_backend_t backend );
        bool get_error() const;

        virtual void run();

    private:
        typedef controlled_vars::auto_init<bool,false> zp_bool_t;

        mutable snap_thread::snap_mutex f_mutex;

        zpsnap_backend_t        f_backend;
        std::string             f_signal_name;
        udp_signal_t            f_udp_signal;
        zp_bool_t               f_error;
    };

    mutable snap_thread::snap_mutex f_mutex;

    typedef std::list<std::string> message_list_t;
    message_list_t          f_message_list;
    udp_monitor             f_monitor;
    snap_thread             f_thread;

    void   process_backend_uri(QString const& uri);
};

} // namespace snap
// vim: ts=4 sw=4 et
