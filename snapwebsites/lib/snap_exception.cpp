// Snap Websites Server -- snap exception handling
// Copyright (C) 2014  Made to Order Software Corp.
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

#include "snap_exception.h"

#include <iostream>

#include <execinfo.h>
#include <unistd.h>

#include "log.h"


namespace snap
{

#if 0
namespace
{
bool g_debug = false;
}
#endif


/** \brief Initialize this Snap! exception.
 *
 * This function checks whether the server was started in debug mode. If so,
 * then all Snap! exceptions get their stack trace printed in stderr for
 * debug purposes.
 *
 * \todo
 * Implement a way to switch off debuging to the log file in a production build.
 */
snap_exception_base::snap_exception_base()
{
    int const max_stack_length(1024);
    void *array[max_stack_length];
    size_t const size = backtrace(array, max_stack_length);

    // Output to stderr
    //
#ifdef DEBUG
    std::cerr << "Callstack after exception:" << std::endl;
    backtrace_symbols_fd( array, size, STDERR_FILENO );
#endif

    // Output to log
    char **stack_string_list = backtrace_symbols( array, size );
    for( size_t idx = 0; idx < size; ++idx )
    {
        const char* stack_string( stack_string_list[idx] );
        SNAP_LOG_ERROR("snap_exception_base(): backtrace=")(stack_string);
    }
    free( stack_string_list );
}


#if 0
/** \brief Set the debug flag.
 *
 * This function is used to set the debug flag used to know whether the
 * stack should be printed out on a throw. By default, the stack does not
 * get printed.
 *
 * This makes use of the --debug (-d) command line flag used on the
 * command line of the server:
 *
 * \code
 *   snapserver -d
 * \endcode
 *
 * It is set just after the command line gets parsed, so very early on in
 * the process and it is not likely that an exception will be missed.
 *
 * \important
 * Note that at this point all Snap exceptions are printed in this way.
 * So to avoid problems, we strongly advise to always avoid exceptions
 * unless something important happened that cannot be fixed up. In other
 * words, if it is logical and safe to call die(), do that instead of
 * throwing an exception.
 *
 * \param[in] debug  Whether the server was started in debug mode (true)
 *                   or not (false).
 */
void snap_exception_base::set_debug(bool const debug)
{
    g_debug = debug;
}
#endif


} // namespace snap
// vim: ts=4 sw=4 et
