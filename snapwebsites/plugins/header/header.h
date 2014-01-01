// Snap Websites Server -- header management (HEAD tags and HTTP headers)
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
#ifndef SNAP_HEADER_H
#define SNAP_HEADER_H

#include "../layout/layout.h"
#include "../path/path.h"

namespace snap
{
namespace header
{


enum name_t
{
    SNAP_NAME_HEADER_GENERATOR,
    SNAP_NAME_HEADER_INTERNAL
};
const char *get_name(name_t name) __attribute__ ((const));


class header_exception : public snap_exception
{
public:
    header_exception(char const *what_msg) : snap_exception("Header: " + std::string(what_msg)) {}
    header_exception(std::string const& what_msg) : snap_exception("Header: " + what_msg) {}
    header_exception(QString const& what_msg) : snap_exception("Header: " + what_msg.toStdString()) {}
};




class header : public plugins::plugin, public path::path_execute, public layout::layout_content
{
public:
                        header();
                        ~header();

    static header *     instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    virtual bool        on_path_execute(QString const& url);
    virtual void        on_generate_main_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                on_generate_header_content(layout::layout *l, QString const& path, QDomElement& header, QDomElement& metadata, QString const& ctemplate);
    //void                on_generate_page_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);

private:
    void initial_update(int64_t variables_timestamp);
    void content_update(int64_t variables_timestamp);

    zpsnap_child_t      f_snap;
};


} // namespace header
} // namespace snap
#endif
// SNAP_HEADER_H
// vim: ts=4 sw=4 et
