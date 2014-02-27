//===============================================================================
// Copyright (c) 2005-2014 by Made to Order Software Corporation
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

// wait for up to 5 minutes (x 60 seconds)
//
#define TIMEOUT (5 * 60 * 1000)
#define BUFSIZE 256

namespace snap
{


snap_listen_thread::snap_listen_thread( udp_server_t udp_server )
    : snap_runner("snap_listen_thread")
    , f_server(udp_server)
    , f_word(Waiting)
{
}


snap_listen_thread::word_t snap_listen_thread::get_word()
{
    snap_thread::snap_lock lock( f_mutex );
    return f_word;
}


void snap_listen_thread::reset_word()
{
    snap_thread::snap_lock lock( f_mutex );
    f_word = Waiting;
}


void snap_listen_thread::run()
{
    while( true )
    {
        // sleep till next PING (but max. 5 minutes)
        //
        std::vector<char> buf;
        buf.resize( BUFSIZE+1, '\0' );
        const int r = f_server->timed_recv( &buf[0], buf.size(), TIMEOUT );
        if( r > -1 )
        {
            buf.resize( r+1 );
            buf.shrink_to_fit();
            std::string word;
            word.resize( r );
            std::copy( buf.begin(), buf.end(), word.begin() );
            //
            if( word == "STOP" )
            {
                // clean STOP
                //
                snap_thread::snap_lock lock( f_mutex );
                SNAP_LOG_TRACE("STOP");
                f_word = ServerStop;
                break;
            }
            else if( word == "NLOG")
            {
                // reset the logs
                //
                snap_thread::snap_lock lock( f_mutex );
                SNAP_LOG_TRACE("NLOG");
                f_word = LogReset;
            }
            else
            {
                SNAP_LOG_WARNING() << "snap_listen_thread received an unknown word '" << word << "'";
            }
        }
    }
}


}
// namespace snap

// vim: ts=4 sw=4 et syntax=cpp.doxygen
