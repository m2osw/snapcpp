// Snap Websites Server -- users handling
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
 * \brief Users handling.
 *
 * This plugin handles the users which includes:
 *
 * \li The log in screen.
 * \li The log out feature and thank you page.
 * \li The registration.
 * \li The verification of an email to register.
 * \li The request for a new password.
 * \li The verification of an email to change a forgotten password.
 *
 * It is also responsible for creating new user accounts, blocking accounts,
 * etc.
 */

#include "users.h"

#include "../output/output.h"
#include "../locale/snap_locale.h"
#include "../messages/messages.h"
#include "../sendmail/sendmail.h"
#include "../server_access/server_access.h"

#include "log.h"
#include "not_reached.h"
#include "not_used.h"
#include "qdomhelpers.h"
#include "qstring_stream.h"

#include <iostream>

#include <QtCassandra/QCassandraLock.h>
#include <QFile>

#include <openssl/evp.h>
#include <openssl/rand.h>

#include "poison.h"


SNAP_PLUGIN_START(users, 1, 0)


namespace
{

const int SALT_SIZE = 32;
// the salt size must be even
BOOST_STATIC_ASSERT((SALT_SIZE & 1) == 0);

const int COOKIE_NAME_SIZE = 12; // the real size is (COOKIE_NAME_SIZE / 3) * 4
// we want 3 bytes to generate 4 characters
BOOST_STATIC_ASSERT((COOKIE_NAME_SIZE % 3) == 0);

} // no name namespace


/** \brief Get a fixed users plugin name.
 *
 * The users plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char *get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_USERS_ANONYMOUS_PATH:
        return "user";

    case name_t::SNAP_NAME_USERS_AUTHOR:
        return "users::author";

    case name_t::SNAP_NAME_USERS_AUTHORED_PAGES:
        return "users::authored_pages";

    case name_t::SNAP_NAME_USERS_AUTO_PATH:
        return "types/users/auto";

    case name_t::SNAP_NAME_USERS_BLACK_LIST:
        return "*black_list*";

    case name_t::SNAP_NAME_USERS_BLOCKED_PATH:
        return "types/users/blocked";

    case name_t::SNAP_NAME_USERS_CHANGING_PASSWORD_KEY:
        return "users::changing_password_key";

    case name_t::SNAP_NAME_USERS_CREATED_TIME:
        return "users::created_time";

    case name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_EMAIL:
        return "users::forgot_password_email";

    case name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_IP:
        return "users::forgot_password_ip";

    case name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_ON:
        return "users::forgot_password_on";

    case name_t::SNAP_NAME_USERS_IDENTIFIER:
        return "users::identifier";

    case name_t::SNAP_NAME_USERS_ID_ROW:
        return "*id_row*";

    case name_t::SNAP_NAME_USERS_INDEX_ROW:
        return "*index_row*";

    case name_t::SNAP_NAME_USERS_LAST_VERIFICATION_SESSION:
        return "users::last_verification_session";

    case name_t::SNAP_NAME_USERS_LOCALE: // format locale for dates/numbers
        return "users::locale";

    case name_t::SNAP_NAME_USERS_LOCALES: // browser/page languages
        return "users::locales";

    case name_t::SNAP_NAME_USERS_LOGIN_IP:
        return "users::login_ip";

    case name_t::SNAP_NAME_USERS_LOGIN_ON:
        return "users::login_on";

    case name_t::SNAP_NAME_USERS_LOGIN_REFERRER:
        return "users::login_referrer";

    case name_t::SNAP_NAME_USERS_LOGIN_SESSION:
        return "users::login_session";

    case name_t::SNAP_NAME_USERS_LOGOUT_IP:
        return "users::logout_ip";

    case name_t::SNAP_NAME_USERS_LOGOUT_ON:
        return "users::logout_on";

    case name_t::SNAP_NAME_USERS_LONG_SESSIONS:
        return "users::long_sessions";

    case name_t::SNAP_NAME_USERS_MODIFIED:
        return "users::modified";

    case name_t::SNAP_NAME_USERS_MULTISESSIONS:
        return "users::multisessions";

    case name_t::SNAP_NAME_USERS_MULTIUSER:
        return "users::multiuser";

    case name_t::SNAP_NAME_USERS_NAME:
        return "users::name";

    case name_t::SNAP_NAME_USERS_NEW_PATH:
        return "types/users/new";

    case name_t::SNAP_NAME_USERS_NOT_MAIN_PAGE:
        return "users::not_main_page";

    case name_t::SNAP_NAME_USERS_ORIGINAL_EMAIL:
        return "users::original_email";

    case name_t::SNAP_NAME_USERS_ORIGINAL_IP:
        return "users::original_ip";

    case name_t::SNAP_NAME_USERS_PASSWORD:
        return "users::password";

    case name_t::SNAP_NAME_USERS_PASSWORD_DIGEST:
        return "users::password::digest";

    case name_t::SNAP_NAME_USERS_PASSWORD_PATH:
        return "types/users/password";

    case name_t::SNAP_NAME_USERS_PASSWORD_SALT:
        return "users::password::salt";

    case name_t::SNAP_NAME_USERS_PATH:
        return "user";

    case name_t::SNAP_NAME_USERS_PICTURE:
        return "users::picture";

    case name_t::SNAP_NAME_USERS_PREVIOUS_LOGIN_IP:
        return "users::previous_login_ip";

    case name_t::SNAP_NAME_USERS_PREVIOUS_LOGIN_ON:
        return "users::previous_login_on";

    // WARNING: We do not use a statically defined name!
    //          To be more secure each Snap! website can use a different
    //          cookie name; possibly one that changes over time and
    //          later by user...
    //case name_t::SNAP_NAME_USERS_SESSION_COOKIE:
    //    // cookie names cannot include ':' so I use "__" to represent
    //    // the namespace separation
    //    return "users__snap_session";

    case name_t::SNAP_NAME_USERS_STATUS:
        return "users::status";

    case name_t::SNAP_NAME_USERS_TABLE:
        return "users";

    case name_t::SNAP_NAME_USERS_TIMEZONE: // user timezone for dates/calendars
        return "users::timezone";

    case name_t::SNAP_NAME_USERS_USERNAME:
        return "users::username";

    case name_t::SNAP_NAME_USERS_VERIFIED_IP:
        return "users::verified_ip";

    case name_t::SNAP_NAME_USERS_VERIFIED_ON:
        return "users::verified_on";

    case name_t::SNAP_NAME_USERS_VERIFY_EMAIL:
        return "users::verify_email";

    case name_t::SNAP_NAME_USERS_WEBSITE_REFERENCE:
        return "users::website_reference";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_USERS_...");

    }
    NOTREACHED();
}


/** \class users
 * \brief The users plugin to handle user accounts.
 *
 * This class handles all the necessary user related pages:
 *
 * \li User log in
 * \li User registration
 * \li User registration token verification
 * \li User registration token re-generation
 * \li User forgotten password
 * \li User forgotten password token verification
 * \li User profile
 * \li User change of password
 * \li ...
 *
 * To enhance the security of the user session we randomly assign the name
 * of the user session cookie. This way robots have a harder time to
 * break-in since each Snap! website will have a different cookie name
 * to track users (and one website may change the name at any time.)

 * \todo
 * To make it even harder we should look into a way to use a cookie
 * that has a different name per user and changes name each time the
 * user logs in. This should be possible since the list of cookies is
 * easy to parse on the server side, then we can test each cookie for
 * valid snap data which have the corresponding snap cookie name too.
 * (i.e. the session would save the cookie name too!)
 *
 * \todo
 * Add a Secure Cookie which is only secure... and if not present
 * renders the logged in user quite less logged in (i.e. "returning
 * registered user".)
 */


/** \brief Initialize the users plugin.
 *
 * This function initializes the users plugin.
 */
users::users()
    //: f_snap(nullptr) -- auto-init
    //, f_user_key("") -- auto-init
    //, f_user_logged_in(false) -- auto-init
    //, f_user_changing_password_key("") -- auto-init
    //, f_info(nullptr) -- auto-init
{
}

/** \brief Destroy the users plugin.
 *
 * This function cleans up the users plugin.
 */
users::~users()
{
}


/** \brief Get a pointer to the users plugin.
 *
 * This function returns an instance pointer to the users plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the users plugin.
 */
users *users::instance()
{
    return g_plugin_users_factory.instance();
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
QString users::description() const
{
    return "The users plugin manages all the users on a website. It is also"
           " capable to create new users which is a Snap! wide feature.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last
 *                          updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t users::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2015, 10, 14, 16, 49, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief First update to run for the users plugin.
 *
 * This function is the first update for the users plugin. It creates
 * the users table.
 *
 * \note
 * We do not cache the users table pointer.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void users::initial_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    get_users_table();
}


/** \brief Update the users plugin content.
 *
 * This function updates the contents in the database using the
 * system update settings found in the resources.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void users::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);
    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the users table.
 *
 * This function creates the users table if it doesn't exist yet. Otherwise
 * it simple returns the existing Cassandra table.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The table is a list of emails (row keys) and passwords. Additional user
 * data is generally added by other plugins (i.e. address, phone number,
 * what the user bought before, etc.)
 *
 * \return The pointer to the users table.
 */
QtCassandra::QCassandraTable::pointer_t users::get_users_table()
{
    return f_snap->create_table(get_name(name_t::SNAP_NAME_USERS_TABLE), "Global users table.");
}

/** \brief Bootstrap the users.
 *
 * This function adds the events the users plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void users::on_bootstrap(::snap::snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN0(users, "server", server, init);
    SNAP_LISTEN0(users, "server", server, process_cookies);
    SNAP_LISTEN0(users, "server", server, attach_to_session);
    SNAP_LISTEN0(users, "server", server, detach_from_session);
    SNAP_LISTEN(users, "server", server, define_locales, _1);
    SNAP_LISTEN(users, "server", server, improve_signature, _1, _2, _3);
    SNAP_LISTEN(users, "server", server, table_is_accessible, _1, _2);
    SNAP_LISTEN0(users, "locale", locale::locale, set_locale);
    SNAP_LISTEN0(users, "locale", locale::locale, set_timezone);
    SNAP_LISTEN(users, "content", content::content, create_content, _1, _2, _3);
    SNAP_LISTEN(users, "path", path::path, can_handle_dynamic_path, _1, _2);
    SNAP_LISTEN(users, "layout", layout::layout, generate_header_content, _1, _2, _3, _4);
    SNAP_LISTEN(users, "layout", layout::layout, generate_page_content, _1, _2, _3, _4);
    SNAP_LISTEN(users, "filter", filter::filter, replace_token, _1, _2, _3, _4);

    f_info.reset(new sessions::sessions::session_info);
}


/** \brief Initialize the users plugin.
 *
 * At this point this function does nothing.
 */
void users::on_init()
{
}


/** \brief Retrieve the user cookie name.
 *
 * This function retrieves the user cookie name. This can be changed on
 * each restart of the server or after a period of time. The idea is to
 * not allow robots to use one statically defined cookie name on all
 * Snap! websites. It is probably easy for them to find out what the
 * current cookie name is, but it's definitively additional work for
 * the hackers.
 *
 * Also since the cookie is marked as HttpOnly, it is even harder for
 * hackers to do much with those.
 *
 * \return The current user cookie name for this website.
 */
QString users::get_user_cookie_name()
{
    QString user_cookie_name(f_snap->get_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_USER_COOKIE_NAME)).stringValue());
    if(user_cookie_name.isEmpty())
    {
        // user cookie name not yet assigned or reset so a new name
        // gets assigned
        unsigned char buf[COOKIE_NAME_SIZE];
        int r(RAND_bytes(buf, sizeof(buf)));
        if(r != 1)
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE,
                    "Service Not Available", "The server was not able to generate a safe random number. Please try again in a moment.",
                    "User cookie name could not be generated as the RAND_bytes() function could not generate enough random data");
            NOTREACHED();
        }
        // actually most ASCII characters are allowed, but to be fair, it
        // is not safe to use most so we limit using a simple array
        char allowed_characters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.";
        for(int i(0); i < (COOKIE_NAME_SIZE - 2); i += 3)
        {
            // we can generate 4 characters with every 3 bytes we read
            int a(buf[i + 0] & 0x3F);
            int b(buf[i + 1] & 0x3F);
            int c(buf[i + 2] & 0x3F);
            int d((buf[i + 0] >> 6) | ((buf[i + 1] >> 4) & 0x0C) | ((buf[i + 2] >> 2) & 0x30));
            if(i == 0 && a >= 52)
            {
                a &= 0x1F;
            }
            user_cookie_name += allowed_characters[a];
            user_cookie_name += allowed_characters[b];
            user_cookie_name += allowed_characters[c];
            user_cookie_name += allowed_characters[d];
        }
        f_snap->set_site_parameter(snap::get_name(snap::name_t::SNAP_NAME_CORE_USER_COOKIE_NAME), user_cookie_name);
    }
    return user_cookie_name;
}


/** \brief Process the cookies.
 *
 * This function is our opportunity to log the user in. We check for the
 * user cookie and use it to know whether the user is currently logged in
 * or not.
 *
 * Note that this session is always created and is used by all the other
 * plugins as the current user session.
 *
 * Only this very function also checks whether the user is currently
 * logged in and defines the user key (email address) if so. Otherwise the
 * session can be used for things such as saving messages between redirects.
 *
 * \important
 * This function cannot be called more than once. It would not properly
 * reset variables if called again.
 */
void users::on_process_cookies()
{
    // prevent cookies on a set of method that do not require them
    QString const method(f_snap->snapenv(get_name(snap::name_t::SNAP_NAME_CORE_HTTP_REQUEST_METHOD)));
    if(method == "HEAD"
    || method == "TRACE")
    {
        return;
    }

    bool create_new_session(true);

    // get cookie name
    QString const user_cookie_name(get_user_cookie_name());

    // any snap session?
    if(f_snap->cookie_is_defined(user_cookie_name))
    {
        // is that session a valid user session?
        QString const session_cookie(f_snap->cookie(user_cookie_name));
        snap_string_list const parameters(session_cookie.split("/"));
        QString const session_key(parameters[0]);
        QString random_key; // TODO: really support the case of "no random key"???
        if(parameters.size() > 1)
        {
            random_key = parameters[1];
        }
        sessions::sessions::instance()->load_session(session_key, *f_info, false);
        QString const path(f_info->get_object_path());
        bool authenticated(true);
        if(f_info->get_session_type() != sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID)
        {
            SNAP_LOG_INFO("cookie refused because session is not marked as valid, ")(static_cast<int>(f_info->get_session_type()));
            authenticated = false;
        }
        if(f_info->get_session_id() != USERS_SESSION_ID_LOG_IN_SESSION)
        {
            SNAP_LOG_INFO("cookie refused because this is not a user session, ")(f_info->get_session_id());
            authenticated = false;
        }
        if(f_info->get_session_random() != random_key.toInt())
        {
            SNAP_LOG_INFO("cookie would be refused because random key ")(random_key)(" does not match ")(f_info->get_session_random());
            //authenticated = false; -- there should be a flag because
            //                          in many cases it kicks someone
            //                          out even when it should not...
            //
            // From what I can tell, this mainly happens if someone uses two
            // tabs accessing the same site. But I've seen it quite a bit
            // if the system crashes and thus does not send the new random
            // number to the user. We could also look into a way to allow
            // the previous random for a while longer.
        }
        if(f_info->get_user_agent() != f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)))
        {
            SNAP_LOG_INFO("cookie refused because user agent \"")(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)))
                            ("\" does not match \"")(f_info->get_user_agent())("\"");
            authenticated = false;
        }
        if(path.left(6) != "/user/")
        {
            SNAP_LOG_INFO("cookie refused because the path does not start with /user/, ")(path);
            authenticated = false;
        }
        if(authenticated)
        {
            // this session qualifies as a log in session
            // so now verify the user
            authenticated_user(path.mid(6), nullptr);
            create_new_session = false;
        }
    }

    // There is a login limit so we do not need to "randomly" limit
    // a visitor user session to a ridiculously small amount unless
    // we think that could increase the database size too much...
    // two reasons to have a very long time to live are:
    //   1) user created a cart and we want the items he put in his
    //      cart to stay there "forever" (at least a year)
    //   2) user was sent to the site through an affiliate link, we
    //      want to reward the affiliate whether the user was sent
    //      there 1 day or 1 year ago
    // To satisfy any user, we need this to be an administrator setup
    // value. By default we use one whole year...
    f_info->set_time_to_live(86400 * 365);  // 365 days

    // create or refresh the session
    if(create_new_session)
    {
        // create a new session
        f_info->set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_USER);
        f_info->set_session_id(USERS_SESSION_ID_LOG_IN_SESSION);
        f_info->set_plugin_owner(get_plugin_name()); // ourselves
        //f_info->set_page_path(); -- default is fine, we do not use the path
        f_info->set_object_path("/user/"); // no user id for the anonymous user
        f_info->set_user_agent(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
        sessions::sessions::instance()->create_session(*f_info);
    }
    else
    {
        // extend the session
        f_info->set_time_to_live(86400 * 5);  // 5 days

        // TODO: change the 5 minutes with a parameter the admin can change
        //       if the last session was created more than 5 minutes ago then
        //       we generate a new random identifier (doing it on each access
        //       generates a lot of problems when the browser tries to load
        //       many things at the same time)
        //
        // TBD: this is not working right if the user attempts to open
        //      multiple pages quickly at the same time
        bool const new_random(f_info->get_date() + 60 * 5 * 1000000 < f_snap->get_start_date());
        sessions::sessions::instance()->save_session(*f_info, new_random);
    }

    // push new cookie info back to the browser
    http_cookie cookie(
            f_snap,
            user_cookie_name,
            QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random())
        );
    cookie.set_expire_in(86400 * 5);  // 5 days
    cookie.set_http_only(); // make it a tad bit safer
    f_snap->set_cookie(cookie);
//std::cerr << "user session id [" << f_info->get_session_key() << "] [" << f_user_key << "]\n";

    if(!f_user_key.isEmpty())
    {
        // make sure user locale/timezone get used on next
        // locale/timezone access
        locale::locale::instance()->reset_locale();

        // send a signal that the user is ready (this signal is also
        // sent when we have a valid cookie)
        logged_in_user_ready();
    }
}


