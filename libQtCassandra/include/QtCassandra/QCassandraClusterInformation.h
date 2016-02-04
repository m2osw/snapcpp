/*
 * Header:
 *      QCassandraClusterInformation.h
 *
 * Description:
 *      Handling of the node information in the Cassandra cluster or
 *      Ring.
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
#ifndef QCASSANDRA_CLUSTER_INFORMATION_H
#define QCASSANDRA_CLUSTER_INFORMATION_H

#include "QCassandraCell.h"
#include "QCassandraColumnPredicate.h"
#include <QUuid>
#include <memory>

namespace QtCassandra
{

// Cassandra Node Description
class QCassandraNode
{
public:
    QString nodeHost() const;
    QString nodeRack() const;
    QString nodeDataCenter() const;

    QString startToken() const;
    QString endToken() const;
    QString startRPCToken() const;
    QString endRPCToken() const;

private:
    friend class QCassandraPrivate;

    QString             f_node_host;
    QString             f_node_rack;
    QString             f_node_data_center;
    QString             f_start_token;
    QString             f_end_token;
    QString             f_start_rpc_token;
    QString             f_end_rpc_token;
};


// Cassandra Cluster Information
class QCassandraClusterInformation
{
public:
    bool isEmpty() const;
    int size() const;
    const QCassandraNode& node(int idx) const;

    const QString& startToken() const;
    const QString& endToken() const;

private:
    friend class QCassandraPrivate;

    QString                     f_start_token;
    QString                     f_end_token;
    QVector<QCassandraNode>     f_nodes;
};


} // namespace QtCassandra
#endif
//#ifndef QCASSANDRA_CLUSTER_INFORMATION_H
// vim: ts=4 sw=4 et
