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
 *      Copyright (c) 2011-2013 Made to Order Software Corp.
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
#ifndef QCASSANDRA_TABLE_H
#define QCASSANDRA_TABLE_H

#include "QCassandraColumnDefinition.h"
#include "QCassandraRowPredicate.h"
#include "QCassandraRow.h"
#include <QPointer>
// GNU does not officially offer cstdint yet
#include <stdint.h>

namespace QtCassandra
{

class QCassandraTablePrivate;
class QCassandraContext;


// Cassandra Column Family (org::apache::cassandra::CfDef)
class QCassandraTable : public QObject
{
public:
    virtual ~QCassandraTable();

    // context name
    QString contextName() const;

    // column family name & identifier
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
    QSharedPointer<QCassandraColumnDefinition> columnDefinition(const QString& column_name);
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
    void update();
    void truncate();
    void clearCache();

    // row handling
    uint32_t readRows(QCassandraRowPredicate& row_predicate);

    QSharedPointer<QCassandraRow> row(const char *row_name);
    QSharedPointer<QCassandraRow> row(const wchar_t *row_name);
    QSharedPointer<QCassandraRow> row(const QString& row_name);
    QSharedPointer<QCassandraRow> row(const QUuid& row_uuid);
    QSharedPointer<QCassandraRow> row(const QByteArray& row_key);
    const QCassandraRows& rows() const;

    QSharedPointer<QCassandraRow> findRow(const char *row_name) const;
    QSharedPointer<QCassandraRow> findRow(const wchar_t *row_name) const;
    QSharedPointer<QCassandraRow> findRow(const QString& row_name) const;
    QSharedPointer<QCassandraRow> findRow(const QUuid& row_uuid) const;
    QSharedPointer<QCassandraRow> findRow(const QByteArray& row_key) const;
    bool exists(const char *row_name) const;
    bool exists(const wchar_t *row_name) const;
    bool exists(const QString& row_name) const;
    bool exists(const QUuid& row_uuid) const;
    bool exists(const QByteArray& row_key) const;
    QCassandraRow& operator[] (const char *row_name);
    QCassandraRow& operator[] (const wchar_t *row_name);
    QCassandraRow& operator[] (const QString& row_name);
    QCassandraRow& operator[] (const QUuid& row_uuid);
    QCassandraRow& operator[] (const QByteArray& row_key);
    const QCassandraRow& operator[] (const char *row_name) const;
    const QCassandraRow& operator[] (const wchar_t *row_name) const;
    const QCassandraRow& operator[] (const QString& row_name) const;
    const QCassandraRow& operator[] (const QUuid& row_uuid) const;
    const QCassandraRow& operator[] (const QByteArray& row_key) const;

    void dropRow(const char *row_name, QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO, int64_t timestamp = 0, consistency_level_t consistency_level = CONSISTENCY_LEVEL_ALL);
    void dropRow(const wchar_t *row_name, QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO, int64_t timestamp = 0, consistency_level_t consistency_level = CONSISTENCY_LEVEL_ALL);
    void dropRow(const QString& row_name, QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO, int64_t timestamp = 0, consistency_level_t consistency_level = CONSISTENCY_LEVEL_ALL);
    void dropRow(const QUuid& row_uuid, QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO, int64_t timestamp = 0, consistency_level_t consistency_level = CONSISTENCY_LEVEL_ALL);
    void dropRow(const QByteArray& row_key, QCassandraValue::timestamp_mode_t mode = QCassandraValue::TIMESTAMP_MODE_AUTO, int64_t timestamp = 0, consistency_level_t consistency_level = CONSISTENCY_LEVEL_ALL);

private:
    QCassandraTable(QCassandraContext *context, const QString& table_name);

    void setFromCassandra();
    void parseTableDefinition(const void *data);
    void prepareTableDefinition(void *data) const;
    void insertValue(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    bool getValue(const QByteArray& row_key, const QByteArray& column_key, QCassandraValue& value);
    void addValue(const QByteArray& row_key, const QByteArray& column_key, int64_t value);
    void assignRow(const QByteArray& row_key, const QByteArray& column_key, const QCassandraValue& value);
    int32_t getCellCount(const QByteArray& row_key, const QCassandraColumnPredicate& column_predicate);
    uint32_t getColumnSlice(const QByteArray& row_key, QCassandraColumnPredicate& column_predicate);
    void remove(const QByteArray& row_key, const QByteArray& column_key, int64_t timestamp, consistency_level_t consistency_level);
    void unparent();

    friend class QCassandraPrivate;
    friend class QCassandraContext;
    friend class QCassandraTablePrivate;
    friend class QCassandraRow;

    controlled_vars::zbool_t                    f_from_cassandra;
    std::auto_ptr<QCassandraTablePrivate>       f_private;
    // f_context is a parent that has a strong shared pointer over us so it
    // cannot disappear before we do, thus only a bare pointer is enough here
    // (there isn't a need to use a QWeakPointer or QPointer either)
    QCassandraContext *                         f_context;
    QCassandraColumnDefinitions                 f_column_definitions;
    QCassandraRows                              f_rows;
};
// list of table definitions mapped against their name (see tableName())
typedef QMap<QString, QSharedPointer<QCassandraTable> > QCassandraTables;



} // namespace QtCassandra
#endif
//#ifndef QCASSANDRA_TABLE_H
// vim: ts=4 sw=4 et
