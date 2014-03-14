// Snap Websites Server -- manage permissions for users, forms, etc.
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


namespace snap
{
namespace permissions
{

enum name_t
{
    SNAP_NAME_PERMISSIONS_ACTION_PATH,
    SNAP_NAME_PERMISSIONS_ADMINISTER,
    SNAP_NAME_PERMISSIONS_DYNAMIC,
    SNAP_NAME_PERMISSIONS_EDIT,
    SNAP_NAME_PERMISSIONS_GROUP,
    SNAP_NAME_PERMISSIONS_GROUP_RETURNING_REGISTERED_USER,
    SNAP_NAME_PERMISSIONS_GROUPS_PATH,
    SNAP_NAME_PERMISSIONS_LOGIN_STATUS_SPAMMER,
    SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR,
    SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_VISITOR,
    SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED,  // partial log in
    SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED,            // full log in
    SNAP_NAME_PERMISSIONS_MAKE_ROOT,
    SNAP_NAME_PERMISSIONS_NAMESPACE,
    SNAP_NAME_PERMISSIONS_PATH,
    SNAP_NAME_PERMISSIONS_RIGHTS_PATH,
    SNAP_NAME_PERMISSIONS_USERS_PATH,
    SNAP_NAME_PERMISSIONS_VIEW
};
char const *get_name(name_t name) __attribute__ ((const));



class permissions_exception : public snap_exception
{
public:
    permissions_exception(char const *what_msg) : snap_exception("Permissions: " + std::string(what_msg)) {}
    permissions_exception(std::string const& what_msg) : snap_exception("Permissions: " + what_msg) {}
    permissions_exception(QString const& what_msg) : snap_exception("Permissions: " + what_msg.toStdString()) {}
};

class permissions_exception_invalid_group_name : public permissions_exception
{
public:
    permissions_exception_invalid_group_name(char const *what_msg) : permissions_exception(what_msg) {}
    permissions_exception_invalid_group_name(std::string const& what_msg) : permissions_exception(what_msg) {}
    permissions_exception_invalid_group_name(QString const& what_msg) : permissions_exception(what_msg) {}
};

class permissions_exception_invalid_path : public permissions_exception
{
public:
    permissions_exception_invalid_path(char const *what_msg) : permissions_exception(what_msg) {}
    permissions_exception_invalid_path(std::string const& what_msg) : permissions_exception(what_msg) {}
    permissions_exception_invalid_path(QString const& what_msg) : permissions_exception(what_msg) {}
};




class permissions : public plugins::plugin, public layout::layout_content, public server::backend_action
{
public:
    class sets_t
    {
    public:
        typedef QVector<QString>        set_t;
        typedef QMap<QString, set_t>    req_sets_t;

                                sets_t(QString const& user_path, content::path_info_t& ipath, QString const& action, QString const& login_status);

        void                    set_login_status(QString const& status);
        QString const&          get_login_status() const;
                       
        QString const&          get_user_path() const;
        content::path_info_t&   get_ipath() const;
        QString const&          get_action() const;

        void                    add_user_right(QString right);
        int                     get_user_rights_count() const;

        void                    add_plugin_permission(QString const& plugin, QString right);

        bool                    is_root() const;
        bool                    allowed() const;

    private:
        QString                 f_user_path;
        content::path_info_t&   f_ipath;
        QString                 f_action;
        QString                 f_login_status;
        set_t                   f_user_rights;
        req_sets_t              f_plugin_permissions;
    };

                            permissions();
                            ~permissions();

    static permissions *    instance();
    virtual QString         description() const;
    virtual int64_t         do_update(int64_t last_updated);

    void                    on_bootstrap(snap_child *snap);
    virtual void            on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                    on_validate_action(content::path_info_t& path, QString const& action, permission_error_callback& err_callback);
    void                    on_access_allowed(QString const& user_path, content::path_info_t& ipath, QString const& action, QString const& login_status, content::permission_flag& result);
    void                    on_register_backend_action(server::backend_action_map_t& actions);
    virtual void            on_backend_action(QString const& action);
    void                    on_user_verified(content::path_info_t& ipath, int64_t identifier);

    SNAP_SIGNAL(get_user_rights, (permissions *perms, sets_t& sets), (perms, sets));
    SNAP_SIGNAL(get_plugin_permissions, (permissions *perms, sets_t& sets), (perms, sets));

    void                    add_user_rights(QString const & right, sets_t& sets);
    void                    add_plugin_permissions(QString const& plugin_name, QString const& group, sets_t& sets);

private:
    void                    content_update(int64_t variables_timestamp);
    void                    recursive_add_user_rights(QString const& key, sets_t& sets);
    void                    recursive_add_plugin_permissions(QString const& plugin_name, QString const& key, sets_t& sets);

    zpsnap_child_t          f_snap;
};

} // namespace permissions
} // namespace snap
// vim: ts=4 sw=4 et
