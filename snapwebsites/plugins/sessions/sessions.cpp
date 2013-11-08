// Snap Websites Server -- manage sessions for users, forms, etc.
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

#include "sessions.h"
#include "../../lib/plugins.h"
#include "../../lib/not_reached.h"
#include "../content/content.h"
#include <QtCassandra/QCassandraValue.h>
#include <openssl/rand.h>
#include <iostream>


SNAP_PLUGIN_START(sessions, 1, 0)


/** \brief Get a fixed sessions plugin name.
 *
 * The sessions plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char *get_name(name_t name)
{
	switch(name) {
	case SNAP_NAME_SESSIONS_TABLE:
		return "sessions";

	case SNAP_NAME_SESSIONS_ID:
		return "sessions::id";

	case SNAP_NAME_SESSIONS_PLUGIN_OWNER:
		return "sessions::plugin_owner";

	case SNAP_NAME_SESSIONS_PAGE_PATH:
		return "sessions::page_path";

	case SNAP_NAME_SESSIONS_OBJECT_PATH:
		return "sessions::object_path";

	case SNAP_NAME_SESSIONS_TIME_TO_LIVE:
		return "sessions::time_to_live";

	case SNAP_NAME_SESSIONS_TIME_LIMIT:
		return "sessions::time_limit";

	case SNAP_NAME_SESSIONS_REMOTE_ADDR:
		return "sessions::remote_addr";

	case SNAP_NAME_SESSIONS_USED_UP:
		return "sessions::used_up";

	default:
		// invalid index
		throw snap_exception();

	}
	NOTREACHED();
}


/** \brief Initialize the session info object.
 *
 * By default a session object is initialized with the following parameters:
 *
 * \li type -- SESSION_INFO_SECURE, the most secure type of session (also the
 *                                  slowest)
 * \li session id -- 0, a default session identifier; this is specific to the
 *                      plugins using this session and 0 is not expected to be
 *                      a valid session identifier for any plugin;
 * \li plugin owner -- "", the name of the plugin that created this session and
 *                         that will process the results
 * \li page path -- "", the path to the page being managed
 *                      (should be set to get_site_key_with_slash())
 * \li object path -- "", the object being built with this session
 *                        (i.e. the user block for the small user log in form)
 * \li time to live -- 300, five minutes which is about right for a secure
 *                          session; this is often changed to one day (86400)
 *                          for standard forms; and to one week (604800) for
 *                          fully public forms (i.e. search form)
 * \li time limit -- 0 (not limited), limit (set in seconds) of when the
 *                   session goes out of date; this is an exact date when
 *                   live, but always reset all sessions at midnight the
 *                   session expires (i.e. you may give a session 1h to no
 *                   matter what)
 */
sessions::session_info::session_info()
	//: f_type(SESSION_INFO_SECURE) -- auto-init
	: f_session_id()
	//, f_plugin_owner("") -- auto-init
	//, f_page_path("") -- auto-init
	//, f_object_path("") -- auto-init
	//, f_time_to_live(300) -- auto-init
	//, f_time_limit(0) -- auto-init
{
}

/** \brief Set the type of session.
 *
 * By default a session object is marked as a secure session (SESSION_INFO_SECURE.)
 *
 * We currently support the following session types:
 *
 * \li SESSION_INFO_SECURE
 *
 * This type of session is expected to have a very short time to live (i.e. 5 min. on an
 * e-commerce site payment area, 1h for a standard logged in user.) This type of session
 * uses 128 bits.
 *
 * \li SESSION_INFO_USER
 *
 * This type of session is expected to be used for user cookies when not accessing an
 * e-commerce site. This type of session uses 64 bits. It should not be used with long
 * lasting logged in users (i.e. cookies that last more than a few hours should use a
 * secure session with 128 bits to better avoid hackers.)
 *
 * \li SESSION_INFO_FORM
 *
 * The form type of session is used to add an identifier in forms that hackers cannot
 * easily determine but without taking any processing time to generate on the server.
 * This type of session should never be used for a cookie representing a logged in
 * user. However, it could be used to track anonymous users. This type of session
 * uses 32 bits.
 *
 * \param[in] type  The new type for this session object.
 *
 * \sa get_session_type()
 */
