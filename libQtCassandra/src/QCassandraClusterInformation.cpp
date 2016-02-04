/*
 * Text:
 *      QCassandraClusterInformation.cpp
 *
 * Description:
 *      Read information about the nodes in the Cassandra cluster.
 *
 * Documentation:
 *      See each function below.
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

#pragma GCC push
#pragma GCC diagnostic ignored "-Wundef"
#include "QCassandraPrivate.h"
#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>
#include <sys/time.h>
#pragma GCC pop

namespace QtCassandra
{



/** \class QCassandraNode
 * \brief This node information class.
 *
 * This class is used to store in memory information about one node of
 * your Cassandra cluster.
 *
 * At this point, the class information we have available is:
 *
 * \li Host -- the host on which the node resides
 * \li Rack -- the name of the rack where the host resides
 * \li Data Center -- the name of the data center where the rack resides
 */

/** \var QCassandraNode::f_node_host
 * \brief The name of the host running this node instance.
 *
 * Each Cassandra node, forming a cluster, is given a basic name
 * which is the name of the computer (hostname).
 *
 * This variable will hold that name.
 *
 * \note
 * The host name may just be the local IP address (or even 127.0.0.1
 * for a single node you are running your development system.)
 */

/** \var QCassandraNode::f_node_rack
 * \brief The name of the rack including this computer.
 *
 * Each Cassandra node runs on a computer which is part of a
 * rack.
 *
 * This variable will hold that name.
 */

/** \var QCassandraNode::f_node_data_center
 * \brief The name of the data center including this computer.
 *
 * Each Cassandra node runs on a computer which is part of a
 * data center.
 *
 * This variable will hold that name.
 */

/** \var QCassandraNode::f_start_token;
 * \brief The start token as a string.
 *
 * This value is a string representing the start token of this
 * node.
 */

/** \var QCassandraNode::f_end_token
 * \brief The end token as a string.
 *
 * This value is a string representing the end token of this node.
 */

/** \var QCassandraNode::f_start_rpc_token;
 * \brief The start RPC token as a string.
 *
 * This value is a string representing the start RPC token of this node.
 */

/** \var QCassandraNode::f_end_rpc_token;
 * \brief The end RPC token as a string.
 *
 * This value is a string representing the end RPC token of this node.
 */



/** \brief The name of the computer with this node.
 *
 * The node runs on a computer which is given a name as defined by
 * this function.
 *
 * This is generally a name you can trust since it is taken from the
 * computer and is clearly a valid name.
 *
 * \return The host name running this node.
 */
QString QCassandraNode::nodeHost() const
{
    return f_node_host;
}


/** \brief The name of the rack that includes the computer with this node.
 *
 * This name represents a rack used to hold the computer running this
 * node. In many cases this is just marked as RC1 if you are using
 * a third party cloud system since you cannot really otherwise
 * know which rack the computer is running on.
 *
 * So the name of the rack may be funky. If you are in full control of
 * your hardware, though, you should make sure that the rack names are
 * correct. It will help you in the long run to know which rack is down
 * and which rack is up.
 *
 * \return The name of the rack this node is part of.
 */
QString QCassandraNode::nodeRack() const
{
    return f_node_rack;
}


/** \brief The name of the data center in which this computer is included.
 *
 * The node runs on a computer which is in a rack (see nodeRack() for
 * details) and that rack is in a data center.
 *
 * This function returns the name of that data center. You should make
 * sure that the data center name is correct because Cassandra uses it
 * to better control inter data center copying mechanism.
 *
 * The copy mechanisms can change depending on whether a node is
 * within a certain rack and within a certain data center.
 *
 * \return The name of the data center this node is part of.
 */
QString QCassandraNode::nodeDataCenter() const
{
    return f_node_data_center;
}


/** \brief The start token of this node.
 *
 * Each node is given a set of tokens defined within a range. This
 * is the start, inclusive, of the range.
 *
 * The first range starts at 0.
 *
 * \return The start token.
 */
