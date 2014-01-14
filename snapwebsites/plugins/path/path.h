// Snap Websites Server -- path handling
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
#ifndef SNAP_PATH_H
#define SNAP_PATH_H

#include "snapwebsites.h"
#include <controlled_vars/controlled_vars_ptr_no_init.h>

namespace snap
{
namespace path
{

enum name_t
{
	SNAP_NAME_PATH_PRIMARY_OWNER
};
const char *get_name(name_t name) __attribute__ ((const));


class path_execute
{
public:
	virtual ~path_execute() {} // ensure proper virtual tables
	virtual bool on_path_execute(QString const& url) = 0;
};

class path : public plugins::plugin
{
public:
						path();
						~path();

	static path *		instance();
	virtual QString		description() const;

	void				on_bootstrap(::snap::snap_child *snap);
	void				on_init();
	void				on_execute(QString const& uri_path);
	plugin *			get_plugin(QString const& uri_path, permission_error_callback& err_callback);

	SNAP_SIGNAL(can_handle_dynamic_path, (path *path_plugin, QString const& cpath), (path_plugin, cpath));
	SNAP_SIGNAL(page_not_found, (path *path_plugin, QString const& cpath), (path_plugin, cpath));

	void 				handle_dynamic_path(plugins::plugin *p);

private:
	typedef controlled_vars::ptr_no_init<plugins::plugin> dynamic_path_plugin_t;

	zpsnap_child_t									f_snap;
	QString											f_primary_owner;
	dynamic_path_plugin_t							f_path_plugin;
	controlled_vars::zint64_t						f_last_modified;
};

} // namespace path
} // namespace snap
#endif
// SNAP_PATH_H
// vim: ts=4 sw=4
