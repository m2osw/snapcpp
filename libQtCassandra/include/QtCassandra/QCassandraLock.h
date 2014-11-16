/*
 * Header:
 *      QCassandraLock.h
 *
 * Description:
 *      The Cassandra Lock is an implementation of the Lamport's bakery algorith.
 *      It can be used to lock tables, rows, or cells. There are many limitations
 *      as noted in the documentation of the QCassandraLock class.
 *
 * Documentation:
 *      See the corresponding .cpp file.
 *
 * License:
 *      Copyright (c) 2013 Made to Order Software Corp.
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
#ifndef QCASSANDRA_LOCK_H
#define QCASSANDRA_LOCK_H

#include "QCassandraTable.h"
#include "QCassandraContext.h"

namespace QtCassandra
{

// QtCassandra lock object
class QCassandraLock : public QObject
{
public:
    QCassandraLock(QCassandraContext::pointer_t context, const QString& object_name = "", const cassandra_consistency_level_t consistency_level = CONSISTENCY_LEVEL_QUORUM);
    QCassandraLock(QCassandraContext::pointer_t context, const QByteArray& object_key, const cassandra_consistency_level_t consistency_level = CONSISTENCY_LEVEL_QUORUM);
    virtual ~QCassandraLock();

    bool lock(const QString& object_name);
    bool lock(const QByteArray& object_key);
    void unlock();

private:
    void internal_init(const QByteArray& object_name);

    QCassandraContext::pointer_t   f_context;
    QCassandraTable::pointer_t     f_table;
    QByteArray                     f_object_name;
    QByteArray                     f_ticket_id;
    controlled_vars::flbool_t      f_locked;
    const consistency_level_t      f_consistency;
};

} // namespace QtCassandra
#endif
//#ifndef QCASSANDRA_LOCK_H
// vim: ts=4 sw=4 et
