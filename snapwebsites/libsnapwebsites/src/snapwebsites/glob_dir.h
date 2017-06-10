// Snap Websites Servers -- glob a directory and enumerate the files
// Copyright (C) 2016-2017  Made to Order Software Corp.
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
#pragma once

#include <QString>

#include <functional>
#include <memory>

#include <glob.h>

namespace snap
{

class glob_dir
{
public:
    glob_dir();

    glob_dir( QString const & path );

    void set_path( QString const& path );

    void enumerate_glob( std::function<void (QString path)> func );

private:
    struct glob_deleter
    {
        void operator()(glob_t* dir)
        {
            globfree( dir );
        }
    };
    typedef std::unique_ptr<glob_t,glob_deleter> glob_pointer_t;
    glob_pointer_t f_dir;

    static int glob_err_callback(const char * epath, int eerrno);
};

} // namespace snap
// vim: ts=4 sw=4 et
