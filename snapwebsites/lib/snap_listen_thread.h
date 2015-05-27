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

#pragma once

#include "snap_thread.h"
#include "udp_client_server.h"

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

    controlled_vars::flbool_t   f_stop_received;
};

}
// namespace

// vim: ts=4 sw=4 et
