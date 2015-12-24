// Snap Websites Server -- verify passwords of all the parts used by snap
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

#include "./password.h"

#include "../permissions/permissions.h"

#include "log.h"
#include "not_reached.h"
#include "not_used.h"

#include <algorithm>
#include <iostream>

#include <openssl/rand.h>

#include <QChar>

#include "poison.h"


SNAP_PLUGIN_START(password, 1, 0)



namespace
{

// random bytes generator
//
class random_generator
{
public:
    static const int    RANDOM_BUFFER_SIZE = 256;

    unsigned char get_byte()
    {
        if(f_pos >= RANDOM_BUFFER_SIZE)
        {
            f_pos = 0;
        }

        if(f_pos == 0)
        {
            // get the random bytes
            RAND_bytes(f_buf, RANDOM_BUFFER_SIZE);
        }

        ++f_pos;
        return f_buf[f_pos - 1];
    }

private:
    unsigned char       f_buf[RANDOM_BUFFER_SIZE];
    size_t              f_pos = 0;
};


} // no name namespace



/* \brief Get a fixed password name.
 *
 * The password plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_PASSWORD_MINIMUM_DIGITS:
        return "password::minimum_digits";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_LENGTH:
        return "password::minimum_length";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_LETTERS:
        return "password::minimum_letters";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_LOWERCASE_LETTERS:
        return "password::minimum_lowercase_letters";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_SPACES:
        return "password::minimum_spaces";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_SPECIAL:
        return "password::minimum_special";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_UNICODE:
        return "password::minimum_unicode";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_UPPERCASE_LETTERS:
        return "password::minimum_uppercase_letters";

    case name_t::SNAP_NAME_PASSWORD_CHECK_BLACKLIST:
        return "password::check_blacklist";

    case name_t::SNAP_NAME_PASSWORD_TABLE:
        return "password";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_PASSWORD_...");

    }
    NOTREACHED();
}









/** \brief Initialize the password plugin.
 *
 * This function is used to initialize the password plugin object.
 */
password::password()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the password plugin.
 *
 * Ensure the password object is clean before it is gone.
 */
password::~password()
{
}


/** \brief Get a pointer to the password plugin.
 *
 * This function returns an instance pointer to the password plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the password plugin.
 */
