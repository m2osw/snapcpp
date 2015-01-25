// Snap Websites Server -- OAuth2 handling
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
 * \brief OAuth2 handling.
 *
 * This plugin handles authentication via OAuth2 by application that
 * want to access private features of a Snap! Website.
 *
 * This plugin does not offer any REST API by itself. Only an authentication
 * process.
 */

#include "oauth2.h"

#include "../users/users.h"
//#include "../messages/messages.h"
//#include "../sendmail/sendmail.h"
//#include "../server_access/server_access.h"

#include "http_strings.h"
//#include "qstring_stream.h"
//#include "not_reached.h"
#include "log.h"

//#include <iostream>
//
//#include <QtCassandra/QCassandraLock.h>
//
//#include <openssl/evp.h>
//#include <openssl/rand.h>
//#include <boost/static_assert.hpp>
//#include <QFile>

#include "poison.h"


SNAP_PLUGIN_START(oauth2, 1, 0)



/** \brief Get a fixed oauth2 plugin name.
 *
 * The oauth2 plugin makes use of different names in the database. This
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
    case SNAP_NAME_OAUTH2_EMAIL:
        return "oauth2::email";

    case SNAP_NAME_OAUTH2_ENABLE:
        return "oauth2::enable";

    case SNAP_NAME_OAUTH2_IDENTIFIER:
        return "oauth2::identifier";

    case SNAP_NAME_OAUTH2_SECRET:
        return "oauth2::secret";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_OAUTH2_...");

    }
    NOTREACHED();
}


/** \class oauth2
 * \brief The oauth2 plugin handles application authentication.
 *
 * Any Snap! website can be setup to accept application authentication.
 *
 * The website generates a token that can be used to log you in.
 */


/** \brief Initialize the oauth2 plugin.
 *
 * This function initializes the oauth2 plugin.
 */
oauth2::oauth2()
    //: f_snap(nullptr) -- auto-init
{
}

/** \brief Destroy the oauth2 plugin.
 *
 * This function cleans up the oauth2 plugin.
 */
oauth2::~oauth2()
{
}


/** \brief Get a pointer to the oauth2 plugin.
 *
 * This function returns an instance pointer to the oauth2 plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the oauth2 plugin.
 */
