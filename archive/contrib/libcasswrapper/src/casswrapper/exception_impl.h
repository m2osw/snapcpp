/*
 * Text:
 *      src/exception_impl.h
 *
 * Description:
 *      Implementation of base exceptions.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
 *
 *      https://snapwebsites.org/
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

#include <stdexcept>
#include <string>

#include <QString>

#include "casswrapper/exception.h"
#include "casswrapper_impl.h"

namespace casswrapper
{


class cassandra_exception_impl : public cassandra_exception_t
{
public:
                            cassandra_exception_impl( const casswrapper::future& future, const QString& msg );
                            cassandra_exception_impl( const QString& what, CassError rc );
                            cassandra_exception_impl( CassError rc, const QString& what );

    virtual uint32_t        getCode()    const override { return f_code;    }
    virtual QString const&  getError()   const override { return f_error;   }
    virtual QString const&  getErrMsg()  const override { return f_errmsg;  }
    virtual QString const&  getMessage() const override { return f_message; }

    virtual const char*     what() const throw() override;

private:
    void                    init();

    CassError   f_code    = CassError();
    QString     f_error   = QString();
    QString     f_errmsg  = QString();
    QString     f_message = QString();
    std::string f_what    = std::string();
};


}
// namespace casswrapper

// vim: ts=4 sw=4 et
