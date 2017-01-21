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

#include "snapwebsites/snap_listen_thread.h"
#include "snapwebsites/log.h"

#include <vector>
#include <algorithm>

#include "snapwebsites/poison.h"


namespace snap
{

namespace
{

// wait for up to 5 minutes (x 60 seconds)
//
int const g_timeout = 5 * 60 * 1000;

// maximum size of word
int const g_bufsize = 256;

} // no name namespace


snap_listen_thread::snap_listen_thread( udp_server_t udp_server )
    : snap_runner("snap_listen_thread")
    , f_server(udp_server)
{
}


snap_listen_thread::word_t snap_listen_thread::get_word()
{
    snap_thread::snap_lock lock( f_mutex );

    if( f_stop_received )
    {
        return word_t::WORD_SERVER_STOP;
    }

    if( f_word_list.empty() )
    {
        return word_t::WORD_WAITING;
    }

    word_t const front_word( f_word_list.front() );
    f_word_list.pop_front();
    return front_word;
}


void snap_listen_thread::run()
{
    for(;;)
    {
        // sleep till next PING (but max. 5 minutes)
        //
        std::string const word( f_server->timed_recv( g_bufsize, g_timeout ) );
        if( word.empty() )
        {
            continue;
        }

        if( word == "STOP" )
        {
            // clean STOP
            //
            snap_thread::snap_lock lock( f_mutex );
            SNAP_LOG_TRACE("STOP received");
            f_stop_received = true;
            break;
        }
        else if( word == "NLOG")
        {
            // reset the logs
            //
            snap_thread::snap_lock lock( f_mutex );
            SNAP_LOG_TRACE("NLOG received");
            f_word_list.push_back( word_t::WORD_LOG_RESET );
        }
        else
        {
            SNAP_LOG_WARNING() << "snap_listen_thread.cpp:snap_listen_thread::run(): received an unknown word '" << word << "'";
        }
    }
}


}
// namespace snap

// vim: ts=4 sw=4 et
