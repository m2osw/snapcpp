// Network Address -- classes functions to ease handling IP addresses
// Copyright (C) 2012-2017  Made to Order Software Corp.
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

// self
//
#include "libaddr/addr.h"
#include "libaddr/version.h"


namespace addr
{

int get_version_major()
{
    return LIBADDR_VERSION_MAJOR;
}


int get_version_minor()
{
    return LIBADDR_VERSION_MINOR;
}


int get_version_patch()
{
    return LIBADDR_VERSION_PATCH;
}


char const * get_version_string()
{
    return LIBADDR_VERSION_STRING;
}



}
// snap_addr namespace
// vim: ts=4 sw=4 et
