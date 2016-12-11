#pragma once

#include <stdexcept>
#include <string>
#include <QString>
#include <QStringList>


namespace casswrapper
{

class exception_base_t
{
public:
    static int const            STACK_TRACE_DEPTH = 20;

                                exception_base_t();
    virtual                     ~exception_base_t() {}

    QStringList const &         get_stack_trace() const { return f_stack_trace; }

private:
    QStringList                 f_stack_trace;

    void                        collect_stack_trace(int stack_track_depth = STACK_TRACE_DEPTH);
};


class exception_t : public std::runtime_error, public exception_base_t
{
public:
    exception_t( const QString&     what );
    exception_t( const std::string& what );
    exception_t( const char*        what );

    virtual const char* what() const throw() override;
};


class cassandra_exception_t : public std::exception, public exception_base_t
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
