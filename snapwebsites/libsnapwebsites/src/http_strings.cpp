// Snap Websites Server -- parse strings
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

#include "snapwebsites/http_strings.h"

#include "snapwebsites/log.h"

#include "snapwebsites/poison.h"


namespace snap
{
namespace http_strings
{




QString WeightedHttpString::part_t::to_string() const
{
    QString result;

    result = f_name;
    for(parameters_t::const_iterator it(f_param.begin());
                                     it != f_param.end();
                                     ++it)
    {
        QString p(it.key());
        if(!it.value().isEmpty())
        {
            p = QString("%1=%2").arg(p).arg(it.value());
        }
        result = QString("%1; %2").arg(result).arg(p);
    }

    return result;
}


WeightedHttpString::WeightedHttpString(QString const & str)
    : f_str(str)
    //, f_parts() -- auto-init
{
    QByteArray utf8(f_str.toUtf8());
    char const * s(utf8.data());
    for(;;)
    {
        while(isspace(*s) || *s == ',')
        {
            ++s;
        }
        if(*s == '\0')
        {
            break;
        }
        char const * v(s);
        while(*s != '\0' && *s != ',' && *s != ';')
        {
            ++s;
        }
        // TODO: add support for a value assigned to the first entry?
        //       (and thus mark it a part as well)
        //
        QString name(QString::fromUtf8(v, static_cast<int>(s - v)));
        name = name.simplified();
        part_t part(name);
        // read all the parameters, although we only keep
        // the 'q' parameter at this time
        //
        while(*s == ';')
        {
            ++s;
            v = s;
            while(*s != '\0' && *s != ',' && *s != ';' && *s != '=')
            {
                ++s;
            }
            QString param_name(QString::fromUtf8(v, static_cast<int>(s - v)));
            param_name = param_name.simplified();
            if(!param_name.isEmpty())
            {
                QString param_value;
                if(*s == '=')
                {
                    ++s;
                    v = s;
                    while(*s != '\0' && *s != ',' && *s != ';')
                    {
                        ++s;
                    }
                    param_value = QString::fromUtf8(v, static_cast<int>(s - v));
                    param_value = param_value.trimmed();
                }
                part.add_parameter(param_name, param_value);

                if(param_name == "q")
                {
                    bool ok(false);
                    float const level(param_value.toFloat(&ok));
                    if(ok && level >= 0)
                    {
                        part.set_level(level);
                    }
                }
                // TODO add support for other parameters, "charset" is one of
                //      them in the Accept header which we want to support
            }
            else if(*s == '=')
            {
                // just ignore that entry...
                ++s;
                while(*s != '\0' && *s != ',' && *s != ';')
                {
                    ++s;
                }

                SNAP_LOG_ERROR("found a spurious equal sign in a weighted string");
            }
        }
        f_parts.push_back(part);
    }
}


float WeightedHttpString::get_level(QString const & name)
{
    const int max_parts(f_parts.size());
    for(int i(0); i < max_parts; ++i)
    {
        if(f_parts[i].get_name() == name)
        {
            return f_parts[i].get_level();
        }
    }
    return -1.0f;
}


QString WeightedHttpString::to_string() const
{
    QString result;
    const int max_parts(f_parts.size());
    for(int i(0); i < max_parts; ++i)
    {
        if(!result.isEmpty())
        {
            result += ", ";
        }
        result += f_parts[i].to_string();
    }
    return result;
}




} // namespace http_strings
} // namespace snap
// vim: ts=4 sw=4 et
