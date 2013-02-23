/*
 * Header:
 *      QCassandraRowPredicate.h
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
#ifndef QCASSANDRA_ROW_PREDICATE_H
#define QCASSANDRA_ROW_PREDICATE_H

#include "QCassandraColumnPredicate.h"
#include <QSharedPointer>

namespace QtCassandra
{

class QCassandraPrivate;

// Predicates for the Cassandra get_slice() function

// common predicate information
class QCassandraRowPredicate //: public QObject -- predicates are copyable and not named
{
public:
    QCassandraRowPredicate();

    QString startRowName() const;
    void setStartRowName(const QString& row_name);
    const QByteArray& startRowKey() const;
    void setStartRowKey(const QByteArray& row_key);

    QString endRowName() const;
    void setEndRowName(const QString& row_name);
    const QByteArray& endRowKey() const;
    void setEndRowKey(const QByteArray& row_key);

    int32_t count() const;
    void setCount(int32_t count = 100);

    bool wrap() const;
    void setWrap(bool wrap = true);

    QSharedPointer<QCassandraColumnPredicate> columnPredicate() const;
    void setColumnPredicate(QSharedPointer<QCassandraColumnPredicate> column_predicate);

private:
    virtual void toPredicate(void *data) const;
    void setLastKey(const QByteArray& row_key);
    bool excludeFirst() const;

    friend class QCassandraPrivate;

    QByteArray                                  f_start_row;
    QByteArray                                  f_end_row;
    cassandra_count_t                           f_count;
    controlled_vars::zbool_t                    f_wrap; // i.e. KeyRange tokens versus keys
    controlled_vars::zbool_t                    f_exclude; // whether f_start_row is excluded
    QSharedPointer<QCassandraColumnPredicate>   f_column_predicate;
};

} // namespace QtCassandra
#endif
//#ifndef QCASSANDRA_ROW_PREDICATE_H
// vim: ts=4 sw=4 et
