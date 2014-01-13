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
    restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );
    f_server = settings.value( "cassandra_host", "127.0.0.1" );
    f_port   = settings.value( "cassandra_port", "9160" );
    f_cassandraServerEdit->setText( QString("%1:%2").arg(f_server.toString()).arg(f_port.toInt()) );
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );

    f_context->setModel( &f_model );
}


SettingsDialog::~SettingsDialog()
{
    QSettings settings( this );
    settings.setValue( "cassandra_host", f_server );
    settings.setValue( "cassandra_port", f_port   );
    settings.setValue( "geometry",       saveGeometry() );
}


#if 0
void SettingsDialog::closeEvent( QCloseEvent * e )
{
    event->accept();
}
#endif



void SettingsDialog::on_lineEdit_editingFinished()
{
    updateContextList();
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
    f_port   = full_server_name.right( colon_pos );

    QCassandra     cassandra;
    if( !cassandra.connect( f_server.toString(), f_port.toInt() ) )
    {
        QMessageBox::critical( this, tr("Error"), tr("Cannot connect to cassandra server!") );
        return;
    }

    qDebug() << "Working on Cassandra Cluster Named" << cassandra.clusterName();
    qDebug() << "Working on Cassandra Protocol Version" << cassandra.protocolVersion();

    const QCassandraContexts& context_list = cassandra.contexts();
    QList<QString> keys = context_list.keys();

    QStringList context_key_list;
    std::for_each( keys.begin(), keys.end(),
                   [&context_key_list]( const QString& key )
    {
        context_key_list << key;
    });

    f_model.setStringList( context_key_list );

    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
}
