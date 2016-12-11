#include "exception_impl.h"

#include <sstream>

#include <cassandra.h>

#include <execinfo.h>
#include <unistd.h>

namespace casswrapper
{


#if 0
//
// Remarked out for now. I don't think we need stack tracing here.
// Plus, having the exception code appear hear means that every project
// that links against libQtCassandra will be forced to also link against
// libcasswrapper. My hope one day soon is to have the tie completely
// severed--which will happen when libQtCassandra no longer needs to enumerate
// the schema meta data.
//
/** \brief Initialize this Snap! exception.
 *
 * Initialize the base exception class. Output a stack trace to the error log.
 *
 * \sa output_stack_trace()
 */
exception_base_t::exception_base_t()
{
    collect_stack_trace();
}


/** \brief Output stack trace to log as an error.
 *
 * This static method outputs the current stack as a trace to the log. If
 * compiled with DEBUG turned on, it will also output to the stderr.
 *
 * By default, the stack trace shows you a number of backtrace equal
 * to STACK_TRACE_DEPTH (which is 20 at time of writing). You may
 * specify another number to get more or less lines. Note that a
 * really large number will generally show you the entire stack since
 * a number larger than the number of function pointers on the stack
 * will return the entire stack.
 *
 * \param[in] stack_trace_depth  The number of lines to output in our stack track.
 */
void exception_base_t::collect_stack_trace( int stack_trace_depth )
{
    std::vector<void *> array;
    array.resize( stack_trace_depth );
    int const size(backtrace( &array[0], stack_trace_depth ));

    // Output to log
    //
    char ** stack_string_list(backtrace_symbols( &array[0], size ));
    for( int idx = 0; idx < size; ++idx )
    {
        char const * stack_string( stack_string_list[idx] );
        //SNAP_LOG_ERROR("exception_base_t(): backtrace=")( stack_string );
        f_stack_trace << stack_string;
    }
    free( stack_string_list );
}

exception_t::exception_t( const QString&     what ) : std::runtime_error(qPrintable(what)) {}
exception_t::exception_t( const std::string& what ) : std::runtime_error(what.c_str())     {}
exception_t::exception_t( const char*        what ) : std::runtime_error(what)             {}

const char* exception_t::what() const throw() { return std::runtime_error::what(); }
#endif

cassandra_exception_impl::cassandra_exception_impl( const casswrapper::future& future, QString const& msg )
    : f_code(future.get_error_code())
    , f_errmsg(future.get_error_message())
    , f_message(msg)
{
    init();
}


cassandra_exception_impl::cassandra_exception_impl( const QString& msg, CassError rc )
    : f_code(rc)
    , f_message(msg)
{
    init();
}


cassandra_exception_impl::cassandra_exception_impl( CassError rc, const QString& msg )
    : f_code(rc)
    , f_message(msg)
{
    init();
}


void cassandra_exception_impl::init()
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


const char* cassandra_exception_impl::what() const throw()
{
    return f_what.c_str();
}


}
// namespace casswrapper

// vim: ts=4 sw=4 et