oauth2 *oauth2::instance()
{
    return g_plugin_oauth2_factory.instance();
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
QString oauth2::description() const
{
    return "The OAuth2 plugin offers an authentication mechanism to"
          " be used by all the other plugins that support a REST API."
          " The administrator of a website can decide whether to authorize"
          " such access or not.";
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
int64_t oauth2::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 1, 23, 13, 39, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the oauth2 plugin content.
 *
 * This function updates the contents in the database using the
 * system update settings found in the resources.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void oauth2::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Bootstrap the oauth2.
 *
 * This function adds the events the oauth2 plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void oauth2::on_bootstrap(::snap::snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(oauth2, "content", content::content, create_content, _1, _2, _3);
}


/** \brief Called each time a page gets created.
 *
 * We use this signal to make sure that the OAuth2 identifier and secret
 * are defined. This will always happens after the settings page is created.
 */
void oauth2::on_create_content(content::path_info_t& ipath, QString const& owner, QString const& type)
{
    static_cast<void>(type);

    if(owner != "output"
    || ipath.get_cpath() != "admin/settings/oauth2")
    {
        return;
    }

    struct secret_t
    {
        static void create(name_t n)
        {
            content::content *content_plugin(content::content::instance());
            QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());

            // make sure the secret does not include a ':' which is not
            // compatible with Basic Auth
            QString secret(users::users::create_password());
            for(;;)
            {
                secret.remove(':');
                if(secret.length() > 64)
                {
                    break;
                }
                QString const extra(users::users::create_password());
                secret += extra;
            }

            content::path_info_t ipath;
            ipath.set_path("admin/settings/oauth2");
            secret_table->row(ipath.get_key())->cell(get_name(n))->setValue(secret);
        }
    };

    secret_t::create(SNAP_NAME_OAUTH2_IDENTIFIER);
    secret_t::create(SNAP_NAME_OAUTH2_SECRET);
}


/** \brief Check for the "/user/oauth2" path.
 *
 * This function ensures that the URL is /user/oauth2 and if so write
 * checks that the application knows the identifier and secret of this
 * website and if so, return a session identifier that can be used to
 * further access the server including private pages.
 *
 * \param[in] ipath  The URL being managed.
 *
 * \return true if the authentication parameters were properly defined,
 *         an error is generated otherwise.
 */
bool oauth2::on_path_execute(content::path_info_t& ipath)
{
    if(ipath.get_cpath() == "user/oauth2")
    {
        // accept XML and JSON
        http_strings::WeightedHttpString encodings(f_snap->snapenv("HTTP_ACCEPT"));
        float const xml_level(encodings.get_level("application/xml"));
        float const json_level(encodings.get_level("application/json"));

        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t secret_table(content_plugin->get_secret_table());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
        content::path_info_t settings_ipath;
        settings_ipath.set_path("admin/settings/oauth2");
        int8_t const enable(revision_table->row(settings_ipath.get_key())->cell(get_name(SNAP_NAME_OAUTH2_ENABLE))->value().safeSignedCharValue());
        if(!enable)
        {
            f_snap->die(snap_child::HTTP_CODE_UNAUTHORIZED,
                        "Unauthorized Authentication",
                        "This website does not authorize OAuth2 authentications at the moment.",
                        "The OAuth2 system is currently disabled.");
            NOTREACHED();
        }
        QString const email(revision_table->row(settings_ipath.get_key())->cell(get_name(SNAP_NAME_OAUTH2_EMAIL))->value().stringValue());
        if(email.isEmpty())
        {
            f_snap->die(snap_child::HTTP_CODE_UNAUTHORIZED,
                        "Invalid Settings",
                        "Your OAuth2 settings do not include a user email for us to log your application in.",
                        "The OAuth2 system is currently \"disabled\" because no user email was specified.");
            NOTREACHED();
        }

        // Retrieve Authorization Field
        QString const authorization(f_snap->snapenv("HTTP_AUTHORIZATION"));
        QStringList const basic_base64(authorization.simplified().split(" "));
        if(basic_base64.size() != 2
        || basic_base64[0].toUpper() != "BASIC")
        {
            f_snap->die(snap_child::HTTP_CODE_UNAUTHORIZED,
                        "Unauthorized Method of Authentication",
                        "We only support the Basic authentication method.",
                        "The authorization did not have 2 parts (Basic and Secret)");
            NOTREACHED();
        }

        // Decrypt the buffer
        QByteArray const base64_buffer(QByteArray::fromBase64(basic_base64[1].toUtf8()));
        QStringList const identifier_secret(QString::fromUtf8(base64_buffer.data()).split(":"));
        if(identifier_secret.size() != 2)
        {
            f_snap->die(snap_child::HTTP_CODE_BAD_REQUEST,
                        "Invalid Authentication",
                        "The authentication identifier and secret codes are expected to include only one colon character.",
                        "The expected authorization \"id:secret\" not available.");
            NOTREACHED();
        }

        // Check validity (i.e. is the application logged in?)
        QString const identifier(secret_table->row(settings_ipath.get_key())->cell(get_name(SNAP_NAME_OAUTH2_IDENTIFIER))->value().stringValue());
        QString const secret(secret_table->row(settings_ipath.get_key())->cell(get_name(SNAP_NAME_OAUTH2_SECRET))->value().stringValue());
        if(identifier != identifier_secret[0]
        || secret     != identifier_secret[1])
        {
            f_snap->die(snap_child::HTTP_CODE_FORBIDDEN,
                        "Forbidden Authentication",
                        "Your OAuth2 identifier and secret do not match this website OAuth2 information.",
                        QString("Invalid%1%2")
                                .arg(identifier != identifier_secret[0] ? " identifier" : "")
                                .arg(secret     != identifier_secret[1] ? " secret"     : ""));
            NOTREACHED();
        }

        // create a new user session
        bool validation_required(false);
        users::users *users_plugin(users::users::instance()->instance());
        QString const details(users_plugin->login_user(email, "", validation_required));

        // generate the result, an OAuth2 session
        if(json_level > xml_level)
        {
            // TODO: we want an XSLT for this process
            f_snap->die(snap_child::HTTP_CODE_NOT_IMPLEMENTED,
                        "No JSON Support Yet",
                        "The OAuth2 does not yet have support for JSON.",
                        "TODO: Implement the OAuth2 XML to JSON XSLT file.");
            NOTREACHED();
        }
        else
        {
            f_snap->output(QString(
                "<?xml version=\"1.0\"?>"
                "<snap>"
                    "<oauth2-session>%1</oauth2-session>"
                    "<result>%2</result>"
                    "%3"
                "</snap>")
                    .arg("session-id")
                    .arg(details.isEmpty() ? "success" : "failure")
                    .arg(details.isEmpty() ? "" : QString("<error>%1</error>")
                            .arg(validation_required ? "The account you chose as the OAuth2 account was not yet validated."
                                                     : "Your OAuth2 credentials were incorrect."))
                );

            if(!details.isEmpty())
            {
                SNAP_LOG_ERROR("Could not log this application in because the user attached to this website OAuth2 was not accepted. Details: ")(details);
            }
        }
        return true;
    }

    return false;
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
