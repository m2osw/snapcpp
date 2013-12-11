// Snap Websites Server -- users handling
// Copyright (C) 2012-2013  Made to Order Software Corp.
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
#include "../content/content.h"
#include "../messages/messages.h"
#include "../sendmail/sendmail.h"
#include "not_reached.h"
#include "log.h"

#include <QFile>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <boost/static_assert.hpp>
#include <QtCassandra/QCassandraLock.h>


SNAP_PLUGIN_START(users, 1, 0)


namespace
{

const int SALT_SIZE = 32;
// the salt size must be even
BOOST_STATIC_ASSERT((SALT_SIZE & 1) == 0);

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
    switch(name) {
    case SNAP_NAME_USERS_ANONYMOUS_PATH:
        return "user";

    case SNAP_NAME_USERS_AUTHOR:
        return "author";

    case SNAP_NAME_USERS_AUTHORED_PAGES:
        return "authored_pages";

    case SNAP_NAME_USERS_AUTO_PATH:
        return "types/users/auto";

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

    case SNAP_NAME_USERS_PREVIOUS_LOGIN_IP:
        return "users::previous_login_ip";

    case SNAP_NAME_USERS_PREVIOUS_LOGIN_ON:
        return "users::previous_login_on";

    case SNAP_NAME_USERS_SESSION_COOKIE:
        // cookie names cannot include ':' so I use "__" to represent
        // the namespace separation
        return "users__snap_session";

    case SNAP_NAME_USERS_STATUS:
        return "status";

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
        throw snap_exception();

    }
    NOTREACHED();
}


/** \brief Initialize the users plugin.
 *
 * This function initializes the users plugin.
 */
