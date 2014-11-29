// Snap Websites Server -- feed management (RSS like feeds and aggregators)
// Copyright (C) 2013-2014  Made to Order Software Corp.
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

#include "snapwebsites.h"

namespace snap
{
namespace feed
{


enum name_t
{
    SNAP_NAME_FEED_AGE,
    SNAP_NAME_FEED_PAGE_LAYOUT
};
char const *get_name(name_t name) __attribute__ ((const));


class feed_exception : public snap_exception
{
public:
    feed_exception(char const *       what_msg) : snap_exception("Feed: " + std::string(what_msg)) {}
    feed_exception(std::string const& what_msg) : snap_exception("Feed: " + what_msg) {}
    feed_exception(QString const&     what_msg) : snap_exception("Feed: " + what_msg.toStdString()) {}
};



class feed : public plugins::plugin
{
public:
                        feed();
                        ~feed();

    static feed *       instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    void                on_backend_process();

private:
    void                content_update(int64_t variables_timestamp);
    void                generate_feeds();

    zpsnap_child_t      f_snap;
};


} // namespace feed
} // namespace snap
// vim: ts=4 sw=4 et
