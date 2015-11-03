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

#include "not_reached.h"
#include "not_used.h"

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(password, 1, 0)


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

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_LETTERS:
        return "password::minimum_letters";

    case name_t::SNAP_NAME_PASSWORD_MINIMUM_LOWERCASE_LETTERS:
        return "password::minimum_uppercase_letters";

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


/** \brief Initialize the password.
 *
 * This function terminates the initialization of the password plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void password::on_bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(password, "users", users::users, check_user_security, _1, _2, _3, _4);
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
    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, content_update);

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
 * blacklist
 *
 * \param[in] user_key  The key to a user (i.e. canonicalized email).
 * \param[in] email  The original user email address.
 * \param[in] password  The password we want to check.
 * \param[in,out] secure  Whether the password / user is considered secure.
 */
void password::on_check_user_security(QString const & user_key, QString const & email, QString const & user_password, content::permission_flag & secure)
{
    NOTUSED(user_key);
    NOTUSED(email);

    if(!secure.allowed())
    {
        return;
    }

    // count the various types of characters
    int lowercase_letters(0);
    int uppercase_letters(0);
    int letters(0);
    int digits(0);
    int spaces(0);
    int special(0);
    int unicode(0);

    for(QChar const * s(user_password.constData()); s->isNull(); ++s)
    {
        switch(s->category())
        {
        case QChar::Letter_Lowercase:
        case QChar::Letter_Other:
            ++letters;
            ++lowercase_letters;
            break;

        case QChar::Letter_Uppercase:
        case QChar::Letter_Titlecase:
            ++letters;
            ++uppercase_letters;
            break;

        case QChar::Number_DecimalDigit:
        case QChar::Number_Letter:
        case QChar::Number_Other:
            ++digits;
            break;

        case QChar::Mark_SpacingCombining:
        case QChar::Separator_Space:
        case QChar::Separator_Line:
        case QChar::Separator_Paragraph:
            ++spaces;
            ++special; // it is also considered special
            break;

        default:
            if(s->unicode() < 0x100)
            {
                ++special;
            }
            break;

        }

        if(s->unicode() >= 0x100)
        {
            ++unicode;
        }
    }

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

    // then check that it is enough of each type
    content::path_info_t settings_ipath;
    settings_ipath.set_path("admin/settings/password");
    QtCassandra::QCassandraRow::pointer_t settings_row(revision_table->row(settings_ipath.get_revision_key()));

    // enough lowercase letters?
    int64_t const minimum_lowercase_letters(settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_LOWERCASE_LETTERS))->value().safeInt64Value(0, 0));
    if(lowercase_letters < minimum_lowercase_letters)
    {
        secure.not_permitted("not enough lowercase letter characters");
        return;
    }

    // enough uppercase letters?
    int64_t const minimum_uppercase_letters(settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_UPPERCASE_LETTERS))->value().safeInt64Value(0, 0));
    if(uppercase_letters < minimum_uppercase_letters)
    {
        secure.not_permitted("not enough uppercase letter characters");
        return;
    }

    // enough letters?
    int64_t const minimum_letters(settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_LETTERS))->value().safeInt64Value(0, 0));
    if(letters < minimum_letters)
    {
        secure.not_permitted("not enough letter characters");
        return;
    }

    // enough digits?
    int64_t const minimum_digits(settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_DIGITS))->value().safeInt64Value(0, 0));
    if(digits < minimum_digits)
    {
        secure.not_permitted("not enough digit characters");
        return;
    }

    // enough spaces?
    int64_t const minimum_spaces(settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_SPACES))->value().safeInt64Value(0, 0));
    if(spaces < minimum_spaces)
    {
        secure.not_permitted("not enough space characters");
        return;
    }

    // enough special?
    int64_t const minimum_special(settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_SPECIAL))->value().safeInt64Value(0, 0));
    if(special < minimum_special)
    {
        secure.not_permitted("not enough special characters");
        return;
    }

    // enough unicode?
    int64_t const minimum_unicode(settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_MINIMUM_UNICODE))->value().safeInt64Value(0, 0));
    if(unicode < minimum_unicode)
    {
        secure.not_permitted("not enough unicode characters");
        return;
    }

    // also check the blacklist?
    int8_t const check_blacklist(settings_row->cell(get_name(name_t::SNAP_NAME_PASSWORD_CHECK_BLACKLIST))->value().safeSignedCharValue(0, 0));
    if(check_blacklist)
    {
        // the password has to be the row name to be spread on all nodes
        //
        QtCassandra::QCassandraTable::pointer_t table(get_password_table());
        if(table->exists(user_password))
        {
            secure.not_permitted("this password is blacklisted and cannot be used");
            return;
        }
    }

    // password is all good
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
