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

    virtual const char* what() const throw() override;

private:
    CassError   f_code;
    QString     f_error;
    QString     f_errmsg;
    QString     f_message;
    std::string f_what;

    void init();
};


}
// namespace casswrapper

// vim: ts=4 sw=4 et
