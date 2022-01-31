// Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapbuilder
// contact@m2osw.com
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// self
//
#include    "snap_builder.h"
#include    "version.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// advgetopt lib
//
#include    <advgetopt/exception.h>


// Qt lib
//
#include    <QMessageBox>


// last include
//
#include    <snapdev/poison.h>



int main(int argc, char * argv[])
{
    try
    {
        QT_REQUIRE_VERSION(argc, argv, QT_VERSION_STR)

        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

        QApplication app(argc, argv);
        app.setApplicationName("snapbuilder");
        app.setApplicationVersion(SNAPBUILDER_VERSION_STRING);
        app.setOrganizationDomain("snapwebsites.org");
        app.setOrganizationName("Made to Order Software Corp.");

        builder::snap_builder window(argc, argv);
        window.show();
        window.run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        exit(e.code());
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_FATAL
            << "an exception occurred: "
            << e.what()
            << SNAP_LOG_SEND;
        return 1;
    }
    catch(...)
    {
        SNAP_LOG_FATAL
            << "an unknown exception occurred."
            << SNAP_LOG_SEND;
        return 1;
    }

    return 0;
}


// vim: ts=4 sw=4 et
