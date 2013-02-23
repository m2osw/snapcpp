// Snap Websites Server -- filter
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
#ifndef SNAP_FILTER_H
#define SNAP_FILTER_H

#include "snap_child.h"
#include "plugins.h"
#include <QtXml/QDomDocument>

namespace snap
{
namespace filter
{

class filter : public plugins::plugin
{
public:
	filter();
	~filter();

	static filter *		instance();
	virtual QString		description() const;

	void	on_bootstrap(::snap::snap_child *snap);
	void	on_xss_filter(QDomNode& node,
						  const QString& accepted_tags,
						  const QString& accepted_attributes);

private:
	snap_child *	f_snap;
};

} // namespace filter
} // namespace snap
#endif
// SNAP_FILTER_H
// vim: ts=4 sw=4
