// Snap Websites Servers -- snap websites child
// Copyright (C) 2013-2015  Made to Order Software Corp.
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

#include <controlled_vars/controlled_vars_need_init.h>
#include <controlled_vars/controlled_vars_auto_init.h>
#include <controlled_vars/controlled_vars_limited_need_enum_init.h>
#include <controlled_vars/controlled_vars_limited_auto_enum_init.h>
#include <controlled_vars/controlled_vars_ptr_need_init.h>

#include <QString>

namespace snap
{
namespace logging
{

enum class log_level_t
{
    LOG_LEVEL_OFF,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE
};

enum class log_security_t
{
    LOG_SECURITY_NONE,
    LOG_SECURITY_SECURE
};

class logger
{
public:
                    logger(log_level_t const log_level, char const * file = nullptr, char const * func = nullptr, int const line = -1);
                    logger(logger const & l);
                    ~logger();

    // avoid assignments
    logger &        operator = (logger const &) = delete;

    logger &        operator () ();
    logger &        operator () (log_security_t const v);
    logger &        operator () (char const * s);
    logger &        operator () (wchar_t const * s);
    logger &        operator () (std::string const & s);
    logger &        operator () (std::wstring const & s);
    logger &        operator () (QString const & s);
    logger &        operator () (char const v);
    logger &        operator () (signed char const v);
    logger &        operator () (unsigned char const v);
    logger &        operator () (signed short const v);
    logger &        operator () (unsigned short const v);
    logger &        operator () (signed int const v);
    logger &        operator () (unsigned int const v);
    logger &        operator () (signed long const v);
    logger &        operator () (unsigned long const v);
    logger &        operator () (signed long long const v);
    logger &        operator () (unsigned long long const v);
    logger &        operator () (float const v);
    logger &        operator () (double const v);
    logger &        operator () (bool const v);

    static bool     is_enabled_for(log_level_t const log_level);

protected:
    mutable controlled_vars::flbool_t   f_ignore;

private:
    typedef controlled_vars::limited_need_enum_init<log_security_t, log_security_t::LOG_SECURITY_NONE, log_security_t::LOG_SECURITY_SECURE> mlog_security_t;
    typedef controlled_vars::limited_need_enum_init<log_level_t, log_level_t::LOG_LEVEL_OFF, log_level_t::LOG_LEVEL_TRACE> mlog_level_t;
    typedef controlled_vars::ptr_need_init<char const> mpcchar_t;

    mlog_level_t                        f_log_level;
    mpcchar_t                           f_file;
    mpcchar_t                           f_func;
    controlled_vars::mint32_t           f_line;
    mlog_security_t                     f_security;
    QString                             f_message;
};

void set_progname               ( std::string const & progname );
std::string get_progname        ();
void configure_console          ();
void configure_logfile          ( QString const & logfile  );
void configure_syslog           ();
void configure_server           ();
void configure_conffile         ( QString const & filename );
void unconfigure                ();
void reconfigure                ();
bool is_configured              ();
void set_log_output_level       ( log_level_t level );
void reduce_log_output_level    ( log_level_t level );
bool is_loggingserver_available ( QString const & logserver );

logger & operator << ( logger & l, QString const &                    msg );
logger & operator << ( logger & l, std::basic_string<char> const &    msg );
logger & operator << ( logger & l, std::basic_string<wchar_t> const & msg );
logger & operator << ( logger & l, char const *                       msg );
logger & operator << ( logger & l, wchar_t const *                    msg );

template <class T>
logger & operator << ( logger & l, T const & msg )
{
    l( msg );
    return l;
}

logger fatal  (char const * file = nullptr, char const * func = nullptr, int line = -1);
logger error  (char const * file = nullptr, char const * func = nullptr, int line = -1);
logger warning(char const * file = nullptr, char const * func = nullptr, int line = -1);
logger info   (char const * file = nullptr, char const * func = nullptr, int line = -1);
logger debug  (char const * file = nullptr, char const * func = nullptr, int line = -1);
logger trace  (char const * file = nullptr, char const * func = nullptr, int line = -1);

#define    SNAP_LOG_FATAL       snap::logging::fatal  (__FILE__, __func__, __LINE__)
#define    SNAP_LOG_ERROR       snap::logging::error  (__FILE__, __func__, __LINE__)
#define    SNAP_LOG_WARNING     snap::logging::warning(__FILE__, __func__, __LINE__)
#define    SNAP_LOG_INFO        snap::logging::info   (__FILE__, __func__, __LINE__)
#define    SNAP_LOG_DEBUG       snap::logging::debug  (__FILE__, __func__, __LINE__)
#define    SNAP_LOG_TRACE       snap::logging::trace  (__FILE__, __func__, __LINE__)

} // namespace logging
} // namespace snap
// vim: ts=4 sw=4 et
