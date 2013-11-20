// Snap Websites Servers -- snap websites child
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
#ifndef SNAP_SNAPCHILD_H
#define SNAP_SNAPCHILD_H

#include "snap_uri.h"
#include "snap_signals.h"
#include "snap_exception.h"
#include "http_cookie.h"
#include "udp_client_server.h"
#include <stdlib.h>
#include <controlled_vars/controlled_vars_need_init.h>
#include <QPointer>
#include <QDomDocument>
#include <QBuffer>
#include <QtCassandra/QCassandra.h>
#include <QtCassandra/QCassandraContext.h>

namespace snap
{

class snap_child_exception : public snap_exception {};
class snap_child_exception_unique_number_error : public snap_child_exception {};
class snap_child_exception_invalid_header_value : public snap_child_exception {};
class snap_child_exception_invalid_header_field_name : public snap_child_exception {};


class server;


class snap_child
{
public:
	enum status_t {
		SNAP_CHILD_STATUS_READY,
		SNAP_CHILD_STATUS_RUNNING
	};
	typedef std::shared_ptr<server> server_pointer_t;

	snap_child(server_pointer_t s);
	~snap_child();

	bool process(int socket);
	void backend();
	status_t check_status();

	const snap_uri& get_uri() const;

	QtCassandra::QCassandraValue get_site_parameter(const QString& name);
	void set_site_parameter(const QString& name, const QtCassandra::QCassandraValue& value);
	QSharedPointer<QtCassandra::QCassandraContext> get_context() { return f_context; }
	const QString& get_domain_key() const { return f_domain_key; }
	const QString& get_website_key() const { return f_website_key; }
	const QString& get_site_key() const { return f_site_key; }
	const QString& get_site_key_with_slash() const { return f_site_key_with_slash; }
	int64_t get_start_date() const { return f_start_date; }
	void set_header(const QString& name, const QString& value);
	void set_cookie(const http_cookie& cookie);
	bool has_header(const QString& name) const;
	QString get_header(const QString& name) const;
	QString get_unique_number();
	QSharedPointer<QtCassandra::QCassandraTable> create_table(const QString& table_name, const QString& comment);
	void new_content();
	static void canonicalize_path(QString& path);
	static QString date_to_string(int64_t v, bool long_format = false);

	QString snapenv(const QString& name) const;
	QString postenv(const QString& name, const QString& default_value = "") const;
	bool cookie_is_defined(const QString& name) const;
	QString cookie(const QString& name) const;
	QString snap_url(const QString& url) const;
	void die(int err_code, QString err_name, const QString& err_description, const QString& err_details);
	static void define_error_name(int err_code, QString& err_name);

	void output(const QString& data);
	void output(const std::string& data);
	void output(const char *data);
	bool empty_output() const;

	void udp_ping(const char *name, const char *message = "PING");
	QSharedPointer<udp_client_server::udp_server> udp_get_server(const char *name);

private:
	void read_environment();
	void init_start_date();
	void setup_uri();
	void snap_info();
	void snap_statistics();
	void connect_cassandra();
	void canonicalize_domain();
	void canonicalize_website();
	void site_redirect();
	void init_plugins();
	void update_plugins(const QStringList& list_of_plugins);
	void execute();
	void process_backend_uri(const QString& uri);
	void write(const char *data, ssize_t size);
	void write(const char *str);
	void write(const QString& str);

	typedef QMap<QString, QString>		environment_map_t;
	typedef QMap<QString, QString>		header_map_t;
	typedef QMap<QString, http_cookie>	cookie_map_t;

	controlled_vars::mint64_t			f_start_date; // time request arrived
	server_pointer_t					f_server;
	QPointer<QtCassandra::QCassandra>	f_cassandra;
	QSharedPointer<QtCassandra::QCassandraContext>	f_context;
	QSharedPointer<QtCassandra::QCassandraTable> f_site_table;
	controlled_vars::fbool_t			f_new_content;
	controlled_vars::fbool_t			f_is_child;
	pid_t								f_child_pid;
	int									f_socket;
	environment_map_t					f_env;
	environment_map_t					f_post;
	environment_map_t					f_browser_cookies;
	controlled_vars::fbool_t			f_has_post;
	mutable controlled_vars::fbool_t	f_fixed_server_protocol;
	snap_uri							f_uri;
	QString								f_domain_key;
	QString								f_website_key;
	QString								f_site_key;
	QString								f_site_key_with_slash;
	QString								f_original_site_key;
	QBuffer								f_output;
	header_map_t						f_header;
	cookie_map_t						f_cookies;
};

typedef std::vector<snap_child *>					snap_child_vector_t;

} // namespace snap
#endif
// SNAP_SNAPCHILD_H
// vim: ts=4 sw=4
