// Snap Websites Server -- users handling
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

#include "users.h"

#include "../output/output.h"
#include "../list/list.h"
#include "../locale/snap_locale.h"
#include "../messages/messages.h"
#include "../server_access/server_access.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qdomhelpers.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/snap_lock.h>

#include <iostream>

#include <QFile>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(users)


users::user_info_t::user_info_t( snap_child * sc )
    : f_snap(sc)
{
}

#if 0
/** \brief Save the user identifier.
 *
 * This function is used to save the user identifier in this object.
 * The identifier is a number which was assigned to the user when
 * he created his account.
 *
 * \param[in] identifier  The identifier of the user.
 *
 * \sa get_identifier()
 */
void users::user_info_t::set_identifier( identifier_t const & v )
{
    f_user_id = v;
}
#endif


void users::user_info_t::set_user_email( QString  const & v )
{
    f_user_email = v;
    email_to_user_key();
}


void users::user_info_t::set_user_name( name_t v )
{
    f_user_email = f_user_key = get_name(v);
}


void users::user_info_t::set_user_name( QString const & v )
{
    f_user_email = f_user_key = v;
}


#if 0
void users::user_info_t::set_user_path( QString  const & v )
{
    f_user_path = v;
}
#endif


void users::user_info_t::set_status( status_t const & v )
{
    f_status = v;
}


void users::user_info_t::set_is_valid( bool v )
{
    f_valid = v;
}


#if 0
/** \brief Get the current user identifer.
 *
 * This function gets the user identifier. If we do not have the user key
 * (his email address) then the function returns 0 (i.e. anonymous user).
 *
 * \warning
 * The identifier returned may NOT be from a logged in user. We may know the
 * user key (his email address) and yet not have a logged in user. Whether
 * the user is logged in needs to be checked with one of the
 * user_is_logged_in() or user_has_administrative_rights() functions.
 *
 * \return The identifer of the current user.
 */
users::identifier_t   const & users::user_info_t::get_identifier() const
{
    if(!f_user_key.isEmpty())
    {
        QtCassandra::QCassandraTable::pointer_t users_table(const_cast<users *>(this)->get_users_table());
        if(users_table->exists(f_user_key))
        {
            QtCassandra::QCassandraValue const value(users_table->row(f_user_key)->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
            if(!value.nullValue())
            {
                return value.int64Value();
            }
        }
    }
    return 0;
}
#endif


QString const & users::user_info_t::get_user_key() const
{
    return f_user_key;
}


QString const & users::user_info_t::get_user_email() const
{
    return f_user_email;
}


#if 0
/** \brief Get the path to a user from an email.
 *
 * This function returns the path of the user corresponding to the specified
 * email. The function returns an empty string if the user is not found.
 *
 * \param[in] email  The email of the user to search the path for.
 *
 * \return The path to the user.
 */
QString const & users::user_info_t::get_user_path() const
{
    if( f_user_key.isEmpty() )
    {
        throw users_exception("You must set the user email first before calling users::user_info_t::get_user_path()!");
    }

    if( exists() )
    {
        QtCassandra::QCassandraRow::pointer_t row(get_user_row());
        QtCassandra::QCassandraValue const value(row->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!value.nullValue())
        {
            int64_t const identifier(value.int64Value());
            return QString("%1/%2").arg(get_name(name_t::SNAP_NAME_USERS_PATH)).arg(identifier);
        }
    }

    return get_name(name_t::SNAP_NAME_USERS_ANONYMOUS_PATH);
}
#endif


users::status_t users::user_info_t::get_status() const
{
    return f_status;
}


bool users::user_info_t::is_valid() const
{
    return f_valid;
}


bool users::user_info_t::exists() const
{
    auto users_table(get_snap()->get_table(get_name(name_t::SNAP_NAME_USERS_TABLE)));
    return users_table->exists(f_user_key);
}


void users::user_info_t::reset()
{
    f_valid      = false;
    f_user_key   = QString();
    f_user_email = QString();
    //f_user_path  = QString();
    f_status     = status_t::STATUS_UNDEFINED;
    f_valid      = false;
}


snap_child*  users::user_info_t::get_snap() const
{
    if( f_snap == nullptr )
    {
        throw users_exception("f_snap was not initialized!");
    }
    return f_snap;
}


/** \brief Canonicalize the user email to use in the "users" table.
 *
 * The "users" table defines each user by email address. The email address
 * is kept as is in the user account itself, but for us to access the
 * database, we have to have a canonicalized user email address.
 *
 * The domain name part (what appears after the AT (@) character) is
 * always made to lowercase. The username is also made lowercase by
 * default. However, a top notch geek website can offer its end
 * users to have lower and upper case usernames in their email
 * address. This is generally fine, although it means you may get
 * entries such as:
 *
 * \code
 *    me@snap.website
 *    Me@snap.website
 *    ME@snap.website
 *    mE@snap.website
 * \endcode
 *
 * and each one will be considered a different account. This can be
 * really frustrating for users who don't understand emails though.
 *
 * The default mode does not require any particular setup.
 * The "Unix" (or geek) mode requires that you set the
 * "users::force_lowercase" field in the sites table to 1.
 * To go back to the default, either set "users::force_lowercase"
 * to 0 or delete it.
 *
 * \note If you change the "users::force_lowercase" setting, you must restart the plugin
 *       due to the static value being preserved.
 *
 * \param[in] email  The email of the user.
 *
 * \return The user_key based on the email all or mostly in lowercase or not.
 *
 * \sa basic_email_canonicalization()
 */
void users::user_info_t::email_to_user_key()
{
    // It is better to use the new strongly typed enumerations.
    //
    enum class force_lowercase_t
    {
        UNDEFINED,
        YES,
        NO
    };

    static force_lowercase_t force_lowercase(force_lowercase_t::UNDEFINED);

    if(force_lowercase == force_lowercase_t::UNDEFINED)
    {
        QtCassandra::QCassandraValue const force_lowercase_parameter(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_FORCE_LOWERCASE)));
        if(force_lowercase_parameter.nullValue()
        || force_lowercase_parameter.safeSignedCharValue())
        {
            // this is the default if undefined
            force_lowercase = force_lowercase_t::YES;
        }
        else
        {
            force_lowercase = force_lowercase_t::NO;
        }
    }

    if(force_lowercase == force_lowercase_t::YES)
    {
        // in this case, it is easy we can force the entire email to lowercase
        f_user_key = f_user_email.toLower();
    }
    else
    {
        // if not forcing the username to lowercase, we still need to force
        // the domain name to lowercase
        //
        f_user_key = users::basic_email_canonicalization(f_user_email);
    }
}


