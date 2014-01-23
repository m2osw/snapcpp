// Snap Servers -- HTTP string handling (splitting, etc.)
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

#include <QString>
#include <QVector>
#include <controlled_vars/controlled_vars_fauto_init.h>

namespace snap
{
namespace http_strings
{



// for HTTP_ACCEPT_ENCODING, HTTP_ACCEPT_LANGUAGE, HTTP_ACCEPT
class WeightedHttpString
{
public:
    class part_t
    {
    public:
        part_t()
            //: f_name("") -- auto-init
            //, f_level(0.0f) -- auto-init
        {
        }

        part_t(QString const& name, float level)
            : f_name(name)
            , f_level(level)
        {
        }

        QString const& get_name() const
        {
            return f_name;
        }

        float get_level() const
        {
            return f_level;
        }

        /** \brief Operator used to sort elements.
         *
         * This oeprator overload is used by the different sort
         * algorithm that we can apply against this type.
         */
        bool operator < (part_t const& rhs) const
        {
            return f_level < rhs.f_level;
        }

    private:
        QString                     f_name;
        controlled_vars::zfloat_t   f_level; // q=0.8
        // TODO add support for any other parameter
    };
    typedef QVector<part_t> part_vector_t; // do NOT use a map, we want to keep them in order!

                        WeightedHttpString(const QString& str);

    QString const&      get_string() const { return f_str; }
    float               get_level(const QString& name);
    part_vector_t const get_parts() const { return f_parts; }

private:
    QString             f_str;
    part_vector_t       f_parts; // do NOT use a map, we want to keep them in order
};



} // namespace http_strings
} // namespace snap
// vim: ts=4 sw=4 et