/** \brief Allow other plugins to authenticate a user.
 *
 * The user cookie is used to determine whether a user is logged in. If
 * a different plugin is used that does not make use of the cookies,
 * then this function can be called with the email address of the user
 * to see whether the user's session is still active.
 *
 * If the path used to access this function starts with /logout then
 * the user is forcibly logged out instead of logged in.
 *
 * \note
 * The specified info is saved in the users' plugin f_info variable
 * member only if the user gets authenticated.
 *
 * \param[in] key  The user email.
 * \param[in] info  A pointer to the user's information to be used.
 *
 * \return true if the user gets authenticated, false in all other cases
 */
bool users::authenticated_user(QString const & key, sessions::sessions::session_info * info)
{
    // called with a seemingly valid key?
    if(key.isEmpty())
    {
        SNAP_LOG_INFO("cannot authenticate user without a key");
        return false;
    }

    // called with the email address of a user who registered before?
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(!users_table->exists(key))
    {
        SNAP_LOG_INFO("user key \"")(key)("\" was not found in the users table");
        return false;
    }

    // is the user/application trying to log out
    QString const uri_path(f_snap->get_uri().path());
    if(uri_path == "logout" || uri_path.left(8) == "logout/")
    {
        // the user is requesting to be logged out, here we avoid
        // dealing with all the session information again this
        // way we right away cancel the log in but we actually
        // keep the session
        f_user_key = key;
        if(info)
        {
            *f_info = *info;
        }
        user_logout();
        return false;
    }

    // the user still has a valid session, but he may
    // not be fully logged in... (i.e. not have as much
    // permission as given with a fresh log in)
    //
    // TODO: we need an additional form to authorize
    //       the user to do more
    time_t limit(info ? info->get_login_limit() : f_info->get_login_limit());
    f_user_logged_in = f_snap->get_start_time() < limit;
    if(!f_user_logged_in)
    {
        SNAP_LOG_INFO("user authentication timed out by ")(limit - f_snap->get_start_time())(" micro seconds");
    }

    // the website may opt out of the long session scheme
    // the following loses the user key if the website
    // administrator said so...
    QtCassandra::QCassandraValue const long_sessions(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_LONG_SESSIONS)));
    if(f_user_logged_in
    || (long_sessions.nullValue() || long_sessions.signedCharValue()))
    {
        f_user_key = key;
        if(info)
        {
            *f_info = *info;
        }
        return true;
    }

    return false;
}


/** \brief This function can be used to log the user out.
 *
 * If your software detects a situation where a currently logged in
 * user should be forcibly logged out, this function can be called.
 * The result is to force the user to log back in.
 *
 * Note that you should let the user know why you are kicking him
 * or her out otherwise they are likely to try to log back in again
 * and again and possibly get locked out (i.e. too many loggin
 * attempts.) In most cases, an error or warning message and a
 * redirect will do. This function does not do either so it is
 * likely that the user will be redirect to the log in page if
 * you do not do a redirect yourself.
 *
 * \warning
 * The function should never be called before the process_cookies()
 * signal gets processed, although this function should work if called
 * from within the user_logged_in() function.
 *
 * \warning
 * If you return from your function (instead of redirecting the user)
 * you may get unwanted results (i.e. the user could still be shown
 * the page accessed.)
 */
void users::user_logout()
{
    // the software is requesting to log the user out
    //
    // cancel the session
    f_info->set_object_path("/user/");

    // drop the referrer if there is one, it is a security
    // issue to keep that info on an explicit log out!
    NOTUSED(sessions::sessions::instance()->detach_from_session(*f_info, get_name(name_t::SNAP_NAME_USERS_LOGIN_REFERRER)));

    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    QtCassandra::QCassandraRow::pointer_t row(users_table->row(f_user_key));

    // Save the date when the user logged out
    QtCassandra::QCassandraValue value;
    value.setInt64Value(f_snap->get_start_date());
    row->cell(get_name(name_t::SNAP_NAME_USERS_LOGOUT_ON))->setValue(value);

    // Save the user IP address when logged out
    value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
    row->cell(get_name(name_t::SNAP_NAME_USERS_LOGOUT_IP))->setValue(value);

    sessions::sessions::instance()->save_session(*f_info, false);

    // Login session was destroyed so we really do not need it here anymore
    QString const last_login_session(row->cell(get_name(name_t::SNAP_NAME_USERS_LOGIN_SESSION))->value().stringValue());
    if(last_login_session == f_info->get_session_key())
    {
        // when clicking the "Log Out" button, we may already have been
        // logged out and if that is the case the session may not be
        // the same, hence the previous test to make sure we only delete
        // the session identifier that correspond to the last session
        //
        row->dropCell(get_name(name_t::SNAP_NAME_USERS_LOGIN_SESSION), QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
    }

    f_user_key.clear();
    f_user_logged_in = false;
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
 * \param[in] email  The user's email.
 * \param[in] field_name  The name of the field where value gets saved.
 * \param[in] value  The value of the field to save.
 *
 * \return true if the value was read from the database.
 *
 * \sa load_user_parameter()
 */
void users::save_user_parameter(QString const & email, QString const & field_name, QtCassandra::QCassandraValue const & value)
{
    int64_t const start_date(f_snap->get_start_date());

    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    QtCassandra::QCassandraRow::pointer_t row(users_table->row(email));

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


void users::save_user_parameter(QString const & email, QString const & field_name, QString const & value)
{
    QtCassandra::QCassandraValue v(value);
    save_user_parameter(email, field_name, v);
}


void users::save_user_parameter(QString const & email, QString const & field_name, int64_t const & value)
{
    QtCassandra::QCassandraValue v(value);
    save_user_parameter(email, field_name, v);
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
 * \param[in] email  The user's email.
 * \param[in] field_name  The name of the field being checked.
 * \param[out] value  The value of the field, empty if undefined.
 *
 * \return true if the value was read from the database.
 *
 * \sa save_user_parameter()
 */
bool users::load_user_parameter(QString const & email, QString const & field_name, QtCassandra::QCassandraValue & value)
{
    // reset the input value by default
    value.setNullValue();

    // make sure that row (a.k.a. user) exists before accessing it
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(!users_table->exists(email))
    {
        return false;
    }
    QtCassandra::QCassandraRow::pointer_t user_row(users_table->row(email));

    // row exists, make sure the user field exists
    if(!user_row->exists(field_name))
    {
        return false;
    }

    // retrieve that parameter
    value = user_row->cell(field_name)->value();

    return true;
}


bool users::load_user_parameter(QString const & email, QString const & field_name, QString & value)
{
    QtCassandra::QCassandraValue v;
    if(load_user_parameter(email, field_name, v))
    {
        value = v.stringValue();
        return true;
    }
    return false;
}


bool users::load_user_parameter(QString const & email, QString const & field_name, int64_t & value)
{
    QtCassandra::QCassandraValue v;
    if(load_user_parameter(email, field_name, v))
    {
        value = v.safeInt64Value();
        return true;
    }
    return false;
}


/** \brief Check whether \p cpath matches our introducers.
 *
 * This function checks that cpath matches our introducer and if
 * so we tell the path plugin that we're taking control to
 * manage this path.
 *
 * We understand "user" as in list of users.
 *
 * We understand "user/<name>" as in display that user information
 * (this may be turned off on a per user or for the entire website.)
 * Websites that only use an email address for the user identification
 * do not present these pages publicly.
 *
 * We understand "profile" which displays the current user profile
 * information in detail and allow for editing of what can be changed.
 *
 * We understand "login" which displays a form for the user to log in.
 *
 * We understand "verify-credentials" which is very similar to "login"
 * albeit simpler and only appears if the user is currently logged in
 * but not recently logged in (i.e. administration rights.)
 *
 * We understand "logout" to allow users to log out of Snap! C++.
 *
 * We understand "register" to display a registration form to users.
 *
 * We understand "verify" to check a session that is being returned
 * as the user clicks on the link we sent on registration.
 *
 * We understand "forgot-password" to let users request a password reset
 * via a simple form.
 *
 * \todo
 * If we cannot find a global way to check the Origin HTTP header
 * sent by the user agent, we probably want to check it here in
 * pages where the referrer should not be a "weird" 3rd party
 * website.
 *
 * \param[in,out] ipath  The path being handled dynamically.
 * \param[in,out] plugin_info  If you understand that cpath, set yourself here.
 */
void users::on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info)
{
    // is that path already going to be handled by someone else?
    // (avoid wasting time if that is the case)
    //
    // this happens when the attachment plugin is to handle user
    // image previews
    if(plugin_info.get_plugin()
    || plugin_info.get_plugin_if_renamed())
    {
        return;
    }

    //
    // WARNING:
    //
    //    DO NOT PROCESS ANYTHING HERE!
    //
    //    At this point we do not know whether the user has the right
    //    permissions yet.
    //
    //    See users::on_path_execute() instead.
    //
    QString cpath(ipath.get_cpath());
    if(cpath == "user"                      // list of (public) users
    || cpath == "profile"                   // the logged in user profile
    || cpath == "login"                     // form to log user in
    || cpath == "logout"                    // log user out
    || cpath == "register"                  // form to let new users register
    || cpath == "verify-credentials"        // re-log user in
    || cpath == "verify"                    // verification form so the user can enter his code
    || cpath.left(7) == "verify/"           // link to verify user's email; and verify/resend form
    || cpath == "forgot-password"           // form for users to reset their password
    || cpath == "new-password"              // form for users to enter their forgotten password verification code
    || cpath.left(13) == "new-password/")   // form for users to enter their forgotten password verification code
    {
        // tell the path plugin that this is ours
        plugin_info.set_plugin(this);
    }
    else if(cpath.left(5) == "user/")       // show a user profile (user/ is followed by the user identifier or some edit page such as user/password)
    {
        snap_string_list const user_segments(cpath.split("/"));
        if(user_segments.size() == 2)
        {
            plugin_info.set_plugin(this);
        }
    }
}


/** \brief Execute the specified path.
 *
 * This is a dynamic page which the users plugin knows how to handle.
 *
 * This function never returns if the "page" is just a verification
 * process which redirects the user (i.e. "verify/<id>", and
 * "new-password/<id>" at this time.)
 *
 * Other paths may also redirect the user in case the path is not
 * currently supported (mainly because the user does not have
 * permission.)
 *
 * \param[in,out] ipath  The canonicalized path.
 *
 * \return true if the processing worked as expected, false if the page
 *         cannot be created ("Page Not Present" results on false)
 */
bool users::on_path_execute(content::path_info_t& ipath)
{
    // handle the few that do some work and redirect immediately
    // (although it could be in the on_generate_main_content()
    // it is a big waste of time to start building a page when
    // we know we'll redirect the user anyway)
    if(ipath.get_cpath().left(7) == "verify/"
    && ipath.get_cpath() != "verify/resend")
    {
        verify_user(ipath);
        NOTREACHED();
    }
    else if(ipath.get_cpath().left(13) == "new-password/")
    {
        verify_password(ipath);
        NOTREACHED();
    }

    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return true;
}


void users::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body, QString const & ctemplate)
{
    QString const cpath(ipath.get_cpath());
    if(!cpath.isEmpty())
    {
        // the switch() optimization is worth it because all user pages
        // hit this test, so saving a few ms is always worth the trouble!
        // (i.e. at the moment, we already have 11 tests; any one cpath
        // would be checked 11 times for any page other than one of those
        // 11 pages... with the new scheme, we compare between 0 and 3 times
        // instead)
        switch(cpath[0].unicode())
        {
        case 'f':
            if(cpath == "forgot-password")
            {
                prepare_forgot_password_form();
            }
            break;

        case 'l':
            if(cpath == "login")
            {
                prepare_login_form();
            }
            else if(cpath == "logout")
            {
                // closing current session if any and show the logout page
                logout_user(ipath, page, body);
                return;
            }
            break;

        case 'n':
            if(cpath == "new-password")
            {
                prepare_new_password_form();
            }
            break;

        //case 'p':
        //  if(cpath == "profile")
        //  {
        //      // TODO: write user profile editor
        //      //       this is /user, /user/###, and /user/me at this point
        //      //user_profile(body);
        //      return;
        //  }
        //  break;

        case 'r':
            // "register" is the same form as "verify" and "verify/resend"
            if(cpath == "register")
            {
                prepare_basic_anonymous_form();
            }
            break;

        case 'u':
            if(cpath == "user")
            {
                // TODO: write user listing (similar to the /admin page
                //       in gathering the info)
                //list_users(body);
                output::output::instance()->on_generate_main_content(ipath, page, body, ctemplate);
                return;
            }
            else if(cpath == "user/password/replace")
            {
                // this is a very special form that is accessible by users who
                // requested to change the password with the "forgot password"
                prepare_replace_password_form(body);
            }
            else if(cpath.left(5) == "user/")
            {
                show_user(ipath, page, body);
                return;
            }
            break;

        case 'v':
            if(cpath == "verify-credentials")
            {
                prepare_verify_credentials_form();
            }
            else if(cpath == "verify"
                 || cpath == "verify/resend")
            {
                prepare_basic_anonymous_form();
            }
            break;

        }
    }
    // any other user page is just like regular content
    output::output::instance()->on_generate_main_content(ipath, page, body, ctemplate);
}



void users::on_generate_boxes_content(content::path_info_t& page_cpath, content::path_info_t& ipath, QDomElement& page, QDomElement& box, QString const& ctemplate)
{
    if(!f_user_key.isEmpty())
    {
        if(ipath.get_cpath().endsWith("login")
        || ipath.get_cpath().endsWith("register"))
        {
            return;
        }
    }

//std::cerr << "GOT TO USER BOXES!!! [" << ipath.get_key() << "]\n";
    if(ipath.get_cpath().endsWith("/login"))
    {
        // do not display the login box on the login page
        // or if the user is already logged in

// DEBUG -- at this point there are conflicts with more than 1 form on a page, so I only allow that form on the home page
//if(page_cpath.get_cpath() != "") return;

        if(page_cpath.get_cpath() == "login"
        || page_cpath.get_cpath() == "register")
        {
            return;
        }
    }

    output::output::instance()->on_generate_main_content(ipath, page, box, ctemplate);
}


void users::on_generate_header_content(content::path_info_t& ipath, QDomElement& header, QDomElement& metadata, QString const& ctemplate)
{
    NOTUSED(ipath);
    NOTUSED(ctemplate);

    QDomDocument doc(header.ownerDocument());

    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());

    // retrieve the row for that user
    if(!f_user_key.isEmpty() && users_table->exists(f_user_key))
    {
        QtCassandra::QCassandraRow::pointer_t user_row(users_table->row(f_user_key));

        {   // snap/head/metadata/desc[@type='users::email']/data
            QDomElement desc(doc.createElement("desc"));
            desc.setAttribute("type", "users::email");
            metadata.appendChild(desc);
            QDomElement data(doc.createElement("data"));
            desc.appendChild(data);
            QDomText text(doc.createTextNode(f_user_key));
            data.appendChild(text);
        }

        {   // snap/head/metadata/desc[@type='users::name']/data
            QtCassandra::QCassandraValue const value(user_row->cell(get_name(name_t::SNAP_NAME_USERS_USERNAME))->value());
            if(!value.nullValue())
            {
                QDomElement desc(doc.createElement("desc"));
                desc.setAttribute("type", get_name(name_t::SNAP_NAME_USERS_NAME));
                metadata.appendChild(desc);
                QDomElement data(doc.createElement("data"));
                desc.appendChild(data);
                QDomText text(doc.createTextNode(value.stringValue()));
                data.appendChild(text);
            }
        }

        {   // snap/head/metadata/desc[@type='users::created']/data
            QtCassandra::QCassandraValue const value(user_row->cell(get_name(name_t::SNAP_NAME_USERS_CREATED_TIME))->value());
            if(!value.nullValue())
            {
                QDomElement desc(doc.createElement("desc"));
                desc.setAttribute("type", "users::created"); // NOTE: in the database it is named "users::created_time"
                metadata.appendChild(desc);
                QDomElement data(doc.createElement("data"));
                desc.appendChild(data);
                QDomText text(doc.createTextNode(f_snap->date_to_string(value.int64Value())));
                data.appendChild(text);
            }
        }
    }
}


void users::on_generate_page_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    NOTUSED(ctemplate);

    // TODO: convert using field_search
    QDomDocument doc(page.ownerDocument());

    // retrieve the authors
    // TODO: add support to retrieve the "author" who last modified this
    //       page (i.e. user reference in the last revision)
    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    QString const link_name(get_name(name_t::SNAP_NAME_USERS_AUTHOR));
    links::link_info author_info(link_name, true, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(author_info));
    links::link_info user_info;
    if(link_ctxt->next_link(user_info))
    {
        // an author is attached to this page!
        //
        // all we want to offer here is the author details defined in the
        // /user/... location although we may want access to his email
        // address too (to display to an admin for example)
        content::path_info_t user_ipath;
        user_ipath.set_path(user_info.key());

        {   // snap/page/body/author[@type="users::name"]/data
            QtCassandra::QCassandraValue const value(content_table->row(user_ipath.get_key())->cell(get_name(name_t::SNAP_NAME_USERS_USERNAME))->value());
            if(!value.nullValue())
            {
                QDomElement author(doc.createElement("author"));
                author.setAttribute("type", get_name(name_t::SNAP_NAME_USERS_NAME));
                body.appendChild(author);
                QDomElement data(doc.createElement("data"));
                author.appendChild(data);
                QDomText text(doc.createTextNode(value.stringValue()));
                data.appendChild(text);
            }
        }

        // TODO test whether the author has a public profile, if so then
        //      add a link to the account
    }
}



