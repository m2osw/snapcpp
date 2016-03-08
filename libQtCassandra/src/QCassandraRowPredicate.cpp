/*
 * Text:
 *      QCassandraRowPredicate.cpp
 *
 * Description:
 *      Definition of row (and column) predicates to get row ranges.
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

#include "QtCassandra/QCassandraRowPredicate.h"
#include <stdexcept>

namespace QtCassandra
{

/** \class QCassandraRowPredicate
 * \brief The row predicate to constrain the number of rows to return.
 *
 * This class defines a row constraint with lower and upper bounds.
 * The QCassandraRowPredicate also includes a QCassandraColumnPredicate
 * to constrain the columns returned by the QCassandraTable::readRows()
 * function.
 *
 * The row predicate uses a start and end row key to constrain the
 * search. However, these are probably not going to work as you expect
 * if you use the RandomPartitioner to sort your rows (and that's not
 * only the default in Cassandra, but you cannot change it after you
 * created your Cassandra cluster!)
 *
 * \note
 * Because the order of the rows in Cassandra may be completely different
 * than the order in which libQtCassandra saves the rows, the last row in
 * the QCassandraTable is not, in most cases, the last row read from
 * Cassandra. The last row read is saved in your row predicate which is
 * an [in,out] parameter to the readRows() function.
 *
 * The row predicate also includes a limit to the number of rows that can
 * be returned at once (it is called count.)
 *
 * In order to read all the rows of a table one wants to:
 *
 * \li Set the start row to the null value (i.e. an empty string)
 *
 * \li Set the end row to the null value (i.e. an empty string)
 *
 * \li Set the wrap mode to true (if your partitioner is RandomPartitioner,
 * I do not recommand using the wrap mode) so the start row is exclusive
 * and the end row is inclusive. The wrapping in itself is not used in this
 * case but the exclusive behavior of the first row is important. Note that
 * since libQtCassandra 0.4.2 we actually achieve the same behavior without
 * the wrap mode
 *
 * \li Set the limit parameter to the number of rows you want to read at
 * once (i.e. limit it to whatever makes sense in your application.)
 * Note that in the wrap mode is false, then we read 1 more row per call
 * because the last row read in the previous call is re-read in the next
 * call. However, it is not returned to you so you don't need to worry.
 *
 * \li After the first read returned and if the number of rows returned
 * was equal to the limit, then call the readRows() function again. The
 * QCassandraRowPredicate was updated in order to read the next count
 * items. Note that rows are cumulated on each readRows(). If you already
 * worked on the previous rows, you probably want to deleted them from
 * memory using the table->clearCache() function before the next readRows()
 * call.
 *
 * Note that the last readRows() is expected to return 0 as the number of
 * rows read.
 *
 * \warning
 * The setWrap() call is not overly compatible with a RandomPartitioner. You
 * can check your cluster by asking for the description in the Cassandra
 * CLI: describe cluster. If Partitioner says
 * org.apache.cassandra.dht.RandomPartitioner then setWrap() should probably
 * not be used. At this point though the library let you do it and if it works
 * for you, all the better.
 *
 * \code
 * QCassandraRowPredicate rowp;
 * rowp.setStartRowName("");
 * rowp.setEndRowName("");
 * rowp.setCount(100); // 100 is the default
 * rowp.setWrap(); // change to wrapping mode (not required)
 * for(;;) {
 *   // we need to clear the cache or we cannot distinguish between
 *   // existing and new rows... (we may want to return a list of the
 *   // matching rows in the readRows() function.)
 *   table->clearCache();
 *   int c = table->readRows(rowp);
 *   if(c == 0) {
 *     break;
 *   }
 *   // handle the result
 *   QCassandraRows const& r(table.rows());
 *   ...
 *   if(c < 100) { // your setLimit() parameter
 *     // if smaller than your limit you're done!
 *     break;
 *   }
 *   // Since version 0.4.2 this change is done internally
 *   // because rows in 'table' are sorted differently
 *   // than rows in Cassandra.
 *   //rowp.setStartRowName(last_row.rowName());
 * }
 * \endcode
 */

/** \var QCassandraRowPredicate::f_start_row
 * \brief The key of the start row to read from the Cassandra server.
 *
 * This value defines the first row that will be returned. The
 * search is inclusive if the wrap parameter is false. It is
 * exclusive when wrap is true.
 *
 * The first search can start with an empty key.
 */

