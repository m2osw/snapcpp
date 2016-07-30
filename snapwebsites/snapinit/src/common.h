/////////////////////////////////////////////////////////////////////////////////
// Snap Init Server -- snap initialization server
// Copyright (C) 2011-2016  Made to Order Software Corp.
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
// This server reads in a configuration file and keeps specified services running.
// When signaled, it will terminate those services cleanly.
/////////////////////////////////////////////////////////////////////////////////
#pragma once

// Qt library
//
#include <QString>

namespace snapinit
{
namespace common
{

int64_t const        SECONDS_TO_MICROSECONDS = 1000000LL;        // 1 second in microseconds

// TODO: this should make use of a class that snapinit derives from
//       so any class can still call these functions, but we could
//       then properly handle various cases (and probably make use
//       of an exception instead of exit(1)...)

bool                is_a_tty();
void                fatal_message(QString const & msg);
[[noreturn]] void   fatal_error(QString const & msg);
void                setup_fatal_pid();

} // namespace common
} // namespace snapinit
// vim: ts=4 sw=4 et
