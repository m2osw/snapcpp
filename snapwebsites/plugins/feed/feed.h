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

#include "../layout/layout.h"
#include "../path/path.h"

namespace snap
{
namespace feed
{


enum name_t
{
    SNAP_NAME_FEED_DATE
};
char const *get_name(name_t name) __attribute__ ((const));


class feed_exception : public snap_exception
{
public:
    feed_exception(char const *what_msg) : snap_exception("Feed: " + std::string(what_msg)) {}
    feed_exception(std::string const& what_msg) : snap_exception("Feed: " + what_msg) {}
    feed_exception(QString const& what_msg) : snap_exception("Feed: " + what_msg.toStdString()) {}
};



class feed : public plugins::plugin, public path::path_execute, public layout::layout_content
{
public:
    static const sessions::sessions::session_info::session_id_t FEED_SESSION_ID_SETTINGS = 1;      // settings-form.xml

                        feed();
                        ~feed();

    static feed *       instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    virtual bool        on_path_execute(const QString& url);
    virtual void        on_generate_main_content(layout::layout *l, content::path_info_t& ipath, QDomElement& page, QDomElement& body, const QString& ctemplate);
    void                on_generate_header_content(layout::layout *l, content::path_info_t& ipath, QDomElement& header, QDomElement& metadata, const QString& ctemplate);
    void                on_can_handle_dynamic_path(content::path_info_t& cpath, path::plugin_info_t& plugin_info);

private:
    void initial_update(int64_t variables_timestamp);
    void content_update(int64_t variables_timestamp);

    zpsnap_child_t                                  f_snap;
};


} // namespace feed
} // namespace snap
// vim: ts=4 sw=4 et
