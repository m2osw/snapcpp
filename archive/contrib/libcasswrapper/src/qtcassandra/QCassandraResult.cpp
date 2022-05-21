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
    , f_pagingSize(PAGING_SIZE)
{
    f_rows.reserve(f_pagingSize);
}


QCassandraResult::~QCassandraResult()
{
}


QVariant QCassandraResult::handle() const
{
    return QVariant( reinterpret_cast<qulonglong>(f_query.data()) );
}


void QCassandraResult::createQuery()
{
    f_query->reset();
    f_query->query( lastQuery() );
    f_query->setPagingSize( f_pagingSize );
    setAt( QSql::BeforeFirstRow );
}


void QCassandraResult::setQuery( QString const& query )
{
    QSqlResult::setQuery( query );

    if( isSelect() && query.contains("COUNT(*)") )
    {
        f_totalCount = 1;
    }
    else if( isSelect() )
    {
        QStringList fromSplit( query.split("FROM") );
        QString count_query_sql( QString("SELECT COUNT(*) FROM %1").arg(fromSplit[1].trimmed()) );
        qDebug() << "Count Query=" << count_query_sql;
        Query::pointer_t count_query( Query::create( f_query->getSession() ) );
        count_query->query( count_query_sql );
        count_query->start();
        f_totalCount = count_query->getVariantColumn(0).toInt();
        count_query->end();
        qDebug() << "f_totalCount=" << f_totalCount;
        f_rows.reserve( f_totalCount );
    }
    else
    {
        f_totalCount = 0;
    }
}


bool QCassandraResult::reset( QString const& query )
{
    setSelect( true );
    setQuery( query );
    createQuery();
    return exec();
}


bool QCassandraResult::prepare( QString const& query )
{
    setSelect( false );
    setQuery( query );
    createQuery();
    return true;
}


int QCassandraResult::size()
{
    return -1;
    //return f_totalCount;  // This can be way too slow if you scrub to the end on a remote connection.
}


int QCassandraResult::numRowsAffected()
{
    return -1;
}


int QCassandraResult::totalCount() const
{
    return f_totalCount;
}


bool QCassandraResult::exec()
{
    try
    {
        f_query->start();
        setActive( true );
        setAt( QSql::BeforeFirstRow );
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


void QCassandraResult::bindValue( int index, const QVariant &val, QSql::ParamType /*paramType*/ )
{
    f_query->bindVariant( index, val );
}


void QCassandraResult::bindValue( const QString &placeholder, const QVariant &val, QSql::ParamType /*paramType*/ )
{
    f_query->bindVariant( placeholder, val );
}


QVariant QCassandraResult::atRow( int const field )
{
    int const idx(at());
    if( static_cast<size_t>(idx) >= f_rows.size() )
    {
        return QVariant();
    }

    auto value( f_rows[idx][field] );
    qDebug() << "f_rows[" << idx << "][" << field << "] = " << value;
    return value;
}


QVariant QCassandraResult::data( int field )
{
    try
    {
        return atRow(field);
    }
    catch( std::out_of_range const & )
    {
        return QVariant();
    }
    catch( ... )
    {
        throw;
    }
}


bool QCassandraResult::isNull( int index )
{
    try
    {
        return atRow(index).isNull();
    }
    catch( std::out_of_range const & )
    {
        return true;
    }
    catch( ... )
    {
        throw;
    }
}


void QCassandraResult::pushRow( std::vector<QVariant> const& columns )
{
    f_rows.push_back( columns );
}


bool QCassandraResult::getNextRow()
{
    if( f_query->nextRow() )
    {
        std::vector<QVariant> columns;
        for( size_t column = 0; column < f_query->columnCount(); ++column )
        {
            columns.push_back( f_query->getVariantColumn(column) );
        }

        pushRow( columns );
        return true;
    }
    return false;
}


bool QCassandraResult::fetch( int i )
{
    if( !isActive() || i < 0 )
    {
        return false;
    }

    if( at() == i )
    {
        return true;
    }

    // This is required because the model will "test"
    // to see if there are 255 records. So in that case,
    // we have to "wind up" until we reach `i`.
    //
    while( at() < i + 1 )
    {
        if( !getNextRow() && !f_query->nextPage() )
        {
            // No more records at all.
            break;
        }
        //
        setAt(at() + 1);
    }

    if( i >= f_totalCount )
    {
        // Reached the end of the entire result set.
        //
        return false;
    }

    setAt(i);
    return true;
}


bool QCassandraResult::fetchFirst()
{
    return fetch( 0 );
}


bool QCassandraResult::fetchLast()
{
    return fetch( size() - 1 );
}


QSqlRecord QCassandraResult::record() const
{
    QSqlRecord   record;

    if( !f_query->isReady() || !f_query->queryActive() )
    {
        return record;
    }

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


int  QCassandraResult::pagingSize() const
{
    return f_pagingSize;
}


void QCassandraResult::setPagingSize( int const size )
{
    f_pagingSize = size;
}


QT_END_NAMESPACE

// vim: ts=4 sw=4 et