void users::on_create_content(content::path_info_t& ipath, QString const& owner, QString const& type)
{
    NOTUSED(owner);
    NOTUSED(type);

    if(!f_user_key.isEmpty())
    {
        QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
        if(users_table->exists(f_user_key))
        {
            QtCassandra::QCassandraValue const value(users_table->row(f_user_key)->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
            if(!value.nullValue())
            {
                int64_t const identifier(value.int64Value());
                QString const site_key(f_snap->get_site_key_with_slash());
                QString const user_key(QString("%1%2/%3").arg(site_key).arg(get_name(name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

                content::path_info_t user_ipath;
                user_ipath.set_path(user_key);

                QString const link_name(get_name(name_t::SNAP_NAME_USERS_AUTHOR));
                bool const source_unique(true);
                links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
                QString const link_to(get_name(name_t::SNAP_NAME_USERS_AUTHORED_PAGES));
                bool const destination_multi(false);
                links::link_info destination(link_to, destination_multi, user_ipath.get_key(), user_ipath.get_branch());
                links::links::instance()->create_link(source, destination);
            }
        }
    }
}


/** \brief Let the user replace their password.
 *
 * This is a very special form that is only accessible when the user
 * requests a special link after forgetting their password.
 *
 * \param[in] body  The body where the form is saved.
 */
void users::prepare_replace_password_form(QDomElement& body)
{
    NOTUSED(body);

    // make sure the user is properly setup
    if(user_is_logged_in())
    {
        // user is logged in already, send him to his normal password form
        f_snap->page_redirect("user/password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER, "Already Logged In", "You are already logged in so you cannot access this page at this time.");
        NOTREACHED();
    }
    if(!f_user_key.isEmpty())
    {
        // user logged in a while back, ask for credentials again
        f_snap->page_redirect("verify-credentials", snap_child::http_code_t::HTTP_CODE_SEE_OTHER, "Not Enough Permissions", "You are logged in with minimal permissions. To access this page we have to verify your credentials.");
        NOTREACHED();
    }
    if(f_user_changing_password_key.isEmpty())
    {
        // user is not even logged in and he did not follow a valid link
        // XXX the login page is probably the best choice?
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER, "Replace Password Not Possible", "You required to change your password in a way which is not current valid. Please go to log in instead.");
        NOTREACHED();
    }
}


/** \brief Show the user profile.
 *
 * This function shows a user profile. By default one can use user/me to
 * see his profile. The administrators can see any profile. Otherwise
 * only public profiles and the user own profile are accessible.
 */
void users::show_user(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    QString user_path(ipath.get_cpath());
    int64_t identifier(0);
    QString user_id(user_path.mid(5));
    if(user_id == "me" || user_id == "password")
    {
        // retrieve the logged in user identifier
        if(f_user_key.isEmpty())
        {
            attach_to_session(get_name(name_t::SNAP_NAME_USERS_LOGIN_REFERRER), "user/password");

            messages::messages::instance()->set_error(
                "Permission Denied",
                "You are not currently logged in. You may check out your profile only when logged in.",
                "attempt to view the current user page when the user is not logged in",
                false
            );
            // redirect the user to the log in page
            f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            NOTREACHED();
        }
        QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
        if(!users_table->exists(f_user_key))
        {
            // This should never happen... we checked that account when the
            // user logged in
            messages::messages::instance()->set_error(
                "Could Not Find Your Account",
                "Somehow we could not find your account on this system.",
                "user account for " + f_user_key + " does not exist at this point",
                true
            );
            // redirect the user to the log in page
            f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            NOTREACHED();
            return;
        }
        QtCassandra::QCassandraValue value(users_table->row(f_user_key)->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
        if(value.nullValue())
        {
            messages::messages::instance()->set_error(
                "Could Not Find Your Account",
                "Somehow we could not find your account on this system.",
                "user account for " + f_user_key + " does not have an identifier",
                true
            );
            // redirect the user to the log in page
            f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            NOTREACHED();
            return;
        }
        identifier = value.int64Value();

        if(user_id == "password")
        {
            // user is editing his password
            prepare_password_form();
            output::output::instance()->on_generate_main_content(ipath, page, body, "");
            return;
        }

        // Probably not necessary to change user_id now
        //user_id = QString("%1").arg(identifier);
        user_path = QString("user/%1").arg(identifier);
    }
    else
    {
        bool ok(false);
        identifier = user_id.toLongLong(&ok);
        if(!ok)
        {
            // invalid user identifier, generate a 404
            f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                    "User Not Found", "This user does not exist. Please check the URI and make corrections as required.",
                    "User attempt to access user \"" + user_id + "\" which does not look like a valid integer.");
            NOTREACHED();
        }

        // verify that the identifier indeed represents a user
        const QString site_key(f_snap->get_site_key_with_slash());
        const QString user_key(site_key + get_name(name_t::SNAP_NAME_USERS_PATH) + "/" + user_id);
        QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
        if(!content_table->exists(user_key))
        {
            f_snap->die(snap_child::http_code_t::HTTP_CODE_NOT_FOUND,
                "User Not Found",
                "We could not find an account for user " + user_id + " on this system.",
                "user account for " + user_id + " does not exist at this point"
            );
            NOTREACHED();
        }
    }
//printf("Got user [%s] / [%ld]\n", cpath.toUtf8().data(), identifier);
//std::cout << "Got user [" << identifier << "]" << std::endl << std::flush;

    // generate the user profile
        // TODO: write user profile viewer (i.e. we need to make use of the identifier here!)
        // WARNING: using a path such as "admin/.../profile" returns all the content of that profile
    content::path_info_t user_ipath;
    user_ipath.set_path(user_path);
    output::output::instance()->on_generate_main_content(user_ipath, page, body, "admin/users/page/profile");
}


/** \brief Generate the password form.
 *
 * This function adds a compiled password form to the body content.
 * (i.e. this is the main page body content.)
 *
 * This form includes the original password, and the new password with
 * a duplicate to make sure the user enters it twice properly.
 *
 * The password can also be changed by requiring the system to send
 * an email. In that case, and if the user then remembers his old
 * password, then this form is hit on the following log in.
 */
void users::prepare_password_form()
{
    if(f_user_key.isEmpty())
    {
        // user needs to be logged in to edit his password
        f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                "Access Denied",
                "You need to be logged in and have enough permissions to access this page.",
                "user attempt to change a password without enough permissions.");
        NOTREACHED();
    }
}


/** \brief Prepare the login form.
 *
 * This function makes sure that the user is not already logged in because
 * if so the user is just sent to his profile (/user/me).
 *
 * Otherwise it saves the HTTP_REFERER information as the redirect
 * after a successfull log in.
 */
void users::prepare_login_form()
{
    if(!f_user_key.isEmpty())
    {
        // user is logged in already, just send him to his profile
        f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    set_referrer( f_snap->snapenv("HTTP_REFERER") );
}


/** \brief Verify user credentials.
 *
 * The verify user credentials form can only appear to users who logged
 * in a while back and who need administrative rights to access a page.
 */
void users::prepare_verify_credentials_form()
{
    // user is an anonymous user, send him to the login form instead
    if(f_user_key.isEmpty())
    {
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    if(user_is_logged_in())
    {
        // ?!? -- what should we do in this case?
        f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
}


/** \brief Log the current user out.
 *
 * Actually this function only generates the log out page. The log out itself
 * is processed at the same time as the cookie in the on_process_cookies()
 * function.
 *
 * This function calls the on_generate_main_content() of the content plugin.
 *
 * \param[in,out] ipath  The path being processed (logout[/...]).
 * \param[in,out] page  The page XML data.
 * \param[in,out] body  The body XML data.
 */
void users::logout_user(content::path_info_t& ipath, QDomElement& page, QDomElement& body)
{
    // generate the body
    // we already logged the user out in the on_process_cookies() function
    if(ipath.get_cpath() != "logout" && ipath.get_cpath() != "logout/")
    {
        // make sure the page exists if the user was sent to antoher plugin
        // path (i.e. logout/fantom from the fantom plugin could be used to
        // display a different greating because the user was kicked out by
        // spirits...); if it does not exist, force "logout" as the default
        QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
        if(!content_table->exists(ipath.get_key()))
        {
            // forcing to exact /logout page
            ipath.set_path("logout");
        }
    }

    output::output::instance()->on_generate_main_content(ipath, page, body, "");
}


/** \brief Prepare a public user form.
 *
 * This function is used to prepare a basic user form which is only
 * intended for anonymous users. All it does is verify that the user
 * is not logged in. If logged in, then the user is simply send to
 * his profile (user/me).
 */
void users::prepare_basic_anonymous_form()
{
    if(!f_user_key.isEmpty())
    {
        // user is logged in already, just send him to his profile
        f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
}


/** \brief Resend a verification email to the user.
 *
 * This function sends the verification email as if the user was just
 * registering. It is at items useful if the first email gets blocked
 * or lost in a junk mail folder.
 *
 * We should also show the "From" email on our forms so users can say
 * that these are okay.
 *
 * \todo
 * Add a question such as "what's your favority movie", "where were you
 * born", etc. so we can limit the number of people who use this form.
 */
void users::prepare_forgot_password_form()
{
    if(!f_user_key.isEmpty())
    {
        // send user to his change password form if he's logged in
        // XXX look into changing this policy and allow logged in
        //     users to request a password change? (I don't think
        //     it matters actually)
        messages::messages::instance()->set_error(
            "You Are Logged In",
            "If you want to change your password and forgot your old password, you'll have to log out and request for a new password while not logged in.",
            "user tried to get to the forgot_password_form() while logged in.",
            false
        );
        f_snap->page_redirect("user/password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
}


/** \brief Allow the user to use his verification code to log in.
 *
 * This function verifies a verification code that was sent so the user
 * could change his password (i.e. an automatic log in mechanism.)
 */
void users::prepare_new_password_form()
{
    if(!f_user_key.isEmpty())
    {
        // send user to his change password form if he's logged in
        // XXX look into changing this policy and allow logged in
        //     users to request a password change? (I don't think
        //     it matters actually)
        messages::messages::instance()->set_error(
            "You Are Already Logged In",
            "If you want to change your password and forgot your old password, you'll have to log out and request for a new password while not logged in.",
            "user tried to get to the forgot_password_form() while logged in.",
            false
        );
        f_snap->page_redirect("user/password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
}


/** \brief Verification of a user.
 *
 * Whenever we generate a registration thank you email, we include a link
 * so the user can verify his email address. This verification happens
 * when the user clicks on the link and is sent to this very function.
 *
 * The path will look like this:
 *
 * \code
 * http[s]://<domain-name>/<path>/verify/<session>
 * \endcode
 *
 * The result is a verified tag on the user so that way we can let the
 * user log in without additional anything.
 *
 * Note that the user agent check can be turned off by software.
 *
 * \todo
 * As an additional verification we could use the cookie that was setup
 * to make sure that the user is the same person. This means the cookie
 * should not be deleted on closure in the event the user is to confirm
 * his email later and wants to close everything in the meantime. Also
 * that would not be good if user A creates an account for user B...
 *
 * \param[in,out] ipath  The path used to access this page.
 */
void users::verify_user(content::path_info_t& ipath)
{
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());

    if(!f_user_key.isEmpty())
    {
        // TODO: consider moving this parameter to the /admin/settings/users
        //       page instead (unless we want to force a "save to sites table"?)
        //
        QtCassandra::QCassandraValue const multiuser(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_MULTIUSER)));
        if(multiuser.nullValue() || !multiuser.signedCharValue())
        {
            // user is logged in already, just send him to his profile
            // (if logged in he was verified in some way!)
            f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            NOTREACHED();
        }

        // this computer is expected to be used by multiple users, the
        // link to /verify/### and /verify/send may be followed on a
        // computer with a logged in user (because we provide those
        // in the email we send just after registration)
        //
        // So in this case we want to log out the current user and
        // process the form as if no one had been logged in.
        f_info->set_object_path("/user/");
        f_info->set_time_to_live(86400 * 5);  // 5 days
        bool const new_random(f_info->get_date() + 60 * 5 * 1000000 < f_snap->get_start_date());

        // drop the referrer if there is one, it is a security
        // issue to keep that info on an almost explicit log out!
        NOTUSED(sessions::sessions::instance()->detach_from_session(*f_info, get_name(name_t::SNAP_NAME_USERS_LOGIN_REFERRER)));

        sessions::sessions::instance()->save_session(*f_info, new_random);

        QString const user_cookie_name(get_user_cookie_name());
        http_cookie cookie(
                f_snap,
                user_cookie_name,
                QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random())
            );
        cookie.set_expire_in(86400 * 5);  // 5 days
        cookie.set_http_only(); // make it a tad bit safer
        f_snap->set_cookie(cookie);

        QtCassandra::QCassandraRow::pointer_t row(users_table->row(f_user_key));

        // Save the date when the user logged out
        QtCassandra::QCassandraValue value;
        value.setInt64Value(f_snap->get_start_date());
        row->cell(get_name(name_t::SNAP_NAME_USERS_LOGOUT_ON))->setValue(value);

        // Save the user IP address when logged out
        value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
        row->cell(get_name(name_t::SNAP_NAME_USERS_LOGOUT_IP))->setValue(value);

        // Login session was destroyed so we really do not need it here anymore
        QString const last_login_session(row->cell(get_name(name_t::SNAP_NAME_USERS_LOGIN_SESSION))->value().stringValue());
        if(last_login_session == f_info->get_session_key())
        {
            // when clicking the "Log Out" button, we may already have been
            // logged out and if that is the case the session may not be
            // the same, hence the previous test to make sure we only delete
            // the session identifier that correspond to the last session
            //
            row->dropCell(get_name(name_t::SNAP_NAME_USERS_LOGIN_SESSION), QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
        }

        f_user_key.clear();
    }

    // remove "verify/" to retrieve the session ID
    QString const session_id(ipath.get_cpath().mid(7));
    sessions::sessions::session_info info;
    sessions::sessions *session(sessions::sessions::instance());
    // TODO: remove the ending characters such as " ", "/", "\" and "|"?
    //       (it happens that people add those by mistake at the end of a URI...)
    session->load_session(session_id, info);
    QString const path(info.get_object_path());
    if(info.get_session_type() != sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID
    || ((info.add_check_flags(0) & info.CHECK_HTTP_USER_AGENT) != 0 && info.get_user_agent() != f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)))
    || path.mid(0, 6) != "/user/")
    {
        // it failed, the session could not be loaded properly
        SNAP_LOG_WARNING("users::verify_user() could not load the user session ")
                            (session_id)(" properly. Session error: ")
                            (sessions::sessions::session_info::session_type_to_string(info.get_session_type()))(".");
        // TODO change message support to use strings from the database so they can get translated
        messages::messages::instance()->set_error(
            "Invalid User Verification Code",
            "The specified verification code (" + session_id
                    + ") is not correct. Please verify that you used the correct link or try to use the form below to enter your verification code."
                      " If you already followed the link once, then you already were verified and all you need to do is click the log in link below.",
            "user trying his verification with code \"" + session_id + "\" got error: "
                    + sessions::sessions::session_info::session_type_to_string(info.get_session_type()) + ".",
            true
        );
        // redirect the user to the verification form
        f_snap->page_redirect("verify", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // it looks like the session is valid, get the user email and verify
    // that the account exists in the database
    QString const email(path.mid(6));
    if(!users_table->exists(email))
    {
        // This should never happen...
        messages::messages::instance()->set_error(
            "Could Not Find Your Account",
            "Somehow we could not find your account on this system.",
            "user account for " + email + " does not exist at this point",
            true
        );
        // redirect the user to the log in page
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QtCassandra::QCassandraRow::pointer_t row(users_table->row(email));
    QtCassandra::QCassandraValue const user_identifier(row->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
    if(user_identifier.nullValue())
    {
        SNAP_LOG_FATAL("users::verify_user() could not load the user identifier, the row exists but the cell did not make it (")
                        (email)("/")
                        (get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))(").");
        // redirect the user to the verification form although it won't work
        // next time either...
        f_snap->page_redirect("verify", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    int64_t const identifier(user_identifier.int64Value());
    content::path_info_t user_ipath;
    user_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

    // before we actually accept this verification code, we must make sure
    // the user is still marked as a new user (he should or the session
    // would be invalid, but for security it is better to check again)
    links::link_info user_status_info(get_name(name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
    links::link_info status_info;
    if(!link_ctxt->next_link(status_info))
    {
        // This should never happen... because the session should logically
        // prevent it from happening (i.e. the status link should always be
        // there) although maybe the admin could delete this link somehow?
        messages::messages::instance()->set_error(
            "Not a New Account",
            "Your account is not marked as a new account. The verification failed.",
            "user account for " + email + ", which is being verified, is not marked as being a new account",
            true
        );
        // redirect the user to the log in page
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // a status link exists...
    QString const site_key(f_snap->get_site_key_with_slash());
    if(status_info.key() != site_key + get_name(name_t::SNAP_NAME_USERS_NEW_PATH))
    {
        // This should never happen... because the session should logically
        // prevent it from happening (i.e. the status link should always be
        // there) although maybe the admin could delete this link somehow?
        messages::messages::instance()->set_error(
            "Not a New Account",
            "Your account is not marked as a new account. The verification failed. You may have been blocked.",
            "user account for " + email + ", which is being verified, is not marked as being a new account: " + status_info.key(),
            true
        );
        // redirect the user to the log in page? (XXX should this be the registration page instead?)
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    // remove the "user/new" status link so the user can now log in
    // he was successfully verified
    links::links::instance()->delete_link(user_status_info);

    // Save the date when the user verified
    QtCassandra::QCassandraValue value;
    value.setInt64Value(f_snap->get_start_date());
    row->cell(get_name(name_t::SNAP_NAME_USERS_VERIFIED_ON))->setValue(value);

    // Save the user IP address when verified
    value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
    row->cell(get_name(name_t::SNAP_NAME_USERS_VERIFIED_IP))->setValue(value);

    // tell other plugins that a new user was created and let them add
    // bells and whisles to the new account
    user_verified(user_ipath, identifier);

    // TODO offer an auto-log in feature
    //      (TBD: this could be done by another plugin via the
    //      user_verified() signal although it makes a lot more sense to
    //      let the users plugin to do such a thing!)

    // send the user to the log in page since he got verified now
    messages::messages::instance()->set_info(
        "Verified!",
        "Thank you for taking the time to register an account with us. Your account is now verified! You can now log in with the form below."
    );
    f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
    NOTREACHED();
}


/** \fn void users::user_verified(content::path_info_t& ipath, int64_t identifier)
 * \brief Signal that a new user was verified.
 *
 * After a user registers, he receives an email with a magic number that
 * needs to be used for the user to register on the system.
 *
 * This signal is used in order to tell other plugins that the user did
 * following that link.
 *
 * \param[in,out] ipath  The user path.
 * \param[in] identifier  The user identifier.
 */


/** \brief Check that password verification code.
 *
 * This function verifies a password verification code that is sent to
 * the user whenever he says he forgot his password.
 *
 * \param[in] ipath  The path used to access this page.
 */
void users::verify_password(content::path_info_t& ipath)
{
    if(!f_user_key.isEmpty())
    {
        // TODO: delete the "password" tag if present
        //
        // user is logged in already, just send him to his profile
        // (if logged in he was verified in some way!)
        f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QString session_id(ipath.get_cpath().mid(13));

    sessions::sessions::session_info info;
    sessions::sessions *session(sessions::sessions::instance());
    // TODO: remove the ending characters such as " ", "/", "\" and "|"?
    //       (it happens that people add those by mistake at the end of a URI...)
    session->load_session(session_id, info);
    const QString path(info.get_object_path());
    if(info.get_session_type() != sessions::sessions::session_info::session_info_type_t::SESSION_INFO_VALID
    || info.get_user_agent() != f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT))
    || path.mid(0, 6) != "/user/")
    {
        // it failed, the session could not be loaded properly
        SNAP_LOG_WARNING("users::process_new_password_form() could not load the user session ")
                            (session_id)(" properly. Session error: ")
                            (sessions::sessions::session_info::session_type_to_string(info.get_session_type()))(".");
        // TODO change message support to use strings from the database so they can get translated
        messages::messages::instance()->set_error(
            "Invalid Forgotten Password Verification Code",
            "The specified verification code (" + session_id
                    + ") is not correct. Please verify that you used the correct link or try to use the form below to enter your verification code."
                      " If you already followed the link once, then you already exhausted that verfication code and if you need another you have to click the Resend link below.",
            "user trying his forgotten password verification with code \"" + session_id + "\" got error: "
                    + sessions::sessions::session_info::session_type_to_string(info.get_session_type()) + ".",
            true
        );
        // we are likely on the verification link for the new password
        // so we want to send people to the new-password page instead
        // XXX should we avoid the redirect if we're already on that page?
        f_snap->page_redirect("new-password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // it looks like the session is valid, get the user email and verify
    // that the account exists in the database
    const QString email(path.mid(6));
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(!users_table->exists(email))
    {
        // This should never happen...
        messages::messages::instance()->set_error(
            "Could Not Find Your Account",
            "Somehow we could not find your account on this system.",
            "user account for " + email + " does not exist at this point",
            true
        );
        // redirect the user to the log in page
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QtCassandra::QCassandraRow::pointer_t row(users_table->row(email));
    QtCassandra::QCassandraValue const user_identifier(row->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
    if(user_identifier.nullValue())
    {
        SNAP_LOG_FATAL("users::process_new_password_form() could not load the user identifier, the row exists but the cell did not make it (")
                        (email)("/")
                        (get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))(").");
        // TODO where to send that user?! have an error page for all of those
        //      "your account is dead, sorry dear..."
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    int64_t const identifier(user_identifier.int64Value());
    content::path_info_t user_ipath;
    user_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

    // before we actually accept this verification code, we must make sure
    // the user is still marked as a new user (he should or the session
    // would be invalid, but for security it is better to check again)
    links::link_info user_status_info(get_name(name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
    links::link_info status_info;
    if(!link_ctxt->next_link(status_info))
    {
        // This should never happen... because the session should logically
        // prevent it from happening (i.e. the status link should always be
        // there) although maybe the admin could delete this link somehow?
        messages::messages::instance()->set_error(
            "Forgotten Password?",
            "It does not look like you requested a new password for your account. The form is being canceled.",
            "user account for " + email + ", which requested a mew password, is not marked as expected a new password",
            true
        );
        // redirect the user to the log in page
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // a status link exists... is it the right one?
    QString const site_key(f_snap->get_site_key_with_slash());
    if(status_info.key() != site_key + get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH))
    {
        // This should never happen... because the session should logically
        // prevent it from happening (i.e. the status link should always be
        // there) although maybe the admin could delete this link somehow?
        messages::messages::instance()->set_error(
            "Forgotten Password?",
            "It does not look like you requested a new password for your account. If you did so multiple times, know that you can only follow one of the links once. Doing so voids the other links.",
            "user account for " + email + ", which requested a new password, is not marked as expecting a new password: " + status_info.key(),
            true
        );
        // redirect the user to the log in page? (XXX should this be the registration page instead?)
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    // remove the "user/password" status link so the user can now log in
    // he was successfully logged in -- don't kill this one yet...
    //links::links::instance()->delete_link(user_status_info);

    // redirect the user to the "semi-public replace password page"
    send_to_replace_password_page(email, false);
    NOTREACHED();
}


/** \brief This function sends the user to the replace password.
 *
 * WARNING: Use this function at your own risk! It allows the user to
 *          change (his) password and thus it should be done only if
 *          you know for sure (as sure as one can be in an HTTP context)
 *          that the user is allowed to do this.
 *
 * This function saves the email of the user to redirect to the
 * /user/password/replace page. That page is semi-public in that it can
 * be accessed by users who forgot their password after they followed
 * a link we generate from the "I forgot my password" account. It is
 * semi-public because, after all, it can be accessed by someone who is
 * not actually logged in.
 *
 * The function redirects you so it does not return.
 *
 * The function saves the date and time when it gets called, and the IP
 * address of the user who triggered the call.
 *
 * \param[in] email  The email of the user to redirect.
 * \param[in] set_status  Whether to setup the user status too.
 */
void users::send_to_replace_password_page(QString const& email, bool const set_status)
{
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    QtCassandra::QCassandraRow::pointer_t row(users_table->row(email));

    if(set_status)
    {
        // mark the user with the types/users/password tag
        // (i.e. user requested a new password)
        QString const link_name(get_name(name_t::SNAP_NAME_USERS_STATUS));
        bool const source_unique(true);
        content::path_info_t user_ipath;
        user_ipath.set_path(get_user_path(email));
        links::link_info source(link_name, source_unique, user_ipath.get_key(), user_ipath.get_branch());
        QString const link_to(get_name(name_t::SNAP_NAME_USERS_STATUS));
        bool const destination_unique(false);
        content::path_info_t dpath;
        dpath.set_path(get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH));
        links::link_info destination(link_to, destination_unique, dpath.get_key(), dpath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // Save the date when the user verified
    QtCassandra::QCassandraValue value;
    value.setInt64Value(f_snap->get_start_date());
    row->cell(get_name(name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_ON))->setValue(value);

    // Save the user IP address when verified
    value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
    row->cell(get_name(name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_IP))->setValue(value);

    f_user_changing_password_key = email;

    // send the user to the "public" replace password page since he got verified
    f_snap->page_redirect("user/password/replace", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
    NOTREACHED();
}


/** \brief Process a post from one of the users forms.
 *
 * This function processes the post of a user form. The function uses the
 * \p ipath parameter in order to determine which form is being processed.
 *
 * \param[in,out] ipath  The path the user is accessing now.
 * \param[in] session_info  The user session being processed.
 */
void users::on_process_form_post(content::path_info_t & ipath, sessions::sessions::session_info const & session_info)
{
    NOTUSED(session_info);

    QString const cpath(ipath.get_cpath());
    if(cpath == "login")
    {
        process_login_form(login_mode_t::LOGIN_MODE_FULL);
    }
    else if(cpath == "verify-credentials")
    {
        process_login_form(login_mode_t::LOGIN_MODE_VERIFICATION);
    }
    else if(cpath == "register")
    {
        process_register_form();
    }
    else if(cpath == "verify/resend")
    {
        process_verify_resend_form();
    }
    else if(cpath == "verify")
    {
        process_verify_form();
    }
    else if(cpath == "forgot-password")
    {
        process_forgot_password_form();
    }
    else if(cpath == "new-password")
    {
        process_new_password_form();
    }
    else if(cpath == "user/password/replace")
    {
        process_replace_password_form();
    }
    else if(cpath == "user/password")
    {
        process_password_form();
    }
    else
    {
        // this should not happen because invalid paths will not pass the
        // session validation process
        throw users_exception_invalid_path("users::on_process_form_post() was called with an unsupported path: \"" + ipath.get_key() + "\"");
    }
}


/** \brief Log the user in from the log in form.
 *
 * This function uses the credentials specified in the log in form.
 * The function searches for the user account and read its hashed
 * password and compare the password typed in the form. If it
 * matches, then the user receives a cookie and is logged in for
 * some time.
 *
 * This function takes a mode.
 *
 * \li LOGIN_MODE_FULL -- full mode (for the login form)
 * \li LOGIN_MODE_VERIFICATION -- verification mode (for the verify-credentials form)
 *
 * \param[in] login_mode  The mode used to log in: full, verification.
 */
void users::process_login_form(login_mode_t login_mode)
{
    messages::messages * messages_plugin(messages::messages::instance());

    // retrieve the row for that user
    QString const key(f_snap->postenv("email"));
    if(login_mode == login_mode_t::LOGIN_MODE_VERIFICATION && f_user_key != key)
    {
        // XXX we could also automatically log the user out and send him
        //     to the log in screen... (we certainly should do so on the
        //     third attempt!)
        messages_plugin->set_error(
            "Wrong Credentials",
            "These are wrong credentials. If you are not sure who you were logged as, please <a href=\"/logout\">log out</a> first and then log back in.",
            QString("users::process_login_form() email mismatched when verifying credentials (got \"%1\", expected \"%2\").").arg(key).arg(f_user_key),
            false
        );
        return;
    }

    QString const password(f_snap->postenv("password"));

    bool validation_required(false);
    QString const details(login_user(key, password, validation_required, login_mode));

    if(!details.isEmpty())
    {
        if(messages_plugin->get_error_count() == 0
        && messages_plugin->get_warning_count() == 0)
        {
            // print an end user message only if the number of
            // errors/warnings is still zero

            // IMPORTANT:
            //   We have ONE error message because whatever the error we do not
            //   want to tell the user exactly what went wrong (i.e. wrong email,
            //   or wrong password.)
            //
            //   This is important because if someone is registered with an email
            //   such as example@snapwebsites.info and a hacker tries that email
            //   and gets an error message saying "wrong password," now the hacker
            //   knows that the user is registered on that Snap! C++ system.

            // user not registered yet?
            // email misspelled?
            // incorrect password?
            // email still not validated?
            //
            // TODO: Put the messages in the database so they can be translated
            messages_plugin->set_error(
                "Could Not Log You In",
                validation_required
                  ? "Your account was not yet <a href=\"/verify\" title=\"Click here to enter a verification code\">validated</a>. Please make sure to first follow the link we sent in your email. If you did not yet receive that email, we can send you another <a href=\"/verify/resend\">confirmation email</a>."
                  : "Your email or password were incorrect. If you are not registered, you may want to consider <a href=\"/register\">registering</a> first?",
                details,
                false // should this one be true?
            );
        }
        else
        {
            // in this case we only want to log the details
            // the plugin that generated errors/warnings is
            // considered to otherwise be in charge
            SNAP_LOG_WARNING("Could not log user in (but another plugin generated an error): ")(details);
        }
    }
}


/** \brief Log a user in.
 *
 * This function can be used to log a user in. You have to be extremely
 * careful to not create a way to log a user without proper credential.
 * This is generally used when a mechanism such a third party authentication
 * mechanism is used to log the user in his account.
 *
 * If the \p password parameter is empty, the system creates a user session
 * without verify the user password. This is the case where another
 * mechanism must have been used to properly log the user before calling
 * this function.
 *
 * The function still verifies that the user was properly verified and
 * not blocked. It also makes sure that the user password does not need
 * to be changed. If a password change is required for that user, then
 * the login fails.
 *
 * \param[in] key  The key of the user: i.e. his email address
 * \param[in] password  The password to log the user in.
 * \param[in] validation_required  Whether the user needs to validate his account.
 * \param[in] login_mode  The mode used to log in: full, verification.
 *
 * \return A string representing an error, an empty string if the login worked
 *         and the user is not being redirected.
 */
QString users::login_user(QString const & key, QString const& password, bool & validation_required, login_mode_t login_mode)
{
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());

    if(users_table->exists(key))
    {
        QtCassandra::QCassandraRow::pointer_t row(users_table->row(key));

        QtCassandra::QCassandraValue value;

        // existing users have a unique identifier
        QtCassandra::QCassandraValue const user_identifier(row->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
        if(user_identifier.size() != sizeof(int64_t))
        {
            messages::messages::instance()->set_error(
                "Could Not Log You In",
                "Somehow your user identifier is not available. Without it we cannot log your in.",
                QString("users::login_user() could not load the user identifier, the row exists but the cell did not make it (%1/%2).")
                             .arg(key).arg(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER)),
                false
            );
            if(login_mode == login_mode_t::LOGIN_MODE_VERIFICATION)
            {
                // force a log out because the user should not be remotely
                // logged in in any way...
                f_snap->page_redirect("logout", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            }
            else
            {
                // XXX should we redirect to some error page in that regard?
                //     (i.e. your user account is messed up, please contact us?)
                f_snap->page_redirect("verify", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
            }
            NOTREACHED();
        }
        user_logged_info_t logged_info;
        logged_info.set_identifier(user_identifier.int64Value());
        logged_info.user_ipath().set_path(QString("%1/%2")
                .arg(get_name(name_t::SNAP_NAME_USERS_PATH))
                .arg(logged_info.get_identifier()));

        // although the user exists, as in, has an account on this Snap!
        // website, that account may not be attached to this website so
        // we need to verify that before moving further.
        QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
        if(!content_table->exists(logged_info.user_ipath().get_key()))
        {
            return "it looks like you have an account on this Snap! system but not this specific website. Please register on this website and try again";
        }

        // before we actually log the user in we must make sure he is
        // not currently blocked or not yet active
        links::link_info user_status_info(get_name(name_t::SNAP_NAME_USERS_STATUS), true, logged_info.user_ipath().get_key(), logged_info.user_ipath().get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
        links::link_info status_info;
        bool force_redirect_password_change(false);
        bool valid(true);
        if(link_ctxt->next_link(status_info))
        {
            QString const site_key(f_snap->get_site_key_with_slash());

//std::cerr << "***\n*** Current user status on log in is [" << status_info.key() << "] / [" << (site_key + get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH)) << "]\n***\n";
            // the status link exists...
            // this means the user is either a new user (not yet verified)
            // or he is blocked
            // either way it means he cannot log in at this time!
            if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_NEW_PATH))
            {
                validation_required = true;
                return "user's account is not yet active (not yet verified)";
            }
            else if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_BLOCKED_PATH))
            {
                return "user's account is blocked";
            }
            else if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_AUTO_PATH))
            {
                return "user did not register, this is an auto-account only";
            }
            else if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH))
            {
                if(password.isEmpty())
                {
                    return "user has to update his password, this application cannot currently log in";
                }
                // user requested a new password but it looks like he
                // remembered the old one in between; for redirect this user
                // to the password form
                //
                // since the user knows his old password, we can log him in
                // and send him to the full fledge password change form
                //
                // note that the status will not change until the user saves
                // his new password so this redirection will happen again and
                // again until the password gets changed
                force_redirect_password_change = true;
            }
            // ignore other statuses at this point
        }
        if(valid)
        {
            bool valid_password(password.isEmpty());
            if(!valid_password)
            {
                // compute the hash of the password
                // (1) get the digest
                value = row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD_DIGEST))->value();
                QString const digest(value.stringValue());

                // (2) we need the passord (passed as a parameter now)
                //QString const password(f_snap->postenv("password"));

                // (3) get the salt in a buffer
                value = row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD_SALT))->value();
                QByteArray const salt(value.binaryValue());

                // (4) compute the expected hash
                QByteArray hash;
                encrypt_password(digest, password, salt, hash);

                // (5) retrieved the saved hash
                value = row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD))->value();
                QByteArray const saved_hash(value.binaryValue());

                // (6) compare both hashes
                // (note: at this point I don't trust the == operator of the QByteArray
                // object; will it work with '\0' bytes???)
                valid_password = hash.size() == saved_hash.size()
                              && memcmp(hash.data(), saved_hash.data(), hash.size()) == 0;
            }

            if(valid_password)
            {
                // User credentials are correct, create a session & cookie

                // log the user in by adding the correct object path
                // the other parameters were already defined in the
                // on_process_cookies() function
                f_info->set_object_path("/user/" + key);
                f_info->set_login_limit(f_snap->get_start_time() + 3600 * 3); // 3 hours (XXX: needs to become a parameter)
                sessions::sessions::instance()->save_session(*f_info, true); // force new random session number

                // if there was another active login for that very user,
                // we want to cancel it and also display a message to the
                // user about the fact
                QString const previous_session(row->cell(get_name(name_t::SNAP_NAME_USERS_LOGIN_SESSION))->value().stringValue());
                if(!previous_session.isEmpty() && previous_session != f_info->get_session_key())
                {
                    // Administrator can turn off that feature
                    QtCassandra::QCassandraValue const multisessions(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_MULTISESSIONS)));
                    if(multisessions.nullValue() || !multisessions.signedCharValue())
                    {
                        // close session
                        sessions::sessions::session_info old_session;
                        sessions::sessions::instance()->load_session(previous_session, old_session, false);
                        old_session.set_object_path("/user/");

                        // drop the referrer if there is one, it is a security
                        // issue to keep that info on an "explicit" log out!
                        NOTUSED(sessions::sessions::instance()->detach_from_session(old_session, get_name(name_t::SNAP_NAME_USERS_LOGIN_REFERRER)));

                        sessions::sessions::instance()->save_session(old_session, false);

                        messages::messages::instance()->set_warning(
                            "Two Sessions",
                            "We detected that you had another session opened. The other session was closed.",
                            QString("users::login_user() deleted old session \"%1\" for user \"%2\".")
                                         .arg(old_session.get_session_key())
                                         .arg(key)
                        );

                        // go on, this is not a fatal error
                    }
                }

                http_cookie cookie(f_snap, get_user_cookie_name(), QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random()));
                cookie.set_expire_in(86400 * 5);  // 5 days
                cookie.set_http_only(); // make it a tad bit safer
                f_snap->set_cookie(cookie);

                // this is now the current user
                f_user_key = key;
                // we just logged in so we are logged in
                // (although the user_logged_in() signal could log the
                // user out if something is awry)
                f_user_logged_in = true;

                // Copy the previous login date and IP to the previous fields
                if(row->exists(get_name(name_t::SNAP_NAME_USERS_LOGIN_ON)))
                {
                    row->cell(get_name(name_t::SNAP_NAME_USERS_PREVIOUS_LOGIN_ON))->setValue(row->cell(get_name(name_t::SNAP_NAME_USERS_LOGIN_ON))->value());
                }
                if(row->exists(get_name(name_t::SNAP_NAME_USERS_LOGIN_IP)))
                {
                    row->cell(get_name(name_t::SNAP_NAME_USERS_PREVIOUS_LOGIN_IP))->setValue(row->cell(get_name(name_t::SNAP_NAME_USERS_LOGIN_IP))->value());
                }

                // Save the date when the user logged in
                value.setInt64Value(f_snap->get_start_date());
                row->cell(get_name(name_t::SNAP_NAME_USERS_LOGIN_ON))->setValue(value);

                // Save the user IP address when logging in
                value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
                row->cell(get_name(name_t::SNAP_NAME_USERS_LOGIN_IP))->setValue(value);

                // Save the user latest session so we can implement the
                // "one session per user" feature (which is the default)
                row->cell(get_name(name_t::SNAP_NAME_USERS_LOGIN_SESSION))->setValue(f_info->get_session_key());

                // Tell all the other plugins that the user is now logged in
                // you may specify a URI to where the user should be sent on
                // log in, used in the redirect below, although we will go
                // to user/password whatever the path is specified here
                logged_info.set_email(key);
                user_logged_in(logged_info);

                // user got logged out by a plugin and not redirected?!
                if(!f_user_key.isEmpty())
                {
                    // make sure user locale/timezone get used on next
                    // locale/timezone access
                    locale::locale::instance()->reset_locale();

                    // send a signal that the user is ready (this signal is also
                    // sent when we have a valid cookie)
                    logged_in_user_ready();

                    if(password.isEmpty())
                    {
                        // This looks like an API login someone, we just
                        // return and let the caller handle the rest
                        return "";
                    }

                    if(force_redirect_password_change)
                    {
                        // this URI has priority over other plugins URIs
                        logged_info.set_uri("user/password");
                    }
                    else if(logged_info.get_uri().isEmpty())
                    {
                        // here we detach from the session since we want to
                        // redirect only once to that page
                        logged_info.set_uri(sessions::sessions::instance()->detach_from_session(*f_info, get_name(name_t::SNAP_NAME_USERS_LOGIN_REFERRER)));
                        if(logged_info.get_uri().isEmpty())
                        {
                            // User is now logged in, redirect him to his profile
                            //
                            // TODO: the admin needs to be able to change that
                            //       default redirect
                            logged_info.set_uri("user/me");
                        }
                    }
                    f_snap->page_redirect(logged_info.get_uri(), snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                    NOTREACHED();
                }

                // user does not have enough permission to log in?
                // (i.e. a pay for website where the account has no more
                //       credit and this very user is not responsible for
                //       the payment)
                return "good credential, invalid status according to another plugin that logged the user out immediately";
            }
            else
            {
                // user mistyped his password?
                return "invalid credentials (password doesn't match)";
            }
        }
    }

    // user mistyped his email or is not registered?
    return "invalid credentials (user with specified email does not exist)";
}


/** \fn void users::user_logged_in(user_logged_info_t& logged_info)
 * \brief Tell plugins that the user is now logged in.
 *
 * This signal is used to tell plugins that the user is now logged in.
 *
 * Note I: this signal only happens at the time the user logs in, not
 * each time the user accesses the server.
 *
 * Note II: a plugin has the capability to log the user out by calling
 * the user_logout() function; this means when your callback gets called
 * the user may not be logged in anymore! This means you should always
 * make a call as follow to verify that the user is indeed logged in
 * before making use of the user's information:
 *
 * \code
 *      if(!users::users::instance()->user_is_logged_in())
 *      {
 *          return;
 *      }
 * \endcode
 *
 * In most cases the plugins are expected to check one thing or another that
 * may be important for that user and act accordingly. If the result is that
 * the user should be sent to a specific page, then the plugin can set the
 * f_uri parameter of the logged_in parameter to that page URI.
 *
 * Note that if multiple plugins want to redirect the user, then which URI
 * should be used is not defined. We may later do a 303 where the system lets
 * the user choose which page to go to. At this time, the last plugin that
 * sets the URI has priority. Note that of course a plugin can decide not
 * to change the URI if it is already set.
 *
 * \note
 * It is important to remind you that if the system has to send the user to
 * change his password, it will do so, whether a plugin sets another URI
 * or not.
 *
 * \param[in] logged_info  The user login information.
 */


/** \brief Register a user.
 *
 * This function saves a user credential information as defined in the
 * registration form.
 *
 * This function creates a new entry in the users table and then links
 * that entry in the current website.
 *
 * \todo
 * We need to look into the best way to implement the connection with
 * the current website. We do not want all the websites to automatically
 * know about all the users (i.e. a website has a list of users, but
 * that's not all the users registered in Snap!)
 */
void users::process_register_form()
{
    messages::messages *messages(messages::messages::instance());

    // We validated the email already and we just don't need to do it
    // twice, if two users create an account "simultaneously (enough)"
    // with the same email, that's probably not a normal user (i.e. a
    // normal user would not be able to create two accounts at the
    // same time.) The email is the row key of the user table.
    QString const email(f_snap->postenv("email"));
    status_t const status(register_user(email, f_snap->postenv("password")));
    switch(status)
    {
    case status_t::STATUS_NEW:
        verify_email(email);
        messages->set_info(
            "We registered your account",
            QString("We sent you an email to \"%1\". In the email there is a link you need to follow to finish your registration.").arg(email)
        );
        // redirect the user to the verification form
        f_snap->page_redirect("verify", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
        break;

    case status_t::STATUS_VALID:
        // already exists since we found a valid entry of this user
        messages->set_error(
            "User Already Exists",
            QString("A user with email \"%1\" already exists. If it is you, then try to request a new password if you need a reminder.").arg(email),
            QString("user \"%1\" trying to register a second time.").arg(email),
            true
        );
        break;

    case status_t::STATUS_BLOCKED:
        // already exists since we found a valid entry of this user
        f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                "Access Denied",
                "You are not allowed to create an account on this website.",
                "User is blocked and doesnot have permission to create an account here.");
        NOTREACHED();
        break;

    default:
        // ???
        f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                "Access Denied",
                "You are not allowed to create an account on this website.",
                QString("register_user() returned an unexpected status (%1).").arg(static_cast<int>(status)));
        NOTREACHED();
        break;

    }
}


/** \brief Send an email so the user can log in without password.
 *
 * This process generates an email with a secure code. It is sent to the
 * user which will have to click on a link to auto-login in his account.
 * Once there, he will be forced to enter a new password (and duplicate
 * thereof).
 *
 * This only works for currently active users.
 */
void users::process_forgot_password_form()
{
    QString email(f_snap->postenv("email"));
    QString details;

    // check to make sure that a user with that email address exists
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(users_table->exists(email))
    {
        QtCassandra::QCassandraRow::pointer_t row(users_table->row(email));

        // existing users have a unique identifier
        // necessary to create the user key below
        QtCassandra::QCassandraValue const user_identifier(row->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            links::link_info status_info;
            QString status;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
                status = status_info.key();
            }
            // empty represents ACTIVE
            // or if user already requested for a new password
            QString const site_key(f_snap->get_site_key_with_slash());
            if(status == "" || status == site_key + get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH))
            {
                // Only users considered active can request a new password
                forgot_password_email(email);

                // mark the user with the types/users/password tag
                QString const link_name(get_name(name_t::SNAP_NAME_USERS_STATUS));
                bool const source_unique(true);
                links::link_info source(link_name, source_unique, user_ipath.get_key(), user_ipath.get_branch());
                QString const link_to(get_name(name_t::SNAP_NAME_USERS_STATUS));
                bool const destination_unique(false);
                content::path_info_t dpath;
                dpath.set_path(get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH));
                links::link_info destination(link_to, destination_unique, dpath.get_key(), dpath.get_branch());
                links::links::instance()->create_link(source, destination);

                // once we sent the new code, we can send the user back
                // to the verify form
                messages::messages::instance()->set_info(
                    "New Verification Email Send",
                    "We just sent you a new verification email. Please check your account and follow the verification link or copy and paste your verification code below."
                );
                f_snap->page_redirect("new-password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                NOTREACHED();
            }
            else
            {
                details = "user " + email + " is not active nor in \"new password\" mode, we do not send verification emails to such";
            }
        }
        else
        {
            details = "somehow we saw that a row existed for " + email + ", but we could not retrieve it";
        }
    }
    else
    {
        // XXX here we could test the email address and if invalid generate
        //     different details (we'd need to do that only if we get quite
        //     a few of those errors, we could then block IPs with repetitive
        //     invalid email addresses)
        //
        // probably a stupid spammer robot
        details = "user asking for forgot-password with an unknown email address: " + email;
    }

    // ONE error so whatever the reason the end user cannot really know
    // whether someone registered with that email address on our systems
    messages::messages::instance()->set_error(
        "Not an Active Account",
        "This email is not from an active account. No email was sent to you.",
        details,
        false
    );
    // no redirect, the same form will be shown again
}


/** \brief Processing the forgotten password verification code.
 *
 * This process verifies that the verification code entered is the one
 * expected for the user to correct a forgotten password.
 *
 * This works only if the user is active with a status of "password".
 * If not we assume that the user already changed his password because
 * (1) we force the user to do so if that status is on; and (2) the
 * link is removed when the new password gets saved successfully.
 */
void users::process_new_password_form()
{
    const QString session_id(f_snap->postenv("verification_code"));
    content::path_info_t ipath;
    ipath.set_path("new-password/" + session_id);
    verify_password(ipath);
}



/** \brief Save the new password assuming everything checks out.
 *
 * This saves the new password in the database and logs the user in so
 * he can go on with his work.
 */
void users::process_replace_password_form()
{
    // make sure the user is properly setup
    if(!f_user_key.isEmpty())
    {
        // user is logged in already, send him to his normal password form
        f_user_changing_password_key.clear();
        f_snap->page_redirect("user/password", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    if(f_user_changing_password_key.isEmpty())
    {
        // user is not logged in and he did not follow a valid link
        // XXX the login page is probably the best choice?
        f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // for errors if any
    QString details;

    // replace the password assuming we can find that user information
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(users_table->exists(f_user_changing_password_key))
    {
        QtCassandra::QCassandraRow::pointer_t row(users_table->row(f_user_changing_password_key));

        // existing users have a unique identifier
        // necessary to create the user key below
        QtCassandra::QCassandraValue const user_identifier(row->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
                QString const site_key(f_snap->get_site_key_with_slash());
                if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH))
                {
                    // We are good, save the new password and remove that link

                    // First encrypt the password
                    QString const password(f_snap->postenv("password"));
                    QByteArray salt;
                    QByteArray hash;
                    QtCassandra::QCassandraValue digest(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_PASSWORD_DIGEST)));
                    if(digest.nullValue())
                    {
                        digest.setStringValue("sha512");
                    }
                    create_password_salt(salt);
                    encrypt_password(digest.stringValue(), password, salt, hash);

                    // Save the hashed password (never the original password!)
                    QtCassandra::QCassandraValue value;
                    value.setBinaryValue(hash);
                    row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD))->setValue(value);

                    // Save the password salt (otherwise we couldn't check whether the user
                    // knows his password!)
                    value.setBinaryValue(salt);
                    row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD_SALT))->setValue(value);

                    // Also save the digest since it could change en-route
                    row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD_DIGEST))->setValue(digest);

                    int64_t const start_date(f_snap->get_start_date());
                    row->cell(get_name(name_t::SNAP_NAME_USERS_MODIFIED))->setValue(start_date);

                    // Unlink from the password tag too
                    links::links::instance()->delete_link(user_status_info);

                    // Now we auto-log in the user... the session should
                    // already be adequate from the on_process_cookies()
                    // call
                    //
                    // TODO to make this safer we really need the extra 3 questions
                    //      and ask them when the user request the new password or
                    //      when he comes back in the replace password form
                    f_info->set_object_path("/user/" + f_user_changing_password_key);
                    f_info->set_login_limit(f_snap->get_start_time() + 3600 * 3); // 3 hours (XXX: needs to become a parameter)
                    sessions::sessions::instance()->save_session(*f_info, true); // force a new random session number

                    http_cookie cookie(f_snap, get_user_cookie_name(), QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random()));
                    cookie.set_expire_in(86400 * 5);  // 5 days
                    cookie.set_http_only(); // make it a tad bit safer
                    f_snap->set_cookie(cookie);

                    f_user_changing_password_key.clear();

                    content::content::instance()->modified_content(user_ipath);

                    // once we sent the new code, we can send the user back
                    // to the verify form
                    messages::messages::instance()->set_info(
                        "Password Changed",
                        "Your new password was saved. Next time you want to log in, you can use your email with this new password."
                    );

                    // TBD: should we use the saved login redirect instead?
                    //      (if not then we probably want to clear it)
                    f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                    NOTREACHED();
                }

                details = "user " + f_user_changing_password_key + " is not new (maybe it is active, blocked, auto...), we do not send verification emails to such";
            }
            else
            {
                // This happens for all users already active, users who are
                // blocked, etc.
                details = "user " + f_user_changing_password_key + " is currently active, we do not send verification emails to such";
            }
        }
        else
        {
            details = "somehow we saw that a row existed for " + f_user_changing_password_key + ", but we could not retrieve the user identifier";
        }
    }
    else
    {
        details = "user " + f_user_changing_password_key + " does not exist in the users table";
    }

    // we're done with this variable
    // we have to explicitly clear it or it may stay around for a long time
    // (i.e. it gets saved in the session table)
    f_user_changing_password_key.clear();

    messages::messages::instance()->set_error(
        "Not a Valid Account",
        "Somehow an error occured while we were trying to update your account password.",
        details,
        false
    );

    // XXX the login page is probably the best choice?
    f_snap->page_redirect("login", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
    NOTREACHED();
}


