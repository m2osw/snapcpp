// Snap Websites Server -- test against the snap_exception class
// Copyright (C) 2014  Made to Order Software Corp.
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

//
// This test verifies that names, versions, and browsers are properly
// extracted from filenames and dependencies and then that the resulting
// versioned_filename and dependency objects compare against each others
// as expected.
//

#include "snap_exception.h"
#include "log.h"
#include "qstring_stream.h"
#include <iostream>
#include <QDir>


int main(int /*argc*/, char * /*argv*/[])
{
    const QString conf_file( QString("%1/test_snap_exception.conf").arg(QDir::currentPath()) );
    snap::logging::configure( conf_file.toAscii().data() );
    //snap::snap_exception::set_debug( true );

    SNAP_LOG_INFO("test_snap_exception");

    try
    {
        throw snap::snap_exception( "This is an exception!" );
    }
    catch( snap::snap_exception& except )
    {
        std::cout << "Caught snap exception " << except.what() << std::endl;
    }

    return 0;
}

// vim: ts=4 sw=4 et
