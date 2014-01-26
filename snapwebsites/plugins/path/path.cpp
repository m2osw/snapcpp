// Snap Websites Server -- path handling
// Copyright (C) 2011-2014  Made to Order Software Corp.
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

#include "path.h"

#include "../messages/messages.h"

#include "not_reached.h"

#include <iostream>

#include "poison.h"



SNAP_PLUGIN_START(path, 1, 0)

/** \brief Get a fixed path name.
 *
 * The path plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
//char const *get_name(name_t name)
//{
//    // Note: <branch>.<revision> are actually replaced by a full version
//    //       when dealing with JavaScript and CSS files (Version: field)
//    switch(name) {
//    case SNAP_NAME_CONTENT_PRIMARY_OWNER:
//        return "path::primary_owner";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid SNAP_NAME_PATH_...");
//
//    }
//    NOTREACHED();
//}







/** \brief Called by plugins that can handle dynamic paths.
 *
 * Some plugins handle a very large number of paths in a fully
 * dynamic manner, which means that they can generate the data
 * for any one of those paths in a way that is extremely fast
 * without the need of creating millions of entries in the
 * database.
 *
 * These plugins are given a chance to handle a path whenever
 * the content plugin calls the can_handle_dynamic_path() signal.
 * At that point, a plugin can respond by calling this function
 * with itself.
 *
 * For example, a plugin that displays a date in different formats
 * could be programmed to understand the special path:
 *
 * \code
 * /formatted-date/YYYYMMDD/FMT
 * \endcode
 *
 * which could be a request to the system to format the date
 * YYYY-MM-DD using format FMT.
 *
 * \param[in] p  The plugin that can handle the path specified in the signal.
 */
void dynamic_plugin_t::set_plugin(plugins::plugin *p)
{
//std::cerr << "handle_dynamic_path(" << p->get_plugin_name() << ")\n";
    if(f_plugin != NULL)
    {
        // two different plugins are fighting for the same path
        // we'll have to enhance our error to give the user a way to choose
        // the plugin one wants to use for this request...
        content::content::instance()->get_snap()->die(snap_child::HTTP_CODE_MULTIPLE_CHOICE,
                    "Multiple Choices",
                    "This page references multiple plugins and the server does not currently have means of choosing one over the other.",
                    "User tried to access dynamic page but more than one plugin says it owns the resource, primary is \""
                            + f_plugin->get_plugin_name() + "\", second request by \"" + p->get_plugin_name());
        NOTREACHED();
    }

    f_plugin = p;
}




/** \brief Initialize the path plugin.
 *
 * This function initializes the path plugin.
 */
path::path()
    //: f_snap(NULL) -- auto-init
{
}


/** \brief Destroy the path plugin.
 *
 * This function cleans up the path plugin.
 */
path::~path()
{
}


/** \brief Get a pointer to the path plugin.
 *
 * This function returns an instance pointer to the path plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the path plugin.
 */
path *path::instance()
{
    return g_plugin_path_factory.instance();
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
QString path::description() const
{
    return "This plugin manages the path to a page. This is used to determine"
        " the plugin that knows how to handle the data displayed to the user"
        " when given a specific path.";
}


/** \brief Bootstrap the path.
 *
 * This function adds the events the path plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void path::on_bootstrap(::snap::snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(path, "server", server, execute, _1);
}


/** \brief Retrieve the plugin corresponding to a path.
 *
 * This function searches for the plugin that is to be used to handle the
 * given path.
 *
 * \param[in,out] ipath  The path to be probed.
 * \param[in,out] err_callback  Interface implementation to call on errors.
 *
 * \return A pointer to the plugin that owns this path.
 */
plugins::plugin *path::get_plugin(content::path_info_t& ipath, permission_error_callback& err_callback)
{
    // get the name of the plugin that owns this URL 
    plugins::plugin *owner_plugin(NULL);

    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    if(content_table->exists(ipath.get_key())
    && content_table->row(ipath.get_key())->exists(content::get_name(content::SNAP_NAME_CONTENT_PRIMARY_OWNER)))
    {
        // get the modified date so we can setup the Last-Modified HTTP header field
        // it is also a way to determine that a path is valid
        QtCassandra::QCassandraValue value(content_table->row(ipath.get_key())->cell(content::get_name(content::SNAP_NAME_CONTENT_CREATED))->value());
        QString owner = content_table->row(ipath.get_key())->cell(QString(content::get_name(content::SNAP_NAME_CONTENT_PRIMARY_OWNER)))->value().stringValue();
        if(value.nullValue() || owner.isEmpty())
        {
            err_callback.on_error(snap_child::HTTP_CODE_NOT_FOUND,
                        "Invalid Page",
                        "An internal error occured and this page cannot properly be displayed at this time.",
                        QString("User tried to access page \"%1\" but it does not look valid (null value? %2, empty owner? %3)")
                                .arg(ipath.get_key())
                                .arg(static_cast<int>(value.nullValue()))
                                .arg(static_cast<int>(owner.isEmpty())));
            return NULL;
        }
		// TODO: this is not correct anymore! (we're getting the creation date, not last mod.)
        f_last_modified = value.int64Value();

        // get the primary owner (plugin name) and retrieve the plugin pointer
//std::cerr << "Execute [" << ipath.get_key() << "] with plugin [" << owner << "]\n";
        owner_plugin = plugins::get_plugin(owner);
        if(owner_plugin == NULL)
        {
            // if the plugin cannot be found then either it was mispelled
            // or the plugin is not currently installed...
            f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                        "Plugin Missing",
                        "This page is not currently available as its plugin is not currently installed.",
                        "User tried to access page \"" + ipath.get_cpath() + "\" but its plugin (" + owner + ") does not exist (not installed? mispelled?)");
            NOTREACHED();
        }
    }
    else
    {
        // this key doesn't exist as is in the database, but...
        // it may be a dynamically defined path, check for a
        // plugin that would have defined such a path
//std::cerr << "Testing for page dynamically [" << ipath.get_cpath() << "]\n");
        dynamic_plugin_t dp;
        can_handle_dynamic_path(ipath, dp);
        owner_plugin = dp.get_plugin();
    }

    if(owner_plugin != NULL)
    {
        // got a valid plugin, verify that the user has permission
        verify_permissions(ipath, err_callback);
    }

    return owner_plugin;
}


