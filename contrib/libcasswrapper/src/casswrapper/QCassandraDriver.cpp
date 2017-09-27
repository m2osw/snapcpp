#include "QCassandraDriver.h"
#include "QCassandraResult.h"

using namespace casswrapper;
using namespace schema;

QT_BEGIN_NAMESPACE


QCassandraDriver::QCassandraDriver(QObject *p=0)
    : f_session( std::make_shared<Session>() )
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
        //case QSqlDriver::EventNotifications     :
        case QSqlDriver::QuerySize              :
        case QSqlDriver::PositionalPlaceholders :
        case QSqlDriver::Transactions           :
        case QSqlDriver::Unicode                : return true;
        default                                 : return false;
    }
}


bool QCassandraDriver::open(const QString & db,
		   const QString & host,
		   int port,
		   const QString& connOpts)
{
    f_db = db;
    bool const use_ssl = (connOpts == "CASSANDRA_USE_SSL");

    try
    {
        f_session->connect( host, port, use_ssl );
    }
    catch( std::exception const& e )
    {
        setLastError
            (
            , QSqlError( tr("Cannot open database! Error=%1").arg(e.what()) )
            , QSqlError::ConnectionError
            );
        return false;
    }

    return true;
}


void QCassandraDriver::close()
{
    f_session->disconnect();
}


bool QCassandraDriver::getBlocking() const
{
    return f_blocking;
}


void QCassandraDriver::setBlocking( bool const blocking ) const
{
    f_blocking = blocking;
}


QVariant QCassandraDriver::handle() const
{
    return f_session.get();
}


QSqlResult *QCassandraDriver::createResult() const
{
    auto result( new QCassandraResult(this) );
    result->setBlocking( f_blocking );
    return result;
}


bool QCassandraDriver::isOpen() const
{
    return f_session->isConnected();
}


QStringList QCassandraDriver::tables(QSql::TableType type) const
{
    SessionMeta::pointer_t meta( SessionMeta::create(f_session) );
    meta->loadSchema();

    KeyspaceMeta::pointer_t db_keyspace( meta->getKeyspaces()[f_db] );

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


QVariant::Type decodeColumnType( column_type_t type )
{
    switch( type )
    {
        case TypeUnknown    :
        case TypeCustom     :
        case TypeDecimal    :
        case TypeLast_entry :
        case TypeUdt        :
        case TypeInet       :
        case TypeList       :
        case TypeSet        :
        case TypeTuple      :
        case TypeMap        :   return QVariant::Invalid;   // TODO Not sure how to handle many of these...

        case TypeBlob       :   return QVariant::ByteArray;
        case TypeBoolean    :   return QVariant::Bool
        case TypeFloat      :
        case TypeDouble     :   return QVariant::Double;
        case TypeTinyInt    :
        case TypeSmallInt   :
        case TypeInt        :
        case TypeVarint     :
        case TypeBigint     :
        case TypeCounter    :   return QVariant::Int;
        case TypeDate       :   return QVariant::Date;
        case TypeTime       :   return QVariant::Time;
        case TypeTimestamp  :   return QVariant::DateTime;
        case TypeAscii      :
        case TypeVarchar    :
        case TypeText       :   return QVariant::String;
        case TypeUuid       :
        case TypeTimeuuid   :   return QVariant::Uuid;
    }
}


QSqlRecord QCassandraDriver::record(const QString& tablename) const
{
    KeyspaceMeta::pointer_t             db_keyspace  ( meta->getKeyspaces()[f_db] );
    KeyspaceMeta::TableMeta::pointer_t  table_record ( db_keyspace->getTables()[tablename] );
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
    if( f_batch.isActive() )
    {
        setLastError
            (
            , QSqlError( tr("Batch is already active! Please commit or rollback.") )
            , QSqlError::TransactionError
            );
        return false;
    }

    f_batch = LoggedBatch::create();
    return true;
}


bool QCassandraDriver::commitTransaction()
{
    if( !f_batch.isActive() )
    {
        setLastError
            (
            , QSqlError( tr("There is no batch active!") )
            , QSqlError::TransactionError
            );
        return false;
    }

    f_batch->run();
    return true;
}


bool QCassandraDriver::rollbackTransaction()
{
    if( !f_batch.isActive() )
    {
        setLastError
            (
            , QSqlError( tr("There is no batch active!") )
            , QSqlError::TransactionError
            );
        return false;
    }

    f_batch.reset();
    return true;
}


bool QCassandraDriver::isTransactionActive() const
{
    return f_batch.isActive();
}


QT_END_NAMESPACE

// vim: ts=4 sw=4 et
