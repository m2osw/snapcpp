/*
 * Text:
 *      main.cpp
 *
 * Description:
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

#include "MainWindow.h"
#include "SettingsDialog.h"

using namespace casswrapper;

int main( int argc, char * argv[] )
{
    QApplication app(argc, argv);
    app.setApplicationName    ( "cassview"           );
    app.setApplicationVersion ( CASSVIEW_VERSION     );
    app.setOrganizationDomain ( "snapwebsites.org"   );
    app.setOrganizationName   ( "M2OSW"              );
    app.setWindowIcon         ( QIcon(":icons/icon") );

    bool show_settings = true;
    do
    {
        SettingsDialog dlg( nullptr, true /*first_time*/ );
        QSettings settings;
        if( settings.contains( "cassandra_host") )
        {
            show_settings = !dlg.tryConnection();
        }

        if( show_settings )
        {
            if( dlg.exec() != QDialog::Accepted )
            {
                qDebug() << "User abort!";
                exit( 1 );
            }
        }
    }
    while( show_settings );

    MainWindow win;
    win.show();

    return app.exec();
}

// vim: ts=4 sw=4 et
