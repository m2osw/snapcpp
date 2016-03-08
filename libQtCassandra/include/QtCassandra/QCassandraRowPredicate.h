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

#include "QtCassandra/QCassandraColumnPredicate.h"

#include <memory>

namespace QtCassandra
{

class QCassandraRowPredicate
{
public:
    QCassandraRowPredicate();

    QString			  startRowName() const;
    void 			  setStartRowName(const QString& row_name);
    const QByteArray& startRowKey() const;
    void 		      setStartRowKey(const QByteArray& row_key);

    QString			  endRowName() const;
    void			  setEndRowName(const QString& row_name);
    const QByteArray& endRowKey() const;
    void			  setEndRowKey(const QByteArray& row_key);

    //QRegExp rowNameMatch() const;
    //void setRowNameMatch(QRegExp const& re);

    int32_t count() const;
    void    setCount( const int32_t count = 100 );

    //bool wrap() const;
    //void setWrap(bool wrap = true);

    QCassandraColumnPredicate::pointer_t columnPredicate() const;
    void                                 setColumnPredicate(QCassandraColumnPredicate::pointer_t column_predicate);

private:
    typedef controlled_vars::limited_auto_init<int32_t, 1, INT_MAX, 100> cassandra_count_t;
    cassandra_count_t                       f_count;

    QCassandraColumnPredicate::pointer_t    f_column_predicate;
};

} // namespace QtCassandra

// vim: ts=4 sw=4 et