void sessions::session_info::set_session_type(session_info_type_t type)
{
	// TODO: get controlled_vars to support enumerations
	f_type = static_cast<int>(type);
}

/** \brief Define a session identifier.
 *
 * This function accepts a session identifier (a number) which represents
 * what this session is about (i.e. the user log in form may use 1 and
 * the user registration may use 2, etc.)
 *
 * The identifier is used when a dynamic identifier is not required to
 * determine what we're expected to do once we get the identifier.
 *
 * \param[in] id  The identifier of this session.
 *
 * \sa get_session_id()
 */
void sessions::session_info::set_session_id(session_id_t id)
{
	f_session_id = id;
}

/** \brief Set the session owner which is the name of a plugin.
 *
 * This function defined the session owner as the name of the a plugin.
 * This is used by the different low level functions to determine which
 * of the plugins is responsible to process a request.
 *
 * \param[in] plugin_owner  The name of the owner of that plugin.
 *
 * \sa get_plugin_owner()
 */
void sessions::session_info::set_plugin_owner(const QString& plugin_owner)
{
	f_plugin_owner = plugin_owner;
}

/** \brief The path to the page where this session identifier is used.
 *
 * For session identifiers that are specific to a page (i.e. a form) this is used to
 * link the session to the page so a user cannot use the same session identifier on
 * another page.
 *
 * Note that the page session identifier is only used for a form that this page
 * represents. Forms that appear in blocks make use of the object path.
 *
 * For cookies that track people this parameter can remain empty for anonymous users
 * and it is set to the user page for logged in users. This way if someone attempts
 * to use the wrong session identifier we can detect it.
 *
 * \param[in] page_path  The path representing the page in question for this session.
 *
 * \sa get_page_path()
 * \sa set_object_path()
 */
void sessions::session_info::set_page_path(const QString& page_path)
{
	f_page_path = page_path;
}

/** \brief The path of the object displaying this content.
 *
 * This path represents the object being displayed. For example, the smaller user
 * log in form (i.e. the log in block) is shown on many pages. Because of that, we
 * cannot use the path to the page and instead we use the path to the object
 * (i.e. the user log in block.)
 *
 * The path should be specified only if it is going to be checked later.
 *
 * \param[in] object_path  The path of the object in question.
 *
 * \sa get_object_path()
 * \sa set_page_path()
 */
void sessions::session_info::set_object_path(const QString& object_path)
{
	f_object_path = object_path;
}

/** \brief The time to live of this session.
 *
 * All sessions have a maximum life time of five minutes by default. This function
 * is used to change the default to a smaller (although unlikely it can be done)
 * or a larger number.
 *
 * Sessions that run out of time do not get deleted immediately from the database,
 * but they are not considered valid so attempting to use them fails with a time
 * out error.
 *
 * For example, a log in form may have a time out of 5 minutes. If you wait more
 * time before logging in your account, the form times out and trying to log in
 * fails (include a JavaScript and you can clearly time out the client side as
 * well.) Imagine this simple scenario:
 *
 * \li User comes to your site
 * \li User clicks to go to the log in screen
 * \li User enters user name and password
 * \li User does not log in yet (gets a phone call...)
 * \li User leaves the computer
 * \li Another user comes to the computer
 * \li That other user just has to click the "Log In" button...
 * \li The account is compromised!
 *
 * The minimum time to live accepted is 1 minute and one second (61 or more.) But
 * you probably should never create a session of less than 5 minutes (500).
 *
 * Setting a sessiong time to live to 0 means that the session never expire. It
 * should really only be used when a form doesn't need to expire (i.e. a search
 * form) but even though, it is most likely not a good idea. Although it can be
 * used when you set a hard coded time limit on a session.
 *
 * \sa get_time_to_live()
 * \sa set_time_limit()
 */
void sessions::session_info::set_time_to_live(int32_t time_to_live)
{
	f_time_to_live = time_to_live;
}

