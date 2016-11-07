#include "casswrapper/exception.h"
#include "CassWrapperImpl.h"

#include <sstream>

#include <cassandra.h>

namespace CassWrapper
{


cassandra_exception_t::cassandra_exception_t( const CassWrapper::future& future, QString const& msg )
    : f_message(msg)
{
    const CassError code( future.get_error_code() );
    f_code   = static_cast<uint32_t>(code);
    f_errmsg = future.get_error_message();

    std::stringstream ss;
    ss << msg.toUtf8().data() << "! Cassandra error: code=" << f_code
       << ", error={" << cass_error_desc(code)
       << "}, message={" << f_errmsg.toUtf8().data()
       << "} aborting operation!";
    f_what = ss.str();
}


const char* cassandra_exception_t::what() const throw()
{
    return f_what.c_str();
}


}
// namespace CassWrapper

// vim: ts=4 sw=4 et
