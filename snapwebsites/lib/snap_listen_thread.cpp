//===============================================================================
// Copyright (c) 2005-2015 by Made to Order Software Corporation
// 
// All Rights Reserved.
// 
// The source code in this file ("Source Code") is provided by Made to Order Software Corporation
// to you under the terms of the GNU General Public License, version 2.0
// ("GPL").  Terms of the GPL can be found in doc/GPL-license.txt in this distribution.
// 
// By copying, modifying or distributing this software, you acknowledge
// that you have read and understood your obligations described above,
// and agree to abide by those obligations.
// 
// ALL SOURCE CODE IN THIS DISTRIBUTION IS PROVIDED "AS IS." THE AUTHOR MAKES NO
// WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
// COMPLETENESS OR PERFORMANCE.
//===============================================================================

#include "snap_listen_thread.h"
#include "log.h"

#include <vector>
#include <algorithm>


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
