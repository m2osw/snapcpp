// Snap Websites Server -- manage sessions for users, forms, etc.
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

#include "../layout/layout.h"

namespace snap
{
namespace sessions
{

enum name_t
{
    SNAP_NAME_SESSIONS_DATE,
    SNAP_NAME_SESSIONS_ID,
    SNAP_NAME_SESSIONS_LOGIN_LIMIT,
    SNAP_NAME_SESSIONS_PAGE_PATH,
    SNAP_NAME_SESSIONS_OBJECT_PATH,
    SNAP_NAME_SESSIONS_PLUGIN_OWNER,
    SNAP_NAME_SESSIONS_REMOTE_ADDR,
    SNAP_NAME_SESSIONS_RANDOM,
    SNAP_NAME_SESSIONS_TABLE,
    SNAP_NAME_SESSIONS_TIME_TO_LIVE,
    SNAP_NAME_SESSIONS_TIME_LIMIT,
    SNAP_NAME_SESSIONS_USED_UP,
    SNAP_NAME_SESSIONS_USER_AGENT
};
char const *get_name(name_t name) __attribute__ ((const));


class sessions_exception : public snap_exception
{
public:
    sessions_exception(const char *what_msg) : snap_exception("Sessions: " + std::string(what_msg)) {}
    sessions_exception(const std::string& what_msg) : snap_exception("Sessions: " + what_msg) {}
    sessions_exception(const QString& what_msg) : snap_exception("Sessions: " + what_msg.toStdString()) {}
};

class sessions_exception_invalid_parameter : public sessions_exception
{
public:
    sessions_exception_invalid_parameter(const char *what_msg) : sessions_exception(what_msg) {}
    sessions_exception_invalid_parameter(const std::string& what_msg) : sessions_exception(what_msg) {}
    sessions_exception_invalid_parameter(const QString& what_msg) : sessions_exception(what_msg.toStdString()) {}
};

class sessions_exception_invalid_range : public sessions_exception
{
public:
    sessions_exception_invalid_range(const char *what_msg) : sessions_exception(what_msg) {}
    sessions_exception_invalid_range(const std::string& what_msg) : sessions_exception(what_msg) {}
    sessions_exception_invalid_range(const QString& what_msg) : sessions_exception(what_msg.toStdString()) {}
};

class sessions_exception_no_random_data : public sessions_exception
{
public:
    sessions_exception_no_random_data(const char *what_msg) : sessions_exception(what_msg) {}
    sessions_exception_no_random_data(const std::string& what_msg) : sessions_exception(what_msg) {}
    sessions_exception_no_random_data(const QString& what_msg) : sessions_exception(what_msg.toStdString()) {}
};


class sessions : public plugins::plugin, public layout::layout_content
{
public:
    class session_info
    {
    public:
        enum session_info_type_t
        {
            SESSION_INFO_SECURE,        // think PCI Compliant website (credit card payment, etc.)
            SESSION_INFO_USER,          // a user cookie when logged in
            SESSION_INFO_FORM,          // a form unique identifier

            SESSION_INFO_VALID,         // the key was loaded successfully
            SESSION_INFO_MISSING,       // the key could not be loaded
            SESSION_INFO_OUT_OF_DATE,   // key is too old
            SESSION_INFO_USED_UP,       // key was already used
            SESSION_INFO_INCOMPATIBLE   // key is not compatible (wrong path, object, etc.)
        };
        typedef int        session_id_t;

        session_info();

        void set_session_type(session_info_type_t type);
        void set_session_id(session_id_t session_id);
        void set_session_key(QString const& session_key);
        void set_session_random();
        void set_session_random(int32_t random);
        void set_plugin_owner(QString const& plugin_owner);
        void set_page_path(QString const& page_path);
        void set_page_path(content::path_info_t& page_ipath);
        void set_object_path(QString const& object_path);
        void set_user_agent(QString const& user_agent);
        void set_time_to_live(int32_t time_to_live);
        void set_time_limit(time_t time_limit);
        void set_login_limit(time_t time_limit);
        void set_date(int64_t date);

        session_info_type_t get_session_type() const;
        session_id_t get_session_id() const;
        QString const& get_session_key() const;
        int32_t get_session_random() const;
        QString const& get_plugin_owner() const;
        QString const& get_page_path() const;
        QString const& get_object_path() const;
        QString const& get_user_agent() const;
        int32_t get_time_to_live() const;
        time_t get_time_limit() const;
        time_t get_login_limit() const;
        int64_t get_date() const;

        static char const *session_type_to_string(session_info_type_t type);

    private:
        // default to SESSION_INFO_SECURE
        typedef controlled_vars::limited_auto_init<session_info_type_t, SESSION_INFO_SECURE, SESSION_INFO_USER, SESSION_INFO_SECURE> auto_session_info_type_t;
        typedef controlled_vars::auto_init<int32_t, 300> time_to_live_t;
        typedef controlled_vars::auto_init<time_t, 0> ztime_t;
        typedef controlled_vars::auto_init<session_id_t, 0> zsession_id_t;

        auto_session_info_type_t    f_type;
        zsession_id_t               f_session_id;
        QString                     f_session_key;
        controlled_vars::zint32_t   f_session_random;
        QString                     f_plugin_owner;
        QString                     f_page_path;
        QString                     f_object_path; // exact path to user, form, etc.
        QString                     f_user_agent;
        time_to_live_t              f_time_to_live;
        ztime_t                     f_time_limit;
        ztime_t                     f_login_limit;
        controlled_vars::zint64_t   f_date;
    };

                            sessions();
                            ~sessions();

    static sessions *       instance();
    virtual QString         description() const;
    virtual int64_t         do_update(int64_t last_updated);

    void                    on_bootstrap(snap_child *snap);
    virtual void            on_generate_main_content(layout::layout *l, content::path_info_t& path, QDomElement& page, QDomElement& body, QString const& ctemplate);

    QString                 create_session(session_info& info);
    void                    save_session(session_info& info, bool new_random);
    void                    load_session(QString const& session_id, session_info& info, bool use_once = true);

    void                    attach_to_session(session_info const& info, QString const& name, QString const& data);
    QString                 detach_from_session(session_info const& info, QString const& name);
    QString                 get_from_session(session_info const& info, QString const& name);

    SNAP_SIGNAL(generate_sessions, (sessions *r), (r));

private:
    void content_update(int64_t variables_timestamp);

    QtCassandra::QCassandraTable::pointer_t get_sessions_table();

    zpsnap_child_t          f_snap;
};

} // namespace sessions
} // namespace snap
// vim: ts=4 sw=4 et