/** \brief Process the password form.
 *
 * This function processes the password form. It verifies that the
 * old_password is correct. If so, it saves the new password in the
 * user's account.
 *
 * The function then redirects the user to his profile (user/me).
 */
void users::process_password_form()
{
    // make sure the user is properly setup
    if(f_user_key.isEmpty())
    {
        // user is not even logged in!?
        f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // for errors if any
    QString details;

    // replace the password assuming we can find that user information
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(users_table->exists(f_user_key))
    {
        // We're good, save the new password and remove that link
        QtCassandra::QCassandraRow::pointer_t row(users_table->row(f_user_key));

        // existing users have a unique identifier
        // necessary to create the user key below
        QtCassandra::QCassandraValue const user_identifier(row->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            bool delete_password_status(false);
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
                QString const site_key(f_snap->get_site_key_with_slash());
                if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_BLOCKED_PATH)
                || status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_AUTO_PATH)
                || status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_NEW_PATH))
                {
                    // somehow the user is not blocked or marked as auto...
                    f_snap->die(snap_child::http_code_t::HTTP_CODE_FORBIDDEN,
                            "Access Denied", "You need to be logged in and have enough permissions to access this page.",
                            "User attempt to change a password in his account which is currently blocked.");
                    NOTREACHED();
                }
                else if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH))
                {
                    // we will be able to delete this one
                    delete_password_status = true;
                }
            }

            // TODO make sure that the new password is not the same as the
            //      last X passwords, including the old_password/new_password
            //      variables as defined here

            // compute the hash of the old password to make sure the user
            // knows his password
            //
            // (1) get the digest
            QtCassandra::QCassandraValue value(row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD_DIGEST))->value());
            QString const old_digest(value.stringValue());

            // (2) we need the passord:
            QString const old_password(f_snap->postenv("old_password"));

            // (3) get the salt in a buffer
            value = row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD_SALT))->value();
            QByteArray const old_salt(value.binaryValue());

            // (4) compute the expected hash
            QByteArray old_hash;
            encrypt_password(old_digest, old_password, old_salt, old_hash);

            // (5) retrieved the saved hashed password
            value = row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD))->value();
            QByteArray const saved_hash(value.binaryValue());

            // (6) verify that it matches
            if(old_hash.size() == saved_hash.size()
            && memcmp(old_hash.data(), saved_hash.data(), old_hash.size()) == 0)
            {
                // The user entered his old password properly
                // save the new password
                QString new_password(f_snap->postenv("new_password"));
                QtCassandra::QCassandraValue new_digest(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_PASSWORD_DIGEST)));
                if(new_digest.nullValue())
                {
                    new_digest.setStringValue("sha512");
                }
                QByteArray new_salt;
                create_password_salt(new_salt);
                QByteArray new_hash;
                encrypt_password(new_digest.stringValue(), new_password, new_salt, new_hash);

                // Save the hashed password (never the original password!)
                value.setBinaryValue(new_hash);
                row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD))->setValue(value);

                // Save the password salt (otherwise we couldn't check whether the user
                // knows his password!)
                value.setBinaryValue(new_salt);
                row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD_SALT))->setValue(value);

                // also save the digest since it could change en-route
                row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD_DIGEST))->setValue(new_digest);

                // Unlink from the password tag too
                if(delete_password_status)
                {
                    links::links::instance()->delete_link(user_status_info);
                }

                content::content::instance()->modified_content(user_ipath);

                // once we sent the new code, we can send the user back
                // to the verify form
                messages::messages::instance()->set_info(
                    "Password Changed",
                    "Your new password was saved. Next time you want to log in, you must use your email with this new password."
                );
                QString referrer(sessions::sessions::instance()->detach_from_session(*f_info, get_name(name_t::SNAP_NAME_USERS_LOGIN_REFERRER)));
                if(referrer == "user/password")
                {
                    // ignore the default redirect if it is to this page
                    referrer.clear();
                }
                if(referrer.isEmpty())
                {
                    // Redirect user to his profile
                    f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                }
                else
                {
                    // If the user logged in when he needed to still change
                    // his password, then there may very be a referrer path
                    f_snap->page_redirect(referrer, snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                }
                NOTREACHED();
            }
            else
            {
                messages::messages::instance()->set_error(
                    "Invalid Password",
                    "The password your entered as your old password is not correct. Please try again.",
                    "user is trying to change his password and he mistyped his existing password",
                    false
                );
                return;
            }
        }
        else
        {
            details = "somehow we saw that a row existed for " + f_user_key + ", but we could not retrieve the user identifier";
        }
    }
    else
    {
        details = "user " + f_user_key + " does not exist in the users table";
    }

    messages::messages::instance()->set_error(
        "Not a Valid Account",
        "Somehow an error occured while we were trying to update your account password.",
        details,
        false
    );

    // XXX the profile page is probably the best choice?
    f_snap->page_redirect("user/me", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
    NOTREACHED();
}


