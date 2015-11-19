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

#include "../users/users.h"

namespace snap
{
namespace password
{


enum class name_t
{
    SNAP_NAME_PASSWORD_CHECK_BLACKLIST,
    SNAP_NAME_PASSWORD_MINIMUM_DIGITS,
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
    password_exception(char const *        what_msg) : snap_exception("versions", what_msg) {}
    password_exception(std::string const & what_msg) : snap_exception("versions", what_msg) {}
    password_exception(QString const &     what_msg) : snap_exception("versions", what_msg) {}
};

class password_exception_invalid_content_xml : public password_exception
{
public:
    password_exception_invalid_content_xml(char const *        what_msg) : password_exception(what_msg) {}
    password_exception_invalid_content_xml(std::string const & what_msg) : password_exception(what_msg) {}
    password_exception_invalid_content_xml(QString const &     what_msg) : password_exception(what_msg) {}
};







class password : public plugins::plugin
{
public:
                        password();
                        ~password();

    // plugins::plugin implementation
    static password *   instance();
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // users signals
    void                on_check_user_security(QString const & user_key, QString const & email, QString const & user_password, bool const bypass_blacklist, content::permission_flag & secure);

    QtCassandra::QCassandraTable::pointer_t get_password_table();

private:
    void                initial_update(int64_t variables_timestamp);
    void                content_update(int64_t variables_timestamp);

    zpsnap_child_t                          f_snap;
    QtCassandra::QCassandraTable::pointer_t f_password_table;
};


} // namespace versions
} // namespace snap
// vim: ts=4 sw=4 et
