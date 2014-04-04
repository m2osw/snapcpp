// Snap Websites Server -- users handling
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
#include "../messages/messages.h"
#include "../sendmail/sendmail.h"

#include "qstring_stream.h"
#include "not_reached.h"
#include "log.h"

#include <iostream>

#include <QtCassandra/QCassandraLock.h>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <boost/static_assert.hpp>
#include <QFile>

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
    case SNAP_NAME_USERS_ANONYMOUS_PATH:
        return "user";

    case SNAP_NAME_USERS_AUTHOR:
        return "users::author";

    case SNAP_NAME_USERS_AUTHORED_PAGES:
        return "users::authored_pages";

    case SNAP_NAME_USERS_AUTO_PATH:
        return "types/users/auto";

    case SNAP_NAME_USERS_BLACK_LIST:
        return "*black_list*";

    case SNAP_NAME_USERS_BLOCKED_PATH:
        return "types/users/blocked";

    case SNAP_NAME_USERS_CHANGING_PASSWORD_KEY:
        return "users::changing_password_key";

    case SNAP_NAME_USERS_CREATED_TIME:
        return "users::created_time";

    case SNAP_NAME_USERS_FORGOT_PASSWORD_EMAIL:
        return "users::forgot_password_email";

    case SNAP_NAME_USERS_FORGOT_PASSWORD_IP:
        return "users::forgot_password_ip";

    case SNAP_NAME_USERS_FORGOT_PASSWORD_ON:
        return "users::forgot_password_on";

    case SNAP_NAME_USERS_IDENTIFIER:
        return "users::identifier";

    case SNAP_NAME_USERS_ID_ROW:
        return "*id_row*";

    case SNAP_NAME_USERS_INDEX_ROW:
        return "*index_row*";

    case SNAP_NAME_USERS_LOCALES:
        return "users::locales";

    case SNAP_NAME_USERS_LOGIN_IP:
        return "users::login_ip";

    case SNAP_NAME_USERS_LOGIN_ON:
        return "users::login_on";

    case SNAP_NAME_USERS_LOGIN_REFERRER:
        return "users::login_referrer";

    case SNAP_NAME_USERS_LOGOUT_IP:
        return "users::logout_ip";

    case SNAP_NAME_USERS_LOGOUT_ON:
        return "users::logout_on";

    case SNAP_NAME_USERS_NEW_PATH:
        return "types/users/new";

    case SNAP_NAME_USERS_ORIGINAL_EMAIL:
        return "users::original_email";

    case SNAP_NAME_USERS_ORIGINAL_IP:
        return "users::original_ip";

    case SNAP_NAME_USERS_PASSWORD:
        return "users::password";

    case SNAP_NAME_USERS_PASSWORD_DIGEST:
        return "users::password::digest";

    case SNAP_NAME_USERS_PASSWORD_PATH:
        return "types/users/password";

    case SNAP_NAME_USERS_PASSWORD_SALT:
        return "users::password::salt";

    case SNAP_NAME_USERS_PATH:
        return "user";

    case SNAP_NAME_USERS_PICTURE:
        return "users::picture";

    case SNAP_NAME_USERS_PREVIOUS_LOGIN_IP:
        return "users::previous_login_ip";

    case SNAP_NAME_USERS_PREVIOUS_LOGIN_ON:
        return "users::previous_login_on";

    // WARNING: We do not use a statically defined name!
    //          To be more secure each Snap! website can use a different
    //          cookie name; possibly one that changes over time and
    //          later by user...
    //case SNAP_NAME_USERS_SESSION_COOKIE:
    //    // cookie names cannot include ':' so I use "__" to represent
    //    // the namespace separation
    //    return "users__snap_session";

    case SNAP_NAME_USERS_STATUS:
        return "users::status";

    case SNAP_NAME_USERS_TABLE:
        return "users";

    case SNAP_NAME_USERS_USERNAME:
        return "users::username";

    case SNAP_NAME_USERS_VERIFIED_IP:
        return "users::verified_ip";

    case SNAP_NAME_USERS_VERIFIED_ON:
        return "users::verified_on";

    case SNAP_NAME_USERS_VERIFY_EMAIL:
        return "users::verify_email";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_USERS_...");

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

    SNAP_PLUGIN_UPDATE(2014, 4, 1, 0, 28, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
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
    static_cast<void>(variables_timestamp);
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
    return f_snap->create_table(get_name(SNAP_NAME_USERS_TABLE), "Global users table.");
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
    SNAP_LISTEN(users, "server", server, improve_signature, _1, _2);
    SNAP_LISTEN(users, "server", server, cell_is_secure, _1, _2, _3, _4);
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
    QString user_cookie_name(f_snap->get_site_parameter(snap::get_name(SNAP_NAME_CORE_USER_COOKIE_NAME)).stringValue());
    if(user_cookie_name.isEmpty())
    {
        // user cookie name not yet assigned or reset so a new name
        // gets assigned
        unsigned char buf[COOKIE_NAME_SIZE];
        int r(RAND_bytes(buf, sizeof(buf)));
        if(r != 1)
        {
            f_snap->die(snap_child::HTTP_CODE_SERVICE_UNAVAILABLE,
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
        f_snap->set_site_parameter(snap::get_name(SNAP_NAME_CORE_USER_COOKIE_NAME), user_cookie_name);
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
 */
void users::on_process_cookies()
{
    bool create_new_session(true);

    // get cookie name
    QString const user_cookie_name(get_user_cookie_name());

    // any snap session?
    if(f_snap->cookie_is_defined(user_cookie_name))
    {
        // is that session a valid user session?
        QString session_cookie(f_snap->cookie(user_cookie_name));
        QStringList parameters(session_cookie.split("/"));
        QString session_key(parameters[0]);
        QString random_key; // no random key?
        if(parameters.size() > 1)
        {
            random_key = parameters[1];
        }
        sessions::sessions::instance()->load_session(session_key, *f_info, false);
        const QString path(f_info->get_object_path());
        if(f_info->get_session_type() == sessions::sessions::session_info::SESSION_INFO_VALID
        && f_info->get_session_id() == USERS_SESSION_ID_LOG_IN_SESSION
        && f_info->get_session_random() == random_key.toInt()
        && f_info->get_user_agent() == f_snap->snapenv(snap::get_name(SNAP_NAME_CORE_HTTP_USER_AGENT))
        && path.left(6) == "/user/")
        {
            // this session qualifies as a log in session
            // so now verify the user
            const QString key(path.mid(6));
            // not authenticated user?
            if(!key.isEmpty())
            {
                QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
                if(users_table->exists(key))
                {
                    // this is a valid user email address!
                    QString uri_path(f_snap->get_uri().path());
                    if(uri_path == "/logout" || uri_path.left(8) == "/logout/")
                    {
                        // the user is requesting to log out, here we avoid
                        // dealing with all the session information again
                        // inside the user_logout() function and this way
                        // we right away cancel the session
                        f_info->set_object_path("/user/");

                        QtCassandra::QCassandraRow::pointer_t row(users_table->row(key));

                        // Save the date when the user logged out
                        QtCassandra::QCassandraValue value;
                        value.setInt64Value(f_snap->get_start_date());
                        row->cell(get_name(SNAP_NAME_USERS_LOGOUT_ON))->setValue(value);

                        // Save the user IP address when logged out
                        value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
                        row->cell(get_name(SNAP_NAME_USERS_LOGOUT_IP))->setValue(value);
                    }
                    else
                    {
                        f_user_key = key;

                        // the user still has a valid session, but he may not
                        // be fully logged in... (i.e. not have as much
                        // permission as given with a fresh log in) -- we need
                        // an additional form to authorize the user to do more
                        f_user_logged_in = f_snap->get_start_time() < f_info->get_login_limit();
                    }
                }
            }
            create_new_session = false;
        }
    }

    // create or refresh the session
    if(create_new_session)
    {
        // create a new session
        f_info->set_session_type(sessions::sessions::session_info::SESSION_INFO_USER);
        f_info->set_session_id(USERS_SESSION_ID_LOG_IN_SESSION);
        f_info->set_plugin_owner(get_plugin_name()); // ourselves
        //f_info->set_page_path(); -- default is fine, we do not use the path
        f_info->set_object_path("/user/"); // no user id for the anonymous user
        f_info->set_user_agent(f_snap->snapenv(snap::get_name(SNAP_NAME_CORE_HTTP_USER_AGENT)));
        f_info->set_time_to_live(86400 * 5);  // 5 days
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
        bool const new_random(f_info->get_date() + 60 * 5 * 1000000 < f_snap->get_start_date());
        sessions::sessions::instance()->save_session(*f_info, new_random);
    }

    http_cookie cookie(
            f_snap,
            user_cookie_name,
            QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random())
        );
    cookie.set_expire_in(86400 * 5);  // 5 days
    cookie.set_http_only(); // make it a tad bit safer
    f_snap->set_cookie(cookie);
//printf("session id [%s]\n", f_info->get_session_key().toUtf8().data());
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
void users::on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info)
{
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
    if(ipath.get_cpath() == "user"                      // list of (public) users
    || ipath.get_cpath().left(5) == "user/"             // show a user profile (user/ is followed by the user identifier or some edit page such as user/password)
    || ipath.get_cpath() == "profile"                   // the logged in user profile
    || ipath.get_cpath() == "login"                     // form to log user in
    || ipath.get_cpath() == "logout"                    // log user out
    || ipath.get_cpath() == "register"                  // form to let new users register
    || ipath.get_cpath() == "verify-credentials"        // re-log user in
    || ipath.get_cpath() == "verify"                    // verification form so the user can enter his code
    || ipath.get_cpath().left(7) == "verify/"           // link to verify user's email; and verify/resend form
    || ipath.get_cpath() == "forgot-password"           // form for users to reset their password
    || ipath.get_cpath() == "new-password"              // form for users to enter their forgotten password verification code
    || ipath.get_cpath().left(13) == "new-password/")   // form for users to enter their forgotten password verification code
    {
        // tell the path plugin that this is ours
        plugin_info.set_plugin(this);
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


void users::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    if(ipath.get_cpath() == "user")
    {
        // TODO: write user listing
        //list_users(body);
        return;
    }
    else if(ipath.get_cpath() == "user/password/replace")
    {
        // this is a very special form that is accessible by users who
        // requested to change the password with the "forgot password"
        prepare_replace_password_form(body);
    }
    else if(ipath.get_cpath().left(5) == "user/")
    {
        show_user(ipath, page, body);
        return;
    }
    //else if(ipath.get_cpath() == "profile")
    //{
    //    // TODO: write user profile editor
    //    //       this is /user, /user/###, and /user/me at this point
    //    //user_profile(body);
    //    return;
    //}
    else if(ipath.get_cpath() == "login")
    {
        prepare_login_form();
    }
    else if(ipath.get_cpath() == "verify-credentials")
    {
        prepare_verify_credentials_form();
    }
    else if(ipath.get_cpath() == "logout")
    {
        // closing current session if any and show the logout page
        logout_user(ipath, page, body);
        return;
    }
    else if(ipath.get_cpath() == "register"
         || ipath.get_cpath() == "verify"
         || ipath.get_cpath() == "verify/resend")
    {
        prepare_basic_anonymous_form();
    }
    else if(ipath.get_cpath() == "forgot-password")
    {
        prepare_forgot_password_form();
    }
    else if(ipath.get_cpath() == "new-password")
    {
        prepare_new_password_form();
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
    static_cast<void>(ipath);
    static_cast<void>(ctemplate);

    QDomDocument doc(header.ownerDocument());

    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());

    // retrieve the row for that user
    if(!f_user_key.isEmpty() && users_table->exists(f_user_key))
    {
        QtCassandra::QCassandraRow::pointer_t user_row(users_table->row(f_user_key));

        {   // snap/head/metadata/desc[type=users::email]/data
            QDomElement desc(doc.createElement("desc"));
            desc.setAttribute("type", "users::email");
            metadata.appendChild(desc);
            QDomElement data(doc.createElement("data"));
            desc.appendChild(data);
            QDomText text(doc.createTextNode(f_user_key));
            data.appendChild(text);
        }

        {   // snap/head/metadata/desc[type=users::name]/data
            QtCassandra::QCassandraValue value(user_row->cell(get_name(SNAP_NAME_USERS_USERNAME))->value());
            if(!value.nullValue())
            {
                QDomElement desc(doc.createElement("desc"));
                desc.setAttribute("type", "users::name");
                metadata.appendChild(desc);
                QDomElement data(doc.createElement("data"));
                desc.appendChild(data);
                QDomText text(doc.createTextNode(value.stringValue()));
                data.appendChild(text);
            }
        }

        {   // snap/head/metadata/desc[type=users::created]/data
            QtCassandra::QCassandraValue value(user_row->cell(get_name(SNAP_NAME_USERS_CREATED_TIME))->value());
            if(!value.nullValue())
            {
                QDomElement desc(doc.createElement("desc"));
                desc.setAttribute("type", "users::created");
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
    static_cast<void>(ctemplate);

    // TODO: convert using field_search
    QDomDocument doc(page.ownerDocument());

    // retrieve the authors
    // TODO: add support to retrieve the "author" who last modified this
    //       page (i.e. user reference in the last revision)
    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    const QString link_name(get_name(SNAP_NAME_USERS_AUTHOR));
    links::link_info author_info(get_name(SNAP_NAME_USERS_AUTHOR), true, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(author_info));
    links::link_info user_info;
    if(link_ctxt->next_link(user_info))
    {
        // an author is attached to this page
        const QString author_key(user_info.key());
        // all we want to offer here is the author details defined in the
        // /user/... location although we may want access to his email
        // address too (to display to an admin for example)
        QtCassandra::QCassandraRow::pointer_t author_row(content_table->row(author_key));

        {   // snap/page/body/author[type=users::name]/data
            QtCassandra::QCassandraValue value(author_row->cell(get_name(SNAP_NAME_USERS_USERNAME))->value());
            if(!value.nullValue())
            {
                QDomElement author(doc.createElement("author"));
                author.setAttribute("type", "users::name");
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
    static_cast<void>(owner);
    static_cast<void>(type);

    if(!f_user_key.isEmpty())
    {
        QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
        if(users_table->exists(f_user_key))
        {
            QtCassandra::QCassandraValue value(users_table->row(f_user_key)->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
            if(!value.nullValue())
            {
                int64_t const identifier(value.int64Value());
                QString const site_key(f_snap->get_site_key_with_slash());
                QString const user_key(QString("%1%2/%3").arg(site_key).arg(get_name(SNAP_NAME_USERS_PATH)).arg(identifier));

                QString const link_name(get_name(SNAP_NAME_USERS_AUTHOR));
                bool const source_unique(true);
                links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
                QString const link_to(get_name(SNAP_NAME_USERS_AUTHORED_PAGES));
                bool const destination_multi(false);
                links::link_info destination(link_to, destination_multi, user_key, ipath.get_branch());
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void users::prepare_replace_password_form(QDomElement& body)
{
    // make sure the user is properly setup
    if(user_is_logged_in())
    {
        // user is logged in already, send him to his normal password form
        f_snap->page_redirect("user/password", snap_child::HTTP_CODE_SEE_OTHER, "Already Logged In", "You are already logged in so you cannot access this page at this time.");
        NOTREACHED();
    }
    if(!f_user_key.isEmpty())
    {
        // user logged in a while back, ask for credentials again
        f_snap->page_redirect("verify-credentials", snap_child::HTTP_CODE_SEE_OTHER, "Not Enough Permissions", "You are logged in with minimal permissions. To access this page we have to verify your credentials.");
        NOTREACHED();
    }
    if(f_user_changing_password_key.isEmpty())
    {
        // user is not even logged in and he did not follow a valid link
        // XXX the login page is probably the best choice?
        f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER, "Replace Password Not Possible", "You required to change your password in a way which is not current valid. Please go to log in instead.");
        NOTREACHED();
    }
}
#pragma GCC diagnostic pop


/** \brief Show the user profile.
 *
 * This function shows a user profile. By default one can use user/me to
 * see his profile. The administrators can see any profile. Otherwise
 * only public profiles and the user own profile are accessible.
 */
void users::show_user(content::path_info_t& ipath, QDomElement& page, QDomElement& body)
{
    QString user_path(ipath.get_cpath());
    int64_t identifier(0);
    QString user_id(user_path.mid(5));
    if(user_id == "me" || user_id == "password")
    {
        // retrieve the logged in user identifier
        if(f_user_key.isEmpty())
        {
            attach_to_session(get_name(SNAP_NAME_USERS_LOGIN_REFERRER), "user/password");

            messages::messages::instance()->set_error(
                "Permission Denied",
                "You are not currently logged in. You may check out your profile only when logged in.",
                "attempt to view the current user page when the user is not logged in",
                false
            );
            // redirect the user to the log in page
            f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
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
            f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
            NOTREACHED();
            return;
        }
        QtCassandra::QCassandraValue value(users_table->row(f_user_key)->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
        if(value.nullValue())
        {
            messages::messages::instance()->set_error(
                "Could Not Find Your Account",
                "Somehow we could not find your account on this system.",
                "user account for " + f_user_key + " does not have an identifier",
                true
            );
            // redirect the user to the log in page
            f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
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
            f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                    "User Not Found", "This user does not exist. Please check the URI and make corrections as required.",
                    "User attempt to access user \"" + user_id + "\" which does not look like a valid integer.");
            NOTREACHED();
        }

        // verify that the identifier indeed represents a user
        const QString site_key(f_snap->get_site_key_with_slash());
        const QString user_key(site_key + get_name(SNAP_NAME_USERS_PATH) + "/" + user_id);
        QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
        if(!content_table->exists(user_key))
        {
            f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
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
        f_snap->die(snap_child::HTTP_CODE_FORBIDDEN, "Access Denied",
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
        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
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
        f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    if(user_is_logged_in())
    {
        // ?!? -- what should we do in this case?
        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
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
        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
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
        f_snap->page_redirect("user/password", snap_child::HTTP_CODE_SEE_OTHER);
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
        f_snap->page_redirect("user/password", snap_child::HTTP_CODE_SEE_OTHER);
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
 * \todo
 * As an additional verification we could use the cookie that was setup
 * to make sure that the user is the same person. This means the cookie
 * should not be deleted on closure in the event the user is to confirm
 * his email later and wants to close everything in the meantime.
 *
 * \param[in,out] ipath  The path used to access this page.
 */
void users::verify_user(content::path_info_t& ipath)
{
    if(!f_user_key.isEmpty())
    {
        // user is logged in already, just send him to his profile
        // (if logged in he was verified in some way!)
        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QString session_id(ipath.get_cpath().mid(7));
    sessions::sessions::session_info info;
    sessions::sessions *session(sessions::sessions::instance());
    // TODO: remove the ending characters such as " ", "/", "\" and "|"?
    //       (it happens that people add those by mistake at the end of a URI...)
    session->load_session(session_id, info);
    QString const path(info.get_object_path());
    if(info.get_session_type() != sessions::sessions::session_info::SESSION_INFO_VALID
    || info.get_user_agent() != f_snap->snapenv(snap::get_name(SNAP_NAME_CORE_HTTP_USER_AGENT))
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
        f_snap->page_redirect("verify", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // it looks like the session is valid, get the user email and verify
    // that the account exists in the database
    QString const email(path.mid(6));
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
        f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QtCassandra::QCassandraRow::pointer_t row(users_table->row(email));
    const QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
    if(user_identifier.nullValue())
    {
        SNAP_LOG_FATAL("users::verify_user() could not load the user identifier, the row exists but the cell did not make it (")
                        (email)("/")
                        (get_name(SNAP_NAME_USERS_IDENTIFIER))(").");
        // redirect the user to the verification form although it won't work
        // next time either...
        f_snap->page_redirect("verify", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    int64_t const identifier(user_identifier.int64Value());
    content::path_info_t user_ipath;
    user_ipath.set_path(QString("%1/%2").arg(get_name(SNAP_NAME_USERS_PATH)).arg(identifier));

    // before we actually accept this verification code, we must make sure
    // the user is still marked as a new user (he should or the session
    // would be invalid, but for security it is better to check again)
    links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
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
        f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // a status link exists...
    QString const site_key(f_snap->get_site_key_with_slash());
    if(status_info.key() != site_key + get_name(SNAP_NAME_USERS_NEW_PATH))
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
        f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    // remove the "user/new" status link so the user can now log in
    // he was successfully logged in
    links::links::instance()->delete_link(user_status_info);

    // Save the date when the user verified
    QtCassandra::QCassandraValue value;
    value.setInt64Value(f_snap->get_start_date());
    row->cell(get_name(SNAP_NAME_USERS_VERIFIED_ON))->setValue(value);

    // Save the user IP address when verified
    value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
    row->cell(get_name(SNAP_NAME_USERS_VERIFIED_IP))->setValue(value);

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
    f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
    NOTREACHED();
}


/** \brief Signal that a new user was verified.
 *
 * After a user registers, he receivs an email with a magic number that
 * needs to be used for the user to register on the system.
 *
 * \param[in,out] ipath  The user path.
 * \param[in] identifier  The user identifier.
 *
 * \return true so the other plugins can receive the signal
 */
bool users::user_verified_impl(content::path_info_t& ipath, int64_t identifier)
{
    static_cast<void>(ipath);
    static_cast<void>(identifier);

    // all the verifications are processed in the verify_user() function
    // as far as the users plugin is concerned, so just return true
    return true;
}


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
        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QString session_id(ipath.get_cpath().mid(13));

    sessions::sessions::session_info info;
    sessions::sessions *session(sessions::sessions::instance());
    // TODO: remove the ending characters such as " ", "/", "\" and "|"?
    //       (it happens that people add those by mistake at the end of a URI...)
    session->load_session(session_id, info);
    const QString path(info.get_object_path());
    if(info.get_session_type() != sessions::sessions::session_info::SESSION_INFO_VALID
    || info.get_user_agent() != f_snap->snapenv(snap::get_name(SNAP_NAME_CORE_HTTP_USER_AGENT))
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
        f_snap->page_redirect("new-password", snap_child::HTTP_CODE_SEE_OTHER);
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
        f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QtCassandra::QCassandraRow::pointer_t row(users_table->row(email));
    const QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
    if(user_identifier.nullValue())
    {
        SNAP_LOG_FATAL("users::process_new_password_form() could not load the user identifier, the row exists but the cell did not make it (")
                        (email)("/")
                        (get_name(SNAP_NAME_USERS_IDENTIFIER))(").");
        // TODO where to send that user?! have an error page for all of those
        //      "your account is dead, sorry dear..."
        f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    int64_t const identifier(user_identifier.int64Value());
    content::path_info_t user_ipath;
    user_ipath.set_path(QString("%1/%2").arg(get_name(SNAP_NAME_USERS_PATH)).arg(identifier));

    // before we actually accept this verification code, we must make sure
    // the user is still marked as a new user (he should or the session
    // would be invalid, but for security it is better to check again)
    links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
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
        f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    // a status link exists... is it the right one?
    QString const site_key(f_snap->get_site_key_with_slash());
    if(status_info.key() != site_key + get_name(SNAP_NAME_USERS_PASSWORD_PATH))
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
        f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    // remove the "user/password" status link so the user can now log in
    // he was successfully logged in -- don't kill this one yet...
    //links::links::instance()->delete_link(user_status_info);

    // Save the date when the user verified
    QtCassandra::QCassandraValue value;
    value.setInt64Value(f_snap->get_start_date());
    row->cell(get_name(SNAP_NAME_USERS_FORGOT_PASSWORD_ON))->setValue(value);

    // Save the user IP address when verified
    value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
    row->cell(get_name(SNAP_NAME_USERS_FORGOT_PASSWORD_IP))->setValue(value);

    f_user_changing_password_key = email;

    // send the user to the log in page since he got verified now
    f_snap->page_redirect("user/password/replace", snap_child::HTTP_CODE_SEE_OTHER);
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
void users::on_process_form_post(content::path_info_t& ipath, sessions::sessions::session_info const& session_info)
{
    static_cast<void>(session_info);

    QString const cpath(ipath.get_cpath());
    if(cpath == "login")
    {
        process_login_form(LOGIN_MODE_FULL);
    }
    else if(cpath == "verify-credentials")
    {
        process_login_form(LOGIN_MODE_VERIFICATION);
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
    QString details;
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());

    bool validation_required(false);

    // retrieve the row for that user
    QString const key(f_snap->postenv("email"));
    if(login_mode == LOGIN_MODE_VERIFICATION && f_user_key != key)
    {
        // XXX we could also automatically log the user out and send him
        //     to the log in screen... (we certainly should do so on the
        //     third attempt!)
        messages::messages::instance()->set_error(
            "Wrong Credentials",
            "These are the wrong credentials. If you are not sure who you were logged as, please <a href=\"/logout\">log out</a> first and then log back in.",
            "users::process_login_form() email mismatched when verifying credentials (got \""
                         + key + "\", expected \"" + f_user_key + "\").",
            false
        );
        return;
    }

    if(users_table->exists(key))
    {
        QtCassandra::QCassandraRow::pointer_t row(users_table->row(key));

        QtCassandra::QCassandraValue value;

        // existing users have a unique identifier
        QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
        if(user_identifier.nullValue())
        {
            messages::messages::instance()->set_error(
                "Could Not Log You In",
                "Somehow your user identifier is not available. Without it we cannot log your in.",
                "users::process_login_form() could not load the user identifier, the row exists but the cell did not make it ("
                             + key + "/" + get_name(SNAP_NAME_USERS_IDENTIFIER) + ").",
                false
            );
            if(login_mode == LOGIN_MODE_VERIFICATION)
            {
                // force a log out because the user should not be remotely
                // logged in in any way...
                f_snap->page_redirect("logout", snap_child::HTTP_CODE_SEE_OTHER);
            }
            else
            {
                // XXX should we redirect to some error page in that regard?
                //     (i.e. your user account is messed up, please contact us?)
                f_snap->page_redirect("verify", snap_child::HTTP_CODE_SEE_OTHER);
            }
            NOTREACHED();
        }
        user_logged_info_t logged_info;
        logged_info.set_identifier(user_identifier.int64Value());
        logged_info.user_ipath().set_path(QString("%1/%2").arg(get_name(SNAP_NAME_USERS_PATH)).arg(logged_info.get_identifier()));

        // before we actually log the user in we must make sure he's
        // not currently blocked or not yet active
        links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, logged_info.user_ipath().get_key(), logged_info.user_ipath().get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
        links::link_info status_info;
        bool force_redirect_password_change(false);
        bool valid(true);
        if(link_ctxt->next_link(status_info))
        {
            QString const site_key(f_snap->get_site_key_with_slash());

//printf("Current status is [%s] / [%s]\n", status_info.key().toUtf8().data(), (site_key + get_name(SNAP_NAME_USERS_PASSWORD_PATH)).toUtf8().data());
            // the status link exists...
            // this means the user is either a new user (not yet verified)
            // or he is blocked
            // either way it means he cannot log in at this time!
            if(status_info.key() == site_key + get_name(SNAP_NAME_USERS_NEW_PATH))
            {
                details = "user's account is not yet active (not yet verified)";
                validation_required = true;
                valid = false;
            }
            else if(status_info.key() == site_key + get_name(SNAP_NAME_USERS_BLOCKED_PATH))
            {
                details = "user's account is blocked";
                valid = false;
            }
            else if(status_info.key() == site_key + get_name(SNAP_NAME_USERS_AUTO_PATH))
            {
                details = "user did not register, this is an auto-account only";
                valid = false;
            }
            else if(status_info.key() == site_key + get_name(SNAP_NAME_USERS_PASSWORD_PATH))
            {
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
            // compute the hash of the password
            // (1) get the digest
            value = row->cell(get_name(SNAP_NAME_USERS_PASSWORD_DIGEST))->value();
            QString const digest(value.stringValue());

            // (2) we need the passord:
            QString const password(f_snap->postenv("password"));

            // (3) get the salt in a buffer
            value = row->cell(get_name(SNAP_NAME_USERS_PASSWORD_SALT))->value();
            QByteArray const salt(value.binaryValue());

            // (4) compute the expected hash
            QByteArray hash;
            encrypt_password(digest, password, salt, hash);

            // (5) retrieved the saved hash
            value = row->cell(get_name(SNAP_NAME_USERS_PASSWORD))->value();
            QByteArray const saved_hash(value.binaryValue());

            // (6) compare both hashes
            // (note: at this point I don't trust the == operator of the QByteArray
            // object; will it work with '\0' bytes???)
            if(hash.size() == saved_hash.size()
            && memcmp(hash.data(), saved_hash.data(), hash.size()) == 0)
            {
                // User credentials are correct, create a session & cookie

                // log the user in by adding the correct object path
                // the other parameters were already defined in the
                // on_process_cookies() function
                f_info->set_object_path("/user/" + key);
                f_info->set_login_limit(f_snap->get_start_time() + 3600 * 3); // 3 hours (needs to become a parameter)
                sessions::sessions::instance()->save_session(*f_info, true); // force new random session number

                http_cookie cookie(f_snap, get_user_cookie_name(), QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random()));
                cookie.set_expire_in(86400 * 5);  // 5 days
                cookie.set_http_only(); // make it a tad bit safer
                f_snap->set_cookie(cookie);

                // this is now the current user
                f_user_key = key;

                // Copy the previous login date and IP to the previous fields
                if(row->exists(get_name(SNAP_NAME_USERS_LOGIN_ON)))
                {
                    row->cell(get_name(SNAP_NAME_USERS_PREVIOUS_LOGIN_ON))->setValue(row->cell(get_name(SNAP_NAME_USERS_LOGIN_ON))->value());
                }
                if(row->exists(get_name(SNAP_NAME_USERS_LOGIN_IP)))
                {
                    row->cell(get_name(SNAP_NAME_USERS_PREVIOUS_LOGIN_IP))->setValue(row->cell(get_name(SNAP_NAME_USERS_LOGIN_IP))->value());
                }

                // Save the date when the user logged out
                value.setInt64Value(f_snap->get_start_date());
                row->cell(get_name(SNAP_NAME_USERS_LOGIN_ON))->setValue(value);

                // Save the user IP address when logged out
                value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
                row->cell(get_name(SNAP_NAME_USERS_LOGIN_IP))->setValue(value);

                // Tell all the other plugins that the user is now logged in
                // you may specify a URI to where the user should be sent on
                // log in, used in the redirect below, although we will go
                // to user/password whatever the path is specified here
                logged_info.set_email(key);
                user_logged_in(logged_info);

                if(force_redirect_password_change)
                {
                    // this URI has priority over other plugins URIs
                    logged_info.set_uri("user/password");
                }
                else if(logged_info.get_uri().isEmpty())
                {
                    // here we detach from the session since we want to
                    // redirect only once to that page
                    logged_info.set_uri(sessions::sessions::instance()->detach_from_session(*f_info, get_name(SNAP_NAME_USERS_LOGIN_REFERRER)));
                    if(logged_info.get_uri().isEmpty())
                    {
                        // User is now logged in, redirect him to his profile
                        //
                        // TODO: the admin needs to be able to change that
                        //       default redirect
                        logged_info.set_uri("user/me");
                    }
                }
                f_snap->page_redirect(logged_info.get_uri(), snap_child::HTTP_CODE_SEE_OTHER);
                NOTREACHED();
            }
            else
            {
                // user mistyped his password?
                details = "invalid credentials (password doesn't match)";
            }
        }
    }
    else
    {
        // user mistyped his email or is not registered?
        details = "invalid credentials (user with specified email does not exist)";
    }

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
    messages::messages::instance()->set_error(
        "Could Not Log You In",
        validation_required
          ? "Your account was not yet validated. Please make sure to first follow the link we sent in your email. If you did not yet receive that email, we can send you another <a href=\"/confirmation-email\">confirmation email</a>."
          : "Your email or password were incorrect. If you are not registered, you may want to consider <a href=\"/register\">registering</a> first?",
        details,
        false // should this one be true?
    );
}


/** \brief Tell plugins that the user is now logged in.
 *
 * This signal is used to tell plugins that the user is now logged in.
 * Note that this signal only happens at the time the user logs in, not
 * each time the user accesses the server.
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
 *
 * \return true if the signal is to be propagated.
 */
bool users::user_logged_in_impl(user_logged_info_t& logged_info)
{
    static_cast<void>(logged_info);

    return true;
}


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
    QString email(f_snap->postenv("email"));
    if(register_user(email, f_snap->postenv("password")))
    {
        verify_email(email);
        messages->set_info(
            "We registered your account",
            "We sent you an email to \"" + email + "\". In the email there is a link you need to follow to finish your registration."
        );
        // redirect the user to the verification form
        f_snap->page_redirect("verify", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    else
    {
        messages->set_error(
            "User Already Exists",
            "A user with email \"" + email + "\" already exists. If it is you, then try to request a new password if you need a reminder.",
            "user \"" + email + "\" trying to register a second time.",
            true
        );
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
        QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(get_name(SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
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
            if(status == "" || status == site_key + get_name(SNAP_NAME_USERS_PASSWORD_PATH))
            {
                // Only users considered active can request a new password
                forgot_password_email(email);

                // mark the user with the types/users/password tag
                QString const link_name(get_name(SNAP_NAME_USERS_STATUS));
                bool const source_unique(true);
                links::link_info source(link_name, source_unique, user_ipath.get_key(), user_ipath.get_branch());
                QString const link_to(get_name(SNAP_NAME_USERS_STATUS));
                bool const destination_unique(false);
                content::path_info_t dpath;
                dpath.set_path(get_name(SNAP_NAME_USERS_PASSWORD_PATH));
                links::link_info destination(link_to, destination_unique, dpath.get_key(), dpath.get_branch());
                links::links::instance()->create_link(source, destination);

                // once we sent the new code, we can send the user back
                // to the verify form
                messages::messages::instance()->set_info(
                    "New Verification Email Send",
                    "We just sent you a new verification email. Please check your account and follow the verification link or copy and paste your verification code below."
                );
                f_snap->page_redirect("new-password", snap_child::HTTP_CODE_SEE_OTHER);
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
        f_snap->page_redirect("user/password", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }
    if(f_user_changing_password_key.isEmpty())
    {
        // user is not logged in and he did not follow a valid link
        // XXX the login page is probably the best choice?
        f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
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
        QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(get_name(SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
                QString const site_key(f_snap->get_site_key_with_slash());
                if(status_info.key() == site_key + get_name(SNAP_NAME_USERS_PASSWORD_PATH))
                {
                    // We're good, save the new password and remove that link

                    // First encrypt the password
                    QString const password(f_snap->postenv("password"));
                    QByteArray salt;
                    QByteArray hash;
                    QtCassandra::QCassandraValue digest(f_snap->get_site_parameter(get_name(SNAP_NAME_USERS_PASSWORD_DIGEST)));
                    if(digest.nullValue())
                    {
                        digest.setStringValue("sha512");
                    }
                    create_password_salt(salt);
                    encrypt_password(digest.stringValue(), password, salt, hash);

                    // Save the hashed password (never the original password!)
                    QtCassandra::QCassandraValue value;
                    value.setBinaryValue(hash);
                    row->cell(get_name(SNAP_NAME_USERS_PASSWORD))->setValue(value);

                    // Save the password salt (otherwise we couldn't check whether the user
                    // knows his password!)
                    value.setBinaryValue(salt);
                    row->cell(get_name(SNAP_NAME_USERS_PASSWORD_SALT))->setValue(value);

                    // Also save the digest since it could change en-route
                    row->cell(get_name(SNAP_NAME_USERS_PASSWORD_DIGEST))->setValue(digest);

                    // Unlink from the password tag too
                    links::links::instance()->delete_link(status_info);

                    // Now we auto-log in the user... the session should
                    // already be adequate from the on_process_cookies()
                    // call
                    //
                    // TODO to make this safer we really need the extra 3 questions
                    //      and ask them when the user request the new password or
                    //      when he comes back in the replace password form
                    f_info->set_object_path("/user/" + f_user_changing_password_key);
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
                    f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
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
    f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
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
        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
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
        QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(get_name(SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            bool delete_password_status(false);
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
                QString const site_key(f_snap->get_site_key_with_slash());
                if(status_info.key() == site_key + get_name(SNAP_NAME_USERS_BLOCKED_PATH)
                || status_info.key() == site_key + get_name(SNAP_NAME_USERS_AUTO_PATH)
                || status_info.key() == site_key + get_name(SNAP_NAME_USERS_NEW_PATH))
                {
                    // somehow the user is not blocked or marked as auto...
                    f_snap->die(snap_child::HTTP_CODE_FORBIDDEN,
                            "Access Denied", "You need to be logged in and have enough permissions to access this page.",
                            "User attempt to change a password in his account which is currently blocked.");
                    NOTREACHED();
                }
                else if(status_info.key() == site_key + get_name(SNAP_NAME_USERS_PASSWORD_PATH))
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
            QtCassandra::QCassandraValue value(row->cell(get_name(SNAP_NAME_USERS_PASSWORD_DIGEST))->value());
            QString const old_digest(value.stringValue());

            // (2) we need the passord:
            QString const old_password(f_snap->postenv("old_password"));

            // (3) get the salt in a buffer
            value = row->cell(get_name(SNAP_NAME_USERS_PASSWORD_SALT))->value();
            QByteArray const old_salt(value.binaryValue());

            // (4) compute the expected hash
            QByteArray old_hash;
            encrypt_password(old_digest, old_password, old_salt, old_hash);

            // (5) retrieved the saved hashed password
            value = row->cell(get_name(SNAP_NAME_USERS_PASSWORD))->value();
            QByteArray const saved_hash(value.binaryValue());

            // (6) verify that it matches
            if(old_hash.size() == saved_hash.size()
            && memcmp(old_hash.data(), saved_hash.data(), old_hash.size()) == 0)
            {
                // The user entered his old password properly
                // save the new password
                QString new_password(f_snap->postenv("new_password"));
                QtCassandra::QCassandraValue new_digest(f_snap->get_site_parameter(get_name(SNAP_NAME_USERS_PASSWORD_DIGEST)));
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
                row->cell(get_name(SNAP_NAME_USERS_PASSWORD))->setValue(value);

                // Save the password salt (otherwise we couldn't check whether the user
                // knows his password!)
                value.setBinaryValue(new_salt);
                row->cell(get_name(SNAP_NAME_USERS_PASSWORD_SALT))->setValue(value);

                // also save the digest since it could change en-route
                row->cell(get_name(SNAP_NAME_USERS_PASSWORD_DIGEST))->setValue(new_digest);

                // Unlink from the password tag too
                if(delete_password_status)
                {
                    links::links::instance()->delete_link(status_info);
                }

                content::content::instance()->modified_content(user_ipath);

                // once we sent the new code, we can send the user back
                // to the verify form
                messages::messages::instance()->set_info(
                    "Password Changed",
                    "Your new password was saved. Next time you want to log in, you must use your email with this new password."
                );
                QString referrer(sessions::sessions::instance()->detach_from_session(*f_info, get_name(SNAP_NAME_USERS_LOGIN_REFERRER)));
                if(referrer == "user/password")
                {
                    // ignore that redirect if it is to this page
                    referrer.clear();
                }
                if(referrer.isEmpty())
                {
                    // Redirect user to his profile
                    f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
                }
                else
                {
                    // If the user logged in when he needed to still change
                    // his password, then there may very be a referrer path
                    f_snap->page_redirect(referrer, snap_child::HTTP_CODE_SEE_OTHER);
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
    f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
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
    QString email(f_snap->postenv("email"));
    QString details;

    // check to make sure that a user with that email address exists
    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    if(users_table->exists(email))
    {
        QtCassandra::QCassandraRow::pointer_t row(users_table->row(email));

        // existing users have a unique identifier
        // necessary to create the user key below
        QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t const identifier(user_identifier.int64Value());
            content::path_info_t user_ipath;
            user_ipath.set_path(QString("%1/%2").arg(get_name(SNAP_NAME_USERS_PATH)).arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_ipath.get_key(), user_ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
                QString const site_key(f_snap->get_site_key_with_slash());
                if(status_info.key() == site_key + get_name(SNAP_NAME_USERS_NEW_PATH))
                {
                    // Only new users are allowed to get another verification email
                    verify_email(email);
                    // once we sent the new code, we can send the user back
                    // to the verify form
                    messages::messages::instance()->set_info(
                        "New Verification Email Send",
                        "We just sent you a new verification email. Please check your account and follow the verification link or copy and paste your verification code below."
                    );
                    f_snap->page_redirect("verify", snap_child::HTTP_CODE_SEE_OTHER);
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
 */
void users::process_verify_form()
{
    // verify the code the user entered, the verify_user() function
    // will automatically redirect us if necessary; we should
    // get an error if redirect to ourselves
    QString verification_code(f_snap->postenv("verification_code"));
    content::path_info_t ipath;
    ipath.set_path("verify/" + verification_code);
    verify_user(ipath);
}


/** \brief Get the registered (MAYBE NOT LOGGED IN) user key.
 *
 * WARNING WARNING WARNING
 * This returns the user key which is his email address. It does not
 * tell you that the user is logged in. For that purpose you MUST
 * use the is_user_logged_in() function.
 *
 * This function returns the key of the user that last logged
 * in. This key is the user's email address. Remember that a
 * user is not considered fully logged in if his sesion his more than
 * 3 hours old. You must make sure to check the is_user_logged_in()
 * too. Note that the permission system should already take care of
 * most of those problems for you anyway, but you need to know what
 * you're doing!
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
 * else if(users::users::instance()->is_user_logged_in())
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
 * use the is_user_logged_in() function.
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
 * \note
 * To test whether the returned value represents the anonymous user,
 * please make use  of get_name() with SNAP_NAME_USERS_ANONYMOUS_PATH.
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
            QtCassandra::QCassandraValue const value(users_table->row(f_user_key)->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
            if(!value.nullValue())
            {
                int64_t const identifier(value.int64Value());
                return QString("%1/%2").arg(get_name(SNAP_NAME_USERS_PATH)).arg(identifier);
            }
        }
    }
    return get_name(SNAP_NAME_USERS_ANONYMOUS_PATH);
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
 * happens whenever the user decides to register.
 *
 * \param[in] email  The email of the user. It must be a valid email address.
 * \param[in] password  The password of the user or "!".
 *
 * \return true if the user was newly created, false otherwise
 */
bool users::register_user(QString const& email, QString const& password)
{
    QByteArray salt;
    QByteArray hash;
    QtCassandra::QCassandraValue digest(f_snap->get_site_parameter(get_name(SNAP_NAME_USERS_PASSWORD_DIGEST)));
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

    QtCassandra::QCassandraTable::pointer_t users_table(get_users_table());
    QString key(email);
    QtCassandra::QCassandraRow::pointer_t row(users_table->row(key));

    QtCassandra::QCassandraValue value;
    value.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
    value.setStringValue(key);

    int64_t identifier(0);
    QString const id_key(get_name(SNAP_NAME_USERS_ID_ROW));
    QString const identifier_key(get_name(SNAP_NAME_USERS_IDENTIFIER));
    QtCassandra::QCassandraValue new_identifier;
    new_identifier.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);

    // we got as much as we could ready before locking
    {
        // first make sure this email is unique
        QtCassandra::QCassandraLock lock(f_snap->get_context(), key);

        // TODO: we have to look at all the possible email addresses
        const char *email_key(get_name(SNAP_NAME_USERS_ORIGINAL_EMAIL));
        QtCassandra::QCassandraCell::pointer_t cell(row->cell(email_key));
        cell->setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
        QtCassandra::QCassandraValue email_data(cell->value());
        if(!email_data.nullValue())
        {
            // someone else already registered with that email
            return false;
        }

        // Note that the email was already checked when coming from the Register
        // form, however, it was check for validity as an email, not checked
        // against a black list or verified in other ways; also the password
        // can this way be checked by another plugin (i.e. password database)
        content::permission_flag secure;
        check_user_security(email, password, secure);
        if(!secure.allowed())
        {
            // well... someone said "don't save that user in there"!
            return false;
        }

        // we're the first to lock this row, the user is therefore unique
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
            QtCassandra::QCassandraValue current_identifier(id_cell->value());
            if(current_identifier.nullValue())
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
                //     if they try again it will fail again anyway...
                return false;
            }
            identifier = current_identifier.int64Value();
        }
        ++identifier;
        new_identifier.setInt64Value(identifier);
        users_table->row(id_key)->cell(identifier_key)->setValue(new_identifier);

        // the lock automatically goes away here
    }

    // WARNING: if this breaks, someone probably changed the value
    //          content; it should be the user email
    users_table->row(get_name(SNAP_NAME_USERS_INDEX_ROW))->cell(new_identifier.binaryValue())->setValue(value);

    // Save the user identifier in his user account so we can easily find
    // the content user for that user account/email
    row->cell(identifier_key)->setValue(new_identifier);

    // Save the hashed password (never the original password!)
    value.setBinaryValue(hash);
    row->cell(get_name(SNAP_NAME_USERS_PASSWORD))->setValue(value);

    // Save the password salt (otherwise we couldn't check whether the user
    // knows his password!)
    value.setBinaryValue(salt);
    row->cell(get_name(SNAP_NAME_USERS_PASSWORD_SALT))->setValue(value);

    // also save the digest since it could change en-route
    row->cell(get_name(SNAP_NAME_USERS_PASSWORD_DIGEST))->setValue(digest);

    // Save the user IP address when registering
    value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
    row->cell(get_name(SNAP_NAME_USERS_ORIGINAL_IP))->setValue(value);

    // Date when the user was created (i.e. now)
    uint64_t created_date(f_snap->get_start_date());
    row->cell(get_name(SNAP_NAME_USERS_CREATED_TIME))->setValue(created_date);

    // Now create the user in the contents
    // (nothing else should be create at the path until now)
    QString user_path(get_name(SNAP_NAME_USERS_PATH));
    QString const site_key(f_snap->get_site_key_with_slash());
    content::path_info_t user_ipath;
    user_ipath.set_path(QString("%1/%2").arg(user_path).arg(identifier));
    content::content *content_plugin(content::content::instance());
    snap_version::version_number_t const branch_number(content_plugin->get_current_user_branch(user_ipath.get_key(), content_plugin->get_plugin_name(), "", true));
    user_ipath.force_branch(branch_number);
    // default revision when creating a new branch
    user_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
    user_ipath.force_locale("xx");
    content_plugin->create_content(user_ipath, get_plugin_name(), "user-page");

    // save a default title and body
    QtCassandra::QCassandraTable::pointer_t data_table(content_plugin->get_data_table());
    QtCassandra::QCassandraRow::pointer_t revision_row(data_table->row(user_ipath.get_revision_key()));
    revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_CREATED))->setValue(created_date);
    // no title or body by default--other plugins could set those to the
    //                              user name or other information
    QString const empty_string;
    revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_TITLE))->setValue(empty_string);
    revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_BODY))->setValue(empty_string);

    // The "public" user account (i.e. in the content table) is limited
    // to the identifier at this point
    //
    // however, we also want to include a link defined as the status
    // at first the user is marked as being new
    // the destination URL is defined in the <link> content
    QString const link_name(get_name(SNAP_NAME_USERS_STATUS));
    bool const source_unique(true);
    // TODO: determine whether "xx" is the correct locale here (we could also
    //       have "" and a default website language...) -- this is the
    //       language of the profile, not the language of the website...
    links::link_info source(link_name, source_unique, user_ipath.get_key(), user_ipath.get_branch(true, "xx"));
    const QString link_to(get_name(SNAP_NAME_USERS_STATUS));
    bool const destination_unique(false);
    content::path_info_t dpath;
    dpath.set_path(get_name(SNAP_NAME_USERS_NEW_PATH));
    links::link_info destination(link_to, destination_unique, dpath.get_key(), dpath.get_branch());
    links::links::instance()->create_link(source, destination);

    user_registered(user_ipath, identifier);

    return true;
}


/** \brief Signal that a user is about to get a new account.
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
 *
 * \return true so other plugins have a chance to verify the user email
 *         and password before we create the new user.
 */
bool users::check_user_security_impl(QString const& email, QString const& password, content::permission_flag& secure)
{
    static_cast<void>(email);
    static_cast<void>(password);
    static_cast<void>(secure);

    return true;
}


/** \brief Signal telling other plugins that a user just registered.
 *
 * Note that this signal is sent when the user was registered and NOT when
 * the user verified his account. This means the user is not really fully
 * authorized on the system yet.
 *
 * \param[in,out] ipath  The path to the new user's account (/user/\<identifier\>)
 * \param[in] identifier  The user identifier.
 *
 * \return true so the signal propagates to other plugins.
 */
bool users::user_registered_impl(content::path_info_t& ipath, int64_t identifier)
{
    static_cast<void>(ipath);
    static_cast<void>(identifier);

    return true;
}


/** \brief Send an email to request email verification.
 *
 * This function generates an email and sends it. The email is used to request
 * the user to verify that he receives said emails.
 *
 * \param[in] email  The user email.
 */
void users::verify_email(const QString& email)
{
    sendmail::sendmail::email e;

    // mark priority as High
    e.set_priority(sendmail::sendmail::email::EMAIL_PRIORITY_HIGH);

    // destination email address
    e.add_header(sendmail::get_name(sendmail::SNAP_NAME_SENDMAIL_TO), email);

    // add the email subject and body using a page
    e.set_email_path("admin/email/users/verify");

    // verification makes use of a session identifier
    sessions::sessions::session_info info;
    info.set_session_type(sessions::sessions::session_info::SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_VERIFY_EMAIL);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    //info.set_page_path(); -- default is okay
    info.set_object_path("/user/" + email);
    info.set_user_agent(f_snap->snapenv(snap::get_name(SNAP_NAME_CORE_HTTP_USER_AGENT)));
    info.set_time_to_live(86400 * 3);  // 3 days
    QString session(sessions::sessions::instance()->create_session(info));
    e.add_parameter(get_name(SNAP_NAME_USERS_VERIFY_EMAIL), session);

    // send the email
    //
    // really this just saves it in the database, the sendmail itself
    // happens on the backend; see sendmail::on_backend_action()
    sendmail::sendmail::instance()->post_email(e);
}


/** \brief Send an email to allow the user to change his password.
 *
 * This function generates an email and sends it to an active user. The
 * email is used to allow the user to change his password without having
 * to enter an old password.
 *
 * \param[in] email  The user email.
 */
void users::forgot_password_email(const QString& email)
{
    sendmail::sendmail::email e;

    // administrator can define this email address
    QtCassandra::QCassandraValue from(f_snap->get_site_parameter(get_name(SNAP_NAME_CORE_ADMINISTRATOR_EMAIL)));
    if(from.nullValue())
    {
        from.setStringValue("contact@snapwebsites.com");
    }
    e.set_from(from.stringValue());

    // mark priority as High
    e.set_priority(sendmail::sendmail::email::EMAIL_PRIORITY_HIGH);

    // destination email address
    e.add_header(sendmail::get_name(sendmail::SNAP_NAME_SENDMAIL_TO), email);

    // add the email subject and body using a page
    e.set_email_path("admin/email/users/forgot-password");

    // verification makes use of a session identifier
    sessions::sessions::session_info info;
    info.set_session_type(sessions::sessions::session_info::SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_FORGOT_PASSWORD_EMAIL);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    //info.set_page_path(); -- default is okay
    info.set_object_path("/user/" + email);
    info.set_user_agent(f_snap->snapenv(snap::get_name(SNAP_NAME_CORE_HTTP_USER_AGENT)));
    info.set_time_to_live(3600 * 8);  // 8 hours
    QString const session(sessions::sessions::instance()->create_session(info));
    e.add_parameter(get_name(SNAP_NAME_USERS_FORGOT_PASSWORD_EMAIL), session);

    // send the email
    //
    // really this just saves it in the database, the sendmail itself
    // happens on the backend; see sendmail::on_backend_action()
    sendmail::sendmail::instance()->post_email(e);
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
 * \param[in] name  The name of the cell that is to be used to save the data.
 * \param[in] data  The data to save in the session.
 *
 * \sa detach_from_session()
 */
void users::attach_to_session(const QString& name, const QString& data)
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
 * \param[in] name  The name of the cell that is to be used to save the data.
 *
 * \return The data read from the session if any, otherwise an empty string.
 *
 * \sa attach_to_session()
 */
QString users::detach_from_session(const QString& name) const
{
    return sessions::sessions::instance()->detach_from_session(*f_info, name);
}


/** \brief Set the referrer path for the current session.
 *
 * Call this instead of "attach_to_session( SNAP_NAME_USERS_LOGIN_REFERRER, cpath )" directly.
 *
 * \note the special cases "/login" and "/logout" will do nothing, since we don't want
 * a referrer in those cases.
 *
 * \sa attach_to_session()
 *
 */
void users::set_referrer( const QString& cpath )
{
    const char* loginref_name( get_name(SNAP_NAME_USERS_LOGIN_REFERRER) );

    // use the current refererrer if there is one as the redirect page
    // after log in; once the log in is complete, redirect to this referrer
    // page; if you send the user on a page that only redirects to /login
    // then the user will end up on his profile (/user/me)
    //
    if( sessions::sessions::instance()->get_from_session( *f_info, loginref_name ).isEmpty() )
    {
        SNAP_LOG_DEBUG() << "SNAP_NAME_USERS_LOGIN_REFERRER being set to " << cpath << " for page path " << f_info->get_page_path();
        QString const site_key(f_snap->get_site_key_with_slash());
        if( !cpath.isEmpty()
            && cpath != site_key + "login"
            && cpath != site_key + "logout"
        )
        {
            attach_to_session( loginref_name, cpath );
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
        attach_to_session(get_name(SNAP_NAME_USERS_CHANGING_PASSWORD_KEY), f_user_changing_password_key);
    }

    // the messages handling is here because the messages plugin cannot have
    // a dependency on the users plugin
    messages::messages *messages_plugin(messages::messages::instance());
    if(messages_plugin->get_message_count() > 0)
    {
        // note that if we lose those "website" messages,
        // they will still be in our logs
        QString const data(messages_plugin->serialize());
        attach_to_session(messages::get_name(messages::SNAP_NAME_MESSAGES_MESSAGES), data);
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
    // here we do a get_from_session() because we may need the variable
    // between several different forms before it gets deleted; the concerned
    // functions will clear() the variable when done with it
    f_user_changing_password_key = detach_from_session(get_name(SNAP_NAME_USERS_CHANGING_PASSWORD_KEY));

    // the messages handling is here because the messages plugin cannot have
    // a dependency on the users plugin
    QString const data(detach_from_session(messages::get_name(messages::SNAP_NAME_MESSAGES_MESSAGES)));
    if(!data.isEmpty())
    {
        messages::messages::instance()->unserialize(data);
    }
}


/** \brief Get the user selected language if user did that.
 *
 * The user can select the language in which he will see most of the
 * website (assuming most was translated.)
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
            QtCassandra::QCassandraValue const value(users_table->row(f_user_key)->cell(get_name(SNAP_NAME_USERS_LOCALES))->value());
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
    static_cast<void>(ipath);
    static_cast<void>(plugin_owner);
    static_cast<void>(xml);

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
        //       available and authorized
        token.f_replacement = "<a href=\"mailto:" + f_user_key + "\">" + f_user_key + "</a>";
        return;
    }

    // anything else requires the user to be verified
    QtCassandra::QCassandraValue const verified_on(users_table->row(f_user_key)->cell(get_name(SNAP_NAME_USERS_VERIFIED_ON))->value());
    if(verified_on.nullValue())
    {
        // not verified yet
        return;
    }

    if(token.is_token("users::since"))
    {
        // make sure that the user created and verified his account
        QtCassandra::QCassandraValue const value(users_table->row(f_user_key)->cell(get_name(SNAP_NAME_USERS_CREATED_TIME))->value());
        int64_t date(value.int64Value());
        token.f_replacement = QString("%1 %2")
                .arg(f_snap->date_to_string(date, f_snap->DATE_FORMAT_SHORT))
                .arg(f_snap->date_to_string(date, f_snap->DATE_FORMAT_TIME));
        // else use was not yet verified
        return;
    }

    if(token.is_token("users::picture"))
    {
        // make sure that the user created and verified his account
        QtCassandra::QCassandraValue const value(users_table->row(f_user_key)->cell(get_name(SNAP_NAME_USERS_PICTURE))->value());
        if(!value.nullValue())
        {
            SNAP_LOG_TRACE() << "second is_token(\"users::picture\")";

            // TBD: not sure right now how we'll offer those
            //      probably with a special path that tells us
            //      to go look in the users' table
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
    char const * const black_list(get_name(SNAP_NAME_USERS_BLACK_LIST));
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
 * \param[in,out] signature  The HTML signature to improve.
 */
void users::on_improve_signature(QString const& path, QString& signature)
{
    (void)path;
    if(!f_user_key.isEmpty())
    {
        QString const link(get_user_path());

        // TODO: translate
        signature += " <a href=\"/" + link + "\">My Account</a>";
    }
}


/** \brief Check whether the cell can securily be used in a script.
 *
 * This signal is sent by the cell() function of snap_expr objects.
 * The plugin receiving the signal can check the table, row, and cell
 * names and mark that specific cell as secure. This will prevent the
 * script writer from accessing that specific cell.
 *
 * This is used, for example, to protect the user password. Even though
 * the password is encrypted, allowing an end user to get a copy of
 * the encrypted password would dearly simplify the work of a hacker in
 * finding the unencrypted password.
 *
 * The \p secure flag is used to mark the cell as secure. Simply call
 * the mark_as_secure() function to do so.
 *
 * \param[in] table  The table being accessed.
 * \param[in] row  The row being accessed.
 * \param[in] cell  The cell being accessed.
 * \param[in] secure  Whether the cell is secure.
 *
 * \return This function returns true in case the signal needs to proceed.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void users::on_cell_is_secure(QString const& table, QString const& row, QString const& cell, server::secure_field_flag_t& secure)
{
    if(table == get_name(SNAP_NAME_USERS_TABLE))
    {
        if(cell == get_name(SNAP_NAME_USERS_PASSWORD)
        && cell == get_name(SNAP_NAME_USERS_PASSWORD_DIGEST)
        && cell == get_name(SNAP_NAME_USERS_PASSWORD_SALT))
        {
            // password is considered secure
            secure.mark_as_secure();
        }
    }
}
#pragma GCC diagnostic pop


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
