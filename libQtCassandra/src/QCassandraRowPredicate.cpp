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

#include "QtCassandra/QCassandraRowPredicate.h"
//#include "QtCassandra/QCassandraTable.h"
#include "thrift-gencpp-cassandra/cassandra_types.h"
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
    : //f_start_row(), -- auto-init
      //f_end_row(), -- auto-init
      f_count(100),
      //f_wrap(false), -- auto-init
      //f_exclude(false), -- auto-init
      f_column_predicate(new QCassandraColumnPredicate)
{
}

/** \brief Retrieve a copy of the start row name.
 *
 * This function returns a copy of the start row name. If the start row
 * key was defined with binary that is not UTF-8 compatible, this function will
 * raise an exception while converting the buffer to UTF-8.
 *
 * \return The start row name.
 *
 * \sa startRowKey()
 * \sa QCassandraColumnPredicate::first_char
 * \sa QCassandraColumnPredicate::last_char
 */
QString QCassandraRowPredicate::startRowName() const
{
    return QString::fromUtf8(f_start_row.data());
}

/** \brief Set the name of the start row.
 *
 * This function defines the name of the start row to retrieve. All the
 * rows defined between the start and end row names/keys will be returned
 * by this predicate. However, rows are most often NOT sorted in your
 * database (you may have changed the settings to support such, but by
 * default they are sorted by MD5 checksums or something of the sort which
 * creates better disparity to place content on each node and not end up
 * with a few nodes doing most of the work.)
 *
 * An empty row name can be used to request the very first row to be
 * returned first. The start row is included in the result except if
 * the wrap parameter is set to true.
 *
 * \warning
 * In most cases your Cassandra rows are NOT sorted so trying to use
 * a range of names will fail badly.
 *
 * \param[in] row_name  The name of the row to start with.
 *
 * \sa setStartRowKey()
 * \sa QCassandraColumnPredicate::first_char
 * \sa QCassandraColumnPredicate::last_char
 */
void QCassandraRowPredicate::setStartRowName(const QString& row_name)
{
    setStartRowKey(row_name.toUtf8());
}

/** \brief Retrieve a copy of the start row key.
 *
 * This function returns a constant reference to the start row key.
 *
 * \return A constant reference to the start row key.
 *
 * \sa startRowName()
 * \sa QCassandraColumnPredicate::first_char
 * \sa QCassandraColumnPredicate::last_char
 */
const QByteArray& QCassandraRowPredicate::startRowKey() const
{
    return f_start_row;
}

/** \brief Set the start row key.
 *
 * This function sets the start row key of this row predicate.
 *
 * \note
 * This function has the side effect of clearing the exclude
 * flag so this very row key will be included in the next
 * readRows() unless wrap is true.
 *
 * \warning
 * In most cases your Cassandra rows are NOT sorted so trying to use
 * a range by name will fail.
 *
 * \param[in] row_key  The new start row key.
 *
 * \sa setStartRowName()
 * \sa QCassandraColumnPredicate::first_char
 * \sa QCassandraColumnPredicate::last_char
 */
void QCassandraRowPredicate::setStartRowKey(const QByteArray& row_key)
{
    if(row_key.size() > 65535) {
        throw std::runtime_error("the start row key is more than 64Kb");
    }
    f_start_row = row_key;
    f_exclude = false;
}

/** \brief Retrieve the end row name.
 *
 * This function retrieves the row key in the form of a row name.
 * The name is the UTF-8 string that you set using setRowName().
 *
 * If you used the setRowKey() and the name was not valid UTF-8, then
 * this function will throw an eror.
 *
 * \return The row string.
 *
 * \sa endRowKey()
 * \sa QCassandraColumnPredicate::first_char
 * \sa QCassandraColumnPredicate::last_char
 */
QString QCassandraRowPredicate::endRowName() const
{
    return QString::fromUtf8(f_end_row.data());
}

/** \brief Set the end row name.
 *
 * This function defines the end row key using the UTF-8 string
 * of the specified row name.
 *
 * Note that the row that matches this key is returned (i.e. the
 * boundary is inclusive.)
 *
 * \warning
 * In most cases your Cassandra rows are NOT sorted so trying to use
 * a range by name will fail.
 *
 * \param[in] row_name  The name of the end row.
 *
 * \sa setEndRowKey()
 * \sa QCassandraColumnPredicate::first_char
 * \sa QCassandraColumnPredicate::last_char
 */
void QCassandraRowPredicate::setEndRowName(const QString& row_name)
{
    setEndRowKey(row_name.toUtf8());
}

/** \brief Retrieve a copy of the end row key.
 *
 * This function returns a constant reference to the current end
 * row key.
 *
 * \return A constant reference key to the end row key.
 *
 * \sa endRowName()
 * \sa QCassandraColumnPredicate::first_char
 * \sa QCassandraColumnPredicate::last_char
 */
const QByteArray& QCassandraRowPredicate::endRowKey() const
{
    return f_end_row;
}

/** \brief Define the last row key.
 *
 * This function sets the last key you're interested in. It can safely be
 * set to an empty key to not bound the last key.
 *
 * \warning
 * In most cases your Cassandra rows are NOT sorted so trying to use
 * a range by name will fail.
 *
 * \param[in] row_key  The binary row key we stop searching.
 *
 * \sa endRowKey()
 * \sa QCassandraColumnPredicate::first_char
 * \sa QCassandraColumnPredicate::last_char
 */
