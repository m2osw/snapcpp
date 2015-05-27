// Snap Websites Server -- configuration reader
// Copyright (C) 2011-2015  Made to Order Software Corp.
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

#include <QMap>
#include <QString>

namespace snap
{

class snap_config
{
public:
    typedef QMap<QString, QString> parameter_map_t;

                    snap_config();

    void            clear();
    void            set_cmdline_params( parameter_map_t const & params );
    void            read_config_file( QString const & filename );

    QString &       operator []( QString const & name );
    QString         operator []( QString const & name ) const;

    bool            contains( QString const & name ) const;

private:
    parameter_map_t f_parameters;
    parameter_map_t f_cmdline_params;
};

}
// namespace snap

// vim: ts=4 sw=4 et
