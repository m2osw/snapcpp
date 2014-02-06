// Snap Websites Server -- users handling
// Copyright (C) 2012-2014  Made to Order Software Corp.
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

#include "../form/form.h"
#include "../sitemapxml/sitemapxml.h"

namespace snap
{
namespace users
{

enum name_t
{
    SNAP_NAME_USERS_ANONYMOUS_PATH,
    SNAP_NAME_USERS_AUTHOR,
    SNAP_NAME_USERS_AUTHORED_PAGES,
    SNAP_NAME_USERS_AUTO_PATH,
    SNAP_NAME_USERS_BLACK_LIST,
    SNAP_NAME_USERS_BLOCKED_PATH,
    SNAP_NAME_USERS_CHANGING_PASSWORD_KEY,
    SNAP_NAME_USERS_CREATED_TIME,
    SNAP_NAME_USERS_FORGOT_PASSWORD_EMAIL,
    SNAP_NAME_USERS_FORGOT_PASSWORD_IP,
    SNAP_NAME_USERS_FORGOT_PASSWORD_ON,
    SNAP_NAME_USERS_IDENTIFIER,
    SNAP_NAME_USERS_ID_ROW,
    SNAP_NAME_USERS_INDEX_ROW,
    SNAP_NAME_USERS_LOCALES,
    SNAP_NAME_USERS_LOGIN_IP,
    SNAP_NAME_USERS_LOGIN_ON,
    SNAP_NAME_USERS_LOGIN_REFERRER,
    SNAP_NAME_USERS_LOGOUT_IP,
    SNAP_NAME_USERS_LOGOUT_ON,
    SNAP_NAME_USERS_NEW_PATH,
    SNAP_NAME_USERS_ORIGINAL_EMAIL,
    SNAP_NAME_USERS_ORIGINAL_IP,
    SNAP_NAME_USERS_PASSWORD,
    SNAP_NAME_USERS_PASSWORD_DIGEST,
    SNAP_NAME_USERS_PASSWORD_PATH,
    SNAP_NAME_USERS_PASSWORD_SALT,
    SNAP_NAME_USERS_PATH,
    SNAP_NAME_USERS_PICTURE,
    SNAP_NAME_USERS_PREVIOUS_LOGIN_IP,
    SNAP_NAME_USERS_PREVIOUS_LOGIN_ON,
    //SNAP_NAME_USERS_SESSION_COOKIE, -- use a random name instead
    SNAP_NAME_USERS_STATUS,
    SNAP_NAME_USERS_TABLE,
    SNAP_NAME_USERS_USERNAME,
    SNAP_NAME_USERS_VERIFIED_IP,
    SNAP_NAME_USERS_VERIFIED_ON,
    SNAP_NAME_USERS_VERIFY_EMAIL
};
char const *get_name(name_t name) __attribute__ ((const));



class users_exception : public snap_exception
{
public:
    users_exception(char const *       what_msg) : snap_exception("users", what_msg) {}
    users_exception(std::string const& what_msg) : snap_exception("users", what_msg) {}
    users_exception(QString const&     what_msg) : snap_exception("users", what_msg) {}
};

class users_exception_invalid_path : public users_exception
{
public:
    users_exception_invalid_path(char const *       what_msg) : users_exception(what_msg) {}
    users_exception_invalid_path(std::string const& what_msg) : users_exception(what_msg) {}
    users_exception_invalid_path(QString const&     what_msg) : users_exception(what_msg) {}
};

class users_exception_size_mismatch : public users_exception
{
public:
    users_exception_size_mismatch(char const *       what_msg) : users_exception(what_msg) {}
    users_exception_size_mismatch(std::string const& what_msg) : users_exception(what_msg) {}
    users_exception_size_mismatch(QString const&     what_msg) : users_exception(what_msg) {}
};

class users_exception_digest_not_available : public users_exception
{
public:
    users_exception_digest_not_available(char const *       what_msg) : users_exception(what_msg) {}
    users_exception_digest_not_available(std::string const& what_msg) : users_exception(what_msg) {}
    users_exception_digest_not_available(QString const&     what_msg) : users_exception(what_msg) {}
};

class users_exception_encryption_failed : public users_exception
{
public:
    users_exception_encryption_failed(char const *       what_msg) : users_exception(what_msg) {}
    users_exception_encryption_failed(std::string const& what_msg) : users_exception(what_msg) {}
    users_exception_encryption_failed(QString const&     what_msg) : users_exception(what_msg) {}
};






class users : public plugins::plugin, public path::path_execute, public layout::layout_content, public layout::layout_boxes, public form::form_post
{
public:
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_LOG_IN = 1;                    // login-form.xml
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_LOG_IN_BOX = 2;                // login-box-form.xml
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_REGISTER = 3;                  // register-form.xml
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_REGISTER_BOX = 4;              // register-box-form.xml
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_FORGOT_PASSWORD = 5;           // forgot-password-form.xml
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_VERIFY = 6;                    // verify-form.xml
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_LOG_IN_SESSION = 7;
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_VERIFY_EMAIL = 8;
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_FORGOT_PASSWORD_EMAIL = 9;
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_RESEND_EMAIL = 10;             // resend-email-form.xml
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_NEW_PASSWORD = 11;             // new-password-form.xml
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_REPLACE_PASSWORD = 12;         // replace-password-form.xml
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_PASSWORD = 13;                 // password-form.xml
    static const sessions::sessions::session_info::session_id_t USERS_SESSION_ID_VERIFY_CREDENTIALS = 14;       // verify-credentials-form.xml

