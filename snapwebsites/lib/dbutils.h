// Snap Websites Server -- manage sessions for users, forms, etc.
// Copyright (C) 2012-2014  Made to Order Software Corp.
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

#include <QtCassandra/QCassandraCell.h>
#include <QtCassandra/QCassandraRow.h>

#include <QString>
#include <QByteArray>

namespace snap
{

class dbutils
{
public:
    dbutils( const QString& table_name, const QString& row_name );

    static QString    byte_to_hex   ( const char        byte );
    static QString    key_to_string ( const QByteArray& key  );
    static QByteArray string_to_key ( const QString&    str  );

    int        get_display_len() const;
    void       set_display_len( const int val );

    QByteArray get_row_key() const;
    QString    get_row_name( QtCassandra::QCassandraRow::pointer_t p_r ) const;

    QString    get_column_name ( QtCassandra::QCassandraCell::pointer_t c ) const;
    QString    get_column_value( QtCassandra::QCassandraCell::pointer_t c, const bool display_only = false ) const;
    void       set_column_value( QtCassandra::QCassandraCell::pointer_t c, const QString& v );

private:
    typedef enum
    {
        CT_uint64_value,
        CT_time_microseconds,
        CT_time_seconds,
        CT_float32_value,
        CT_uint32_value,
        CT_int8_value,
        CT_uint8_value,
        CT_hexarray_value,
        CT_hexarray_limited_value,
        CT_md5array_value,
        CT_secure_value,
        CT_string_value
    } column_type_t;

    column_type_t get_column_type( QtCassandra::QCassandraCell::pointer_t c ) const;

    QString f_tableName;
    QString f_rowName;
    int     f_displayLen;
};

}
// namespace snap

// vim: ts=4 sw=4 et
