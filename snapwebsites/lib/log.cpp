// Snap Websites Server -- log services
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

#include "log.h"

#include "not_reached.h"
#include "snap_exception.h"

#include <syslog.h>

#include <log4cplus/configurator.h>
#include <log4cplus/logger.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>

#include <QFileInfo>

#include "poison.h"

namespace snap
{

namespace logging
{

namespace
{
    QString             g_log_config_filename;
    QString             g_log_output_filename;
    log4cplus::Logger   g_logger;
    log4cplus::Logger   g_secure_logger;

    typedef enum { unconfigured_logger, console_logger, file_logger, conffile_logger } logging_type_t;
    logging_type_t      g_logging_type( unconfigured_logger );
}
// no name namespace



/** \brief Unconfigure the logger and reset.
 *
 * This is an internal function which is here to prevent code duplication.
 *
 * \sa configure()
 */
void unconfigure()
{
    if(g_logging_type != unconfigured_logger )
    {
        // shutdown the previous version before re-configuring
        // (this is done after a fork() call.)
        log4cplus::Logger::shutdown();
        g_logging_type = unconfigured_logger;
    }
}


/** \brief Configure log4cplus system to the console.
 *
 * This function is the default called in case the user has not specified
 * a configuration file to read.
 *
 * It sets up a default appender to the standard output.
 *
 * \note
 * This function marks that the logger was configured. The other functions
 * do not work (do nothing) until this happens. In case of the server,
 * configure() is called from the server::config() function. If no configuration
 * file is defined then the other functions will do nothing.
 *
 * \sa fatal()
 * \sa error()
 * \sa warning()
 * \sa info()
 * \sa server::config()
 * \sa unconfigure()
 */
void configureConsole()
{
    unconfigure();

    log4cplus::SharedAppenderPtr
            appender(new log4cplus::ConsoleAppender());
    appender->setName(LOG4CPLUS_TEXT("console"));
    const log4cplus::tstring pattern( "%b:%L:%h: %m%n" );
    appender->setLayout( std::auto_ptr<log4cplus::Layout>( new log4cplus::PatternLayout(pattern)) );
    appender->setThreshold( log4cplus::INFO_LOG_LEVEL );

    g_log_config_filename.clear();
    g_log_output_filename.clear();
    g_logging_type    = console_logger;
    g_logger          = log4cplus::Logger::getInstance("snap");
    g_secure_logger   = log4cplus::Logger::getInstance("security");

    g_logger.addAppender( appender );
    g_secure_logger.addAppender( appender );
    setLogOutputLevel( LOG_LEVEL_INFO );        // TODO: This is broken! For some reason log4cplus won't change the threshold level...
}


/** \brief Configure log4cplus system turning on the rolling file appender.
 *
 * This function is the default called in case the user has not specified
 * a configuration file to read.
 *
 * It sets up a default appender to the standard lo
 *
 * \note
 * This function marks that the logger was configured. The other functions
 * do not work (do nothing) until this happens. In case of the server,
 * configure() is called from the server::config() function. If no configuration
 * file is defined then the other functions will do nothing.
 *
 * \param[in] logfile  The name of the configuration file.
 *
 * \sa fatal()
 * \sa error()
 * \sa warning()
 * \sa info()
 * \sa server::config()
 * \sa unconfigure()
 */
void configureLogfile( QString const& logfile )
{
    unconfigure();

    if( logfile.isEmpty() )
    {
        throw snap_exception( "No output logfile specified!" );
        NOTREACHED();
    }

    log4cplus::SharedAppenderPtr
            appender(new log4cplus::RollingFileAppender( logfile.toUtf8().data() ));
    appender->setName(LOG4CPLUS_TEXT("log_file"));
    const log4cplus::tstring pattern( "%d{%Y/%m/%d %H:%M:%S} %h Snap[%i]: %m (%b:%L)%n" );
    appender->setLayout( std::auto_ptr<log4cplus::Layout>( new log4cplus::PatternLayout(pattern)) );
    appender->setThreshold( log4cplus::INFO_LOG_LEVEL );

    g_log_config_filename.clear();
    g_log_output_filename = logfile;
    g_logging_type        = file_logger;
    g_logger              = log4cplus::Logger::getInstance("snap");
    g_secure_logger       = log4cplus::Logger::getInstance("security");

    g_logger.addAppender( appender );
    g_secure_logger.addAppender( appender );
    setLogOutputLevel( LOG_LEVEL_INFO );        // TODO: This is broken! For some reason log4cplus won't change the threshold level...
}


/** \brief Configure from a log4cplus header file.
 *
 * This function sends the specified \p filename to the log4cplus configurator
 * for initialization.
 *
 * If \p filename is empty (undefined in the server configuration file) then
 * the /etc/snapwebsites/log.conf file is used if it exists. If not, then
 * no configuration is created.
 *
 * \note
 * This function marks that the logger was configured. The other functions
 * do not work (do nothing) until this happens. In case of the server,
 * configure() is called from the server::config() function. If no configuration
 * file is defined then the other functions will do nothing.
 *
 * \param[in] filename  The name of the configuration file.
 *
 * \sa fatal()
 * \sa error()
 * \sa warning()
 * \sa info()
 * \sa server::config()
 * \sa unconfigure()
 */
void configureConffile(QString const& filename)
{
    unconfigure();

    QFileInfo info(filename);
    if(!info.exists())
    {
        throw snap_exception( QObject::tr("Cannot open logger configuration file [%1].").arg(filename) );
        NOTREACHED();
    }

    g_log_config_filename   = filename;
    g_logging_type          = conffile_logger;
    log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_C_STR_TO_TSTRING(filename.toUtf8().data()));
    g_logger                = log4cplus::Logger::getInstance("snap");
    g_secure_logger         = log4cplus::Logger::getInstance("security");
}


/** \brief Ensure that the configuration is still in place.
 *
 * On a fork() the configuration of log4cplus is lost. We have to
 * call this function again before we can use the logs again.
 *
 * \note
 * TBD -- is it really necessary to reconfigure after a fork() or
 * would the logger know how to handle that case?
 */
void reconfigure()
{
    switch( g_logging_type )
    {
    case console_logger:
        configureConsole();
        break;

    case file_logger:
        configureLogfile( g_log_output_filename );
        break;

    case conffile_logger:
        configureConffile( g_log_config_filename );
        break;

    default:
        /* do nothing */
        unconfigure();
    }
}


/** \brief Return the current configuration status.
 *
 * This function returns true if the log facility was successfully
 * configured, false otherwise.
 *
 * \return true if the configure() function was called with success.
 */
bool is_configured()
{
    return g_logging_type != unconfigured_logger;
}


/* \brief Set the current logging threshold
 *
 * Tells log4cplus to limit the logging output to the specified threshold.
 *
 * \todo This is broken! For some reason log4cplus won't change the threshold level using this method.
 */
void setLogOutputLevel( log_level_t level )
{
    log4cplus::LogLevel new_level = log4cplus::OFF_LOG_LEVEL;

    switch(level)
    {
    case LOG_LEVEL_OFF:
        new_level = log4cplus::OFF_LOG_LEVEL;
        return;

    case LOG_LEVEL_FATAL:
        new_level = log4cplus::FATAL_LOG_LEVEL;
        break;

    case LOG_LEVEL_ERROR:
        new_level = log4cplus::ERROR_LOG_LEVEL;
        break;

    case LOG_LEVEL_WARNING:
        new_level = log4cplus::WARN_LOG_LEVEL;
        break;

    case LOG_LEVEL_INFO:
        new_level = log4cplus::INFO_LOG_LEVEL;
        break;

    case LOG_LEVEL_DEBUG:
        new_level = log4cplus::DEBUG_LOG_LEVEL;
        break;

    case LOG_LEVEL_TRACE:
        new_level = log4cplus::TRACE_LOG_LEVEL;
        break;
    }

    g_logger.setLogLevel( new_level );
    g_secure_logger.setLogLevel( new_level );
}


logger& operator << ( logger& logger, const QString& msg )
{
    logger( msg );
    return logger;
}


logger& operator << ( logger& logger, const std::basic_string<char>& msg )
{
    logger( msg.c_str() );
    return logger;
}


logger& operator << ( logger& logger, const std::basic_string<wchar_t>& msg )
{
    logger( msg.c_str() );
    return logger;
}


logger& operator << ( logger& logger, const char* msg )
{
    logger( msg );
    return logger;
}


logger& operator << ( logger& logger, const wchar_t* msg )
{
    logger( msg );
    return logger;
}


/** \brief Create a log object with the specified information.
 *
 * This function generates a log object that can be used to generate
 * a log message with the () operator and then gets logged using
 * log4cplus on destruction.
 *
 * The level can be set to any one of the log levels available in
 * the log_level_t enumeration. The special LOG_LEVEL_OFF value can be
 * used to avoid the log altogether (can be handy when you support a
 * varying log level.)
 *
 * By default logs are not marked as secure. If you are creating a log
 * that should only go to the secure logger, then use the () operator
 * with the LOG_SECURITY_SECURE value as in:
 *
 * \code
 *   // use the "security" logger
 *   SNAP_LOG_FATAL(LOG_SECURITY_SECURE)("this is not authorized!");
 * \endcode
 *
 * \param[in] log_level  The level of logging.
 * \param[in] file  The name of the source file that log was generated from.
 * \param[in] func  The name of the function that log was generated from.
 * \param[in] line  The line number that log was generated from.
 */
logger::logger(log_level_t log_level, const char *file, const char *func, int line)
    : f_log_level(log_level)
    , f_file(file)
    , f_func(func)
    , f_line(line)
    , f_security(LOG_SECURITY_NONE)
    //, f_message() -- auto-init
{
}

/** \brief Output the log created with the () operators.
 *
 * The destructor of the log object is where things happen. This function
 * prints out the message that was built using the different () operators
 * and the parameters specified in the constructor.
 *
 * The snap log level is converted to a log4cplus log level (and a syslog
 * level in case log4cplus is not available.)
 *
 * If the () operator was used with LOG_SECURITY_SECURE, then the message
 * is sent using the "security" logger. Otherwise it uses the standard
 * "snap" logger.
 */
logger::~logger()
{
    log4cplus::LogLevel ll(log4cplus::FATAL_LOG_LEVEL);
    int sll(-1);  // syslog level if log4cplus not available (if -1 don't syslog() anything)
    switch(f_log_level)
    {
    case LOG_LEVEL_OFF:
        // off means we don't emit anything
        return;

    case LOG_LEVEL_FATAL:
        ll = log4cplus::FATAL_LOG_LEVEL;
        sll = LOG_CRIT;
        break;

    case LOG_LEVEL_ERROR:
        ll = log4cplus::ERROR_LOG_LEVEL;
        sll = LOG_ERR;
        break;

    case LOG_LEVEL_WARNING:
        ll = log4cplus::WARN_LOG_LEVEL;
        sll = LOG_WARNING;
        break;

    case LOG_LEVEL_INFO:
        ll = log4cplus::INFO_LOG_LEVEL;
        sll = LOG_INFO;
        break;

    case LOG_LEVEL_DEBUG:
        ll = log4cplus::DEBUG_LOG_LEVEL;
        break;

    case LOG_LEVEL_TRACE:
        ll = log4cplus::TRACE_LOG_LEVEL;
        break;

    }

    // TBD: is the exists() call doing anything for us here?
    if( (g_logging_type == unconfigured_logger) || !log4cplus::Logger::exists(f_security == LOG_SECURITY_SECURE ? "security" : "snap"))
    {
        // not even configured, return immediately
        if(sll != -1)
        {
            if(f_file == NULL)
            {
                f_file = "unknown-file";
            }
            if(f_func == NULL)
            {
                f_func = "unknown-func";
            }
            syslog(sll, "%s (%s:%s: %d)", f_message.toUtf8().data(), f_file, f_func, f_line);
        }
        return;
    }
    if(f_func != NULL)
    {
        // TBD: how should we really include the function name to the log4cplus messages?
        //
        // Note: we permit ourselves to modify f_message since we're in the destructor
        //       about to leave this object anyway.
        f_message += QString(" (in function \"%1()\")").arg(f_func);
    }

    // actually emit the log
    if(f_security == LOG_SECURITY_SECURE)
    {
        // generally this at least goes in the /var/log/syslog
        // and it may also go in a secure log file (i.e. not readable by everyone)
        g_secure_logger.log(ll, LOG4CPLUS_C_STR_TO_TSTRING(f_message.toUtf8().data()), f_file, f_line);
    }
    else
    {
        g_logger.log(ll, LOG4CPLUS_C_STR_TO_TSTRING(f_message.toUtf8().data()), f_file, f_line);
    }
}

logger& logger::operator () ()
{
    // does nothing
    return *this;
}

logger& logger::operator () (const log_security_t v)
{
    f_security = v;
    return *this;
}

logger& logger::operator () (const char *s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    f_message += QString::fromUtf8(s);
    return *this;
}

logger& logger::operator () (const wchar_t *s)
{
    f_message += QString::fromWCharArray(s);
    return *this;
}

logger& logger::operator () (const QString& s)
{
    f_message += s;
    return *this;
}

logger& logger::operator () (const char v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}

logger& logger::operator () (const signed char v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}

logger& logger::operator () (const unsigned char v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}

logger& logger::operator () (const signed short v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}

logger& logger::operator () (const unsigned short v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}

logger& logger::operator () (const signed int v)
{
    f_message += QString("%1").arg(v);
    return *this;
}

logger& logger::operator () (const unsigned int v)
{
    f_message += QString("%1").arg(v);
    return *this;
}

logger& logger::operator () (const signed long v)
{
    f_message += QString("%1").arg(v);
    return *this;
}

logger& logger::operator () (const unsigned long v)
{
    f_message += QString("%1").arg(v);
    return *this;
}

logger& logger::operator () (const signed long long v)
{
    f_message += QString("%1").arg(v);
    return *this;
}

logger& logger::operator () (const unsigned long long v)
{
    f_message += QString("%1").arg(v);
    return *this;
}

logger& logger::operator () (const float v)
{
    f_message += QString("%1").arg(v);
    return *this;
}

logger& logger::operator () (const double v)
{
    f_message += QString("%1").arg(v);
    return *this;
}

logger& logger::operator () (const bool v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}

logger fatal(const char *file, const char *func, int line)
{
    logger l(LOG_LEVEL_FATAL, file, func, line);
    l.operator () ("fatal: ");
    return l;
}

logger error(const char *file, const char *func, int line)
{
    logger l(LOG_LEVEL_ERROR, file, func, line);
    l.operator () ("error: ");
    return l;
}

logger warning(const char *file, const char *func, int line)
{
    logger l(LOG_LEVEL_WARNING, file, func, line);
    l.operator () ("warning: ");
    return l;
}

logger info(const char *file, const char *func, int line)
{
    logger l(LOG_LEVEL_INFO, file, func, line);
    l.operator () ("info: ");
    return l;
}

logger debug(const char *file, const char *func, int line)
{
    logger l(LOG_LEVEL_DEBUG, file, func, line);
    l.operator () ("debug: ");
    return l;
}

logger trace(const char *file, const char *func, int line)
{
    logger l(LOG_LEVEL_INFO, file, func, line);
    l.operator () ("trace: ");
    return l;
}


} // namespace logging

} // namespace snap

// vim: ts=4 sw=4 et
