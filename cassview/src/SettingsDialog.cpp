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

namespace
{
    const char* SERVER_ID      = "cassandra_host"       ;
    const char* PORT_ID        = "cassandra_port"       ;
    const char* PROMPT_ID      = "prompt_before_commit" ;
    const char* GEOMETRY_ID    = "settings_geometry"    ;
    const char* SERVER_DEFAULT = "127.0.0.1"            ;
    const int   PORT_DEFAULT   = 9042                   ;
    const bool  PROMPT_DEFAULT = true                   ; // Prompt before saving to database
}


SettingsDialog::SettingsDialog( QWidget *p, const bool first_time )
    : QDialog(p)
{
    setupUi( this );

    QSettings settings( this );
    restoreGeometry( settings.value( GEOMETRY_ID, saveGeometry() ).toByteArray() );
    f_server           = settings.value( SERVER_ID, SERVER_DEFAULT );
    f_port             = settings.value( PORT_ID,   PORT_DEFAULT   );
    f_promptBeforeSave = settings.value( PROMPT_ID, PROMPT_DEFAULT );
    f_hostnameEdit->setText( f_server.toString() );
    f_portEdit->setValue( f_port.toInt() );
    f_promptCB->setChecked( f_promptBeforeSave.toBool() );
    f_buttonBox->button( QDialogButtonBox::Ok )->setEnabled( first_time );
}


SettingsDialog::~SettingsDialog()
{
    QSettings settings( this );
    settings.setValue( GEOMETRY_ID, saveGeometry() );
}


bool SettingsDialog::tryConnection( QWidget* p )
{
    try
    {
        QSettings settings;
        const QString server = settings.value( SERVER_ID, SERVER_DEFAULT ).toString();
        const int     port   = settings.value( PORT_ID,   PORT_DEFAULT   ).toInt();

        QCassandraSession::pointer_t session( QCassandraSession::create() );
        session->connect( server, port );
    }
    catch( const std::exception& ex )
    {
        qDebug()
            << "Caught exception in SettingsDialog::tryConnection()! "
            << ex.what();
        QMessageBox::critical( p, tr("Cassview Connection Error"), tr("Cannot connect to cassandra server!") );
        return false;
    }

    return true;
}

void SettingsDialog::on_f_buttonBox_accepted()
{
    QSettings settings( this );

    const QString prev_server = settings.value( SERVER_ID, SERVER_DEFAULT ).toString();
    const int     prev_port   = settings.value( PORT_ID,   PORT_DEFAULT   ).toInt();
    //
    settings.setValue( SERVER_ID, f_server           );
    settings.setValue( PORT_ID,   f_port             );

    if( !tryConnection( this ) )
    {
        // Put back the old values and return, causing the dialog to stay open.
        settings.setValue( SERVER_ID, prev_server           );
        settings.setValue( PORT_ID,   prev_port             );
        return;
    }

    // Accept the settings and exit
    //
    settings.setValue( PROMPT_ID, f_promptBeforeSave );

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