/** \brief Verify for permissions.
 *
 * This function calculates the permissions of the user to access the
 * specified path with the specified action. If the result is that the
 * current user does not have permission to access the page, then the
 * function checks whether the user is logged in. If not, he gets
 * sent to the log in page after saving the current path as the place
 * to come back after logging in. If the user is already logged in,
 * then an Access Denied error is generated.
 *
 * \param[in,out] ipath  The path which permissions are being checked.
 * \param[in,out] err_callback  An object with on_error() and on_redirect()
 *                              functions.
 */
void path::verify_permissions(content::path_info_t& ipath, permission_error_callback& err_callback)
{
    QString qs_action(f_snap->get_server_parameter("qs_action"));
    QString action;
    snap_uri const& uri(f_snap->get_uri());
    if(uri.has_query_option(qs_action))
    {
        // the user specified an action
        action = uri.query_option(qs_action);
    }
    if(action.isEmpty())
    {
        // use the default
        action = default_action(ipath);
    }

    // save the action found in the URI so that way any plugin can access
    // that information at any point, not just the verify_rights() function
    f_snap->set_action(action);

    // only actions that are defined in the permission types are
    // allowed, anything else is funky action from a hacker or
    // whatnot and we just die with an error in that case
    validate_action(ipath, action, err_callback);
}


/** \brief Check whether a user has permission to access a page.
 *
 * This event is sent to all plugins that want to check for permissions.
 * In general, just the permissions plugin does that work, but other
 * plugins can also check. The result is true by default and if any
 * plugin decides that the page is not accessible, the result is set
 * to false. A plugin is not allowed to set the flag back to false.
 *
 * \param[in] user_path  The path to the user being checked.
 * \param[in,out] path  The path being checked.
 * \param[in] action  The action being checked.
 * \param[in] login_status  The status the user is in.
 * \param[in,out] result  The returned result.
 *
 * \return true if the signal should be propagated.
 */
bool path::access_allowed_impl(QString const& user_path, content::path_info_t& ipath, QString const& action, QString const& login_status, content::permission_flag& result)
{
    (void) user_path;
    (void) ipath;
    (void) action;
    (void) login_status;
    return result.allowed();
}


/** \brief Validate the user action.
 *
 * This function validates the user action. If invalid or if that means
 * the user does not have enough rights to access the specified path,
 * then the event calls die() at some point and returns.
 *
 * \param[in,out] ipath  The path being validated.
 * \param[in] action  The action being performed against \p path.
 * \param[in,out] err_callback  Call functions on errors.
 *
 * \return true if the event has to carry on.
 */
bool path::validate_action_impl(content::path_info_t& ipath, QString const& action, permission_error_callback& err_callback)
{
    (void) ipath;
    (void) action;
    (void) err_callback;
    return true;
}

/** \brief Dynamically compute the default action.
 *
 * Depending on the path and method (GET, POST, DELETE, PUT...) the system
 * reacts with a default action.
 */
QString path::default_action(content::path_info_t& ipath)
{
    if(f_snap->has_post())
    {
        // this could also be "edit" or "create"...
        // but "administer" is more restrictive at this point
        return "administer";
    }

    if(ipath.get_cpath() == "admin" || ipath.get_cpath().startsWith("admin/"))
    {
        return "administer";
    }

    return "view";
}


