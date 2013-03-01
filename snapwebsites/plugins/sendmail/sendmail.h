// Snap Websites Server -- queue emails for the backend to send
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
#ifndef SNAP_SENDMAIL_H
#define SNAP_SENDMAIL_H

#include "plugins.h"
#include "snap_child.h"
#include <QMap>
#include <QVector>
#include <QByteArray>

namespace snap
{
namespace sendmail
{

class sendmail_exception : public snap_exception {};
class sendmail_exception_no_magic : public sendmail_exception {};
class sendmail_exception_invalid_argument : public sendmail_exception {};

enum name_t
{
	SNAP_NAME_MESSAGES_TABLE
};
const char *get_name(name_t name);


class sendmail : public plugins::plugin
{
public:
	class email
	{
	public:
		enum email_list_t
		{
			EMAIL_LIST_TO,
			EMAIL_LIST_CC,
			EMAIL_LIST_BCC
		};
		typedef QMap<email_list_t, QVector<QString> > list_vector_t;

		enum email_priority_t
		{
			EMAIL_PRIORITY_BULK = 1,
			EMAIL_PRIORITY_LOW,
			EMAIL_PRIORITY_NORMAL,
			EMAIL_PRIORITY_HIGH,
			EMAIL_PRIORITY_URGENT
		};

		typedef QMap<QString, QString> header_map_t;

		class email_attachment
		{
		public:
			email_attachment();

			void set_data(const QByteArray& data, QString mime_type);
			void add_header(const QString& name, const QString& value);

		private:
			header_map_t		f_header;
			QByteArray			f_data;
		};
		typedef QVector<email_attachment> attachment_t;

		email();
		~email();

		void				set_from(const QString& from);
		void				set_cumulative(const QString& object);
		void				set_priority(email_priority_t priority = EMAIL_PRIORITY_NORMAL);
		void				clear_list(email_list_t list);
		void				add_to_list(email_list_t list, const QString& address);
		void				add_to_list(email_list_t list, const QStringList& addresses);
		void				set_subject(const QString& subject);
		void				add_header(const QString& name, const QString& value);
		void				add_attachment(const email_attachment& data);

	private:
		QString						f_from;
		QString						f_cumulative;
		list_vector_t				f_list;
		QString						f_subject;
		header_map_t				f_header;
		attachment_t				f_attachment;
	};

	sendmail();
	~sendmail();

	static sendmail *	instance();
	virtual QString		description() const;
	virtual int64_t 	do_update(int64_t last_updated);

	void				on_bootstrap(snap_child *snap);
	void				on_sendemail(const email& e);

private:
	void content_update(int64_t variables_timestamp);

	zpsnap_child_t				f_snap;
};

} // namespace sendmail
} // namespace snap
#endif
// SNAP_MESSAGES_H
// vim: ts=4 sw=4
