// Snap Websites Server -- path handling
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

#include "path.h"

#include "../links/links.h"
#include "../messages/messages.h"
#include "../server_access/server_access.h"

#include "not_reached.h"
#include "log.h"
#include "qstring_stream.h"

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(path, 1, 0)

/* \brief Get a fixed path name.
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
//    switch(name)
//    {
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




class path_error_callback : public permission_error_callback
{
public:
    path_error_callback(snap_child *snap, content::path_info_t ipath)
        : f_snap(snap)
        , f_ipath(ipath)
        //, f_plugin(nullptr)
    {
    }

    void set_plugin(plugins::plugin *p)
    {
        f_plugin = p;
    }

    void on_error(snap_child::http_code_t err_code, QString const& err_name, QString const& err_description, QString const& err_details, bool const err_by_mime_type)
    {
        if(err_by_mime_type && f_plugin)
        {
            // will this plugin handle that error?
            permission_error_callback::error_by_mime_type *handle_error(dynamic_cast<permission_error_callback::error_by_mime_type *>(f_plugin.get()));
            if(handle_error)
            {
                // attempt to inform the user using the proper type of data
                // so that way it is easier to debug than sending HTML
                try
                {
                    // define a default error name if undefined
                    QString http_name;
                    f_snap->define_http_name(err_code, http_name);

                    // log the error
                    SNAP_LOG_FATAL("path::on_error(): ")(err_details)(" (")(static_cast<int>(err_code))(" ")(err_name)(": ")(err_description)(")");

                    // On error we do not return the HTTP protocol, only the Status field
                    // it just needs to be first to make sure it works right
                    f_snap->set_header("Status", QString("%1 %2\n")
                            .arg(static_cast<int>(err_code))
                            .arg(http_name));

                    // content type has to be defined by the handler, also
                    // the output auto-generated
                    //f_snap->set_header(get_name(SNAP_NAME_CORE_CONTENT_TYPE_HEADER), "text/html; charset=utf8", HEADER_MODE_EVERYWHERE);
                    //f_snap->output_result(HEADER_MODE_ERROR, html.toUtf8());
                    handle_error->on_handle_error_by_mime_type(err_code, err_name, err_description, f_ipath.get_key());
                }
                catch(...)
                {
                    // ignore all errors because at this point we must die quickly.
                    SNAP_LOG_FATAL("path.cpp:on_error(): try/catch caught an exception");
                }

                // exit with an error
                exit(1);
                NOTREACHED();
            }
        }
        f_snap->die(err_code, err_name, err_description, err_details);
        NOTREACHED();
    }

    void on_redirect(
            /* message::set_error() */ QString const& err_name, QString const& err_description, QString const& err_details, bool err_security,
            /* snap_child::page_redirect() */ QString const& path, snap_child::http_code_t const http_code)
    {
        // TODO: remove this message dependency
        messages::messages::instance()->set_error(err_name, err_description, err_details, err_security);
        server_access::server_access *server_access_plugin(server_access::server_access::instance());
        if(server_access_plugin->is_ajax_request())
        {
            // Since the user sent an AJAX request, returning
            // a redirect won't work as expected... instead we
            // reply with a redirect in AJAX.
            //
            // TODO: The redirect requires the result of the AJAX
            //       request to be 'true'... verify that this is
            //       not in conflict with what we are trying to
            //       achieve here
            //
//std::cerr << "***\n*** PATH Permission denied, but we can ask user for credentials with a redirect...\n***\n";
            server_access_plugin->create_ajax_result(f_ipath, true);
            server_access_plugin->ajax_redirect(QString("/%1").arg(path), "_top");
            server_access_plugin->ajax_output();
            f_snap->output_result(snap_child::HEADER_MODE_REDIRECT, f_snap->get_output());
            f_snap->exit(0);
        }
        else
        {
            f_snap->page_redirect(path, http_code, err_description, err_details);
        }
        NOTREACHED();
    }

private:
    zpsnap_child_t          f_snap;
    content::path_info_t&   f_ipath;
    plugins::plugin_zptr_t  f_plugin;
};


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
    if(f_plugin != nullptr)
    {
        // two different plugins are fighting for the same path
        // we'll have to enhance our error to give the user a way to choose
        // the plugin one wants to use for this request...
        content::content::instance()->get_snap()->die(snap_child::HTTP_CODE_MULTIPLE_CHOICE,
                "Multiple Choices",
                "This page references multiple plugins and the server does not currently have means of choosing one over the other.",
                QString("User tried to access dynamic page but more than one plugin says it owns the resource, primary is \"%1\", second request by \"%2\"")
                        .arg(f_plugin->get_plugin_name()).arg(p->get_plugin_name()));
        NOTREACHED();
    }

    f_plugin = p;
}


