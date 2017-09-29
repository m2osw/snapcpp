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


void QCassandraResult::parseSelectStmt()
{
    f_orderedColumns.clear();

    // Determine keyspace and table
    //
    QString lq( lastQuery() );
    lq.replace('\n',' ');
    QStringList const word_list  ( lq.split("FROM") );
    QStringList const space_list ( word_list[1].trimmed().split(' ') );
    QString     const table_name ( space_list[0].trimmed() );
    QStringList const db_table   ( table_name.split('.') );
    //
    SessionMeta::pointer_t sm( SessionMeta::create(f_query->getSession()) );
    sm->loadSchema();
    auto km = sm->getKeyspaces().at(db_table[0]);
    auto tm = km->getTables().at(db_table[1]);

    // Parse fields
    //
    QString     const   select      ( word_list[0].trimmed().split("SELECT")[1].trimmed() );
    auto        const & meta_columns( tm->getColumns() );
    if( select == "*" )
    {
        for( auto col_pair : meta_columns )
        {
            column_t c;
            c.f_name = col_pair.first;
            c.f_type = col_pair.second->getColumnType();
            f_orderedColumns.push_back( c );
        }
    }
    else
    {
        QStringList const fields( select.split(',') );
        for( auto field : fields )
        {
            field = field.trimmed();
            column_t c;
            //
            auto col_iter = meta_columns.find(field);
            if( col_iter == meta_columns.end() )
            {
                c.f_name = field.contains("AS")
                         ? field.split("AS")[1].trimmed()
                         : field;
                c.f_type = field.contains("COUNT")
                        ? column_type_t::TypeInt
                        : column_type_t::TypeText;
            }
            else
            {
                c.f_name = field;
                c.f_type = (*col_iter).second->getColumnType();
            }
            //
            f_orderedColumns.push_back( c );
        }
    }
}


bool QCassandraResult::reset( QString const& query )
{
    setQuery( query );
    createQuery();
    setSelect( true );
    parseSelectStmt();
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

    for( auto col : f_orderedColumns )
    {
        QSqlField f ( col.f_name
                    , QCassandraDriver::decodeColumnType( col.f_type )
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