/** \brief Limit the time by date.
 *
 * This function saves the time limit of a session to the specified date.
 * In this case, the date is absolute. We use the standard Unix date (i.e.
 * number of seconds since Jan 1, 1970.)
 *
 * After that date the session becomes invalid. It is not necessarilly
 * removed from the database, although if a users attempts to use it, it
 * will fail.
 *
 * In most cases you want to use the time_to_live parameter rather than a
 * hard coded Unix timestamp.
 *
 * When creating a session, the limit must represent at least 1 minute and
 * one second from now (now is defined as the start date defined in the
 * snap_child object.)
 *
 * A limit of zero means that the time limit is not used.
 *
 * \param[in] time_limit  The time when the session becomes completely invalid.
 *
 * \sa get_time_limit()
 * \sa set_time_to_live()
 */
void sessions::session_info::set_time_limit(time_t time_limit)
{
	f_time_limit = time_limit;
}

/** \brief Retrieve the type of this session.
 *
 * This function is used to retrieve the type of this session.
 *
 * More details can be found in the set_session_type() function.
 *
 * \return The type of the session.
 *
 * \sa set_session_type()
 */
sessions::session_info::session_info_type_t sessions::session_info::get_session_type() const
{
	return f_type;
}

/** \brief Define a session identifier.
 *
 * This function returns the session identifier of this session.
 *
 * See the set_session_id() function for more information.
 *
 * \return The identifier of this session.
 *
 * \sa set_session_id()
 */
sessions::session_info::session_id_t sessions::session_info::get_session_id() const
{
	return f_session_id;
}

/** \brief Set the session owner which is the name of a plugin.
 *
 * This function defined the session owner as the name of the a plugin.
 * This is used by the different low level functions to determine which
 * of the plugins is responsible to process a request.
 *
 * \param[in] plugin_owner  The name of the owner of that plugin.
 *
 * \sa set_plugin_owner()
 */
const QString& sessions::session_info::get_plugin_owner() const
{
	return f_plugin_owner;
}

/** \brief Retrieve the path of the page linked to this session.
 *
 * This function is used to retrieve the type of this session.
 *
 * More details can be found in the set_session_type() function.
 *
 * \return The type of the session.
 *
 * \sa set_page_path()
 * \sa get_object_path()
 */
const QString& sessions::session_info::get_page_path() const
{
	return f_page_path;
}

/** \brief Get the path of the attached object.
 *
 * A session may be created for a specific object that may appear on
 * many different pages. This object path can be retrieved using
 * this function.
 *
 * \return This session object path.
 *
 * \sa set_object_path()
 * \sa get_page_path()
 */
const QString& sessions::session_info::get_object_path() const
{
	return f_object_path;
}

/** \brief Get the time to live of this session.
 *
 * This function returns the time this session will live. After that time, use of
 * the session fails (even if everything else is valid.)
 *
 * See the set_time_to_live() function for more information.
 *
 * \return The time to live of this session in seconds.
 *
 * \sa set_time_to_live()
 * \sa get_time_limit()
 */
int32_t sessions::session_info::get_time_to_live() const
{
	return f_time_to_live;
}

/** \brief Get the time limit of this session.
 *
 * This function returns the Unix date when the session goes out of business.
 * After that specific date, the session is considered invalid.
 *
 * See the set_time_limit() function for more information.
 *
 * \return The session Unix time limit.
 *
 * \sa set_time_limit()
 * \sa get_time_to_live()
 */
time_t sessions::session_info::get_time_limit() const
{
	return f_time_limit;
}



/** \brief Initialize the sessions plugin.
 *
 * This function is used to initialize the sessions plugin object.
 */
sessions::sessions()
	//: f_snap(NULL) -- auto-init
{
}

/** \brief Clean up the sessions plugin.
 *
 * Ensure the sessions object is clean before it is gone.
 */
sessions::~sessions()
{
}

/** \brief Initialize the sessions.
 *
 * This function terminates the initialization of the sessions plugin
 * by registering for different events it supports.
 *
 * \param[in] snap  The child handling this request.
 */
void sessions::on_bootstrap(snap_child *snap)
{
	f_snap = snap;
}

/** \brief Get a pointer to the sessions plugin.
 *
 * This function returns an instance pointer to the sessions plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the sessions plugin.
 */
