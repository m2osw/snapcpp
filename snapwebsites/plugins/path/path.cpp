// Snap Websites Server -- path handling
// Copyright (C) 2011-2012  Made to Order Software Corp.
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
const char *get_name(name_t name)
{
    switch(name) {
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
    //, f_path_plugin() -- auto-init
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


/** \brief Add a path to the database.
 *
 * The current scheme works by adding your plugin paths to the database.
 * These can include JSON paths, paths to lists of data, paths to pages
 * of content to display to end users, etc.
 *
 * This functions takes 3 parameters that are all important:
 *
 * 1) Primary Owner
 *
 * This is the name of the plug that manages the given path. When executing
 * a path the system will (a) gather information about that path; and (b)
 * retrieve the owner and send a signal about executing that path.
 *
 * 2) Path
 *
 * The actual path. This is, for example, "robots.txt". This represents
 * the path to the page read by robots to know what pages should be
 * indexed and which ones should never be indexed.
 *
 * The index page is represened by "". It is handled by the "content" plugin.
 * Since the content plugin is mandatory, you cannot redefine the index
 * somewhere else.
 *
 * 3) Timestamp
 *
 * This is the timestamp that is to be used to register the path information.
 * The timestamp is in microseconds (usec). It is particularly important when
 * initializing paths at the time an update runs on your website. When creating
 * a new page of content (i.e. the user is posting data to his website) then
 * the current time is used.
 *
 * \param[in] primary_owner  The name of the plugin handling these requests.
 * \param[in] path  The path without protocol and domain information.
 * \param[in] timestamp  The time and date when the path is being created.
 */
//void path::add_path(const QString& primary_owner,
//                    const QString& path,
//                    int64_t timestamp)
//{
//    QString cpath(path);
//    snap_child::canonicalize_path(cpath);
//    const QString key(f_snap->get_site_key() + cpath);
//    QtCassandra::QCassandraValue value(primary_owner);
//    value.setTimestamp(timestamp);
//    content::content::instance()->get_content_table()->row(key)->cell(QString("path::primary_owner"))->setValue(value);
//}


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
void path::on_execute(const QString& uri_path)
{
    // get the name of the plugin that owns this URL 
    QString cpath(uri_path);
    snap_child::canonicalize_path(cpath);
    QString const key(f_snap->get_site_key_with_slash() + cpath);
    QString owner;
    dynamic_path_plugin_t path_plugin;
    QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
    bool const page_exists(content_table->exists(key));
    if(page_exists)
    {
        // this should work so we go ahead and set the Last-Modified field in the header
        QtCassandra::QCassandraValue value(content_table->row(key)->cell(QString(content::get_name(content::SNAP_NAME_CONTENT_MODIFIED)))->value());
        owner = content_table->row(key)->cell(QString(get_name(SNAP_NAME_PATH_PRIMARY_OWNER)))->value().stringValue();
        if(value.nullValue() || owner.isEmpty())
        {
            f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                        "Invalid Page",
                        "An internal error occured and this page cannot properly be displayed at this time.",
                        QString("User tried to access page \"%1\" but it does not look valid (value null? %2, owner null? %3)")
                                .arg(key).arg(static_cast<int>(value.nullValue())).arg(static_cast<int>(owner.isEmpty())));
            NOTREACHED();
        }
        // ddd, dd MMM yyyy hh:mm:ss +0000
        uint64_t const last_modified(value.int64Value());
        f_snap->set_header("Last-Modified", f_snap->date_to_string(last_modified, snap_child::DATE_FORMAT_HTTP));

        // get the primary owner (plugin name) and retrieve the plugin pointer
//std::cerr << "Execute [" << key.toUtf8().data() << "] with plugin [" << owner.toUtf8().data() << "]\n";
        path_plugin = plugins::get_plugin(owner);
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
    // if a plugin pointer was defined we expect that the dynamic_cast<> will
    // always work, however path_plugin may be NULL
    path_execute *pe(dynamic_cast<path_execute *>(path_plugin.get()));
    if(pe == NULL)
    {
        // not found, give a chance to some plugins to do something with the
        // current data (i.e. auto-search, internally redirect to a nice
        // Page Not Found page, etc.)
        page_not_found(this, cpath);
        if(f_snap->empty_output())
        {
            // no page_not_found() plugin support...
            if(page_exists)
            {
                f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                            "Plugin Missing",
                            "This page is not currently available as its plugin is not currently installed.",
                            "User tried to access page \"" + cpath + "\" but its plugin (" + owner + ") refused it");
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
        // found it, execute the path for real

        // get the action, if no action is defined, then use the default
        // which  is "view" unless we are POSTing
        f_snap->verify_permissions();

        // if the user POSTed something, manage that content first, the
        // effect is often to redirect the user in which case we want to
        // emit an HTTP Location and return; also, with AJAX we may end
        // up stopping early (i.e. not generate a full page but instead
        // return the "form results".)
        f_snap->process_post();

        if(!pe->on_path_execute(cpath))
        {
            // TODO (TBD):
            // page_not_found() not called here because the page exists
            // it's just not available right now and thus we
            // may not want to replace it with something else?
            f_snap->die(snap_child::HTTP_CODE_NOT_FOUND,
                    "Page Not Present",
                    "Somehow this page is not currently available.",
                    "User tried to access page \"" + cpath + "\" but its plugin (" + owner + ") refused it");
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
                            + f_primary_owner + "\", second request by \"" + p->get_plugin_name() + " \"["+g_undefined+"]");
        NOTREACHED();
    }
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
