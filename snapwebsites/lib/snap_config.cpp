// Snap Websites Server -- configuration reader
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

#include "snap_config.h"
#include "log.h"
#include "not_reached.h"

#include <QFile>

#include <iostream>
#include <memory>
#include <sstream>

#include <syslog.h>

#include "poison.h"

namespace snap
{


snap_config::snap_config()
{
    // empty
}


QString& snap_config::operator []( const QString& name )
{
    return f_parameters[name];
}


QString snap_config::operator []( const QString& name ) const
{
    return f_parameters[name];
}


void snap_config::clear()
{
    f_cmdline_params.clear();
    f_parameters.clear();
}


void snap_config::set_cmdline_params( const parameter_map_t& params )
{
    f_cmdline_params = params;
}


/** \brief Read the configuration file into memory.
 *
 * \param[in] filename  The name of the file to read the parameters from.
 */
void snap_config::read_config_file( QString const& filename )
{
    // read the configuration file now
    QFile c;
    c.setFileName(filename);
    c.open(QIODevice::ReadOnly);
    if(!c.isOpen())
    {
        // if for nothing else we need to have the list of plugins so we always
        // expect to have a configuration file... if we're here we could not
        // read it, unfortunately
        std::stringstream ss;
        ss << "cannot read configuration file \"" << filename.toUtf8().data() << "\"";
        SNAP_LOG_FATAL() << ss.str() << ".";
        syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
        exit(1);
    }

    // read the configuration file variables as parameters
    //
    // TODO: use C++ and getline() so we do not have to limit the length of a line
    char buf[1024];
    for(int line(1); c.readLine(buf, sizeof(buf)) > 0; ++line)
    {
        // make sure the last byte is '\0'
        buf[sizeof(buf) - 1] = '\0';
        int len(static_cast<int>(strlen(buf)));
        if(len == 0 || (buf[len - 1] != '\n' && buf[len - 1] != '\r'))
        {
            std::stringstream ss;
            ss << "line " << line << " in \"" << filename.toUtf8().data() << "\" is too long";
            SNAP_LOG_ERROR() << ss.str() << ".";
            syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
            exit(1);
        }
        buf[len - 1] = '\0';
        --len;
        while(len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        {
            --len;
            buf[len] = '\0';
        }
        if(len == 0)
        {
            // empty line
            continue;
        }
        char *n(buf);
        while(isspace(*n))
        {
            ++n;
        }
        if(*n == '#' || *n == '\0')
        {
            // comment or empty line
            continue;
        }
        char *v(n);
        while(*v != '=' && *v != '\0')
        {
            // TODO verify that the name is only ASCII? (probably not too
            //      important because if not it will be ignored anyway)
            //      Note that the layout expects names including colons (:)
            //      as a namespace separator: layout::layout, layout::theme.
            ++v;
        }
        if(*v != '=')
        {
            std::stringstream ss;
            ss << "invalid variable on line " << line << " in \"" << filename.toUtf8().data() << "\", no equal sign found";
            SNAP_LOG_ERROR() << ss.str() << ".";
            syslog( LOG_CRIT, "%s, server not started. (in server::config())", ss.str().c_str() );
            exit(1);
        }
        char *e;
        for(e = v; e > n && isspace(e[-1]); --e);
        *e = '\0';
        do
        {
            ++v;
        }
        while(isspace(*v));
        for(e = v + strlen(v); e > v && isspace(e[-1]); --e);
        *e = '\0';
        if(v != e && ((v[0] == '\'' && e[-1] == '\'') || (v[0] == '"' && e[-1] == '"')))
        {
            // remove single or double quotes
            v++;
            e[-1] = '\0';
        }
        // keep the command line defined parameters
        if(!f_cmdline_params.contains(n))
        {
            f_parameters[n] = QString::fromUtf8(v);
        }
        else
        {
            SNAP_LOG_WARNING("warning: parameter \"")(n)("\" from the configuration file (")
                      (v)(") ignored as it was specified on the command line (")
                      (f_parameters[n])(").");
        }
    }
}


bool snap_config::contains( const QString& name ) const
{
    return f_parameters.contains( name );
}

}
//namespace snap

// vim: ts=4 sw=4 et syntax=cpp.doxygen
