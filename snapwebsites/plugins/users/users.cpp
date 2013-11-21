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
#include "../sessions/sessions.h"
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
    case SNAP_NAME_USERS_CREATED_TIME:
        return "users::created_time";

    case SNAP_NAME_USERS_ORIGINAL_EMAIL:
        return "users::original_email";

    case SNAP_NAME_USERS_ORIGINAL_IP:
        return "users::original_ip";

    case SNAP_NAME_USERS_PASSWORD:
        return "users::password";

    case SNAP_NAME_USERS_PASSWORD_DIGEST:
        return "users::password::digest";

    case SNAP_NAME_USERS_PASSWORD_SALT:
        return "users::password::salt";

    case SNAP_NAME_USERS_TABLE:
        return "users";

    case SNAP_NAME_USERS_USERNAME:
        return "users::username";

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
    SNAP_PLUGIN_UPDATE(2013, 11, 20, 1, 8, 0, content_update);

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
printf("add_xml()anew\n");
    content::content::instance()->add_xml("users");
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
    SNAP_LISTEN(users, "path", path::path, can_handle_dynamic_path, _1, _2);
    SNAP_LISTEN(users, "layout", layout::layout, generate_header_content, _1, _2, _3, _4);
    //SNAP_LISTEN(users, "layout", layout::layout, generate_page_content, _1, _2, _3, _4);
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
 * snap_session cookie and use it to know whether the user is currently
 * logged in.
 */
