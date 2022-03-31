// Snap Websites Server -- Network watchdog
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
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

// our lib
//
#include "snapwatchdog/snapwatchdog.h"

// snapwebsites lib
//
#include <snapwebsites/plugins.h>
#include <snapwebsites/snap_child.h>

// Qt lib
//
#include <QDomDocument>



namespace snap
{
namespace network
{

enum class name_t
{
    SNAP_NAME_WATCHDOG_NETWORK_NAME
};
char const * get_name(name_t name) __attribute__ ((const));



class network_exception : public snap_exception
{
public:
    network_exception(char const *        what_msg) : snap_exception("network", what_msg) {}
    network_exception(std::string const & what_msg) : snap_exception("network", what_msg) {}
    network_exception(QString const &     what_msg) : snap_exception("network", what_msg) {}
};

class network_exception_invalid_argument : public network_exception
{
public:
    network_exception_invalid_argument(char const *        what_msg) : network_exception(what_msg) {}
    network_exception_invalid_argument(std::string const & what_msg) : network_exception(what_msg) {}
    network_exception_invalid_argument(QString const &     what_msg) : network_exception(what_msg) {}
};





class network
    : public plugins::plugin
{
public:
                        network();
                        network(network const & rhs) = delete;
    virtual             ~network() override;

    network &           operator = (network const & rhs) = delete;

    static network *    instance();

    // plugins::plugin implementation
    virtual QString     description() const override;
    virtual QString     dependencies() const override;
    virtual int64_t     do_update(int64_t last_updated) override;
    virtual void        bootstrap(snap_child * snap) override;

    // server signals
    void                on_init();
    void                on_process_watch(QDomDocument doc);

private:
    bool                find_snapcommunicator(QDomElement e);
    bool                verify_snapcommunicator_connection(QDomElement e);

    watchdog_child *    f_snap = nullptr;
    QString             f_network_data_path = QString();
};

} // namespace network
} // namespace snap
// vim: ts=4 sw=4 et
