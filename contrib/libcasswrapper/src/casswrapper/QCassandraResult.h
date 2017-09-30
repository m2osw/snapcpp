#pragma once

#include <QtSql/QSqlDriver>
#include <QtSql/QSqlResult>

#include <casswrapper/query.h>

#ifdef QT_PLUGIN
#	define Q_EXPORT_SQLDRIVER_CASSANDRA
#else
#	define Q_EXPORT_SQLDRIVER_CASSANDRA Q_SQL_EXPORT
#endif

QT_BEGIN_NAMESPACE

class QCassandraDriver;

class QCassandraResult : public QSqlResult, public QObject
{
    friend class QCassandraDriver;

public:
    explicit QCassandraResult( QCassandraDriver const *db );
    virtual ~QCassandraResult();

    QVariant        handle()   const override;
    bool            isBlocking() const;
    void            setBlocking( bool const val = true );

protected:
    bool            reset( QString const& query )   override;
    bool            prepare( QString const& query ) override;
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

    QSqlRecord      record() const                  override;

private:
    casswrapper::Query::pointer_t                f_query;
    bool                                         f_blocking = false;

    typedef std::vector<std::vector<QVariant>> row_array_t;
    row_array_t      f_rows;

    struct column_t
    {
        QString                            f_name;
        casswrapper::schema::column_type_t f_type;
    };
    std::vector<column_t> f_orderedColumns;

    void            createQuery();
    bool            fetchPage();

private slots:
    void    onQueryFinished( casswrapper::Query::pointer_t q );
};

QT_END_NAMESPACE

// vim: ts=4 sw=4 et
