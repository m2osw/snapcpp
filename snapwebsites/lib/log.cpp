// Snap Websites Server -- log services
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

#include "log.h"

#include "not_reached.h"
#include "snap_exception.h"
#include "snapwebsites.h"

#include <syslog.h>

#include <boost/algorithm/string/replace.hpp>

#include <log4cplus/configurator.h>
#include <log4cplus/logger.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/syslogappender.h>

#include <QFileInfo>

#include "poison.h"


/** \file
 * \brief Handle logging in the Snap environment.
 *
 * The snap::logging namespace defines a set of functions and classes used
 * to setup the snap logger that can be easily accessed with the following
 * macros:
 *
 * \li SNAP_LOG_FATAL -- output what is viewed as fatal error
 * \li SNAP_LOG_ERROR -- output an error
 * \li SNAP_LOG_WARNING -- output a warning
 * \li SNAP_LOG_INFO -- output some information
 * \li SNAP_LOG_DEBUG -- output debug information
 * \li SNAP_LOG_TRACE -- output trace information
 *
 * The macros should be used so that way you include the filename and line
 * number of where the message is generated from. That information is then
 * available to be printed in the logs.
 *
 * The macros define a logger object that accepts messages with either the
 * << operator or the () operator, both of which support null terminated
 * C strings (char and wchar_t), QString, std::string, std::wstring,
 * and all basic types (integers and floats).
 *
 * The () operator also accepts the security enumeration as input, so you
 * can change the level to SECURE at any time when you generate a log.
 *
 * \code
 *      SNAP_LOG_INFO("User password is: ")
 *              (snap::logging::log_security_t::LOG_SECURITY_SECURE)
 *              (password);
 *
 *      SNAP_LOG_FATAL("We could not read resources: ") << filename;
 * \endcode
 *
 * Try to remember that the \\n character is not necessary. The logger
 * will automatically add a newline at the end of each log message.
 *
 * \note
 * The newer versions of the log4cplus library offer a very similar set
 * of macros. These macro, though, do not properly check out all of
 * our flags and levels so you should avoid them for now.
 *
 * To setup the logging system, the snapserver makes use of up to three
 * files:
 *
 * \li logserver.properties
 * \li log.properties
 * \li loggingserver.properties
 * \li snapcgilog.properties
 *
 * The path and filename of the logserver.properties file is defined
 * in the snapserver.conf file under the variable name log_server
 *
 * \code
 *      log_server=/etc/snapwebsites/logserver.properties
 * \endcode
 *
 * The loggingserver may not be running so the snapserver first checks
 * the availability. If available, then it uses it. If it cannot be
 * found, then it insteads tries with the log.properties file, of more
 * exactly the file defined in log_config of the snapserver.conf file.
 *
 * \code
 *      log_config=/etc/snapwebsites/log.properties
 * \endcode
 *
 * The loggingserver itself will make use of the loggingserver.properties
 * file. This is expected to be setup in the script starting the server.
 * The filename and path are given on the command line:
 *
 * \code
 *      loggingserver 9998 /etc/snapwebsites/loggingserver.properties
 * \endcode
 *
 * The backends run just like the snapserver so they get the same logger
 * settings.
 *
 * The snap.cgi tool, however, has its own setup. It first checks the
 * command line, and if no configuration is defined on the command
 * line it uses the log_config=... parameter from the snapcgi.conf
 * file. The default file is snapcgilog.properties.
 *
 * \code
 *      log_config=/etc/snapwebsites/snapcgilog.properties
 * \endcode
 *
 * \sa log4cplus/include/log4cplus/loggingmacros.h
 */

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

    enum class logging_type_t
        { UNCONFIGURED_LOGGER
        , CONSOLE_LOGGER
        , FILE_LOGGER
        , CONFFILE_LOGGER
        , SYSLOG_LOGGER
        };
    logging_type_t      g_logging_type( logging_type_t::UNCONFIGURED_LOGGER );
    logging_type_t      g_last_logging_type( logging_type_t::UNCONFIGURED_LOGGER );
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
    if( g_logging_type != logging_type_t::UNCONFIGURED_LOGGER )
    {
        // shutdown the previous version before re-configuring
        // (this is done after a fork() call.)
        //
        log4cplus::Logger::shutdown();
        g_logging_type = logging_type_t::UNCONFIGURED_LOGGER;
        //g_last_logging_type = ... -- keep the last valid configuration
        //  type so we can call reconfigure() and get it back "as expected"
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
 * Format documentation:
 * http://log4cplus.sourceforge.net/docs/html/classlog4cplus_1_1PatternLayout.html
 *
 * \sa fatal()
 * \sa error()
 * \sa warning()
 * \sa info()
 * \sa server::config()
 * \sa unconfigure()
 */
void configure_console()
{
    unconfigure();

    log4cplus::SharedAppenderPtr
            appender(new log4cplus::ConsoleAppender());
    appender->setName(LOG4CPLUS_TEXT("console"));
    const log4cplus::tstring pattern
                ( boost::replace_all_copy(server::instance()->servername(), "%", "%%").c_str()
                + log4cplus::tstring("[%i]:%b:%L:%h: %m%n")
                );
    //const log4cplus::tstring pattern( "%b:%L:%h: %m%n" );
// log4cplus only accepts std::auto_ptr<> which is deprecated in newer versions
// of g++ so we have to make sure the deprecation definition gets ignored
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    appender->setLayout( std::auto_ptr<log4cplus::Layout>( new log4cplus::PatternLayout(pattern)) );
#pragma GCC diagnostic pop
    appender->setThreshold( log4cplus::INFO_LOG_LEVEL );

    g_log_config_filename.clear();
    g_log_output_filename.clear();
    g_logging_type       = logging_type_t::CONSOLE_LOGGER;
    g_last_logging_type  = logging_type_t::CONSOLE_LOGGER;
    g_logger             = log4cplus::Logger::getInstance("snap");
    g_secure_logger      = log4cplus::Logger::getInstance("security");

    g_logger.addAppender( appender );
    g_secure_logger.addAppender( appender );
    set_log_output_level( log_level_t::LOG_LEVEL_INFO );        // TODO: This is broken! For some reason log4cplus won't change the threshold level...
}


/** \brief Configure log4cplus system turning on the rolling file appender.
 *
 * This function is called when the user has specified to write logs to a file.
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
void configure_logfile( QString const & logfile )
{
    unconfigure();

    if( logfile.isEmpty() )
    {
        throw snap_exception( "No output logfile specified!" );
    }

    log4cplus::SharedAppenderPtr
            appender(new log4cplus::RollingFileAppender( logfile.toUtf8().data() ));
    appender->setName(LOG4CPLUS_TEXT("log_file"));
    log4cplus::tstring const pattern
                ( log4cplus::tstring("%d{%Y/%m/%d %H:%M:%S} %h ")
                + boost::replace_all_copy(server::instance()->servername(), "%", "%%").c_str()
                + log4cplus::tstring("[%i]: %m (%b:%L)%n")
                );
// log4cplus only accepts std::auto_ptr<> which is deprecated in newer versions
// of g++ so we have to make sure the deprecation definition gets ignored
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    appender->setLayout( std::auto_ptr<log4cplus::Layout>( new log4cplus::PatternLayout(pattern)) );
#pragma GCC diagnostic pop
    appender->setThreshold( log4cplus::INFO_LOG_LEVEL );

    g_log_config_filename.clear();
    g_log_output_filename = logfile;
    g_logging_type        = logging_type_t::FILE_LOGGER;
    g_last_logging_type   = logging_type_t::FILE_LOGGER;
    g_logger              = log4cplus::Logger::getInstance("snap");
    g_secure_logger       = log4cplus::Logger::getInstance("security");

    g_logger.addAppender( appender );
    g_secure_logger.addAppender( appender );
    set_log_output_level( log_level_t::LOG_LEVEL_INFO );        // TODO: This is broken! For some reason log4cplus won't change the threshold level...
}


/** \brief Configure log4cplus system to the syslog.
 *
 * Set up the logging to be routed to the syslog.
 *
 * \note
 * This function marks that the logger was configured. The other functions
 * do not work (do nothing) until this happens. In case of the server,
 * configure() is called from the server::config() function. If no configuration
 * file is defined then the other functions will do nothing.
 *
 * Format documentation:
 * http://log4cplus.sourceforge.net/docs/html/classlog4cplus_1_1PatternLayout.html
 *
 * \sa fatal()
 * \sa error()
 * \sa warning()
 * \sa info()
 * \sa server::config()
 * \sa unconfigure()
 */
void configure_sysLog()
{
    unconfigure();

    const std::string servername( server::instance()->servername() );
    log4cplus::SharedAppenderPtr appender( new log4cplus::SysLogAppender( servername ) );
    const log4cplus::tstring pattern
                ( boost::replace_all_copy(servername, "%", "%%").c_str()
                + log4cplus::tstring("[%i]:%b:%L:%h: %m%n")
                );
// log4cplus only accepts std::auto_ptr<> which is deprecated in newer versions
// of g++ so we have to make sure the deprecation definition gets ignored
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    appender->setLayout( std::auto_ptr<log4cplus::Layout>( new log4cplus::PatternLayout(pattern)) );
#pragma GCC diagnostic pop
    appender->setThreshold( log4cplus::INFO_LOG_LEVEL );

    g_log_config_filename.clear();
    g_log_output_filename.clear();
    g_logging_type        = logging_type_t::SYSLOG_LOGGER;
    g_last_logging_type   = logging_type_t::SYSLOG_LOGGER;
    g_logger              = log4cplus::Logger::getInstance("snap");
    g_secure_logger       = log4cplus::Logger::getInstance("security");

    g_logger.addAppender( appender );
    g_secure_logger.addAppender( appender );
    set_log_output_level( log_level_t::LOG_LEVEL_INFO );        // TODO: This is broken! For some reason log4cplus won't change the threshold level...
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
void configure_conffile(QString const & filename)
{
    unconfigure();

    QFileInfo info(filename);
    if(!info.exists())
    {
        throw snap_exception( QObject::tr("Cannot open logger configuration file [%1].").arg(filename) );
    }

    g_log_config_filename   = filename;
    g_logging_type          = logging_type_t::CONFFILE_LOGGER;
    g_last_logging_type     = logging_type_t::CONFFILE_LOGGER;

    // note the doConfigure() may throw if the log.properties is invalid
    //
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
    switch( g_last_logging_type )
    {
    case logging_type_t::CONSOLE_LOGGER:
        configure_console();
        break;

    case logging_type_t::FILE_LOGGER:
        configure_logfile( g_log_output_filename );
        break;

    case logging_type_t::CONFFILE_LOGGER:
        configure_conffile( g_log_config_filename );
        break;

    case logging_type_t::SYSLOG_LOGGER:
        configure_sysLog();
        break;

    default:
        /* do nearly nothing */
        unconfigure();
        break;

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
    return g_logging_type != logging_type_t::UNCONFIGURED_LOGGER;
}


/* \brief Set the current logging threshold
 *
 * Tells log4cplus to limit the logging output to the specified threshold.
 *
 * \todo This is broken! For some reason log4cplus won't change the threshold level using this method.
 */
void set_log_output_level( log_level_t level )
{
    log4cplus::LogLevel new_level = log4cplus::OFF_LOG_LEVEL;

    switch(level)
    {
    case log_level_t::LOG_LEVEL_OFF:
        new_level = log4cplus::OFF_LOG_LEVEL;
        return;

    case log_level_t::LOG_LEVEL_FATAL:
        new_level = log4cplus::FATAL_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_ERROR:
        new_level = log4cplus::ERROR_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_WARNING:
        new_level = log4cplus::WARN_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_INFO:
        new_level = log4cplus::INFO_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_DEBUG:
        new_level = log4cplus::DEBUG_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_TRACE:
        new_level = log4cplus::TRACE_LOG_LEVEL;
        break;

    }

    log4cplus::Logger::getRoot().setLogLevel( new_level );
    g_logger.setLogLevel( new_level );
    g_secure_logger.setLogLevel( new_level );
}


/** \brief Check whether the loggingserver is available.
 *
 * This function quickly checks whether the loggingserver is running
 * with "our port".
 *
 * If the server is available, then it gets used. This is generally only
 * checked in the server. Snap children will use the loggingserver if the
 * Snap server is setup to use the loggingserver.
 *
 * \param[in] logserver  The filename (and path) to the logging server
 *                       appender properties.
 *
 * \return true if the logging server is currently running.
 */
bool is_loggingserver_available ( QString const & logserver )
{
    // Note: if logserver is an empty string we assume that the logging
    //       server was not setup; otherwise the following may actually
    //       return true which is wrong in this case

    if( logserver.isEmpty() )
    {
        return false;
    }

    // get the address and port from the logserver.properties file
    log4cplus::helpers::Properties logserver_properties(logserver.toUtf8().data());

    // check properties that make use of the log4cplus::SocketAppender
    // these may have any name even if we use "server" by default
    std::vector<log4cplus::tstring> names(logserver_properties.propertyNames());
    for(auto & n : names)
    {
        // the string must start with "log4cplus.appender."
        log4cplus::tstring prefix(n.substr(0, 19));
        if(prefix == "log4cplus.appender.")
        {
            log4cplus::tstring name(logserver_properties.getProperty(n));
            if( name == LOG4CPLUS_TEXT("log4cplus::SocketAppender") )
            {
                // this is a server, check for availability
                //log4cplus::helpers::Properties socket_properties(logserver_properties.getPropertySubset(n + "."));
                log4cplus::tstring const host(logserver_properties.getProperty(n + LOG4CPLUS_TEXT(".host")));
                unsigned int port(0);
                logserver_properties.getUInt(port, n + LOG4CPLUS_TEXT(".port"));
                //log4cplus::tstring const server_name(logserver_properties.getProperty(n + LOG4CPLUS_TEXT(".ServerName"))); -- not necessary for our test

                if(!host.empty() && port < 65536)
                {
                    log4cplus::helpers::Socket socket(host, port);
                    if(socket.isOpen())
                    {
                        log4cplus::helpers::SocketBuffer version_request(sizeof(unsigned int));
                        // -2 is a version request from the loggingserver executable
                        version_request.appendInt(static_cast<unsigned int>(-2));
                        if(socket.write(version_request))
                        {
                            // read reply size
                            log4cplus::helpers::SocketBuffer version_size(sizeof(unsigned int));
                            if(socket.read(version_size))
                            {
                                log4cplus::helpers::SocketBuffer version(version_size.readInt());
                                if(socket.read(version))
                                {
//std::cerr << "*** found server at [" << host << "] and [" << port << "] -> [" << std::string(version.getBuffer(), version.getSize()) << "]\n";
                                    // this socket appender works
                                    // TODO: test that the version is compatible?
                                    //std::string version_str(version.getBuffer(), version.getSize());
                                    //if(version_str != log4cplus::versionStr) ...
                                    continue;
                                }
                            }
                        }
                    }
                }

                // if any one socket appender fails, we want to avoid
                // loggingserver(s); that way we avoid long waits trying to
                // connect each time we create a new snap_child process
                return false;
            }
        }
    }

    // all appenders are A-Okay
    // if all appenders are something else than a socket appender, then
    // of course we will always return true
    return true;
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
logger::logger(log_level_t log_level, char const *file, char const *func, int line)
    : f_log_level(log_level)
    , f_file(file)
    , f_func(func)
    , f_line(line)
    , f_security(log_security_t::LOG_SECURITY_NONE)
    //, f_message() -- auto-init
    //, f_ignore(false) -- auto-init
{
}


/** \brief Create a copy of this logger instance.
 *
 * This function creates a copy of the logger instance. This happens when
 * you use the predefined fatal(), error(), warning(), ... functions since
 * the logger instantiated inside the function is returned and thus copied
 * once or twice (the number of copies will depend on the way the compiler
 * is capable of optimizing our work.)
 *
 * \note
 * The copy has a side effect on the input logger: it marks it as "please
 * ignore that copy" so its destructor does not print out anything.
 *
 * \param[in] l  The logger to duplicate.
 */
logger::logger(logger const& l)
    : f_log_level(l.f_log_level)
    , f_file(l.f_file)
    , f_func(l.f_func)
    , f_line(l.f_line)
    , f_security(l.f_security)
    , f_message(l.f_message)
{
    l.f_ignore = true;
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
    if(f_ignore)
    {
        // someone made a copy, this version we ignore
        return;
    }

    log4cplus::LogLevel ll(log4cplus::FATAL_LOG_LEVEL);
    int sll(-1);  // syslog level if log4cplus not available (if -1 don't syslog() anything)
    bool console(false);
    char const *level_str(nullptr);
    switch(f_log_level)
    {
    case log_level_t::LOG_LEVEL_OFF:
        // off means we don't emit anything
        return;

    case log_level_t::LOG_LEVEL_FATAL:
        ll = log4cplus::FATAL_LOG_LEVEL;
        sll = LOG_CRIT;
        console = true;
        level_str = "fatal error";
        break;

    case log_level_t::LOG_LEVEL_ERROR:
        ll = log4cplus::ERROR_LOG_LEVEL;
        sll = LOG_ERR;
        console = true;
        level_str = "error";
        break;

    case log_level_t::LOG_LEVEL_WARNING:
        ll = log4cplus::WARN_LOG_LEVEL;
        sll = LOG_WARNING;
        console = true;
        level_str = "warning";
        break;

    case log_level_t::LOG_LEVEL_INFO:
        ll = log4cplus::INFO_LOG_LEVEL;
        sll = LOG_INFO;
        break;

    case log_level_t::LOG_LEVEL_DEBUG:
        ll = log4cplus::DEBUG_LOG_LEVEL;
        break;

    case log_level_t::LOG_LEVEL_TRACE:
        ll = log4cplus::TRACE_LOG_LEVEL;
        break;

    }

    // TBD: is the exists() call doing anything for us here?
    if( (g_logging_type == logging_type_t::UNCONFIGURED_LOGGER)
    ||  !log4cplus::Logger::exists(log_security_t::LOG_SECURITY_SECURE == f_security ? "security" : "snap"))
    {
        // if not even configured, return immediately
        if(sll != -1)
        {
            if(!f_file)
            {
                f_file = "unknown-file";
            }
            if(!f_func)
            {
                f_func = "unknown-func";
            }
            syslog(sll, "%s (%s:%s: %d)", f_message.toUtf8().data(), f_file.get(), f_func.get(), static_cast<int32_t>(f_line));
        }
    }
    else
    {
        if(!f_func)
        {
            // TBD: how should we really include the function name to the log4cplus messages?
            //
            // Note: we permit ourselves to modify f_message since we are in the destructor
            //       about to leave this object anyway.
            f_message += QString(" (in function \"%1()\")").arg(f_func);
        }

        // actually emit the log
        if(log_security_t::LOG_SECURITY_SECURE == f_security)
        {
            // generally this at least goes in the /var/log/syslog
            // and it may also go in a secure log file (i.e. not readable by everyone)
            //
            g_secure_logger.log(ll, LOG4CPLUS_C_STR_TO_TSTRING(f_message.toUtf8().data()), f_file, f_line);
        }
        else
        {
            g_logger.log(ll, LOG4CPLUS_C_STR_TO_TSTRING(f_message.toUtf8().data()), f_file, f_line);

            // full logger used, do not report error in console, logger can
            // do it if the user wants to
            //
            console = false;
        }
    }

    if(console && isatty(fileno(stdout)))
    {
        std::cerr << level_str << ":" << f_file.get() << ":" << f_line << ": " << f_message.toUtf8().data() << std::endl;
    }
}


logger& logger::operator () ()
{
    // does nothing
    return *this;
}


logger& logger::operator () (log_security_t const v)
{
    f_security = v;
    return *this;
}


logger& logger::operator () (char const *s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    // TODO: change control characters to \xXX
    f_message += QString::fromUtf8(s);
    return *this;
}


logger& logger::operator () (wchar_t const *s)
{
    // TODO: change control characters to \xXX
    f_message += QString::fromWCharArray(s);
    return *this;
}


logger& logger::operator () (std::string const& s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    // TODO: change control characters to \xXX
    f_message += QString::fromUtf8(s.c_str());
    return *this;
}


logger& logger::operator () (std::wstring const& s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    // TODO: change control characters to \xXX
    f_message += QString::fromWCharArray(s.c_str());
    return *this;
}


logger& logger::operator () (QString const& s)
{
    // TODO: change control characters to \xXX
    f_message += s;
    return *this;
}


logger& logger::operator () (char const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger& logger::operator () (signed char const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger& logger::operator () (unsigned char const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger& logger::operator () (signed short const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger& logger::operator () (unsigned short const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger& logger::operator () (signed int const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger& logger::operator () (unsigned int const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger& logger::operator () (signed long const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger& logger::operator () (unsigned long const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger& logger::operator () (signed long long const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger& logger::operator () (unsigned long long const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger& logger::operator () (float const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger& logger::operator () (double const v)
{
    f_message += QString("%1").arg(v);
    return *this;
}


logger& logger::operator () (bool const v)
{
    f_message += QString("%1").arg(static_cast<int>(v));
    return *this;
}


logger& operator << ( logger& l, QString const& msg )
{
    return l( msg );
}


logger& operator << ( logger& l, std::basic_string<char> const& msg )
{
    return l( msg );
}


logger& operator << ( logger& l, std::basic_string<wchar_t> const& msg )
{
    return l( msg );
}


logger& operator << ( logger& l, char const* msg )
{
    return l( msg );
}


logger& operator << ( logger& l, wchar_t const* msg )
{
    return l( msg );
}


logger fatal(char const *file, char const *func, int line)
{
    logger l(log_level_t::LOG_LEVEL_FATAL, file, func, line);
    return l.operator () ("fatal error: ");
}

logger error(char const *file, char const *func, int line)
{
    logger l(log_level_t::LOG_LEVEL_ERROR, file, func, line);
    return l.operator () ("error: ");
}

logger warning(char const *file, char const *func, int line)
{
    logger l(log_level_t::LOG_LEVEL_WARNING, file, func, line);
    return l.operator () ("warning: ");
}

logger info(char const *file, char const *func, int line)
{
    logger l(log_level_t::LOG_LEVEL_INFO, file, func, line);
    return l.operator () ("info: ");
}

logger debug(char const *file, char const *func, int line)
{
    logger l(log_level_t::LOG_LEVEL_DEBUG, file, func, line);
    return l.operator () ("debug: ");
}

logger trace(char const *file, char const *func, int line)
{
    logger l(log_level_t::LOG_LEVEL_INFO, file, func, line);
    return l.operator () ("trace: ");
}


} // namespace logging

} // namespace snap

// vim: ts=4 sw=4 et
