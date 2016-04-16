/*
 * Header:
 *      QtCassandraTable.h
 *
 * Description:
 *      Handling of the cassandra::CfDef.
 *
 * Documentation:
 *      See the corresponding .cpp file.
 *
 * License:
 *      Copyright (c) 2011-2016 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include "QtCassandra/QCassandraConsistencyLevel.h"
#include "QtCassandra/QCassandraPredicate.h"
#include "QtCassandra/QCassandraQuery.h"
#include "QtCassandra/QCassandraRow.h"
#include "QtCassandra/QCassandraSchema.h"

#include <controlled_vars/controlled_vars_auto_enum_init.h>

// GNU does not officially offer cstdint yet
#include <stdint.h>
#include <memory>

namespace QtCassandra
{

class CfDef;
class QCassandraContext;


// Cassandra Column Family
class QCassandraTable
    : public QObject
    , public std::enable_shared_from_this<QCassandraTable>
{
public:
    typedef std::shared_ptr<QCassandraTable> pointer_t;

    virtual ~QCassandraTable();

    QCassandraSession::pointer_t session() const { return f_session; }

    // context name
    const QString& contextName() const;
    const QString& tableName()   const;

    // fields
    //
    const QCassandraSchema::Value::map_t& fields() const;
    QCassandraSchema::Value::map_t&       fields();

    // handling
    void create();
    //void update();
    void truncate();
    void clearCache();

    // row handling
    uint32_t readRows(QCassandraRowPredicate::pointer_t row_predicate = QCassandraRowPredicate::pointer_t() );

    QCassandraRow::pointer_t    row(const char*       row_name);
    QCassandraRow::pointer_t    row(const QString&    row_name);
    QCassandraRow::pointer_t    row(const QByteArray& row_name);
    const QCassandraRows&       rows();

    QCassandraRow::pointer_t    findRow(const char* row_name) const;
    QCassandraRow::pointer_t    findRow(const QString& row_name) const;
    QCassandraRow::pointer_t    findRow(const QByteArray& row_name) const;
    bool                        exists(const char* row_name) const;
    bool                        exists(const QString& row_name) const;
    bool                        exists(const QByteArray& row_name) const;
    QCassandraRow&              operator[] (const char* row_name);
    QCassandraRow&              operator[] (const QString& row_name);
    QCassandraRow&              operator[] (const QByteArray& row_name);
    const QCassandraRow&        operator[] (const char* row_name) const;
    const QCassandraRow&        operator[] (const QString& row_name) const;
    const QCassandraRow&        operator[] (const QByteArray& row_name) const;

    void dropRow
        ( const char*       row_name
        , QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO
        , int64_t timestamp = 0
        , consistency_level_t consistency_level = CONSISTENCY_LEVEL_DEFAULT
        );
    void dropRow
        ( const QString&    row_name
        , QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO
        , int64_t timestamp = 0
        , consistency_level_t consistency_level = CONSISTENCY_LEVEL_DEFAULT
        );
    void dropRow
        ( const QByteArray& row_name
        , QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO
        , int64_t timestamp = 0
        , consistency_level_t consistency_level = CONSISTENCY_LEVEL_DEFAULT
        );

    std::shared_ptr<QCassandraContext> parentContext() const;

private:
    QCassandraTable(std::shared_ptr<QCassandraContext> context, const QString& table_name);

    void        setFromCassandra();
    void        parseTableDefinition( QCassandraSchema::SessionMeta::KeyspaceMeta::TableMeta::pointer_t table_meta );
    void        insertValue(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    bool        getValue(const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value);
    void        assignRow(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    int32_t     getCellCount(const QByteArray& row_key, QCassandraCellPredicate::pointer_t column_predicate);
    void 		remove
                ( const QByteArray& row_key
                , const QByteArray& column_key
                , int64_t timestamp = 0
                , consistency_level_t consistency_level = CONSISTENCY_LEVEL_DEFAULT
                );
    void 		remove
                ( const QByteArray& row_key
                , int64_t timestamp = 0
                , consistency_level_t consistency_level = CONSISTENCY_LEVEL_DEFAULT
                );

    bool		isCounterClass();

    void 		loadTables();
    void		addRow( const QByteArray& row_key, const QByteArray& column_key, const QByteArray& data );

    QString     getTableOptions() const;

    friend class QCassandraContext;
    friend class QCassandraRow;

    QCassandraSchema::SessionMeta::KeyspaceMeta::TableMeta::pointer_t	f_schema;

    controlled_vars::zbool_t                    f_from_cassandra;
    std::shared_ptr<QCassandraContext>          f_context;
    QString										f_tableName;
    QCassandraRows                              f_rows;

    QCassandraSession::pointer_t                f_session;
    QCassandraQuery::pointer_t                  f_query;
};

// list of table definitions mapped against their name (see tableName())
typedef QMap<QString, QCassandraTable::pointer_t > QCassandraTables;

} // namespace QtCassandra

// vim: ts=4 sw=4 et