/** \brief "Resend" the verification email.
 *
 * This function runs whenever a user requests the system to send an
 * additional verification code a given email address.
 *
 * Before we proceed, we verify that the user status is "new" (tag
 * as such.) If not, we generate an error and prevent the email from
 * being sent.
 */
void users::process_verify_resend_form()
{
    QString const email(f_snap->postenv("email"));
    QString details;

    // check to make sure that a user with that email address exists
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(users_table->exists(email))
    {
        QtCassandra::QCassandraRow::pointer_t row(users_table->row(email));

        // existing users have a unique identifier
        // necessary to create the user key below
        QtCassandra::QCassandraValue const user_identifier(row->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(get_name(name_t::SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
                QString const site_key(f_snap->get_site_key_with_slash());
                if(status_info.key() == site_key + get_name(name_t::SNAP_NAME_USERS_NEW_PATH))
                {
                    // Only new users are allowed to get another verification email
                    verify_email(email);
                    // once we sent the new code, we can send the user back
                    // to the verify form
                    messages::messages::instance()->set_info(
                        "New Verification Email Send",
                        "We just sent you a new verification email. Please check your account and follow the verification link or copy and paste your verification code below."
                    );
                    f_snap->page_redirect("verify", snap_child::http_code_t::HTTP_CODE_SEE_OTHER);
                    NOTREACHED();
                }

                details = "user " + email + " is not new (maybe it is active, blocked, auto...), we do not send verification emails to such";
            }
            else
            {
                // This happens for all users already active, users who are
                // blocked, etc.
                details = "user " + email + " is currently active, we do not send verification emails to such";
            }
        }
        else
        {
            details = "somehow we saw that a row existed for " + email + ", but we could not retrieve it";
        }
    }
    else
    {
        // XXX here we could test the email address and if invalid generate
        //     different details (we'd need to do that only if we get quite
        //     a few of those errors, we could then block IPs with repetitive
        //     invalid email addresses)
        //
        // probably a stupid spammer robot
        details = "user asking for verify-resend with an unknown email address: " + email;
    }

    // ONE error so whatever the reason the end user cannot really know
    // whether someone registered with that email address on our systems
    messages::messages::instance()->set_error(
        "Not a New Account",
        "This email is not from a new account. It may be from an already active account, or from someone who never registered with us, or someone who is currently blocked. <strong>No verification email was sent.</strong>",
        details,
        false
    );
    // no redirect, the same form will be shown again
}


/** \brief Process the verification code.
 *
 * This function runs the verify_user() function with the code that the
 * user entered in the form. This is similar to going to the
 * verify/\<verification_code> page to get an account confirmed.
 *
 * The verification code gets "simplified" as in all spaces get removed.
 * The code cannot include spaces anyway and when someone does a copy &
 * paste, at times, a space is added at the end. This way, such spaces
 * will be ignored.
 */
void users::process_verify_form()
{
    // verify the code the user entered, the verify_user() function
    // will automatically redirect us if necessary; we should
    // get an error if redirect to ourselves
    QString verification_code(f_snap->postenv("verification_code"));
    content::path_info_t ipath;
    ipath.set_path("verify/" + verification_code.simplified());
    verify_user(ipath);
}


/** \brief Get the registered (MAYBE NOT LOGGED IN) user key.
 *
 * WARNING WARNING WARNING
 * This returns the user key which is his email address. It does not
 * tell you that the user is logged in. For that purpose you MUST
 * use the user_is_logged_in() function.
 *
 * This function returns the key of the user that last logged
 * in. This key is the user's email address. Remember that by default a
 * user is not considered fully logged in if his sesion his more than
 * 3 hours old. You must make sure to check the user_is_logged_in()
 * too. Note that the permission system should already take care of
 * most of those problems for you anyway, but you need to know what
 * you are doing!
 *
 * If the user is not recognized, then his key is the empty string. This
 * is a fast way to know whether the current user is logged in, registed,
 * or just a visitor:
 *
 * \code
 * if(users::users::instance()->get_user_key().isEmpty())
 * {
 *   // anonymous visitory user code
 * }
 * else if(users::users::instance()->user_is_logged_in())
 * {
 *   // user recently logged in (last 3 hours by default)
 *   // here you can process "dangerous / top-secret" stuff
 * }
 * else
 * {
 *   // registered user code
 *   // user logged in more than 3 hours ago and is now considered
 *   // a registered user, opposed to a logged in user who can
 *   // make changes to his account, etc.
 * }
 * \endcode
 *
 * \note
 * We return a copy of the key, opposed to a const reference, because really
 * it is too dangerous to allow someone from the outside to temper with this
 * variable.
 *
 * WARNING WARNING WARNING
 * This returns the user key which is his email address. It does not
 * tell you that the user is logged in. For that purpose you MUST
 * use the user_is_logged_in() function.
 *
 * \return The user email address (which is the user key in the users table).
 */
QString users::get_user_key() const
{
    return f_user_key;
}


/** \brief Get the user path.
 *
 * This function gets the user path in the content. If the user is not
 * logged in, the function returns "user" which represents the anonymous
 * user.
 *
 * \warning
 * The path returned may NOT be from a logged in user. We may know the
 * user key (his email address) and yet not have a logged in user. Whether
 * the user is logged in needs to be checked with the user_is_logged_in()
 * function.
 *
 * \note
 * To test whether the returned value represents the anonymous user,
 * please make use  of get_name() with name_t::SNAP_NAME_USERS_ANONYMOUS_PATH.
 *
 * \return The path to the currently logged in user or "user".
 */
QString users::get_user_path() const
{
    if(!f_user_key.isEmpty())
    {
        QtCassandra::QCassandraTable::pointer_t users_table(const_cast<users *>(this)->get_users_table());
        if(users_table->exists(f_user_key))
        {
            QtCassandra::QCassandraValue const value(users_table->row(f_user_key)->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
            if(!value.nullValue())
            {
                int64_t const identifier(value.int64Value());
                return QString("%1/%2").arg(get_name(name_t::SNAP_NAME_USERS_PATH)).arg(identifier);
            }
        }
    }
    return get_name(name_t::SNAP_NAME_USERS_ANONYMOUS_PATH);
}


/** \brief Get the current user identifer.
 *
 * This function gets the user identifier. If we do not have the user key
 * (his email address) then the function returns 0 (i.e. anonymous user).
 *
 * \warning
 * The identifier returned may NOT be from a logged in user. We may know the
 * user key (his email address) and yet not have a logged in user. Whether
 * the user is logged in needs to be checked with the user_is_logged_in()
 * function.
 *
 * \return The identifer of the current user.
 */
int64_t users::get_user_identifier() const
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


/** \brief Check the current status of the specified user.
 *
 * This function checks the status of the user specified by an
 * email address.
 *
 * \note
 * The function returns STATUS_UNDEFINED if the email address is
 * the empty string.
 *
 * \note
 * The function returns STATUS_UNKNOWN if the status is not known
 * by the users plugin. The status itself is saved in the status_key
 * parameter so one can further check what the status is and act on
 * it appropriately.
 *
 * \todo
 * Allow the use of the user path and user identifier instead of
 * just the email address.
 *
 * \param[in] email  The email address of the user being checked.
 * \param[out] status_key  Return the status key if available.
 *
 * \return The status of the user.
 */
users::status_t users::user_status(QString const & email, QString & status_key)
{
    status_key.clear();

    if(email.isEmpty())
    {
        return status_t::STATUS_UNDEFINED;
    }

    QString const user_path(get_user_path(email));
    if(user_path.isEmpty())
    {
        return status_t::STATUS_NOT_FOUND;
    }
    content::path_info_t user_ipath;
    user_ipath.set_path(user_path);

    // before we actually accept this verification code, we must make sure
    // the user is still marked as a new user (he should or the session
    // would be invalid, but for security it is better to check again)
    links::link_info user_status_info(get_name(name_t::SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
    links::link_info status_info;
    if(!link_ctxt->next_link(status_info))
    {
        // if the status link does not exist, then the user is considered
        // verified and valid
        return status_t::STATUS_VALID;
    }
    status_key = status_info.key();

    // a status link exists... check that the user is not marked as a NEW user
    QString const site_key(f_snap->get_site_key_with_slash());
    if(status_key == site_key + get_name(name_t::SNAP_NAME_USERS_NEW_PATH))
    {
        return status_t::STATUS_NEW;
    }
    if(status_key == site_key + get_name(name_t::SNAP_NAME_USERS_BLOCKED_PATH))
    {
        return status_t::STATUS_BLOCKED;
    }
    if(status_key == site_key + get_name(name_t::SNAP_NAME_USERS_AUTO_PATH))
    {
        return status_t::STATUS_AUTO;
    }
    if(status_key != site_key + get_name(name_t::SNAP_NAME_USERS_PASSWORD_PATH))
    {
        return status_t::STATUS_PASSWORD;
    }

    // anything else we do not know what the heck it is
    // (we'll need a signal to allow for extensions by other plugins)
    return status_t::STATUS_UNKNOWN;
}


/** \brief Retrieve the user identifier from its user path.
 *
 * This function parses the path to a user's account and return its
 * identifier (i.e. the number after the slash in "user/123".)
 *
 * The path may include the site key as well. It will be ignored as expected.
 *
 * WARNING: This function does NOT return the current user identifier.
 * It returns the identifier of the user path passed as a parameter.
 *
 * \note
 * The current user identifier can be retrieved using the get_user_identifier()
 * function with no parameters.
 *
 * \param[in] user_path  The path to the user.
 *
 * \return The user identifier if it worked, -1 if the path is invalid
 *         and does not represent a user identifier.
 */
int64_t users::get_user_identifier(QString const & user_path) const
{
    QString const site_key(f_snap->get_site_key_with_slash());
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


/** \brief Given a user path, return his email address.
 *
 * This function transforms the specified user path and transforms it
 * in his identifier and then it calls the other get_user_email()
 * function.
 *
 * The user path may or not include the site key. Both cases function
 * perfectly.
 *
 * \param[in] user_path  The path to the user data in the content table.
 *
 * \return The email address of the user, if the user is defined in the
 *         database.
 */
QString users::get_user_email(QString const & user_path)
{
    return get_user_email(get_user_identifier(user_path));
}


/** \brief Given a user identifier, return his email address.
 *
 * The email address of a user is the key used to access his private
 * data in the users table.
 *
 * Note that an invalid identifier will make this function return an
 * empty string (i.e. not such user.)
 *
 * \param[in] identifier  The identifier of the user to retrieve the email for.
 *
 * \return The email address of the user, if the user is defined in the
 *         database.
 */
QString users::get_user_email(int64_t const identifier)
{
    if(identifier > 0)
    {
        QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
        QtCassandra::QCassandraRow::pointer_t row(users_table->row(get_name(name_t::SNAP_NAME_USERS_INDEX_ROW)));

        QByteArray key;
        QtCassandra::appendInt64Value(key, identifier);
        if(row->exists(key))
        {
            // found the email
            return row->cell(key)->value().stringValue();
        }
    }

    return "";
}


/** \brief Get the path to a user from an email.
 *
 * This function returns the path of the user corresponding to the specified
 * email. The function returns an empty string if the user is not found.
 *
 * \param[in] email  The email of the user to search the path for.
 *
 * \return The path to the user.
 */
QString users::get_user_path(QString const & email)
{
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(users_table->exists(email))
    {
        QtCassandra::QCassandraRow::pointer_t row(users_table->row(email));
        QtCassandra::QCassandraValue const value(users_table->row(email)->cell(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!value.nullValue())
        {
            int64_t const identifier(value.int64Value());
            return QString("%1/%2").arg(get_name(name_t::SNAP_NAME_USERS_PATH)).arg(identifier);
        }
    }

    return "";
}


/** \brief Register a new user in the database
 *
 * If you find out that a user is not yet registered but still want to
 * save some information about that user (i.e. when sending an email to
 * someone) then this function is used for that purpose.
 *
 * This function accepts an email and a password. The password can be set
 * to "!" to prevent that user from logging in (password too small!) but
 * still have an account. The account can later be activated, which
 * happens whenever the user decides to register "for real" (i.e. the
 * "!" accounts are often used for users added to mailing lists and alike.)
 *
 * If you are creating a user as an administrator or similar role, you
 * may want to give the user a full account. This is doable by creating
 * a random password and passing that password to this function. The
 * user will be considered fully registered in that case. The password
 * can be generated using the create_password() function.
 *
 * \param[in] email  The email of the user. It must be a valid email address.
 * \param[in] password  The password of the user or "!".
 *
 * \return STATUS_NEW if the user was just created and a verification email
 *         is expected to be sent to him or her;
 *         STATUS_VALID if the user was accepted in this website and already
 *         verified his email address;
 *         STATUS_BLOCKED if this email address is blocked on this website
 *         or entire Snap! environment or the user already exists but was
 *         blocked by an administrator;
 */
users::status_t users::register_user(QString const& email, QString const& password)
{
    // make sure that the user email is valid
    f_snap->verify_email(email);

    QByteArray salt;
    QByteArray hash;
    QtCassandra::QCassandraValue digest(f_snap->get_site_parameter(get_name(name_t::SNAP_NAME_USERS_PASSWORD_DIGEST)));
    if(password == "!")
    {
        // special case; these users cannot log in
        // (probably created because they signed up to a newsletter or comments)
        digest.setStringValue("no password");
        salt = "no salt";
        hash = "!";
    }
    else
    {
        if(digest.nullValue())
        {
            digest.setStringValue("sha512");
        }
        create_password_salt(salt);
        encrypt_password(digest.stringValue(), password, salt, hash);
    }

    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    QtCassandra::QCassandraRow::pointer_t row(users_table->row(email));

    QtCassandra::QCassandraValue value;
    value.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
    value.setStringValue(email);

    int64_t identifier(0);
    status_t status(status_t::STATUS_NEW);
    bool new_user(false);
    QString const id_key(get_name(name_t::SNAP_NAME_USERS_ID_ROW));
    QString const identifier_key(get_name(name_t::SNAP_NAME_USERS_IDENTIFIER));
    QString const email_key(get_name(name_t::SNAP_NAME_USERS_ORIGINAL_EMAIL));
    QString const user_path(get_name(name_t::SNAP_NAME_USERS_PATH));
    QtCassandra::QCassandraValue new_identifier;
    new_identifier.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);

    // we got as much as we could ready before locking
    {
        // first make sure this email is unique
        QtCassandra::QCassandraLock lock(f_snap->get_context(), email);

        // TODO: we have to look at all the possible email addresses
        QtCassandra::QCassandraCell::pointer_t cell(row->cell(email_key));
        cell->setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
        QtCassandra::QCassandraValue const email_data(cell->value());
        if(!email_data.nullValue())
        {
            // TODO: move this case under the locked block since
            //       the lock is not necessary to do this work
            //
            // "someone else" already registered with that email
            // first check whether that user exists on this website
            QtCassandra::QCassandraValue const existing_identifier(row->cell(identifier_key)->value());
            if(existing_identifier.size() != sizeof(int64_t))
            {
                // this means no user can register until this value gets
                // fixed somehow!
                messages::messages::instance()->set_error(
                    "Failed Creating User Account",
                    "Somehow we could not determine your user identifier. Please try again later.",
                    "users::register_user() could not load the identifier of an existing user, the user seems to exist but the users::identifier cell seems wrong ("
                                 + email + "/" + identifier_key + ").",
                    false
                );
                // XXX redirect user to an error page instead?
                //     if they try again it will fail again until the
                //     database gets fixed properly...
                return status_t::STATUS_UNDEFINED;
            }
            identifier = existing_identifier.int64Value();

            // okay, so the user exists on at least one website
            // check whether it exists on this website and if not add it
            //
            // TBD: should we also check the cell with the website reference
            //      in the user table? (users::website_reference::<site_key>)
            content::path_info_t existing_ipath;
            existing_ipath.set_path(QString("%1/%2").arg(user_path).arg(identifier));
            if(content_table->exists(existing_ipath.get_key()))
            {
                // it exists, just return the current status of that existing user
                QString ignore_status_key;
                return user_status(email, ignore_status_key);
            }
            // user exists in the Snap! system but not this website
            // so we want to add it to this website, but we will return
            // its current status "instead" of STATUS_NEW (note that
            // the current status could be STATUS_NEW if the user
            // registered in another website but did not yet verify his
            // email address.)
            status = status_t::STATUS_VALID;
        }
        else
        {
            // Note that the email was already checked when coming from the Register
            // form, however, it was checked for validity as an email, not checked
            // against a black list or verified in other ways; also the password
            // can this way be checked by another plugin (i.e. password database)
            content::permission_flag secure;
            check_user_security(email, password, secure);
            if(!secure.allowed())
            {
                // well... someone said "do not save that user in there"!
                return status_t::STATUS_BLOCKED;
            }

            // we are the first to lock this row, the user is therefore unique
            // so go on and register him

            // Save the first email the user had when registering
            row->cell(email_key)->setValue(value);

            // In order to register the user in the contents we want a
            // unique identifier for each user, for that purpose we use
            // a special row in the users table and since we have a lock
            // we can safely do a read-increment-write cycle.
            if(users_table->exists(id_key))
            {
                QtCassandra::QCassandraRow::pointer_t id_row(users_table->row(id_key));
                QtCassandra::QCassandraCell::pointer_t id_cell(id_row->cell(identifier_key));
                id_cell->setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
                QtCassandra::QCassandraValue const current_identifier(id_cell->value());
                if(current_identifier.size() != sizeof(int64_t))
                {
                    // this means no user can register until this value gets
                    // fixed somehow!
                    messages::messages::instance()->set_error(
                        "Failed Creating User Account",
                        "Somehow we could not generate a user identifier for your account. Please try again later.",
                        "users::register_user() could not load the *id_row* identifier, the row exists but the cell did not make it ("
                                     + id_key + "/" + identifier_key + ").",
                        false
                    );
                    // XXX redirect user to an error page instead?
                    //     if they try again it will fail again until the
                    //     database gets fixed properly...
                    return status_t::STATUS_UNDEFINED;
                }
                identifier = current_identifier.int64Value();
            }
            ++identifier;
            new_user = true;
            new_identifier.setInt64Value(identifier);
            users_table->row(id_key)->cell(identifier_key)->setValue(new_identifier);
        }
        // the lock automatically goes away here
    }

    // WARNING: if this breaks, someone probably changed the value
    //          content; it should be the user email
    uint64_t const created_date(f_snap->get_start_date());
    if(new_user)
    {
        users_table->row(get_name(name_t::SNAP_NAME_USERS_INDEX_ROW))->cell(new_identifier.binaryValue())->setValue(value);

        // Save the user identifier in his user account so we can easily find
        // the content user for that user account/email
        row->cell(identifier_key)->setValue(new_identifier);

        // Save the hashed password (never the original password!)
        value.setBinaryValue(hash);
        row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD))->setValue(value);

        // Save the password salt (otherwise we couldn't check whether the user
        // knows his password!)
        value.setBinaryValue(salt);
        row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD_SALT))->setValue(value);

        // also save the digest since it could change en-route
        row->cell(get_name(name_t::SNAP_NAME_USERS_PASSWORD_DIGEST))->setValue(digest);

        // Save the user IP address when registering
        value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
        row->cell(get_name(name_t::SNAP_NAME_USERS_ORIGINAL_IP))->setValue(value);

        // Date when the user was created (i.e. now)
        // if that field does not exist yet (it could if the user unsubscribe
        // from a mailing list or something similar)
        if(!row->exists(get_name(name_t::SNAP_NAME_USERS_CREATED_TIME)))
        {
            row->cell(get_name(name_t::SNAP_NAME_USERS_CREATED_TIME))->setValue(created_date);
        }
    }

    // Add a reference back to the website were the user is being added so
    // that way we can generate a list of such websites in the user's account
    // the reference appears in the cell name and the value is the time when
    // the user registered for that website
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const website_reference(QString("%1::%2")
            .arg(get_name(name_t::SNAP_NAME_USERS_WEBSITE_REFERENCE))
            .arg(site_key));
    row->cell(website_reference)->setValue(created_date);

    // Now create the user in the contents
    // (nothing else should be create at the path until now)
    content::path_info_t user_ipath;
    user_ipath.set_path(QString("%1/%2").arg(user_path).arg(identifier));
    content::content *content_plugin(content::content::instance());
    snap_version::version_number_t const branch_number(content_plugin->get_current_user_branch(user_ipath.get_key(), "", true));
    user_ipath.force_branch(branch_number);
    // default revision when creating a new branch
    user_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
    user_ipath.force_locale("xx");
    content_plugin->create_content(user_ipath, get_plugin_name(), "user-page");

    // mark when the user was created in the branch
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
    QtCassandra::QCassandraRow::pointer_t branch_row(branch_table->row(user_ipath.get_branch_key()));
    branch_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(created_date);

    // save a default title and body
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(user_ipath.get_revision_key()));
    revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED))->setValue(created_date);
    // no title or body by default--other plugins could set those to the
    //                              user name or other information
    QString const empty_string;
    revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_TITLE))->setValue(empty_string);
    revision_row->cell(content::get_name(content::name_t::SNAP_NAME_CONTENT_BODY))->setValue(empty_string);

    // if already marked as valid, for sure do not mark this user as new!?
    if(status != status_t::STATUS_VALID)
    {
        // The "public" user account (i.e. in the content table) is limited
        // to the identifier at this point
        //
        // however, we also want to include a link defined as the status
        // at first the user is marked as being new
        // the destination URL is defined in the <link> content
        QString const link_name(get_name(name_t::SNAP_NAME_USERS_STATUS));
        bool const source_unique(true);
        // TODO: determine whether "xx" is the correct locale here (we could also
        //       have "" and a default website language...) -- this is the
        //       language of the profile, not the language of the website...
        links::link_info source(link_name, source_unique, user_ipath.get_key(), user_ipath.get_branch(true, "xx"));
        QString const link_to(get_name(name_t::SNAP_NAME_USERS_STATUS));
        bool const destination_unique(false);
        content::path_info_t dpath;
        dpath.set_path(get_name(name_t::SNAP_NAME_USERS_NEW_PATH));
        links::link_info destination(link_to, destination_unique, dpath.get_key(), dpath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // last time the user data was modified
    row->cell(get_name(name_t::SNAP_NAME_USERS_MODIFIED))->setValue(created_date);

    user_registered(user_ipath, identifier);

    return status;
}


/** \fn void users::check_user_security(QString const& email, QString const& password, content::permission_flag& secure)
 * \brief Signal that a user is about to get a new account.
 *
 * This signal is called before a new user gets created.
 *
 * \warning
 * At this point this signal is sent when the user account is still locked.
 * This means you MUST return (i.e. avoid calling die() because it does
 * not return...) and the SEGV, BUS, ILL signals will block that user in
 * lock mode forever. This may block the software when it tries to create
 * another user... so be careful.
 *
 * \param[in] email  The email of the user about to be registered
 * \param[in] password  The user password.
 * \param[in,out] secure  The flag defining whether the flag is secure.
 */


/** \fn void users::user_registered(content::path_info_t& ipath, int64_t identifier)
 * \brief Signal telling other plugins that a user just registered.
 *
 * Note that this signal is sent when the user was registered and NOT when
 * the user verified his account. This means the user is not really fully
 * authorized on the system yet.
 *
 * \param[in,out] ipath  The path to the new user's account (/user/\<identifier\>)
 * \param[in] identifier  The user identifier.
 */


/** \brief Send an email to request email verification.
 *
 * This function generates an email and sends it. The email is used to request
 * the user to verify that he receives said emails.
 *
 * \param[in] email  The user email.
 */
void users::verify_email(QString const& email)
{
    sendmail::sendmail::email e;

    // mark priority as High
    e.set_priority(sendmail::sendmail::email::email_priority_t::EMAIL_PRIORITY_HIGH);

    // destination email address
    e.add_header(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_TO), email);

    e.add_parameter(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST), "true");

    // add the email subject and body using a page
    e.set_email_path("admin/email/users/verify");

    // verification makes use of a session identifier
    sessions::sessions::session_info info;
    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_VERIFY_EMAIL);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    //info.set_page_path(); -- default is okay
    info.set_object_path("/user/" + email);
    info.set_user_agent(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
    info.set_time_to_live(86400 * 3);  // 3 days
    QString const session(sessions::sessions::instance()->create_session(info));
    e.add_parameter(get_name(name_t::SNAP_NAME_USERS_VERIFY_EMAIL), session);

    // to allow a "resend" without regenerating a new session, we save
    // the session identifier--since those are short lived, it will anyway
    // not be extremely useful, but some systems may use that once in a while
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    //if(!users_table->exists(f_user_key)) ... ?
    QtCassandra::QCassandraValue session_value(session);
    int64_t const ttl(86400 * 3 - 86400 / 2); // keep in the database for a little less than the session itself
    session_value.setTtl(ttl);
    users_table->row(email)->cell(get_name(name_t::SNAP_NAME_USERS_LAST_VERIFICATION_SESSION))->setValue(session_value);

    // send the email
    //
    // really this just saves it in the database, the sendmail itself
    // happens on the backend; see sendmail::on_backend_action()
    sendmail::sendmail::instance()->post_email(e);
}


