#include "QCassandraDriver.h"
#include "QCassandraResult.h"

#include "casswrapper/schema.h"

#include <QtSql/QtSql>

#include <memory>

using namespace casswrapper;
using namespace schema;

QT_BEGIN_NAMESPACE


QCassandraDriver::QCassandraDriver(QObject *p)
    : QSqlDriver(p)
    , f_session( Session::create() )
{
}


QCassandraDriver::~QCassandraDriver()
{
}


bool QCassandraDriver::hasFeature(DriverFeature f) const
{
    switch( f )
    {
        case QSqlDriver::BLOB                   :
        case QSqlDriver::PositionalPlaceholders :
        case QSqlDriver::Transactions           :
        case QSqlDriver::Unicode                : return true;
        default                                 : return false;
    }
}


bool QCassandraDriver::open ( const QString & db
                            , const QString & /*user*/
                            , const QString & /*password*/
                            , const QString & host
                            , int             port
                            , const QString & connOpts
                            )
{
    try
    {
        f_db = db;

        if( f_db.isEmpty() )
        {
            throw std::runtime_error( "Cassandra keyspace (database) MUST be specified!" );
        }

        bool const use_ssl = (connOpts == "CASSANDRA_USE_SSL");
        f_session->connect( host, port, use_ssl );
        //
        // USE the specified DB
        //
        Query::pointer_t use_query( Query::create(f_session) );
        use_query->query( "USE " + f_db );
        use_query->start();
        use_query->end();
    }
    catch( std::exception const& e )
    {
        setLastError( QSqlError
                      ( tr("Cannot open database!")
                      , e.what()
                      , QSqlError::TransactionError
                      )
                    );
        return false;
    }

    return true;
}


void QCassandraDriver::close()
{
    f_session->disconnect();
}


QVariant QCassandraDriver::handle() const
{
    return reinterpret_cast<qulonglong>(f_session.get());
}


QSqlResult *QCassandraDriver::createResult() const
{
    std::unique_ptr<QCassandraResult> result( new QCassandraResult(this) );
    return result.release();
}


bool QCassandraDriver::isOpen() const
{
    return f_session->isConnected();
}


QStringList QCassandraDriver::tables(QSql::TableType type) const
{
    auto meta( SessionMeta::create(f_session) );
    meta->loadSchema();

    auto db_keyspace( meta->getKeyspaces().at(f_db) );

    QStringList table_list;

    for( auto const & pair : db_keyspace->getTables() )
    {
        QString const table_name(pair.first);
        switch( type )
        {
            case QSql::Tables:
                if( !table_name.startsWith("system") )
                {
                    table_list << table_name;
                }
                break;

            case QSql::SystemTables:
                if( table_name.startsWith("system") )
                {
                    table_list << table_name;
                }
                break;

            case QSql::Views:
                // None in Cassandra
                break;

            case QSql::AllTables:
                table_list << table_name;
                break;
        }
    }

    return table_list;
}


QVariant::Type QCassandraDriver::decodeColumnType( column_type_t type )
{
    switch( type )
    {
    case column_type_t::TypeUnknown    :
    case column_type_t::TypeCustom     :
    case column_type_t::TypeDecimal    :
    case column_type_t::TypeLast_entry :
    case column_type_t::TypeUdt        :
    case column_type_t::TypeInet       :
    case column_type_t::TypeList       :
    case column_type_t::TypeSet        :
    case column_type_t::TypeTuple      :
    case column_type_t::TypeMap        :   return QVariant::Invalid;   // TODO Not sure how to handle many of these...

    case column_type_t::TypeBlob       :   return QVariant::ByteArray;
    case column_type_t::TypeBoolean    :   return QVariant::Bool;
    case column_type_t::TypeFloat      :
    case column_type_t::TypeDouble     :   return QVariant::Double;
    case column_type_t::TypeTinyInt    :
    case column_type_t::TypeSmallInt   :
    case column_type_t::TypeInt        :
    case column_type_t::TypeVarint     :
    case column_type_t::TypeBigint     :
    case column_type_t::TypeCounter    :   return QVariant::Int;
    case column_type_t::TypeDate       :   return QVariant::Date;
    case column_type_t::TypeTime       :   return QVariant::Time;
    case column_type_t::TypeTimestamp  :   return QVariant::DateTime;
    case column_type_t::TypeAscii      :
    case column_type_t::TypeVarchar    :
    case column_type_t::TypeText       :   return QVariant::String;
    case column_type_t::TypeUuid       :
    case column_type_t::TypeTimeuuid   :   return QVariant::Uuid;
    default                            :   return QVariant::Invalid;
    }
}


QSqlRecord QCassandraDriver::record(const QString& tablename) const
{
    auto meta( SessionMeta::create(f_session) );
    meta->loadSchema();

    auto db_keyspace  ( meta->getKeyspaces().at(f_db) );
    auto table_record ( db_keyspace->getTables().at(tablename) );
    if( !table_record )
    {
        // Not found...
        return QSqlRecord();
    }

    QSqlRecord  record;

    for( auto col_pair : table_record->getColumns() )
    {
        QSqlField f ( col_pair.first
                    , decodeColumnType(col_pair.second->getColumnType())
        );
        f.setRequired(true);
        //f.setLength(field->length);       // TODO: needed?
        //f.setPrecision(field->decimals);  // TODO: needed?
        record.append(f);
    }

    return record;
}


bool QCassandraDriver::beginTransaction()
{
    if( f_batch->isActive() )
    {
        setLastError( QSqlError
                      ( tr("Batch is already active! Please commit or rollback.")
                      , QString()
                      , QSqlError::TransactionError
                      )
                    );
        return false;
    }

    f_batch = LoggedBatch::create();
    return true;
}


bool QCassandraDriver::commitTransaction()
{
    if( !f_batch->isActive() )
    {
        setLastError( QSqlError
                      ( tr("There is no batch active!")
                      , QString()
                      , QSqlError::TransactionError
                      )
                    );
        return false;
    }

    f_batch->run();
    return true;
}


bool QCassandraDriver::rollbackTransaction()
{
    if( !f_batch->isActive() )
    {
        setLastError( QSqlError
                      ( tr("There is no batch active!")
                      , QString()
                      , QSqlError::TransactionError
                      )
                    );
        return false;
    }

    f_batch.reset();
    return true;
}


bool QCassandraDriver::isTransactionActive() const
{
    return f_batch->isActive();
}


void QCassandraDriver::emitQueryFinishedSignal() const
{
    // Kludge because the notification method is not 'const'.
    //
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QVariant data;
    emit const_cast<QCassandraDriver*>(this)->notification(
              "QCassandraDriver::queryFinished()"
            , QSqlDriver::SelfSource
            , data);
#else
    emit const_cast<QCassandraDriver*>(this)->notification(
              "QCassandraDriver::queryFinished()");
#endif
}


QT_END_NAMESPACE

// vim: ts=4 sw=4 et
