/*
 * Implementation:
 *      exception.cpp
 *
 * Description:
 *      The implementation that gathers the stack whenever an exception occurs.
 *      This is particularly useful to debug exceptions and possibly fix the
 *      code quickly.
 *
 * License:
 *      Copyright (c) 2011-2016 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "exception.h"

// C++ includes
//
#include <sstream>

// C lib includes
//
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
 * Initialize the base exception class by generating the output of
 * a stack trace to a list of strings.
 *
 * \warning
 * At this time every single exception derived from exception_t generates
 * a stack trace. Note that in most cases, our philosophy is to generate
 * exceptions only in very exceptional cases and not on every single error
 * so the event should be rare in a normal run of our daemons.
 *
 * \sa collect_stack_trace()
 */
exception_base_t::exception_base_t()
{
    collect_stack_trace( f_stack_trace );
}


/** \brief Collect the stack trace in a list of strings.
 *
 * This static method collects the current stack as a trace to log later.
 *
 * By default, the stack trace shows you a number of backtrace equal
 * to STACK_TRACE_DEPTH (which is 20 at time of writing). You may
 * specify another number to get more or less lines. Note that a
 * really large number will generally show you the entire stack since
 * a number larger than the number of function pointers on the stack
 * will return the entire stack.
 *
 * \param[in,out] stack_trace  The list of strings were the stack is added.
 * \param[in] stack_trace_depth  The number of lines to collect.
 */
void exception_base_t::collect_stack_trace( QStringList & stack_trace, int stack_trace_depth )
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
        stack_trace << stack_string;
    }
    free( stack_string_list );
}


/** \brief Initialize an exception from a QString.
 *
 * This function initializes an exception settings its 'what' string to
 * the specified \p what parameter.
 *
 * \param[in] what  The string used to initialize the exception what parameter.
 */
exception_t::exception_t( const QString & what )
    : std::runtime_error(qUtf8Printable(what))
{
}


/** \brief Initialize an exception from a C++ string.
 *
 * This function initializes an exception settings its 'what' string to
 * the specified \p what parameter.
 *
 * \param[in] what  The string used to initialize the exception what parameter.
 */
exception_t::exception_t( const std::string & what )
    : std::runtime_error(what.c_str())
{
}


/** \brief Initialize an exception from a C string.
 *
 * This function initializes an exception settings its 'what' string to
 * the specified \p what parameter.
 *
 * \param[in] what  The string used to initialize the exception what parameter.
 */
exception_t::exception_t( const char * what )
    : std::runtime_error(what)
{
}


/** \brief Retrieve the `what` parameter as passed to the constructor.
 *
 * This function returns the what description of the exception when the
 * exception was initialized.
 *
 * \note
 * We have an overload because of the dual derivation.
 *
 * \return A pointer to the what string. Must be used before the exception
 *         gets destructed.
 */
char const * exception_t::what() const throw()
{
    return std::runtime_error::what();
}


}
// namespace libexcept

// vim: ts=4 sw=4 et