/** \brief Analyze the URL and execute the corresponding callback.
 *
 * This function looks for the page that needs to be displayed
 * from the URL information.
 *
 * \todo
 * Should we also test with case insensitive paths? (i.e. if all
 * else failed) Or should we make sure URL is all lowercase and
 * thus always make it case insensitive?
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void path::on_execute(QString const& uri_path)
{
    class error_callback : public permission_error_callback
    {
    public:
        error_callback(snap_child *snap)
            : f_snap(snap)
        {
        }

        void on_error(snap_child::http_code_t err_code, QString const& err_name, QString const& err_description, QString const& err_details)
        {
            f_snap->die(err_code, err_name, err_description, err_details);
            NOTREACHED();
        }

        void on_redirect(
                /* message::set_error() */ QString const& err_name, QString const& err_description, QString const& err_details, bool err_security,
                /* snap_child::page_redirect() */ QString const& path, snap_child::http_code_t http_code)
        {
            // TODO: remove this message dependency
            messages::messages::instance()->set_error(err_name, err_description, err_details, err_security);
            f_snap->page_redirect(path, http_code, err_description, err_details);
            NOTREACHED();
        }

    private:
        zpsnap_child_t      f_snap;
    } main_page_error_callback(f_snap);

    content::path_info_t ipath;
    ipath.set_path(uri_path);
    ipath.set_main_page(true);
#ifdef DEBUG
std::cerr << "path::on_execute(\"" << uri_path << "\") -> [" << ipath.get_cpath() << "] [" << ipath.get_branch() << "] [" << ipath.get_revision() << "]\n";
#endif

    f_last_modified = 0;
    plugins::plugin *path_plugin(get_plugin(ipath, main_page_error_callback));

    // The last modification date is saved in the get_plugin()
    // It's a bit ugly but that way we test there that the page is valid and
    // we avoid having to search that information again to define the
    // corresponding header. However, it cannot be done in the get_plugin()
    // function since it may be called for other pages than the main page.
    //
    // ddd, dd MMM yyyy hh:mm:ss +0000
    if(0 != f_last_modified)
    {
        f_snap->set_header("Last-Modified", f_snap->date_to_string(f_last_modified, snap_child::DATE_FORMAT_HTTP));
    }

    // if a plugin pointer was defined we expect that the dynamic_cast<> will
    // always work, however path_plugin may be NULL
    path_execute *pe(dynamic_cast<path_execute *>(path_plugin));
    if(pe == NULL)
    {
        // not found, give a chance to some plugins to do something with the
        // current data (i.e. auto-search, internally redirect to a nice
        // Page Not Found page, etc.)
        page_not_found(ipath);
        if(f_snap->empty_output())
        {
            // no page_not_found() plugin support...
            if(path_plugin != NULL)
            {
                // if the page exists then
                QString const owner(path_plugin->get_plugin_name());
                f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                            "Plugin Missing",
                            "This page is not currently available as its plugin is not currently installed.",
                            "User tried to access page \"" + ipath.get_cpath() + "\" but its plugin (" + owner + ") does not yet implement the path_execute");
            }
            else
            {
                f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                            "Page Not Found",
                            "This page does not exist on this website.",
                            "User tried to access page \"" + ipath.get_cpath() + "\" and no dynamic path handling happened");
            }
            NOTREACHED();
        }
    }
    else
    {
        // execute the path for real

        // if the user POSTed something, manage that content first, the
        // effect is often to redirect the user in which case we want to
        // emit an HTTP Location and return; also, with AJAX we may end
        // up stopping early (i.e. not generate a full page but instead
        // return the "form results".)
        f_snap->process_post();

        if(!pe->on_path_execute(ipath))
        {
            // TODO (TBD):
            // page_not_found() not called here because the page exists
            // it's just not available right now and thus we
            // may not want to replace it with something else?
            f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                    "Page Not Present",
                    "Somehow this page is not currently available.",
                    "User tried to access page \"" + ipath.get_cpath() + "\" but its plugin (" + path_plugin->get_plugin_name() + ") refused it");
            NOTREACHED();
        }
    }
}


/** \brief Default implementation of the dynamic path handler.
 *
 * This function doesn't do anything as the path plugin does not itself
 * offer another way to handle a path than checking the database (which
 * has priority and thus this function never gets called if that happens.)
 *
 * \param[in] ipath  The canonicalized path to be checked.
 * \param[in.out] plugin_info  Will hold the plugin that can handle this
 *                             dynamic path.
 *
 * \return true if the signal is to be propagated to all the other plugins.
 */
bool path::can_handle_dynamic_path_impl(content::path_info_t& ipath, dynamic_plugin_t& plugin_info)
{
    (void) ipath;
    (void) plugin_info;
    return true;
}


/** \brief Default implementation of the page not found signal.
 *
 * This function doesn't do anything as the path plugin does not itself
 * offer another way to handle a path than checking the database (which
 * has priority and thus this function never gets called if that happens.)
 *
 * If no other plugin transforms the result then a standard, plain text
 * 404 will be presented to the user.
 *
 * \param[in] ipath  The canonicalized path that generated a page not found.
 */
bool path::page_not_found_impl(content::path_info_t& ipath)
{
    (void) ipath;
    return true;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
