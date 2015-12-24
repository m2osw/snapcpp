// Snap Websites Server -- password policy handling
// Copyright (C) 2012-2015  Made to Order Software Corp.
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

#include "password.h"

#include "poison.h"


SNAP_PLUGIN_EXTENSION_START(password)


/** \brief The policy to use with this object.
 *
 * The constructor loads the policy specified by name. If you do not
 * specify a policy name (i.e. use an emptry string, "") then the
 * initialization is not applied.
 *
 * \param[in] policy_name  The name of the policy to load.
 */
policy_t::policy_t(QString const & policy_name)
{
    if(!policy_name.isEmpty()
    && policy_name != "blacklist")
    {
        content::content * content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

        // load the policy from the database
        content::path_info_t settings_ipath;
        settings_ipath.set_path(QString("admin/settings/password/%1").arg(policy_name));
        QtCassandra::QCassandraRow::pointer_t settings_row(revision_table->row(settings_ipath.get_revision_key()));

        f_minimum_length            = settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_LENGTH))->value().safeInt64Value(0, 0);
        f_minimum_lowercase_letters = settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_LOWERCASE_LETTERS))->value().safeInt64Value(0, 0);
        f_minimum_uppercase_letters = settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_UPPERCASE_LETTERS))->value().safeInt64Value(0, 0);
        f_minimum_letters           = settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_LETTERS))->value().safeInt64Value(0, 0);
        f_minimum_digits            = settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_DIGITS))->value().safeInt64Value(0, 0);
        f_minimum_spaces            = settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_SPACES))->value().safeInt64Value(0, 0);
        f_minimum_special           = settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_SPECIAL))->value().safeInt64Value(0, 0);
        f_minimum_unicode           = settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_UNICODE))->value().safeInt64Value(0, 0);
        f_check_blacklist           = settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_CHECK_BLACKLIST))->value().safeSignedCharValue(0, 0);
    }
}


/** \brief Count the characters of a password.
 *
 * The policy structure is used to either load a policy (see constructor)
 * or to count the characters found in a user password (this function.)
 *
 * In order to use the policy_t class for a password count instead of
 * a policy loaded from the database, one calls this function.
 *
 * \param[in] user_password  The password the user entered.
 */
void policy_t::count_password_characters(QString const & user_password)
{
    // count the various types of characters
    f_minimum_length = user_password.length();

    for(QChar const * s(user_password.constData()); !s->isNull(); ++s)
    {
//SNAP_LOG_WARNING("character ")(static_cast<char>(s->unicode))(" -- category ")(static_cast<int>(s->category()));
        switch(s->category())
        {
        case QChar::Letter_Lowercase:
        case QChar::Letter_Other:
            ++f_minimum_letters;
            ++f_minimum_lowercase_letters;
            break;

        case QChar::Letter_Uppercase:
        case QChar::Letter_Titlecase:
            ++f_minimum_letters;
            ++f_minimum_uppercase_letters;
            break;

        case QChar::Number_DecimalDigit:
        case QChar::Number_Letter:
        case QChar::Number_Other:
            ++f_minimum_digits;
            break;

        case QChar::Mark_SpacingCombining:
        case QChar::Separator_Space:
        case QChar::Separator_Line:
        case QChar::Separator_Paragraph:
            ++f_minimum_spaces;
            ++f_minimum_special; // it is also considered special
            break;

        default:
            if(s->unicode() < 0x100)
            {
                ++f_minimum_special;
            }
            break;

        }

        if(s->unicode() >= 0x100)
        {
            ++f_minimum_unicode;
        }
    }
}


/** \brief The minimum number of characters.
 *
 * When loading the policy from the database, this is the
 * miminum number of characters that must exist in the
 * password, counting hidden characters like 0xFEFF.
 *
 * When counting the characters of a password, this is the
 * total number of characters found.
 *
 * \return The minimum number of characters in a password.
 */
int64_t policy_t::get_minimum_length() const
{
    return f_minimum_length;
}


/** \brief The minimum number of lowercase letters characters.
 *
 * When loading the policy from the database, this is the
 * number of lowercase letters characters that must exist in the
 * password.
 *
 * When counting the characters of a password, this is the
 * set of lowercase letters characters found.
 *
 * \return The minimum number of lowercase letters characters.
 */
int64_t policy_t::get_minimum_lowercase_letters() const
{
    return f_minimum_lowercase_letters;
}


/** \brief The minimum number of uppercase letters characters.
 *
 * When loading the policy from the database, this is the
 * number of uppercase letters characters that must exist in the
 * password.
 *
 * When counting the characters of a password, this is the
 * set of uppercase letters characters found.
 *
 * \return The minimum number of uppercase letters characters.
 */
int64_t policy_t::get_minimum_uppercase_letters() const
{
    return f_minimum_uppercase_letters;
}


/** \brief The minimum number of letters characters.
 *
 * When loading the policy from the database, this is the
 * number of letters characters that must exist in the
 * password.
 *
 * When counting the characters of a password, this is the
 * set of letters characters found.
 *
 * \note
 * Letters in this context is any Uncode character that
 * resolves as a letter, whether uppercase or lowercase.
 *
 * \return The minimum number of letters characters.
 */
int64_t policy_t::get_minimum_letters() const
{
    return f_minimum_letters;
}