sessions *sessions::instance()
{
	return g_plugin_sessions_factory.instance();
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
QString sessions::description() const
{
	return "The sessions plugin is used by many other plugins to generate"
		" session identifiers and save information about the given session."
		" This is useful for many different reasons. In case of a user, a"
		" session is used to make sure that the same user comes back to the"
		" website. It is also used by forms to make sure that a for submission"
		" is valid.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run yet.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t sessions::do_update(int64_t last_updated)
{
	SNAP_PLUGIN_UPDATE_INIT();

	SNAP_PLUGIN_UPDATE(2012, 12, 29, 13, 45, 0, content_update);

	SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief Update the content with our references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void sessions::content_update(int64_t variables_timestamp)
{
	content::content::instance()->add_xml("sessions");
}

/** \brief Initialize the sessions table.
 *
 * This function creates the sessions table if it doesn't exist yet. Otherwise
 * it simple returns the existing Cassandra table.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \note
 * At this point this function is private because we don't think it should
 * directly be accessible from the outside. Note that this table includes
 * all the sessions for all the websites running on a system.
 *
 * \return The pointer to the sessions table.
 */
QSharedPointer<QtCassandra::QCassandraTable> sessions::get_sessions_table()
{
	return f_snap->create_table(get_name(SNAP_NAME_SESSIONS_TABLE), "Sessions table.");
}

/** \brief Implementation of the generate_sessions signal.
 *
 * This function readies the generate_sessions signal.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool sessions::generate_sessions_impl(sessions * /*r*/)
{
	return true;
}

/** \brief Generate the actual content of the statistics page.
 *
 * This function generates the contents of the statistics page of the
 * sessions plugin.
 *
 * \param[in] l  The layout used to generate this page.
 * \param[in] path  The path to this page.
 * \param[in] page  The page element being generated.
 * \param[in] body  The body element being generated.
 */
void sessions::on_generate_main_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body)
{
	// generate the statistics in the body then call the content generator
	// (how do we do that at this point? do we assume that the backend takes
	// care of it?)
	content::content::instance()->on_generate_main_content(l, path, page, body);
}

/** \brief Create a new session.
 *
 * This function creates a new session using the specified information.
 * Later one can load a session to verify the validity of some data
 * such as a form post or a user cookie.
 *
 * The function returns the session identifier which includes letters
 * and digits (A-Za-z0-9).
 *
 * The session must include a path (either the page or object path as
 * defined in the session \p info parameter.) This path is used as the
 * key to access the session information. It is also the path used to
 * verify the session again when necessary. If both, the page and the
 * object are defined, then the page has priority and it becomes the
 * session database key.
 *
 * \note
 * The bit size of the source of the entropy (random values) is more
 * important than the bit size of the actual session token. For example,
 * an MD5 hash produces a 128 bit value. However, the MD5 hash of
 * incremental values, a timestamp, or 8-bit random numbers are each
 * insecure because the source of the random values can be easily predicted.
 * Consequently, the 128 bit size does not represent an accurate measure
 * of the session token. The minimum size of the entropy source is 32 bits,
 * although larger pools (48 or 64 bits) may be necessary for sites with
 * over 10,000 concurrent users per hour.
 *
 * \warning
 * The function checks that the time the session will live is over 1
 * minute. Anything smaller than that and you get a throw.
 *
 * \bugs
 * We are using the OpenSSL RAND_bytes() function that makes use of a
 * context to know which randomizer to use. We should look into forcing
 * a specific generator when called.
 *
 * \param[in] info  The session information.
 *
 * \return The session identifier.
 */
QString sessions::create_session(const session_info& info)
{
	// creating a session of less than 1 minute?!
	time_t time_limit(info.get_time_limit());
	int32_t time_to_live(info.get_time_to_live());
	int64_t start_date(f_snap->get_start_date());
	time_t now(start_date / 1000000); // in seconds
	if((time_limit != 0 && time_limit <= now + 60)
	|| (time_to_live != 0 && time_to_live <= 60))
	{
		throw std::runtime_error("you cannot create a session of 1 minute or less");
	}

	// make sure that we have at least one path defined
	// (this is our session key so it is required)
	const QString& page_path(info.get_page_path());
	const QString& object_path(info.get_object_path());
	if(page_path.isEmpty() && object_path.isEmpty())
	{
		throw std::runtime_error("any session must have at least one path defined");
	}

	// TODO? Need we set a specific OpenSSL random generator?
	//       Although the default works for session identifiers
	//       someone could change that under our feet (since it
	//       looks like those functions have a global context)

	// the maximum size we currently use is 16 bytes (128 bits)
	unsigned char buf[16];

	int size(0);
	switch(info.get_session_type())
	{
	case session_info::SESSION_INFO_SECURE:
		size = 16;
		break;

	case session_info::SESSION_INFO_USER:
		size = 8;
		break;

	case session_info::SESSION_INFO_FORM:
		size = 4;
		break;

	default:
		throw std::logic_error("used an undefined session type in create_session()");

	}

	// generate the session identifier
	int r(RAND_bytes(buf, size));
	if(r != 1)
	{
		throw std::runtime_error("RAND_bytes() could not generate a random number.");
	}

	// make the key specific to that website and append the session identifier
	QString key(f_snap->get_website_key() + "/");
	QString result;
	for(int i(0); i < size; ++i)
	{
		QString hex(QString("%1").arg(static_cast<int>(buf[i]), 2, 16, static_cast<QChar>('0')));
		key += hex;
		result += hex;
	}

	// define timestamp for the session value in seconds
	int64_t timestamp(0);
	if(time_limit == 0)
	{
		if(time_to_live == 0)
		{
			// never expire we use 1 year which is
			// way over the head of everyone
			timestamp = now + 86400 * 364;
		}
		else
		{
			timestamp = now + time_to_live;
		}
	}
	else
	{
		if(time_to_live == 0)
		{
			timestamp = time_limit;
		}
		else
		{
			timestamp = now + time_to_live;
			if(timestamp < time_limit)
			{
				// keep the largest dead line time
				timestamp = time_limit;
			}
		}
	}
	// keep it in the database for 1 more day than what we need it for
	// the difference should always fit 32 bits
	int64_t ttl(timestamp + 86400 - now);
	if(ttl < 0 || ttl > 0x7FFFFFFF)
	{
		throw std::logic_error("the session computed ttl is out of bounds");
	}

	QSharedPointer<QtCassandra::QCassandraTable> table(get_sessions_table());
	QSharedPointer<QtCassandra::QCassandraRow> row(table->row(key));

	QtCassandra::QCassandraValue value;
	value.setTtl(static_cast<int32_t>(ttl));

	value.setInt32Value(info.get_session_id());
	row->cell(get_name(SNAP_NAME_SESSIONS_ID))->setValue(value);

	value.setStringValue(info.get_plugin_owner());
	row->cell(get_name(SNAP_NAME_SESSIONS_PLUGIN_OWNER))->setValue(value);

	value.setStringValue(info.get_page_path());
	row->cell(get_name(SNAP_NAME_SESSIONS_PAGE_PATH))->setValue(value);

	value.setStringValue(info.get_object_path());
	row->cell(get_name(SNAP_NAME_SESSIONS_OBJECT_PATH))->setValue(value);

	value.setInt32Value(info.get_time_to_live());
	row->cell(get_name(SNAP_NAME_SESSIONS_TIME_TO_LIVE))->setValue(value);

	value.setInt64Value(timestamp);
	row->cell(get_name(SNAP_NAME_SESSIONS_TIME_LIMIT))->setValue(value);

	value.setStringValue(f_snap->snapenv("REMOTE_ADDR"));
	row->cell(get_name(SNAP_NAME_SESSIONS_REMOTE_ADDR))->setValue(value);

	return result;
}

/** \brief Load a session previously created with create_session().
 *
 * This function loads a session that one previously created with the
 * create_session() function.
 *
 * The info parameter gets reset by the function, which means that all
 * the input values are overwritten. It then sets the session type to
 * one of the following values to determine the validity of the data:
 *
 * \li SESSION_INFO_VALID -- the session is considered valid and it can
 * be used safely; the session info is perfectly valid
 *
 * \li SESSION_INFO_MISSING -- the session is missing; in most cases this
 * is because a hacker attempted to post a session and it was already
 * discarded (i.e. the hacker is reusing the same identifier over and
 * over again) or the hacker used a random number that doesn't exist in
 * the database; the session info is not valid
 *
 * \li SESSION_INFO_USED_UP -- the session was already used; it is not
 * possible to re-use it again; the session info is otherwise valid
 *
 * \li SESSION_INFO_INCOMPATIBLE -- the session is not compatible as some
 * parameters do not match the expected values; this can be set by the
 * caller (at this point this very function doesn't use this error code);
 * the session info is other valid
 *
 * \param[in] session_id  The session identifier to load.
 * \param[out] info  The variable where the session variables get saved.
 * \param[in] use_once  Whether this session can be used more than once.
 */
void sessions::load_session(const QString& session_id, session_info& info, bool use_once)
{
	// reset this info (although it is likely already brand new...)
	info = session_info();

	QString key(f_snap->get_website_key() + "/" + session_id);

	QSharedPointer<QtCassandra::QCassandraTable> table(get_sessions_table());
	if(!table->exists(key))
	{
		// if the key doesn't exists it was either tempered with
		// or the database already deleted it (i.e. it timed out)
		info.set_session_type(session_info::SESSION_INFO_MISSING);
		return;
	}

	QSharedPointer<QtCassandra::QCassandraRow> row(table->row(key));
	if(!row)
	{
		// XXX
		// if we get a problem here it's probably something else
		// than a missing row...
		info.set_session_type(session_info::SESSION_INFO_MISSING);
		return;
	}

	QtCassandra::QCassandraValue value;

	value = row->cell(get_name(SNAP_NAME_SESSIONS_ID))->value();
	if(value.nullValue())
	{
		// row timed out between calls
		info.set_session_type(session_info::SESSION_INFO_MISSING);
		return;
	}
	info.set_session_id(value.int32Value());

	value = row->cell(get_name(SNAP_NAME_SESSIONS_PLUGIN_OWNER))->value();
	if(value.nullValue())
	{
		// row timed out between calls
		info.set_session_type(session_info::SESSION_INFO_MISSING);
		return;
	}
	info.set_plugin_owner(value.stringValue());

	value = row->cell(get_name(SNAP_NAME_SESSIONS_PAGE_PATH))->value();
	info.set_page_path(value.stringValue());

	value = row->cell(get_name(SNAP_NAME_SESSIONS_OBJECT_PATH))->value();
	info.set_object_path(value.stringValue());

	value = row->cell(get_name(SNAP_NAME_SESSIONS_TIME_TO_LIVE))->value();
	if(value.nullValue())
	{
		// row timed out between calls
		info.set_session_type(session_info::SESSION_INFO_MISSING);
		return;
	}
	info.set_time_to_live(value.int32Value());

	value = row->cell(get_name(SNAP_NAME_SESSIONS_TIME_LIMIT))->value();
	if(value.nullValue())
	{
		// row timed out between calls
		info.set_session_type(session_info::SESSION_INFO_MISSING);
		return;
	}
	info.set_time_limit(value.int64Value());

	// At this point we don't have a field in the info structure for this one
	// value = row->cell(get_name(SNAP_NAME_SESSIONS_REMOTE_ADDR))->value();

	int64_t start_date(f_snap->get_start_date());
	time_t now(start_date / 1000000); // in seconds
	if(info.get_time_limit() < now)
	{
		info.set_session_type(session_info::SESSION_INFO_OUT_OF_DATE);
		return;
	}

	if(use_once)
	{
		value = row->cell(get_name(SNAP_NAME_SESSIONS_USED_UP))->value();
		if(value.nullValue())
		{
			// IMPORTANT NOTE:
			// As a side effect, since we just read values with a TTL
			// this 'value' variable already has the expected TTL!
			value.setCharValue(1);
			row->cell(get_name(SNAP_NAME_SESSIONS_USED_UP))->setValue(value);
		}
		else
		{
			info.set_session_type(session_info::SESSION_INFO_USED_UP);
			return;
		}
	}

	// only case when it is valid
	info.set_session_type(session_info::SESSION_INFO_VALID);
}

SNAP_PLUGIN_END()

// vim: ts=4 sw=4
