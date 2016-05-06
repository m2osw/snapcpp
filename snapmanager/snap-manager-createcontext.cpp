// Snap Manager -- snap database manager Initialize Website dialog
// Copyright (C) 2011-2016  Made to Order Software Corp.
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

#include "snap-manager-createcontext.h"

#include <QSettings>

#include <stdio.h>

using namespace QtCassandra;

snap_manager_createcontext::snap_manager_createcontext
        ( QWidget *                     snap_parent
        , QCassandraSession::pointer_t	session
        )
    : QDialog(snap_parent)
    , f_session(session)
    //, f_query()               -- initialized below
    //, f_close_button()        -- initialized below
    //, f_send_request_button() -- initialized below
    //, f_snap_server_host()    -- initialized below
    //, f_snap_server_port()    -- initialized below
    //, f_website_url()         -- initialized below
    //, f_port()                -- initialized below
    //, f_initialize_website()  -- auto-init
    //, f_timer_id(0)           -- auto-init
{
    setWindowModality(Qt::ApplicationModal);
    setupUi(this);

    QSettings settings( this );
    //restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );
    //restoreState   ( settings.value( "state"   , saveState()    ).toByteArray() );
    //
    replicationFactor->setText( settings.value( "createcontext_replicationfactor", "3"      ).toString() );
    strategy         ->setCurrentIndex( settings.value( "createcontext_strategy",  0        ).toInt()    );
    dataCenters      ->setText( settings.value( "createcontext_datacenter",        "DC1"    ).toString() );
    snapServerName   ->setText( settings.value( "createcontext_snapservername",    ""       ).toString() );

    // setup widgets
    f_cancel_button = getChild<QPushButton>(this, "cancelButton");
    f_createcontext_button = getChild<QPushButton>(this, "createContextButton");
    f_replication_factor = getChild<QLineEdit>(this, "replicationFactor");
    f_strategy = getChild<QComboBox>(this, "strategy");
    f_data_centers = getChild<QTextEdit>(this, "dataCenters");
    f_snap_server_name = getChild<QLineEdit>(this, "snapServerName");

    // Close
    connect( f_cancel_button, &QPushButton::clicked, this, &snap_manager_createcontext::cancel );

    // Send Request
    connect( f_createcontext_button, &QPushButton::clicked, this, &snap_manager_createcontext::createcontext );
}


snap_manager_createcontext::~snap_manager_createcontext()
{
}


void snap_manager_createcontext::close()
{
    hide();

    QSettings settings( this );
    settings.setValue( "createcontext_replicationfactor", replicationFactor->text()         );
    settings.setValue( "createcontext_strategy",          strategy         ->currentIndex() );
    settings.setValue( "createcontext_datacenter",        dataCenters      ->toPlainText()  );
    settings.setValue( "createcontext_snapservername",    snapServerName   ->text()         );
    //settings.setValue( "geometry",            saveGeometry()           );
}