users::users()
    //: f_snap(NULL) -- auto-init
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
    SNAP_PLUGIN_UPDATE(2013, 12, 8, 2, 3, 23, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief First update to run for the users plugin.
 *
 * This function is the first update for the users plugin. It installs
 * the initial data required by the users plugin.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void users::initial_update(int64_t variables_timestamp)
{
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
QSharedPointer<QtCassandra::QCassandraTable> users::get_users_table()
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
    SNAP_LISTEN(users, "content", content::content, create_content, _1, _2);
    SNAP_LISTEN(users, "path", path::path, can_handle_dynamic_path, _1, _2);
    SNAP_LISTEN(users, "layout", layout::layout, generate_header_content, _1, _2, _3, _4, _5);
    SNAP_LISTEN(users, "layout", layout::layout, generate_page_content, _1, _2, _3, _4, _5);
    //SNAP_LISTEN(users, "filter", filter::filter, replace_token, _1, _2, _3);

    f_info.reset(new sessions::sessions::session_info);
}


/** \brief Initialize the users plugin.
 *
 * At this point this function does nothing.
 */
void users::on_init()
{
}


/** \brief Process the cookies.
 *
 * This function is our opportunity to log the user in. We check for the
 * cookie named SNAP_NAME_USERS_SESSION_COOKIE and use it to know whether
 * the user is currently logged in or not.
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

    // any snap session?
    if(f_snap->cookie_is_defined(get_name(SNAP_NAME_USERS_SESSION_COOKIE)))
    {
        // is that session a valid user session?
        QString session_cookie(f_snap->cookie(get_name(SNAP_NAME_USERS_SESSION_COOKIE)));
        QStringList parameters(session_cookie.split("/"));
        QString session_key(parameters[0]);
        QString random_key;
        if(parameters.size() > 1)
        {
            random_key = parameters[1];
        }
        sessions::sessions::instance()->load_session(session_key, *f_info, false);
        const QString path(f_info->get_object_path());
        if(f_info->get_session_type() == sessions::sessions::session_info::SESSION_INFO_VALID
        && f_info->get_session_id() == USERS_SESSION_ID_LOG_IN_SESSION
        && f_info->get_session_random() == random_key.toInt()
        && path.left(6) == "/user/")
        {
            // this session qualifies as a log in session
            // so now verify the user
            const QString key(path.mid(6));
            // not authenticated user?
            if(!key.isEmpty())
            {
                QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());
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

                        QSharedPointer<QtCassandra::QCassandraRow> row(users_table->row(key));

                        // Save the date when the user logged out
                        QtCassandra::QCassandraValue value;
                        value.setInt64Value(f_snap->get_uri().option("start_date").toLongLong());
                        row->cell(get_name(SNAP_NAME_USERS_LOGOUT_ON))->setValue(value);

                        // Save the user IP address when logged out
                        value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
                        row->cell(get_name(SNAP_NAME_USERS_LOGOUT_IP))->setValue(value);
                    }
                    else
                    {
                        f_user_key = key;
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
        f_info->set_time_to_live(86400 * 5);  // 5 days
        sessions::sessions::instance()->create_session(*f_info);
    }
    else
    {
        // extend the session
        f_info->set_time_to_live(86400 * 5);  // 5 days
        sessions::sessions::instance()->save_session(*f_info);
    }

    //
    // TODO here we want to add a parameter to the session, a parameter
    //      which changes each time this user accesses the website
    //      and that additional identifier must also match (we send it
    //      in the cookie)
    //
    http_cookie cookie(
            f_snap,
            get_name(SNAP_NAME_USERS_SESSION_COOKIE),
            QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random())
        );
    cookie.set_expire_in(86400 * 5);  // 5 days
    f_snap->set_cookie(cookie);
//printf("session id [%s]\n", f_info->get_session_key().toUtf8().data());
}


/** \brief Check whether \p cpath matches our introducer.
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
 * \param[in] path_plugin  A pointer to the path plugin.
 * \param[in] cpath  The path being handled dynamically.
 */
void users::on_can_handle_dynamic_path(path::path *path_plugin, const QString& cpath)
{
    if(cpath == "user"                      // list of (public) users
    || cpath.left(5) == "user/"             // show a user profile (user/ is followed by the user identifier or some edit page such as user/password)
    || cpath == "profile"                   // the logged in user profile
    || cpath == "login"                     // form to log user in
    || cpath == "logout"                    // log user out
    || cpath == "register"                  // form to let new users register
    || cpath == "verify"                    // verification form so the user can enter his code
    || cpath.left(7) == "verify/"           // link to verify user's email; and verify/resend form
    || cpath == "forgot-password"           // form for users to reset their password
    || cpath == "new-password"              // form for users to enter their forgotten password verification code
    || cpath.left(13) == "new-password/")   // form for users to enter their forgotten password verification code
    {
        // tell the path plugin that this is ours
        path_plugin->handle_dynamic_path("user", this);
    }
}


/** \brief Execute the specified path.
 *
 * This is a dynamic page.
 *
 * \param[in] cpath  The canonalized path.
 */
bool users::on_path_execute(const QString& cpath)
{
    f_snap->output(layout::layout::instance()->apply_layout(cpath, this));

    return true;
}


void users::on_generate_main_content(layout::layout *l, const QString& cpath, QDomElement& page, QDomElement& body, const QString& ctemplate)
{
    if(cpath == "user")
    {
        // TODO: write user listing
        //list_users(body);
    }
    else if(cpath == "user/password/replace")
    {
        // this is a very special form that is accessible by users who
        // requested to change the password with the "forgot password"
        generate_replace_password_form(body);
    }
    else if(cpath.left(5) == "user/")
    {
        show_user(l, cpath, page, body);
    }
    else if(cpath == "profile")
    {
        // TODO: write user profile editor
        //user_profile(body);
    }
    else if(cpath == "login")
    {
        generate_login_form(body);
    }
    else if(cpath == "logout")
    {
        // closing current session if any and show the logout page
        logout_user(l, cpath, page, body);
    }
    else if(cpath == "register")
    {
        generate_register_form(body);
    }
    else if(cpath == "verify")
    {
        generate_verify_form(body);
    }
    else if(cpath == "verify/resend")
    {
        generate_resend_email_form(body);
    }
    else if(cpath.left(7) == "verify/")
    {
        verify_user(cpath);
    }
    else if(cpath == "forgot-password")
    {
        generate_forgot_password_form(body);
    }
    else if(cpath == "new-password")
    {
        generate_new_password_form(body);
    }
    else if(cpath.left(13) == "new-password/")
    {
        verify_password(cpath);
    }
    else
    {
        // any other user page is just like regular content
        content::content::instance()->on_generate_main_content(l, cpath, page, body, ctemplate);
    }
}


void users::on_generate_header_content(layout::layout *l, const QString& path, QDomElement& header, QDomElement& metadata, const QString& ctemplate)
{
    QDomDocument doc(header.ownerDocument());

    QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());

    // retrieve the row for that user
    if(!f_user_key.isEmpty() && users_table->exists(f_user_key))
    {
        QSharedPointer<QtCassandra::QCassandraRow> user_row(users_table->row(f_user_key));

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


void users::on_generate_page_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate)
{
    QDomDocument doc(page.ownerDocument());

    // retrieve the author
    QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
    const QString site_key(f_snap->get_site_key_with_slash());
    const QString page_key(site_key + path);
    QSharedPointer<QtCassandra::QCassandraRow> content_row(content_table->row(page_key));
    const QString link_name(get_name(SNAP_NAME_USERS_AUTHOR));
    links::link_info author_info(get_name(SNAP_NAME_USERS_AUTHOR), true, page_key);
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(author_info));
    links::link_info user_info;
    if(link_ctxt->next_link(user_info))
    {
        // an author is attached to this page
        const QString author_key(user_info.key());
        // all we want to offer here is the author details defined in the
        // /user/... location although we may want access to his email
        // address too (to display to an admin for example)
        QSharedPointer<QtCassandra::QCassandraRow> author_row(content_table->row(author_key));

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


void users::on_create_content(const QString& path, const QString& owner)
{
    if(!f_user_key.isEmpty())
    {
        //QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
        //QSharedPointer<QtCassandra::QCassandraRow> row(content_table->row(key));

        //const QString primary_owner(path::get_name(path::SNAP_NAME_PATH_PRIMARY_OWNER));
        //row->cell(primary_owner)->setValue(owner);

        QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());
        if(users_table->exists(f_user_key))
        {
            QtCassandra::QCassandraValue value(users_table->row(f_user_key)->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
            if(value.nullValue())
            {
                int64_t identifier(value.int64Value());
                const QString site_key(f_snap->get_site_key_with_slash());
                const QString user_key(site_key + get_name(SNAP_NAME_USERS_PATH) + QString("/%1").arg(identifier));
                const QString key(site_key + path);

                const QString link_name(get_name(SNAP_NAME_USERS_AUTHOR));
                const bool source_unique(true);
                links::link_info source(link_name, source_unique, key);
                const QString link_to(get_name(SNAP_NAME_USERS_AUTHORED_PAGES));
                const bool destination_multi(false);
                links::link_info destination(link_to, destination_multi, user_key);
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
void users::generate_replace_password_form(QDomElement& body)
{
    // make sure the user is properly setup
    if(!f_user_key.isEmpty())
    {
        // user is logged in already, send him to his normal password form
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

    QDomDocument doc(body.ownerDocument());

    QDomDocument replace_password_form(on_get_xml_form("user/password/replace"));
    if(replace_password_form.isNull())
    {
        // invalid (could not load the form!)
        return;
    }

    sessions::sessions::session_info info;
    info.set_session_type(info.SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_REPLACE_PASSWORD);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    info.set_page_path("user/password/replace");
    //info.set_object_path(); -- default is okay
    info.set_time_to_live(3600);  // 1h -- we want to have a JS that clears the data in 5 min. though
    QDomDocument result(form::form::instance()->form_to_html(info, replace_password_form));
    //f_snap->output(result.toString());

    {
        // we assume that the body content is valid because when we created it
        // we checked the data and if the user data was invalid XML then we
        // already saved a place holder warning to the user about the fact!
        QDomElement content_tag(doc.createElement("content"));
        body.appendChild(content_tag);
        content_tag.appendChild(doc.importNode(result.documentElement(), true));
    }

    { // /snap/page/body/titles/title
        QDomElement titles(doc.createElement("titles"));
        body.appendChild(titles);
        QDomElement title(doc.createElement("title"));
        titles.appendChild(title);
        QDomText text(doc.createTextNode(form::form::instance()->get_form_title("Replace Your Password")));
        title.appendChild(text);
    }
}


/** \brief Show the user profile.
 *
 * This function shows a user profile. By default one can use user/me to
 * see his profile. The administrators can see any profile. Otherwise
 * only public profiles and the user own profile are accessible.
 */
void users::show_user(layout::layout *l, const QString& cpath, QDomElement& page, QDomElement& body)
{
    int64_t identifier(0);
    QString user_id(cpath.mid(5));
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
            // TODO: save current path so login can come back here on success
            // redirect the user to the log in page
            f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
            NOTREACHED();
        }
        QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());
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
            generate_password_form(body);
            return;
        }

        // Probably not necessary to change this one
        //user_id = QString("%1").arg(identifier);
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
                    "User attempt to access user \"" + user_id + "\" which is not defined as a domain.");
            NOTREACHED();
        }

        // verify that the identifier indeed represents a user
        const QString site_key(f_snap->get_site_key_with_slash());
        const QString user_key(site_key + get_name(SNAP_NAME_USERS_PATH) + "/" + user_id);
        QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
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
printf("Got user [%ld]\n", identifier);
fflush(stdout);

    // generate the default body
        // TODO: write user profile viewer (i.e. we need to make use of the identifier here!)
        // WARNING: using a path such as "admin/.../profile" returns all the content of that profile
    content::content::instance()->on_generate_main_content(l, cpath, page, body, "admin/users/page/profile");
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
 *
 * \param[in] body  The body where we're to add the login form.
 */
void users::generate_password_form(QDomElement& body)
{
    if(f_user_key.isEmpty())
    {
        // user needs to be logged in to edit his password
        f_snap->die(snap_child::HTTP_CODE_FORBIDDEN,
                "Access Denied", "You need to be logged in and have enough permissions to access this page.",
                "user attempt to change a password without enough permissions.");
        NOTREACHED();
    }

    QDomDocument doc(body.ownerDocument());

    QDomDocument password_form(on_get_xml_form("user/password"));
    if(password_form.isNull())
    {
        // invalid (could not load the form!)
        return;
    }

    sessions::sessions::session_info info;
    info.set_session_type(info.SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_LOG_IN);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    info.set_page_path("user/password");
    //info.set_object_path(); -- default is okay
    info.set_time_to_live(3600);  // 1h -- we want to have a JS that clears the data in 5 min. though
    QDomDocument result(form::form::instance()->form_to_html(info, password_form));
    //f_snap->output(result.toString());

    {
        // we assume that the body content is valid because when we created it
        // we checked the data and if the user data was invalid XML then we
        // already saved a place holder warning to the user about the fact!
        QDomElement content_tag(doc.createElement("content"));
        body.appendChild(content_tag);
        content_tag.appendChild(doc.importNode(result.documentElement(), true));
    }

    { // /snap/page/body/titles/title
        QDomElement titles(doc.createElement("titles"));
        body.appendChild(titles);
        QDomElement title(doc.createElement("title"));
        titles.appendChild(title);
        QDomText text(doc.createTextNode(form::form::instance()->get_form_title("User Password")));
        title.appendChild(text);
    }
}


/** \brief Generate the login form.
 *
 * This function adds a compiled login form to the body content.
 * (i.e. this is the main page body content.)
 *
 * \param[in] body  The body where we're to add the login form.
 */
void users::generate_login_form(QDomElement& body)
{
    if(!f_user_key.isEmpty())
    {
        // user is logged in already, just send him to his profile
        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QDomDocument doc(body.ownerDocument());

    QDomDocument login_form(on_get_xml_form("login"));
    if(login_form.isNull())
    {
        // invalid (could not load the form!)
        return;
    }

    sessions::sessions::session_info info;
    info.set_session_type(info.SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_LOG_IN);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    info.set_page_path("login");
    //info.set_object_path(); -- default is okay
    info.set_time_to_live(3600);  // 1h -- we want to have a JS that clears the data in 5 min. though
    QDomDocument result(form::form::instance()->form_to_html(info, login_form));
    //f_snap->output(result.toString());

    {
        // we assume that the body content is valid because when we created it
        // we checked the data and if the user data was invalid XML then we
        // already saved a place holder warning to the user about the fact!
        QDomElement content_tag(doc.createElement("content"));
        body.appendChild(content_tag);
        content_tag.appendChild(doc.importNode(result.documentElement(), true));
    }

    { // /snap/page/body/titles/title
        QDomElement titles(doc.createElement("titles"));
        body.appendChild(titles);
        QDomElement title(doc.createElement("title"));
        titles.appendChild(title);
        QDomText text(doc.createTextNode(form::form::instance()->get_form_title("User Log In")));
        title.appendChild(text);
    }

    // use the current refererrer if there is one as the redirect page
    // after log in; once the log in is complete, redirect to this referrer
    // page; if you send the user on a page that only redirects to /login
    // then the user will end up on his profile (/user/me)
    if(sessions::sessions::instance()->get_from_session(*f_info, get_name(SNAP_NAME_USERS_LOGIN_REFERRER)).isEmpty())
    {
        QString referrer(f_snap->snapenv("HTTP_REFERER"));
        if(!referrer.isEmpty() && referrer != f_snap->get_site_key_with_slash() + "login")
        {
            attach_to_session(get_name(SNAP_NAME_USERS_LOGIN_REFERRER), referrer);
        }
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
 * \param[in] l  The layout concerned to generated this page.
 * \param[in] cpath  The path being processed (logout[/...]).
 * \param[in] page  The page XML data.
 * \param[in] body  The body XML data.
 */
void users::logout_user(layout::layout *l, QString cpath, QDomElement& page, QDomElement& body)
{
    // generate the body
    // we already logged the user out in the on_process_cookies() function
    if(cpath != "logout" && cpath != "logout/")
    {
        // make sure the page exists if the user was sent to antoher plugin
        // path (i.e. logout/fantom from the fantom plugin could be used to
        // display a different greating because the user was kicked out by
        // magic...); if it does not exist, force "logout" as the default
        QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
        if(!content_table->exists(cpath))
        {
            cpath = "logout";
        }
    }
    content::content::instance()->on_generate_main_content(l, cpath, page, body, "");
}


/** \brief Generate the registration form.
 *
 * This function adds a compiled registration form to the body content.
 * (i.e. this is the main page body content.)
 *
 * \param[in] body  The body where we're to add the registration form.
 */
void users::generate_register_form(QDomElement& body)
{
    if(!f_user_key.isEmpty())
    {
        // user is logged in already, just send him to his profile
        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QDomDocument doc(body.ownerDocument());

    QDomDocument register_form(on_get_xml_form("register"));
    if(register_form.isNull())
    {
        // invalid (could not load the form!)
        return;
    }

    sessions::sessions::session_info info;
    info.set_session_type(info.SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_REGISTER);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    info.set_page_path("register");
    //info.set_object_path(); -- default is okay
    info.set_time_to_live(3600);  // 1h -- we want to have a JS that clears the data in 5 min. though
    QDomDocument result(form::form::instance()->form_to_html(info, register_form));
    //f_snap->output(result.toString());

    {
        // we assume that the body content is valid because when we created it
        // we checked the data and if the user data was invalid XML then we
        // already saved a place holder warning to the user about the fact!
        QDomElement content_tag(doc.createElement("content"));
        body.appendChild(content_tag);
        content_tag.appendChild(doc.importNode(result.documentElement(), true));
    }

    { // /snap/page/body/titles/title
        QDomElement titles(doc.createElement("titles"));
        body.appendChild(titles);
        QDomElement title(doc.createElement("title"));
        titles.appendChild(title);
        QDomText text(doc.createTextNode(form::form::instance()->get_form_title("User Registration")));
        title.appendChild(text);
    }
}


/** \brief Generate the verification form.
 *
 * This function adds a compiled verification form to the body content.
 * (i.e. this is the main page body content.)
 *
 * This form shows one input box for the verification code the user
 * received in his email. It is customary to send the user to this
 * page right after a valid registration.
 *
 * \param[in] body  The body where we're to add the verification form.
 */
void users::generate_verify_form(QDomElement& body)
{
    if(!f_user_key.isEmpty())
    {
        // user is logged in already, just send him to his profile
        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QDomDocument doc(body.ownerDocument());

    QDomDocument verify_form(on_get_xml_form("verify"));
    if(verify_form.isNull())
    {
        // invalid (could not load the form!)
        return;
    }

    sessions::sessions::session_info info;
    info.set_session_type(info.SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_VERIFY);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    info.set_page_path("verify");
    //info.set_object_path(); -- default is okay
    info.set_time_to_live(3600);  // 1h -- we want to have a JS that clears the data in 5 min. though
    QDomDocument result(form::form::instance()->form_to_html(info, verify_form));
    //f_snap->output(result.toString());

    {
        // we assume that the body content is valid because when we created it
        // we checked the data and if the user data was invalid XML then we
        // already saved a place holder warning to the user about the fact!
        QDomElement content_tag(doc.createElement("content"));
        body.appendChild(content_tag);
        content_tag.appendChild(doc.importNode(result.documentElement(), true));
    }

    { // /snap/page/body/titles/title
        QDomElement titles(doc.createElement("titles"));
        body.appendChild(titles);
        QDomElement title(doc.createElement("title"));
        titles.appendChild(title);
        QDomText text(doc.createTextNode(form::form::instance()->get_form_title("User Verification")));
        title.appendChild(text);
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
 * \param[in] body  The body where we're to add the resend verification
 *                  email form.
 */
void users::generate_resend_email_form(QDomElement& body)
{
    if(!f_user_key.isEmpty())
    {
        // user is logged in already, just send him to his profile
        // XXX add a message?
        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QDomDocument doc(body.ownerDocument());

    QDomDocument resend_email_form(on_get_xml_form("verify/resend"));
    if(resend_email_form.isNull())
    {
        // invalid (could not load the form!)
        return;
    }

    sessions::sessions::session_info info;
    info.set_session_type(info.SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_RESEND_EMAIL);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    info.set_page_path("verify/resend");
    //info.set_object_path(); -- default is okay
    info.set_time_to_live(3600);  // 1h -- we want to have a JS that clears the data in 5 min. though
    QDomDocument result(form::form::instance()->form_to_html(info, resend_email_form));
    //f_snap->output(result.toString());

    {
        // we assume that the body content is valid because when we created it
        // we checked the data and if the user data was invalid XML then we
        // already saved a place holder warning to the user about the fact!
        QDomElement content_tag(doc.createElement("content"));
        body.appendChild(content_tag);
        content_tag.appendChild(doc.importNode(result.documentElement(), true));
    }

    { // /snap/page/body/titles/title
        QDomElement titles(doc.createElement("titles"));
        body.appendChild(titles);
        QDomElement title(doc.createElement("title"));
        titles.appendChild(title);
        QDomText text(doc.createTextNode(form::form::instance()->get_form_title("Resend Verification Code")));
        title.appendChild(text);
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
 *
 * \param[in] body  The body where we're to add the resend verification
 *                  email form.
 */
void users::generate_forgot_password_form(QDomElement& body)
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

    QDomDocument doc(body.ownerDocument());

    QDomDocument forgot_password_form(on_get_xml_form("forgot-password"));
    if(forgot_password_form.isNull())
    {
        // invalid (could not load the form!)
        return;
    }

    sessions::sessions::session_info info;
    info.set_session_type(info.SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_FORGOT_PASSWORD);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    info.set_page_path("forgot-password");
    //info.set_object_path(); -- default is okay
    info.set_time_to_live(3600);  // 1h -- we want to have a JS that clears the data in 5 min. though
    QDomDocument result(form::form::instance()->form_to_html(info, forgot_password_form));
    //f_snap->output(result.toString());

    {
        // we assume that the body content is valid because when we created it
        // we checked the data and if the user data was invalid XML then we
        // already saved a place holder warning to the user about the fact!
        QDomElement content_tag(doc.createElement("content"));
        body.appendChild(content_tag);
        content_tag.appendChild(doc.importNode(result.documentElement(), true));
    }

    { // /snap/page/body/titles/title
        QDomElement titles(doc.createElement("titles"));
        body.appendChild(titles);
        QDomElement title(doc.createElement("title"));
        titles.appendChild(title);
        QDomText text(doc.createTextNode(form::form::instance()->get_form_title("Forgot Password")));
        title.appendChild(text);
    }
}


/** \brief Allow the user to use his verification code to log in.
 *
 * This function verifies a verification code that was sent so the user
 * could change his password (i.e. an automatic log in mechanism.)
 *
 * \param[in] body  The body where we're to add the resend verification
 *                  email form.
 */
void users::generate_new_password_form(QDomElement& body)
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

    QDomDocument doc(body.ownerDocument());

    QDomDocument new_password_form(on_get_xml_form("new-password"));
    if(new_password_form.isNull())
    {
        // invalid (could not load the form!)
        return;
    }

    sessions::sessions::session_info info;
    info.set_session_type(info.SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_NEW_PASSWORD);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    info.set_page_path("new-password");
    //info.set_object_path(); -- default is okay
    info.set_time_to_live(3600);  // 1h -- we want to have a JS that clears the data in 5 min. though
    QDomDocument result(form::form::instance()->form_to_html(info, new_password_form));
    //f_snap->output(result.toString());

    {
        // we assume that the body content is valid because when we created it
        // we checked the data and if the user data was invalid XML then we
        // already saved a place holder warning to the user about the fact!
        QDomElement content_tag(doc.createElement("content"));
        body.appendChild(content_tag);
        content_tag.appendChild(doc.importNode(result.documentElement(), true));
    }

    { // /snap/page/body/titles/title
        QDomElement titles(doc.createElement("titles"));
        body.appendChild(titles);
        QDomElement title(doc.createElement("title"));
        titles.appendChild(title);
        QDomText text(doc.createTextNode(form::form::instance()->get_form_title("Forgotten Password Verification Code")));
        title.appendChild(text);
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
 * \param[in] cpath  The path used to access this page.
 */
void users::verify_user(const QString& cpath)
{
    if(!f_user_key.isEmpty())
    {
        // user is logged in already, just send him to his profile
        // (if logged in he was verified in some way!)
        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
        NOTREACHED();
    }

    QString session_id(cpath.mid(7));
    sessions::sessions::session_info info;
    sessions::sessions *session(sessions::sessions::instance());
    // TODO: remove the ending characters such as " ", "/", "\" and "|"?
    //       (it happens that people add those by mistake at the end of a URI...)
    session->load_session(session_id, info);
    const QString path(info.get_object_path());
    if(info.get_session_type() != sessions::sessions::session_info::SESSION_INFO_VALID
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
        return;
    }

    // it looks like the session is valid, get the user email and verify
    // that the account exists in the database
    QString email(path.mid(6));
    QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());
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
        return;
    }

    QSharedPointer<QtCassandra::QCassandraRow> row(users_table->row(email));
    const QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
    if(user_identifier.nullValue())
    {
        SNAP_LOG_FATAL("users::verify_user() could not load the user identifier, the row exists but the cell did not make it (")
                        (email)("/")
                        (get_name(SNAP_NAME_USERS_IDENTIFIER))(").");
        // redirect the user to the verification form although it won't work
        // next time either...
        f_snap->page_redirect("verify", snap_child::HTTP_CODE_SEE_OTHER);
        return;
    }
    const int64_t identifier(user_identifier.int64Value());
    const QString site_key(f_snap->get_site_key_with_slash());
    const QString user_key(site_key + get_name(SNAP_NAME_USERS_PATH) + QString("/%1").arg(identifier));

    // before we actually accept this verification code, we must make sure
    // the user is still marked as a new user (he should or the session
    // would be invalid, but for security it is better to check again)
    links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_key);
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
        return;
    }

    // a status link exists...
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
        return;
    }
    // remove the "user/new" status link so the user can now log in
    // he was successfully logged in
    links::links::instance()->delete_link(user_status_info);

    // Save the date when the user verified
    QtCassandra::QCassandraValue value;
    value.setInt64Value(f_snap->get_uri().option("start_date").toLongLong());
    row->cell(get_name(SNAP_NAME_USERS_VERIFIED_ON))->setValue(value);

    // Save the user IP address when verified
    value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
    row->cell(get_name(SNAP_NAME_USERS_VERIFIED_IP))->setValue(value);

    // TODO offer an auto-log in feature

    // send the user to the log in page since he got verified now
    messages::messages::instance()->set_info(
        "Verified!",
        "Thank you for taking the time to register an account with us. Your account is now verified! You can now log in with the form below."
    );
    f_snap->page_redirect("login", snap_child::HTTP_CODE_SEE_OTHER);
    NOTREACHED();
}


/** \brief Check that password verification code.
 *
 * This function verifies a password verification code that is sent to
 * the user whenever he says he forgot his password.
 *
 * \param[in] cpath  The path used to access this page.
 */
void users::verify_password(const QString& cpath)
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

    QString session_id(cpath.mid(13));

    sessions::sessions::session_info info;
    sessions::sessions *session(sessions::sessions::instance());
    // TODO: remove the ending characters such as " ", "/", "\" and "|"?
    //       (it happens that people add those by mistake at the end of a URI...)
    session->load_session(session_id, info);
    const QString path(info.get_object_path());
    if(info.get_session_type() != sessions::sessions::session_info::SESSION_INFO_VALID
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
    QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());
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

    QSharedPointer<QtCassandra::QCassandraRow> row(users_table->row(email));
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
    const int64_t identifier(user_identifier.int64Value());
    const QString site_key(f_snap->get_site_key_with_slash());
    const QString user_key(site_key + get_name(SNAP_NAME_USERS_PATH) + QString("/%1").arg(identifier));

    // before we actually accept this verification code, we must make sure
    // the user is still marked as a new user (he should or the session
    // would be invalid, but for security it is better to check again)
    links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_key);
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
    value.setInt64Value(f_snap->get_uri().option("start_date").toLongLong());
    row->cell(get_name(SNAP_NAME_USERS_FORGOT_PASSWORD_ON))->setValue(value);

    // Save the user IP address when verified
    value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
    row->cell(get_name(SNAP_NAME_USERS_FORGOT_PASSWORD_IP))->setValue(value);

    f_user_changing_password_key = email;

    // send the user to the log in page since he got verified now
    f_snap->page_redirect("user/password/replace", snap_child::HTTP_CODE_SEE_OTHER);
    NOTREACHED();
}


/** \brief Retrieve the XML form for that path.
 *
 * This function retrieves the XML form for the specified path. It is used
 * by the form plugin when a post is received to determine whether the
 * data is valid or not.
 *
 * \param[in] cpath  The canonalized path to return the XML data from.
 *
 * \return A DOM document with one of the users' forms.
 */
QDomDocument users::on_get_xml_form(const QString& cpath)
{
    // forms are saved as static variables so calling the function more
    // than once for the same form simply returns the same document
    static QDomDocument invalid_form;
    static QDomDocument email_form;
    static QDomDocument forgot_password_form;
    static QDomDocument login_form;
    static QDomDocument new_password_form;
    static QDomDocument password_form;
    static QDomDocument register_form;
    static QDomDocument replace_password_form;
    static QDomDocument resend_email_form;
    static QDomDocument verify_form;

    if(cpath == "forgot-password")
    {
        if(forgot_password_form.isNull())
        {
            // login page if user is not logged in, user account/stats otherwise
            QFile file(":/xml/users/forgot-password-form.xml");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not open forgot-password-form.xml resource file.");
                return invalid_form;
            }
            if(!forgot_password_form.setContent(&file, true))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not parse forgot-password-form.xml resource file.");
                return invalid_form;
            }
        }
        return forgot_password_form;
    }

    if(cpath == "login")
    {
        if(login_form.isNull())
        {
            // login page if user is not logged in, user account/stats otherwise
            QFile file(":/xml/users/login-form.xml");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not open login-form.xml resource file.");
                return invalid_form;
            }
            if(!login_form.setContent(&file, true))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not parse login-form.xml resource file.");
                return invalid_form;
            }
        }
        return login_form;
    }

    if(cpath == "new-password")
    {
        if(new_password_form.isNull())
        {
            // login page if user is not logged in, user account/stats otherwise
            QFile file(":/xml/users/new-password-form.xml");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not open new-password-form.xml resource file.");
                return invalid_form;
            }
            if(!new_password_form.setContent(&file, true))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not parse new-password-form.xml resource file.");
                return invalid_form;
            }
        }
        return new_password_form;
    }

    if(cpath == "user/password")
    {
        if(password_form.isNull())
        {
            // login page if user is not logged in, user account/stats otherwise
            QFile file(":/xml/users/password-form.xml");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not open password-form.xml resource file.");
                return invalid_form;
            }
            if(!password_form.setContent(&file, true))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not parse password-form.xml resource file.");
                return invalid_form;
            }
        }
        return password_form;
    }

    if(cpath == "register")
    {
        if(register_form.isNull())
        {
            // login page if user is not logged in, user account/stats otherwise
            QFile file(":/xml/users/register-form.xml");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not open register-form.xml resource file.");
                return invalid_form;
            }
            if(!register_form.setContent(&file, true))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not parse register-form.xml resource file.");
                return invalid_form;
            }
        }
        return register_form;
    }

    if(cpath == "user/password/replace")
    {
        if(replace_password_form.isNull())
        {
            // login page if user is not logged in, user account/stats otherwise
            QFile file(":/xml/users/replace-password-form.xml");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not open replace-password-form.xml resource file.");
                return invalid_form;
            }
            if(!replace_password_form.setContent(&file, true))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not parse replace-password-form.xml resource file.");
                return invalid_form;
            }
        }
        return replace_password_form;
    }

    if(cpath == "verify/resend")
    {
        if(resend_email_form.isNull())
        {
            // login page if user is not logged in, user account/stats otherwise
            QFile file(":/xml/users/resend-email-form.xml");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not open resend-email-form.xml resource file.");
                return invalid_form;
            }
            if(!resend_email_form.setContent(&file, true))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not parse resend-email-form.xml resource file.");
                return invalid_form;
            }
        }
        return resend_email_form;
    }

    if(cpath == "verify")
    {
        if(verify_form.isNull())
        {
            // verification page to enter secret code sent to a user
            // after registration
            QFile file(":/xml/users/verify-form.xml");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not open verify-form.xml resource file.");
                return invalid_form;
            }
            if(!verify_form.setContent(&file, true))
            {
                SNAP_LOG_FATAL("users::on_get_xml_form() could not parse verify-form.xml resource file.");
                return invalid_form;
            }
        }
        return verify_form;
    }

    return invalid_form;
}

