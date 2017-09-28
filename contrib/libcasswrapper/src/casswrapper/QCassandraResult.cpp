#include "QCassandraDriver.h"
#include "QCassandraResult.h"

using namespace casswrapper;

QT_BEGIN_NAMESPACE

namespace
{
    int const PAGING_SIZE = 100;
}

QCassandraResult::QCassandraDriver(QCassandraDriver *db)
    : QCassandraResult(db)
    , f_query(Query::create(db->f_session))
{
    connect( f_query.get(), &Query::queryFinished
           , this         , &QCassandraResult::onQueryFinished
           );

    f_rows.reserve(PAGING_SIZE);
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
    }
}


QVariant QCassandraResult::handle() const
{
    return f_query.get();
}


QVariant QCassandraResult::data( int field )
{
    if( !isSelect() || field >= f_query->columnCount() )
    {
        qWarning("QCassandraResult::data(): column %d out of range!", field);
        return QVariant();
    }
}


bool QCassandraResult::isBlocking() const
{
    return f_blocking;
}


void QCassandraResult::setBlocking( bool const val )
{
    f_blocking = val;
}


bool QCassandraResult::reset( QString const& query )
{
    f_query->reset();
    f_query->query( query );
    f_query->setPagingSize( PAGING_SIZE );
    setAt( QSql::BeforeFirstRow );
}


int QCassandraResult::size()
{
    return f_rows.size();
}


bool QCassandraResult::exec()
{
    try
    {
        f_query->start( f_blocking );
        //
        if( f_blocking )
        {
            while( fetchPage() );
        }
    }
    catch( exception const& x )
    {
        setLastError
            (
            , QSqlError( tr("Query error=%1").arg(x.what()) )
            , QSqlError::StatementError
            );
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
    f_query->bindValue( index, val );
}


void QCassandraResult::bindValue( const QString &placeholder, const QVariant &val, QSql::ParamType /*paramType*/ )
{
    f_query->bindValue( placeholder, val );
}


QVariant QCassandraResult::data( int field )
{
    //return f_query->getVariantColumn(field);
    return f_rows[at()][field];
}


bool QCassandraResult::isNull( int index )
{
    return data( index ).isNull();
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
        // We wait until the result comes in
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


QT_END_NAMESPACE

// vim: ts=4 sw=4 et
