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

#include "../path/path.h"
#include "../layout/layout.h"
#include <QSet>

namespace snap
{
namespace permissions
{

class permissions_exception : public snap_exception {};
class permissions_exception_invalid_field_name : public permissions_exception {};
class permissions_exception_already_defined : public permissions_exception {};

enum name_t
{
    SNAP_NAME_PERMISSIONS_PATH,
    SNAP_NAME_PERMISSIONS_ACTION_PATH,
    SNAP_NAME_PERMISSIONS_GROUPS_PATH,
    SNAP_NAME_PERMISSIONS_RIGHTS_PATH
};
const char *get_name(name_t name);


class permissions : public plugins::plugin, public layout::layout_content
{
public:
    class sets_t
    {
    public:
        typedef QSet<QString>           set_t;
        typedef QMap<QString, set_t>    req_sets_t;

                            sets_t(const QString& user_path, const QString& path, const QString& action);

        const QString&      get_user_path() const;
        const QString&      get_path() const;
        const QString&      get_action() const;

        void                add_user_right(const QString& right);
        int                 get_user_rights_count() const;

        void                add_plugin_permission(const QString& plugin, const QString& right);
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
    virtual void            on_generate_main_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate);
    void                    on_validate_action(const QString& path, QString& action);

    SNAP_SIGNAL(get_user_rights, (permissions *p, sets_t& sets), (p, sets));
    SNAP_SIGNAL(get_plugin_permissions, (permissions *p, sets_t& sets), (p, sets));

    bool                    access_allowed(const QString& user_path, const QString& path, QString& action);

private:
    void content_update(int64_t variables_timestamp);

    zpsnap_child_t          f_snap;
};

} // namespace permissions
} // namespace snap
#endif
// SNAP_PERMISSIONS_H
// vim: ts=4 sw=4 et
