#include "QtCassandra/QCassandraException.h"

#include <sstream>

#include <execinfo.h>
#include <unistd.h>

namespace QtCassandra
{


/** \brief Initialize this Snap! exception.
 *
 * Initialize the base exception class. Output a stack trace to the error log.
 *
 * \sa output_stack_trace()
 */
QCassandraExceptionBase::QCassandraExceptionBase()
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
void QCassandraExceptionBase::collect_stack_trace( int stack_trace_depth )
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


QCassandraException::QCassandraException( const QString&     what ) : std::runtime_error(qPrintable(what)) {}
QCassandraException::QCassandraException( const std::string& what ) : std::runtime_error(what.c_str())     {}
QCassandraException::QCassandraException( const char*        what ) : std::runtime_error(what)             {}


QCassandraLogicException::QCassandraLogicException( const QString&     what ) : QCassandraException(qPrintable(what)) {}
QCassandraLogicException::QCassandraLogicException( const std::string& what ) : QCassandraException(what.c_str())     {}
QCassandraLogicException::QCassandraLogicException( const char*        what ) : QCassandraException(what)             {}


QCassandraOverflowException::QCassandraOverflowException( const QString&     what ) : QCassandraException(qPrintable(what)) {}
QCassandraOverflowException::QCassandraOverflowException( const std::string& what ) : QCassandraException(what.c_str())     {}
QCassandraOverflowException::QCassandraOverflowException( const char*        what ) : QCassandraException(what)             {}


}
// namespace QtCassandra

// vim: ts=4 sw=4 et
