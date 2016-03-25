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

#include "QtCassandra/QCassandraPredicate.h"
#include "QtCassandra/QCassandraRow.h"
#include "QtCassandra/QCassandraQuery.h"

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

    // context name
    QString contextName() const;

    //void setTableName(const QString& name);
    QString tableName() const;
    void setIdentifier(int32_t identifier);
    void unsetIdentifier();
    bool hasIdentifier() const;
    int32_t identifier() const;
    void setComment(QString comment);
    void unsetComment();
    bool hasComment() const;
    QString comment() const;

    // columns information (defaults)
    void setColumnType(const QString& column_type);
    void unsetColumnType();
    bool hasColumnType() const;
    QString columnType() const;
    void setDefaultValidationClassForCounters();
    void setDefaultValidationClass(const QString& validation_class);
    void unsetDefaultValidationClass();
    bool hasDefaultValidationClass() const;
    QString defaultValidationClass() const;
    void setKeyValidationClass(const QString& validation_class);
    void unsetKeyValidationClass();
    bool hasKeyValidationClass() const;
    QString keyValidationClass() const;
    void setKeyAlias(const QString& key_alias);
    void unsetKeyAlias();
    bool hasKeyAlias() const;
    QString keyAlias() const;
    void setComparatorType(const QString& comparator_type);
    void unsetComparatorType();
    bool hasComparatorType() const;
    QString comparatorType() const;
    void setSubcomparatorType(const QString& subcomparator_type);
    void unsetSubcomparatorType();
    bool hasSubcomparatorType() const;
    QString subcomparatorType() const;

    // columns information (specific)
    QCassandraColumnDefinition::pointer_t columnDefinition(const QString& column_name);
    const QCassandraColumnDefinitions& columnDefinitions() const;

    // cache handling
    void setCaching(const QString& caching); // since 1.1
    void unsetCaching();
    bool hasCaching() const;
    QString caching() const;
    void setRowCacheSize(double size); // deprecated in 1.1
    void unsetRowCacheSize();
    bool hasRowCacheSize() const;
    double rowCacheSize() const;
    void setRowCacheSavePeriodInSeconds(int32_t seconds); // deprecated in 1.1
    void unsetRowCacheSavePeriodInSeconds();
    bool hasRowCacheSavePeriodInSeconds() const;
    int32_t rowCacheSavePeriodInSeconds() const;
    void setKeyCacheSize(double size); // deprecated in 1.1
    void unsetKeyCacheSize();
    bool hasKeyCacheSize() const;
    double keyCacheSize() const;
    void setKeyCacheSavePeriodInSeconds(int32_t seconds); // deprecated in 1.1
    void unsetKeyCacheSavePeriodInSeconds();
    bool hasKeyCacheSavePeriodInSeconds() const;
    int32_t keyCacheSavePeriodInSeconds() const;
    void setRowCacheKeysToSave(int32_t count); // since 1.0 -- deprecated in 1.1
    void unsetRowCacheKeysToSave();
    bool hasRowCacheKeysToSave() const;
    int32_t keyRowCacheKeysToSave() const;
    void setBloomFilterFPChance(double chance); // since 1.0
    void unsetBloomFilterFPChance();
    bool hasBloomFilterFPChance() const;
    double bloomFilterFPChance() const;

    // ring maintenance
    void setReadRepairChance(double repair_chance);
    void unsetReadRepairChance();
    bool hasReadRepairChance() const;
    double readRepairChance() const;
    void setDCLocalReadRepairChance(double repair_chance); // since 1.1
    void unsetDCLocalReadRepairChance();
    bool hasDCLocalReadRepairChance() const;
    double dcLocalReadRepairChance() const;
    void setReplicateOnWrite(bool replicate_on_write);
    void unsetReplicateOnWrite();
    bool hasReplicateOnWrite() const;
    bool replicateOnWrite() const;
    void setMergeShardsChance(double merge_shards_chance); // deprecated in 1.1
    void unsetMergeShardsChance();
    bool hasMergeShardsChance() const;
    double mergeShardsChance() const;
    void setRowCacheProvider(const QString& provider); // deprecated in 1.1
    void unsetRowCacheProvider();
    bool hasRowCacheProvider() const;
    QString rowCacheProvider() const;

    // memory handling
    void setGcGraceSeconds(int32_t seconds);
    void unsetGcGraceSeconds();
    bool hasGcGraceSeconds() const;
    int32_t gcGraceSeconds() const;
    void setMemtableFlushAfterMins(int32_t minutes); // deprecated in 1.1
    void unsetMemtableFlushAfterMins();
    bool hasMemtableFlushAfterMins() const;
    int32_t memtableFlushAfterMins() const;
    void setMemtableThroughputInMb(int32_t megabytes); // deprecated in 1.1
    void unsetMemtableThroughputInMb();
    bool hasMemtableThroughputInMb() const;
    int32_t memtableThroughputInMb() const;
    void setMemtableOperationsInMillions(int32_t operations); // deprecated in 1.1
    void unsetMemtableOperationsInMillions();
    bool hasMemtableOperationsInMillions() const;
    int32_t memtableOperationsInMillions() const;

    // compression handling
    void setMinCompactionThreshold(int32_t threshold);
    void unsetMinCompactionThreshold();
    bool hasMinCompactionThreshold() const;
    double minCompactionThreshold() const;
    void setMaxCompactionThreshold(int32_t threshold);
    void unsetMaxCompactionThreshold();
    bool hasMaxCompactionThreshold() const;
    double maxCompactionThreshold() const;
    void setCompactionStrategy(const QString& compaction_strategy); // since 1.0
    void unsetCompactionStrategy();
    bool hasCompactionStrategy() const;
    QString compactionStrategy() const;
    void setCompactionStrategyOption(const QString& option_name, const QString& value); // since 1.0
    void unsetCompactionStrategyOption(const QString& option_name);
    bool hasCompactionStrategyOption(const QString& option_name) const;
    bool hasCompactionStrategyOptions() const;
    QString compactionStrategyOption(const QString& option_name) const;
    void setCompressionOption(const QString& option_name, const QString& value); // since 1.0
    void unsetCompressionOption(const QString& option_name);
    bool hasCompressionOption(const QString& option_name) const;
    bool hasCompressionOptions() const;
    QString compressionOption(const QString& option_name) const;

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

    QCassandraRow::pointer_t    findRow(const char* row_name);
    QCassandraRow::pointer_t    findRow(const QString& row_name);
    QCassandraRow::pointer_t    findRow(const QByteArray& row_name);
    bool                        exists(const char* row_name) const;
    bool                        exists(const QString& row_name) const;
    bool                        exists(const QByteArray& row_name) const;
    QCassandraRow&              operator[] (const char* row_name);
    QCassandraRow&              operator[] (const QString& row_name);
    QCassandraRow&              operator[] (const QByteArray& row_name);
    const QCassandraRow&        operator[] (const char* row_name) const;
    const QCassandraRow&        operator[] (const QString& row_name) const;
    const QCassandraRow&        operator[] (const QByteArray& row_name) const;

    void dropRow(const char* row_name, QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO, int64_t timestamp = 0);
    void dropRow(const QString& row_name, QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO, int64_t timestamp = 0);
    void dropRow(const QByteArray& row_name, QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO, int64_t timestamp = 0);

    std::shared_ptr<QCassandraContext> parentContext() const;

