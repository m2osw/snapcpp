// Snap Manager -- snap database manager to work on Cassandra's tables
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

#include "snap-manager.h"
#include "snap-manager-about.h"
#include "snap-manager-help.h"
#include "snap-manager-decode-utf8.h"
#include "snapwebsites.h"
#include "snap_uri.h"
#include "../plugins/content/content.h"
#include <QHostAddress>
#include <QApplication>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>
#include <QSettings>
#include <libtld/tld.h>
#include <stdio.h>


// get a child that MUST exist
template<class T>
T *getChild(QWidget *parent, const char *name)
{
    T *w = parent->findChild<T *>(name);
    if(w == NULL) {
            QString error(QString("Can't find the widget: %1.").arg(name));
            QMessageBox msg(QMessageBox::Critical, "Internal Error", error, QMessageBox::Ok, parent);
            msg.exec();
            exit(1);
            /*NOTREACHED*/
    }

    return w;
}

snap_manager::snap_manager(QWidget *snap_parent)
    : QMainWindow(snap_parent)
{
    setupUi(this);

    QSettings settings( this );
    restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );
    restoreState   ( settings.value( "state"   , saveState()    ).toByteArray() );
    //
    cassandraHost->setText  ( settings.value( "cassandra_host", "127.0.0.1" ).toString() );
    cassandraPort->setText  ( settings.value( "cassandra_port", 4004        ).toString() );
    snapServerHost->setText ( settings.value( "snap_host",      "localhost" ).toString() );
    snapServerPort->setText ( settings.value( "snap_port",      "9160"      ).toString() );

    // Help
    QAction *a = getChild<QAction>(this, "actionSnap_Manager_Help");
    connect(a, SIGNAL(activated()), this, SLOT(help()));

    // About
    a = getChild<QAction>(this, "actionAbout_Snap_Manager");
    connect(a, SIGNAL(activated()), this, SLOT(about()));

    // Tools: Reset Domains Index
    f_reset_domains_index = getChild<QAction>(this, "actionResetDomainsIndex");
    connect(f_reset_domains_index, SIGNAL(activated()), this, SLOT(reset_domains_index()));

    // Tools: Reset Websites Index
    f_reset_websites_index = getChild<QAction>(this, "actionResetWebsitesIndex");
    connect(f_reset_websites_index, SIGNAL(activated()), this, SLOT(reset_websites_index()));

    // Tools: Decode UTF-8
    a = getChild<QAction>(this, "actionDecodeUTF8");
    connect(a, SIGNAL(activated()), this, SLOT(decode_utf8()));

    f_tabs = getChild<QTabWidget>(this, "tabWidget");
    f_tabs->setTabEnabled(TAB_HOSTS, false);
    f_tabs->setTabEnabled(TAB_DOMAINS, false);
    f_tabs->setTabEnabled(TAB_WEBSITES, false);
    f_tabs->setTabEnabled(TAB_SITES, false);

    // Snap! Server Connect
    QPushButton *b = getChild<QPushButton>(this, "snapTest");
    connect(b, SIGNAL(clicked()), this, SLOT(snapTest()));
    b = getChild<QPushButton>(this, "snapStats");
    connect(b, SIGNAL(clicked()), this, SLOT(snapStats()));

    // Snap! Server Info
    QListWidget *console = getChild<QListWidget>(this, "snapServerConsole");
    console->addItem("snap::server version: " + QString(snap::server::version()));
    console->addItem("Not tested.");

    // Cassandra Info
    console = getChild<QListWidget>(this, "cassandraConsole");
    console->addItem("libQtCassandra version: " + QString(f_cassandra->version()));
    console->addItem("Not connected.");

    // get host friends that are going to be used here and there
    f_host_filter = getChild<QPushButton>(this, "hostFilter");
    //connect(f_host_filter, SIGNAL(itemClicked()), this, SLOT(on_domainFilter_clicked()));
    f_host_filter_string = getChild<QLineEdit>(this, "hostFilterString");
    f_host_list = getChild<QListWidget>(this, "hostList");
    //connect(f_host_list, SIGNAL(itemClicked()), this, SLOT(on_hostList_itemClicked()));
    f_host_name = getChild<QLineEdit>(this, "hostName");
    f_host_new = getChild<QPushButton>(this, "hostNew");
    //connect(f_host_new, SIGNAL(clicked()), this, SLOT(on_hostNew_clicked()));
    f_host_save = getChild<QPushButton>(this, "hostSave");
    //connect(f_host_save, SIGNAL(clicked()), this, SLOT(on_hostSave_clicked()));
    f_host_cancel = getChild<QPushButton>(this, "hostCancel");
    //connect(f_host_cancel, SIGNAL(clicked()), this, SLOT(on_hostCancel_clicked()));
    f_host_delete = getChild<QPushButton>(this, "hostDelete");
    //connect(f_host_delete, SIGNAL(clicked()), this, SLOT(on_hostDelete_clicked()));

    // get domain friends that are going to be used here and there
    f_domain_filter = getChild<QPushButton>(this, "domainFilter");
    //connect(f_domain_filter, SIGNAL(itemClicked()), this, SLOT(on_domainFilter_clicked()));
    f_domain_filter_string = getChild<QLineEdit>(this, "domainFilterString");
    f_domain_list = getChild<QListWidget>(this, "domainList");
    //connect(f_domain_list, SIGNAL(itemClicked()), this, SLOT(on_domainList_itemClicked()));
    f_domain_name = getChild<QLineEdit>(this, "domainName");
    f_domain_rules = getChild<QTextEdit>(this, "domainRules");
    f_domain_new = getChild<QPushButton>(this, "domainNew");
    //connect(f_domain_new, SIGNAL(clicked()), this, SLOT(on_domainNew_clicked()));
    f_domain_save = getChild<QPushButton>(this, "domainSave");
    //connect(f_domain_save, SIGNAL(clicked()), this, SLOT(on_domainSave_clicked()));
    f_domain_cancel = getChild<QPushButton>(this, "domainCancel");
    //connect(f_domain_cancel, SIGNAL(clicked()), this, SLOT(on_domainCancel_clicked()));
    f_domain_delete = getChild<QPushButton>(this, "domainDelete");
    //connect(f_domain_delete, SIGNAL(clicked()), this, SLOT(on_domainDelete_clicked()));

    // get website friends that are going to be used here and there
    f_website_list = getChild<QListWidget>(this, "websiteList");
    //connect(f_website_list, SIGNAL(itemClicked()), this, SLOT(on_websiteList_itemClicked()));
    f_website_name = getChild<QLineEdit>(this, "fullDomainName");
    f_website_rules = getChild<QTextEdit>(this, "websiteRules");
    f_website_new = getChild<QPushButton>(this, "websiteNew");
    //connect(f_website_new, SIGNAL(clicked()), this, SLOT(on_websiteNew_clicked()));
    f_website_save = getChild<QPushButton>(this, "websiteSave");
    //connect(f_website_save, SIGNAL(clicked()), this, SLOT(on_websiteSave_clicked()));
    f_website_cancel = getChild<QPushButton>(this, "websiteCancel");
    //connect(f_website_cancel, SIGNAL(clicked()), this, SLOT(on_websiteCancel_clicked()));
    f_website_delete = getChild<QPushButton>(this, "websiteDelete");
    //connect(f_website_delete, SIGNAL(clicked()), this, SLOT(on_websiteDelete_clicked()));

    // get sites friends that are going to be used here and there
    f_sites_filter = getChild<QPushButton>(this, "sitesFilter");
    //connect(f_sites_filter, SIGNAL(itemClicked()), this, SLOT(on_sitesFilter_clicked()));
    f_sites_filter_string = getChild<QLineEdit>(this, "sitesFilterString");
    f_sites_list = getChild<QListWidget>(this, "sitesList");
    //connect(f_sites_list, SIGNAL(itemClicked()), this, SLOT(on_sitesList_itemClicked()));
    f_sites_name = getChild<QLineEdit>(this, "sitesDomainName");
    f_sites_parameters = getChild<QTableWidget>(this, "sitesParameters");
    f_sites_parameter_name = getChild<QLineEdit>(this, "sitesParameterName");
    f_sites_parameter_value = getChild<QLineEdit>(this, "sitesParameterValue");
    f_sites_parameter_type = getChild<QComboBox>(this, "sitesParameterType");
    f_sites_new = getChild<QPushButton>(this, "sitesNew");
    //connect(f_sites_new, SIGNAL(clicked()), this, SLOT(on_sitesNew_clicked()));
    f_sites_save = getChild<QPushButton>(this, "sitesSave");
    //connect(f_sites_save, SIGNAL(clicked()), this, SLOT(on_sitesSave_clicked()));
    f_sites_delete = getChild<QPushButton>(this, "sitesDelete");
    //connect(f_sites_delete, SIGNAL(clicked()), this, SLOT(on_sitesDelete_clicked()));

    f_sites_parameters->setColumnCount(2);
    QStringList labels;
    labels += QString("Name");
    labels += QString("Value");
    f_sites_parameters->setHorizontalHeaderLabels(labels);

    f_sites_parameter_type->addItem("Null");
    f_sites_parameter_type->addItem("String"); // this is the default
    f_sites_parameter_type->addItem("Boolean");
    f_sites_parameter_type->addItem("Integer (8 bit)");
    f_sites_parameter_type->addItem("Integer (16 bit)");
    f_sites_parameter_type->addItem("Integer (32 bit)");
    f_sites_parameter_type->addItem("Integer (64 bit)");
    f_sites_parameter_type->addItem("Floating Point (32 bit)");
    f_sites_parameter_type->addItem("Floating Point (64 bit)");
    f_sites_parameter_type->setCurrentIndex(1);

    connect( qApp, SIGNAL(aboutToQuit()), this, SLOT(on_about_to_quit()) );
}

