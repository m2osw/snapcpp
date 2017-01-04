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


users::user_info_t::user_info_t()
{
}


users::user_info_t::user_info_t( snap_child * sc )
    : f_snap(sc)
{
}


users::user_info_t::user_info_t( snap_child * sc, QString const & val )
    : f_snap(sc)
{
    f_identifier = get_user_id_by_path( get_snap(), val );
    if( f_identifier == -1 )
    {
        f_user_email = val;
        email_to_user_key();
        set_user_id_by_email();
        if( f_identifier != -1 )
        {
            set_value(name_t::SNAP_NAME_USERS_ORIGINAL_EMAIL, f_user_email);
            set_value(name_t::SNAP_NAME_USERS_CURRENT_EMAIL,  f_user_key);
        }
    }
    else
    {
        f_user_email = get_value(name_t::SNAP_NAME_USERS_ORIGINAL_EMAIL).stringValue();
        f_user_key	 = get_value(name_t::SNAP_NAME_USERS_CURRENT_EMAIL) .stringValue();
    }
}


users::user_info_t::user_info_t( snap_child * sc, name_t const & name )
    : f_snap(sc)
{
    f_user_email = f_user_key = get_name(name);
    set_user_id_by_email();
}


users::user_info_t::user_info_t( snap_child * sc, identifier_t const & id )
    : f_snap(sc)
    , f_identifier(id)
{
    f_user_email = get_value( name_t::SNAP_NAME_USERS_ORIGINAL_EMAIL ).stringValue();
    f_user_key	 = get_value( name_t::SNAP_NAME_USERS_CURRENT_EMAIL  ).stringValue();
}


users::identifier_t users::user_info_t::get_user_id_by_path( snap_child* snap, QString const& user_path )
{
    QString const site_key(snap->get_site_key_with_slash());
    int pos(0);
    if(user_path.startsWith(site_key))
    {
        // "remove" the site key, including the slash
        pos = site_key.length();
    }
    if(user_path.mid(pos, 5) == "user/")
    {
        QString const identifier_string(user_path.mid(pos + 5));
        bool ok(false);
        int64_t identifier(identifier_string.toLongLong(&ok, 10));
        if(ok)
        {
            return identifier;
        }
    }

    return -1;
}


QString users::user_info_t::get_full_anonymous_path()
{
    return QString("/%1/").arg(get_name(name_t::SNAP_NAME_USERS_ANONYMOUS_PATH));
}


void users::user_info_t::change_user_email( QString const & new_user_email )
{
    // Rotate the backups to the new
    QString const email_backup_base( get_name(name_t::SNAP_NAME_USERS_BACKUP_EMAIL_BASE) );
    for( int i = MAX_EMAIL_BACKUPS-1; i > 1; --i )
    {
        QString const prev_name( QString("%1_%2").arg(email_backup_base).arg(i-1) );
        QString const new_name ( QString("%1_%2").arg(email_backup_base).arg(i)   );
        auto prev_value( get_value(prev_name) );
        set_value( new_name, prev_value.binaryValue() );
    }

    set_value( name_t::SNAP_NAME_USERS_CURRENT_EMAIL, new_user_email );
}


void users::user_info_t::set_user_id_by_email()
{
    auto users_table( get_snap()->get_table(get_name(name_t::SNAP_NAME_USERS_TABLE)) );
    auto row        ( users_table->row(get_name(name_t::SNAP_NAME_USERS_INDEX_ROW))  );

    QByteArray key;
    QtCassandra::appendStringValue(key, f_user_key);
    if(row->exists(key))
    {
        // found the user, retrieve the current id
        f_identifier = row->cell(key)->value().int64Value();
    }
}


/** \brief Check whether the specified user is marked as being an example.
 *
 * You may call this function to determine whether a user is marked as
 * an example. This happens whenever a user is created with an example
 * email address such as john@example.com.
 *
 * The function expects the email address of the user. It first canonicolize
 * the email and then checks in the database to see whether the user is
 * considered an example or not.
 *
 * We use the database instead of parsing the email so really any user
 * can be marked as an example user.
 *
 * \note
 * If the specified email does not represent a registered user, then the
 * function always returns false.
 *
 * \param[in] email  The email address of the user to check.
 *
 * \return true if the user is marked as being an example.
 */
bool users::user_info_t::user_is_an_example_from_email() const
{
    if( !exists() )
    {
        return false;
    }
    return get_value(name_t::SNAP_NAME_USERS_EXAMPLE).safeSignedCharValue() != 0;
}


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
users::identifier_t users::user_info_t::get_identifier() const
{
    return f_identifier;
}

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
    f_identifier = v;
    set_value( name_t::SNAP_NAME_USERS_IDENTIFIER, v );
}


