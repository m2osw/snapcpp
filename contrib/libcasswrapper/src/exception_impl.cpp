#include "exception_impl.h"

#include <sstream>

#include <cassandra.h>

namespace casswrapper
{


exception_t::exception_t( const QString&     what ) : std::runtime_error(qPrintable(what)) {}
exception_t::exception_t( const std::string& what ) : std::runtime_error(what.c_str())     {}
exception_t::exception_t( const char*        what ) : std::runtime_error(what)             {}

cassandra_exception_t::cassandra_exception_t( const casswrapper::future& future, QString const& msg )
    : f_code(future.get_error_code())
    , f_errmsg(future.get_error_message())
    , f_message(msg)
{
    init();
}


cassandra_exception_t::cassandra_exception_t( const QString& msg, CassError rc )
    : f_code(rc)
    , f_message(msg)
{
    init();
}


cassandra_exception_t::cassandra_exception_t( CassError rc, const QString& msg )
    : f_code(rc)
    , f_message(msg)
{
    init();
}


void cassandra_exception_t::init()
{
    std::stringstream ss;
    ss << f_message.toUtf8().data()
       << "! Cassandra error: code=" << static_cast<uint32_t>(f_code)
       << ", error={" << cass_error_desc(f_code);
    if( !f_errmsg.isEmpty() )
    {
       ss << "}, error message={" << f_errmsg.toUtf8().data();
    }
    ss << "} aborting operation!";
    f_what = ss.str();
}


const char* cassandra_exception_t::what() const throw()
{
    return f_what.c_str();
}


}
// namespace casswrapper

// vim: ts=4 sw=4 et
