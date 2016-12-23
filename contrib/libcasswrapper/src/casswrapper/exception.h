#pragma once

#include <libexcept/exception.h>

#include <string>
#include <QString>
#include <QStringList>


namespace casswrapper
{


class cassandra_exception_t : public libexcept::exception_base_t
{
public:
    virtual uint32_t        getCode()    const = 0;
    virtual QString const&  getError()   const = 0;
    virtual QString const&  getErrMsg()  const = 0;
    virtual QString const&  getMessage() const = 0;

    virtual const char* what() const throw() = 0;
};


}
// namespace casswrapper

// vim: ts=4 sw=4 et
