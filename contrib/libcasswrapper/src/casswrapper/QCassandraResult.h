#pragma once

#include <QtSql/qsqldriver.h>
#include <QtSql/qsqlresult.h>

#include <casswrapper/query.h>

#ifdef QT_PLUGIN
#	define Q_EXPORT_SQLDRIVER_CASSANDRA
#else
#	define Q_EXPORT_SQLDRIVER_CASSANDRA Q_SQL_EXPORT
#endif

QT_BEGIN_NAMESPACE

class QCassandraDriver;

class Q_EXPORT_SQLDRIVER_CASSANDRA QCassandraResult : public QSqlDriver
{
    friend class QCassandraResult;
    Q_OBJECT

public:
    explicit QCassandraResult( QCassandraDriver const *db );
    virtual ~QCassandraResult();

    QVariant        handle()   const override;
    bool            isBlocking() const;
    void            setBlocking( bool const val = true );

protected:
    bool            reset( QString const& query )   override;
    int             size()                          override;
    int             numRowsAffected()               override;
    bool            exec()                          override;

    void 	        bindValue( int index,                   const QVariant &val, QSql::ParamType paramType ) override;
    void 	        bindValue( const QString &placeholder,  const QVariant &val, QSql::ParamType paramType ) override;

    QVariant        data( int field )               override;
    bool            isNull(int index)               override;

    bool            fetch( int i )                  override;
    bool            fetchFirst()                    override;
    bool            fetchLast()                     override;

    //QSqlRecord      record() const                  override;

private:
    casswrapper::Query::pointer_t   f_query;
    bool                            f_blocking = false;

    typedef std::vector<std::vector<QVariant>> row_array_t;
    row_array_t                     f_rows;

    bool                            fetchPage();

private slots:
    void    onQueryFinished( Query::pointer_t q );
};

QT_END_NAMESPACE

// vim: ts=4 sw=4 et
