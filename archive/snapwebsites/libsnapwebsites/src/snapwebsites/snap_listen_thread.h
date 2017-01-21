// Snap Websites Server -- listen for a UDP signal (still used? we have snap_communicator now...)
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

#pragma once

#include "snapwebsites/snap_thread.h"
#include "snapwebsites/udp_client_server.h"

#include <list>

#include <QSharedPointer>

namespace snap
{

class snap_listen_thread
    : public snap_thread::snap_runner
{
public:
    typedef QSharedPointer<udp_client_server::udp_server> udp_server_t;

    snap_listen_thread( udp_server_t udp_server );

    // from class snap_thread
    virtual void run();

    enum class word_t
    {
        WORD_WAITING,
        WORD_SERVER_STOP,
        WORD_LOG_RESET
    };
    word_t get_word();

private:
    snap_thread::snap_mutex     f_mutex;
    udp_server_t                f_server;

    typedef std::deque<word_t>  word_list_t; // TODO: look into reusing the snap_thread queue instead of our own reimplementation
    word_list_t                 f_word_list;

    bool                        f_stop_received = false;
};

}
// namespace

// vim: ts=4 sw=4 et
