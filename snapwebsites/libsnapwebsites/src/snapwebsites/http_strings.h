// Snap Servers -- HTTP string handling (splitting, etc.)
// Copyright (C) 2013-2017  Made to Order Software Corp.
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
#include <QVector>

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
        typedef QVector<part_t> vector_t; // do NOT use a map, we want to keep them in order!

        // part_t() is for the vector, otherwise we cannot initialize it properly
        part_t()
            //: f_name("") -- auto-init
            //, f_level(0.0f) -- auto-init
        {
        }

        // an authoritative document at the IANA clearly says that
        // the default level (quality value) is 1.0f.
        part_t(QString const & name)
            : f_name(name)
            , f_level(1.0f)
        {
        }

        QString const & get_name() const
        {
            return f_name;
        }

        float get_level() const
        {
            return f_level;
        }

        void set_level(float const level)
        {
            f_level = level;
        }

        QString get_parameter(QString const & name) const
        {
            if(!f_param.contains(name))
            {
                return "";
            }
            return f_param[name];
        }

        void add_parameter(QString const & name, QString const & value)
        {
            f_param[name] = value;
        }

        QString to_string() const;

        /** \brief Operator used to sort elements.
         *
         * This oeprator overload is used by the different sort
         * algorithm that we can apply against this type.
         */
        bool operator < (part_t const & rhs) const
        {
            return f_level < rhs.f_level;
        }

    private:
        typedef QMap<QString, QString>      parameters_t;

        QString                     f_name;
        float                       f_level = 0.0f; // q=0.8
        // TODO add support for any other parameter
        parameters_t                f_param;
    };

                            WeightedHttpString(QString const & str);

    QString const &         get_string() const { return f_str; }
    float                   get_level(QString const & name);
    part_t::vector_t &      get_parts() { return f_parts; }
    QString                 to_string() const;

private:
    QString                 f_str;
    part_t::vector_t        f_parts; // do NOT use a map, we want to keep them in order
};



} // namespace http_strings
} // namespace snap
// vim: ts=4 sw=4 et
