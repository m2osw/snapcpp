/*
 * Text:
 *      src/exception_impl.cpp
 *
 * Description:
 *      Handling of the connection to the Cassandra database via the
 *      cassandra-cpp-driver API.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2017 Made to Order Software Corp.
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
#include "exception_impl.h"

#include <sstream>

#include <cassandra.h>

#include <execinfo.h>
#include <unistd.h>

namespace casswrapper
{


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
