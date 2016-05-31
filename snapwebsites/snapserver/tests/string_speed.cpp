// Snap Websites Server -- test against the snap_exception class
// Copyright (C) 2014-2016  Made to Order Software Corp.
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
#include "not_reached.h"

#include <iostream>
#include <cctype>
#include <cstdint>

#include <sys/resource.h>

#include <QDir>


int64_t get_current_date()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return static_cast<int64_t>(usage.ru_utime.tv_sec) * static_cast<int64_t>(1000000)
         + static_cast<int64_t>(usage.ru_utime.tv_usec);
}



int main(int /*argc*/, char * /*argv*/[])
{
    // prepare a string
    QString path("finball/redirect/vendor-brand");

    // try == with full path
    int j(0);
    int64_t a_start(get_current_date());
    for(int i(0); i < 10000000; ++i)
    {
        bool const unused(path == "finball/redirect/vendor-brand");
        if(unused)
        {
            j++;
        }
    }
    int64_t a_end(get_current_date());

    // try endsWith() with the shortest possible path
    int64_t b_start(get_current_date());
    for(int i(0); i < 10000000; ++i)
    {
        bool const unused(path.endsWith("/vendor-brand"));
        if(unused)
        {
            j++;
        }
    }
    int64_t b_end(get_current_date());

    int64_t a_diff = (a_end - a_start);
    int64_t b_diff = (b_end - b_start);

    std::cerr << "j = " << j << " iterations\n"
              << "a = " << a_diff << "\n"
              << "b = " << b_diff << "\n"
              << "diff = " << labs(a_diff - b_diff) << "\n";

    return 0;
}

// vim: ts=4 sw=4 et
