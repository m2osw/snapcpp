// Snap Websites Servers -- snap websites child
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

namespace snap
{
namespace logging
{

enum log_level_t
{
    LOG_LEVEL_OFF,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE
};

enum log_security_t
{
    LOG_SECURITY_NONE,
    LOG_SECURITY_SECURE
};

class logger
{
public:
    logger(log_level_t log_level, char const *file = NULL, char const *func = NULL, int line = -1);
    ~logger();

    logger& operator () (log_security_t const v);
    logger& operator () (char const *s);
    logger& operator () (wchar_t const *s);
    logger& operator () (QString const& s);
    logger& operator () (char const v);
    logger& operator () (signed char const v);
    logger& operator () (unsigned char const v);
    logger& operator () (signed short const v);
    logger& operator () (unsigned short const v);
    logger& operator () (signed int const v);
    logger& operator () (unsigned int const v);
    logger& operator () (signed long const v);
    logger& operator () (unsigned long const v);
    logger& operator () (signed long long const v);
    logger& operator () (unsigned long long const v);
    logger& operator () (float const v);
    logger& operator () (double const v);

private:
    log_level_t     f_log_level;
    char const *    f_file;
    char const *    f_func;
    int             f_line;
    log_security_t  f_security;
    QString         f_message;
};

void configure(QString filename);
void reconfigure();
bool is_configured();

logger fatal  (char const *file = NULL, char const *func = NULL, int line = -1);
logger error  (char const *file = NULL, char const *func = NULL, int line = -1);
logger warning(char const *file = NULL, char const *func = NULL, int line = -1);
logger info   (char const *file = NULL, char const *func = NULL, int line = -1);
logger debug  (char const *file = NULL, char const *func = NULL, int line = -1);
logger trace  (char const *file = NULL, char const *func = NULL, int line = -1);

#define    SNAP_LOG_FATAL       snap::logging::fatal  (__FILE__, __func__, __LINE__)
#define    SNAP_LOG_ERROR       snap::logging::error  (__FILE__, __func__, __LINE__)
#define    SNAP_LOG_WARNING     snap::logging::warning(__FILE__, __func__, __LINE__)
#define    SNAP_LOG_INFO        snap::logging::info   (__FILE__, __func__, __LINE__)
#define    SNAP_LOG_DEBUG       snap::logging::debug  (__FILE__, __func__, __LINE__)
#define    SNAP_LOG_TRACE       snap::logging::trace  (__FILE__, __func__, __LINE__)

} // namespace logging
} // namespace snap
// vim: ts=4 sw=4 et