/** \brief Resend a verification email.
 *
 * This function is a repeat of the verify_email() function. That is,
 * by default it attempts to reuse the same session information to
 * send the verification email to the user. It is generally used by
 * an administrator who registered a user on their behalf and is told
 * that the user did not receive their verification email.
 *
 * If the function is called too long after the session was created,
 * it will be erased by Cassandra so a new session gets created
 * instead. Unfortunately, there is no information to the end user
 * if that happens.
 *
 * If the verification email is not sent, then the function returns false.
 * This specifically happens if the users table does not have a user
 * with the specified email.
 *
 * \param[in] email  The user email.
 *
 * \return true if the email was sent, false otherwise.
 */
bool users::resend_verification_email(QString const& email)
{
    // to allow a "resend" without regenerating a new session, we save
    // the session identifier--since those are short lived, it will anyway
    // not be extremely useful, but some systems may use that once in a while
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(!users_table->exists(email))
    {
        return false;
    }
    QString const session(users_table->row(email)->cell(get_name(name_t::SNAP_NAME_USERS_LAST_VERIFICATION_SESSION))->value().stringValue());
    if(session.isEmpty())
    {
        verify_email(email);
        return true;
    }

    sendmail::sendmail::email e;

    // mark priority as High
    e.set_priority(sendmail::sendmail::email::email_priority_t::EMAIL_PRIORITY_HIGH);

    e.add_parameter(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST), "true");

    // destination email address
    e.add_header(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_TO), email);

    // add the email subject and body using a page
    e.set_email_path("admin/email/users/verify");

    // verification makes use of the existing session identifier
    e.add_parameter(get_name(name_t::SNAP_NAME_USERS_VERIFY_EMAIL), session);

    // send the email
    //
    // really this just saves it in the database, the sendmail itself
    // happens on the backend; see sendmail::on_backend_action()
    sendmail::sendmail::instance()->post_email(e);

    return true;
}


