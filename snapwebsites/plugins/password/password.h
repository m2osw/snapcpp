// Snap Websites Server -- check password strength
// Copyright (C) 2011-2015  Made to Order Software Corp.
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

#include "../editor/editor.h"

namespace snap
{
namespace password
{


enum class name_t
{
    SNAP_NAME_PASSWORD_CHECK_BLACKLIST,
    SNAP_NAME_PASSWORD_MINIMUM_DIGITS,
    SNAP_NAME_PASSWORD_MINIMUM_LENGTH,
    SNAP_NAME_PASSWORD_MINIMUM_LETTERS,
    SNAP_NAME_PASSWORD_MINIMUM_LOWERCASE_LETTERS,
    SNAP_NAME_PASSWORD_MINIMUM_SPACES,
    SNAP_NAME_PASSWORD_MINIMUM_SPECIAL,
    SNAP_NAME_PASSWORD_MINIMUM_UNICODE,
    SNAP_NAME_PASSWORD_MINIMUM_UPPERCASE_LETTERS,
    SNAP_NAME_PASSWORD_TABLE
};
char const * get_name(name_t name) __attribute__ ((const));


class password_exception : public snap_exception
{
public:
    explicit password_exception(char const *        what_msg) : snap_exception("versions", what_msg) {}
    explicit password_exception(std::string const & what_msg) : snap_exception("versions", what_msg) {}
    explicit password_exception(QString const &     what_msg) : snap_exception("versions", what_msg) {}
};

class password_exception_invalid_content_xml : public password_exception
{
public:
    explicit password_exception_invalid_content_xml(char const *        what_msg) : password_exception(what_msg) {}
    explicit password_exception_invalid_content_xml(std::string const & what_msg) : password_exception(what_msg) {}
    explicit password_exception_invalid_content_xml(QString const &     what_msg) : password_exception(what_msg) {}
};





class policy_t
{
public:
    explicit        policy_t(QString const & policy_name = QString());

    void            count_password_characters(QString const & password);

    int64_t         get_minimum_length() const;
    int64_t         get_minimum_lowercase_letters() const;
    int64_t         get_minimum_uppercase_letters() const;
    int64_t         get_minimum_letters() const;
    int64_t         get_minimum_digits() const;
    int64_t         get_minimum_spaces() const;
    int64_t         get_minimum_special() const;
    int64_t         get_minimum_unicode() const;
    bool            get_check_blacklist() const;

    QString         compare(policy_t const & rhs) const;
    QString         is_blacklisted(QString const & user_password) const;

private:
    int64_t         f_minimum_length = 0;
    int64_t         f_minimum_lowercase_letters = 0;
    int64_t         f_minimum_uppercase_letters = 0;
    int64_t         f_minimum_letters = 0;
    int64_t         f_minimum_digits = 0;
    int64_t         f_minimum_spaces = 0;
    int64_t         f_minimum_special = 0;
    int64_t         f_minimum_unicode = 0;
    bool            f_check_blacklist = false;
};




class password : public plugins::plugin
{
public:
                        password();
                        ~password();

    // plugins::plugin implementation
    static password *   instance();
    virtual QString     settings_path() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // users signals
    void                on_check_user_security(users::users::user_security_t & security);

    // editor signals
    void                on_prepare_editor_form(editor::editor * e);

    QtCassandra::QCassandraTable::pointer_t get_password_table();
    QString             check_password_against_policy(QString const & user_password, QString const & policy);
    QString             create_password(QString const & policy = "users");

private:
    void                initial_update(int64_t variables_timestamp);
    void                content_update(int64_t variables_timestamp);

    zpsnap_child_t                          f_snap;
    QtCassandra::QCassandraTable::pointer_t f_password_table;
};


} // namespace versions
} // namespace snap
// vim: ts=4 sw=4 et