/** \brief Process a post from one of the users forms.
 *
 * This function processes the post of a user form. The form is defined as
 * the session identifier.
 */
void users::on_process_post(const QString& cpath, const sessions::sessions::session_info& info)
{
    if(cpath == "login")
    {
        process_login_form();
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
        throw std::logic_error(("users::on_process_post() was called with an unsupported path: \"" + cpath + "\"").toUtf8().data());
    }
}


/** \brief Log the user in from the log in form.
 *
 * This function uses the credentials specified in the log in form.
 * The function searches for the user account and read its hashed
 * password and compare the password typed in the form. If it
 * matches, then the user receives a cookie and is logged in for
 * some time.
 */
void users::process_login_form()
{
    QString details;
    QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());
    QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());

    bool validation_required(false);

    // retrieve the row for that user
    QString key(f_snap->postenv("email"));
    if(users_table->exists(key))
    {
        QSharedPointer<QtCassandra::QCassandraRow> row(users_table->row(key));

        QtCassandra::QCassandraValue value;

        // existing users have a unique identifier
        QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
        if(user_identifier.nullValue())
        {
            messages::messages::instance()->set_error(
                "Could Not Log You In",
                "Somehow your user identifier is not available. Without we cannot log your in.",
                "users::process_login_form() could not load the user identifier, the row exists but the cell did not make it ("
                             + key + "/" + get_name(SNAP_NAME_USERS_IDENTIFIER) + ").",
                false
            );
            // XXX should we redirect to some error page in that regard?
            //     (i.e. your user account is messed up, please contact us?)
            f_snap->page_redirect("verify", snap_child::HTTP_CODE_SEE_OTHER);
            return;
        }
        int64_t identifier(user_identifier.int64Value());
        const QString site_key(f_snap->get_site_key_with_slash());
        QString user_key(site_key + get_name(SNAP_NAME_USERS_PATH) + QString("/%1").arg(identifier));

        // before we actually log the user in we must make sure he's
        // not currently blocked or not yet active
        links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_key);
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
        links::link_info status_info;
        bool force_redirect_password_change(false);
        bool valid(true);
        if(link_ctxt->next_link(status_info))
        {
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
            const QString digest(value.stringValue());

            // (2) we need the passord:
            const QString password(f_snap->postenv("password"));

            // (3) get the salt in a buffer
            value = row->cell(get_name(SNAP_NAME_USERS_PASSWORD_SALT))->value();
            const QByteArray salt(value.binaryValue());

            // (4) compute the expected hash
            QByteArray hash;
            encrypt_password(digest, password, salt, hash);

            // (5) retrieved the saved hash
            value = row->cell(get_name(SNAP_NAME_USERS_PASSWORD))->value();
            const QByteArray saved_hash(value.binaryValue());

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
                sessions::sessions::instance()->save_session(*f_info);

                http_cookie cookie(f_snap, get_name(SNAP_NAME_USERS_SESSION_COOKIE), QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random()));
                cookie.set_expire_in(86400 * 5);  // 5 days
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
                value.setInt64Value(f_snap->get_uri().option("start_date").toLongLong());
                row->cell(get_name(SNAP_NAME_USERS_LOGIN_ON))->setValue(value);

                // Save the user IP address when logged out
                value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
                row->cell(get_name(SNAP_NAME_USERS_LOGIN_IP))->setValue(value);

                if(force_redirect_password_change)
                {
                    f_snap->page_redirect("user/password", snap_child::HTTP_CODE_SEE_OTHER);
                }
                else
                {
                    // here we detach from the session since we want to
                    // redirect only once to that page
                    QString referrer(sessions::sessions::instance()->detach_from_session(*f_info, get_name(SNAP_NAME_USERS_LOGIN_REFERRER)));
                    if(referrer.isEmpty())
                    {
                        // User is now logged in, redirect him to another page
                        // TODO: give priority to the saved redirect... (which is not yet implemented!)
                        // go to the user profile (the admin needs to be able to change that default redirect)
                        f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
                    }
                    else
                    {
                        f_snap->page_redirect(referrer, snap_child::HTTP_CODE_SEE_OTHER);
                    }
                }
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
    QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());
    if(users_table->exists(email))
    {
        QSharedPointer<QtCassandra::QCassandraRow> row(users_table->row(email));

        // existing users have a unique identifier
        // necessary to create the user key below
        QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t identifier(user_identifier.int64Value());
            const QString site_key(f_snap->get_site_key_with_slash());
            QString user_key(site_key + get_name(SNAP_NAME_USERS_PATH) + QString("/%1").arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_key);
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
            if(status == "" || status == site_key + get_name(SNAP_NAME_USERS_PASSWORD_PATH))
            {
                // Only users considered active can request a new password
                forgot_password_email(email);

                // mark the user with the types/users/password tag
                const QString link_name(get_name(SNAP_NAME_USERS_STATUS));
                const bool source_unique(true);
                links::link_info source(link_name, source_unique, user_key);
                const QString link_to(get_name(SNAP_NAME_USERS_STATUS));
                const bool destination_unique(false);
                QString destination_key(site_key + get_name(SNAP_NAME_USERS_PASSWORD_PATH));
                links::link_info destination(link_to, destination_unique, destination_key);
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
    verify_password("new-password/" + session_id);
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
    QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());
    if(users_table->exists(f_user_changing_password_key))
    {
        QSharedPointer<QtCassandra::QCassandraRow> row(users_table->row(f_user_changing_password_key));

        // existing users have a unique identifier
        // necessary to create the user key below
        QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t identifier(user_identifier.int64Value());
            const QString site_key(f_snap->get_site_key_with_slash());
            QString user_key(site_key + get_name(SNAP_NAME_USERS_PATH) + QString("/%1").arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_key);
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
                if(status_info.key() == site_key + get_name(SNAP_NAME_USERS_PASSWORD_PATH))
                {
                    // We're good, save the new password and remove that link

                    // First encrypt the password
                    QString password(f_snap->postenv("password"));
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
                    sessions::sessions::instance()->save_session(*f_info);

                    http_cookie cookie(f_snap, get_name(SNAP_NAME_USERS_SESSION_COOKIE), QString("%1/%2").arg(f_info->get_session_key()).arg(f_info->get_session_random()));
                    cookie.set_expire_in(86400 * 5);  // 5 days
                    f_snap->set_cookie(cookie);

                    f_user_changing_password_key.clear();

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
    QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());
    if(users_table->exists(f_user_key))
    {
        // We're good, save the new password and remove that link
        QSharedPointer<QtCassandra::QCassandraRow> row(users_table->row(f_user_key));

        // existing users have a unique identifier
        // necessary to create the user key below
        QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t identifier(user_identifier.int64Value());
            const QString site_key(f_snap->get_site_key_with_slash());
            QString user_key(site_key + get_name(SNAP_NAME_USERS_PATH) + QString("/%1").arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_key);
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            bool delete_password_status(false);
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
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
            const QString old_digest(value.stringValue());

            // (2) we need the passord:
            const QString old_password(f_snap->postenv("old_password"));

            // (3) get the salt in a buffer
            value = row->cell(get_name(SNAP_NAME_USERS_PASSWORD_SALT))->value();
            const QByteArray old_salt(value.binaryValue());

            // (4) compute the expected hash
            QByteArray old_hash;
            encrypt_password(old_digest, old_password, old_salt, old_hash);

            // (5) retrieved the saved hashed password
            value = row->cell(get_name(SNAP_NAME_USERS_PASSWORD))->value();
            const QByteArray saved_hash(value.binaryValue());

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

                // once we sent the new code, we can send the user back
                // to the verify form
                messages::messages::instance()->set_info(
                    "Password Changed",
                    "Your new password was saved. Next time you want to log in, you must use your email with this new password."
                );
                f_snap->page_redirect("user/me", snap_child::HTTP_CODE_SEE_OTHER);
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
    QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());
    if(users_table->exists(email))
    {
        QSharedPointer<QtCassandra::QCassandraRow> row(users_table->row(email));

        // existing users have a unique identifier
        // necessary to create the user key below
        QtCassandra::QCassandraValue user_identifier(row->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
        if(!user_identifier.nullValue())
        {
            int64_t identifier(user_identifier.int64Value());
            const QString site_key(f_snap->get_site_key_with_slash());
            QString user_key(site_key + get_name(SNAP_NAME_USERS_PATH) + QString("/%1").arg(identifier));

            // verify the status of this user
            links::link_info user_status_info(get_name(SNAP_NAME_USERS_STATUS), true, user_key);
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(user_status_info));
            links::link_info status_info;
            if(link_ctxt->next_link(status_info))
            {
                // a status link exists...
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
    verify_user("verify/" + verification_code);
}


/** \brief Get the logged in user key.
 *
 * This function returns the key of the user that is currently logged
 * in. This key is the user's email address.
 *
 * If the user is not logged in, then his key is the empty string. This
 * is a fast way to know whether the current user is logged in:
 *
 * \code
 * if(users::users::instance()->get_user_key().isEmpty())
 * {
 *   // anonymous user code
 * }
 * else
 * {
 *   // logged in user code
 * }
 * \endcode
 *
 * \note
 * We return a copy of the key, opposed to a const reference, because really
 * it is too dangerous to allow someone from the outside to temper with this
 * variable.
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
        QSharedPointer<QtCassandra::QCassandraTable> users_table(const_cast<users *>(this)->get_users_table());
        if(users_table->exists(f_user_key))
        {
            const QtCassandra::QCassandraValue value(users_table->row(f_user_key)->cell(get_name(SNAP_NAME_USERS_IDENTIFIER))->value());
            if(value.nullValue())
            {
                const int64_t identifier(value.int64Value());
                return get_name(SNAP_NAME_USERS_PATH) + QString("/%1").arg(identifier);
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
bool users::register_user(const QString& email, const QString& password)
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

    QSharedPointer<QtCassandra::QCassandraTable> users_table(get_users_table());
    QString key(email);
    QSharedPointer<QtCassandra::QCassandraRow> row(users_table->row(key));

    QtCassandra::QCassandraValue value;
    value.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
    value.setStringValue(key);

    int64_t identifier(0);
    QString id_key(get_name(SNAP_NAME_USERS_ID_ROW));
    QString identifier_key(get_name(SNAP_NAME_USERS_IDENTIFIER));
    QtCassandra::QCassandraValue new_identifier;
    new_identifier.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);

    // we got as much as we could ready before locking
    {
        // first make sure this email is unique
        QtCassandra::QCassandraLock lock(f_snap->get_context(), key);

        // TODO: we have to look at all the possible email addresses
        const char *email_key(get_name(SNAP_NAME_USERS_ORIGINAL_EMAIL));
        QSharedPointer<QtCassandra::QCassandraCell> cell(row->cell(email_key));
        cell->setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
        QtCassandra::QCassandraValue email_data(cell->value());
        if(!email_data.nullValue())
        {
            // someone else already registered with that email
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
            QSharedPointer<QtCassandra::QCassandraRow> id_row(users_table->row(id_key));
            QSharedPointer<QtCassandra::QCassandraCell> id_cell(id_row->cell(identifier_key));
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
    uint64_t created_date(f_snap->get_uri().option("start_date").toLongLong());
    row->cell(get_name(SNAP_NAME_USERS_CREATED_TIME))->setValue(created_date);

    // Now create the user in the contents
    // (nothing else should be create at the path until now)
    QString user_path(get_name(SNAP_NAME_USERS_PATH));
    const QString site_key(f_snap->get_site_key_with_slash());
    QString user_key(user_path + QString("/%1").arg(identifier));
    content::content::instance()->create_content(user_key, get_plugin_name());

    // The "public" user account (i.e. in the content table) is limited
    // to the identifier at this point
    //
    // however, we also want to include a link defined as the status
    // at first the user is marked as being new
    // the destination URL is defined in the <link> content
    const QString link_name(get_name(SNAP_NAME_USERS_STATUS));
    const bool source_unique(true);
    links::link_info source(link_name, source_unique, site_key + user_key);
    const QString link_to(get_name(SNAP_NAME_USERS_STATUS));
    const bool destination_unique(false);
    QString destination_key(site_key + get_name(SNAP_NAME_USERS_NEW_PATH));
    links::link_info destination(link_to, destination_unique, destination_key);
    links::links::instance()->create_link(source, destination);

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
    e.set_email_path("admin/users/mail/verify");

    // verification makes use of a session identifier
    sessions::sessions::session_info info;
    info.set_session_type(sessions::sessions::session_info::SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_VERIFY_EMAIL);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    //info.set_page_path(); -- default is okay
    info.set_object_path("/user/" + email);
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
    e.set_email_path("admin/users/mail/forgot-password");

    // verification makes use of a session identifier
    sessions::sessions::session_info info;
    info.set_session_type(sessions::sessions::session_info::SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_FORGOT_PASSWORD_EMAIL);
    info.set_plugin_owner(get_plugin_name()); // ourselves
    //info.set_page_path(); -- default is okay
    info.set_object_path("/user/" + email);
    info.set_time_to_live(3600 * 8);  // 8 hours
    QString session(sessions::sessions::instance()->create_session(info));
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
        sessions::sessions::instance()->attach_to_session(*f_info, get_name(SNAP_NAME_USERS_CHANGING_PASSWORD_KEY), f_user_changing_password_key);
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
    f_user_changing_password_key = sessions::sessions::instance()->get_from_session(*f_info, get_name(SNAP_NAME_USERS_CHANGING_PASSWORD_KEY));
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
 * \exception std::logic_error
 * This exception is raised if the salt byte array is not exactly SALT_SIZE
 * bytes. For new passwords, you want to call the create_password_salt()
 * function to create the salt buffer.
 *
 * \exception std::runtime_error
 * This exception is raised if any of the OpenSSL digest functions fail.
 * This include an invalid digest name and adding/retrieving data to/from
 * the digest.
 *
 * \param[in] digest  The name of the digest to use (i.e. "sha512").
 * \param[in] password  The password to encrypt.
 * \param[in] salt  The salt information, necessary to encrypt passwords.
 * \param[out] hash  The resulting password hash.
 */
void users::encrypt_password(const QString& digest, const QString& password, const QByteArray& salt, QByteArray& hash)
{
    // it is an out only so reset it immediately
    hash.clear();

    // verify the size
    if(salt.size() != SALT_SIZE)
    {
        throw std::logic_error("salt buffer must be exactly SALT_SIZE bytes (missed calling create_password_salt()?)");
    }
    unsigned char buf[SALT_SIZE];
    memcpy(buf, salt.data(), SALT_SIZE);

    // Initialize so we gain access to all the necessary digests
    OpenSSL_add_all_digests();

    // retrieve the digest we want to use
    // (TODO: allows website owners to change this value)
    const EVP_MD *md(EVP_get_digestbyname(digest.toUtf8().data()));
    if(md == NULL)
    {
        throw std::runtime_error("the specified digest could not be found");
    }

    // initialize the digest context
    EVP_MD_CTX mdctx;
    EVP_MD_CTX_init(&mdctx);
    if(EVP_DigestInit_ex(&mdctx, md, NULL) != 1)
    {
        throw std::runtime_error("EVP_DigestInit_ex() failed digest initialization");
    }

    // add first salt
    if(EVP_DigestUpdate(&mdctx, buf, SALT_SIZE / 2) != 1)
    {
        throw std::runtime_error("EVP_DigestUpdate() failed digest update (salt1)");
    }

    // add password (encrypt to UTF-8)
    const char *pwd(password.toUtf8().data());
    if(EVP_DigestUpdate(&mdctx, pwd, strlen(pwd)) != 1)
    {
        throw std::runtime_error("EVP_DigestUpdate() failed digest update (password)");
    }

    // add second salt
    if(EVP_DigestUpdate(&mdctx, buf + SALT_SIZE / 2, SALT_SIZE / 2) != 1)
    {
        throw std::runtime_error("EVP_DigestUpdate() failed digest update (salt2)");
    }

    // retrieve the result of the hash
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len(EVP_MAX_MD_SIZE);
    if(EVP_DigestFinal_ex(&mdctx, md_value, &md_len) != 1)
    {
        throw std::runtime_error("EVP_DigestFinal_ex() digest finalization failed");
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
 * \li ...
 *
 * \param[in] f  The filter object.
 * \param[in] token  The token object, with the token name and optional parameters.
 */
//void users::on_replace_token(filter::filter *f, QDomDocument& xml, filter::filter::token_info_t& token)
//{
//    if(token.f_name.mid(0, 7) == "users::")
//    {
//    }
//}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
