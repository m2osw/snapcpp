/*
 * Text:
 *      SettingsDialog.cpp
 *
 * Description:
 *		TODO
 *
 * Documentation:
 *
 * License:
 *      Copyright (c) 2011-2012 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <QtCassandra/QCassandra.h>
#include <algorithm>
#include "qt4.h"

#include "SettingsDialog.h"

using namespace QtCassandra;


SettingsDialog::SettingsDialog(QWidget *p)
    : QDialog(p)
{
    setupUi( this );

    QSettings settings( this );
    restoreGeometry( settings.value( "settings_geometry", saveGeometry() ).toByteArray() );
    f_server  = settings.value( "cassandra_host", "127.0.0.1" );
    f_port    = settings.value( "cassandra_port", "9160" );
    f_context = settings.value( "context" );
    f_cassandraServerEdit->setText( QString("%1:%2").arg(f_server.toString()).arg(f_port.toInt()) );
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );

    f_contextList->setModel( &f_model );
	f_contextList->setSelectionMode( QListView::SingleSelection );

    updateContextList();
}


SettingsDialog::~SettingsDialog()
{
    QSettings settings( this );
    settings.setValue( "settings_geometry",   saveGeometry()	);
}


void SettingsDialog::updateContextList()
{
    f_model.setStringList( QStringList() );

    const QString full_server_name( f_cassandraServerEdit->text() );
    const int colon_pos = full_server_name.indexOf( ':' );
    if( colon_pos == -1 )
    {
        QMessageBox::critical( this, tr("Error"), tr("Name must be in the form server:port") );
        f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );
        return;
    }

    f_server = full_server_name.left( colon_pos );
    f_port   = full_server_name.mid ( colon_pos+1 );

    QCassandra     cassandra;
    if( !cassandra.connect( f_server.toString(), f_port.toInt() ) )
    {
        QMessageBox::critical( this, tr("Error"), tr("Cannot connect to cassandra server!") );
        return;
    }

    //qDebug() << "Working on Cassandra Cluster Named" << cassandra.clusterName();
    //qDebug() << "Working on Cassandra Protocol Version" << cassandra.protocolVersion();

    const QCassandraContexts& context_list = cassandra.contexts();
    QList<QString> keys = context_list.keys();

    QStringList context_key_list;
    std::for_each( keys.begin(), keys.end(),
                   [&context_key_list]( const QString& key )
    {
        context_key_list << key;
    });

    f_model.setStringList( context_key_list );

    const int idx = context_key_list.indexOf( f_context.toString() );
    if( idx != -1 )
	{
        QModelIndex model_index( f_model.index( idx ) );
        QItemSelectionModel* select( f_contextList->selectionModel() );
		Q_ASSERT(select);
		select->select( model_index, QItemSelectionModel::Select );
	}

    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
}


void SettingsDialog::on_f_cassandraServerEdit_lostFocus()
{
    updateContextList();
}


void SettingsDialog::on_f_contextList_clicked(const QModelIndex &index)
{
    f_context = f_model.stringList()[index.row()];
}


void SettingsDialog::on_SettingsDialog_accepted()
{
}

void SettingsDialog::on_f_buttonBox_accepted()
{
    accept();
    //
    QSettings settings( this );
    settings.setValue( "cassandra_host", f_server  );
    settings.setValue( "cassandra_port", f_port    );
    settings.setValue( "context",        f_context );
}

void SettingsDialog::on_f_buttonBox_rejected()
{
    reject();
}
