#include "exception.h"

#include <sstream>

#include <execinfo.h>
#include <unistd.h>

namespace libexcept
{


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
 * \param[in] depth  The number of lines to grab in our stack track.
 *
 * \sa output_stack_trace()
 */
exception_base_t::exception_base_t( int const depth )
{
    collect_stack_trace( depth );
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
 * \param[in] stack_trace_depth  The number of lines to capture in our stack track.
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
        f_stack_trace << stack_string;
    }
    free( stack_string_list );
}

exception_t::exception_t( const QString&     what ) : std::runtime_error(qPrintable(what)) {}
exception_t::exception_t( const std::string& what ) : std::runtime_error(what.c_str())     {}
exception_t::exception_t( const char*        what ) : std::runtime_error(what)             {}

const char* exception_t::what() const throw() { return std::runtime_error::what(); }


}
// namespace libexcept

// vim: ts=4 sw=4 et
