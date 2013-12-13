// Snap Websites Server -- robots.txt
// Copyright (C) 2011-2013  Made to Order Software Corp.
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
#ifndef SNAP_ROBOTSTXT_H
#define SNAP_ROBOTSTXT_H

#include "../layout/layout.h"

namespace snap
{
namespace robotstxt
{

class robotstxt_exception : public snap_exception
{
public:
    robotstxt_exception(const char *what_msg) : snap_exception("robots.txt: " + std::string(what_msg)) {}
    robotstxt_exception(const std::string& what_msg) : snap_exception("robots.txt: " + what_msg) {}
    robotstxt_exception(const QString& what_msg) : snap_exception("robots.txt: " + what_msg.toStdString()) {}
};

class robotstxt_exception_invalid_field_name : public robotstxt_exception
{
public:
    robotstxt_exception_invalid_field_name(const char *what_msg) : robotstxt_exception(what_msg) {}
    robotstxt_exception_invalid_field_name(const std::string& what_msg) : robotstxt_exception(what_msg) {}
    robotstxt_exception_invalid_field_name(const QString& what_msg) : robotstxt_exception(what_msg.toStdString()) {}
};

class robotstxt_exception_already_defined : public robotstxt_exception
{
public:
    robotstxt_exception_already_defined(const char *what_msg) : robotstxt_exception(what_msg) {}
    robotstxt_exception_already_defined(const std::string& what_msg) : robotstxt_exception(what_msg) {}
    robotstxt_exception_already_defined(const QString& what_msg) : robotstxt_exception(what_msg.toStdString()) {}
};



class robotstxt : public plugins::plugin, public path::path_execute, public layout::layout_content
{
public:
    static const char *ROBOT_NAME_ALL;
    static const char *ROBOT_NAME_GLOBAL;
    static const char *FIELD_NAME_DISALLOW;

    robotstxt();
    ~robotstxt();

    static robotstxt *  instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    virtual bool        on_path_execute(const QString& url);
    void                on_generate_header_content(layout::layout *l, const QString& path, QDomElement& header, QDomElement& metadata, const QString& ctemplate);
    virtual void        on_generate_main_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate);
    void                on_generate_page_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate);

    SNAP_SIGNAL(generate_robotstxt, (robotstxt *r), (r));

    void        add_robots_txt_field(const QString& value,
                                     const QString& field = FIELD_NAME_DISALLOW,
                                     const QString& robot = ROBOT_NAME_ALL,
                                     bool unique = false);

    void        output() const;

private:
    void initial_update(int64_t variables_timestamp);
    void content_update(int64_t variables_timestamp);
    void define_robots(const QString& path);

    struct robots_field_t
    {
        QString     f_field;
        QString     f_value;
    };
    typedef std::vector<robots_field_t> robots_field_array_t;
    typedef std::map<const QString, robots_field_array_t> robots_txt_t;

    zpsnap_child_t      f_snap;
    robots_txt_t        f_robots_txt;

    QString             f_robots_path; // path that the cache represents
    QString             f_robots_cache;
};

} // namespace robotstxt
} // namespace snap
#endif
// SNAP_ROBOTSTXT_H
// vim: ts=4 sw=4 et
