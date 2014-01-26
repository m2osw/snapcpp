// Snap Websites Server -- manage sessions for users, forms, etc.
// Copyright (C) 2013-2014  Made to Order Software Corp.
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

#include <controlled_vars/controlled_vars_limited_auto_init.h>
#include <QString>
#include <QStringList>

namespace snap
{
namespace compression
{

// compression level is a percent (a number from 0 to 100)
typedef controlled_vars::limited_auto_init<int, 0, 100, 50> level_t;

// all compressors derive from this class
class compressor_t
{
public:
    static const char * BEST_COMPRESSION;
    static const char * NO_COMPRESSION;

                        compressor_t(const char *name);
    virtual             ~compressor_t();
    virtual const char *get_name() const = 0;
    virtual QByteArray  compress(const QByteArray& input, level_t level, bool text) = 0;
    virtual bool        compatible(const QByteArray& input) const = 0;
    virtual QByteArray  decompress(const QByteArray& input) = 0;
};

void register_compressor(compressor_t *compressor_name);
QStringList compressor_list();
QByteArray compress(QString& compressor_name, const QByteArray& input, level_t level, bool text);
QByteArray decompress(QString& compressor_name, const QByteArray& input);

} // namespace snap
} // namespace compression
// vim: ts=4 sw=4 et
