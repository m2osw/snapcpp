// Snap Websites Server -- manage permissions for users, forms, etc.
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
#ifndef SNAP_PERMISSIONS_H
#define SNAP_PERMISSIONS_H

#include "../layout/layout.h"
#include <QSet>

namespace snap
{
namespace permissions
{

enum name_t
{
    SNAP_NAME_PERMISSIONS_DYNAMIC,
    SNAP_NAME_PERMISSIONS_PATH,
    SNAP_NAME_PERMISSIONS_ACTION_PATH,
    SNAP_NAME_PERMISSIONS_GROUPS_PATH,
    SNAP_NAME_PERMISSIONS_RIGHTS_PATH
};
const char *get_name(name_t name) __attribute__ ((const));



class permissions_exception : public snap_exception
{
public:
    permissions_exception(const char *what_msg) : snap_exception("Permissions: " + std::string(what_msg)) {}
    permissions_exception(const std::string& what_msg) : snap_exception("Permissions: " + what_msg) {}
    permissions_exception(const QString& what_msg) : snap_exception("Permissions: " + what_msg.toStdString()) {}
};

class permissions_exception_invalid_group_name : public permissions_exception
{
public:
    permissions_exception_invalid_group_name(const char *what_msg) : permissions_exception(what_msg) {}
    permissions_exception_invalid_group_name(const std::string& what_msg) : permissions_exception(what_msg) {}
    permissions_exception_invalid_group_name(const QString& what_msg) : permissions_exception(what_msg) {}
};

class permissions_exception_invalid_path : public permissions_exception
{
public:
    permissions_exception_invalid_path(const char *what_msg) : permissions_exception(what_msg) {}
    permissions_exception_invalid_path(const std::string& what_msg) : permissions_exception(what_msg) {}
    permissions_exception_invalid_path(const QString& what_msg) : permissions_exception(what_msg) {}
};




class permissions : public plugins::plugin, public layout::layout_content
{
public:
    class sets_t
    {
    public:
        typedef QSet<QString>           set_t;
        typedef QMap<QString, set_t>    req_sets_t;

                            sets_t(QString const& user_path, QString const& path, QString const& action);

        QString const &     get_user_path() const;
        QString const &     get_path() const;
        QString const &     get_action() const;

        void                add_user_right(QString const& right);
        int                 get_user_rights_count() const;

        void                add_plugin_permission(QString const& plugin, QString const& right);

        bool                is_root() const;
        bool                allowed() const;

    private:
        QString             f_user_path;
        QString             f_path;
        QString             f_action;
        set_t               f_user_rights;
        req_sets_t          f_plugin_permissions;
    };

                            permissions();
                            ~permissions();

    static permissions *    instance();
    virtual QString         description() const;
    virtual int64_t         do_update(int64_t last_updated);

    void                    on_bootstrap(snap_child *snap);
    virtual void            on_generate_main_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                    on_validate_action(QString const& path, QString const& action);
    void                    on_access_allowed(QString const& user_path, QString const& path, QString const& action, server::permission_flag& result);

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
#endif
// SNAP_PERMISSIONS_H
// vim: ts=4 sw=4 et
