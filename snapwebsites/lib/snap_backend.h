// Snap Websites Servers -- snap websites child process hanlding
// Copyright (C) 2011-2014  Made to Order Software Corp.
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
#pragma once

#include "snap_child.h"

namespace snap
{

class snap_backend : public snap_child
{
public:
    snap_backend( server_pointer_t s );
    ~snap_backend();

    void        run_backend();

private:
    void        process_backend_uri(QString const& uri);
};

} // namespace snap
// vim: ts=4 sw=4 et
