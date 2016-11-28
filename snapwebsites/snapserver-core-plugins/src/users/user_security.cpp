// Snap Websites Server -- users security handling
// Copyright (C) 2012-2016  Made to Order Software Corp.
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

/** \file
 * \brief Users security check structure handling.
 *
 * This file is the implementation of the user_security_t class used
 * to check whether a user is considered valid before registering him
 * or sending an email to him.
 */

#include "users.h"

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(users)


void users::user_security_t::set_user_key(QString const & user_key)
{
    f_user_key = user_key;
}


void users::user_security_t::set_email(QString const & email, bool allow_example_domain)
{
    f_email = email;
    f_allow_example_domain = allow_example_domain;
}


void users::user_security_t::set_password(QString const & password)
{
    f_password = password;
}


void users::user_security_t::set_policy(QString const & policy)
{
    f_policy = policy;
}


void users::user_security_t::set_bypass_blacklist(bool const bypass)
{
    f_bypass_blacklist = bypass;
}


void users::user_security_t::set_example(bool const example)
{
    f_example = example;
}


void users::user_security_t::set_status(status_t status)
{
    // we can only change the status once from valid to something else
    //
    if(f_status == status_t::STATUS_VALID)
    {
        f_status = status;
    }
}


bool users::user_security_t::has_password() const
{
    return f_password != "!";
}


QString const & users::user_security_t::get_user_key() const
{
    return f_user_key;
}


QString const & users::user_security_t::get_email() const
{
    return f_email;
}


QString const & users::user_security_t::get_password() const
{
    return f_password;
}


QString const & users::user_security_t::get_policy() const
{
    return f_policy;
}


bool users::user_security_t::get_bypass_blacklist() const
{
    return f_bypass_blacklist;
}


bool users::user_security_t::get_allow_example_domain() const
{
    return f_allow_example_domain;
}


bool users::user_security_t::get_example() const
{
    return f_example;
}


content::permission_flag &  users::user_security_t::get_secure()
{
    return f_secure;
}


users::users::status_t users::user_security_t::get_status() const
{
    return f_status;
}



SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
