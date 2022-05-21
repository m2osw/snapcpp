#pragma once

#include <QtSql/QSqlDriver>
#include <QtSql/QSqlResult>
#include <QMutex>

#include "casswrapper/query.h"

#ifdef QT_PLUGIN
#	define Q_EXPORT_SQLDRIVER_CASSANDRA
#else
#	define Q_EXPORT_SQLDRIVER_CASSANDRA Q_SQL_EXPORT
#endif

QT_BEGIN_NAMESPACE

class QCassandraDriver;

class QCassandraResult
        : public QSqlResult
        , public QObject
{
    friend class QCassandraDriver;

public:
    explicit QCassandraResult( QCassandraDriver const *db );
    virtual ~QCassandraResult();

    QVariant        handle()   const override;

protected:
    void            setQuery( QString const& query ) override;
    bool            reset( QString const& query )    override;
    bool            prepare( QString const& query )  override;
    int             size()                           override;
    int             numRowsAffected()                override;
    bool            exec()                           override;

    void 	        bindValue( int index,                   const QVariant &val, QSql::ParamType paramType ) override;
    void 	        bindValue( const QString &placeholder,  const QVariant &val, QSql::ParamType paramType ) override;

    QVariant        data( int field )                override;
    bool            isNull(int index)                override;

    bool            fetch( int i )                   override;
    bool            fetchFirst()                     override;
    bool            fetchLast()                      override;

    QSqlRecord      record() const                   override;

    int             pagingSize() const;
    void            setPagingSize( int const size );

signals:
    void            queryPageFinished();

private:
    typedef std::vector<std::vector<QVariant>> row_array_t;

    struct column_t
    {
        QString                            f_name = QString();
        casswrapper::schema::column_type_t f_type = casswrapper::schema::column_type_t();
    };

    void            createQuery();
    int             totalCount() const;

    void            pushRow( std::vector<QVariant> const& columns );
    bool            getNextRow();
    QVariant        atRow( int const field );

    casswrapper::Query::pointer_t       f_query = casswrapper::Query::pointer_t();
    int                                 f_totalCount = 0;
    int                                 f_pagingSize = -1;
    row_array_t                         f_rows = row_array_t();
    std::vector<column_t>               f_orderedColumns = std::vector<column_t>();
};

QT_END_NAMESPACE

// vim: ts=4 sw=4 et
