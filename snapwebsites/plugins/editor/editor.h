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

class editor_exception_invalid_editor_form_xml : public editor_exception
{
public:
    editor_exception_invalid_editor_form_xml(char const *       what_msg) : editor_exception(what_msg) {}
    editor_exception_invalid_editor_form_xml(std::string const& what_msg) : editor_exception(what_msg) {}
    editor_exception_invalid_editor_form_xml(QString const&     what_msg) : editor_exception(what_msg) {}
};



enum name_t
{
    SNAP_NAME_EDITOR_DRAFTS_PATH,
    SNAP_NAME_EDITOR_LAYOUT,
    SNAP_NAME_EDITOR_PAGE_TYPE,
    SNAP_NAME_EDITOR_TYPE_EXTENDED_FORMAT_PATH,
    SNAP_NAME_EDITOR_TYPE_FORMAT_PATH
};
char const *get_name(name_t name) __attribute__ ((const));


class editor : public plugins::plugin, public path::path_execute, public layout::layout_content, public form::form_post, public layout::layout_boxes
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

    typedef QMap<QString, QString> params_map_t;

    struct editor_uri_token
    {
        editor_uri_token(content::path_info_t& ipath, QString const& page_name, params_map_t const& params)
            : f_ipath(ipath)
            , f_page_name(page_name)
            , f_params(params)
            //, f_token("") -- auto-init
            //, f_result("") -- auto-init
        {
        }

        content::path_info_t&   f_ipath;
        QString const&          f_page_name;
        params_map_t const&     f_params;
        QString                 f_token;
        QString                 f_result;
    };

                        editor();
                        ~editor();

    static editor *     instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QSharedPointer<QtCassandra::QCassandraTable> get_emails_table();

    void                on_bootstrap(snap_child *snap);
    void                on_generate_header_content(content::path_info_t& path, QDomElement& header, QDomElement& metadata, QString const& ctemplate);
    virtual void        on_generate_main_content(content::path_info_t& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    bool                on_path_execute(content::path_info_t& ipath);
    void                on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info);
    virtual void        on_process_form_post(content::path_info_t& cpath, sessions::sessions::session_info const& info);
    void                on_validate_post_for_widget(content::path_info_t& ipath, sessions::sessions::session_info& info,
                                         QDomElement const& widget, QString const& widget_name,
                                         QString const& widget_type, bool is_secret);
    void                on_process_post(QString const& uri_path);
    void                on_generate_page_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate);
    virtual void        on_generate_boxes_content(content::path_info_t& page_cpath, content::path_info_t& ipath, QDomElement& page, QDomElement& box, QString const& ctemplate);

    QString             format_uri(QString const& format, content::path_info_t& ipath, QString const& page_name, params_map_t const& params);
    static save_mode_t  string_to_save_mode(QString const& mode);
    static QString      clean_post_value(QString const& widget_type, QString const& value);
    void                parse_out_inline_img(content::path_info_t& ipath, QString& body);
    QDomDocument        get_editor_widgets(content::path_info_t& ipath);

    SNAP_SIGNAL(save_editor_fields, (content::path_info_t& ipath, QtCassandra::QCassandraRow::pointer_t row), (ipath, row));
    SNAP_SIGNAL(validate_editor_post_for_widget, (content::path_info_t& ipath, sessions::sessions::session_info& info, QDomElement const& widget, QString const& widget_name, QString const& widget_type, QString const& value, bool const is_secret), (ipath, info, widget, widget_name, widget_type, value, is_secret));
    SNAP_SIGNAL(replace_uri_token, (editor_uri_token& token_info), (token_info));
    SNAP_SIGNAL(editor_process_post_result, (content::path_info_t& ipath, bool const succeeded), (ipath, succeeded));

private:
    void                content_update(int64_t variables_timestamp);
    void                process_new_draft();
    void                editor_save(content::path_info_t& ipath, sessions::sessions::session_info& info);
    void                editor_create_new_branch(content::path_info_t& ipath);
    bool                save_inline_image(content::path_info_t& ipath, QDomElement img, QString const& src);

    zpsnap_child_t      f_snap;
};

} // namespace editor
} // namespace snap
// vim: ts=4 sw=4 et
