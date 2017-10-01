#include "QCassandraDriver.h"
#include "QCassandraResult.h"

#include "casswrapper/schema.h"
#include "casswrapper/qstring_stream.h"

#include <QtSql/QtSql>

#include <iostream>

using namespace casswrapper;
using namespace schema;

QT_BEGIN_NAMESPACE

namespace
{
    int const PAGING_SIZE = 100;
}

QCassandraResult::QCassandraResult(QCassandraDriver const *db)
    : QSqlResult(db)
    , f_query(Query::create(db->f_session))
{
    f_rows.reserve(PAGING_SIZE);
    connect( f_query.get() , &Query::queryFinished
           , this          , &QCassandraResult::onQueryFinished
           );
}


QCassandraResult::~QCassandraResult()
{
}


void QCassandraResult::onQueryFinished( Query::pointer_t q )
{
    Q_ASSERT( q.get() == f_query.get() );

    if( !f_blocking )
    {
        fetchPage();
        dynamic_cast<const QCassandraDriver*>(driver())->emitQueryFinishedSignal();
    }
}


QVariant QCassandraResult::handle() const
{
    return QVariant( reinterpret_cast<qulonglong>(f_query.get()) );
}


bool QCassandraResult::isBlocking() const
{
    return f_blocking;
}


void QCassandraResult::setBlocking( bool const val )
{
    f_blocking = val;
}


void QCassandraResult::createQuery()
{
    f_query->reset();
    f_query->query( lastQuery() );
    f_query->setPagingSize( PAGING_SIZE );
    setAt( QSql::BeforeFirstRow );
}


bool QCassandraResult::reset( QString const& query )
{
    setQuery( query );
    createQuery();
    setSelect( true );
    return true;
}


bool QCassandraResult::prepare( QString const& query )
{
    setQuery( query );
    createQuery();
    setSelect( false );
    return true;
}


int QCassandraResult::size()
{
    return f_rows.size();
}

int QCassandraResult::numRowsAffected()
{
    return -1;  // TODO: implement for non-select queries
}

bool QCassandraResult::exec()
{
    try
    {
        f_query->start( f_blocking );
        setActive( true );
        //
        if( f_blocking )
        {
            while( fetchPage() );
        }
        return true;
    }
    catch( std::exception const& x )
    {
        setLastError( QSqlError
                      ( QObject::tr("Query error=%1").arg(x.what())
                      , QString()
                      , QSqlError::StatementError
                      )
                    );
        return false;
    }
}


bool QCassandraResult::fetchPage()
{
    setActive( true );

    while( f_query->nextRow() )
    {
        std::vector<QVariant> columns;
        for( size_t column = 0; column < f_query->columnCount(); ++column )
        {
            columns.push_back( f_query->getVariantColumn(column) );
        }
        f_rows.push_back( columns );
    }

    return f_query->nextPage(f_blocking);
}


void QCassandraResult::bindValue( int index, const QVariant &val, QSql::ParamType /*paramType*/ )
{
    f_query->bindVariant( index, val );
}


void QCassandraResult::bindValue( const QString &placeholder, const QVariant &val, QSql::ParamType /*paramType*/ )
{
    f_query->bindVariant( placeholder, val );
}


QVariant QCassandraResult::data( int field )
{
    //return f_query->getVariantColumn(field);
    return f_rows[at()][field];
}


bool QCassandraResult::isNull( int index )
{
    return f_rows[at()][index].isNull();
}


bool QCassandraResult::fetch( int i )
{
    try
    {
        f_rows.at(i);    // If out of range, it throws.
        setAt(i);
        return true;
    }
    catch( std::out_of_range const & )
    {
        setAt( QSql::AfterLastRow );
        return false;
    }
}


bool QCassandraResult::fetchFirst()
{
    return fetch( 0 );
}


bool QCassandraResult::fetchLast()
{
    return fetch( f_rows.size()-1 );
}


QSqlRecord QCassandraResult::record() const
{
    QSqlRecord  record;

    for( size_t index = 0; index < f_query->columnCount(); ++index )
    {
        QSqlField f ( f_query->columnName( index )
                    , QCassandraDriver::decodeColumnType( f_query->columnType( index ) )
        );
        f.setRequired(true);
        //f.setLength(field->length);       // TODO: needed?
        //f.setPrecision(field->decimals);  // TODO: needed?
        record.append(f);
    }

    return record;
}


QT_END_NAMESPACE

// vim: ts=4 sw=4 et
