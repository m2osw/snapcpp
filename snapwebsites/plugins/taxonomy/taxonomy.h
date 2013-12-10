// Snap Websites Server -- taxonomy
// Copyright (C) 2012  Made to Order Software Corp.
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
#ifndef SNAP_TAXONOMY_H
#define SNAP_TAXONOMY_H

#include "../path/path.h"
#include "../layout/layout.h"
#include "snap_child.h"
#include "plugins.h"

namespace snap
{
namespace taxonomy
{

enum name_t {
	SNAP_NAME_TAXONOMY_NAME
};
const char *get_name(name_t name);


class taxonomy : public plugins::plugin, public path::path_execute, public layout::layout_content
{
public:
	taxonomy();
	~taxonomy();

	static taxonomy *	instance();
	virtual QString		description() const;
	virtual int64_t		do_update(int64_t last_updated);

	void				on_bootstrap(::snap::snap_child *snap);
	virtual bool		on_path_execute(const QString& url);
	virtual void 		on_generate_main_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate);

	QtCassandra::QCassandraValue	find_type_with(const QString& cpath, const QString& taxonomy, const QString& col_name, const QString& limit_name);

private:
	void				initial_update(int64_t variables_timestamp);
	void				content_update(int64_t variables_timestamp);

	zpsnap_child_t		f_snap;
};

} // namespace taxonomy
} // namespace snap
#endif
// SNAP_TAXONOMY_H
// vim: ts=4 sw=4
