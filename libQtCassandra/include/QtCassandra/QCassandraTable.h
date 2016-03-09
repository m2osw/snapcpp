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

#include "QtCassandra/QCassandraRowPredicate.h"
#include "QtCassandra/QCassandraRow.h"
#include "QtCassandra/QCassandraTools.h"

// GNU does not officially offer cstdint yet
#include <stdint.h>
#include <memory>

namespace QtCassandra
{

class QCassandraContext;


// Cassandra Column Family (org::apache::cassandra::CfDef)
class QCassandraTable
    : public QObject
    , public std::enable_shared_from_this<QCassandraTable>
{
public:
    typedef std::shared_ptr<QCassandraTable> pointer_t;

    virtual ~QCassandraTable();

    // context name
    QString contextName() const;

    //void setTableName(const QString& name);
    QString tableName() const;

    // Types allowed are "general", "compaction", "compression" and "caching".
    // This options need to be set prior to calling "create()".
    //
    // See https://cassandra.apache.org/doc/cql3/CQL.html#createTableStmt
    //
    QString   option      ( const QString& option_type, const QString& option_name) const;
    QString&  option      ( const QString& option_type, const QString& option_name);
    void      unsetOption ( const QString& option_type, const QString& option_name);

    // handling
    void create();
    //void update();
    void truncate();
    void clearCache();

    // row handling
    uint32_t readRows(QCassandraRowPredicate& row_predicate);

    QCassandraRow::pointer_t    row(const QString&    row_name);
    QCassandraRow::pointer_t    row(const QByteArray& row_name);
    const QCassandraRows&       rows() const;

    QCassandraRow::pointer_t    findRow(const QString& row_name) const;
    bool                        exists(const QString& row_name) const;
    QCassandraRow&              operator[] (const QString& row_name);
    const QCassandraRow&        operator[] (const QString& row_name) const;

    void dropRow(const QString& row_name, QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO, int64_t timestamp = 0, consistency_level_t consistency_level = CONSISTENCY_LEVEL_ALL);

    std::shared_ptr<QCassandraContext> parentContext() const;

private:
    QCassandraTable(std::shared_ptr<QCassandraContext> context, const QString& table_name);

    int32_t rowCount( const QByteArray& row_key ) const;

    void        setFromCassandra();
    void        parseTableDefinition(const void *data);
    void        prepareTableDefinition(void *data) const;
    void        insertValue(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    bool        getValue(const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value);
    void        addValue(const QByteArray& row_key, const QByteArray& column_key, int64_t value);
    void        assignRow(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    int32_t     getCellCount(const QByteArray& row_key, const QCassandraColumnPredicate& column_predicate);
    void 		remove( const QByteArray& row_key, const QByteArray& column_key );
    void 		remove( const QByteArray& row_key );

    int32_t getCurrentCount();

    friend class QCassandraContext;
    friend class QCassandraRow;

    typedef QMap<QString,QString>       option_map_t;
    typedef QMap<QString,option_map_t>  type_option_map_t;

    std::shared_ptr<QCassandraContext>          f_context;
    QString                                     f_tableName;
    type_option_map_t                           f_options;
    //controlled_vars::flbool_t                   f_from_cassandra;
    QCassandraRows                              f_rows;
    QCassandraRowPredicate*                     f_currentPredicate;

    statement_pointer_t                         f_queryStmt;
    future_pointer_t                            f_sessionExecute;
    result_pointer_t                            f_currentQueryResult;
};

// list of table definitions mapped against their name (see tableName())
typedef QMap<QString, QCassandraTable::pointer_t > QCassandraTables;

} // namespace QtCassandra

// vim: ts=4 sw=4 et