bool users::user_info_t::value_exists( QString const & v ) const
{
    return get_user_row()->exists(v);
}


bool users::user_info_t::value_exists( name_t  const & v ) const
{
    return value_exists(get_name(v));
}


users::user_info_t::cell_t users::user_info_t::get_cell( QString const & name ) const
{
    return get_user_row()->cell(name);
}


users::user_info_t::cell_t users::user_info_t::get_cell( name_t const & name ) const
{
    return get_cell(get_name(name));
}


users::user_info_t::value_t users::user_info_t::get_value( QString const & name ) const
{
    return get_cell(name)->value();
}


users::user_info_t::value_t users::user_info_t::get_value( name_t const & name ) const
{
    return get_value(get_name(name));
}


void users::user_info_t::set_value( QString const & name, value_t const & value )
{
    get_cell(name)->setValue(value);
}


void users::user_info_t::set_value( name_t const & name, value_t const & value )
{
    set_value( get_name(name), value );
}


void users::user_info_t::delete_value( QString const & name )
{
    get_user_row()->dropCell(name);
}


void users::user_info_t::delete_value( name_t const & name )
{
    delete_value( get_name(name) );
}


void users::user_info_t::set_status( status_t const & v )
{
    f_status = v;
}


QString const & users::user_info_t::get_user_key() const
{
    return f_user_key;
}


QString const & users::user_info_t::get_user_email() const
{
    return f_user_email;
}


void users::user_info_t::set_user_email ( QString const & val )
{
    f_user_email = val;
    email_to_user_key();
    set_value( name_t::SNAP_NAME_USERS_ORIGINAL_EMAIL, f_user_email );
    set_value( name_t::SNAP_NAME_USERS_CURRENT_EMAIL , f_user_key   );
}


/** \brief Get the path to a user from an email.
 *
 * This function returns the path of the user corresponding to the specified
 * email. The function returns an the ANONYMOUS path if the user is not found.
 *
 * \param[in] email  The email of the user to search the path for.
 *
 * \return The path to the user.
 */
QString users::user_info_t::get_user_path() const
{
    if( exists() )
    {
        return get_user_basepath( false /*front_slash*/ );
    }

    // TODO: should this be an empty string?
    //
    return get_name(name_t::SNAP_NAME_USERS_ANONYMOUS_PATH);
}


QString users::user_info_t::get_user_basepath( bool const front_slash ) const
{
    return QString("%1%2/%3")
            .arg(front_slash? "/": "")
            .arg(get_name(name_t::SNAP_NAME_USERS_PATH))
            .arg(get_identifier())
            ;
}


users::status_t users::user_info_t::get_status() const
{
    return f_status;
}


bool users::user_info_t::is_valid() const
{
    return f_snap && (f_identifier != -1);
}


bool users::user_info_t::exists() const
{
    auto users_table(get_snap()->get_table(get_name(name_t::SNAP_NAME_USERS_TABLE)));
    return users_table->exists(QtCassandra::QCassandraValue(f_identifier).binaryValue());
}


void users::user_info_t::reset()
{
    f_user_key     = QString();
    f_user_email   = QString();
    f_status       = status_t::STATUS_UNDEFINED;
    f_identifier   = -1;
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
void users::user_info_t::save_user_parameter(QString const & field_name, QtCassandra::QCassandraValue const & value)
{
    int64_t const start_date(get_snap()->get_start_date());

    // mark when we created the user if that is not yet defined
    if( !value_exists(name_t::SNAP_NAME_USERS_CREATED_TIME) )
    {
        set_value( name_t::SNAP_NAME_USERS_CREATED_TIME, start_date );
    }

    // save the external plugin parameter
    set_value( field_name, value );

    // mark the user as modified
    set_value( name_t::SNAP_NAME_USERS_MODIFIED, start_date );
}


void users::user_info_t::save_user_parameter(QString const & field_name, QString const & value)
{
    QtCassandra::QCassandraValue v(value);
    save_user_parameter(field_name, v);
}


void users::user_info_t::save_user_parameter(QString const & field_name, int64_t const & value)
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

    // make sure that row (a.k.a. user) exists before accessing it
    if( !exists() )
    {
        return false;
    }

    // row exists, make sure the user field exists
    if( !value_exists(field_name) )
    {
        return false;
    }

    // retrieve that parameter
    value = get_value(field_name);

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
    return users_table->row(QtCassandra::QCassandraValue(f_identifier).binaryValue());
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
