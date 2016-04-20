/*
 * Text:
 *      QCassandra.cpp
 *
 * Description:
 *      Handling of the cassandra::CassandraClient and corresponding transports,
 *      protocols, sockets, etc.
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
#include <sys/time.h>
#pragma GCC pop

#include "QtCassandra/QCassandra.h"
#include "QtCassandra/QCassandraSchema.h"

#include <QtCore>

#include <cassandra.h>

#include <iostream>
#include <sstream>

/** \brief The QtCassandra namespace includes all the Cassandra extensions.
 *
 * This namespace includes all the Cassandra extensions in Qt. Note that we
 * suggest that you try to avoid using this namespace as in:
 *
 * \code
 * using QtCassandra;
 * \endcode
 *
 * Yet... without using the namespace, all the [] operators are not accessible
 * so the advanced C++ syntax is not available to you. In other words, you
 * should have such statements wherever you want to access the QtCassandra
 * data with the advanced C++ syntax.
 */
namespace QtCassandra
{

using namespace QCassandraSchema;

/** \mainpage
 *
 * \image html logo.png
 *
 * \section summary Summary
 *
 * -- \ref libqtcassandra
 *
 * \par
 * \ref cpplib
 *
 * \par
 * \ref organization
 *
 * \par
 * \ref communication
 *
 * \par
 * \ref threading
 *
 * \par
 * \ref qt
 *
 * -- \ref cassandra
 *
 * \par
 * \ref terminology
 *
 * \par
 * \ref faq
 *
 * \par
 * \ref changes
 *
 * -- \ref copyright
 *
 * \section libqtcassandra libQtCassandra
 *
 * \subsection cpplib A C++ library
 *
 * It looks like there was a need for a C++ library for the Cassandra system.
 * Looking around the only one available was indeed in C++ and helping a bit
 * by hiding some of the Thrift implementation, but in the end you still had
 * to know how the Thrift structure had to be used to make it all work.
 *
 * So, I decided to write my own version, especially, I was thinking that it
 * should be possible to access the data with the following simple array like
 * syntax:
 *
 * \code
 *   cluster[context][table][row][column] = value;
 *   value = cluster[context][table][row][column];
 * \endcode
 *
 * In other words, you should be able to create a cluster object and then from
 * that object access the contexts (keyspaces), the tables (column families),
 * the rows and columns (actually, the values are saved in cells in this
 * library, but the name used in the cell is the name of the column.)
 *
 * Looking at the implementation of a system I will create soon after having
 * done this library it very much feels like this is going to be the basis for
 * many accesses to the Cassandra database.
 *
 * \todo
 * The one thing still missing is a clean way of handling the exception. At
 * this time the libQtCassandra library generate runtime_error and logic_error
 * when you missuse a function. It also captures some Thrift exception
 * in a few cases (although we try to avoid doing so, the connect() is the
 * main one that we capture so you can attempt to connect and instead of
 * throwing you get a return value of false when the connect failed.)
 * But... at this time, if you get a Cassandra Thrift exception, you cannot
 * do anything with it unless you include the Cassandra Thrift header files
 * (otherwise you don't get the declaration of these exceptions.)
 * Thus, we should overload all the exceptions and have try/catch around
 * all the calls to the Cassandra Thrift interface and convert those
 * exception in a libQtCassandra exception.
 *
 * \subsection organization The libQtCassandra organization
 *
 * \li Cluster
 *
 * The cluster is defined in a QCassandra object. At the time you call connect()
 * you get a connection to the Cassandra server and you can start using the
 * other functions.
 *
 * Clusters are used to manage contexts and to access the Cassandra server.
 *
 * Note that internally all accesses to the Cassandra system is done with
 * one f_client pointer.
 *
 * \li Contexts
 *
 * A cluster can have many contexts. It has the default System contexts that
 * comes with the Cassandra environment. Others can be create()'d, and opened
 * on later connections.
 *
 * Contexts can be used to manage tables.
 *
 * \li Tables
 *
 * A table has a name and it includes one to many rows. The number of rows can
 * be really large and not impact the Cassandra cluster much.
 *
 * The readRows() function is used to read a range of rows as determined by
 * the row predicate.
 *
 * \li Column Definitions
 *
 * A table may have a list of column definitions. You generally do not need a
 * column definition unless you want to force a certain type of data in a
 * column. In that case a column definition is required (Assuming that the
 * default type is not what you want for that column.)
 *
 * We do not use this information internally. Only to forward to the Cassandra
 * server (and also we read the current status from the Cassandra server.)
 *
 * At a later time, we may check the type defined here and in the row (if not
 * defined as a column type) and check the data supplied to verify that the
 * data passed to the row is valid before sending it to the server. This is
 * probably not necessary because if you have bad data your application won't
 * work, no matter what.
 *
 * \li Rows
 *
 * A row is named using a key, which means the name can include any
 * character including '\0' (it just cannot be empty.) The key can actually
 * be an integer, or even a complete structure. Rows include cells.
 *
 * Contrary to an SQL engine where the number of columns is generally relatively
 * restrained, the number of cells in a row can be very large and accessing a
 * specific cell remains fast (assuming the cell names remain reasonably small.)
 *
 * For this reason, when you read a row, you do not have to read all the
 * cells on that row. Instead, you may read just one or two cells.
 *
 * Row names can use UUIDs, in that case use a QUuid object and directly call
 * the different table functions that accept a row key name.
 *
 * \li Cells
 *
 * A cell is named using a column key, which means the name can include any
 * character including '\0' (it just cannot be empty.) Just like for rows,
 * the key can be an integer, or a structure. With a well organized structure
 * you can actually sort your data in some pretty advanced manner.
 *
 * Cell keys are attached to a value. This value is limited to 2Gb in size,
 * although remember that sending 2Gb over a network connection is going to
 * be slow and the entire data of one cell must be transferred when written
 * or read.
 *
 * Note that when you create an index, you may use a default value (including
 * the Null value.) For example, if you have a Page table with a row numbered
 * 3 and that row has a cell named Path with a value X. You may add this row
 * like this (somewhat simplified for this example):
 *
 * \code
 * // add the value "X" to row "3" in table "Page"
 * int64_t row_number = 3;
 * QCassandraValue row_id;
 * row_id = row_number;
 * QCassandraValue value;
 * value = "X";
 * context["Page"][row_id]["Path"] = value;
 *
 * // "simultaneously," create an index from value to row identifier
 * QCassandraValue empty;
 * context["Page_Path_Index"][value.binaryValue()][row_id] = empty;
 *
 * // search rows in pages that have a path set to value
 * context["Page_Path_Index"][value.binaryValue()].readCells(predicate);
 * \endcode
 *
 * As you can see, the second write uses the empty value so you do not waste
 * any space in the database (note however that at this time there is a
 * problem with the QRow::exists() function which creates a cell when it
 * doesn't exist so an empty value may not always be practical.)
 *
 * Note that when writing a value to a cell, the consistency level of the
 * value is used. When reading a cell, the value is returned so its
 * consistency level cannot be changed by you before the call. By default
 * the value as defined by the setDefaultConsistencyLevel() function is
 * used (the default being ONE.) You may change that default if you are
 * expected to use that consistency level in the majority of places.
 * Otherwise, you can also set the consistency level on a cell. Remember
 * that this is used for reads only. There are additional details in the
 * QCassandraCell::setConsistencyLevel() function
 *
 * Cell names can use UUIDs, in that case use a QUuid object and directly call
 * the different row functions that accept a cell key name.
 *
 * \li Values
 *
 * The cells are set to a specific value using the QCassandraValue class.
 * Beside their binary data, values have a timestamp that represents the time
 * and date when they were created and a TTL (time to live) value in seconds.
 * By default the timestamp is set to gettimeofday() and the TTL is set to 0
 * (which means permanent.)
 *
 * The values are cached in memory by the libQtCassandra library. Not only
 * that, multiple write of the same value to the same cell will generate a
 * single write to the Cassandra database (i.e. the timestamp is ignored in
 * this case, see the QCassandraCell class for more info.)
 *
 * Values also include a consistency level. By default this is set to ONE
 * which may not be what you want to have... (in many cases QUORUM is
 * better for writes.) The consistency level is defined here because it can
 * then easily be propagated when using the array syntax.
 *
 * \code
 * QCassandraValue v;
 * v.setDoubleValue(3.14159);
 * v.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_EACH_QUORUM);
 * v.setTimestamp(counter);
 * v.setTtl(60 * 60 * 24);  // live for 1 day
 * ...
 * \endcode
 *
 * \image html CassandraOrganization.png
 *
 * \subsection communication Communication between objects
 *
 * In general, the objects communicate between parent and child. However, some
 * times the children need to access the QCassandraPrivate functions to send
 * an order to the Cassandra server. This is done by implementing functions
 * in the parents and calling functions in cascade.
 *
 * The simplest message sent to the Cassandra server comes from the connect()
 * which is the first time happen. It starts from the QCassandra object and
 * looks something like this:
 *
 * \msc
 * QCassandra,QCassandraPrivate,Thrift,Cassandra;
 * QCassandra=>QCassandraPrivate [label="connect()"];
 * QCassandraPrivate=>Thrift [label="connect()"];
 * Thrift->Cassandra [label="[RPC Call]"];
 * ...;
 * Cassandra->Thrift [label="[RPC Reply]"];
 * Thrift>>QCassandraPrivate [label="return"];
 * QCassandraPrivate>>QCassandra [label="return"];
 * \endmsc
 *
 * When you save a QCassandraValue in a QCassandraCell, then the cell calls
 * the row, which calls the table, which calls the context which has access
 * to the QCassandraPrivate:
 *
 * \msc
 * width=900;
 * QCassandraCell,QCassandraRow,QCassandraTable,QCassandraContext,QCassandra,QCassandraPrivate,Thrift,Cassandra;
 * QCassandraCell=>QCassandraRow [label="insertValue()"];
 * QCassandraRow=>QCassandraTable [label="insertValue()"];
 * QCassandraTable=>QCassandraContext [label="insertValue()"];
 * QCassandraContext=>QCassandra [label="makeCurrent()", linecolor="red"];
 * QCassandra abox Cassandra [label="if context is not already current"];
 * QCassandra=>QCassandraPrivate [label="setContext()"];
 * QCassandraPrivate=>Thrift [label="set_keyspace()"];
 * Thrift->Cassandra [label="[RPC Call]"];
 * ...;
 * Cassandra->Thrift [label="[RPC Reply]"];
 * Thrift>>QCassandraPrivate [label="return"];
 * QCassandraPrivate>>QCassandra [label="return"];
 * QCassandra abox Cassandra [label="end if"];
 * QCassandra>>QCassandraContext [label="return"];
 * QCassandraContext=>QCassandra [label="getPrivate()"];
 * QCassandra>>QCassandraContext [label="return"];
 * QCassandraContext=>QCassandraPrivate [label="insertValue()"];
 * QCassandraPrivate=>Thrift [label="insert()"];
 * Thrift->Cassandra [label="[RPC Call]"];
 * ...;
 * Cassandra->Thrift [label="[RPC Reply]"];
 * Thrift>>QCassandraPrivate [label="return"];
 * QCassandraPrivate>>QCassandraContext [label="return"];
 * QCassandraContext>>QCassandraTable [label="return"];
 * QCassandraTable>>QCassandraRow [label="return"];
 * QCassandraRow>>QCassandraCell [label="return"];
 * \endmsc
 *
 * As you can see the context makes itself current as required. This ensures
 * that the correct context is current when a Cassandra RPC event is sent.
 * This is completely automatic so you do not need to know whether the
 * context is current. Note that the function is optimized (i.e. the pointer
 * of the current context is saved in the QCassandra object so if it doesn't
 * change we avoid the set_keyspace() call.)
 *
 * Messages from rows, tables, and contexts work the same way, only they do
 * not include calls from the lower layers.
 *
 * The drop() calls have one additional call which deletes the children that
 * were just dropped. (i.e. if you drop a row, then its cells are also dropped.)
 *
 * \subsection threading Multi-thread support
 *
 * The libQtCassandra library is <strong>NOT</strong> multi-thread safe. This
 * is for several reasons, the main one being that we are not currently looking
 * into writing multi-threaded applications (on a server with a heavy load, you
 * have many processes running in parallel and having multiple threads in each
 * doesn't save you anything.)
 *
 * If you plan to have multiple threads, I currently suggest you create one
 * QCassandra object per thread. The results will be similar, although it will
 * make use of more memory and more accesses to the Cassandra server (assuming
 * each thread accesses the common data, in that case you probably want to
 * manage
 * your own cache of the data.)
 *
 * \subsection qt Why Qt?
 *
 * First of all, calm down, you won't need to run X or some other graphical
 * environment. We only use QtCore.
 *
 * Qt has many objects ready made that work. It seems to make sense to use
 * them. More specifically, it has a QString which supports UTF-8 out of the
 * box. So instead of using iconv and std::string, Qt seems like a good choice
 * to me.
 *
 * The main objects we use are the QString, QByteArray, QMap, QVector, and
 * QObject. The QByteArray is used to transport binary data. The QMap is
 * used to sort out the list of contexts, tables, rows, cells, and column
 * definitions.
 *
 * Note that the QByteArray has a great advantage over having your own buffer:
 * it knows how to share its data pointer between multiple instances. Only a
 * write will require a copy of the data buffer.
 *
 * Many of the classes in the libQtCassandra derive from the QObject class.
 * This means they cannot be copied. Especially, the QCassandra object
 * which includes the socket connection to the Cassandra server (on top of
 * the contexts, tables, rows, cells, column definitions... and since they
 * all have a parent/child scheme!)
 *
 * \todo
 * In regard to QObject's, we want to make use of the QObject name field to save
 * smaller, QString names in objects. That way you could look for an object
 * in memory with the find<>() algorithm.
 *
 * \todo
 * The QObject is capable of managing parent/children. We want to make use
 * of that capability for the cluster's contexts, the context's tables,
 * the table's rows, the row's cells. That way the data is managed 100% like
 * a Qt tree which would allow some algorithms to work on the tree without
 * having to know anything of the libQtCassandra library.
 *
 * \section cassandra The Cassandra System
 *
 * \subsection terminology Terminology Explained
 *
 * The Cassandra System comes with a terminology that can easily throw off
 * people who are used to more conventional database systems (most of that
 * terminology comes from the
 * <a href="http://snapwebsites.org/book/other-interesting-projects#big_tables">
 * Big Table document by Google</a>.)
 *
 * This library attempts to hide some of the Cassandra terminology by offering
 * objects that seem to be a little closer to what you'd otherwise expect in a
 * database environment.
 *
 * One Cassandra server instance runs against one cluster. We kept the term
 * cluster as it is the usual term for a set of databases. Writing this in
 * terms of C++ array syntax, the system looks like a multi-layer array as
 * in (you can use this syntax with libQtCassandra, btw):
 *
 * \code
 *   cluster[context][table][row][column] = value;
 *   value = cluster[context][table][row][column];
 * \endcode
 *
 * Note that in Cassandra terms, it would look like this instead:
 *
 * \code
 *   cluster[keyspace][column_family][key][column] = value;
 *   value = cluster[keyspace][column_family][key][column];
 * \endcode
 *
 * One cluster is composed of multiple contexts, what Cassandra calls a
 * keyspace. One context corresponds to one database. A context can be
 * setup to replicate or not and it manages memory caches (it includes
 * many replication and cache parameters.) We call these the context
 * because once a cluster connection is up, you can only have one
 * active context at a time. (If you worked with OpenGL, then this
 * is very similar to the glMakeCurrent() function call.)
 *
 * Although the libQtCassandra library 100% hides the current context
 * calls since it knows when a context or another needs to be curent,
 * switching between contexts can be costly. Instead you may want to
 * look into using two QCassandra objects each with a different context.
 *
 * Different contexts are useful in case you want to use one context for
 * statistic data or other data that are not required to be read as
 * quickly as your main data and possibly needs much less replication
 * (i.e. ONE for writes and ALL for reads on a statistic table would
 * work greatly.)
 *
 * One context is composed of tables, what Cassandra calls a column family.
 * By default, all the tables are expected to get replicated as defined
 * in this context. However, some data may be marked as temporary with
 * a time to live (TTL). Data with a very small TTL is likely to only
 * live in the memory cache and never make it to the disk.
 *
 * Note that the libQtCassandra library lets you create table objects
 * that do not exist in the Cassandra system. These are memory only
 * tables (when you quit they're gone!) These can be used to manage
 * run-time globals via the libQtCassandra system. Obviously, it would
 * most certainly be more effective (faster) to just use normal C/C++
 * globals. However, it can be useful to call a function that usually
 * accesses a Cassandra table, but in that case you dynamically
 * generate said data.
 *
 * A table is identified by a name. At this time, we only offer QString
 * for table names. Table names must be letters, digits and the
 * underscore. This limitation comes from the fact that it is used to
 * represent a filename. Similarly, it may be limited in length (OS
 * dependent I guess, the Cassandra system does not say from what I've
 * seen. Anyway it should be easy to keep the table names small.)
 *
 * Tables are composed of rows. Here the scheme somewhat breaks from
 * the usual SQL database as rows are independent from each others.
 * This is because one row may have 10 "columns," and the other may have
 * just 1. Each row is identified by what Cassandra calls a key. The key
 * can either be a string or a binary identifier (i.e. an int64_t for
 * example, we use QByteArray for those in libQtCassandra.)
 *
 * The name of a row can be typed. In most cases, the default binary
 * type is enough (assuming you save integers in big endians, which
 * is what the libQtCassandra does.) This is important when you want
 * to use a Row Predicate.
 *
 * Rows are composed of cells. Cassandra calls them columns, but in
 * practice, the name/value pair is just a Cell. Although some tables
 * may define column types and those cells (with the same name) will
 * then be typed and part of that column.
 *
 * A column is a name and a value pair. It is possible to change the
 * characteristics of a column on how it gets duplicated, cached, and
 * how the column gets compared with a Column Predicate.
 *
 * The name of a column can be a QString or binary data. It is often
 * a QString as it looks like the name of a variable (var=\<value>).
 *
 * The row and column names are both limited to 64Kb. The value of
 * a column is currently limited to 2Gb, however, you'll need a
 * HUGE amount of memory (~6Gb+) to be able to handle such large
 * values and not only that, you need to do it serially (i.e. one
 * process at a time can send that much data or the memory will
 * quickly exhaust and the processes will fail.) It is strongly
 * advised that you limit your values to Mb instead.
 *
 * By default, the QCassandra object checks the size with a much
 * small limit (64Mb) to prevent problems. At a later time, we may
 * offer a blob handling that will save large files by breaking
 * them up in small parts saved in the Cassandra database.
 *
 * \subsection faq FAQ
 *
 * In general, the libQtCassandra documentation will tell you how
 * things work in the function you're trying to use. This means
 * you should not really need to know all the details about the
 * Cassandra system up front to start using it in C++ with this
 * library.
 *
 * For example, the QCassandraTable::dropRow() function works.
 * The row doesn't disappear immediately from the database, but
 * all the cells are. The QCassandraTable::readRows() function
 * will correctly skip the row as long as you include at least
 * one column in your predicate. All of these details are found
 * in the corresponding function documentation.
 *
 * Actual Cassandra's FAQ: http://wiki.apache.org/cassandra/FAQ
 *
 * \section copyright libQtCassandra copyright and license
 *
 * Copyright (c) 2011-2016 Made to Order Software Corp.
 *
 * http://snapwebsites.org/<br/>
 * contact@m2osw.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/** \class QCassandra
 * \brief The Cassandra class definition.
 *
 * This class is used to handle a Cassandra connection and read/write data
 * to and from a Cassandra database.
 *
 * This is the first object you want to create as all things come out of it.
 */

/** \var const int QT_CASSANDRA_LIBRARY_VERSION_MAJOR;
 * \brief The major library version.
 *
 * This value represents the major library version at the time you
 * compile your program. To get the library you are linked to at
 * runtime use QCassandra::versionMajor().
 *
 * \sa versionMajor()
 */

/** \var const int QT_CASSANDRA_LIBRARY_VERSION_MINOR;
 * \brief The minor library version.
 *
 * This value represents the minor library version at the time you
 * compile your program. To get the library you are linked to at
 * runtime use QCassandra::versionMinor().
 *
 * \sa versionMinor()
 */

/** \var const int QT_CASSANDRA_LIBRARY_VERSION_PATCH;
 * \brief The patch library version.
 *
 * This value represents the patch library version at the time you
 * compile your program. To get the library you are linked to at
 * runtime use QCassandra::versionPatch().
 *
 * \sa versionPatch()
 */

/** \var QCassandra::f_current_context
 * \brief A pointer to the current context.
 *
 * The current context has to be set once. We save the pointer that way
 * we can avoid call the set_keyspace() function if the context you are
 * trying to make current already is current.
 *
 * The libQtCassandra library does not give you access to the
 * QCassandraContext::makeCurrent() function
 */

/** \var QCassandra::f_contexts_read
 * \brief Whether the map of contexts were read from Cassandra.
 *
 * This flag defines whether the f_contexts was already initialized or
 * not. This allows to call the describe_keyspaces() function a maximum
 * of one time per connection.
 */

/** \var QCassandra::f_contexts
 * \brief The map of contexts defined in memory.
 *
 * This variable holds the list of contexts defined in memory.
 * Whenever you try to access a context the system reads the cluster
 * description (once) and return the corresponding Cassandra database
 * context.
 *
 * It is possible to create a memory context (i.e. get a context that
 * doesn't exist in the Cassandra database and never call the
 * Context::create() function.)
 */

/** \var QCassandra::f_cluster_name
 * \brief The name of the cluster we're connected to.
 *
 * This variable holds the name of the cluster the QCassandra object
 * is connected to. This variable caches the cluster name so we don't
 * have to query the cluster about its name more than once.
 */

/** \var QCassandra::f_protocol_version
 * \brief The version of the protocol we're connected to.
 *
 * This variable holds the version of the protocol the QCassandra object
 * is connected to. This variable caches the protocol version so we don't
 * have to query the cluster about its protocol version more than once.
 */

/** \var QCassandra::f_partitioner
 * \brief The partitioner available in this Cassandra cluster.
 *
 * This variable caches the name of the partitioner as read from the
 * Cassandra cluster.
 */

/** \var QCassandra::f_snitch
 * \brief The snitch used by this Cassandra cluster.
 *
 * This variable caches the name of the snitch as read from the
 * Cassandra cluster.
 */

/** \brief Initialize the QCassandra object.
 *
 * This function makes the QCassandra object ready.
 *
 * Mainly, it allocates a QCassandraPrivate object that handles all
 * the necessary data transfers between the QCassandra object and the
 * Cassandra server.
 *
 * Next you are expected to connect to the server and eventually
 * change the default consistency level and on larger system.
 *
 * \sa connect()
 * \sa setDefaultConsistencyLevel()
 */
QCassandra::QCassandra()
    : f_session( QCassandraSession::create() )
    // f_current_context(nullptr) -- auto-init
    // f_contexts() -- auto-init
    // f_cluster_name("") -- auto-init
    // f_protocol_version("") -- auto-init
    // f_partitioner("") -- auto-init
    // f_snitch("") -- auto-init
    , f_default_consistency_level( CONSISTENCY_LEVEL_ONE )
    // default is CONSISTENCY_LEVEL_DEFAULT
{
    // we are passing this to the private object that we control
    // so we make make sure it is used wisely; at this time it is
    // just saved in a variable member
}

/** \brief Create the QCassandra instance.
 *
 * This factory creates a new object wrapped in a shared pointer. The contructor
 * is private, so this function must be used.
 *
 * \sa QCassandra()
 */
QCassandra::pointer_t QCassandra::create()
{
    return pointer_t( new QCassandra );
}

/** \brief Cleanup the Cassandra object.
 *
 * This function cleans up the QCassandra object.
 *
 * This includes disconnecting from the Cassandra server and release
 * memory resources tight to this object.
 */
QCassandra::~QCassandra()
{
    disconnect();
}

/** \brief Connect to a Cassandra Cluster.
 *
 * This function connects to a Cassandra Cluster. Which cluster is determined
 * by the host and port parameters.
 *
 * One cluster may include many database contexts (i.e. keyspaces.) Each
 * database
 * context (keyspace) has a set of parameters defining its duplication mechanism
 * among other things. Before working with a database context, one must call the
 * the setCurrentContext() function.
 *
 * The function first disconnects the existing connection when there is one.
 *
 * Many other functions require you to call this connect() function first. You
 * are likely to get a runtime exception if you don't.
 *
 * Note that the previous connection is lost whether or not the new one
 * succeeds.
 *
 * \param[in] host      The host, defaults to "localhost" (an IP address,
 * computer
 *                      hostname, domain name, etc.)
 * \param[in] port      The connection port, defaults to 9042.
 *
 * \return true if the connection succeeds, throws otherwise
 */
bool QCassandra::connect( const QString& host, const int port )
{
    QStringList host_list;
    host_list << host;
    return connect( host_list, port );
}

/** \brief Connect to a Cassandra Cluster.
 *
 * This function connects to a Cassandra Cluster. Which cluster is determined
 * by the host and port parameters.
 *
 * One cluster may include many database contexts (i.e. keyspaces.) Each
 * database
 * context (keyspace) has a set of parameters defining its duplication mechanism
 * among other things. Before working with a database context, one must call the
 * the setCurrentContext() function.
 *
 * The function first disconnects the existing connection when there is one.
 *
 * Many other functions require you to call this connect() function first. You
 * are likely to get a runtime exception if you don't.
 *
 * Note that the previous connection is lost whether or not the new one
 * succeeds.
 *
 * \param[in] host_list The list of hosts, AKA contact points (IP addresses,
 * computer
 *                      hostnames, domain names, etc.)
 * \param[in] port      The connection port, defaults to 9042.
 *
 * \return true if the connection succeeds, throws otherwise
 */
bool QCassandra::connect( const QStringList& host_list, const int port )
{
    // disconnect any existing connection
    disconnect();

    f_session = QCassandraSession::create();
    f_session->connect( host_list, port ); // throws on failure!

    QCassandraQuery local_table( f_session );
    local_table.query( "SELECT cluster_name, native_protocol_version, partitioner FROM system.local" );
    local_table.start();
    //
    if( !local_table.nextRow() )
    {
        throw std::runtime_error( "Error in database table system.local!" );
    }

    f_cluster_name     = local_table.getStringColumn( "cluster_name"            );
    f_protocol_version = local_table.getStringColumn( "native_protocol_version" );
    f_partitioner      = local_table.getStringColumn( "partitioner"             );
    //
    // I have no idea how to get this from the new CQL-based c++ interface.
    //
    f_snitch = "TODO!";

    return true;
}

/** \brief Break the connection to Cassandra.
 *
 * This function breaks the connection to Cassandra.
 *
 * This function has the side effect of clearing the cluster name,
 * protocol version, and current context.
 */
void QCassandra::disconnect()
{
    f_session.reset();

    f_current_context.reset();
    f_contexts.clear();
    f_cluster_name = "";
    f_protocol_version = "";
    f_partitioner = "";
    f_snitch = "TODO";
    f_default_consistency_level = CONSISTENCY_LEVEL_ONE;
}

/** \brief Check whether the object is connected to the server.
 *
 * This function returns true when this object is connected to the
 * backend Cassandra server.
 *
 * The function is fast and does not actually verify that the TCP/IP
 * connection is still up.
 *
 * \return true if connect() was called and succeeded.
 */
bool QCassandra::isConnected() const
{
    return f_session->isConnected();
}

/** \brief Get the name of the Cassandra cluster.
 *
 * This function determines the name of the cluster. The name cannot
 * change unless you renew the connection to a different Cassandra
 * cluster.
 *
 * The QCassandra object remembers the name. Calling this function
 * more than once is very fast.
 *
 * You must be connected for this function to work.
 *
 * \note
 * The f_cluster_name variable will be set even though the function
 * is a const. This is a cache so it makes sense. The variable is
 * NOT marked mutable because this is the only case where it happens.
 *
 * \return The name of the cluster.
 */
const QString &QCassandra::clusterName() const
{
    return f_cluster_name;
}

/** \brief Get the version of the cluster protocol.
 *
 * This function determines the version of the protocol. The version
 * cannot change unless you renew the connection to a different
 * Cassandra cluster.
 *
 * The QCassandra object remembers the version. Calling this function
 * more than once is very fast.
 *
 * You must be connected for this function to work.
 *
 * \note
 * The f_protocol_version variable will be set even though the function
 * is a const. This is a cache so it makes sense. The variable is
 * NOT marked mutable because this is the only case where it happens.
 *
 * \return The version of the protocol.
 */
const QString &QCassandra::protocolVersion() const
{
    return f_protocol_version;
}

/** \brief Get the partitioner of the cluster.
 *
 * This function retrieves the name of the partitioner in use by the
 * cluster. Some paritioners do not support readRows() and thus knowing
 * what partitioner is in use can be useful to know which algorithm
 * your application should use.
 *
 * \note
 * Cassandra defines the RandomPartitioner by default. That partition
 * does not work well with readRows().
 *
 * \return The name of the partitioner.
 *
 * \sa readRows()
 */
const QString &QCassandra::partitioner() const
{
    return f_partitioner;
}

/** \brief Get the snitch of the cluster.
 *
 * This function retrieves the name of the snitch in use by the
 * cluster.
 *
 * \return The name of the snitch.
 *
 * \todo I have no idea how to read this using CQL!
 *
 * \sa readRows()
 */
const QString &QCassandra::snitch() const
{
    return f_snitch;
}

/** \brief Retrieve a context by name.
 *
 * This function searches for a context by name. If the context does not
 * exist yet, then it gets created in memory.
 *
 * You should be connected for this function to work as expected although
 * it is possible to create a memory only context.
 *
 * The context is not created in the Cassandra database. If it doesn't
 * exist in Cassandra, it is only created in memory until you call its
 * create() function. This gives you an opportunity to setup the context
 * including its tables before creating it.
 *
 * The following shows you an example:
 *
 * \code
 *  context = cassandra.context(context_name);
 *  context->setStrategyClass("org.apache.cassandra.locator.SimpleStrategy");
 *  context->setReplicationFactor(1);
 *  context->create();
 * \endcode
 *
 * Note that if you do not know whether the context exists, use the
 * findContext()
 * function first, then check whether the context was found.
 *
 * \param[in] context_name  The name of the context to search.
 *
 * \return A shared pointer to a cassandra context.
 */
QCassandraContext::pointer_t QCassandra::context( const QString &context_name )
{
    // get the list of existing contexts
    const QCassandraContexts &cs = contexts();

    // already exists?
    QCassandraContexts::const_iterator ci = cs.find( context_name );
    if ( ci != cs.end() )
    {
        return ci.value();
    }

    // otherwise create a new one
    QCassandraContext::pointer_t c(
        new QCassandraContext( shared_from_this(), context_name ) );
    f_contexts.insert( context_name, c );
    retrieveContext( context_name );
    return c;
}

/** \brief Make the specified context the current context.
 *
 * This function assigns the specified context as the current context
 * of the Cassandra server.
 *
 * The Cassandra servers work with keyspaces. One keyspace is similar
 * to a database. This defines what we call a context. The
 * setCurrentContext() function defines the active context.
 *
 * If the QCassandra object is not yet connected, the context is saved
 * as the current context of the QCassandra object, but it gets lost
 * once you connect.
 *
 * The only way to make a context not the current one is by setting
 * another context as the current one.
 *
 * You must be connected for this function to really work.
 *
 * \note
 * It is not required to set the current context manually. This is done
 * automatically when you use a context.
 *
 * \note
 * It is wiser to create multiple QCassandra objects than to swap contexts all
 * the time. Note that even if you do not explicitly call this function, using
 * multiple contexts from the same QCassandra object will be slower as the
 * context has to be switched each time.
 *
 * \param[in] c  The context to set as current.
 *
 * \sa currentContext()
 * \sa QCassandraContext::makeCurrent()
 */
void QCassandra::setCurrentContext( QCassandraContext::pointer_t c )
{
    // emit the change only if not the same context
    if ( f_current_context != c )
    {
        // f_private->setContext( c->contextName() );
        // we save the current context only AFTER the call to setContext()
        // in case it throws (and then the current context would be wrong)
        f_current_context = c;
    }
}

/** \brief Internal function that clears the current context as required.
 *
 * Whenever a context is being dropped, it cannot remain the current context.
 * This function is used to remove it from being current.
 *
 * \param[in] c  The context that is about to be dropped.
 */
void QCassandra::clearCurrentContextIf( const QCassandraContext &c )
{
    if ( f_current_context.get() == &c )
    {
        f_current_context.reset();
    }
}


#if 0
/** \brief Retrieve the description of all columns for each table
 *
 * \param[in,out]  cf_def  The "columnfamily" (i.e. table) information structure that we will populate from the query.
 */
void QCassandra::retrieveColumn( ColumnDef& col_def, SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::pointer_t column ) const
{
    auto fields( column->getFields() );

    col_def.__set_name( column->getName().toUtf8().data() );

    auto
    fields_iter = fields.find("index_name");    if( fields_iter != fields.end() ) col_def.__set_index_name(       fields_iter->second->string()    );
    fields_iter = fields.find("validator");     if( fields_iter != fields.end() ) col_def.__set_validation_class( fields_iter->second->string()    );
    fields_iter = fields.find("index_options"); if( fields_iter != fields.end() ) col_def.__set_index_options(    fields_iter->second->stringMap() );

    fields_iter = fields.find("index_type");
    if( fields_iter != fields.end() )
    {
        const QString index_type( fields_iter->second->variant().toString().toLower() );
        if( index_type == "keys" )
        {
            col_def.__set_index_type( IndexType::KEYS );
        }
        else if( index_type == "custom" )
        {
            col_def.__set_index_type( IndexType::CUSTOM );
        }
        else if( index_type == "composites" )
        {
            col_def.__set_index_type( IndexType::COMPOSITES );
        }
    }
}


#if 0
/** \brief Retrieve the description of all triggers for each table
 *
 * \param[in,out]  cf_def  The "columnfamily" (i.e. table) information structure that we will populate from the query.
 */
void QCassandra::retrieveTriggers( CfDef& cf_def ) const
{
    const QString query( QString("SELECT trigger_name, trigger_options "
                                 "FROM system.schema_triggers "
                                 "WHERE keyspace_name = '%1' "
                                 "AND columnfamily_name = '%2'")
                         .arg(cf_def.keyspace.c_str())
                         .arg(cf_def.name.c_str())
                         );

    QCassandraQuery the_query( f_session );
    the_query.query( query );
    the_query.start();

    std::vector<TriggerDef> trig_def_list;
    while( the_query.nextRow() )
    {
        TriggerDef trig_def;
        trig_def.__set_name    ( the_query.getStringColumn ("trigger_name").toUtf8().data() );
        trig_def.__set_options ( the_query.getMapColumn    ("trigger_options")              );
        trig_def_list.push_back( trig_def );
    }

    cf_def.__set_triggers( trig_def_list );
}
#endif


/** \brief Retrieve the description of all tables.
 *
 * \param[in,out]  ks_def  The keyspace information structure that we will populate from the query.
 */
void QCassandra::retrieveTable( CfDef& cf_def, SessionMeta::KeyspaceMeta::TableMeta::pointer_t table ) const
{
    auto fields( table->getFields() );

    cf_def.__set_name( table->getName().toUtf8().data() );

    auto
    fields_iter = fields.find("keyspace_name");               if( fields_iter != fields.end() ) cf_def.__set_keyspace(                    fields_iter->second->string()             );
    fields_iter = fields.find("type");                        if( fields_iter != fields.end() ) cf_def.__set_column_type(                 fields_iter->second->string()             );
    fields_iter = fields.find("comparator");                  if( fields_iter != fields.end() ) cf_def.__set_comparator_type(             fields_iter->second->string()             );
    fields_iter = fields.find("subcomparator");               if( fields_iter != fields.end() ) cf_def.__set_subcomparator_type(          fields_iter->second->string()             );
    fields_iter = fields.find("comment");                     if( fields_iter != fields.end() ) cf_def.__set_comment(                     fields_iter->second->string()             );
    fields_iter = fields.find("read_repair_chance");          if( fields_iter != fields.end() ) cf_def.__set_read_repair_chance(          fields_iter->second->variant().toDouble() );
    fields_iter = fields.find("gc_grace_seconds");            if( fields_iter != fields.end() ) cf_def.__set_read_repair_chance(          fields_iter->second->variant().toInt()    );
    fields_iter = fields.find("default_validator");           if( fields_iter != fields.end() ) cf_def.__set_default_validation_class(    fields_iter->second->string()             );
    fields_iter = fields.find("cf_id");                       if( fields_iter != fields.end() ) cf_def.__set_id(                          fields_iter->second->variant().toInt()    );
    fields_iter = fields.find("min_compaction_threshold");    if( fields_iter != fields.end() ) cf_def.__set_min_compaction_threshold(    fields_iter->second->variant().toInt()    );
    fields_iter = fields.find("max_compaction_threshold");    if( fields_iter != fields.end() ) cf_def.__set_max_compaction_threshold(    fields_iter->second->variant().toInt()    );
    fields_iter = fields.find("key_validator");               if( fields_iter != fields.end() ) cf_def.__set_key_validation_class(        fields_iter->second->string()             );
    fields_iter = fields.find("key_aliases");                 if( fields_iter != fields.end() ) cf_def.__set_key_alias(                   fields_iter->second->string()             );
    fields_iter = fields.find("compaction_strategy_class");   if( fields_iter != fields.end() ) cf_def.__set_compaction_strategy(         fields_iter->second->string()             );
    fields_iter = fields.find("compaction_strategy_options"); if( fields_iter != fields.end() ) cf_def.__set_compaction_strategy_options( fields_iter->second->stringMap()          );
    fields_iter = fields.find("compression_parameters");      if( fields_iter != fields.end() ) cf_def.__set_compression_options(         fields_iter->second->stringMap()          );
    fields_iter = fields.find("bloom_filter_fp_chance");      if( fields_iter != fields.end() ) cf_def.__set_bloom_filter_fp_chance(      fields_iter->second->variant().toDouble() );
    fields_iter = fields.find("caching");                     if( fields_iter != fields.end() ) cf_def.__set_caching(                     fields_iter->second->string()             );
    fields_iter = fields.find("memtable_flush_period_in_ms"); if( fields_iter != fields.end() ) cf_def.__set_memtable_flush_period_in_ms( fields_iter->second->variant().toInt()    );
    fields_iter = fields.find("default_time_to_live");        if( fields_iter != fields.end() ) cf_def.__set_default_time_to_live(        fields_iter->second->variant().toInt()    );
    fields_iter = fields.find("speculative_retry");           if( fields_iter != fields.end() ) cf_def.__set_speculative_retry(           fields_iter->second->string()             );

    std::vector<ColumnDef> col_def_list;
    for( const auto pair : table->getColumns() )
    {
        ColumnDef col_def;
        retrieveColumn( col_def, pair.second );
        col_def_list.push_back( col_def );
    }
    cf_def.__set_column_metadata( col_def_list );
}
#endif


/** \brief Retrieve the description of a keyspace.
 *
 * This function requests for the descriptions of a specific keyspace
 * (context). It is used to rebuild the list of tables after a clearCache()
 * call on a context object.
 *
 * The QCassandra object is responsible for caching the result. The result
 * should not change until we create a new table although if another process
 * on another machine changes the Cassandra cluster structure, it will not
 * be seen until the cache gets cleared.
 *
 * \param[in] context_name  The name of the context to re-describe.
 */
void QCassandra::retrieveContext( SessionMeta::KeyspaceMeta::pointer_t keyspace ) const
{
    QCassandraContext::pointer_t c( const_cast<QCassandra*>(this)->context(keyspace->getName()) );
    c->parseContextDefinition( keyspace );
}


void QCassandra::retrieveContext( const QString& context_name ) const
{
    SessionMeta::pointer_t session_meta( SessionMeta::create(f_session) );
    session_meta->loadSchema();
    const auto& keyspaces(session_meta->getKeyspaces());
    auto iter = keyspaces.find(context_name);
    if( iter != keyspaces.end() )
    {
        retrieveContext( iter->second );
    }
}


/** \brief Get the map of contexts.
 *
 * This function returns the map of contexts (keyspaces) help in this
 * Cassandra cluster.
 *
 * The cluster may include any number of contexts also it is wise to
 * limit yourself to a relatively small number of contexts since it
 * is loaded each time you connect to the database.
 *
 * Note that the function returns a reference to the internal map of
 * contexts. It is wise to stop using it if you call a function that
 * may change the map (createContext(), dropContext(), etc.)
 *
 * You must be connected for this function to work.
 *
 * \return A reference to the internal map of contexts.
 */
const QCassandraContexts &QCassandra::contexts() const
{
    if( !f_contexts_read )
    {
        SessionMeta::pointer_t session_meta( SessionMeta::create(f_session) );
        session_meta->loadSchema();

        f_contexts_read = true;

        for( auto keyspace : session_meta->getKeyspaces() )
        {
            retrieveContext( keyspace.second );
        }
    }
    return f_contexts;
}

/** \brief Search for a context.
 *
 * This function searches for a context. If it exists, its shared pointer is
 * returned. Otherwise, it returns a NULL pointer (i.e. the
 * QSharedPointer::isNull() function returns true.)
 *
 * Note that if you do create an in memory context, then this function can be
 * used to retrieve it too. In memory contexts are those that did not yet
 * exist in the database and never had their create() function called.
 *
 * \todo
 * Add a way to distinguish in memory only contexts and Cassandra contexts.
 * This is important to know whether a context can be dropped.
 *
 * \param[in] context_name  The name of the context to retrieve.
 *
 * \return A shared pointer to the context.
 *
 * \sa contexts()
 * \sa QCassandraContext::create()
 */
QCassandraContext::pointer_t QCassandra::findContext( const QString &context_name ) const
{
    QCassandraContexts::const_iterator ci( contexts().find( context_name ) );
    if ( ci == f_contexts.end() )
    {
        QCassandraContext::pointer_t null;
        return null;
    }
    return *ci;
}

/** \brief Retrieve a context reference.
 *
 * The array operator searches for a context by name and returns
 * its reference. This is useful to access data with array like
 * syntax as in:
 *
 * \code
 * cluster[context_name][table_name][column_name] = value;
 * \endcode
 *
 * \exception std::runtime_error
 * If the context doesn't exist, this function raises an exception
 * since otherwise the reference would be a NULL pointer.
 *
 * \param[in] context_name  The name of the context to retrieve.
 *
 * \return A reference to the named context.
 */
QCassandraContext &QCassandra::operator[]( const QString &context_name )
{
    QCassandraContext::pointer_t context_obj( findContext( context_name ) );
    if ( !context_obj )
    {
        throw std::runtime_error(
            "named context was not found, cannot return a reference" );
    }

    return *context_obj;
}

/** \brief Retrieve a constant context reference.
 *
 * This array operator is the same as the other one, just this one deals
 * with constant contexts. It can be used to retrieve values from the
 * Cassandra cluster you're connected to:
 *
 * \code
 * value = cluster[context_name][table_name][column_name];
 * \endcode
 *
 * \exception std::runtime_error
 * If the context doesn't exist, this function raises an exception
 * since otherwise the reference would be a NULL pointer.
 *
 * \param[in] context_name  The name of the context to retrieve.
 *
 * \return A constant reference to the named context.
 */
const QCassandraContext &QCassandra::
operator[]( const QString &context_name ) const
{
    const QCassandraContext::pointer_t context_obj(
        findContext( context_name ) );
    if ( !context_obj )
    {
        throw std::runtime_error(
            "named context was not found, cannot return a reference" );
    }

    return *context_obj;
}

/** \brief Drop a context from the database and memory.
 *
 * Use this function when you want to completely drop a context
 * from the Cassandra system and from memory. After this call
 * the context, its tables, their rows, and cells are all marked
 * as dead whether you still have shared pointers on them or not.
 * (i.e. you cannot use them anymore.)
 *
 * \warning
 * If the context does not exist in Cassandra, this function call
 * raises an exception in newer versions of the Cassandra system
 * (in version 0.8 it would just return silently.) You may want to
 * call the findContext() function first to know whether the context
 * exists before calling this function.
 *
 * \param[in] context_name  The name of the context to drop.
 *
 * \sa QCassandraContext::drop()
 */
void QCassandra::dropContext( const QString &context_name )
{
    QCassandraContext::pointer_t c( context( context_name ) );

    // first do the context drop in Cassandra
    c->drop();

    // forget about this context in the QCassandra object
    f_contexts.remove( context_name );
}

/** \brief Retrieve the current default consistency level.
 *
 * This function returns the current default consistency level used by
 * most of the server functions. You may change the default from the
 * default Cassandra value of ONE (which is good for reads.) In many cases,
 * it is recommended that you use QUORUM, or at least LOCAL QUORUM.
 *
 * Different predicate and the value object have their own consistency
 * levels. If those are set to DEFAULT, then this very value is used
 * instead.
 *
 * \return The current default consistency level.
 */
consistency_level_t QCassandra::defaultConsistencyLevel() const
{
    return f_default_consistency_level;
}

/** \brief Change the current default consistency level.
 *
 * This function changes the current default consistency level used by
 * most of the server functions. In many cases, it is recommended that
 * you use QUORUM, but it very much depends on your application and
 * node setup.
 *
 * Different predicate and the value object have their own consistency
 * levels. If those are set to DEFAULT (their default,) then this very
 * value is used instead.
 *
 * \note
 * This function does not accept the CONSISTENCY_LEVEL_DEFAULT since
 * that is not a valid Cassandra consistency level.
 *
 * \exception std::runtime_error
 * This exception is raised if the value passed to this function is not
 * a valid consistency level.
 *
 * \param[in] default_consistency_level  The new default consistency level.
 */
void QCassandra::setDefaultConsistencyLevel(
    consistency_level_t default_consistency_level )
{
    // make sure the consistency level exists
    if ( default_consistency_level != CONSISTENCY_LEVEL_ONE &&
         default_consistency_level != CONSISTENCY_LEVEL_QUORUM &&
         default_consistency_level != CONSISTENCY_LEVEL_LOCAL_QUORUM &&
         default_consistency_level != CONSISTENCY_LEVEL_EACH_QUORUM &&
         default_consistency_level != CONSISTENCY_LEVEL_ALL &&
         default_consistency_level != CONSISTENCY_LEVEL_ANY &&
         default_consistency_level != CONSISTENCY_LEVEL_TWO &&
         default_consistency_level != CONSISTENCY_LEVEL_THREE )
    {
        throw std::runtime_error( "invalid default server consistency level" );
    }

    f_default_consistency_level = default_consistency_level;
}

/** \brief Retrieve the major version number.
 *
 * This function dynamically returns the library major version.
 *
 * \return The major version number.
 */
int QCassandra::versionMajor()
{
    return QT_CASSANDRA_LIBRARY_VERSION_MAJOR;
}

/** \brief Retrieve the minor version number.
 *
 * This function dynamically returns the library minor version.
 *
 * \return The minor version number.
 */
int QCassandra::versionMinor()
{
    return QT_CASSANDRA_LIBRARY_VERSION_MINOR;
}

/** \brief Retrieve the patch version number.
 *
 * This function dynamically returns the library patch version.
 *
 * \return The patch version number.
 */
int QCassandra::versionPatch()
{
    return QT_CASSANDRA_LIBRARY_VERSION_PATCH;
}

/** \brief Retrieve the library version number in the form of a string.
 *
 * This function dynamically returns the library version as a string.
 * This string includes all 3 parts of the version number separated
 * by period (.) characters.
 *
 * \return The patch version number.
 */
const char *QCassandra::version()
{
    return QT_CASSANDRA_LIBRARY_VERSION_STRING;
}

/** \brief Get the time of day.
 *
 * This function returns the time of day in micro seconds. It is a
 * static function since there is no need for the this pointer in
 * this helper function.
 *
 * \return The time of day in microseconds.
 */
int64_t QCassandra::timeofday()
{
    struct timeval tv;

    // we ignore timezone as it can also generate an error
    gettimeofday( &tv, NULL );

    return static_cast<int64_t>( tv.tv_sec ) * 1000000 +
           static_cast<int64_t>( tv.tv_usec );
}

} // namespace QtCassandra
// vim: ts=4 sw=4 et
