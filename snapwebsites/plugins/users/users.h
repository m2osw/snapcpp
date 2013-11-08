// Snap Websites Server -- users handling
// Copyright (C) 2012-2013  Made to Order Software Corp.
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
#ifndef SNAP_USERS_H
#define SNAP_USERS_H

#include "../layout/layout.h"
#include "../form/form.h"
#include "../sitemapxml/sitemapxml.h"

namespace snap
{
namespace users
{

enum name_t
{
    SNAP_NAME_USERS_TABLE,
    SNAP_NAME_USERS_PASSWORD,
    SNAP_NAME_USERS_PASSWORD_DIGEST,
    SNAP_NAME_USERS_PASSWORD_SALT,
    SNAP_NAME_USERS_ORIGINAL_EMAIL,
    SNAP_NAME_USERS_ORIGINAL_IP
};
const char *get_name(name_t name);



class users : public plugins::plugin, public path::path_execute, public layout::layout_content, public form::form_post
{
public:
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_LOG_IN = 1;
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_LOG_IN_BLOCK = 2;
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_REGISTER = 3;
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_REGISTER_BLOCK = 4;
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_PASSWORD = 5;
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_PASSWORD_BLOCK = 6;
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_LOG_IN_SESSION = 7;

    users();
    virtual ~users();

    static users *          instance();
    virtual QString         description() const;
    virtual int64_t         do_update(int64_t last_updated);
    QSharedPointer<QtCassandra::QCassandraTable> get_users_table();

    void                    on_bootstrap(::snap::snap_child *snap);
    void                    on_init();
    void                    on_can_handle_dynamic_path(path::path *path_plugin, const QString& cpath);
    virtual void            on_generate_main_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body);
    void                    on_generate_sitemapxml(sitemapxml::sitemapxml *sitemap);
    bool                    on_path_execute(const QString& cpath);
    void                    on_process_cookies();

    virtual QDomDocument    on_get_xml_form(const QString& cpath);
    virtual void            on_process_post(const QString& uri_path, const sessions::sessions::session_info& info);

    bool                    register_user(const QString& email, const QString& password);
    void                    verify_email(const QString& email);

private:
    void                    initial_update(int64_t variables_timestamp);
    void                    content_update(int64_t variables_timestamp);
    void                    generate_login_form(QDomElement& body);
    void                    generate_register_form(QDomElement& body);
    void                    process_login_form();
    void                    process_register_form();
    void                    create_password_salt(QByteArray& salt);
    void                    encrypt_password(const QString& digest, const QString& password, const QByteArray& salt, QByteArray& hash);

    zpsnap_child_t          f_snap;
};

} // namespace users
} // namespace snap
#endif
// SNAP_USERS_H
// vim: ts=4 sw=4 et
