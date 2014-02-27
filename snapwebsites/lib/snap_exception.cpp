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
#include "log.h"

#include <iostream>

#include <execinfo.h>
#include <unistd.h>

#include "poison.h"



namespace snap
{

namespace
{
    // TODO: we can at some point put this into the configuration file.
    //
    const int STACK_TRACE_DEPTH = 20;
}


/** \brief Initialize this Snap! exception.
 *
 * Initialize the base exception class. Output a stack trace to the error log.
 *
 * \sa output_stack_trace()
 */
snap_exception_base::snap_exception_base()
{
    output_stack_trace();
}


/** \brief Output stack trace to log as an error.
 *
 * This static method outputs the current stack as a trace to the log. If
 * compiled with DEBUG turned on, it will also output to the stderr.
 */
void snap_exception_base::output_stack_trace()
{
    void *array[STACK_TRACE_DEPTH];
    size_t const size = backtrace( array, STACK_TRACE_DEPTH );

    // Output to log
    //
    char **stack_string_list = backtrace_symbols( array, size );
    for( size_t idx = 0; idx < size; ++idx )
    {
        const char* stack_string( stack_string_list[idx] );
        SNAP_LOG_ERROR("snap_exception_base(): backtrace=")(stack_string);
    }
    free( stack_string_list );
}


} // namespace snap
// vim: ts=4 sw=4 et