void users::on_process_cookies()
{
    // any snap session?
    if(!f_snap->cookie_is_defined("snap_session"))
    {
        return;
    }

    // is that session a valid user session?
    sessions::sessions::session_info info;
    QString user_session(f_snap->cookie("snap_session"));
    sessions::sessions::instance()->load_session(user_session, info);
    const QString path(info.get_object_path());
    if(info.get_session_type() != sessions::sessions::session_info::SESSION_INFO_VALID
    || info.get_session_id() != USERS_SESSION_ID_LOG_IN_SESSION
    || path.left(6) != "/user/")
    {
        // this is not a log in session, so we ignore it
        return;
    }

    // valid session, now verify the user
    const QString key(path.mid(6));
    QSharedPointer<QtCassandra::QCassandraTable> table(get_users_table());
    if(!table->exists(key))
    {
        // this is not a valid user email address
        return;
    }
    f_user_key = key;

    // refresh the session and the cookie (so it extends it)
    // TBD: here Drupal refreshes the session identifier, should we
    //      as well? it could be problematic though
    info.set_time_to_live(86400 * 5);  // 5 days
    http_cookie cookie(f_snap, "snap_session", user_session);
    cookie.set_expire_in(86400 * 5);  // 5 days
    f_snap->set_cookie(cookie);
//printf("session id [%s]\n", user_session.toUtf8().data());
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
    if(cpath == "user"                  // list of (public) users
    || cpath.left(5) == "user/"         // show a user profile
    || cpath == "profile"               // the logged in user profile
    || cpath == "login"                 // form to log user in
    || cpath == "logout"                // log user out
    || cpath == "register"              // form to let new users register
    || cpath == "verify"                // link to verify user's email
    || cpath == "forgot-passowrd")      // form for users to reset their password
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


void users::on_generate_main_content(layout::layout *l, const QString& cpath, QDomElement& page, QDomElement& body)
{
    if(cpath == "user")
    {
        // TODO: write user listing
        //list_users(body);
    }
    else if(cpath.left(5) == "user/")
    {
        // TODO: write user profile viewer
        //show_user(body);
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
        // TODO: write log out feature
        // closing current session if any and show the logout page
    }
    else if(cpath == "register")
    {
        generate_register_form(body);
    }
    else if(cpath == "verify")
    {
        //verify_user();
    }
    else if(cpath == "forgot-password")
    {
        // TODO: create forget password form
    }
    else
    {
        // a type is just like a regular page
        content::content::instance()->on_generate_main_content(l, cpath, page, body);
    }
}


void users::on_generate_header_content(layout::layout *l, const QString& path, QDomElement& header, QDomElement& metadata)
{
    QDomDocument doc(header.ownerDocument());

    QSharedPointer<QtCassandra::QCassandraTable> table(get_users_table());

    // retrieve the row for that user
    if(!f_user_key.isEmpty() && table->exists(f_user_key))
    {
        QSharedPointer<QtCassandra::QCassandraRow> row(table->row(f_user_key));

        {   // snap/head/metadata/desc[type=users::email]/data
            QDomElement desc(doc.createElement("desc"));
            desc.setAttribute("type", "users::email");
            metadata.appendChild(desc);
            QDomElement data(doc.createElement("data"));
            desc.appendChild(data);
            QDomText text(doc.createTextNode(f_user_key));
            data.appendChild(text);
        }

        {   // snap/head/metadata/desc[type=user_name]/data
            QtCassandra::QCassandraValue value(row->cell(get_name(SNAP_NAME_USERS_USERNAME))->value());
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

        {   // snap/head/metadata/desc[type=user_created]/data
            QtCassandra::QCassandraValue value(row->cell(get_name(SNAP_NAME_USERS_CREATED_TIME))->value());
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


/** \brief Generate the login form.
 *
 * This function adds a compiled login form to the body content.
 * (i.e. this is the main page body content.)
 *
 * \param[in] body  The body where we're to add the login form.
 */
void users::generate_login_form(QDomElement& body)
{
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
    info.set_plugin_owner("users"); // ourselves
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
    info.set_plugin_owner("users"); // ourselves
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
    static QDomDocument invalid_form;
    static QDomDocument login_form;
    static QDomDocument register_form;
    static QDomDocument password_form;

    if(cpath == "login")
    {
        if(login_form.isNull())
        {
            // login page if user is not logged in, user account/stats otherwise
            QFile file(":/xml/users/login-form.xml");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("user::on_path_execute() could not open login-form.xml resource file.");
                return invalid_form;
            }
            if(!login_form.setContent(&file, true))
            {
                SNAP_LOG_FATAL("user::on_path_execute() could not parse login-form.xml resource file.");
                return invalid_form;
            }
        }
        return login_form;
    }

    if(cpath == "register")
    {
        if(register_form.isNull())
        {
            // login page if user is not logged in, user account/stats otherwise
            QFile file(":/xml/users/register-form.xml");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("user::on_path_execute() could not open register-form.xml resource file.");
                return invalid_form;
            }
            if(!register_form.setContent(&file, true))
            {
                SNAP_LOG_FATAL("user::on_path_execute() could not parse register-form.xml resource file.");
                return invalid_form;
            }
        }
        return register_form;
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
    else if(cpath == "forgot-password")
    {
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
    QSharedPointer<QtCassandra::QCassandraTable> table(get_users_table());

    // retrieve the row for that user
    QString key(f_snap->postenv("email"));
    if(table->exists(key))
    {
        QSharedPointer<QtCassandra::QCassandraRow> row(table->row(key));

        QtCassandra::QCassandraValue value;

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
            // User credentials are correct, create a cookie
            sessions::sessions::session_info info;
            info.set_session_type(sessions::sessions::session_info::SESSION_INFO_USER);
            info.set_session_id(USERS_SESSION_ID_LOG_IN_SESSION);
            info.set_plugin_owner("users"); // ourselves
            //info.set_page_path(); -- default is okay
            info.set_object_path("/user/" + key);
            info.set_time_to_live(86400 * 5);  // 5 days
            QString session(sessions::sessions::instance()->create_session(info));
            http_cookie cookie(f_snap, "snap_session", session);
            cookie.set_expire_in(86400 * 5);  // 5 days
            f_snap->set_cookie(cookie);

            // this is now the current user
            f_user_key = key;

            // User is now logged in, redirect him to another page
            // TODO: redirect...
            return;
        }
        else
        {
            // user mistyped his password?
            details = "invalid credentials (password doesn't match)";
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

    // user not registered yet? (or email misspelled)
    messages::messages::instance()->set_error(
        "Could Not Log You In",
        "Your email or password were incorrect. If you are not registered, you may want to consider <a href=\"/register\">registering</a> first?.",
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

    QSharedPointer<QtCassandra::QCassandraTable> table(get_users_table());
    QString key(email);
    QSharedPointer<QtCassandra::QCassandraRow> row(table->row(key));

    QtCassandra::QCassandraValue value;
    value.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
    value.setStringValue(key);

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

        // the lock automatically goes away here
    }

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
    e.set_email_path("admin/users/mail/verify");

    // verification makes use of a session identifier
    sessions::sessions::session_info info;
    info.set_session_type(sessions::sessions::session_info::SESSION_INFO_USER);
    info.set_session_id(USERS_SESSION_ID_VERIFY_EMAIL);
    info.set_plugin_owner("users"); // ourselves
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




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
