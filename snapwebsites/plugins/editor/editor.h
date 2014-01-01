// Snap Websites Server -- handle the JavaScript WYSIWYG editor
// Copyright (C) 2013  Made to Order Software Corp.
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
#ifndef SNAP_EDITOR_H
#define SNAP_EDITOR_H

#include "../content/content.h"

namespace snap
{
namespace editor
{

class editor_exception : public snap_exception
{
public:
    editor_exception(char const *whatmsg) : snap_exception("editor", whatmsg) {}
    editor_exception(std::string const& what_msg) : snap_exception("editor", what_msg) {}
    editor_exception(QString const& whatmsg) : snap_exception("editor", whatmsg) {}
};

class editor_exception_invalid_argument : public editor_exception
{
public:
    editor_exception_invalid_argument(char const *what_msg) : editor_exception(what_msg) {}
    editor_exception_invalid_argument(std::string const& what_msg) : editor_exception(what_msg) {}
    editor_exception_invalid_argument(QString const& what_msg) : editor_exception(what_msg) {}
};

//class editor_exception_too_many_levels : public editor_exception
//{
//public:
//    editor_exception_too_many_levels(char const *what_msg) : editor_exception(what_msg) {}
//    editor_exception_too_many_levels(std::string const& what_msg) : editor_exception(what_msg) {}
//    editor_exception_too_many_levels(QString const& what_msg) : editor_exception(what_msg) {}
//};


enum name_t
{
    SNAP_NAME_EDITOR,
};
const char *get_name(name_t name) __attribute__ ((const));


class editor : public plugins::plugin, public layout::layout_content
{
public:
                        editor();
                        ~editor();

    static editor *     instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QSharedPointer<QtCassandra::QCassandraTable> get_emails_table();

    void                on_bootstrap(snap_child *snap);
    void                on_register_backend_action(snap::server::backend_action_map_t& actions);
    void                on_generate_header_content(layout::layout *l, QString const& path, QDomElement& header, QDomElement& metadata, QString const& ctemplate);
    virtual void        on_generate_main_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);

private:
    void                initial_update(int64_t variables_timestamp);
    void                content_update(int64_t variables_timestamp);

    zpsnap_child_t      f_snap;
};

} // namespace editor
} // namespace snap
#endif
// SNAP_EDITOR_H
// vim: ts=4 sw=4 et