/** \var QCassandraRowPredicate::f_end_row
 * \brief The key of the last row to read from the Cassandra server.
 *
 * This value defines the end row that will be returned. The
 * search is always inclusive of the end_row parameter.
 */

/** \var QCassandraRowPredicate::f_count
 * \brief The number of rows you want to find.
 *
 * The search of rows can be limited by this count value. If
 * you want to search all the rows with multiple searches,
 * make sure to set f_wrap to true (setWrap()) and copy the
 * row key of the last row found to the start bound.
 */

/** \var QCassandraRowPredicate::f_wrap
 * \brief Whether the row search should wrap.
 *
 * Whether the row search should wrap.
 *
 * When the wrap flag is true, the start bound is exclusive. In
 * all other cases the bounds are inclusive. Also, if the end
 * bound is larger than the start bound, the predicate returns
 * nothing unless the wrap flag is set to true.
 */

/** \var QCassandraRowPredicate::f_exclude
 * \brief Whether the f_start_key should be excluded from results.
 *
 * Whenever you read many rows using the readRows() function,
 * the only safe way to make sure you read all the rows is to
 * reuse the last row you read as the first to read next. Yet,
 * in most cases users do not want to get the same data twice.
 * This flag tells the readRow() function to ignore the first
 * result whenever this flag is true.
 *
 * Note also that when the flag is true the get_slice_rows()
 * function makes use of f_count + 1.
 */

/** \var QCassandraRowPredicate::f_column_predicate
 * \brief A copy of the column predicate.
 *
 * This is a column predicate to define the list of columns that
 * you want to retrieve from a readRows() call.
 *
 * \sa setColumnPredicate()
 */

/** \brief Initializes a row range predicate.
 *
 * This function initializes a row range predicate.
 *
 * By default, all the rows of a table are returned (limited to the
 * count parameter.) If you add a start row name and a end row
 * name, then only the rows defined between those (boundaries included
 * unless you set wrap to true) will be returned.
 *
 * \warning
 * The default is to read ALL the rows and ALL the columns of a table.
 * This may mean a lot of data. 100 rows x 100 columns with each
 * column using an average of 1,000 bytes is 10Mb to transmit over the
 * wire. Think about it twice because if you make use of a single column
 * and that were to be an integer (i.e. 4 bytes) you'd instead read 400
 * bytes.
 *
 * The constructor sets the number of rows to return to 100 by default.
 */
QCassandraRowPredicate::QCassandraRowPredicate()
    : f_count(100)
{
}


// All of these are stubs:
//
QString			  QCassandraRowPredicate::startRowName() const  { return QString(); }
void 			  QCassandraRowPredicate::setStartRowName(const QString& /*row_name*/) {}
const QByteArray& QCassandraRowPredicate::startRowKey() const   { return QString(); }
void 		      QCassandraRowPredicate::setStartRowKey(const QByteArray& /*row_key*/) {}

QString			  QCassandraRowPredicate::endRowName() const { return QString(); }
void			  QCassandraRowPredicate::setEndRowName(const QString& /*row_name*/) {}
const QByteArray& QCassandraRowPredicate::endRowKey() const { return QString(); }
void			  QCassandraRowPredicate::setEndRowKey(const QByteArray& /*row_key*/) {}

QCassandraColumnPredicate::pointer_t columnPredicate() const { return f_column_predicate; }
void                                 setColumnPredicate(QCassandraColumnPredicate::pointer_t column_predicate) { f_column_predicate = column_predicate; }



/** \brief Return the maximum number of rows that will be returned.
 *
 * This function retrieves the maximum number of rows that a row slice
 * request will return. By default it is set to 100.
 *
 * \return The maximum number of rows to be returned by requests.
 *
 * \sa setCount()
 */
int32_t QCassandraRowPredicate::count() const
{
    return f_count;
}


/** \brief Change the number of rows to return.
 *
 * This function defines the number of rows a table request will
 * return when querying for a slice.
 *
 * The default is 100.
 *
 * Keep in mind that the entire set of rows will be returned in a
 * single message, thus returning a very large number can fill up
 * your memory quickly.
 *
 * \param[in] count  The new number of rows to return when querying for a slice.
 *
 * \sa count()
 */
void QCassandraRowPredicate::setCount( const int32_t val )
{
    f_count = val;
}


} // namespace QtCassandra
// vim: ts=4 sw=4 et
