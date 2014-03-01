// Snap Websites Server -- handle the JavaScript WYSIWYG editor
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

#include "../users/users.h"

namespace snap
{
namespace editor
{

class editor_exception : public snap_exception
{
public:
    editor_exception(char const *       what_msg) : snap_exception("editor", what_msg) {}
    editor_exception(std::string const& what_msg) : snap_exception("editor", what_msg) {}
    editor_exception(QString const&     what_msg) : snap_exception("editor", what_msg) {}
};

class editor_exception_invalid_argument : public editor_exception
{
public:
    editor_exception_invalid_argument(char const *       what_msg) : editor_exception(what_msg) {}
    editor_exception_invalid_argument(std::string const& what_msg) : editor_exception(what_msg) {}
    editor_exception_invalid_argument(QString const&     what_msg) : editor_exception(what_msg) {}
};

class editor_exception_invalid_path : public editor_exception
{
public:
    editor_exception_invalid_path(char const *       what_msg) : editor_exception(what_msg) {}
    editor_exception_invalid_path(std::string const& what_msg) : editor_exception(what_msg) {}
    editor_exception_invalid_path(QString const&     what_msg) : editor_exception(what_msg) {}
};



enum name_t
{
    SNAP_NAME_EDITOR_DRAFTS_PATH,
    SNAP_NAME_EDITOR_PAGE_TYPE
};
char const *get_name(name_t name) __attribute__ ((const));


class editor : public plugins::plugin, public path::path_execute, public layout::layout_content, public form::form_post
{
public:
    static int const    EDITOR_SESSION_ID_EDIT = 1;

    enum save_mode_t
    {
        EDITOR_SAVE_MODE_UNKNOWN = -1,
        EDITOR_SAVE_MODE_DRAFT,
        EDITOR_SAVE_MODE_PUBLISH,
        EDITOR_SAVE_MODE_SAVE,
        EDITOR_SAVE_MODE_NEW_BRANCH,
        EDITOR_SAVE_MODE_AUTO_DRAFT
    };

                        editor();
                        ~editor();

    static editor *     instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QSharedPointer<QtCassandra::QCassandraTable> get_emails_table();

    void                on_bootstrap(snap_child *snap);
    void                on_register_backend_action(snap::server::backend_action_map_t& actions);
    void                on_generate_header_content(content::path_info_t& path, QDomElement& header, QDomElement& metadata, QString const& ctemplate);
    virtual void        on_generate_main_content(content::path_info_t& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    bool                on_path_execute(content::path_info_t& ipath);
    void                on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info);
    virtual void        on_process_form_post(content::path_info_t& cpath, sessions::sessions::session_info const& info);
    void                on_validate_post_for_widget(content::path_info_t& ipath, sessions::sessions::session_info& info,
                                         QDomElement const& widget, QString const& widget_name,
                                         QString const& widget_type, bool is_secret);
    void                on_process_post(QString const& uri_path);

    static save_mode_t  string_to_save_mode(QString const& mode);

    SNAP_SIGNAL(save_editor_fields, (content::path_info_t& ipath, QtCassandra::QCassandraRow::pointer_t row), (ipath, row));

private:
    void                content_update(int64_t variables_timestamp);
    void                process_new_draft();
    void                editor_save(content::path_info_t& ipath);

    zpsnap_child_t      f_snap;
};

} // namespace editor
} // namespace snap
// vim: ts=4 sw=4 et