/** \brief Tell the system that a fallback exists for this path.
 *
 * Some plugins may understand a path even if not an exact match as
 * otherwise expected by the system.
 *
 * For example, the attachment plugin understands all of the following
 * even though the only file that really exists in the database is
 * "jquery.js":
 *
 * \li jquery.js.gz
 * \li jquery.min.js
 * \li jquery.min.js.gz
 * \li jquery-1.2.3.js
 * \li jquery-1.2.3.js.gz
 * \li jquery-1.2.3.min.js
 * \li jquery-1.2.3.min.js.gz
 *
 * File types of filenames that we support in the core:
 *
 * \li Compressions: .gz, .bz2, .xz, ...
 * \li Minified: .min.js, .min.css
 * \li Resized: -32x32.png, -64x64.jpg, ...
 * \li Cropped: -32x32+64+64.png
 * \li Black and White: -bw.png, -bw.jpg, ...
 * \li Converted: file is .pdf, user gets a .png ...
 * \li Book: .pdf on the root page of a book tree
 *
 * \param[in] plugin  The plugin that understands the path.
 */
void dynamic_plugin_t::set_plugin_if_renamed(plugins::plugin *p, QString const& cpath)
{
    if(f_plugin_if_renamed)
    {
        // in this case we really cannot handle the path properly...
        // I'm not too sure how we can resolve the problem because we
        // cannot be sure in which order the plugins will be executing
        // the tests...
        content::content::instance()->get_snap()->die(snap_child::HTTP_CODE_MULTIPLE_CHOICE,
                    "Multiple Choices",
                    "This page references multiple plugins if the path is renamed and the server does not currently have means of choosing one over the other.",
                    QString("User tried to access dynamic page, but more than one plugin says it can handle it: primary \"%2\", second request \"%3\".")
                            .arg(f_plugin_if_renamed->get_plugin_name()).arg(p->get_plugin_name()));
        NOTREACHED();
    }

    f_plugin_if_renamed = p;
    f_cpath_renamed = cpath;
}


/** \brief Initialize the path plugin.
 *
 * This function initializes the path plugin.
 */