void QCassandraRowPredicate::setEndRowKey(const QByteArray& row_key)
{
    if(row_key.size() > 65535) {
        throw std::runtime_error("the end row key is more than 64Kb");
    }
    f_end_row = row_key;
}

/** \brief Define the last row key that was read.
 *
 * This function changes the first/last key as required for the next
 * readRows() function call to return the following rows.
 *
 * \param[in] row_key  The binary row key we start reading on the next call.
 *
 * \sa endRowKey()
 * \sa QCassandraColumnPredicate::first_char
 * \sa QCassandraColumnPredicate::last_char
 */
void QCassandraRowPredicate::setLastKey(const QByteArray& row_key)
{
    if(row_key.size() > 65535) {
        // note that this one comes from Cassandra so it should never happen
        throw std::runtime_error("the last row key is too large (over 64Kb)");
    }
    f_start_row = row_key;
    // inclusive, so we flag this key as "exclude on next call"
    // if f_wrap is false
    f_exclude = !f_wrap;
}

/** \brief Check whether the first row should be excluded.
 *
 * This function is used by the QCassandraPrivate implementation to know
 * whether the f_start_row was already returned in a previous call. If so
 * then the first row in the next call is to be silently skipped.
 *
 * The flag is set to true by QCassandraPrivate when it calls the
 * setLastKey() function to change the f_start_row value of the predicate.
 *
 * \return true if the first result needs to be skipped.
 */
bool QCassandraRowPredicate::excludeFirst() const
{
    return f_exclude;
}

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
void QCassandraRowPredicate::setCount(int32_t val)
{
    f_count = val;
}

/** \brief Return whether the row predicate is wrapped or not.
 *
 * This function returns the current status of the wrap flag. If true
 * then it wraps which means it has slightly different semantics and
 * it gives you a way to navigate through all the rows. See the setWrap()
 * function for more info.
 *
 * \return The current status of the wrap flag.
 *
 * \sa setWrap()
 */
bool QCassandraRowPredicate::wrap() const
{
    return f_wrap;
}

/** \brief Define whether the specified keys wrap.
 *
 * By default, the specified keys are inclusive and do not wrap. This
 * means a key range from J to L will return all the rows that are
 * defined between J and L inclusive. On the other hand, the range L
 * to J will return an empty set.
 *
 * When you set the wrap flag to true, then the range J to L will return
 * all the keys except the one that match J. And the range L to J will
 * return the set of keys after L and before J, J included.
 *
 * \param[in] wrap  true to get the keys to wrap.
 *
 * \sa wrap()
 */
void QCassandraRowPredicate::setWrap(bool val)
{
    f_wrap = val;
}

/** \brief Retrieve a pointer to the column predicate.
 *
 * This function returns a shared pointer to the column predicate of
 * the row predicate. This pointer can be used to modify the existing
 * predicate.
 *
 * The best is for you to allocate your own predicate and set it using
 * the setColumnPredicate() function.
 *
 * This function is also used to retrieve the column predicate and
 * transform it in a SlicePredicate.
 *
 * \return A shared pointer to the column predicate attached to this row predicate.
 *
 * \sa setColumnPredicate()
 */
QCassandraColumnPredicate::pointer_t QCassandraRowPredicate::columnPredicate() const
{
    return f_column_predicate;
}

/** \brief Set a new column predicate.
 *
 * As the rows are being read, the Cassandra server also reads the columns.
 * Which columns should be read can be determined by the column predicate.
 * By default, the column predicate is set to "read all the columns."
 *
 * Note that we save a shared pointer to your column predicate. This means
 * you need to allocate it. You may also want to retrieve a copy of the
 * object internal column predicate with the columnPredicate() function
 * and directly modify that copy (assuming you don't want to use the
 * Name or Range specilized column predicate.)
 *
 * \param[in] column_predicate  A column predicate instance.
 *
 * \sa columnPredicate()
 */
void QCassandraRowPredicate::setColumnPredicate(QCassandraColumnPredicate::pointer_t column_predicate)
{
    f_column_predicate = column_predicate;
}

/** \brief Transform to a Thrift predicate.
 *
 * This function is used to transform a QCassandraColumnRangePredicate object to
 * a Cassandra SlicePredicate structure.
 *
 * The input parameter is set to void * because the function is defined in
 * the public header file and thus cannot directly make use of the Thrift
 * type definitions.
 *
 * \param[in] data  The pointer to the SlicePredicate to setup.
 */
void QCassandraRowPredicate::toPredicate(void *data) const
{
    org::apache::cassandra::KeyRange *key_range = reinterpret_cast<org::apache::cassandra::KeyRange *>(data);

    std::string start(f_start_row.data(), f_start_row.size());
    std::string end(f_end_row.data(), f_end_row.size());
    if(f_wrap) {
        key_range->__set_start_token(start);
        key_range->__set_end_token(end);
    }
    else {
        key_range->__set_start_key(start);
        key_range->__set_end_key(end);
    }
    key_range->__set_count(f_count + (f_exclude ? 1 : 0));
}


} // namespace QtCassandra
// vim: ts=4 sw=4 et
