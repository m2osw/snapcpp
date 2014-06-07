/*
 * Header:
 *      QCassandraColumnPredicate.h
 *
 * Description:
 *      Handling of the cassandra::SlicePredicate to retrieve a set of columns
 *      all at once.
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
#ifndef QCASSANDRA_COLUMN_PREDICATE_H
#define QCASSANDRA_COLUMN_PREDICATE_H

#include "QCassandraConsistencyLevel.h"
#include <controlled_vars/controlled_vars_limited_auto_init.h>
#include <controlled_vars/controlled_vars_limited_auto_enum_init.h>
#include <QByteArray>
#include <QVector>
#include <QObject>
#include <memory>

namespace QtCassandra
{

class QCassandraPrivate;

// count must be positive and defaults to 100
typedef controlled_vars::limited_auto_init<int32_t, 1, INT_MAX, 100> cassandra_count_t;

// Predicates for the Cassandra get_slice() function

// common predicate information
class QCassandraColumnPredicate //: public QObject -- predicates are copyable and not named
{
public:
    typedef std::shared_ptr<QCassandraColumnPredicate> pointer_t;

    QCassandraColumnPredicate();
    virtual ~QCassandraColumnPredicate();

    // The name predicates can have any character from \0 to \uFFFD
    // (although in full Unicode, you may want to use \U10FFFD but at this
    // point I limit the code to \uFFFD because QChar uses a ushort)
    //
    // Note: Qt strings use UTF-16, but in a QChar, I'm not too sure we
    //       can put a value more than 0xFFFF... so we'd need the last_char
    //       to be a QString to support the max. character in Unicode!
    static const QChar first_char;
    static const QChar last_char;

    consistency_level_t consistencyLevel() const;
    void setConsistencyLevel(consistency_level_t consistency_level);

private:
    virtual void toPredicate(void *data) const;

    friend class QCassandraPrivate;

    consistency_level_t         f_consistency_level;
};

// name based predicate (specific names)
class QCassandraColumnNamePredicate : public QCassandraColumnPredicate
{
public:
    typedef std::shared_ptr<QCassandraColumnNamePredicate> pointer_t;
    typedef QVector<QByteArray> QCassandraColumnKeys;

    QCassandraColumnNamePredicate();

    void clearColumns();
    void addColumnName(const QString& column_name);
    void addColumnKey(const QByteArray& column_key);
    const QCassandraColumnKeys& columnKeys() const;

private:
    virtual void toPredicate(void *data) const;

    QCassandraColumnKeys        f_column_keys;
};

// range based predicate (all columns between a specific range)
class QCassandraColumnRangePredicate : public QCassandraColumnPredicate
{
public:
    typedef std::shared_ptr<QCassandraColumnRangePredicate> pointer_t;
    QCassandraColumnRangePredicate();

    QString startColumnName() const;
    void setStartColumnName(const QString& column_name);
    const QByteArray& startColumnKey() const;
    void setStartColumnKey(const QByteArray& column_key);

    QString endColumnName() const;
    void setEndColumnName(const QString& column_name);
    const QByteArray& endColumnKey() const;
    void setEndColumnKey(const QByteArray& column_key);

    bool reversed() const;
    void setReversed(bool reversed = true);

    int32_t count() const;
    void setCount(int32_t count = 100);
    bool index() const;
    void setIndex(bool new_index = true);

private:
    virtual void toPredicate(void *data) const;
    void setLastKey(const QByteArray& column_key);
    bool excludeFirst() const;

    friend class QCassandraPrivate;

    QByteArray                  f_start_column;
    QByteArray                  f_end_column;
    controlled_vars::flbool_t   f_reversed;
    controlled_vars::flbool_t   f_index; // whether predicate is used as an index
    controlled_vars::flbool_t   f_exclude; // whether f_start_column is excluded
    cassandra_count_t           f_count;
};

} // namespace QtCassandra
#endif
//#ifndef QCASSANDRA_COLUMN_PREDICATE_H
// vim: ts=4 sw=4 et
