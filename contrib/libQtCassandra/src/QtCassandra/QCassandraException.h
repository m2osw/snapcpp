#pragma once

#include <stdexcept>
#include <string>
#include <QString>
#include <QStringList>


namespace QtCassandra
{

class QCassandraExceptionBase
{
public:
    static int const            STACK_TRACE_DEPTH = 20;

                                QCassandraExceptionBase();
    virtual                     ~QCassandraExceptionBase() {}

    QStringList const &         get_stack_trace() const { return f_stack_trace; }

private:
    QStringList                 f_stack_trace;

    void                        collect_stack_trace(int stack_track_depth = STACK_TRACE_DEPTH);
};


class QCassandraException : public std::runtime_error, public QCassandraExceptionBase
{
public:
    QCassandraException( const QString&     what );
    QCassandraException( const std::string& what );
    QCassandraException( const char*        what );
};


class QCassandraLogicException : public QCassandraException
{
public:
    QCassandraLogicException( const QString&     what );
    QCassandraLogicException( const std::string& what );
    QCassandraLogicException( const char*        what );
};


class QCassandraOverflowException : public QCassandraException
{
public:
    QCassandraOverflowException( const QString&     what );
    QCassandraOverflowException( const std::string& what );
    QCassandraOverflowException( const char*        what );
};


}
// namespace casswrapper

// vim: ts=4 sw=4 et