password * password::instance()
{
    return g_plugin_password_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString password::settings_path() const
{
    return "/admin/settings/password";
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString password::description() const
{
    return "Check passwords of newly created users for strength."
          " The plugin verifies various settings to ensure the strength of passwords."
          " It can also check a database of black listed passwords.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString password::dependencies() const
{
    return "|permissions|users|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t password::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2015, 12, 23, 16, 56, 51, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief First update to run for the list plugin.
 *
 * This function is the first update for the list plugin. It creates
 * the list and listref tables.
 *
 * \note
 * We reset the cached pointer to the tables to make sure that they get
 * synchronized when used for the first time (very first initialization
 * only, do_update() is not generally called anyway, unless you are a
 * developer with the debug mode turned on.)
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void password::initial_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    get_password_table();
    f_password_table.reset();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void password::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the password.
 *
 * This function terminates the initialization of the password plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void password::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(password, "editor", editor::editor, prepare_editor_form, _1);
    SNAP_LISTEN(password, "users", users::users, check_user_security, _1);
}


/** \brief Add the locale widget to the editor XSLT.
 *
 * The editor is extended by the locale plugin by adding a time zone
 * and other various widgets.
 *
 * \param[in] e  A pointer to the editor plugin.
 */
void password::on_prepare_editor_form(editor::editor * e)
{
    e->add_editor_widget_templates_from_file(":/xsl/password_widgets/password-form.xsl");
}


/** \brief Initialize the password table.
 *
 * This function creates the password table if it does not exist yet.
 * Otherwise it simple initializes the f_password_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The password table is used to record passwords that get blacklisted.
 * All of those are exclusively coming from the backend. There is
 * no interface on the website to add invalid password to avoid any
 * problems.
 *
 * \return The pointer to the list table.
 */
QtCassandra::QCassandraTable::pointer_t password::get_password_table()
{
    if(!f_password_table)
    {
        f_password_table = f_snap->create_table(get_name(name_t::SNAP_NAME_PASSWORD_TABLE), "Website password table.");
    }
    return f_password_table;
}


/** \brief Check a password of a user.
 *
 * This function checks the user password for strength and against a
 * blacklist.
 *
 * \note
 * The password may be set to "!" in which case it gets ignored. This
 * is because "!" cannot be valid as the editor will enforce a length
 * of at least 8 characters (10 by default) and thus "!" cannot in
 * any way represent a password entered by the end user.
 *
 * \param[in] user_key  The key to a user (i.e. canonicalized email).
 * \param[in] email  The original user email address.
 * \param[in] password  The password we want to check.
 * \param[in] policy  The name of the policy used to check this user's password.
 * \param[in] bypass_blacklist  Whether the email blacklist should be bypassed.
 * \param[in,out] secure  Whether the password / user is considered secure.
 */
void password::on_check_user_security(users::users::user_security_t & security)
{
    if(!security.get_secure().allowed()
    || !security.has_password())
    {
        return;
    }

    QString const reason(check_password_against_policy(security.get_password(), security.get_policy()));
    if(!reason.isEmpty())
    {
        SNAP_LOG_TRACE("password::on_check_user_security(): password was not accepted: ")(reason);
        security.get_secure().not_permitted(reason);
        security.set_status(users::users::status_t::STATUS_PASSWORD);
    }
}


/** \brief Check password against a specific policy.
 *
 * This function is used to calculate the strength of a password depending
 * on a policy.
 *
 * \param[in] user_password  The password being checked.
 * \param[in] policy  The policy used to verify the password strength.
 *
 * \return A string with some form of error message about the password
 *         weakness(es) or an empty string if the password is okay.
 */
QString password::check_password_against_policy(QString const & user_password, QString const & policy)
{
    policy_t const pp(policy);

    policy_t up;
    up.count_password_characters(user_password);

    QString const too_small(up.compare(pp));
    if(!too_small.isEmpty())
    {
        return too_small;
    }

    return pp.is_blacklisted(user_password);
}


/** \brief Create a default password.
 *
 * In some cases an administrator may want to create an account for a user
 * which should then have a valid, albeit unknown, password.
 *
 * This function can be used to create that password.
 *
 * It is strongly advised to NOT send such passwords to the user via email
 * because they will contain all sorts of "strange" characters and emails
 * are notoriously not safe.
 *
 * The password will be at least 64 characters, more if the policy
 * requires more. The type of characters is also defined by the
 * policy and quite shuffled before the function returns.
 *
 * \param[in] policy  Create password that is valid for this policy.
 *
 * \return The string with the new password.
 */
QString password::create_password(QString const & policy)
{
    // to create a password that validates against a certain policy
    // we have to make sure that we have all the criterias covered
    // so we need to have the policy information and generate the
    // password as expected
    //
    policy_t pp(policy);

    random_generator gen;

    QString result;

    // to generate characters of each given type, we loop through
    // each set and then we randomize the final string
    //
    int64_t const minimum_lowercase_letters(pp.get_minimum_lowercase_letters());
    if(minimum_lowercase_letters > 0)
    {
        for(int64_t idx(0); idx < minimum_lowercase_letters; ++idx)
        {
            // lower case letters are between 'a' and 'z'
            ushort const c(gen.get_byte() % 26 + 'a');
            result += QChar(c);
        }
    }

    int64_t const minimum_uppercase_letters(pp.get_minimum_uppercase_letters());
    if(minimum_uppercase_letters > 0)
    {
        for(int64_t idx(0); idx < minimum_uppercase_letters; ++idx)
        {
            // lower case letters are between 'A' and 'Z'
            ushort const c(gen.get_byte() % 26 + 'A');
            result += QChar(c);
        }
    }

    int64_t const minimum_letters(pp.get_minimum_letters());
    if(minimum_letters > minimum_lowercase_letters + minimum_uppercase_letters)
    {
        for(int64_t idx(minimum_lowercase_letters + minimum_uppercase_letters); idx < minimum_uppercase_letters; ++idx)
        {
            // letters are between 'A' and 'Z' or 'a' and 'z'
            ushort c(gen.get_byte() % (26 * 2) + 'A');
            if(c > 'Z')
            {
                c += 'a' - 'Z' - 1;
            }
            result += QChar(c);
        }
    }

    int64_t const minimum_digits(pp.get_minimum_digits());
    if(minimum_digits > 0)
    {
        for(int64_t idx(0); idx < minimum_digits; ++idx)
        {
            // digits are between '0' and '9'
            int byte(gen.get_byte());
            ushort const c1(byte % 10 + '0');
            result += QChar(c1);
            if(idx + 1 < minimum_digits)
            {
                ++idx;
                ushort const c2(byte / 10 % 10 + '0');
                result += QChar(c2);
            }
        }
    }

    int64_t const minimum_spaces(pp.get_minimum_spaces());
    if(minimum_spaces > 0)
    {
        for(int64_t idx(0); idx < minimum_spaces; ++idx)
        {
            // TBD: should we support all the different types of
            //      spaces instead?
            ushort const c(' ');
            result += QChar(c);
        }
    }

    int64_t const minimum_special(pp.get_minimum_special());
    if(minimum_special > minimum_spaces)
    {
        for(int64_t idx(minimum_spaces); idx < minimum_special; )
        {
            ushort const byte(gen.get_byte());
            QChar const c(byte);
            switch(c.category())
            {
            case QChar::Letter_Lowercase:
            case QChar::Letter_Other:
            case QChar::Letter_Uppercase:
            case QChar::Letter_Titlecase:
            case QChar::Number_DecimalDigit:
            case QChar::Number_Letter:
            case QChar::Number_Other:
            case QChar::Mark_SpacingCombining:
            case QChar::Separator_Space:
            case QChar::Separator_Line:
            case QChar::Separator_Paragraph:
                break;

            default:
                result += c;
                ++idx;
                break;

            }
        }
    }

    int64_t const minimum_unicode(pp.get_minimum_unicode());
    if(minimum_unicode > 0)
    {
        for(int64_t idx(0); idx < minimum_unicode; )
        {
            // Unicode are characters over 0x0100, although
            // we avoid surrogates because they are more complicated
            // to handle and not as many characters are assigned in
            // those pages
            //
            ushort const s((gen.get_byte() << 8) | gen.get_byte());
            if(s >= 0x0100 && (s < 0xD800 || s > 0xDFFF))
            {
                QChar const c(s);
                if(c.unicodeVersion() != QChar::Unicode_Unassigned)
                {
                    // only keep assigned (known) unicode characters
                    //
                    // TODO: 
                    //
                    result += c;
                    ++idx;
                }
            }
        }
    }

    // we want a minimum of 64 character long passwords at this point
    //
    for(int64_t const minimum_length(std::max(pp.get_minimum_length(), static_cast<int64_t>(64))); result.length() < minimum_length; )
    {
        // include some other characters from the ASCII range to reach
        // the minimum length of the policy
        //
        ushort const byte(gen.get_byte() % (0x7E - 0x20 + 1) + 0x20);
        result += QChar(byte);
    }

    // shuffle all the characters once so that way it does not appear
    // in the order it was created above
    //
    for(int j(0); j < result.length(); ++j)
    {
        int i(0);
        if(result.length() < 256)
        {
            i = gen.get_byte() % result.length();
        }
        else
        {
            i = ((gen.get_byte() << 8) | gen.get_byte()) % result.length();
        }
        QChar c(result[i]);
        result[i] = result[j];
        result[j] = c;
    }

    // make sure that it worked as expected
    //
    QString const reason(check_password_against_policy(result, policy));
    if(!reason.isEmpty())
    {
        throw snap_logic_exception("somehow we generated a password that did not match the policy we were working against...");
    }

    return result;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
