/*
 * Header:
 *      exception.h
 *
 * Description:
 *      The exception base class declarations.
 *
 * Documentation:
 *      See the corresponding .cpp file.
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
#pragma once

// C++ includes
//
#include <stdexcept>
#include <string>

// Qt includes
//
#include <QString>
#include <QStringList>


namespace libexcept
{


class exception_base_t
{
public:
    static int const            STACK_TRACE_DEPTH = 20;

                                explicit exception_base_t();

    virtual                     ~exception_base_t() {}

    QStringList const &         get_stack_trace() const { return f_stack_trace; }

    static void                 collect_stack_trace( QStringList & stack_strace, int stack_track_depth = STACK_TRACE_DEPTH);

private:
    QStringList                 f_stack_trace;
};


class exception_t : public std::runtime_error, public exception_base_t
{
public:
                                explicit exception_t( QString const &     what );
                                explicit exception_t( std::string const & what );
                                explicit exception_t( char const *        what );

    virtual                     ~exception_t() override {}

    virtual char const *        what() const throw() override;
};


}
// namespace libexcept

// vim: ts=4 sw=4 et
