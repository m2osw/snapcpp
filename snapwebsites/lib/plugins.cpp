// Snap Websites Server -- plugin loader
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

#include "plugins.h"
#include "log.h"

#include <iostream>
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

QMap<QString, plugin *> g_plugins;
QString g_next_register_name;
QString g_next_register_filename;


/** \brief Load a complete list of available plugins.
 *
 * This is used in the administrator screen to offer users a complete list of
 * plugins that can be installed.
 *
 * \param[in] plugin_path  The path to all the Snap plugins.
 *
 * \return A list of plugin names.
 */
QStringList list_all(const QString& plugin_path)
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
 * This function CANNOT use a glob to read all the plugins in a directory.
 * At this point we assume that each website will use more or less of
 * the installed plugins and thus loading them all is not the right way of
 * handling the loading. Thus we now get a \p list_of_plugins parameter
 * with the name of the plugins we want to dlopen().
 *
 * \param[in] plugin_paths  The colon (:) separated list of paths to
 *                          directories with plugins.
 * \param[in] server  A pointer to the server to register it as a plugin.
 * \param[in] list_of_plugins  The list of plugins to load.
 *
 * \return true if all the modules were loaded.
 */
bool load(const QString& plugin_paths, plugin_ptr_t server, const QStringList& list_of_plugins)
{
// Doug;
// "This defeats the purpose of a shared_ptr, but this is because all plugins are treated as barepointers. This needs to be fixed in another iteration..."
//
// Alexis:
// "I do not think we want to have shared pointer of plugins for several reasons:
//    1. plugins are allocated in a way that makes it complicated to handle
//       as a shared pointer (although we could make it more complicated just
//       for the fun of using shared pointers...)
//    2. but the most important reason: this is a server which means:
//       (a) we load, (b) we run in under 1 second, (c) we quit; the need
//       of shared pointer is very important in a process that never dies
//       for very long period of time; here it's plugins, clearly loaded once
//       for a very short amount of time (there is an exception with backends
//       though, but it still very safe without the shared pointer.)
//    3. when loading the .so file, the plugins are not automatically
//       complete which allows us to not have to load all plugins and yet
//       have plugin A make use of plugin B but only if available; shared
//       pointers prevent that scheme (and actually the load may fail)
//    4. I have no clue what would happen when unloading plugins, it is likely
//       to crash, especially if the plugins are not unloaded "in the right
//       order" (whatever that might be)
// I would strongly argue that your addition should be removed."
//
// Doug replies:
// "I would counter that a shared pointer is telling the developer that the main
// manager owns the pointer and manages its lifetime. That it gets handed out
// to plugins shouldn't matter. The weak_ptr is availble to deal with the
// scenario that the manager destroyed the pointer (the plugin can and should
// check the lock). And the act of handing it out is indeed sharing it.
//
// "However, thinking on it, perhaps instead of a shared_ptr, we really need an
// auto_ptr, or at least some way of making sure the object really does get
// destroyed properly (destructor of a main class). Presently destruction is
// ignored, which isn't a problem for now, but possibly could be if the server,
// on exit, must restore a shared resource that is used by another process
// (like releasing a semaphore, for example). What I dislike is that
// destruction of the server object is literally ignored. That could come back
// to bite us on the rear in the future."
// 
// "I want to leave this here for now until we address this issue. Hope you
// understand..."
//
//#pragma message("Please restore the plugin pointer to a non-shared pointer. (see detailed reason above this message)")
    g_plugins.insert("server", server.get());

    QStringList const paths(plugin_paths.split(':'));

    bool good = true;
    for(QStringList::const_iterator it(list_of_plugins.begin());
                                    it != list_of_plugins.end();
                                    ++it)
    {
        QString const name(*it);
        if(name == "server")
        {

            // the Snap server is already added to the list under that name!
            SNAP_LOG_ERROR() << "error: a plugin cannot be called \"server\".";
            good = false;
            continue;
        }
        // in case we get multiple calls to this function we must make sure that
        // all plugins have a distinct name (i.e. a plugin factory could call
        // this function to load sub-plugins!)
        if(exists(name))
        {
            SNAP_LOG_ERROR() << "error: two plugins cannot be named the same, found \""
                            << name.toUtf8().data() << "\" twice.";
            good = false;
            continue;
        }
        if(!verify_plugin_name(name))
        {
            good = false;
            continue;
        }
        // check that the file exists, if not we simply skip the
        // load step and generate an error
        //
        // First, check the proper installation place:
        //
        QString const filename(find_plugin_filename(paths, name));
        if(filename.isEmpty())
        {
            SNAP_LOG_ERROR() << "error: plugin named \"" << name << "\" ("
                << filename << ") not found in the plugin directory."
               ;
            good = false;
            continue;
        }

        // load the plugin; the plugin will register itself
        g_next_register_name = name;
        g_next_register_filename = filename;
        // TODO: Use RTLD_NOW instead of RTLD_LAZY in DEBUG mode
        //         so we discover missing symbols
        void const * const h(dlopen(filename.toUtf8().data(), RTLD_LAZY | RTLD_GLOBAL));
        if(h == nullptr)
        {
            int const e(errno);
            SNAP_LOG_ERROR() << "error: cannot load plugin file \"" << filename << "\" (errno: " << e << ", " << dlerror() << ")";
            good = false;
            continue;
        }
        g_next_register_name.clear();
        g_next_register_filename.clear();
//SNAP_LOG_ERROR() << "note: registering plugin: \"" << name << "\"";
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
QString find_plugin_filename(QStringList const& plugin_paths, QString const& name)
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
bool verify_plugin_name(QString const& name)
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
            SNAP_LOG_ERROR() << "error: plugin name \"" << name
                    << "\" includes forbidden characters.";
            return false;
        }
    }
    // Note: we know that name is not empty
    QChar first(name[0]);
    if(first == '.' || first == '-')
    {
        SNAP_LOG_ERROR() << "error: plugin name \"" << name
                << "\" cannot start with a period (.) or dash (-)."
               ;
        return false;
    }
    // Note: we know that name is not empty
    QChar last(name[name.length() - 1]);
    if(last == '.' || last == '-')
    {
        std::cerr << "error: plugin name \"" << name
                << "\" cannot end with a period (.) or dash (-)."
               ;
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
bool exists(QString const& name)
{
    return g_plugins.contains(name);
}


/** \brief Register a plugin in the list of plugins.
 *
 * This function is called by plugin factories to register new plugins.
 * Do not attempt to call this function directly or you'll get an
 * exception.
 *
 * \exception 
 *
 * \param[in] name  The name of the plugin being added.
 * \param[in] p  A pointer to the plugin being added.
 */
void register_plugin(QString const& name, plugin *p)
{
    if(name.isEmpty()) {
        throw plugin_exception("plugin name missing when registering... expected \"" + name + "\".");
    }
    if(name != g_next_register_name) {
        throw plugin_exception("it is not possible to register a plugin (" + name + ") other than the one being loaded (" + g_next_register_name + ").");
    }
    if(exists(name)) {
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
{
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
int64_t plugin::do_update(int64_t /*last_updated*/)
{
    SNAP_PLUGIN_UPDATE_INIT();

    // in a complete implementation you'd have entries like this one:
    //SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);

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
plugin *get_plugin(const QString& name)
{
    return g_plugins.value(name, nullptr);
}


} // namespace plugins
} // namespace snap
// vim: ts=4 sw=4 et
