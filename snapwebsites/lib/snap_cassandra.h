// Snap Websites Server -- snap websites server
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

#include "snap_config.h"

#include <controlled_vars/controlled_vars.h>
#include <QtCassandra/QCassandra.h>

#include <QString>

namespace snap
{


class snap_cassandra
{
public:
    snap_cassandra();

    void connect( snap_config* config );
    void init_context();
    QtCassandra::QCassandraContext::pointer_t get_snap_context();

    QString get_cassandra_host() const;
    int32_t get_cassandra_port() const;

private:
    QtCassandra::QCassandra::pointer_t f_cassandra;
    QString                            f_cassandra_host;
    controlled_vars::zint32_t          f_cassandra_port;
};


}
// namespace snap

// vim: ts=4 sw=4 et syntax=cpp.doxygen
