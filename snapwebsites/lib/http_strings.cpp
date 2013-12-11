// Snap Websites Server -- parse strings
// Copyright (C) 2013  Made to Order Software Corp.
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

#include "http_strings.h"

namespace snap
{
namespace http_strings
{


WeightedHttpString::WeightedHttpString(const QString& str)
    : f_str(str)
    //, f_parts() -- auto-init
{
    QByteArray utf8(f_str.toUtf8());
    const char *s(utf8.data());
    while(*s != '\0')
    {
        while(isspace(*s) || *s == ',')
        {
            ++s;
        }
        if(*s == '\0')
        {
            break;
        }
        const char *v(s);
        while(*s != '\0' && *s != ',' && *s != ';')
        {
            ++s;
        }
        QString name(QString::fromUtf8(v, s - v));
        name = name.simplified();
        // an authoritative document at the IANA clearly says that
        // the default level (quality value) is 1.0f.
        float level(1.0f);
        // read all the parameters, although we only keep
        // the 'q' parameter at this time
        while(*s == ';')
        {
            ++s;
            v = s;
            while(*s != '\0' && *s != ',' && *s != ';' && *s != '=')
            {
                ++s;
            }
            QString param_name(QString::fromUtf8(v, s - v));
            param_name = param_name.simplified();
            QString param_value;
            if(*s == '=')
            {
                ++s;
                v = s;
                while(*s != '\0' && *s != ',' && *s != ';')
                {
                    ++s;
                }
                param_value = QString::fromUtf8(v, s - v);
                param_value = param_value.trimmed();
            }
            if(param_name == "q")
            {
                bool ok(false);
                level = param_value.toFloat(&ok);
                if(!ok || level < 0)
                {
                    // not okay, keep 1.0f instead
                    level = 1.0f;
                }
            }
            // TODO add support for other parameters, "charset" is one of
            //      them in the Accept header which we want to support
        }
        part_t part(name, level);
        f_parts.push_back(part);
    }
}


float WeightedHttpString::get_level(const QString& name)
{
    const int max(f_parts.size());
    for(int i(0); i < max; ++i)
    {
        if(f_parts[i].get_name() == name)
        {
            return f_parts[i].get_level();
        }
    }
    return -1.0f;
}




} // namespace http_strings
} // namespace snap
// vim: ts=4 sw=4 et