/** \brief Send an email to allow the user to change his password.
 *
 * This function generates an email and sends it to an active user. The
 * email is used to allow the user to change his password without having
 * to enter an old password.
 *
 * \param[in] email  The user email.
 */
void users::forgot_password_email(QString const& email)
{
    sendmail::sendmail::email e;

    // administrator can define this email address
    QtCassandra::QCassandraValue from(f_snap->get_site_parameter(get_name(snap::name_t::SNAP_NAME_CORE_ADMINISTRATOR_EMAIL)));
    if(from.nullValue())
    {
        from.setStringValue("contact@snapwebsites.com");
    }
    e.set_from(from.stringValue());

    // mark priority as High
    e.set_priority(sendmail::sendmail::email::email_priority_t::EMAIL_PRIORITY_HIGH);

    e.add_parameter(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_BYPASS_BLACKLIST), "true");

    // destination email address
    e.add_header(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_TO), email);

    // add the email subject and body using a page
    e.set_email_path("admin/email/users/forgot-password");

    // verification makes use of a session identifier
    sessions::sessions::session_info info;
    info.set_session_type(sessions::sessions::session_info::session_info_type_t::SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_FORGOT_PASSWORD_EMAIL);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    //info.set_page_path(); -- default is okay
    info.set_object_path("/user/" + email);
    info.set_user_agent(f_snap->snapenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
    info.set_time_to_live(3600 * 8);  // 8 hours
    QString const session(sessions::sessions::instance()->create_session(info));
    e.add_parameter(get_name(name_t::SNAP_NAME_USERS_FORGOT_PASSWORD_EMAIL), session);

    // send the email
    //
    // really this just saves it in the database, the sendmail itself
    // happens on the backend; see sendmail::on_backend_action()
    sendmail::sendmail::instance()->post_email(e);
}


/** \brief Get a constant reference to the session information.
 *
 * This function can be used to retrieve a reference to the session
 * information of the current user. Note that could be an anonymous
 * user. It is up to you to determine whether the user is logged in
 * if the intend is to use the session information only of logged in
 * users.
 *
 * \return A constant reference to this user session information.
 */
sessions::sessions::session_info const& users::get_session() const
{
    if(f_info)
    {
        return *f_info;
    }

    throw snap_logic_exception("users::get_sessions() called when the session point is still nullptr");
}


/** \brief Save the specified data to the user session.
 *
 * This function is used to attach data to the current user session so it
 * can be retrieved on a later request. Note that the detach_from_session()
 * will also delete the data from the session as it is expected to only be
 * used once. If you need it again, then call the attach_to_session()
 * function again (in the grand scheme of things it should be 100%
 * automatic!)
 *
 * The \p name parameter should be qualified (i.e. "messages::messages").
 *
 * The data to be attached must be in the form of a string. If you are saving
 * a large structure, or set of structures, make sure to use serialization
 * first.
 *
 * \note
 * The data string cannot be an empty string. Cassandra does not like that
 * and on read, an empty string is viewed as "that data is undefined."
 *
 * \param[in] name  The name of the cell that is to be used to save the data.
 * \param[in] data  The data to save in the session.
 *
 * \sa detach_from_session()
 */
void users::attach_to_session(QString const& name, QString const& data)
{
    sessions::sessions::instance()->attach_to_session(*f_info, name, data);
}


/** \brief Retrieve the specified data from the user session.
 *
 * This function is used to retrieve data that was previously attached
 * to the user session with a call to the attach_to_session() function.
 *
 * Note that the data retreived in this way is deleted from the session
 * since we do not want to offer this data more than once (although in
 * some cases it may be necessary to do so, then the attach_to_session()
 * should be called again.)
 *
 * \note
 * The function is NOT a constant since it modifies the database by
 * deleting the data being detached.
 *
 * \param[in] name  The name of the cell that is to be used to save the data.
 *
 * \return The data read from the session if any, otherwise an empty string.
 *
 * \sa attach_to_session()
 */
QString users::detach_from_session(QString const & name)
{
    return sessions::sessions::instance()->detach_from_session(*f_info, name);
}


/** \brief Retrieve data that was attached to the user session.
 *
 * This function can be used to read a session entry from the user session
 * without having to detach that information from the session. This is
 * useful in cases where data is expected to stay in the session for
 * long period of time (i.e. the cart of a user).
 *
 * If no data was attached to that named session field, then the function
 * returns an empty string. Remember that saving an empty string as session
 * data is not possible.
 *
 * \param[in] name  The name of the parameter to retrieve.
 *
 * \return The data attached to the named session field.
 */
QString users::get_from_session(QString const& name) const
{
    return sessions::sessions::instance()->get_from_session(*f_info, name);
}


/** \brief Set the referrer path for the current session.
 *
 * Call this function instead of
 *
 * \code
 *      attach_to_session( name_t::SNAP_NAME_USERS_LOGIN_REFERRER, path );
 * \endcode
 *
 * This way we can make sure that a certain number of paths never get
 * saved for the log in redirect.
 *
 * \note
 * The special cases "/login" and "/logout" will do nothing, since we
 * do not want a referrer in those cases.
 *
 * \note
 * This function ensures that the path gets canonicalized before it
 * gets used.
 *
 * \param[in] path  The path to the page being viewed as the referrer.
 *
 * \sa attach_to_session()
 * \sa detach_from_session()
 */
void users::set_referrer( QString path )
{
    // this is acceptable and it happens
    //
    // (note that if you want to go to the home page, you may want
    // to use f_snap->get_site_key_with_slash() instead of "" or "/")
    if(path.isEmpty())
    {
        return;
    }

    // canonicalize the path
    content::path_info_t ipath;
    ipath.set_path(path);
    path = ipath.get_key();  // make sure it is canonicalized

    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    if(!content_table->exists(ipath.get_key())
    && ipath.get_real_key().isEmpty())
    {
        // TODO: dynamic pages are expected to end up as a "real key" entry
        //       we will need to do more tests to make sure this works as
        //       expected, although this code should work already
        //
        SNAP_LOG_ERROR("path \"")(path)("\" was not found in the database?!");
        return;
    }

    // check whether this is our current page
    content::path_info_t main_ipath;
    main_ipath.set_path(f_snap->get_uri().path());
    if(path == main_ipath.get_key())
    {
        // this is the main page, verify it is not an AJAX path
        // because redirects to those fail big time
        // (we really need a much stronger way of testing such!)
        //
        // TBD:  the fact that the request is AJAX does not 100%
        //       of the time mean that it could not be a valid
        //       referrer, but close enough at this point
        //
        if(server_access::server_access::instance()->is_ajax_request())
        {
            return;
        }
    }

    // if the page is linked to the "not-main-page" type, then it cannot
    // be a referrer so we drop it right here (this is used by pages such
    // as boxes and other pages that are not expected to become main pages)
    // note that this does not prevent one from going to the page, only
    // the system will not redirect one to such a page
    QString const link_name(get_name(name_t::SNAP_NAME_USERS_NOT_MAIN_PAGE));
    links::link_info not_main_page_info(link_name, true, path, ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(not_main_page_info));
    links::link_info type_info;
    if(link_ctxt->next_link(type_info))
    {
        return;
    }

    // use the current refererrer if there is one as the redirect page
    // after log in; once the log in is complete, redirect to this referrer
    // page; if you send the user on a page that only redirects to /login
    // then the user will end up on his profile (/user/me)
    //
    char const *loginref_name( get_name(name_t::SNAP_NAME_USERS_LOGIN_REFERRER) );
    if( sessions::sessions::instance()->get_from_session( *f_info, loginref_name ).isEmpty() )
    {
        SNAP_LOG_DEBUG() << "name_t::SNAP_NAME_USERS_LOGIN_REFERRER being set to " << path << " for page path " << f_info->get_page_path();

        // verify that it is not /login or /logout because those cause
        // real problems!
        QString const site_key(f_snap->get_site_key_with_slash());
        if( path != site_key + "login"
         && path != site_key + "logout")
        {
            // everything okay!
            attach_to_session( loginref_name, path );
        }
    }
}


/** \brief Save the user session identifier on password change.
 *
 * To avoid loggin people before they are done changing their password,
 * so that way they cannot go visit all the private pages on the website,
 * we use a session variable to save the information about the user who
 * is changing his password.
 */