path::path()
    //: f_snap(nullptr) -- auto-init
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
    plugins::plugin *owner_plugin(nullptr);

    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    if(content_table->exists(ipath.get_key())
    && content_table->row(ipath.get_key())->exists(content::get_name(content::SNAP_NAME_CONTENT_PRIMARY_OWNER)))
    {
        // verify that the status is good for displaying this page
        content::path_info_t::status_t status(ipath.get_status());
        switch(status.get_state())
        {
        case content::path_info_t::status_t::state_t::UNKNOWN_STATE:
            // TBD: should we throw instead when unknown (because get_state()
            //      is not expected to ever return that value)
        case content::path_info_t::status_t::state_t::CREATE:
        case content::path_info_t::status_t::state_t::DELETED:
            // TODO: for administrators who can undelete pages, the DELETED
            //       will need special handling at some point
            // TBD: maybe we should use 403 instead of 404?
            err_callback.on_error(snap_child::HTTP_CODE_NOT_FOUND,
                        "Unknow Page Status",
                        "An internal error occured and this page cannot properly be displayed at this time.",
                        QString("User tried to access page \"%1\" but its status state is %2.")
                                .arg(ipath.get_key())
                                .arg(static_cast<int>(status.get_state())),
                        false);
            return nullptr;

        case content::path_info_t::status_t::state_t::NORMAL:
        case content::path_info_t::status_t::state_t::HIDDEN:   // TBD -- probably requires special handling to know whether we can show those pages
        case content::path_info_t::status_t::state_t::MOVED:    // MOVED pages will redirect a little later (if allowed)
            break;

        }

        // get the modified date so we can setup the Last-Modified HTTP header field
        // it is also another way to determine that a path is valid
        QtCassandra::QCassandraValue const value(content_table->row(ipath.get_key())->cell(content::get_name(content::SNAP_NAME_CONTENT_CREATED))->value());
        QString const owner(content_table->row(ipath.get_key())->cell(content::get_name(content::SNAP_NAME_CONTENT_PRIMARY_OWNER))->value().stringValue());
        if(value.nullValue() || owner.isEmpty())
        {
            err_callback.on_error(snap_child::HTTP_CODE_NOT_FOUND,
                        "Invalid Page",
                        "An internal error occured and this page cannot properly be displayed at this time.",
                        QString("User tried to access page \"%1\" but it does not look valid (null value? %2, empty owner? %3)")
                                .arg(ipath.get_key())
                                .arg(static_cast<int>(value.nullValue()))
                                .arg(static_cast<int>(owner.isEmpty())),
                        false);
            return nullptr;
        }
        // TODO: this is not correct anymore! (we're getting the creation date, not last mod.)
        f_last_modified = value.int64Value();

        // get the primary owner (plugin name) and retrieve the plugin pointer
//std::cerr << "Execute [" << ipath.get_key() << "] with plugin [" << owner << "]\n";
        owner_plugin = plugins::get_plugin(owner);
        if(owner_plugin == nullptr)
        {
            // if the plugin cannot be found then either it was mispelled
            // or the plugin is not currently installed...
            f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                        "Plugin Missing",
                        "This page is not currently available as its plugin is not currently installed.",
                        QString("User tried to access page \"%1\" but its plugin (%2) does not exist (not installed? mispelled?)")
                                .arg(ipath.get_cpath())
                                .arg(owner));
            NOTREACHED();
        }
    }
    else
    {
        // this key does not exist as is in the database, but...
        // it may be a dynamically defined path, check for a
        // plugin that would have defined such a path
//std::cerr << "Testing for page dynamically [" << ipath.get_cpath() << "]\n";
        dynamic_plugin_t dp;
        can_handle_dynamic_path(ipath, dp);
        owner_plugin = dp.get_plugin();

        if(owner_plugin == nullptr)
        {
            // a plugin (such as the attachment, images, or search plugins)
            // may take care of this path
            owner_plugin = dp.get_plugin_if_renamed();
            if(owner_plugin != nullptr)
            {
                ipath.set_parameter("renamed_path", dp.get_renamed_path());
            }
        }
    }

    if(owner_plugin != nullptr)
    {
        // got a valid plugin, verify that the user has permission
        path_error_callback *pec(dynamic_cast<path_error_callback *>(&err_callback));
        if(pec)
        {
            pec->set_plugin(owner_plugin);
        }
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
    QString action(ipath.get_parameter("action"));
    if(action.isEmpty())
    {
        QString const qs_action(f_snap->get_server_parameter("qs_action"));
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

        // save the action in the path
        ipath.set_parameter("action", action);
    }

    SNAP_LOG_TRACE() << "verify_permissions(): ipath=" << ipath.get_key() << ", action=" << action;

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
    static_cast<void>(user_path);
    static_cast<void>(ipath);
    static_cast<void>(action);
    static_cast<void>(login_status);

    return result.allowed();
}


/** \fn void path::validate_action(content::path_info_t& ipath, QString const& action, permission_error_callback& err_callback)
 * \brief Validate the user action.
 *
 * This function validates the user action. If invalid or if that means
 * the user does not have enough rights to access the specified path,
 * then the event calls die() at some point and returns.
 *
 * \param[in,out] ipath  The path being validated.
 * \param[in] action  The action being performed against \p path.
 * \param[in,out] err_callback  Call functions on errors.
 */



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
    content::path_info_t ipath;
    ipath.set_path(uri_path);
    ipath.set_main_page(true);
