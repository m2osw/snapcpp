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
#include <QtCassandra/QCassandraSession.h>
#include <algorithm>

#include <QtWidgets>

#include "SettingsDialog.h"

using namespace QtCassandra;


SettingsDialog::SettingsDialog(QWidget *p)
    : QDialog(p)
{
    setupUi( this );

    QSettings settings( this );
    restoreGeometry( settings.value( "settings_geometry", saveGeometry() ).toByteArray() );
    f_server           = settings.value( "cassandra_host",       "127.0.0.1" );
    f_port             = settings.value( "cassandra_port",       "9042"      );
    f_promptBeforeSave = settings.value( "prompt_before_commit", true        );
    f_hostnameEdit->setText( f_server.toString() );
    f_portEdit->setValue( f_port.toInt() );
    f_promptCB->setChecked( f_promptBeforeSave.toBool() );
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( false );
}


SettingsDialog::~SettingsDialog()
{
    QSettings settings( this );
    settings.setValue( "settings_geometry",   saveGeometry()	);
}


void SettingsDialog::on_f_buttonBox_accepted()
{
    try
    {
        QCassandraSession::pointer_t session( QCassandraSession::create() );
        session->connect( f_server.toString(), f_port.toInt() );
    }
    catch( const std::exception& ex )
    {
        qDebug()
            << "Caught exception in SettingsDialog::on_f_buttonBox_accepted()! "
            << ex.what();
        QMessageBox::critical( this, tr("Error"), tr("Cannot connect to cassandra server!") );
        return;
    }

    QSettings settings( this );
    settings.setValue( "cassandra_host",       f_server           );
    settings.setValue( "cassandra_port",       f_port             );
    settings.setValue( "prompt_before_commit", f_promptBeforeSave );

    accept();
}

void SettingsDialog::on_f_buttonBox_rejected()
{
    reject();
}


void SettingsDialog::closeEvent( QCloseEvent * e )
{
    // Closing the dialog by the "x" constitutes "reject."
    //
    e->accept();

    reject();
}


void SettingsDialog::on_f_hostnameEdit_textEdited(const QString &arg1)
{
    f_server = arg1;
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
}


void SettingsDialog::on_f_portEdit_valueChanged(int arg1)
{
    f_port = arg1;
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
}

void SettingsDialog::on_f_promptCB_toggled(bool checked)
{
    f_promptBeforeSave = checked;
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
}