void users::on_attach_to_session()
{
    if(!f_user_changing_password_key.isEmpty())
    {
        attach_to_session(get_name(name_t::SNAP_NAME_USERS_CHANGING_PASSWORD_KEY), f_user_changing_password_key);
    }

    // the messages handling is here because the messages plugin cannot have
    // a dependency on the users plugin
    messages::messages *messages_plugin(messages::messages::instance());
    if(messages_plugin->get_message_count() > 0)
    {
        // note that if we lose those "website" messages,
        // they will still be in our logs
        QString const data(messages_plugin->serialize());
        attach_to_session(messages::get_name(messages::name_t::SNAP_NAME_MESSAGES_MESSAGES), data);
        messages_plugin->clear_messages();
    }
}


/** \brief Retrieve data that was attached to a session.
 *
 * This function is the opposite of the on_attach_to_session(). It is
 * called before the execute() to reinitialize objects that previously
 * saved data in the user session.
 */
void users::on_detach_from_session()
{
    // TODO:
    // here we probably should do a get_from_session() because we may need
    // the variable between several different forms before it really gets
    // deleted permanently; (i.e. we are reattaching now, but if a crash
    // occurs between the detach and attach, we lose the information!)
    // the concerned function(s) should clear() the variable when
    // officially done with it
    f_user_changing_password_key = detach_from_session(get_name(name_t::SNAP_NAME_USERS_CHANGING_PASSWORD_KEY));

    // the message handling is here because the messages plugin cannot have
    // a dependency on the users plugin which is the one handling the session
    QString const data(detach_from_session(messages::get_name(messages::name_t::SNAP_NAME_MESSAGES_MESSAGES)));
    if(!data.isEmpty())
    {
        messages::messages::instance()->unserialize(data);
    }
}


/** \brief Get the user selected language if user did that.
 *
 * The user can select the language in which he will see most of the
 * website (assuming most was translated in those languages.)
 *
 * \param[in,out] locales  Locales as defined by the user.
 */
void users::on_define_locales(QString& locales)
{
    if(!f_user_key.isEmpty())
    {
        QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
        if(users_table->exists(f_user_key))
        {
            QtCassandra::QCassandraValue const value(users_table->row(f_user_key)->cell(get_name(name_t::SNAP_NAME_USERS_LOCALES))->value());
            if(!value.nullValue())
            {
                if(locales.isEmpty())
                {
                    locales = value.stringValue();
                }
                else
                {
                    locales += ',';
                    locales += value.stringValue();
                }
            }
        }
    }
}


/** \brief Create a default password.
 *
 * In some cases an administrator may want to create an account for a user
 * which should then have a valid, albeit unknown, password.
 *
 * This function can be used to create that password.
 *
 * It is strongly advised to NOT send such passwords to the user via email
 * because they may contain "strange" characters and emails are notoriously
 * not safe.
 *
 * \todo
 * Look into defining a set of character in each language instead of
 * just basic ASCII.
 *
 * \return The string with the new password.
 */
QString users::create_password()
{
    // a "large" set of random bytes
    const int PASSWORD_SIZE = 256;
    unsigned char buf[PASSWORD_SIZE];

    QString result;
    do
    {
        // get the random bytes
        RAND_bytes(buf, sizeof(buf));

        for(int i(0); i < PASSWORD_SIZE; ++i)
        {
            // only use ASCII characters
            if(buf[i] >= ' ' && buf[i] < 0x7F)
            {
                result += buf[i];
            }
        }
    }
    while(result.length() < 64); // just in case, make sure it is long enough

    return result;
}


/** \brief Create a new salt for a password.
 *
 * Every time you get to encrypt a new password, call this function to
 * get a new salt. This is important to avoid having the same hash for
 * the same password for multiple users.
 *
 * Imagine a user creating 3 accounts and each time using the exact same
 * password. Just using an md5sum it would encrypt that password to
 * exactly the same 16 bytes. In other words, if you crack one, you
 * crack all 3 (assuming you have access to the database you can
 * immediately see that all those accounts have the exact same password.)
 *
 * The salt prevents such problems. Plus we add 256 bits of completely
 * random entropy to the digest used to encrypt the passwords. This
 * in itself makes it for a much harder to decrypt hash.
 *
 * The salt is expected to be saved in the database along the password.
 *
 * \param[out] salt  The byte array receiving the new salt.
 */
void users::create_password_salt(QByteArray& salt)
{
    // we use 16 bytes before and 16 bytes after the password
    // so create a salt of SALT_SIZE bytes (256 bits at time of writing)
    unsigned char buf[SALT_SIZE];
    /*int r(*/ RAND_bytes(buf, sizeof(buf));
    salt.clear();
    salt.append(reinterpret_cast<char *>(buf), sizeof(buf));
}


/** \brief Encrypt a password.
 *
 * This function generates a strong hash of a user password to prevent
 * easy brute force "decryption" of the password. (i.e. an MD5 can be
 * decrypted in 6 hours, and a SHA1 password, in about 1 day, with a
 * $100 GPU as of 2012.)
 *
 * Here we use 2 random salts (using RAND_bytes() which is expected to
 * be random enough for encryption like algorithms) and the specified
 * digest to encrypt (okay, hash--a one way "encryption") the password.
 *
 * Read more about hash functions on
 * http://ehash.iaik.tugraz.at/wiki/The_Hash_Function_Zoo
 *
 * \exception users_exception_size_mismatch
 * This exception is raised if the salt byte array is not exactly SALT_SIZE
 * bytes. For new passwords, you want to call the create_password_salt()
 * function to create the salt buffer.
 *
 * \exception users_exception_digest_not_available
 * This exception is raised if any of the OpenSSL digest functions fail.
 * This include an invalid digest name and adding/retrieving data to/from
 * the digest.
 *
 * \param[in] digest  The name of the digest to use (i.e. "sha512").
 * \param[in] password  The password to encrypt.
 * \param[in] salt  The salt information, necessary to encrypt passwords.
 * \param[out] hash  The resulting password hash.
 */
void users::encrypt_password(QString const& digest, QString const& password, QByteArray const& salt, QByteArray& hash)
{
    // it is an out only so reset it immediately
    hash.clear();

    // verify the size
    if(salt.size() != SALT_SIZE)
    {
        throw users_exception_size_mismatch("salt buffer must be exactly SALT_SIZE bytes (missed calling create_password_salt()?)");
    }
    unsigned char buf[SALT_SIZE];
    memcpy(buf, salt.data(), SALT_SIZE);

    // Initialize so we gain access to all the necessary digests
    OpenSSL_add_all_digests();

    // retrieve the digest we want to use
    // (TODO: allows website owners to change this value)
    const EVP_MD *md(EVP_get_digestbyname(digest.toUtf8().data()));
    if(md == nullptr)
    {
        throw users_exception_digest_not_available("the specified digest could not be found");
    }

    // initialize the digest context
    EVP_MD_CTX mdctx;
    EVP_MD_CTX_init(&mdctx);
    if(EVP_DigestInit_ex(&mdctx, md, nullptr) != 1)
    {
        throw users_exception_encryption_failed("EVP_DigestInit_ex() failed digest initialization");
    }

    // add first salt
    if(EVP_DigestUpdate(&mdctx, buf, SALT_SIZE / 2) != 1)
    {
        throw users_exception_encryption_failed("EVP_DigestUpdate() failed digest update (salt1)");
    }

    // add password (encrypt to UTF-8)
    const char *pwd(password.toUtf8().data());
    if(EVP_DigestUpdate(&mdctx, pwd, strlen(pwd)) != 1)
    {
        throw users_exception_encryption_failed("EVP_DigestUpdate() failed digest update (password)");
    }

    // add second salt
    if(EVP_DigestUpdate(&mdctx, buf + SALT_SIZE / 2, SALT_SIZE / 2) != 1)
    {
        throw users_exception_encryption_failed("EVP_DigestUpdate() failed digest update (salt2)");
    }

    // retrieve the result of the hash
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len(EVP_MAX_MD_SIZE);
    if(EVP_DigestFinal_ex(&mdctx, md_value, &md_len) != 1)
    {
        throw users_exception_encryption_failed("EVP_DigestFinal_ex() digest finalization failed");
    }
    hash.append(reinterpret_cast<char *>(md_value), md_len);

    // clean up the context
    // (note: the return value is not documented so we ignore it)
    EVP_MD_CTX_cleanup(&mdctx);
}


/** \brief Replace a token with a corresponding value.
 *
 * This function replaces the users tokens with their value. In some cases
 * the values were already computed in the XML document, so all we have to do is query
 * the XML and return the corresponding value.
 *
 * The supported tokens are:
 *
 * \li users::email -- the user email as is
 * \li users::email_anchor -- the user email as an anchor (mailto:)
 * \li users::since -- the date and time when the user registered
 *
 * \param[in,out] ipath  The path to the page being worked on.
 * \param[in] plugin_owner  The plugin that owns this ipath content.
 * \param[in,out] xml  The XML document used with the layout.
 * \param[in,out] token  The token object, with the token name and optional parameters.
 */
void users::on_replace_token(content::path_info_t& ipath, QString const& plugin_owner, QDomDocument& xml, filter::filter::token_info_t& token)
{
    NOTUSED(ipath);
    NOTUSED(plugin_owner);
    NOTUSED(xml);

    if(!token.is_namespace("users::"))
    {
        // not a users plugin token
        return;
    }

    bool const users_picture(token.is_token("users::picture"));
    if(users_picture)
    {
        SNAP_LOG_TRACE() << "first is_token(\"users::picture\")";
        // setup as the default image by default
        token.f_replacement = "<img src=\"/images/users/default-user-image.png\" alt=\"Default user picture\" width=\"32\" height=\"32\"/>";
    }

    if(f_user_key.isEmpty())
    {
        // user not logged in
        return;
    }

    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(!users_table->exists(f_user_key))
    {
        // cannot find user...
        return;
    }

    if(token.is_token("users::email"))
    {
        token.f_replacement = f_user_key;
        return;
    }

    if(token.is_token("users::email_anchor"))
    {
        // TODO: replace f_user_key with the user first/last names when
        //       available AND authorized
        token.f_replacement = "<a href=\"mailto:" + f_user_key + "\">" + f_user_key + "</a>";
        return;
    }

    // anything else requires the user to be verified
    QtCassandra::QCassandraValue const verified_on(users_table->row(f_user_key)->cell(get_name(name_t::SNAP_NAME_USERS_VERIFIED_ON))->value());
    if(verified_on.nullValue())
    {
        // not verified yet
        return;
    }

    if(token.is_token("users::since"))
    {
        // make sure that the user created and verified his account
        QtCassandra::QCassandraValue const value(users_table->row(f_user_key)->cell(get_name(name_t::SNAP_NAME_USERS_CREATED_TIME))->value());
        int64_t date(value.int64Value());
        token.f_replacement = QString("%1 %2")
                .arg(f_snap->date_to_string(date, f_snap->date_format_t::DATE_FORMAT_SHORT))
                .arg(f_snap->date_to_string(date, f_snap->date_format_t::DATE_FORMAT_TIME));
        // else use was not yet verified
        return;
    }

    if(token.is_token("users::picture"))
    {
        // make sure that the user created and verified his account
        QtCassandra::QCassandraValue const value(users_table->row(f_user_key)->cell(get_name(name_t::SNAP_NAME_USERS_PICTURE))->value());
        if(!value.nullValue())
        {
            SNAP_LOG_TRACE() << "second is_token(\"users::picture\")";

            // TBD: not sure right now how we will offer those
            //      probably with a special path that tells us
            //      to go look in the users' table
            //
            //      We may also want to only offer the Avatar for
            //      user picture(s)
            //
            token.f_replacement = QString("<img src=\"...\"/>");
        }
    }
}


/** \brief Determine whether the current user is considered to be a spammer.
 *
 * This function checks the user IP address and if black listed, then we
 * return true meaning that we consider that user as a spammer. This limits
 * access to the bare minimum which generally are:
 *
 * \li The home page
 * \li The privacy policy
 * \li The terms and conditions
 * \li The files referenced by those items (CSS, JavaScript, images, etc.)
 *
 * \return true if the user is a considered to be a spammer.
 */
bool users::user_is_a_spammer()
{
    // TODO implement the actual test
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    char const * const black_list(get_name(name_t::SNAP_NAME_USERS_BLACK_LIST));
    if(users_table->exists(black_list))
    {
        // the row exists, check the IP
        // TODO canonicalize the IP address as an IPv6 so it matches whatever
        //      the system we're on
        QString const ip(f_snap->snapenv("REMOTE_ADDR"));
        QtCassandra::QCassandraRow::pointer_t row(users_table->row(black_list));
        if(row->exists(ip))
        {
            // "unfortunately" this user is marked as a spammer
            return true;
        }
    }
    return false;
}


/** \brief Whether the user was logged in recently.
 *
 * This function MUST be called to know whether the user is a logged in
 * user or just a registered user with a valid session.
 *
 * What's the difference really?
 *
 * \li A user who logged in within the last 3 hours (can be changed) has
 *     more permissions; for example he can see all his account details
 *     and edit them.
 * \li A user who is just a registered user can only see the publicly
 *     visible information from his account and he has no way to edit
 *     anything without first going to the verify credential page.
 */
bool users::user_is_logged_in()
{
    return f_user_logged_in;
}


/** \brief Improves the error signature.
 *
 * This function adds the user profile link to the brief signature of die()
 * errors. This is done only if the user is logged in.
 *
 * \param[in] path  The path to the page that generated the error.
 * \param[in] doc  The DOM document.
 * \param[in,out] signature_tag  The DOM element where signature anchors are added.
 */
void users::on_improve_signature(QString const & path, QDomDocument doc, QDomElement signature_tag)
{
    NOTUSED(path);

    if(!f_user_key.isEmpty())
    {
        // add a space between the previous link and this one
        snap_dom::append_plain_text_to_node(signature_tag, " ");

        // add a link to the user account
        QDomElement a_tag(doc.createElement("a"));
        a_tag.setAttribute("class", "user-account");
        a_tag.setAttribute("target", "_top");
        a_tag.setAttribute("href", QString("/%1").arg(get_user_path()));
        // TODO: translate
        snap_dom::append_plain_text_to_node(a_tag, "My Account");

        signature_tag.appendChild(a_tag);
    }
}


/** \brief Signal called when a plugin requests the locale to be set.
 *
 * This signal is called whenever a plugin requests that the locale be
 * set before using a function that is affected by locale parameters.
 *
 * This very function setups the locale to the user locale if the
 * user is logged in.
 *
 * If the function is called before the user is logged in, then nothing
 * happens. The users plugin makes sure to reset the locale information
 * once the user gets logged in.
 */
void users::on_set_locale()
{
    // we may have a user defined locale
    QString const user_path(get_user_path());
    if(user_path != get_name(name_t::SNAP_NAME_USERS_ANONYMOUS_PATH))
    {
        content::content * content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

        content::path_info_t user_ipath;
        user_ipath.set_path(user_path);

        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(user_ipath.get_revision_key()));
        QString const user_locale(revision_row->cell(get_name(name_t::SNAP_NAME_USERS_LOCALE))->value().stringValue());
        if(!user_locale.isEmpty())
        {
            locale::locale::instance()->set_current_locale(user_locale);
        }
    }
}


/** \brief Signal called when a plugin requests the timezone to be set.
 *
 * This signal is called whenever a plugin requests that the timezone be
 * set before using a function that is affected by the timezone parameter.
 *
 * This very function setups the timezone to the user timezone if the
 * user is logged in.
 *
 * If the function is called before the user is logged in, then nothing
 * happens. The users plugin makes sure to reset the timezone information
 * once the user gets logged in.
 */
void users::on_set_timezone()
{
    // we may have a user defined timezone
    QString const user_path(get_user_path());
    if(!user_path.isEmpty())
    {
        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

        content::path_info_t user_ipath;
        user_ipath.set_path(user_path);

        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(user_ipath.get_revision_key()));
        QString const user_timezone(revision_row->cell(get_name(name_t::SNAP_NAME_USERS_TIMEZONE))->value().stringValue());
        if(!user_timezone.isEmpty())
        {
            locale::locale::instance()->set_current_timezone(user_timezone);
        }
    }
}


/** \brief Repair the author link.
 *
 * When cloning a page, we repair the author link and then add
 * a "cloned by" link to the current user.
 *
 * The "cloned by" link does NOT ever get "repaired".
 */
void users::repair_link_of_cloned_page(QString const& clone, snap_version::version_number_t branch_number, links::link_info const& source, links::link_info const& destination, bool const cloning)
{
    NOTUSED(cloning);

    if(source.name() == get_name(name_t::SNAP_NAME_USERS_AUTHOR)
    && destination.name() == get_name(name_t::SNAP_NAME_USERS_AUTHORED_PAGES))
    {
        links::link_info src(get_name(name_t::SNAP_NAME_USERS_AUTHOR), true, clone, branch_number);
        links::links::instance()->create_link(src, destination);
    }
    // else ...
    // users also have a status, but no one should allow a user to be cloned
    // and thus the status does not need to be handled here (what would we
    // do really with it here? mark the user as blocked?)
}


/** \brief Check whether the cell can securily be used in a script.
 *
 * This signal is sent by the cell() function of snap_expr objects.
 * The plugin receiving the signal can check the table, row, and cell
 * names and mark that specific cell as secure. This will prevent the
 * script writer from accessing that specific cell.
 *
 * In case of the content plugin, this is used to protect all contents
 * in the secret table.
 *
 * The \p secure flag is used to mark the cell as secure. Simply call
 * the mark_as_secure() function to do so.
 *
 * \param[in] table  The table being accessed.
 * \param[in] accessible  Whether the cell is secure.
 *
 * \return This function returns true in case the signal needs to proceed.
 */
void users::on_table_is_accessible(QString const & table_name, server::accessible_flag_t & accessible)
{
    if(table_name == get_name(name_t::SNAP_NAME_USERS_TABLE))
    {
        // the users table includes the user passwords, albeit
        // encrypted, we just do not ever want to share any of
        // that
        //
        accessible.mark_as_secure();
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
