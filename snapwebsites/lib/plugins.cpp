// Snap Websites Server -- plugin loader
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

#include "plugins.h"
#include "log.h"

#include <qstring_stream.h>

#include <dlfcn.h>
#include <glob.h>
#include <sys/stat.h>

#include <QDir>
#include <QMap>
#include <QFileInfo>

#include "poison.h"


namespace snap
{
namespace plugins
{

plugin_map_t        g_plugins;
plugin_vector_t     g_ordered_plugins;
QString             g_next_register_name;
QString             g_next_register_filename;


/** \brief Load a complete list of available plugins.
 *
 * This is used in the administrator screen to offer users a complete list of
 * plugins that can be installed.
 *
 * \param[in] plugin_path  The path to all the Snap plugins.
 *
 * \return A list of plugin names.
 */
snap_string_list list_all(const QString& plugin_path)
{
    // note that we expect the plugin directory to be clean
    // (we may later check the validity of each directory to make 100% sure
    // that it includes a corresponding .so file)
    QDir dir(plugin_path);
    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}


/** \brief Load all the plugins.
 *
 * Someone who wants to remove a plugin simply deletes it or its
 * softlink at least.
 *
 * \warning
 * This function CANNOT use glob() to read all the plugins in a directory.
 * At this point we assume that each website will use more or less of
 * the installed plugins and thus loading them all is not the right way of
 * handling the loading. Thus we now get a \p list_of_plugins parameter
 * with the name of the plugins we want to dlopen().
 *
 * \todo
 * Look into the shared pointers and unloading plugins, if that ever
 * happens (I don't think it does.)
 *
 * \param[in] plugin_paths  The colon (:) separated list of paths to
 *                          directories with plugins.
 * \param[in] server  A pointer to the server to register it as a plugin.
 * \param[in] list_of_plugins  The list of plugins to load.
 *
 * \return true if all the modules were loaded.
 */
bool load(QString const & plugin_paths, snap_child * snap, plugin_ptr_t server, snap_string_list const & list_of_plugins)
{
    g_plugins.insert("server", server.get());

    snap_string_list const paths(plugin_paths.split(':'));

    bool good(true);
    for(snap_string_list::const_iterator it(list_of_plugins.begin());
                                    it != list_of_plugins.end();
                                    ++it)
    {
        QString const name(*it);

        // the Snap server is already added to the list under that name!
        //
        if(name == "server")
        {
            SNAP_LOG_ERROR("error: a plugin cannot be called \"server\".");
            good = false;
            continue;
        }

        // in case we get multiple calls to this function we must make sure that
        // all plugins have a distinct name (i.e. a plugin factory could call
        // this function to load sub-plugins!)
        //
        if(exists(name))
        {
            SNAP_LOG_ERROR("error: two plugins cannot be named the same, found \"")(name)("\" twice.");
            good = false;
            continue;
        }

        // make sure the name is one we consider valid; we may end up
        // using plugin names in scripts and thus want to only support
        // a small set of characters; any other name is refused by
        // the verify_plugin_name() function (which prints an error
        // message already so no need to another one here)
        //
        if(!verify_plugin_name(name))
        {
            good = false;
            continue;
        }

        // check that the file exists, if not we generate an error
        //
        QString const filename(find_plugin_filename(paths, name));
        if(filename.isEmpty())
        {
            SNAP_LOG_ERROR("plugin named \"")(name)("\" not found in the plugin directory. (paths: ")(plugin_paths)(")");
            good = false;
            continue;
        }

        // TBD: Use RTLD_NOW instead of RTLD_LAZY in DEBUG mode
        //      so we discover missing symbols would be nice, only
        //      that would require loading in the correct order...
        //      (see dlopen() call below)
        //

        // load the plugin; the plugin will register itself
        //
        // use some really ugly globals because dlopen() does not give us
        // a way to pass parameters to the plugin factory constructor
        //
        g_next_register_name = name;
        g_next_register_filename = filename;
        void const * const h(dlopen(filename.toUtf8().data(), RTLD_LAZY | RTLD_GLOBAL));
        if(h == nullptr)
        {
            int const e(errno);
            SNAP_LOG_ERROR("error: cannot load plugin file \"")(filename)("\" (errno: ")(e)(", ")(dlerror())(")");
            good = false;
            continue;
        }
        g_next_register_name.clear();
        g_next_register_filename.clear();
//SNAP_LOG_ERROR("note: registering plugin: \"")(name)("\"");
    }

    // set the g_ordered_plugins with the default order as alphabetical,
    // although we check dependencies to properly reorder as expected
    // by what each plugin tells us what its dependencies are
    //
    for(auto p : g_plugins)
    {
        QString const column_name(QString("|%1|").arg(p->get_plugin_name()));
        for(plugin_vector_t::iterator sp(g_ordered_plugins.begin());
                                      sp != g_ordered_plugins.end();
                                      ++sp)
        {
            if((*sp)->dependencies().indexOf(column_name) >= 0)
            {
                g_ordered_plugins.insert(sp, p);
                goto inserted;
            }
        }
        // if not before another plugin, insert at the end by default
        g_ordered_plugins.push_back(p);
inserted:;
    }

    // bootstrap() functions have to be called in order to get all the
    // signals registered in order! (YES!!! This one for() loop makes
    // all the signals work as expected by making sure they are in a
    // very specific order)
    //
    for(auto p : g_ordered_plugins)
    {
        p->bootstrap(snap);
    }

    return good;
}


/** \brief Try to find the plugin using the list of paths.
 *
 * This function searches for a plugin in each one of the specified
 * paths and as:
 *
 * \code
 *    <path>/<name>.so
 *    <path>/lib<name>.so
 *    <path>/<name>/<name>.so
 *    <path>/<name>/lib<name>.so
 * \endcode
 *
 * \todo
 * We may change the naming convention to make use of the ${PROJECT_NAME}
 * in the CMakeLists.txt files. In that case we'd end up with names
 * that include the work plugin as in:
 *
 * \code
 *    <path>/libplugin_<name>.so
 * \endcode
 *
 * \param[in] plugin_paths  The list of paths to check with.
 * \param[in] name  The name of the plugin being searched.
 *
 * \return The full path and filename of the plugin or empty if not found.
 */
QString find_plugin_filename(snap_string_list const & plugin_paths, QString const & name)
{
    int const max_paths(plugin_paths.size());
    for(int i(0); i < max_paths; ++i)
    {
        QString const path(plugin_paths[i]);
        QString filename(QString("%1/%2.so").arg(path).arg(name));
        if(QFile::exists(filename))
        {
            return filename;
        }

        //
        // If not found, see if it has a "lib" at the front of the file:
        //
        filename = QString("%1/lib%2.so").arg(path).arg(name);
        if(QFile::exists(filename))
        {
            return filename;
        }

        //
        // If not found, see if it lives under a named folder:
        //
        filename = QString("%1/%2/%2.so").arg(path).arg(name);
        if(QFile::exists(filename))
        {
            return filename;
        }

        //
        // Last test: check plugin names starting with "lib" in named folder:
        //
        filename = QString("%1/%2/lib%2.so").arg(path).arg(name);
        if(QFile::exists(filename))
        {
            return filename;
        }
    }

    return "";
}


/** \brief Verify that a name is a valid plugin name.
 *
 * This function checks a string to know whether it is a valid plugin name.
 *
 * A valid plugin name is a string of letters (A-Z or a-z), digits (0-9),
 * and the underscore (_), dash (-), and period (.). Although the name
 * cannot start or end with a dash or a period.
 *
 * \todo
 * At this time this function prints errors out in stderr. This may
 * change later and errors will be sent to the logger. However, we
 * need to verify that the logger is ready when this function gets
 * called.
 *
 * \param[in] name  The name to verify.
 *
 * \return true if the name is considered valid.
 */
bool verify_plugin_name(QString const & name)
{
    if(name.isEmpty())
    {
        SNAP_LOG_ERROR() << "error: an empty plugin name is not valid.";
        return false;
    }
    for(QString::const_iterator p(name.begin()); p != name.end(); ++p)
    {
        if((*p < 'a' || *p > 'z')
        && (*p < 'A' || *p > 'Z')
        && (*p < '0' || *p > '9')
        && *p != '_' && *p != '-' && *p != '.')
        {
            SNAP_LOG_ERROR("error: plugin name \"")(name)("\" includes forbidden characters.");
            return false;
        }
    }
    // Note: we know that name is not empty
    QChar const first(name[0]);
    if(first == '.'
    || first == '-'
    || (first >= '0' && first <= '9'))
    {
        SNAP_LOG_ERROR("error: plugin name \"")(name)("\" cannot start with a digit (0-9), a period (.), or dash (-).");
        return false;
    }
    // Note: we know that name is not empty
    QChar const last(name[name.length() - 1]);
    if(last == '.' || last == '-')
    {
        SNAP_LOG_ERROR("error: plugin name \"")(name)("\" cannot end with a period (.) or dash (-).");
        return false;
    }

    return true;
}


/** \brief Check whether a plugin was loaded.
 *
 * This function searches the list of loaded plugins and returns true if the
 * plugin with the speficied \p name exists.
 *
 * \param[in] name  The name of the plugin to check for.
 *
 * \return true if the plugin is loaded, false otherwise.
 */
bool exists(QString const & name)
{
    return g_plugins.contains(name);
}


/** \brief Register a plugin in the list of plugins.
 *
 * This function is called by plugin factories to register new plugins.
 * Do not attempt to call this function directly or you'll get an
 * exception.
 *
 * \exception plugin_exception
 * If the name is empty, the name does not correspond to the plugin
 * being loaded, or the plugin is being loaded for the second time,
 * then this exception is raised.
 *
 * \param[in] name  The name of the plugin being added.
 * \param[in] p  A pointer to the plugin being added.
 */
void register_plugin(QString const & name, plugin * p)
{
    if(name.isEmpty())
    {
        throw plugin_exception("plugin name missing when registering... expected \"" + name + "\".");
    }
    if(name != g_next_register_name)
    {
        throw plugin_exception("it is not possible to register a plugin (" + name + ") other than the one being loaded (" + g_next_register_name + ").");
    }
#ifdef DEBUG
    // this is not possible if you use the macro, but in case you create
    // your own factory instance by hand, it is a requirement too
    //
    if(name != p->get_plugin_name())
    {
        throw plugin_exception("somehow your plugin factory name is \"" + p->get_plugin_name() + "\" when we were expecting \"" + name + "\".");
    }
#endif
    if(exists(name))
    {
        // this should not happen except if the plugin factory was attempting
        // to register the same plugin many times in a row
        throw plugin_exception("it is not possible to register a plugin more than once (" + name + ").");
    }
    g_plugins.insert(name, p);
}


/** \brief Initialize a plugin.
 *
 * This function initializes the plugin with its filename.
 */
plugin::plugin()
    : f_name(g_next_register_name)
    , f_filename(g_next_register_filename)
    //, f_last_modification(0) -- auto-init
    //, f_version_major(0) -- auto-init
    //, f_version_minor(0) -- auto-init
{
}


/** \brief Define the version of the plugin.
 *
 * This function saves the version of the plugin in the object.
 * This way other systems can access the version.
 *
 * In general you never call that function. It is automatically
 * called by the SNAP_PLUGIN_START() macro. Note that the
 * function cannot be called more than once and the version
 * cannot be zero or negative.
 *
 * \param[in] version_major  The major version of the plugin.
 * \param[in] version_minor  The minor version of the plugin.
 */
void plugin::set_version(int version_major, int version_minor)
{
    if(f_version_major != 0
    || f_version_minor != 0)
    {
        // version was already defined; it cannot be set again
        throw plugin_exception(QString("version of plugin \"%1\" already defined.").arg(f_name));
    }

    if(version_major < 0
    || version_minor < 0
    || (version_major == 0 && version_minor == 0))
    {
        // version cannot be negative or null
        throw plugin_exception(QString("version of plugin \"%1\" cannot be zero or negative (%2.%3).")
                    .arg(f_name).arg(version_major).arg(version_minor));
    }

    f_version_major = version_major;
    f_version_minor = version_minor;
}


/** \brief Retrieve the major version of this plugin.
 *
 * This function returns the major version of this plugin. This is the
 * same version as defined in the plugin factory.
 *
 * \return The major version of the plugin.
 */
int plugin::get_major_version() const
{
    return f_version_major;
}


/** \brief Retrieve the minor version of this plugin.
 *
 * This function returns the minor version of this plugin. This is the
 * same version as defined in the plugin factory.
 *
 * \return The minor version of the plugin.
 */
int plugin::get_minor_version() const
{
    return f_version_minor;
}


/** \brief Retrieve the name of the plugin as defined on creation.
 *
 * This function returns the name of the plugin as defined on
 * creation of the plugin. It is not possible to modify the
 * name for safety.
 *
 * \return The name of the plugin as defined in your SNAP_PLUGIN_START() instantiation.
 */
QString plugin::get_plugin_name() const
{
    return f_name;
}


/** \brief Get the last modification date of the plugin.
 *
 * This function reads the modification date on the plugin file to determine
 * when it was last modified. This date can be used to determine whether the
 * plugin was modified since the last time we ran snap with this website.
 *
 * \return The last modification date and time in micro seconds.
 */
int64_t plugin::last_modification() const
{
    if(0 == f_last_modification)
    {
        // read the info only once
        struct stat s;
        if(stat(f_filename.toUtf8().data(), &s) == 0)
        {
            // should we make use of the usec mtime when available?
            f_last_modification = static_cast<int64_t>(s.st_mtime * 1000000LL);
        }
        // else TBD: should we throw here?
    }

    return f_last_modification;
}


/** \fn QString plugin::dependencies() const;
 * \brief Return a list of required dependencies.
 *
 * This function returns a list of dependencies, plugin names written
 * between pipes (|). All plugins have at least one dependency since
 * most plugins will not work without the base plugin (i.e. "|server|"
 * is the bottom most base you can use in your plugin).
 *
 * At this time, the "content" and "test_plugin_suite" plugins have no
 * dependencies.
 *
 * \note
 * Until "links" is merged with "content", it will depend on "content"
 * so that way "links" signals are registered after "content" signals.
 *
 * \return A list of plugin names representing all dependencies.
 */


/** \fn void plugin::bootstrap(snap_child * snap)
 * \brief Bootstrap this plugin.
 *
 * The bootstrap virtual function is used to initialize the plugins. At
 * this point all the plugins are loaded, however, they are not yet
 * ready to receive signals because all plugins are not yet connected.
 * The bootstrap() function is actually used to get all the listeners
 * registered.
 *
 * Note that the plugin implementation loads all the plugins, sorts them,
 * then calls their bootstrap() function. Afterward, the init() function
 * is likely called. The bootstrap() registers signals and the server
 * init() signal can be used to send signals since at that point all the
 * plugins are properly installed and have all of their signals registered.
 *
 * \note
 * This is a pure virtual which is not implemented here so that way your
 * plugin will crash the server if you did not implement this function
 * which is considered mandatory.
 *
 * \param[in,out] snap  The snap child process.
 */


/** \brief Run an update.
 *
 * This function is a stub that does nothing. It is here so any plug in that
 * does not need an update does not need to define an "empty" function.
 *
 * At this time the function ignores the \p last_updated parameter and 
 * it always returns the same date: Jan 1, 1990 at 00:00:00.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t plugin::do_update(int64_t last_updated)
{
    static_cast<void>(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();

    // in a complete implementation you have entries like this one:
    //SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Run a dynamic update.
 *
 * This function is called after the do_update(). This very version is
 * a stub that does nothing. It can be overloaded to create content in
 * the database after the content.xml was installed fully. In other
 * words, the dynamic update can make use of data that the content.xml
 * will be adding ahead of time.
 *
 * At this time the function ignores the \p last_updated parameter and 
 * it always returns the same date: Jan 1, 1990 at 00:00:00.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t plugin::do_dynamic_update(int64_t last_updated)
{
    static_cast<void>(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();

    // in a complete implementation you have entries like this one:
    //SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, dynamic_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Retrieve a pointer to an existing plugin.
 *
 * This function returns a pointer to a plugin that was previously loaded
 * with the load() function. If you only need to test whether a plugin
 * exists, then you should use exists() instead.
 *
 * \note
 * This function should not be called until your plugin bootstrap() function
 * is called. Before then, there are no guarantees that the plugin was already
 * loaded.
 *
 * \param[in] name  The name of the plugin to retrieve.
 *
 * \return This function returns a pointer to the named plugin, or nullptr
 * if the plugin was not loaded.
 *
 * \sa load()
 * \sa exists()
 */
plugin * get_plugin(QString const & name)
{
    return g_plugins.value(name, nullptr);
}


/** \brief Retrieve the list of plugins.
 *
 * This function returns the list of plugins that were loaded in this
 * session. Remember that plugins are loaded each time a client accesses
 * the server.
 *
 * This means that the list is complete only once you are in the snap
 * child and after the plugins were initialized. If you are in a plugin,
 * this means the list is not complete in the constructor. It is complete
 * anywhere else.
 *
 * \return List of plugins in a map indexed by plugin name
 *         (i.e. alphabetical order).
 */
plugin_map_t const & get_plugin_list()
{
    return g_plugins;
}


/** \brief Retrieve the list of plugins.
 *
 * This function returns the list of plugins that were sorted, once
 * loaded, using their dependencies. This is a vector since we need
 * to keep a very specific order of the plugins.
 *
 * This list is empty until all the plugins were loaded.
 *
 * This list should be empty when your plugins constructors are called.
 *
 * \return The sorted list of plugins in a vector.
 */
plugin_vector_t const & get_plugin_vector()
{
    return g_ordered_plugins;
}


} // namespace plugins
} // namespace snap
// vim: ts=4 sw=4 et
