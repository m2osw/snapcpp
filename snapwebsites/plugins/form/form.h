// Snap Websites Server -- form handling
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
#ifndef SNAP_FROM_H
#define SNAP_FROM_H

#include "../sessions/sessions.h"
#include <QtCassandra/QCassandraTable.h>
#include <QDomDocument>

namespace snap
{
namespace form
{

enum name_t {
	SNAP_NAME_FORMS_TABLE
};
const char *get_name(name_t name);


class form_post
{
public:
	virtual ~form_post() {}

	virtual QDomDocument on_get_xml_form(const QString& cpath) = 0;
	virtual void on_process_post(const QString& cpath, const sessions::sessions::session_info& info) = 0;
};


class form : public plugins::plugin
{
public:
	form();
	~form();

	static form *		instance();
	virtual QString		description() const;
	QSharedPointer<QtCassandra::QCassandraTable> get_form_table();

	void on_bootstrap(::snap::snap_child *snap);
	void on_init();
	void on_process_post(const QString& uri_path);

	SNAP_SIGNAL(form_element, (form *f), (f));
	SNAP_SIGNAL(validate_post_for_widget, (const QString& cpath, sessions::sessions::session_info& info, const QDomElement& widget, const QString& widget_name, const QString& widget_type, bool is_secret), (cpath, info, widget, widget_name, widget_type, is_secret));

	QDomDocument form_to_html(const sessions::sessions::session_info& info, const QDomDocument& xml);
	void add_form_elements(QDomDocument& add);
	void add_form_elements(QString& filename);

	static QString text_64max(const QString& text, bool is_secret);
	static QString html_64max(const QString& html, bool is_secret);
	static int count_text_lines(const QString& text);
	static int count_html_lines(const QString& html);

private:
	zpsnap_child_t					f_snap;
	controlled_vars::fbool_t		f_form_initialized;
	QDomDocument					f_form_elements;
	QDomElement 					f_form_stylesheet;
	QString							f_form_elements_string;
};

} // namespace form
} // namespace snap
#endif
// SNAP_FORM_H
// vim: ts=4 sw=4