void snap_manager_createcontext::cancel()
{
    close();

    emit disconnectRequested();
#if 0
    // allow user to try again
    snap_manager * sm(dynamic_cast<snap_manager *>(parent()));
    if(sm)
    {
        sm->cassandraDisconnectButton_clicked();
    }
#endif
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
void snap_manager_createcontext::createcontext()
{
    int const s(strategy->currentIndex());

    snap::snap_string_list data_centers;
    {
        snap::snap_string_list const names(dataCenters->toPlainText().split('\n'));
        int const max_names(names.size());
        for(int idx(0); idx < max_names; ++idx)
        {
            // remove all spaces in each name
            QString const name(names[idx].simplified());
            if(!name.isEmpty())
            {
                data_centers << name;
            }
        }
        if(data_centers.isEmpty() && s != 0)
        {
            QMessageBox msg(
                    QMessageBox::Information,
                    "Invalid List of Data Centers",
                    "When using a strategy other than Simple the list of Data Centers cannot be empty.",
                    QMessageBox::Ok,
                    this);
            msg.exec();
            dataCenters->setFocus();
            return;
        }
    }

    // make sure the host name is correct (i.e. [a-zA-Z0-9_]+)
    QString const host_name(snapServerName->text());
    {
        if(host_name.isEmpty())
        {
            QMessageBox msg(
                    QMessageBox::Critical,
                    "Invalid Host Name",
                    "The host name is a mandatory field.",
                    QMessageBox::Ok,
                    this);
            msg.exec();
            snapServerName->setFocus();
            return;
        }
        int const max(host_name.length());
        for(int i(0); i < max; ++i)
        {
            int const c(host_name[i].unicode());
            if((c < 'a' || c > 'z')
            && (c < 'A' || c > 'Z')
            && (c < '0' || c > '9' || i == 0) // cannot start with a digit
            && c != '_')
            {
                QMessageBox msg(
                        QMessageBox::Critical,
                        "Invalid Host Name",
                        "The host name must only be composed of letters, digits, and underscores, also it cannot start with a digit ([0-9a-zA-Z_]+)",
                        QMessageBox::Ok,
                        this);
                msg.exec();
                snapServerName->setFocus();
                return;
            }
        }
    }

#if 0
    snap_manager * sm(dynamic_cast<snap_manager *>(parent()));

    if(sm)
    {
        // our parent has all the necessary info about cassandra, so it
        // implements the actual function to create the context...
        //
        sm->create_context(replicationFactor->text().toInt(), s, data_centers, host_name);

        close();

        // allow user to try again
        sm->context_is_valid();
    }
#endif
    // Display wait dialog here modelessly
    //
    create_context( replicationFactor->text().toInt(), s, data_centers, host_name );
}
#pragma GCC diagnostic pop


/** \brief Create the snap_websites context and first few tables.
 *
 * This function creates the snap_websites context.
 *
 * The strategy is defined as a number which represents the selection
 * in the QComboBox of the dialog we just shown to the user. The
 * values are:
 *
 * \li 0 -- Simple
 * \li 1 -- Local
 * \li 2 -- Network
 *
 * \warning
 * It is assumed that you checked all the input parameters validity:
 *
 * \li the replication_factor is under or equal to the number of Cassandra nodes
 * \li the strategy can only be 0, 1, or 2
 * \li the data_centers list cannot be empty
 * \li the host_name must match [a-zA-Z_][a-zA-Z_0-9]*
 *
 * \param[in] replication_factor  Number of times the data will be duplicated.
 * \param[in] strategy  The strategy used to manage the cluster.
 * \param[in] data_centers  List of data centers, one per line, used if
 *                          strategy is not Simple (0).
 * \param[in] host_name  The name of a host to runn a snap server instance.
 */
void snap_manager::create_context(int replication_factor, int strategy, snap::snap_string_list const & data_centers, QString const & host_name)
{
    // when called here we have f_session defined but no context yet

    QListWidget * console = getChild<QListWidget>(this, "cassandraConsole");

    // create a new context
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    console->addItem("Create \"" + context_name + "\" context.");

    f_query = std::make_shared<QCassandraQuery>( f_session );

    QString query_str( QString("CREATE KEYSPACE %1\n")
        .arg(context_name)
        );


    // this is the default for contexts, but just in case we were
    // to change that default at a later time...
    //
    query_str += QString("WITH durable_writes = true\n");

    auto& replication_map(fields["replication"].map());
    replication_map.clear();

    // for developers testing with a few nodes in a single data center,
    // SimpleStrategy is good enough; for anything larger ("a real
    // cluster",) it won't work right
    query_str += QString("AND replication =\n");
    if(strategy == 0 /*"simple"*/)
    {
        query_str += QString( "\t{ 'class': 'SimpleStrategy', 'replication_factor': '1' }\n" );
    }
    else
    {
        query_str += QString( "\t{ 'class': 'NetworkTopologyStrategy',\n" );

        // else strategy == 1 /*"network"*/
        //
        QString s;
        QString const replication(QString("%1").arg(replication_factor));
        int const max_names(data_centers.size());
        for(int idx(0); idx < max_names; ++idx)
        {
            if( !s.isEmpty() )
            {
                s += ",\n";
            }
            s += QString("\t\t'%1': '%2'").arg(data_centers[idx]).arg(replication);
        }
        query_str += s + "}\n";
    }

    f_query->query( query_str );
    f_query->start( false /*don't block*/ );

    QTimer::singleShot( 500, this, &snap_manager_createcontext::onCreateContextTimer );
}


void snap_manager_createcontext::onCreateContextTimer()
{
    if( !f_query->isReady() )
    {
        // Set the timer again and check the status of the query when it expires
        //
        QTimer::singleShot( 500, this, &snap_manager::onCreateContextTimer );
        return;
    }

    // Keyspace has been created, so we can continue now
    //
    f_query->end();
    f_query.reset();

    // add the snap server host name to the list of hosts that may
    // create a lock
    //
    create_table(snap::get_name(snap::name_t::SNAP_NAME_DOMAINS),  "List of domain descriptions.");
    f_context->addLockHost(host_name);
    f_host_list->addItem(host_name);

    // now we want to add the "domains" and "websites" tables to be
    // complete
    //
    create_table(snap::get_name(snap::name_t::SNAP_NAME_DOMAINS),  "List of domain descriptions.");
    create_table(snap::get_name(snap::name_t::SNAP_NAME_WEBSITES), "List of website descriptions.");
}


// vim: ts=4 sw=4 et
