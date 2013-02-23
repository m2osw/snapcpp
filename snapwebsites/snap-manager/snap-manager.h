// Snap Manager -- Cassandra manager for Snap! Servers
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
#ifndef SNAP_MANAGER_H
#define SNAP_MANAGER_H

#include "snapwebsites.h"
#include <boost/shared_ptr.hpp>
#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <QtNetwork/QTcpSocket>
#include <QtGui/QMainWindow>
#include <QtCassandra/QCassandra.h>
#include <QtCassandra/QCassandraContext.h>

#include "ui_snap-manager-mainwindow.h"

// We do not use namespaces because that doesn't work too well with
// Qt tools such as moc.

class snap_manager : public QMainWindow, public Ui_MainWindow
{
	Q_OBJECT

public:
	snap_manager(QWidget *parent = NULL);
	virtual ~snap_manager();

private slots:
	void about();
	void help();
	void decode_utf8();
	void snapTest();
	void snapStats();
	void cassandraConnect();
	void cassandraDisconnect();
	void reset_domains_index();
	void reset_websites_index();
	void on_domainFilter_clicked();
	void on_domainNew_clicked();
	void on_domainList_itemClicked(QListWidgetItem *item);
	void on_domainSave_clicked();
	void on_domainCancel_clicked();
	void on_domainDelete_clicked();
	void on_websiteNew_clicked();
	void on_websiteList_itemClicked(QListWidgetItem *item);
	void on_websiteSave_clicked();
	void on_websiteCancel_clicked();
	void on_websiteDelete_clicked();
	void quit();

private:
	void loadDomains();
	void domainWithSelection();
	bool domainChanged();

	void loadWebsites();
	void websiteWithSelection();
	bool websiteChanged();

	virtual void closeEvent(QCloseEvent *event);

	QPointer<QWidget>			f_about;
	QPointer<QWidget>			f_help;
	QPointer<QWidget>			f_decode_utf8;
	QPointer<QTabWidget>		f_tabs;
	QPointer<QWidget>			f_tab_connect;
	int							f_idx_connect;
	QPointer<QWidget>			f_tab_domain;
	int							f_idx_domain;

	QPointer<QAction>			f_reset_domains_index;
	QPointer<QAction>			f_reset_websites_index;

	// snap domain
	QString						f_domain_org_name; // the original name (in case user changes it)
	QString						f_domain_org_rules; // the original rules (to check Cancel properly)
	QPointer<QPushButton>		f_domain_filter;
	QPointer<QLineEdit>			f_domain_filter_string;
	QPointer<QListWidget>		f_domain_list;
	QPointer<QLineEdit>			f_domain_name;
	QPointer<QTextEdit>			f_domain_rules;
	QPointer<QPushButton>		f_domain_new;
	QPointer<QPushButton>		f_domain_save;
	QPointer<QPushButton>		f_domain_cancel;
	QPointer<QPushButton>		f_domain_delete;

	// snap website
	QString						f_website_org_name; // the original name (in case user changes it)
	QString						f_website_org_rules; // the original rules (to check Cancel properly)
	QPointer<QListWidget>		f_website_list;
	QPointer<QLineEdit>			f_website_name;
	QPointer<QTextEdit>			f_website_rules;
	QPointer<QPushButton>		f_website_new;
	QPointer<QPushButton>		f_website_save;
	QPointer<QPushButton>		f_website_cancel;
	QPointer<QPushButton>		f_website_delete;

	// snap server
	QString						f_snap_host;
	int							f_snap_port;

	// cassandra data
	QString						f_cassandra_host;
	int							f_cassandra_port;
	QPointer<QtCassandra::QCassandra>				f_cassandra;
	QSharedPointer<QtCassandra::QCassandraContext>	f_context;
};



#endif
// SNAP_MANAGER_H
// vim: ts=4 sw=4