private:
    QCassandraTable(std::shared_ptr<QCassandraContext> context, const QString& table_name);

    void        setFromCassandra();
    void        parseTableDefinition( const CfDef& data );
    void        prepareTableDefinition( CfDef& data ) const;
    void        insertValue(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    bool        getValue(const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value);
    void        assignRow(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    int32_t     getCellCount(const QByteArray& row_key, const QCassandraCellPredicate& column_predicate);
    uint32_t    getColumnSlice(const QByteArray& row_key, QCassandraColumnPredicate& column_predicate);
    void 		remove( const QByteArray& row_key, const QByteArray& column_key );
    void 		remove( const QByteArray& row_key );

    void 		loadTables();
    void		addRow( const QByteArray& row_key, const QByteArray& column_key, const QByteArray& data );

    QString     getTableOptions( CfDef& cf ) const;

    friend class QCassandraContext;
    friend class QCassandraRow;

    controlled_vars::zbool_t                    f_from_cassandra;
    std::auto_ptr<CfDef>                        f_private;
    std::shared_ptr<QCassandraContext>          f_context;
    QCassandraColumnDefinitions                 f_column_definitions;
    QCassandraRows                              f_rows;

    QCassandraSession::pointer_t                f_session;
    QCassandraQuery::pointer_t                  f_query;
};

// list of table definitions mapped against their name (see tableName())
typedef QMap<QString, QCassandraTable::pointer_t > QCassandraTables;

} // namespace QtCassandra

// vim: ts=4 sw=4 et
