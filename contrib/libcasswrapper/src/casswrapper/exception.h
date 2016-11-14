#pragma once

#include <stdexcept>
#include <string>
#include <QString>


namespace casswrapper
{

class exception_t : public std::runtime_error
{
public:
    exception_t( const QString&     what );
    exception_t( const std::string& what );
    exception_t( const char*        what );
};

class cassandra_exception_t : public std::exception
{
public:
    virtual uint32_t        getCode()    const = 0;
    virtual QString const&  getError()   const = 0;
    virtual QString const&  getErrMsg()  const = 0;
    virtual QString const&  getMessage() const = 0;
};


}
// namespace casswrapper

// vim: ts=4 sw=4 et
