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
#include "not_reached.h"
#include "../content/content.h"
#include "../messages/messages.h"
#include <iostream>


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
char const *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_PATH_PRIMARY_OWNER:
        return "path::primary_owner";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_PATH_...");

    }
    NOTREACHED();
}

namespace
{
char const * const g_undefined = "undefined";
}

/** \brief Initialize the path plugin.
 *
 * This function initializes the path plugin.
 */
path::path()
    //: f_snap(NULL) -- auto-init
    : f_primary_owner(g_undefined)
    //, f_path_plugin() -- not initialized
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

    SNAP_LISTEN0(path, "server", server, init);
    SNAP_LISTEN(path, "server", server, execute, _1);
}


/** \brief Initialize the path plugin.
 *
 * At this point this function does nothing.
 */
void path::on_init()
{
}




plugins::plugin *path::get_plugin(QString const& uri_path, permission_error_callback& err_callback)
{
    // get the name of the plugin that owns this URL 
    QString cpath(uri_path);
    snap_child::canonicalize_path(cpath);
    QString const key(f_snap->get_site_key_with_slash() + cpath);
    QString owner;
    dynamic_path_plugin_t path_plugin;
    // TODO: remove direct dependency on the content plugin
    QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
    bool const page_exists(content_table->exists(key)
                        && content_table->row(key)->exists(content::get_name(content::SNAP_NAME_CONTENT_MODIFIED)));
    if(page_exists)
    {
        // this should work so we go ahead and set the Last-Modified field in the header
        QtCassandra::QCassandraValue value(content_table->row(key)->cell(QString(content::get_name(content::SNAP_NAME_CONTENT_MODIFIED)))->value());
        owner = content_table->row(key)->cell(QString(get_name(SNAP_NAME_PATH_PRIMARY_OWNER)))->value().stringValue();
        if(value.nullValue() || owner.isEmpty())
        {
            err_callback.on_error(snap_child::HTTP_CODE_NOT_FOUND,
                        "Invalid Page",
                        "An internal error occured and this page cannot properly be displayed at this time.",
                        QString("User tried to access page \"%1\" but it does not look valid (null value? %2, empty owner? %3)")
                                .arg(key).arg(static_cast<int>(value.nullValue())).arg(static_cast<int>(owner.isEmpty())));
            return NULL;
        }
        f_last_modified = value.int64Value();

        // get the primary owner (plugin name) and retrieve the plugin pointer
//std::cerr << "Execute [" << key.toUtf8().data() << "] with plugin [" << owner.toUtf8().data() << "]\n";
        path_plugin = plugins::get_plugin(owner);
        if(!path_plugin)
        {
            // if the plugin cannot be found then either it was mispelled
            // or the plugin is not currently installed...
            f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                        "Plugin Missing",
                        "This page is not currently available as its plugin is not currently installed.",
                        "User tried to access page \"" + cpath + "\" but its plugin (" + owner + ") does not exist (not installed? mispelled?)");
            NOTREACHED();
        }
    }
    else
    {
        // this key doesn't exist as is in the database, but...
        // it may be a dynamically defined path, check for a
        // plugin that would have defined such a path
//printf("Testing for page dynamically [%s]\n", cpath.toUtf8().data());
        f_primary_owner = g_undefined;
        f_path_plugin.reset();
        can_handle_dynamic_path(this, cpath);
        owner = f_primary_owner;
        path_plugin = f_path_plugin;
        f_primary_owner = g_undefined; // reset to a value considered invalid
    }

    if(path_plugin)
    {
        // got a valid plugin, verify that the user has permission
        f_snap->verify_permissions(cpath, err_callback);
    }

    return path_plugin.get();
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

    dynamic_path_plugin_t path_plugin(get_plugin(uri_path, main_page_error_callback));

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
    path_execute *pe(dynamic_cast<path_execute *>(path_plugin.get()));
    if(pe == NULL)
    {
        // not found, give a chance to some plugins to do something with the
        // current data (i.e. auto-search, internally redirect to a nice
        // Page Not Found page, etc.)
        QString cpath(uri_path);
        snap_child::canonicalize_path(cpath);
        page_not_found(this, cpath);
        if(f_snap->empty_output())
        {
            // no page_not_found() plugin support...
            if(path_plugin)
            {
                // if the page exists then
                QString const owner(path_plugin->get_plugin_name());
                f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                            "Plugin Missing",
                            "This page is not currently available as its plugin is not currently installed.",
                            "User tried to access page \"" + cpath + "\" but its plugin (" + owner + ") does not yet implement the path_execute");
            }
            else
            {
                f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                            "Page Not Found",
                            "This page does not exist on this website.",
                            "User tried to access page \"" + cpath + "\" and no dynamic path handling happened");
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

        QString cpath(uri_path);
        snap_child::canonicalize_path(cpath);
        if(!pe->on_path_execute(cpath))
        {
            // TODO (TBD):
            // page_not_found() not called here because the page exists
            // it's just not available right now and thus we
            // may not want to replace it with something else?
            f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                    "Page Not Present",
                    "Somehow this page is not currently available.",
                    "User tried to access page \"" + cpath + "\" but its plugin (" + path_plugin->get_plugin_name() + ") refused it");
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
 * \param[in] path_plugin  The path plugin.
 * \param[in] cpath  The canonicalized path to be checked
 */
bool path::can_handle_dynamic_path_impl(path * /*path_plugin*/, const QString& /*cpath*/)
{
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
 * \param[in] path_plugin  The path plugin.
 * \param[in] cpath  The canonicalized path to be checked
 */
bool path::page_not_found_impl(path * /*path_plugin*/, const QString& /*cpath*/)
{
    return true;
}


/** \brief Called by plugins that can handle dynamic paths.
 *
 * Some plugins handle a very large number of paths in a fully
 * dynamic manner, which means that they can generate the data
 * for any one of those paths in a way that is extremely fast
 * without the need of creating millions of entries in the
 * database.
 *
 * These plugins are given a chance to handle a path whenever
 * the path plugin calls the can_handle_dynamic_path() signal.
 * At that point, a plugin can respond by calling this function
 * with itself.
 *
 * For example, a plugin that displays a date in different formats
 * could be programmed to understand the special path
 *
 * /formatted-date/YYYYMMDD/FMT
 *
 * which means format the date YYYY-MM-DD using the format FMT.
 *
 * \param[in] p  The plugin that can handle this dynamic path.
 */
void path::handle_dynamic_path(plugins::plugin *p)
{
//printf("handle_dynamic_path(%s)\n", plugin_name.toUtf8().data());
    if(f_primary_owner == g_undefined)
    {
        f_primary_owner = p->get_plugin_name();
        f_path_plugin = p;
    }
    else
    {
        // two different plugins are fighting for the same path
        // we'll have to enhance our error to give the user a way to choose
        // the plugin one wants to use for this request...
        f_snap->die(snap_child::HTTP_CODE_MULTIPLE_CHOICE,
                    "Multiple Choices",
                    "This page references multiple plugins and the server does not currently have means of choosing one over the other.",
                    "User tried to access dynamic page but more than one plugin says it owns the resource, primary is \""
                            + f_primary_owner + "\", second request by \"" + p->get_plugin_name() + " \"[" + g_undefined + "]");
        NOTREACHED();
    }
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