snap_manager::~snap_manager()
{
}


void snap_manager::on_about_to_quit()
{
    QSettings settings( this );
    settings.setValue( "cassandra_host", cassandraHost->text()  );
    settings.setValue( "cassandra_port", cassandraPort->text()  );
    settings.setValue( "snap_host",      snapServerHost->text() );
    settings.setValue( "snap_port",      snapServerPort->text() );
    settings.setValue( "geometry",       saveGeometry()         );
    settings.setValue( "state",          saveState()            );
}


void snap_manager::about()
{
    if(f_about == NULL) {
        f_about = new snap_manager_about(this);
    }
    f_about->show();
}

void snap_manager::help()
{
    if(f_help == NULL) {
        f_help = new snap_manager_help(this);
    }
    f_help->show();
}

void snap_manager::decode_utf8()
{
    if(f_decode_utf8 == NULL) {
        f_decode_utf8 = new snap_manager_decode_utf8(this);
    }
    f_decode_utf8->show();
}

void snap_manager::snapTest()
{
    // retrieve the current values
    QLineEdit *l = getChild<QLineEdit>(this, "snapServerHost");
    f_snap_host = l->text();
    if(f_snap_host.isEmpty()) {
        f_snap_host = "localhost";
    }
    l = getChild<QLineEdit>(this, "snapServerPort");
    if(l->text().isEmpty()) {
        f_snap_port = 4004;
    }
    else {
        f_snap_port = l->text().toInt();
    }

    QListWidget *console = getChild<QListWidget>(this, "snapServerConsole");
    console->clear();
    console->addItem("snap::server version: " + QString(snap::server::version()));
    console->addItem("Host: " + f_snap_host);
    console->addItem("Port: " + QString::number(f_snap_port));

    // reconnect with the new info
    // note: the disconnect does nothing if not already connected
    QHostAddress addr(f_snap_host);
    QTcpSocket socket;
    socket.connectToHost(addr, f_snap_port);
    if(!socket.waitForConnected()) {
        // did not work...
        console->addItem("Not connected.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to connect to your Snap! Server. Please verify that it is up and running and accessible (no firewall) from this computer.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    // send the #INFO command
    if(socket.write("#INFO\n", 6) != 6) {
        console->addItem("Unknown state.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to communicate with the Snap! Server (write error).", QMessageBox::Ok, this);
        msg.exec();
        return;
    }

    // read the results of the #INFO command
    bool started(false);
    if(!socket.waitForReadyRead()) {
        console->addItem("Unknown state.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager connection did not last, cannot read from it. Socket error: " + QString::number(static_cast<int>(socket.error())), QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    for(;;) {
        // versions are expected to be relatively small so 256 chars per line is enough
        char buf[256];
        int r = socket.readLine(buf, sizeof(buf));
        if(r <= 0) {
            // note that r == 0 is not an error but it should not happen
            // (i.e. I/O is blocking so we should not return too soon.)
            console->addItem("Unknown state.");
            QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to communicate with the Snap! Server (read error).", QMessageBox::Ok, this);
            msg.exec();
            return;
        }
        if(!started) {
            if(strcmp(buf, "#START\n") != 0) {
                console->addItem("Connected with an invalid status.");
                QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was able to communicate with the Snap! Server but got unexpected protocol data.", QMessageBox::Ok, this);
                msg.exec();
                return;
            }
            started = true;
        }
        else if(strcmp(buf, "#END\n") == 0) {
            // got the #END mark, we're done
            break;
        }
        else {
            char *v = strchr(buf, '=');
            if(v == NULL) {
                console->addItem("Connected with an invalid status.");
                QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was able to communicate with the Snap! Server but got unexpected variable data.", QMessageBox::Ok, this);
                msg.exec();
                return;
            }
            QString name(QString::fromAscii(buf, v - buf));
            QString value(v + 1);
            value = value.trimmed();
            if(name == "VERSION") {
                console->addItem("Live Snap Server v" + value);
            }
            else if(name == "OS") {
                console->addItem("Operating System: " + value);
            }
            else if(name == "QT") {
                console->addItem("Snap Server compiled with Qt v" + value);
            }
            else if(name == "RUNTIME_QT") {
                console->addItem("Snap Server running with Qt v" + value);
            }
            else if(name == "LIBTLD") {
                console->addItem("Snap Server compiled with libtld v" + value);
            }
            else if(name == "RUNTIME_LIBTLD") {
                console->addItem("Snap Server running with libtld v" + value);
            }
            else if(name == "LIBQTCASSANDRA") {
                console->addItem("Snap Server compiled with libQtCassandra v" + value);
            }
            else if(name == "RUNTIME_LIBQTCASSANDRA") {
                console->addItem("Snap Server running with libQtCassandra v" + value);
            }
            else if(name == "LIBQTSERIALIZATION") {
                console->addItem("Snap Server compiled with libQtSerialization v" + value);
            }
            else if(name == "RUNTIME_LIBQTSERIALIZATION") {
                console->addItem("Snap Server running with libQtSerialization v" + value);
            }
            else {
                console->addItem("Unknown variable: " + name + "=" + value);
            }
        }
    }
}

void snap_manager::snapStats()
{
    // retrieve the current values
    QLineEdit *l = getChild<QLineEdit>(this, "snapServerHost");
    f_snap_host = l->text();
    if(f_snap_host.isEmpty()) {
        f_snap_host = "localhost";
    }
    l = getChild<QLineEdit>(this, "snapServerPort");
    if(l->text().isEmpty()) {
        f_snap_port = 4004;
    }
    else {
        f_snap_port = l->text().toInt();
    }

    QListWidget *console = getChild<QListWidget>(this, "snapServerConsole");
    console->clear();
    console->addItem("snap::server version: " + QString(snap::server::version()));
    console->addItem("Host: " + f_snap_host);
    console->addItem("Port: " + QString::number(f_snap_port));

    // reconnect with the new info
    // note: the disconnect does nothing if not already connected
    QHostAddress addr(f_snap_host);
    QTcpSocket socket;
    socket.connectToHost(addr, f_snap_port);
    if(!socket.waitForConnected()) {
        // did not work...
        console->addItem("Not connected.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to connect to your Snap! Server. Please verify that it is up and running and accessible (no firewall) from this computer.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    // send the #STATS command
    if(socket.write("#STATS\n", 7) != 7) {
        console->addItem("Unknown state.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to communicate with the Snap! Server (write error).", QMessageBox::Ok, this);
        msg.exec();
        return;
    }

    // read the results of the #INFO command
    bool started(false);
    if(!socket.waitForReadyRead()) {
        console->addItem("Unknown state.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager connection did not last, cannot read from it. Socket error: " + QString::number(static_cast<int>(socket.error())), QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    for(;;) {
        // versions are expected to be relatively small so 256 chars per line is enough
        char buf[256];
        int r = socket.readLine(buf, sizeof(buf));
        if(r <= 0) {
            // note that r == 0 is not an error but it should not happen
            // (i.e. I/O is blocking so we should not return too soon.)
            console->addItem("Unknown state.");
            QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was not able to communicate with the Snap! Server (read error).", QMessageBox::Ok, this);
            msg.exec();
            return;
        }
        if(!started) {
            if(strcmp(buf, "#START\n") != 0) {
                console->addItem("Connected with an invalid status.");
                QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was able to communicate with the Snap! Server but got unexpected protocol data.", QMessageBox::Ok, this);
                msg.exec();
                return;
            }
            started = true;
        }
        else if(strcmp(buf, "#END\n") == 0) {
            // got the #END mark, we're done
            break;
        }
        else {
            char *v = strchr(buf, '=');
            if(v == NULL) {
                console->addItem("Connected with an invalid status.");
                QMessageBox msg(QMessageBox::Critical, "Connection to Snap! Server", "Snap! Manager was able to communicate with the Snap! Server but got unexpected variable data.", QMessageBox::Ok, this);
                msg.exec();
                return;
            }
            QString name(QString::fromAscii(buf, v - buf));
            QString value(v + 1);
            value = value.trimmed();
            if(name == "VERSION") {
                console->addItem("Live Snap Server v" + value);
                // add an empty line before the stats
                console->addItem(" ");
            }
            else if(name == "CONNECTIONS_COUNT") {
                console->addItem("Connections: " + value);
            }
            else {
                console->addItem("Unknown variable: " + name + "=" + value);
            }
        }
    }
}

void snap_manager::on_f_cassandraConnectButton_clicked()
{
    f_cassandraConnectButton->setEnabled( false );
    f_cassandraDisconnectButton->setEnabled( false );

    if(f_cassandra.isNull()) {
        f_cassandra = new QtCassandra::QCassandra;
    }

    // save the old values
    QString old_host(f_cassandra_host);
    int old_port(f_cassandra_port);

    // retrieve the current values
    QLineEdit *l = getChild<QLineEdit>(this, "cassandraHost");
    f_cassandra_host = l->text();
    if(f_cassandra_host.isEmpty()) {
        f_cassandra_host = "localhost";
    }
    l = getChild<QLineEdit>(this, "cassandraPort");
    if(l->text().isEmpty()) {
        f_cassandra_port = 9160;
    }
    else {
        f_cassandra_port = l->text().toInt();
    }

    // if old != new then disconnect
    if(f_cassandra_host == old_host && f_cassandra_port == old_port && f_cassandra->isConnected()) {
        // nothing changed, stay put
        f_cassandraConnectButton->setEnabled( true );
        return;
    }

    QListWidget *console = getChild<QListWidget>(this, "cassandraConsole");
    console->clear();
    console->addItem("libQtCassandra version: " + QString(f_cassandra->version()));
    console->addItem("Host: " + f_cassandra_host);
    console->addItem("Port: " + QString::number(f_cassandra_port));
    f_tabs->setTabEnabled(TAB_HOSTS, false);
    f_tabs->setTabEnabled(TAB_DOMAINS, false);
    f_tabs->setTabEnabled(TAB_WEBSITES, false);
    f_tabs->setTabEnabled(TAB_SITES, false);

    // reconnect with the new info
    // note: the disconnect does nothing if not already connected
    f_cassandra->disconnect();
    if(!f_cassandra->connect(f_cassandra_host, f_cassandra_port)) {
        // did not work...
        console->addItem("Not connected.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Cassandra", "Snap! Manager was not able to connect to your Cassandra Cluster. Please verify that it is up and running and accessible (no firewall) from this computer.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }

    // read and display the Cassandra information
    console->addItem("Cluster Name: " + f_cassandra->clusterName());
    console->addItem("Protocol Version: " + f_cassandra->protocolVersion());

    // read all the contexts so the findContext() works
    f_cassandra->contexts();
    QString context_name(snap::get_name(snap::SNAP_NAME_CONTEXT));
    f_context = f_cassandra->findContext(context_name);
    if(f_context.isNull())
    {
        // we connected to the database, but it is not properly initialized
        console->addItem("The \"" + context_name + "\" context is not defined.");
        QMessageBox msg(QMessageBox::Critical, "Connection to Cassandra", "Snap! Manager was able to connect to your Cassandra Cluster but it does not include a \"" + context_name + "\" context. The Snap! Server creates the necessary context and tables, have you run it?", QMessageBox::Ok, this);
        msg.exec();
        return;
    }

    // also check for the 2 main tables
    snap::name_t names[2] = { snap::SNAP_NAME_DOMAINS, snap::SNAP_NAME_WEBSITES /*, snap::SNAP_NAME_SITES*/ };
    for(int i = 0; i < 2; ++i)
    {
        QString table_name(snap::get_name(names[i]));
        QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
        if(table.isNull())
        {
            // we connected to the database, but it is not properly initialized
            console->addItem("The \"" + table_name + "\" table is not defined.");
            QMessageBox msg(QMessageBox::Critical, "Connection to Cassandra", "Snap! Manager was able to connect to your Cassandra Cluster but it does not include a \"" + table_name + "\" table. The Snap! Server creates the necessary context and tables, have you run it?", QMessageBox::Ok, this);
            msg.exec();
            return;
        }
    }

    // we could also check for the sites, content, and links tables

    // allow reseting indexes
    f_reset_domains_index->setEnabled(true);
    f_reset_websites_index->setEnabled(true);

    // TODO: call these functions when their respective tab is clicked instead!
    loadHosts();
    loadDomains();
    loadSites();

    // we just need to be connected for TAB_SITES
    f_tabs->setTabEnabled(TAB_SITES, true);

    f_cassandraDisconnectButton->setEnabled( true );
}

void snap_manager::on_f_cassandraDisconnectButton_clicked()
{
    f_cassandraConnectButton->setEnabled( false );
    f_cassandraDisconnectButton->setEnabled( false );

    // disconnect by deleting the object altogether
    delete f_cassandra;

    QListWidget *console = getChild<QListWidget>(this, "cassandraConsole");
    console->clear();
    console->addItem("libQtCassandra version: " + QString(f_cassandra->version()));
    console->addItem("Not connected.");

    f_reset_domains_index->setEnabled(false);
    f_reset_websites_index->setEnabled(false);

    f_tabs->setTabEnabled(TAB_HOSTS, false);
    f_tabs->setTabEnabled(TAB_DOMAINS, false);
    f_tabs->setTabEnabled(TAB_WEBSITES, false);
    f_tabs->setTabEnabled(TAB_SITES, false);

    // this doesn't get cleared otherwise
    f_host_list->clearSelection();
    f_host_filter_string->setText("");
    f_host_org_name = "";
    f_host_name->setText("");

    // this doesn't get cleared otherwise
    f_domain_list->clearSelection();
    f_domain_filter_string->setText("");
    f_domain_org_name = "";
    f_domain_name->setText("");
    f_domain_org_rules = "";
    f_domain_rules->setText("");

    // just in case, reset the sites widgets too
    f_sites_org_name = "";
    f_sites_name->setText("");
    f_sites_parameters->setEnabled(false);
    f_sites_parameter_name->setEnabled(false);
    f_sites_parameter_name->setText("");
    f_sites_parameter_value->setEnabled(false);
    f_sites_parameter_value->setText("");
    f_sites_parameter_type->setEnabled(false);
    f_sites_parameter_type->setCurrentIndex(1);
    f_sites_new->setEnabled(false);
    f_sites_save->setEnabled(false);
    f_sites_delete->setEnabled(false);

    f_cassandraConnectButton->setEnabled( true );
}

void snap_manager::reset_domains_index()
{
    // get the table and delete the index row if it exists
    QString domain_table_name(snap::get_name(snap::SNAP_NAME_DOMAINS));
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(domain_table_name));
    QString row_index_name(snap::get_name(snap::SNAP_NAME_INDEX)); // "*index*"
    if(table->exists(row_index_name)) {
        // if the index exists, drop it so we can restart from scratch
        table->dropRow(row_index_name);
    }

    // go through all the domain rows
    int count(0);
    QSharedPointer<QtCassandra::QCassandraColumnNamePredicate> column_predicate(new QtCassandra::QCassandraColumnNamePredicate);
    column_predicate->addColumnName("core::rules"); // get one column to avoid getting all!
    QtCassandra::QCassandraRowPredicate row_predicate;
    row_predicate.setColumnPredicate(column_predicate);
    for(;;) {
        table->clearCache();
        uint32_t max(table->readRows(row_predicate));
        if(max == 0) {
            break;
        }
        const QtCassandra::QCassandraRows& rows(table->rows());
        for(QtCassandra::QCassandraRows::const_iterator o(rows.begin());
                o != rows.end(); ++o)
        {
            // we do not care about the cells, what's important is the name
            // of the domain and of this row
            (*table)[row_index_name][o.key()] = QtCassandra::QCassandraValue();
            ++count;
        }
    }

    QMessageBox msg(QMessageBox::Information, "Reset Domains Index", QString("The domains index was reset with %1 entries.").arg(count), QMessageBox::Ok, this);
    msg.exec();
}

void snap_manager::reset_websites_index()
{
    QString domain_table_name(snap::get_name(snap::SNAP_NAME_DOMAINS));
    QSharedPointer<QtCassandra::QCassandraTable> domain_table(f_context->findTable(domain_table_name));

    // get the table and delete the index row if it exists
    QString table_name(snap::get_name(snap::SNAP_NAME_WEBSITES));
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
    QString row_index_name(snap::get_name(snap::SNAP_NAME_INDEX)); // "*index*"
    if(table->exists(row_index_name)) {
        // if the index exists, drop it so we can restart from scratch
        table->dropRow(row_index_name);
    }

    // go through all the website rows
    int count(0);
    QSharedPointer<QtCassandra::QCassandraColumnNamePredicate> column_predicate(new QtCassandra::QCassandraColumnNamePredicate);
    column_predicate->addColumnName("core::rules"); // get one column to avoid getting all!
    QtCassandra::QCassandraRowPredicate row_predicate;
    row_predicate.setColumnPredicate(column_predicate);
    for(;;) {
        table->clearCache();
        uint32_t max(table->readRows(row_predicate));
        if(max == 0) {
            break;
        }
        const QtCassandra::QCassandraRows& rows(table->rows());
        for(QtCassandra::QCassandraRows::const_iterator o(rows.begin());
                o != rows.end(); ++o) {
            // we do not care about the cells, what's important is the name
            // of the domain and of this row
            QByteArray row_key(o.key());
            QString website_name(QString::fromUtf8(row_key.data()));
            tld_result r;
            tld_info info;
            const char *d = row_key.data();
            r = tld(d, &info);
            if(r != TLD_RESULT_SUCCESS) {
                QMessageBox msg(QMessageBox::Critical, "Invalid TLD in Domain Name", "The TLD of this domain: \"" + website_name + "\" is not valid. This entry will be skipped.", QMessageBox::Ok, this);
                msg.exec();
                continue; // ignore entry
            }
            const char *domain = d; // by default assume no sub-domain
            for(; d < info.f_tld; ++d) {
                if(*d == '.') {
                    domain = d + 1;
                }
            }
            // check that the domain still exists, if not, offer the user
            // to delete that entry, it won't be used (or even accessible)
            if(!domain_table->exists(QString(domain))) {
                QMessageBox msg(QMessageBox::Critical, "Unknown Domain Name", "The domain for website: \"" + website_name + "\" is not defined. You won't be able to access this entry unless you create that domain. Should I delete that entry?", QMessageBox::Yes | QMessageBox::No, this);
                int choice(msg.exec());
                if(choice == QMessageBox::Yes) {
                    table->dropRow(row_key);
                    continue;
                }
            }
            (*table)[row_index_name][QString(domain) + "::" + website_name] = QtCassandra::QCassandraValue();
            ++count;
        }
    }

    QMessageBox msg(QMessageBox::Information, "Reset Websites Index", QString("The websites index was reset with %1 entries.").arg(count), QMessageBox::Ok, this);
    msg.exec();
}

void snap_manager::loadHosts()
{
    // we just checked to know whether the table existed so it cannot fail here
    // however the index table could be missing...
    f_host_list->clear();

    QString table_name(f_context->lockTableName());
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
    if( table.isNull() )
    {
        QString msg( tr("The table '%1' was not found in the current context. Are you sure the context is set up correctly?").arg(table_name) );
        QMessageBox::critical( this, tr("Error!"), msg );
        return;
    }

    QSharedPointer<QtCassandra::QCassandraRow> row(table->row(f_context->lockHostsKey()));

    QtCassandra::QCassandraColumnRangePredicate hosts_predicate;
    QString filter(f_host_filter_string->text());
    if(filter.length() != 0)
    {
        // assign the filter only if not empty
        hosts_predicate.setStartColumnName(filter);
        hosts_predicate.setEndColumnName(filter + QtCassandra::QCassandraColumnPredicate::last_char);
    }
    row->clearCache(); // remove any previous load results
    row->readCells(hosts_predicate);

    // now we have a list of rows to read as defined in row->Cells()
    const QtCassandra::QCassandraCells& row_keys(row->cells());

    for(QtCassandra::QCassandraCells::const_iterator it(row_keys.begin());
        it != row_keys.end();
        ++it)
    {
        // the cell key is actually the row name which is the host name
        // which is exactly what we want to display in our list!
        f_host_list->addItem(it.key());
    }

    // at first some of the entries are disabled
    // until a select is made or New is clicked
    f_host_name->setEnabled(false);
    f_host_org_name = ""; // not editing, this is new
    f_host_name->setText("");
    f_host_save->setEnabled(false);
    f_host_cancel->setEnabled(false);
    f_host_delete->setEnabled(false);

    // allow user to go to that tab
    f_tabs->setTabEnabled(TAB_HOSTS, true);
}

void snap_manager::on_hostList_itemClicked(QListWidgetItem *item)
{
    // same host? if so, skip on it
    if(f_host_org_name == item->text() && !f_host_org_name.isEmpty())
    {
        return;
    }

    // check whether the current info was modified
    if(!hostChanged())
    {
        // user canceled his action
        // TODO: we need to reset the item selection...
        QList<QListWidgetItem *> items(f_host_list->findItems(f_host_org_name, Qt::MatchExactly));
        if(items.count() > 0)
        {
            f_host_list->setCurrentItem(items[0]);
        }
        else
        {
            f_host_list->clearSelection();
        }
        return;
    }

    f_host_org_name = item->text();
    f_host_name->setText(f_host_org_name);

    hostWithSelection();
}

void snap_manager::on_hostNew_clicked()
{
    // check whether the current info was modified
    if(!hostChanged())
    {
        // user canceled his action
        return;
    }

    f_host_list->clearSelection();

    f_host_org_name = ""; // not editing, this is new
    f_host_name->setText("");

    hostWithSelection();
    f_host_delete->setEnabled(false);
}

void snap_manager::on_hostSave_clicked()
{
    QString name(f_host_name->text());
    if(name.isEmpty())
    {
        QMessageBox msg(QMessageBox::Critical, "Name Missing", "You cannot create a new host entry without giving the host a valid name.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    if(name != f_host_org_name)
    {
        // make sure the host name is correct (i.e. [a-zA-Z0-9_]+)
        const int max(name.length());
        for(int i(0); i < max; ++i)
        {
            int c(name[i].unicode());
            if((c < 'a' || c > 'z')
            && (c < 'A' || c > 'Z')
            && (c < '0' || c > '9' || i == 0) // cannot start with a digit
            && c != '_')
            {
                QMessageBox msg(QMessageBox::Critical, "Invalid Host Name", "The host name must only be composed of letters, digits, and underscores although it cannot start with a digit ([0-9a-zA-Z_]+)", QMessageBox::Ok, this);
                msg.exec();
                return;
            }
        }

        // host name is considered valid for now
        f_context->addLockHost(name);

        // the data is now in the database, add it to the table too
        if(f_host_org_name == "")
        {
            f_host_list->addItem(name);

            // make sure we select that item too
            QList<QListWidgetItem *> items(f_host_list->findItems(name, Qt::MatchExactly));
            if(items.count() > 0)
            {
                f_host_list->setCurrentItem(items[0]);
            }
        }

        f_host_org_name = name;

        hostWithSelection();
    }
}

void snap_manager::on_hostCancel_clicked()
{
    // check whether the current info was modified
    if(!hostChanged())
    {
        // user canceled his action
        return;
    }

    // restore the original values
    f_host_name->setText(f_host_org_name);

    if(f_host_org_name.length() == 0)
    {
        // if we had nothing selected, reset everything
        f_host_name->setEnabled(false);
        f_host_save->setEnabled(false);
        f_host_cancel->setEnabled(false);
        f_host_delete->setEnabled(false);
    }
}

void snap_manager::on_hostDelete_clicked()
{
    QString name(f_host_name->text());

    // verify that the user really wants to delete this host
    QMessageBox msg(QMessageBox::Critical, "Delete Host", "<font color=\"red\"><b>WARNING:</b></font> You are about to delete host \"" + name + "\". Are you absolutely sure you want to do that?", QMessageBox::Ok | QMessageBox::Cancel, this);
    int choice = msg.exec();
    if(choice != QMessageBox::Ok)
    {
        return;
    }

    f_context->removeLockHost(name);

    delete f_host_list->currentItem();

    f_host_list->clearSelection();

    // mark empty
    f_host_org_name = "";
    f_host_name->setText("");

    // in effect we just lost our selection
    f_host_name->setEnabled(false);
    f_host_save->setEnabled(false);
    f_host_cancel->setEnabled(false);
    f_host_delete->setEnabled(false);
}

void snap_manager::hostWithSelection()
{
    // now there is a selection, everything is enabled
    f_host_name->setEnabled(true);
    f_host_save->setEnabled(true);
    f_host_cancel->setEnabled(true);
    f_host_delete->setEnabled(true);
}

bool snap_manager::hostChanged()
{
    // if something changed we want to warn the user before going further
    if(f_host_org_name != f_host_name->text())
    {
        QMessageBox msg(QMessageBox::Critical, "Host Name Modified", "You made changes to this entry and did not Save it yet. Do you really want to continue? If you click Ok you will lose your changes.", QMessageBox::Ok | QMessageBox::Cancel, this);
        int choice = msg.exec();
        if(choice != QMessageBox::Ok)
        {
            return false;
        }
    }

    return true;
}

void snap_manager::on_hostFilter_clicked()
{
    // make sure the user did not change something first
    if(hostChanged())
    {
        // user is okay with losing changes or did not make any
        // the following applies the filter (Apply button)
        loadHosts();
    }
}

void snap_manager::loadDomains()
{
    // we just checked to know whether the table existed so it cannot fail here
    // however the index table could be missing...
    f_domain_list->clear();

    QString table_name(snap::get_name(snap::SNAP_NAME_DOMAINS));
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
    if( table.isNull() )
    {
        QString msg( tr("The table '%1' was not found in the current context. Are you sure the context is set up correctly?").arg(table_name) );
        QMessageBox::critical( this, tr("Error!"), msg );
        return;
    }

    QString row_index_name(snap::get_name(snap::SNAP_NAME_INDEX)); // "*index*"
    if(!table->exists(row_index_name))
    {
        // if the index doesn't exist, no rows were ever saved anyway,
        // so that's it!
        //return; -- if we're connected we need to run the whole thing anyway
    }
    QSharedPointer<QtCassandra::QCassandraRow> row(table->row(row_index_name));

    QtCassandra::QCassandraColumnRangePredicate domain_predicate;
    QString filter(f_domain_filter_string->text());
    if(filter.length() != 0)
    {
        // assign the filter only if not empty
        domain_predicate.setStartColumnName(filter);
        domain_predicate.setEndColumnName(filter + QtCassandra::QCassandraColumnPredicate::last_char);
    }
    row->clearCache(); // remove any previous load results
    row->readCells(domain_predicate);

    // now we have a list of rows to read as defined in row->Cells()
    const QtCassandra::QCassandraCells& row_keys(row->cells());

    for(QtCassandra::QCassandraCells::const_iterator it(row_keys.begin());
        it != row_keys.end();
        ++it)
    {
        // the cell key is actually the row name which is the domain name
        // which is exactly what we want to display in our list!
        f_domain_list->addItem(it.key());
    }

    // at first some of the entries are disabled
    // until a select is made or New is clicked
    f_domain_name->setEnabled(false);
    f_domain_org_name = ""; // not editing, this is new
    f_domain_name->setText("");
    f_domain_rules->setEnabled(false);
    f_domain_org_rules = "";
    f_domain_rules->setText("");
    f_domain_save->setEnabled(false);
    f_domain_cancel->setEnabled(false);
    f_domain_delete->setEnabled(false);

    // allow user to go to that tab
    f_tabs->setTabEnabled(TAB_DOMAINS, true);
    f_tabs->setTabEnabled(TAB_WEBSITES, false); // we lose focus so we want to reset that one
}

void snap_manager::domainWithSelection()
{
    // now there is a selection, everything is enabled
    f_domain_name->setEnabled(true);
    f_domain_rules->setEnabled(true);
    f_domain_save->setEnabled(true);
    f_domain_cancel->setEnabled(true);
    f_domain_delete->setEnabled(true);

    // this is "complicated" since we will have to use the
    // f_domain_org_name until the user saves since the name
    // may change in between...
    bool enable_websites = f_domain_org_name != "";
    f_tabs->setTabEnabled(TAB_WEBSITES, enable_websites);
    if(enable_websites) {
        // TODO: call that function when the tab is clicked instead!
        loadWebsites();
    }
}

bool snap_manager::domainChanged()
{
    // if something changed we want to warn the user before going further
    if(f_domain_org_name != f_domain_name->text()
    || f_domain_org_rules != f_domain_rules->toPlainText())
    {
        QMessageBox msg(QMessageBox::Critical, "Domain Modified", "You made changes to this entry and did not Save it yet. Do you really want to continue? If you click Ok you will lose your changes.", QMessageBox::Ok | QMessageBox::Cancel, this);
        int choice = msg.exec();
        if(choice != QMessageBox::Ok)
        {
            return false;
        }
    }

    return true;
}

void snap_manager::on_domainFilter_clicked()
{
    // make sure the user did not change something first
    if(domainChanged())
    {
        // user is okay with losing changes or did not make any
        // the following applies the filter (Apply button)
        loadDomains();
    }
}

void snap_manager::on_domainList_itemClicked(QListWidgetItem *item)
{
    // same domain? if so, skip on it
    if(f_domain_org_name == item->text() && !f_domain_org_name.isEmpty())
    {
        return;
    }

    // check whether the current info was modified
    if(!domainChanged())
    {
        // user canceled his action
        // TODO: we need to reset the item selection...
        QList<QListWidgetItem *> items(f_domain_list->findItems(f_domain_org_name, Qt::MatchExactly));
        if(items.count() > 0)
        {
            f_domain_list->setCurrentItem(items[0]);
        }
        else
        {
            f_domain_list->clearSelection();
        }
        return;
    }

    f_domain_org_name = item->text();
    f_domain_name->setText(f_domain_org_name);

    // IMPORTANT: note that f_domain_org_name changed to the item->text() value
    QString table_name(snap::get_name(snap::SNAP_NAME_DOMAINS));
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
    QSharedPointer<QtCassandra::QCassandraRow> row(table->row(f_domain_org_name));
    if(row->exists(QString("core::original_rules")))
    {
        QtCassandra::QCassandraValue rules((*table)[f_domain_org_name][QString("core::original_rules")]);
        f_domain_org_rules = rules.stringValue();
    }
    else
    {
        // this case happens after a delete (i.e. the row still exist but is empty)
        f_domain_org_rules = "";
    }
    f_domain_rules->setText(f_domain_org_rules);

    domainWithSelection();
}

void snap_manager::on_domainNew_clicked()
{
    // check whether the current info was modified
    if(!domainChanged())
    {
        // user canceled his action
        return;
    }

    f_domain_list->clearSelection();

    f_domain_org_name = ""; // not editing, this is new
    f_domain_name->setText("");
    f_domain_org_rules = "";
    f_domain_rules->setText("");

    domainWithSelection();
    f_domain_delete->setEnabled(false);
}

void snap_manager::on_domainSave_clicked()
{
    QString name(f_domain_name->text());
    if(name.isEmpty())
    {
        QMessageBox msg(QMessageBox::Critical, "Name Missing", "You cannot create a new domain entry without giving the domain a valid name.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    QString rules(f_domain_rules->toPlainText());
    if(rules.isEmpty())
    {
        QMessageBox msg(QMessageBox::Critical, "Rules Missing", "Adding a domain requires you to enter at least one rule.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    if(name != f_domain_org_name || rules != f_domain_org_rules)
    {
        // make sure the domain name is correct (i.e. domain + TLD)
        tld_result r;
        tld_info info;
        // save in temporary buffer otherwise we'd lose the string pointers
        // in the tld_info structure
        QByteArray str(name.toUtf8());
        const char *d = str.data();
        r = tld(d, &info);
        if(r != TLD_RESULT_SUCCESS)
        {
            QMessageBox msg(QMessageBox::Critical, "Invalid TLD in Domain Name", "The TLD must be a known TLD. The tld() function could not determine the TLD of this domain name. Please check the domain name and make the necessary adjustments.", QMessageBox::Ok, this);
            msg.exec();
            return;
        }
        // TODO: accept a period at the beginning (although we want to remove it)
        //       so .snapwebsites.org would become snapwebsites.org
        for(; d < info.f_tld; ++d)
        {
            if(*d == '.')
            {
                QMessageBox msg(QMessageBox::Critical, "Invalid sub-domain in Domain Name", "Your domain name cannot include any sub-domain names. Instead, the rules determine how the sub-domains are used and the attached websites.", QMessageBox::Ok, this);
                msg.exec();
                return;
            }
        }

        // domain name is considered valid for now
        // check the rules
        snap::snap_uri_rules domain_rules;
        QByteArray compiled_rules;
        if(!domain_rules.parse_domain_rules(rules, compiled_rules))
        {
            QMessageBox msg(QMessageBox::Critical, "Invalid Domain Rules", "An error was detected in your domain rules: " + domain_rules.errmsg(), QMessageBox::Ok, this);
            msg.exec();
            return;
        }

        QString table_name(snap::get_name(snap::SNAP_NAME_DOMAINS));
        QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));

        if(name != f_domain_org_name)
        {
            // user is creating a new entry, so we want to prevent
            // overwriting an existing entry
            if(table->exists(name))
            {
                // got the row, check whether the "core::original_rules" exists
                QSharedPointer<QtCassandra::QCassandraRow> row(table->row(name));
                if(row->exists(QString("core::original_rules")))
                {
                    if(f_domain_org_name.isEmpty())
                    {
                        QMessageBox msg(QMessageBox::Critical, "Domain Name already defined", "You asked to create a new Domain Name and yet you specified a Domain Name that is already defined in the database. Please change the Domain Name or Cancel and then edit the existing name.", QMessageBox::Ok, this);
                        msg.exec();
                    }
                    else
                    {
                        QMessageBox msg(QMessageBox::Critical, "Domain Name already defined", "You asked to create a new Domain Name and yet you specified a Domain Name that is already defined in the database. Please change the Domain Name or Cancel and then edit the existing name.", QMessageBox::Ok, this);
                        msg.exec();
                    }
                    return;
                }
            }
        }

        // save in the index
        QString row_index_name(snap::get_name(snap::SNAP_NAME_INDEX)); // "*index*"
        (*table)[row_index_name][name] = QtCassandra::QCassandraValue();

        // it worked
        (*table)[name][QString("core::original_rules")] = QtCassandra::QCassandraValue(rules);
        (*table)[name][QString("core::rules")] = QtCassandra::QCassandraValue(compiled_rules);

        // the data is now in the database, add it to the table too
        if(f_domain_org_name == "")
        {
            f_domain_list->addItem(name);

            // make sure we select that item too
            QList<QListWidgetItem *> items(f_domain_list->findItems(name, Qt::MatchExactly));
            if(items.count() > 0)
            {
                f_domain_list->setCurrentItem(items[0]);
            }
        }

        f_domain_org_name = name;
        f_domain_org_rules = rules;

        domainWithSelection();
    }
}

void snap_manager::on_domainCancel_clicked()
{
    // check whether the current info was modified
    if(!domainChanged())
    {
        // user canceled his action
        return;
    }

    // restore the original values
    f_domain_name->setText(f_domain_org_name);
    f_domain_rules->setText(f_domain_org_rules);

    if(f_domain_org_name.length() == 0)
    {
        // if we had nothing selected, reset everything
        f_domain_name->setEnabled(false);
        f_domain_rules->setEnabled(false);
        f_domain_save->setEnabled(false);
        f_domain_cancel->setEnabled(false);
        f_domain_delete->setEnabled(false);
    }
}

void snap_manager::on_domainDelete_clicked()
{
    QString name(f_domain_name->text());

    // verify that the user really wants to delete this domain
    QMessageBox msg(QMessageBox::Critical, "Delete Domain", "<font color=\"red\"><b>WARNING:</b></font> You are about to delete domain \"" + name + "\" and ALL of its websites definitions. Are you absolutely sure you want to do that?", QMessageBox::Ok | QMessageBox::Cancel, this);
    int choice = msg.exec();
    if(choice != QMessageBox::Ok)
    {
        return;
    }

    QString row_index_name(snap::get_name(snap::SNAP_NAME_INDEX)); // "*index*"

    // delete all the sub-domains
    {
        QString table_name(snap::get_name(snap::SNAP_NAME_WEBSITES));
        QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
        if(table->exists(row_index_name))
        {
            QSharedPointer<QtCassandra::QCassandraRow> row(table->row(row_index_name));

            // Use a predicate to limit the list to the websites defined for that
            // domain the start is the domain followed by :: (m2osw.com::) and
            // the end is defined such as it encompasses all the possible websites
            // for that domain (m2osw.com:;).
            // Note that we're using our index row to read those entries because we do
            // not force a sort on row keys.
            QtCassandra::QCassandraColumnRangePredicate domain_predicate;
            domain_predicate.setStartColumnName(name + "::");
            domain_predicate.setEndColumnName(name + ":;"); // ';' > ':'
            row->clearCache(); // remove any previous load results
            row->readCells(domain_predicate);

            // now we have a list of rows to read as defined in row->Cells()

            int mid_pos(name.length() + 2);
            const QtCassandra::QCassandraCells& row_keys(row->cells());
            for(;;)
            {
                // because we do a delete, we have to check the cells
                // reference on each iteration (we cannot use an iterator)
                if(row_keys.isEmpty())
                {
                    break;
                }

                // drop all of those (in case there were errors, those should
                // all get erased)
                QString website_name(row_keys.begin().key());
                row->dropCell(website_name);
                if(website_name.length() > mid_pos)
                {
                    table->dropRow(website_name.mid(mid_pos));
                }
            }
        }
    }

    // remove from the list of domains
    {
        QString table_name(snap::get_name(snap::SNAP_NAME_DOMAINS));
        QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
        table->dropRow(name);

        // remove from the index too
        QSharedPointer<QtCassandra::QCassandraRow> row(table->findRow(row_index_name));
        if(row)
        {
            row->dropCell(f_domain_name->text(), QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
        }
    }

    delete f_domain_list->currentItem();

    f_domain_list->clearSelection();

    // mark empty
    f_domain_org_name = "";
    f_domain_name->setText("");
    f_domain_org_rules = "";
    f_domain_rules->setText("");

    // in effect we just lost our selection
    f_domain_name->setEnabled(false);
    f_domain_rules->setEnabled(false);
    f_domain_save->setEnabled(false);
    f_domain_cancel->setEnabled(false);
    f_domain_delete->setEnabled(false);

    f_tabs->setTabEnabled(TAB_WEBSITES, false);
}

void snap_manager::loadWebsites()
{
    // we just checked to know whether the table existed so it cannot fail here
    f_website_list->clear();
    QString table_name(snap::get_name(snap::SNAP_NAME_WEBSITES));
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
    QString row_index_name(snap::get_name(snap::SNAP_NAME_INDEX)); // "*index*"
    if(!table->exists(row_index_name))
    {
        // if the index doesn't exist, no rows were ever saved anyway,
        // so that's it!
        return;
    }
    QSharedPointer<QtCassandra::QCassandraRow> row(table->row(row_index_name));

    // Use a predicate to limit the list to the websites defined for that domain
    // the start is the plain domain (m2osw.com) and the end is defined such as
    // it encompasses all the possible domain names (.m2osw.com).
    // Note that we're using our index row to read those entries because we do
    // not force a sort on row keys.
    QtCassandra::QCassandraColumnRangePredicate domain_predicate;
    domain_predicate.setStartColumnName(f_domain_org_name + "::");
    domain_predicate.setEndColumnName(f_domain_org_name + ":;"); // ';' > ':'
    row->clearCache(); // remove any previous load results
    row->readCells(domain_predicate);

    // now we have a list of rows to read as defined in row->Cells()
    const QtCassandra::QCassandraCells& row_keys(row->cells());

    int mid_pos(f_domain_org_name.length() + 2);
    for(QtCassandra::QCassandraCells::const_iterator it(row_keys.begin());
        it != row_keys.end();
        ++it)
    {
        // the cell key is actually the row name which is the domain name
        // which is exactly what we want to display in our list!
        // (although it starts with the domain name and a double colon that
        // we want to remove)
        if(it.key().length() <= mid_pos)
        {
            // note that the length of the key is at least 4 additional
            // characters but at this point we don't make sure that the
            // key itself is fully correct (it should be)
            QMessageBox msg(QMessageBox::Warning, "Invalid Website Index", "Somehow we have found an invalid entry in the list of websites. It is suggested that you regenerate the index. Note that this index is not used by the Snap server itself.", QMessageBox::Ok, this);
            continue;
        }
        f_website_list->addItem(it.key().mid(mid_pos));
    }

    // at first some of the entries are disabled
    // until a select is made or New is clicked
    f_website_name->setEnabled(false);
    f_website_rules->setEnabled(false);
    f_website_save->setEnabled(false);
    f_website_cancel->setEnabled(false);
    f_website_delete->setEnabled(false);

    f_website_org_name = "";
    f_website_org_rules = "";
    f_website_name->setText("");
    f_website_rules->setText("");
}

void snap_manager::websiteWithSelection()
{
    // now there is a selection, everything is enabled
    f_website_name->setEnabled(true);
    f_website_rules->setEnabled(true);
    f_website_save->setEnabled(true);
    f_website_cancel->setEnabled(true);
    f_website_delete->setEnabled(true);
}

bool snap_manager::websiteChanged()
{
    // if something changed we want to warn the user before going further
    if(f_website_org_name != f_website_name->text()
    || f_website_org_rules != f_website_rules->toPlainText())
    {
        QMessageBox msg(QMessageBox::Critical, "Website Modified", "You made changes to this entry and did not Save it yet. Do you really want to continue? If you click Ok you will lose your changes.", QMessageBox::Ok | QMessageBox::Cancel, this);
        int choice = msg.exec();
        if(choice != QMessageBox::Ok)
        {
            return false;
        }
    }

    return true;
}

void snap_manager::on_websiteList_itemClicked(QListWidgetItem *item)
{
    // check whether the current info was modified
    if(!websiteChanged()) {
        // user canceled his action
        // TODO: we need to reset the item selection...
        return;
    }

    f_website_org_name = item->text();
    f_website_name->setText(f_website_org_name);

    // IMPORTANT: note that f_website_org_name changed to the item->text() value
    QString table_name(snap::get_name(snap::SNAP_NAME_WEBSITES));
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
    QSharedPointer<QtCassandra::QCassandraRow> row(table->row(f_website_org_name));
    if(row->exists(QString("core::original_rules"))) {
        QtCassandra::QCassandraValue rules((*table)[f_website_org_name][QString("core::original_rules")]);
        f_website_org_rules = rules.stringValue();
    }
    else {
        // this case happens after a delete (i.e. the row still exist but is empty)
        f_website_org_rules = "";
    }
    f_website_rules->setText(f_website_org_rules);

    websiteWithSelection();
}

void snap_manager::on_websiteNew_clicked()
{
    // check whether the current info was modified
    if(!websiteChanged()) {
        // user canceled his action
        return;
    }

    f_website_list->clearSelection();

    f_website_org_name = ""; // not editing, this is new
    f_website_name->setText("");
    f_website_org_rules = "";
    f_website_rules->setText("");

    websiteWithSelection();
    f_website_delete->setEnabled(false);
}

void snap_manager::on_websiteSave_clicked()
{
    QString name(f_website_name->text());
    if(name.isEmpty())
    {
        QMessageBox msg(QMessageBox::Critical, "Name Missing", "You cannot create a new website entry without giving the website a valid name.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    QString rules(f_website_rules->toPlainText());
    if(rules.isEmpty())
    {
        QMessageBox msg(QMessageBox::Critical, "Rules Missing", "Adding a website requires you to enter at least one rule.", QMessageBox::Ok, this);
        msg.exec();
        return;
    }
    if(name != f_website_org_name || rules != f_website_org_rules)
    {
        // first make sure the domain name corresponds to the domain
        // being edited; it is important for the following reasons:
        //
        // 1) we use that in the website index for this entry
        //
        // 2) the user could not find his website otherwise (plus it may
        //    not correspond to any other domain and would not make it
        //    in the right index)
        //
        bool valid(false);
        if(name.length() > f_domain_org_name.length()) {
            QString domain(name.mid(name.length() - 1 - f_domain_org_name.length()));
            if(domain == "." + f_domain_org_name) {
                valid = true;
            }
        }
        if(!valid) {
            QMessageBox msg(QMessageBox::Critical, "Invalid Domain Name", "The full domain name of a website must end with the exact domain name of the website you are editing.", QMessageBox::Ok, this);
            msg.exec();
            return;
        }

        // make sure the domain name is correct (i.e. at least "domain + TLD")
        tld_result r;
        tld_info info;
        // save in temporary buffer otherwise we'd lose the string pointers
        // in the tld_info structure
        QByteArray str(name.toUtf8());
        const char *d = str.data();
        r = tld(d, &info);
        if(r != TLD_RESULT_SUCCESS) {
            QMessageBox msg(QMessageBox::Critical, "Invalid TLD in Full Domain Name", "The TLD must be a known TLD. The tld() function could not determine the TLD of this full domain name. Please check the full domain name and make the necessary adjustments.", QMessageBox::Ok, this);
            msg.exec();
            return;
        }

        // full domain name is considered valid for now
        snap::snap_uri_rules website_rules;
        QByteArray compiled_rules;
        if(!website_rules.parse_website_rules(rules, compiled_rules)) {
            QMessageBox msg(QMessageBox::Critical, "Invalid Website Rules", "An error was detected in your website rules: " + website_rules.errmsg(), QMessageBox::Ok, this);
            msg.exec();
            return;
        }

        QString table_name(snap::get_name(snap::SNAP_NAME_WEBSITES));
        QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));

        if(name != f_website_org_name) {
            // user is creating a new entry or changing the name of an existing
            // entry, so we want to prevent overwriting an existing entry
            if(table->exists(name)) {
                // got the row, check whether the "core::original_rules" exists
                QSharedPointer<QtCassandra::QCassandraRow> row(table->row(name));
                if(row->exists(QString("core::original_rules"))) {
                    if(f_website_org_name.isEmpty()) {
                        QMessageBox msg(QMessageBox::Critical, "Full Domain Name already defined", "You asked to create a new Full Domain Name and yet you specified a Full Domain Name that is already defined in the database. Please change the Full Domain Name or Cancel and then edit the existing website entry.", QMessageBox::Ok, this);
                        msg.exec();
                    }
                    else {
                        QMessageBox msg(QMessageBox::Critical, "Full Domain Name already defined", "You attempted to rename a Full Domain Name and yet you specified a Full Domain Name that is already defined in the database. Please change the Full Domain Name or Cancel and then edit the existing website entry.", QMessageBox::Ok, this);
                        msg.exec();
                    }
                    return;
                }
            }
        }

        // add that one in the index
        QString row_index_name(snap::get_name(snap::SNAP_NAME_INDEX)); // "*index*"
        (*table)[row_index_name][f_domain_org_name + "::" + name] = QtCassandra::QCassandraValue();

        // it worked, save the results
        (*table)[name][QString("core::original_rules")] = QtCassandra::QCassandraValue(rules);
        (*table)[name][QString("core::rules")] = QtCassandra::QCassandraValue(compiled_rules);

        // the data is now in the database, add it to the table too
        if(f_website_org_name == "") {
            f_website_list->addItem(name);

            // make sure we select that item too
            QList<QListWidgetItem *> items(f_website_list->findItems(name, Qt::MatchExactly));
            if(items.count() > 0)
            {
                f_website_list->setCurrentItem(items[0]);
            }
        }

        f_website_org_name = name;
        f_website_org_rules = rules;

        // now the delete is available
        f_website_delete->setEnabled(true);
    }
}

void snap_manager::on_websiteCancel_clicked()
{
    // check whether the current info was modified
    if(!websiteChanged()) {
        // user canceled his action
        return;
    }

    // restore the original values
    f_website_name->setText(f_website_org_name);
    f_website_rules->setText(f_website_org_rules);
}

void snap_manager::on_websiteDelete_clicked()
{
    QString name(f_website_name->text());

    // verify that the user really wants to delete this website
    QMessageBox msg(QMessageBox::Critical, "Delete Website", "<font color=\"red\"><b>WARNING:</b></font> You are about to delete website \"" + name + "\". Are you sure you want to do that?", QMessageBox::Ok | QMessageBox::Cancel, this);
    int choice = msg.exec();
    if(choice != QMessageBox::Ok)
    {
        return;
    }

    QString table_name(snap::get_name(snap::SNAP_NAME_WEBSITES));
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
    table->dropRow(name);

    QString row_index_name(snap::get_name(snap::SNAP_NAME_INDEX)); // "*index*"
    QSharedPointer<QtCassandra::QCassandraRow> row(table->findRow(row_index_name));
    if(row)
    {
        row->dropCell(f_domain_org_name + "::" + name);
    }

    delete f_website_list->currentItem();

    // all those are not valid anymore
    f_website_name->setEnabled(false);
    f_website_rules->setEnabled(false);
    f_website_save->setEnabled(false);
    f_website_cancel->setEnabled(false);
    f_website_delete->setEnabled(false);

    // mark empty
    f_website_org_name = "";
    f_website_org_rules = "";
    f_website_name->setText("");
    f_website_rules->setText("");
}

bool snap_manager::sitesChanged()
{
#if 0
    // TODO: this always fails, so we need to fix this problem!
    // f_sites_org_parameter_* are never set.
    //
    // if something changed we want to warn the user before going further
    if(f_sites_org_parameter_name != f_sites_parameter_name->text()
    || f_sites_org_parameter_value != f_sites_parameter_value->text()
    || f_sites_org_parameter_type != f_sites_parameter_type->currentIndex())
    {
        QMessageBox msg(QMessageBox::Critical, "Site Parameter Modified", "You made changes to this parameter and did not Save it yet. Do you really want to continue? If you click Ok you will lose your changes.", QMessageBox::Ok | QMessageBox::Cancel, this);
        int choice = msg.exec();
        if(choice != QMessageBox::Ok)
        {
            return false;
        }
    }
#endif

    return true;
}

void snap_manager::loadSites()
{
    // we just checked to know whether the table existed so it cannot fail here
    // however the index table could be missing...
    f_sites_list->clear();

    // TBD: we would need to have an "*index*" so we can cleanly search for
    //      the list of sites; so at this point we ignore the filter info
    //QString row_index_name(snap::get_name(snap::SNAP_NAME_INDEX)); // "*index*"

    QString table_name(snap::get_name(snap::SNAP_NAME_SITES));
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
    if(!table.isNull())
    {
        // if the table does not exist yet skip this part!
        // this is possible until you access an actual website; although we
        // will change the behavior at some point it is still that way now

        // without a filter the rows will be disorganized, although until you
        // have more than 100 it should look good
        QtCassandra::QCassandraRowPredicate row_predicate;
        table->clearCache();
        table->readRows(row_predicate);
        const QtCassandra::QCassandraRows& rows(table->rows());

        for(QtCassandra::QCassandraRows::const_iterator it(rows.begin());
            it != rows.end();
            ++it)
        {
            // the row key is actually the name of the concern domain
            f_sites_list->addItem(it.key());
        }
    }

    // at first some of the entries are disabled
    // until a select is made or New is clicked
    f_sites_name->setText("");
    f_sites_parameters->setEnabled(false);
    f_sites_parameter_name->setEnabled(false);
    f_sites_parameter_name->setText("");
    f_sites_parameter_value->setEnabled(false);
    f_sites_parameter_value->setText("");
    f_sites_parameter_type->setEnabled(false);
    f_sites_parameter_type->setCurrentIndex(1);
    f_sites_new->setEnabled(false);
    f_sites_save->setEnabled(false);
    f_sites_delete->setEnabled(false);
}

void snap_manager::on_sitesFilter_clicked()
{
    // make sure the user did not change something first
    if(sitesChanged())
    {
        // warning about the fact that the filter is currently ignored
        if(!f_sites_filter_string->text().isEmpty())
        {
            QMessageBox msg(QMessageBox::Critical, "Internal Error", "WARNING: The *index* for the sites table was not yet defined. The filter will therefore be ignored.", QMessageBox::Ok, this);
            msg.exec();
        }

        // user is okay with losing changes or did not make any
        // the following applies the filter (Apply button)
        loadSites();
    }
}

void snap_manager::on_sitesList_itemClicked(QListWidgetItem *item)
{
    // same site? if so, skip on it
    if(f_sites_org_name == item->text() && !f_sites_org_name.isEmpty())
    {
        return;
    }

    // check whether the current info was modified
    if(!sitesChanged())
    {
        // user canceled his action
        // TODO: we need to reset the item selection...
        QList<QListWidgetItem *> items(f_sites_list->findItems(f_sites_org_name, Qt::MatchExactly));
        if(items.count() > 0)
        {
            f_sites_list->setCurrentItem(items[0]);
        }
        return;
    }

    f_sites_org_name = item->text();
    f_sites_name->setText(f_sites_org_name);
    f_sites_parameters->clearContents();

    // IMPORTANT: note that f_sites_org_name changed to the item->text() value
    QString table_name(snap::get_name(snap::SNAP_NAME_SITES));
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
    QSharedPointer<QtCassandra::QCassandraRow> row(table->row(f_sites_org_name));
    QtCassandra::QCassandraColumnRangePredicate parameters_predicate;
    parameters_predicate.setCount(1000); // that should be sufficient for 99% of the websites out there
    row->clearCache();
    uint32_t count(row->readCells(parameters_predicate));
    f_sites_parameters->setRowCount(count);
    const QtCassandra::QCassandraCells& parameters(row->cells());
    int row_pos(0);
    for(QtCassandra::QCassandraCells::const_iterator c(parameters.begin());
            c != parameters.end(); ++c, ++row_pos)
    {
        QTableWidgetItem *param_name(new QTableWidgetItem(QString(c.key())));
        f_sites_parameters->setItem(row_pos, 0, param_name);
        // TODO: value needs to be typed...
        QTableWidgetItem *param_value(new QTableWidgetItem(c.value()->value().stringValue()));
        f_sites_parameters->setItem(row_pos, 1, param_value);
    }

    f_sites_parameters->setEnabled(true);
}

void snap_manager::closeEvent(QCloseEvent *close_event)
{
    if(!domainChanged())
    {
        close_event->ignore();
        return;
    }
    if(!websiteChanged())
    {
        close_event->ignore();
        return;
    }
    if(!sitesChanged())
    {
        close_event->ignore();
        return;
    }

    close_event->accept();
}

void snap_manager::quit()
{
    if(!domainChanged())
    {
        return;
    }
    if(!websiteChanged())
    {
        return;
    }
    if(!sitesChanged())
    {
        return;
    }
    exit(0);
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName   ( "snap-manager"              );
    app.setApplicationVersion( SNAPWEBSITES_VERSION_STRING );
    app.setOrganizationDomain( "snapwebsites.org"          );
    app.setOrganizationName  ( "M2OSW"                     );

    snap_manager win;
    win.show();

    return app.exec();
}

// vim: ts=4 sw=4 et