QString QCassandraNode::startToken() const
{
    return f_start_token;
}


/** \brief The end token of this node.
 *
 * Each node is given a set of tokens defined within a range. This
 * is the end of the range.
 *
 * \return The end token.
 */
QString QCassandraNode::endToken() const
{
    return f_end_token;
}


/** \brief The start RPC token of this node.
 *
 * Each node is given a set of tokens defined within a range. This
 * is the start, inclusive, of the range.
 *
 * The first range starts at 0.
 *
 * \return The start RPC token.
 */
QString QCassandraNode::startRPCToken() const
{
    return f_start_rpc_token;
}


/** \brief The end RPC token of this node.
 *
 * Each node is given a set of tokens defined within a range. This
 * is the end of the range.
 *
 * \return The end RPC token.
 */
QString QCassandraNode::endRPCToken() const
{
    return f_end_rpc_token;
}





/** \class QCassandraClusterInformation
 * \brief This class is used to get the Cassandra cluster information.
 *
 * This class is used to gather and store in memory information about
 * your Cassandra cluster.
 *
 * At this point, the function reads information about the Cassandra
 * ring. This is each node with their host, rack, and data center
 * names, as well as their start and end tokens.
 */

/** \var QCassandraClusterInformation::f_start_token
 * \brief The start token of the whole cluster.
 *
 * The Cassandra cluster of nodes handles a set of tokens as defined
 * on each node. This value represents the start of the entire cluster.
 * Generally the start is zero.
 */

/** \var QCassandraClusterInformation::f_end_token
 * \brief The end token of the whole cluster.
 *
 * The Cassandra cluster of nodes handles a set of tokens as defined
 * on each node. This value represents the end of the entire cluster.
 * Generally the end is zero.
 */

/** \var QCassandraClusterInformation::f_nodes
 * \brief A vector of nodes.
 *
 * This varibale holds the set of nodes found in the Cassandra
 * cluster you connected to.
 */




/** \brief Check whether the cluster information is empty.
 *
 * This function check to know whether at least one node was
 * found in your cluster.
 *
 * \return true if the cluster is empty (not running?)
 */
bool QCassandraClusterInformation::isEmpty() const
{
    return f_nodes.isEmpty();
}


/** \brief Get the number of nodes defined in this cluster.
 *
 * This function returns the number of nodes found in this cluster.
 *
 * The size is zero if the cluster information could not be gathered or
 * was not yet gathered.
 *
 * The number represents the maximum index one can use with the
 * node() function, that very value exluded.
 *
 * \return The number of nodes found in this cluster.
 */
int QCassandraClusterInformation::size() const
{
    return f_nodes.size();
}


/** \brief Retrieve a constant reference to a node.
 *
 * This function gets a reference to the node specified by \p idx.
 *
 * \param[in] idx  The index of the node to retrieve.
 *
 * \return The reference of the node at the specified index.
 */
const QCassandraNode& QCassandraClusterInformation::node(int idx) const
{
    if(static_cast<uint32_t>(idx) >= static_cast<uint32_t>(f_nodes.size()))
    {
        throw std::runtime_error("QCassandraClusterInformation::node(): index cannot be out of range or negative");
    }
    return f_nodes[idx];
}


/** \brief The cluster start token.
 *
 * Each node has a start and end token. This function returns the start
 * token of the "first" node.
 *
 * \return The cluster start token as a string.
 */
const QString& QCassandraClusterInformation::startToken() const
{
    return f_start_token;
}


/** \brief The cluster end token.
 *
 * Each node has a start and end token. This function returns the end
 * token of the "last" node.
 *
 * \return The cluster end token as a string.
 */
const QString& QCassandraClusterInformation::endToken() const
{
    return f_end_token;
}



} // namespace QtCassandra
// vim: ts=4 sw=4 et