#ifdef DEBUG
SNAP_LOG_TRACE() << "path::on_execute(\"" << uri_path << "\") -> [" << ipath.get_cpath() << "] [" << ipath.get_branch() << "] [" << ipath.get_revision() << "]";
#endif

    // allow modules to redirect now, it has to be really early, note
    // that it will be BEFORE the path module verifies the permissions
    check_for_redirect(ipath);

    path_error_callback main_page_error_callback(f_snap, ipath);

    f_last_modified = 0;
    plugins::plugin *path_plugin(get_plugin(ipath, main_page_error_callback));

    preprocess_path(ipath, path_plugin);

    // save the main page action found in the URI so that way any plugin
    // can access that information at any point, not just the
    // verify_rights() function
    f_snap->set_action(ipath.get_parameter("action"));

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
    // always work, however path_plugin may be nullptr
    path_execute *pe(dynamic_cast<path_execute *>(path_plugin));
    if(pe == nullptr)
    {
        // not found, give a chance to some plugins to do something with the
        // current data (i.e. auto-search, internally redirect to a nice
        // Page Not Found page, etc.)
        page_not_found(ipath);
        if(f_snap->empty_output())
        {
            // no page_not_found() plugin support...
            if(path_plugin != nullptr)
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
        //
        // TBD: Could we not also allow a post in case we did not find
        //      a plugin to handle the page? (i.e. when pe is nullprt)
        f_snap->process_post();

        // if the buffer is still empty, the post process did not generate
        // an AJAX response, so go on by executing the page
        if(f_snap->empty_output())
        {
            if(!pe->on_path_execute(ipath))
            {
                // TODO (TBD):
                // page_not_found() not called here because the page exists
                // it is just not available right now and thus we
                // may not want to replace it with something else?
                f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                        "Page Not Present",
                        "Somehow this page is not currently available.",
                        QString("User tried to access page \"%1\" but the page's plugin (%2) refused it.")
                                .arg(ipath.get_cpath()).arg(path_plugin->get_plugin_name()));
                NOTREACHED();
            }
        }
    }
}


/** \brief Allow modules to redirect before we do anything else.
 *
 * This signal is used to allow plugins to redirect before we hit anything
 * else. Note that this happens BEFORE we check for permissions.
 *
 * \param[in,out] ipath  The path the client is trying to access.
 */
bool path::check_for_redirect_impl(content::path_info_t& ipath)
{
    // check whether the page mode is currently MOVED
    content::path_info_t::status_t status(ipath.get_status());
    if(status.get_state() == content::path_info_t::status_t::state_t::MOVED)
    {
        // the page was moved, get the new location and auto-redirect
        // user
        //
        // TODO: avoid auto-redirect if user is an administrator so that
        //       way the admin can reuse the page in some way
        //
        // TBD: what code is the most appropriate here? (we are using 301
        //      for now, but 303 or 307 could be better?)
        //
        links::link_info info(content::get_name(content::SNAP_NAME_CONTENT_ORIGINAL_PAGE), true, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info clone_info;
        if(link_ctxt->next_link(clone_info))
        {
            content::path_info_t imoved;
            imoved.set_path(clone_info.key());
            if(imoved.get_status().get_state() == content::path_info_t::status_t::state_t::NORMAL)
            {
                // we have a valid destination, go there
                f_snap->page_redirect(imoved.get_key(),
                            snap_child::HTTP_CODE_MOVED_PERMANENTLY,
                            "Redirect to the new version of this page.",
                            QString("This page (%1) was moved so we are redirecting this user to the new location (%2).")
                                    .arg(ipath.get_key()).arg(imoved.get_key()));
                NOTREACHED();
            }
            // else -- if the destination status is MOVED, we can loop over it!
        }

        // we cannot redirect to the copy, so just say not found
        f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                    "Invalid Page",
                    "This page is not currently valid. It can not be viewed.",
                    QString("User tried to access page \"%1\" but it is marked as MOVED and the destination is either unspecified or not NORMAL.")
                            .arg(ipath.get_key()));
        NOTREACHED();
    }

    return true;
}


/** \fn void path::preprocess_path(content::path_info_t& ipath, plugins::plugin *owner_plugin)
 * \brief Allow other modules to do some pre-processing.
 *
 * This signal is sent just before we run the actual execute() function
 * of the plugins. This can be useful to make some early changes to
 * the database so the page being displayed uses the correct data.
 *
 * \param[in,out] ipath  The path of the page being processed.
 * \param[in] owner_plugin  The plugin that owns this page (may be nullptr).
 */


/** \fn void path::can_handle_dynamic_path(content::path_info_t& ipath, dynamic_plugin_t& plugin_info)
 * \brief Default implementation of the dynamic path handler.
 *
 * This function doesn't do anything as the path plugin does not itself
 * offer another way to handle a path than checking the database (which
 * has priority and thus this function never gets called if that happens.)
 *
 * \param[in] ipath  The canonicalized path to be checked.
 * \param[in.out] plugin_info  Will hold the plugin that can handle this
 *                             dynamic path.
 */


/** \fn void path::page_not_found(content::path_info_t& ipath)
 * \brief Default implementation of the page not found signal.
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



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
