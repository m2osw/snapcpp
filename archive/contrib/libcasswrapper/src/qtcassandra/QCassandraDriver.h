#pragma once

#include <QtSql/QSqlDriver>
#include <QtSql/QSqlResult>

#include <casswrapper/batch.h>
#include <casswrapper/query.h>
#include <casswrapper/session.h>

#ifdef QT_PLUGIN
#define Q_EXPORT_SQLDRIVER_CASSANDRA
#else
#define Q_EXPORT_SQLDRIVER_CASSANDRA Q_SQL_EXPORT
#endif

QT_BEGIN_NAMESPACE

class QCassandraResult;

class Q_EXPORT_SQLDRIVER_CASSANDRA QCassandraDriver
        : public QSqlDriver
{
    friend class QCassandraResult;
    Q_OBJECT

public:
    explicit QCassandraDriver(QObject *p=0);
    ~QCassandraDriver();

    bool hasFeature(DriverFeature f) const override;
    bool open ( const QString & db
              , const QString & user     = QString()
              , const QString & password = QString()
              , const QString & host     = QString()
              , int             port     = -1
              , const QString & connOpts = QString()
              ) override;

    void close() override;

    QVariant handle() const      override;

    QSqlResult *    createResult()                      const override;
    QStringList     tables(QSql::TableType)             const override;
    QSqlRecord      record(const QString& tablename)    const override;
    bool            isOpen()                            const override;

    bool            beginTransaction()    override;
    bool            commitTransaction()   override;
    bool            rollbackTransaction() override;

    bool            isTransactionActive() const;

    static QVariant::Type decodeColumnType( casswrapper::schema::column_type_t type );

    // TODO: do we actually need these?
    //QSqlIndex       primaryIndex(const QString& tablename) const override;
    //QString         formatValue(const QSqlField &field, bool trimStrings) const override;
    //QString         escapeIdentifier(const QString &identifier, IdentifierType type) const override;
    //bool            isIdentifierEscaped(const QString &identifier, IdentifierType type) const override;

private:
    void                            emitQueryFinishedSignal() const;

    casswrapper::Session::pointer_t f_session = casswrapper::Session::pointer_t();
    casswrapper::Batch::pointer_t   f_batch = casswrapper::Batch::pointer_t();
    QString                         f_db = QString();
};

QT_END_NAMESPACE

// vim: ts=4 sw=4 et