/** \brief Save a user parameter.
 *
 * This function is used to save a field directly in the "users" table.
 * Whether the user is already a registered user does not matter, the
 * function accepts to save the parameter. This is particularly important
 * for people who want to register for a newsletter or unsubscribe from
 * the website as a whole (See the sendmail plugin).
 *
 * If a value with the same field name exists, it gets overwritten.
 *
 * \param[in] field_name  The name of the field where value gets saved.
 * \param[in] value  The value of the field to save.
 *
 * \sa load_user_parameter()
 */
void users::user_info_t::save_user_parameter(QString const & field_name, QtCassandra::QCassandraValue const & value) const
{
    int64_t const start_date(get_snap()->get_start_date());

    //QString const user_key(email_to_user_key(email));

    //auto users_table(get_snap()->get_table(get_name(name_t::SNAP_NAME_USERS_TABLE)));
    QtCassandra::QCassandraRow::pointer_t row(get_user_row()); //users_table->row(user_key));

    // mark when we created the user if that is not yet defined
    if(!row->exists(get_name(name_t::SNAP_NAME_USERS_CREATED_TIME)))
    {
        row->cell(get_name(name_t::SNAP_NAME_USERS_CREATED_TIME))->setValue(start_date);
    }

    // save the external plugin parameter
    row->cell(field_name)->setValue(value);

    // mark the user as modified
    row->cell(get_name(name_t::SNAP_NAME_USERS_MODIFIED))->setValue(start_date);
}


void users::user_info_t::save_user_parameter(QString const & field_name, QString const & value) const
{
    QtCassandra::QCassandraValue v(value);
    save_user_parameter(field_name, v);
}


void users::user_info_t::save_user_parameter(QString const & field_name, int64_t const & value) const
{
    QtCassandra::QCassandraValue v(value);
    save_user_parameter(field_name, v);
}


/** \brief Retrieve a user parameter.
 *
 * This function is used to read a field directly from the "users" table.
 * If the value exists, then the function returns true and the \p value
 * parameter is set to its content. If the field cannot be found, then
 * the function returns false.
 *
 * If your value cannot be an empty string, then just testing whether
 * value is the empty string on return is enough to know whether the
 * field was defined in the database.
 *
 * \param[in] field_name  The name of the field being checked.
 * \param[out] value  The value of the field, empty if undefined.
 *
 * \return true if the value was read from the database.
 *
 * \sa save_user_parameter()
 */
bool users::user_info_t::load_user_parameter(QString const & field_name, QtCassandra::QCassandraValue & value) const
{
    // reset the input value by default
    value.setNullValue();

    //QString const user_key(email_to_user_key(email));

    // make sure that row (a.k.a. user) exists before accessing it
    if( !exists() )
    {
        return false;
    }

#if 0
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(!users_table->exists(user_key))
    {
        return false;
    }
#endif
    //QtCassandra::QCassandraRow::pointer_t user_row(users_table->row(user_key));
    QtCassandra::QCassandraRow::pointer_t user_row(get_user_row());

    // row exists, make sure the user field exists
    if(!user_row->exists(field_name))
    {
        return false;
    }

    // retrieve that parameter
    value = user_row->cell(field_name)->value();

    return true;
}


bool users::user_info_t::load_user_parameter(QString const & field_name, QString & value) const
{
    QtCassandra::QCassandraValue v;
    if(load_user_parameter(field_name, v))
    {
        value = v.stringValue();
        return true;
    }
    return false;
}


bool users::user_info_t::load_user_parameter(QString const & field_name, int64_t & value) const
{
    QtCassandra::QCassandraValue v;
    if(load_user_parameter(field_name, v))
    {
        value = v.safeInt64Value();
        return true;
    }
    return false;
}


QtCassandra::QCassandraRow::pointer_t users::user_info_t::get_user_row() const
{
    auto users_table(get_snap()->get_table(get_name(name_t::SNAP_NAME_USERS_TABLE)));
    return users_table->row(f_user_key);    // TODO: change to uint64_t id
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
