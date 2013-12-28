// Snap Websites Servers -- basic UTF-8 handling
// Copyright (C) 2011-2013  Made to Order Software Corp.
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
#ifndef SNAP_UTF8_H
#define SNAP_UTF8_H

#include "snap_uri.h"
#include "snap_signals.h"
#include "snap_exception.h"
#include "http_cookie.h"
#include "udp_client_server.h"
#include <stdlib.h>
#include <controlled_vars/controlled_vars_need_init.h>
#include <QPointer>
#include <QDomDocument>
#include <QBuffer>
#include <QtCassandra/QCassandra.h>
#include <QtCassandra/QCassandraContext.h>

namespace snap
{


bool is_valid_ascii(char const *string);
bool is_valid_utf8(char const *string);


} // namespace snap
#endif
// SNAP_UTF8_H
// vim: ts=4 sw=4 et
