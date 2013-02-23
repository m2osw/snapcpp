// Snap Websites Servers -- HTTP cookie handling (outgoing)
// Copyright (C) 2013  Made to Order Software Corp.
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
#ifndef SNAP_HTTP_COOKIE_H
#define SNAP_HTTP_COOKIE_H

#include <controlled_vars/controlled_vars_auto_init.h>
#include <controlled_vars/controlled_vars_ptr_auto_init.h>
#include <QDateTime>

namespace snap
{

class snap_child;
typedef controlled_vars::ptr_auto_init<snap_child>	zpsnap_child_t;

class http_cookie
{
public:
	enum http_cookie_type_t
	{
		HTTP_COOKIE_TYPE_PERMANENT,
		HTTP_COOKIE_TYPE_SESSION,
		HTTP_COOKIE_TYPE_DELETE
	};

	http_cookie(); // for QMap to work; DO NOT USE!
	http_cookie(snap_child *snap, const QString& name, const QString& value = "");

	void set_value(const QString& value);
	void set_value(const QByteArray& value);
	void set_domain(const QString& domain);
	void set_path(const QString& path);
	void set_delete();
	void set_session();
	void set_expire(const QDateTime& date_time);
	void set_expire_in(int seconds);
	void set_secure(bool secure = true);
	void set_http_only(bool http_only = true);

	const QString& get_name() const;
	const QByteArray& get_value() const;
	http_cookie_type_t get_type() const;
	const QString& get_domain() const;
	const QString& get_path() const;
	const QDateTime& get_expire() const;
	bool get_secure() const;
	bool get_http_only() const;

	QString to_http_header() const;

private:
	zpsnap_child_t				f_snap;      // the snap child that created this cookie
	QString						f_name;      // name of the cookie
	QByteArray					f_value;     // the cookie value (binary buffer)
	QString						f_domain;    // domain for which the cookie is valid
	QString						f_path;      // path under which the cookie is valid
	QDateTime					f_expire;    // when to expire the cookie (if null, session, if past delete)
	controlled_vars::fbool_t	f_secure;    // only valid on HTTPS
	controlled_vars::fbool_t	f_http_only; // JavaScript cannot access this cookie
};



} // namespace snap
#endif
// SNAP_HTTP_COOKIE_H
// vim: ts=4 sw=4