/** \brief The minimum number of digits characters.
 *
 * When loading the policy from the database, this is the
 * number of digits characters that must exist in the
 * password.
 *
 * When counting the characters of a password, this is the
 * set of digits characters found.
 *
 * \note
 * Any character considered a digit by Unicode is counted
 * as such. So it does not need to be '0' to '9' from
 * the ASCII range (byte codes 0x30 to 0x39.)
 *
 * \return The minimum number of digits characters.
 */
int64_t policy_t::get_minimum_digits() const
{
    return f_minimum_digits;
}


/** \brief The minimum number of spaces characters.
 *
 * When loading the policy from the database, this is the
 * number of spaces characters that must exist in the
 * password.
 *
 * When counting the characters of a password, this is the
 * set of spaces characters found.
 *
 * \note
 * Any Unicode character viewed as a space is counted as
 * such. This is not limiteed to character 0x20.
 *
 * \return The minimum number of spaces characters.
 */
int64_t policy_t::get_minimum_spaces() const
{
    return f_minimum_spaces;
}


/** \brief The minimum number of special characters.
 *
 * When loading the policy from the database, this is the
 * number of special characters that must exist in the
 * password.
 *
 * When counting the characters of a password, this is the
 * set of special characters found.
 *
 * \return The minimum number of special characters.
 */
int64_t policy_t::get_minimum_special() const
{
    return f_minimum_special;
}


/** \brief The minimum number of unicode characters.
 *
 * When loading the policy from the database, this is the
 * number of Unicode characters that must exist in the
 * password.
 *
 * When counting the characters of a password, this is the
 * set of Unicode characters found (i.e. any character with
 * a code over 0x0100.)
 *
 * \return The minimum number of Unicode characters.
 */
int64_t policy_t::get_minimum_unicode() const
{
    return f_minimum_unicode;
}


/** \brief Check whether the blacklist should be looked up.
 *
 * This function returns true if the blacklist should be looked up
 * when a new password is being defined by a user. By default this
 * is false, although it certain is a good idea to check because
 * those lists are known by hackers and thus these passwords will
 * be checked against your Snap! websites, over and over again.
 *
 * \return Whether the blacklist should be checked.
 */
bool policy_t::get_check_blacklist() const
{
    return f_check_blacklist;
}


/** \brief Check whether a policy is smaller than the other.
 *
 * This function checks whether the left hand side (this) has
 * any of its minimum parameters which is smaller than the
 * right hand side (rhs) policy. If so, then the function
 * returns an error message in the string.
 *
 * If the left is larger or equal, then the function returns
 * an empty string.
 *
 * This is used to compare  password against a policy loaded
 * from the database.
 *
 * \code
 *      policy_t pp("protected-nodes");
 *
 *      policy_t up;
 *      up.count_password_characters(user_password);
 *
 *      QString const r(up.compare(pp));
 *      if(r.isEmpty())
 *      {
 *          // password characters have the expected mix!
 *      }
 *      else
 *      {
 *          // password strength too weak
 *          // "r" is the message about what is missing
 *      }
 * \endcode
 *
 * \param[in] rhs  The policy to check the password against.
 */
QString policy_t::compare(policy_t const & rhs) const
{
    // enough lowercase letters?
    if(f_minimum_lowercase_letters < rhs.f_minimum_lowercase_letters)
    {
        return "not enough lowercase letter characters";
    }

    // enough uppercase letters?
    if(f_minimum_uppercase_letters < rhs.f_minimum_uppercase_letters)
    {
        return "not enough uppercase letter characters";
    }

    // enough letters?
    if(f_minimum_letters < rhs.f_minimum_letters)
    {
        return "not enough letter characters";
    }

    // enough digits?
    if(f_minimum_digits < rhs.f_minimum_digits)
    {
        return "not enough digit characters";
    }

    // enough spaces?
    if(f_minimum_spaces < rhs.f_minimum_spaces)
    {
        return "not enough space characters";
    }

    // enough special?
    if(f_minimum_special < rhs.f_minimum_special)
    {
        return "not enough special characters";
    }

    // enough unicode?
    if(f_minimum_unicode < rhs.f_minimum_unicode)
    {
        return "not enough unicode characters";
    }

    // password is all good
    return QString();
}


/** \brief Check whether the user password is blacklisted.
 *
 * Our system maintains a list of words that we want to forbid
 * users from ever entering as passwords because they are known
 * by hackers and thus not useful as a security token.
 *
 * \todo
 * Later we may have degrees of blacklisted password, i.e. we may
 * still authorize some of those if they pass the policy rules.
 *
 * \param[in] user_password  The password to be checked.
 *
 * \return An error message if the password is blacklisted.
 */
QString policy_t::is_blacklisted(QString const & user_password) const
{
    // also check against the blacklist?
    //
    if(f_check_blacklist)
    {
        // the password has to be the row name to be spread on all nodes
        //
        // later we may use columns to define whether a password 100%
        // forbidden (password1,) "mostly" forbidden (complex enough
        // for the current policy,) etc.
        //
        QtCassandra::QCassandraTable::pointer_t table(password::password::instance()->get_password_table());
        if(table->exists(user_password.toLower()))
        {
            return "this password is blacklisted and cannot be used";
        }
    }

    // not black listed
    return QString();
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