                            users();
    virtual                 ~users();

    static users *          instance();
    virtual QString         description() const;
    virtual int64_t         do_update(int64_t last_updated);
    QtCassandra::QCassandraTable::pointer_t get_users_table();

    void                    on_bootstrap(::snap::snap_child *snap);
    void                    on_init();
    void                    on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info);
    void                    on_generate_header_content(content::path_info_t& path, QDomElement& hader, QDomElement& metadata, QString const& ctemplate);
    virtual void            on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate);
    virtual void            on_generate_boxes_content(content::path_info_t& page_ipath, content::path_info_t& ipath, QDomElement& page, QDomElement& boxes, QString const& ctemplate);
    void                    on_generate_page_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate);
    bool                    on_path_execute(content::path_info_t& ipath);
    void                    on_process_cookies();
    void                    on_attach_to_session();
    void                    on_detach_from_session();
    void                    on_define_locales(QString& locales);
    void                    on_create_content(content::path_info_t& path, QString const& owner, QString const& type);
    void                    on_improve_signature(QString const& path, QString& signature);
    void                    on_replace_token(content::path_info_t& ipath, QString const& plugin_owner, QDomDocument& xml, filter::filter::token_info_t& token);

    virtual void            on_process_form_post(content::path_info_t& ipath, sessions::sessions::session_info const& session_info);

    QString                 get_user_cookie_name();
    QString                 get_user_key() const;
    QString                 get_user_path() const;
    bool                    user_is_a_spammer();
    bool                    user_is_logged_in();
    bool                    register_user(QString const& email, QString const& password);
    void                    attach_to_session(QString const& name, QString const& data);
    QString                 detach_from_session(QString const& name) const;

private:
    enum login_mode_t
    {
        LOGIN_MODE_FULL,
        LOGIN_MODE_VERIFICATION
    };
    void                    content_update(int64_t variables_timestamp);
    void                    show_user(content::path_info_t& cpath, QDomElement& page, QDomElement& body);
    void                    prepare_login_form();
    void                    logout_user(content::path_info_t& cpath, QDomElement& page, QDomElement& body);
    void                    prepare_basic_anonymous_form();
    void                    prepare_forgot_password_form();
    void                    prepare_password_form();
    void                    prepare_new_password_form();
    void                    process_login_form(login_mode_t login_mode);
    void                    prepare_verify_credentials_form();
    void                    process_register_form();
    void                    create_password_salt(QByteArray& salt);
    void                    encrypt_password(QString const& digest, QString const& password, QByteArray const& salt, QByteArray& hash);
    void                    verify_user(content::path_info_t& ipath);
    void                    process_verify_form();
    void                    process_verify_resend_form();
    void                    process_forgot_password_form();
    void                    process_new_password_form();
    void                    process_password_form();
    void                    process_replace_password_form();
    void                    prepare_replace_password_form(QDomElement& body);
    void                    verify_email(QString const& email);
    void                    verify_password(content::path_info_t& cpath);
    void                    forgot_password_email(QString const& email);

    zpsnap_child_t              f_snap;
    QString                     f_user_key; // logged in user email address
    controlled_vars::fbool_t    f_user_logged_in;
    QString                     f_user_changing_password_key; // not quite logged in user
    std::shared_ptr<sessions::sessions::session_info> f_info; // user, logged in or anonymous, cookie related information
};

} // namespace users
} // namespace snap
// vim: ts=4 sw=4 et
