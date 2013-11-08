// Snap Websites Server -- sitemap.xml
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
#ifndef SNAP_SITEMAPXML_H
#define SNAP_SITEMAPXML_H

#include "../path/path.h"
#include "../robotstxt/robotstxt.h"
#include <string>
#include <map>

namespace snap
{
namespace sitemapxml
{

enum name_t
{
	SNAP_NAME_SITEMAPXML_COUNT,
	SNAP_NAME_SITEMAPXML_SITEMAP_XML,
	SNAP_NAME_SITEMAPXML_PRIORITY
};
const char *get_name(name_t name);


class sitemapxml_exception : public snap_exception {};
class sitemapxml_exception_missing_links_table : public sitemapxml_exception {};

class sitemapxml : public plugins::plugin, public path::path_execute
{
public:
	class url_info
	{
	public:
		static const int FREQUENCY_NONE = 0;
		static const int FREQUENCY_NEVER = -1;

		url_info();

		void set_uri(const QString& uri);
		void set_priority(float priority);
		void set_last_modification(time_t last_modification);
		void set_frequency(int frequency);

		QString get_uri() const;
		float get_priority() const;
		time_t get_last_modification() const;
		int get_frequency() const;

		bool operator < (const url_info& rhs) const;

	private:
		typedef controlled_vars::auto_init<time_t, 0>		ztime_t;
		typedef controlled_vars::auto_init<int, 604800>		weekly_t;

		QString						f_uri;					// the page URI
		controlled_vars::mfloat_t	f_priority;				// 0.001 to 1.0, default 0.5
		ztime_t						f_last_modification;	// Unix date when it was last modified
		weekly_t					f_frequency;			// number of seconds between modifications
	};
	typedef std::vector<url_info>		url_info_list_t;

	sitemapxml();
	~sitemapxml();

	static sitemapxml *	instance();
	virtual QString		description() const;
	virtual int64_t 	do_update(int64_t last_updated);

	void				on_bootstrap(::snap::snap_child *snap);
	void				on_generate_robotstxt(robotstxt::robotstxt *r);
	void				on_backend_process();
	virtual bool 		on_path_execute(const QString& url);

	SNAP_SIGNAL(generate_sitemapxml, (sitemapxml *sitemap), (sitemap));

	void				add_url(const url_info& url);

private:
	void		initial_update(int64_t variables_timestamp);
	void		content_update(int64_t variables_timestamp);

	zpsnap_child_t		f_snap;
	url_info_list_t		f_url_info;
};

} // namespace sitemapxml
} // namespace snap
#endif
// SNAP_SITEMAPXML_H
// vim: ts=4 sw=4
