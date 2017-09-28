#pragma once

#include <QtSql/qsqldriver.h>
#include <QtSql/qsqlresult.h>

#include <casswrapper/session.h>

#ifdef QT_PLUGIN
#	define Q_EXPORT_SQLDRIVER_CASSANDRA
#else
#	define Q_EXPORT_SQLDRIVER_CASSANDRA Q_SQL_EXPORT
#endif

QT_BEGIN_NAMESPACE

class QCassandraResult;

class Q_EXPORT_SQLDRIVER_CASSANDRA QCassandraDriver : public QSqlDriver
{
    friend class QCassandraResult;
    Q_OBJECT

public:
    explicit QCassandraDriver(QObject *p=0);
    ~QCassandraDriver();

    bool hasFeature(DriverFeature f) const override;
    bool open(const QString & db,
               const QString & host,
               int port,
               const QString& connOpts) override;	// Not sure about connOpts
    void close() override;

    bool    getBlocking() const;
    void    setBlocking( bool const blocking ) const;

    QVariant handle() const      override;

    QSqlResult *	createResult()                      const override;
    QStringList 	tables(QSql::TableType)             const override;
    QSqlRecord 		record(const QString& tablename)    const override;
    bool            isOpen()                            const override;

    bool            beginTransaction()    override;
    bool            commitTransaction()   override;
    bool            rollbackTransaction() override;

    bool            isTransactionActive() const;

    // TODO: do we actually need these?
    //QSqlIndex 		primaryIndex(const QString& tablename) const override;
    //QString 		formatValue(const QSqlField &field, bool trimStrings) const override;
    //QString			escapeIdentifier(const QString &identifier, IdentifierType type) const override;
    //bool			isIdentifierEscaped(const QString &identifier, IdentifierType type) const override;

private:
    casswrapper::Session::pointer_t f_session;
    casswrapper::Batch::pointer_t   f_batch;
    QString                         f_db;
    bool                            f_blocking = true;
};

QT_END_NAMESPACE

// vim: ts=4 sw=4 et
