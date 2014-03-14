// Snap Websites Server -- path handling
// Copyright (C) 2011-2014  Made to Order Software Corp.
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

#include "../content/content.h"

namespace snap
{
namespace path
{

//enum name_t
//{
//    SNAP_NAME_PATH_PRIMARY_OWNER
//};
//char const *get_name(name_t name) __attribute__ ((const));


class dynamic_plugin_t
{
public:
                        dynamic_plugin_t()
                            : f_plugin(nullptr)
                        {
                        }

    plugins::plugin *   get_plugin() const { return f_plugin; }
    void                set_plugin(plugins::plugin *plugin);

private:
                        // prevent copies or a user could reset the pointer!
                        dynamic_plugin_t(dynamic_plugin_t const& rhs);
                        dynamic_plugin_t& operator = (dynamic_plugin_t const& rhs);

    plugins::plugin *   f_plugin;
};


class path_execute
{
public:
    virtual         ~path_execute() {} // ensure proper virtual tables
    virtual bool    on_path_execute(content::path_info_t& ipath) = 0;
};


class path : public plugins::plugin
{
public:
                                    path();
    virtual                         ~path();

    static path *                   instance();
    virtual QString                 description() const;

    void                            on_bootstrap(::snap::snap_child *snap);
    void                            on_init();
    void                            on_execute(QString const& uri_path);
    plugin *                        get_plugin(content::path_info_t& uri_path, permission_error_callback& err_callback);
    void                            verify_permissions(content::path_info_t& ipath, permission_error_callback& err_callback);
    QString                         default_action(content::path_info_t& ipath);

    SNAP_SIGNAL(can_handle_dynamic_path, (content::path_info_t& ipath, dynamic_plugin_t& plugin_info), (ipath, plugin_info));
    SNAP_SIGNAL(page_not_found, (content::path_info_t& ipath), (ipath));
    SNAP_SIGNAL(validate_action, (content::path_info_t& ipath, QString const& action, permission_error_callback& err_callback), (ipath, action, err_callback));
    SNAP_SIGNAL(access_allowed, (QString const& user_path, content::path_info_t& ipath, QString const& action, QString const& login_status, content::permission_flag& result), (user_path, ipath, action, login_status, result));

    void                            handle_dynamic_path(plugins::plugin *p);

private:
    zpsnap_child_t                  f_snap;
    controlled_vars::zint64_t       f_last_modified;
};

} // namespace path
} // namespace snap
// vim: ts=4 sw=4 et
