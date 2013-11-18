// Snap Websites Server -- plugin loader
// Copyright (C) 2011-2013  Made to Order Software Corp.
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
#include <dlfcn.h>
#include <glob.h>
#include <sys/stat.h>
#include <iostream>
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
 * \todo
 * If the boost filesystem offers a way to glob on files, then make use
 * of it to read the folder files.
 *
 * \param[in] plugin_path  The path to the directory with the plugins.
 * \param[in] server  A pointer to the server to register it as a plugin.
 * \param[in] list_of_plugins  The list of plugins to load.
 *
 * \return true if all the modules were loaded.
 */
bool load(const QString& plugin_path, plugin *server, const QStringList& list_of_plugins)
{
	g_plugins.insert("server", server);

	QString path(plugin_path);
	//if(glob(path.toUtf8().data(), GLOB_ERR | GLOB_NOSORT, NULL, &g) != 0) {
	//	std::cerr << "error: cannot read plugins directory \"" << plugin_path.toUtf8().data() << "\"." << std::endl;
	//	return false;
	//}

	bool good = true;
	for(QStringList::const_iterator it(list_of_plugins.begin());
									it != list_of_plugins.end();
									++it)
	{
		QString name(*it);
		if(name == "server") {
			// the Snap server is already added to the list under that name!
			std::cerr << "error: a plugin cannot be called \"server\"." << std::endl;
			good = false;
			continue;
		}
		// in case we get multiple calls to this function we must make sure that
		// all plugins have a distinct name (i.e. a plugin factory could call
		// this function to load sub-plugins!)
		if(exists(name)) {
			std::cerr << "error: two plugins cannot be named the same, found \""
							<< name.toUtf8().data() << "\" twice." << std::endl;
			good = false;
			continue;
		}
		if(!verify_plugin_name(name)) {
			good = false;
			continue;
		}
		// check that the file exists, if not we simply skip the
		// load step and generate an error
		QString filename = path + "/" + name + "/" + name + ".so";
		if(!QFile::exists(filename)) {
			// plugin names may start with "lib..."
			filename = path + "/" + name + "/lib" + name + ".so";
			if(!QFile::exists(filename)) {
				std::cerr << "error: plugin named \"" << name.toUtf8().data()
						<< "\" (" << filename.toUtf8().data()
						<< ") not found in the plugin directory."
						<< std::endl;
				good = false;
				continue;
			}
		}

		// load the plugin; the plugin will register itself
		g_next_register_name = name;
		g_next_register_filename = filename;
		// TODO: Use RTLD_NOW instead of RTLD_LAZY in DEBUG mode
		//		 so we discover missing symbols
		void *h = dlopen(filename.toUtf8().data(), RTLD_LAZY | RTLD_GLOBAL);
		if(h == NULL) {
			int e(errno);
			std::cerr << "error: cannot load plugin file \"" << filename.toUtf8().data() << "\" (errno: " << e << ", " << dlerror() << ")" << std::endl;
			good = false;
			continue;
		}
		g_next_register_name.clear();
		g_next_register_filename.clear();
//std::cerr << "note: registering plugin: \"" << name << "\"" << std::endl;
	}

	return good;
}

/** \brief Verify that a name is a valid plugin name.
 *
 * This function checks a string to know whether it is a valid plugin name.
 *
 * A valid plugin name is a string of letters (A-Z or a-z), digits (0-9),
 * and the underscore (_), dash (-), and period (.). Although the name
 * cannot start or end with a dash or a period.
 */
bool verify_plugin_name(const QString& name)
{
	if(name.isEmpty()) {
		std::cerr << "error: an empty plugin name is not valid." << std::endl;
		return false;
	}
	for(QString::const_iterator p(name.begin()); p != name.end(); ++p) {
		if((*p < 'a' || *p > 'z')
		&& (*p < 'A' || *p > 'Z')
		&& (*p < '0' || *p > '9')
		&& *p != '_' && *p != '-' && *p != '.') {
			std::cerr << "error: plugin name \"" << name.toUtf8().data()
					<< "\" includes forbidden characters." << std::endl;
			return false;
		}
	}
	// Note: we know that name is not empty
	QChar first(name[0]);
	if(first == '.' || first == '-') {
		std::cerr << "error: plugin name \"" << name.toUtf8().data()
				<< "\" cannot start with a period (.) or dash (-)."
				<< std::endl;
		return false;
	}
	// Note: we know that name is not empty
	QChar last(name[name.length() - 1]);
	if(last == '.' || last == '-') {
		std::cerr << "error: plugin name \"" << name.toUtf8().data()
				<< "\" cannot end with a period (.) or dash (-)."
				<< std::endl;
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
bool exists(const QString& name)
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
void register_plugin(const QString& name, plugin *p)
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
	: f_name(g_next_register_name),
	  f_filename(g_next_register_filename)
	  //f_last_modification(0) -- auto-init
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
	if(0 == f_last_modification) {
		// read the info only once
		struct stat s;
		if(stat(f_filename.toUtf8().data(), &s) == 0) {
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
 * \return This function returns a pointer to the named plugin, or NULL
 * if the plugin was not loaded.
 *
 * \sa load()
 * \sa exists()
 */
plugin *get_plugin(const QString& name)
{
	return g_plugins.value(name, NULL);
}

} // namespace plugins
} // namespace snap
// vim: ts=4 sw=4
