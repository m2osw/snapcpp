/*
 * Text:
 *      QCassandraColumnPredicate.cpp
 *
 * Description:
 *      Definition of column predicates to get row slices.
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

#include "QtCassandra/QCassandraColumnPredicate.h"
#include <stdexcept>

namespace QtCassandra
{

/** \class QCassandraColumnPredicate
 * \brief Test to search for columns.
 *
 * The default column predicate returns all the columns defined in your
 * rows. This base class cannot be changed to constrain the list of
 * columns returned by a readCells() or readRows() call.
 *
 * You may want to consider using a QCassandraColumnRangePredicate or
 * QCassandraColumnNamePredicate instead. However, if you are very
 * likely to read all the columns anyway, it may be faster to just
 * use the QCassandraColumnPredicate and have a single transfer from
 * Cassandra.
 *
 * \note
 * I call the finish column the "end column". Somehow, it makes more
 * sense to me. Plus the row predicate uses the word "end" and not
 * the word "finish". Not only that, the one bound that changes
 * is the start row name when you set the wrap mode to true. So if
 * one bound was to have a different name, it would be the start
 * row name and not the "finish" column of the column predicate.
 */

/** \var QCassandraColumnPredicate::f_consistency_level
 * \brief The consistency level used along this predicate.
 *
 * Whenever accessing the Cassandra server to read or write data,
 * a consistency level is specified. This is the one when you
 * read a list of columns.
 */

/** \brief Define the first possible character in a column key.
 *
 * This character can be used to define the very first character
 * in a column key. Note though that it is rarely used because
 * the empty string serves the purpose and is more likely what
 * you want.
 *
 * The first character is '\0'.
 */
const QChar QCassandraColumnPredicate::first_char = QChar('\0');

/** \brief Define the last possible character in a column key.
 *
 * This character can be used to define the very last character
 * in a column key.
 *
 * The last character is '\\uFFFD'.
 *
 * \note
 * This character can also be used in row predicates.
 */
const QChar QCassandraColumnPredicate::last_char = QChar(L'\uFFFD');

/** \brief Initializes the base class of the column predicate.
 *
 * When you create a QCassandraColumnPredicate object, you tell the
 * system to return all the columns of the table row you are querying.
 * To only query a given set of columns, use the
 * QCassandraColumnNamePredicate (i.e. specific set of columns) or
 * the QCassandraColumnRangePredicate (i.e. all the columns between
 * the start and end boundaries.)
 *
 * The information defined in the base are available to both the
 * column name predicate and the column range predicate.
 */
QCassandraColumnPredicate::QCassandraColumnPredicate()
    //: f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
{
}

/** \brief Clean up a column predicate object.
 *
 * This function currently does nothing.
 */
QCassandraColumnPredicate::~QCassandraColumnPredicate()
{
}

#if 0
/** \brief Transform to a Thrift predicate.
 *
 * This function is used to transform a QCassandraColumnPredicate object to
 * a Cassandra SlicePredicate structure.
 *
 * The input parameter is set to void * because the function is defined in
 * the public header file and thus cannot directly make use of the Thrift
 * type definitions.
 *
 * \param[in] data  The pointer to the SlicePredicate to setup.
 */
void QCassandraColumnPredicate::toPredicate(void *data) const
{
    org::apache::cassandra::SlicePredicate *slice_predicate = reinterpret_cast<org::apache::cassandra::SlicePredicate *>(data);

    // using empty strings for the start & end makes the slice range
    // return all the columns; although this way we're not offering
    // a limit and the reversed flag...
    slice_predicate->__isset.slice_range = true;
}
#endif









/** \class QCassandraColumnNamePredicate
 * \brief A column predicate using a list of column names.
 *
 * This class defines a list of binary column keys that will
 * be returned when reading a list of columns in a row.
 */

/** \typedef QCassandraColumnNamePredicate::QCassandraColumnKeys
 * \brief A map of column keys.
 *
 * This array is used to save the column keys.
 */

/** \var QCassandraColumnNamePredicate::f_column_keys
 * \brief The named columns to return.
 *
 * This parameter is a list of binary column keys. The number of
 * column names is not limited although you may want to limit
 * the number of columns read at once and use a slice instead.
 */

/** \brief Initializes a column name predicate.
 *
 * This function initializes a column name predicate.
 *
 * By default, all the columns of a table are returned. If you add column
 * names, then only those columns are returned. This predicate is used
 * to specify the exact column names you want to retrieve.
 */
QCassandraColumnNamePredicate::QCassandraColumnNamePredicate()
    //: QCassandraColumnPredicate() -- auto-init
    //  f_column_keys() -- auto-init
{
}

/** \brief This function can be used to clear all the column names.
 *
 * This function can be used to clear all the column names you added to this
 * column name predicate. This way you can reuse the same column name
 * predicate for multiple queries.
 */
void QCassandraColumnNamePredicate::clearColumns()
{
    f_column_keys.clear();
}

/** \brief Add a column name.
 *
 * This function adds a column name to the list of columns to return when
 * using this predicate.
 *
 * This predicate is used to define the exact column names to retrieve. It
 * is particularly useful to read a set of columns in a row.
 *
 * \param[in] column_name  Add a column name to the predicate.
 */
void QCassandraColumnNamePredicate::addColumnName(const QString& column_name)
{
    addColumnKey(column_name.toUtf8());
}

/** \brief Add a column key.
 *
 * This function adds a binary column key to the list of columns to return when
 * using this predicate.
 *
 * This predicate is used to define the exact binary column keys to retrieve. It
 * is particularly useful to read a set of columns in a row.
 *
 * \param[in] column_key  Add a column key to the predicate.
 */
void QCassandraColumnNamePredicate::addColumnKey(const QByteArray& column_key)
{
    if(column_key.size() > 65535) {
        throw std::runtime_error("the column key is more than 64Kb");
    }
    f_column_keys.append(column_key);
}

/** \brief Retrieve the complete array of column names.
 *
 * This function returns a constant reference to the column keys saved
 * in this predicate.
 *
 * Note that if you call one of the add functions or the clear function, the
 * list may become undefined.
 *
 * \return A constant reference to the array of column keys.
 */
const QCassandraColumnNamePredicate::QCassandraColumnKeys& QCassandraColumnNamePredicate::columnKeys() const
{
    return f_column_keys;
}

#if 0
/** \brief Transform to a Thrift predicate.
 *
 * This function is used to transform a QCassandraColumnNamePredicate object to
 * a Cassandra SlicePredicate structure.
 *
 * The input parameter is set to void * because the function is defined in
 * the public header file and thus cannot directly make use of the Thrift
 * type definitions.
 *
 * \param[in] data  The pointer to the SlicePredicate to setup.
 */
void QCassandraColumnNamePredicate::toPredicate(void *data) const
{
    org::apache::cassandra::SlicePredicate *slice_predicate = reinterpret_cast<org::apache::cassandra::SlicePredicate *>(data);

    slice_predicate->column_names.clear();
    for(QCassandraColumnKeys::const_iterator
            it = f_column_keys.begin();
            it != f_column_keys.end();
            ++it)
    {
        std::string key(it->data(), it->size());
        slice_predicate->column_names.push_back(key);
    }
    slice_predicate->__isset.column_names = true;
    slice_predicate->__isset.slice_range = false;
}
#endif








/** \class QCassandraColumnRangePredicate
 * \brief Define a range of columns.
 *
 * This function constrains the set of columns to return using
 * a lower and higher bounds. The bounds are inclusive (if they
 * match a column name, they will be returned.
 *
 * It is expected that the lower bound be smaller than the higher
 * bound.
 *
 * To avoid problems with transfering too much data, the column
 * range is also bound by a counter. If more cells are found in
 * the row, then only so many will be returned.
 *
 * It is also possible to retrieve the columns in reverse order.
 * This is really only useful if you have a large number of columns
 * since once returned the columns are saved in a map which means
 * it is sorted in some arbitrary way. You may need to clear the
 * row cache to be able to read more data if the set is really
 * large or some of the cells contain really large buffers (i.e.
 * large images.)
 *
 * Remember that once you executed and made use of data
 * with large cells, it is wise to clear the cache in the row,
 * the table, or the context.
 *
 * \sa setCount()
 * \sa QCassandraRow::clearCache()
 * \sa QCassandraCell::clearCache()
 */

/** \var QCassandraColumnRangePredicate::f_start_column
 * \brief The first column to return.
 *
 * This parameter defines the binary key of the first column. The
 * key is inclusive so if a column matches this key, it is also
 * returned.
 */

/** \var QCassandraColumnRangePredicate::f_end_column
 * \brief The last column to return.
 *
 * This parameter defines the binary key of the last column. The
 * key is inclusive so if a column matches this key, it is also
 * returned.
 */

/** \var QCassandraColumnRangePredicate::f_reversed
 * \brief Whether the list of columns is read in reversed order.
 *
 * This parameter can be used to search the column from the last to
 * the first instead of the first to the last. However, remember that
 * cells are returned in a map and that map is sorted in binary order
 * and thus you lose the sorting from the database columns. This will
 * be fixed at a later time to offer a vector instead so the sort order
 * from the Cassandra cluster can be preserved.
 */

/** \var QCassandraColumnRangePredicate::f_index
 * \brief Whether the predicate is used to read an index or not.
 *
 * This flag is set to false by default, meaning that you are not
 * reading the columns as a set of index entries.
 *
 * When set to true (see setIndex() for additional information)
 * then the readCells() of the row object you are managing will
 * return sets of columns going through your entire set of
 * columns as defined by your range.
 *
 * \sa setIndex()
 * \sa readCellts()
 */

/** \var QCassandraColumnRangePredicate::f_exclude
 * \brief Whether the next readCells() should exclude the first result.
 *
 * This flag is set to false by default, it changes to true whenever the
 * private class finds the last item. It gets reset back to false only
 * when the user change the start or end column names.
 *
 * \sa setIndex()
 * \sa readCellts()
 */

/** \var QCassandraColumnRangePredicate::f_count
 * \brief The maximum number of columns to retrieve.
 *
 * This parameter defines the maximum number of columns that will be
 * read using this predicate.
 *
 * The number is ignored if the read is made using column names
 * (since in that case the limit is defined by the number of
 * column names you defined.)
 */

/** \brief Initializes a column range predicate.
 *
 * This function initializes a column range predicate.
 *
 * By default, all the columns of a table are returned (limited by the
 * count parameter.) If you add a start column name and an end column
 * name, then only the columns defined between those (boundaries included)
 * will be returned.
 *
 * The constructor sets the reversed status to false (i.e. you get columns
 * from the first to the last,) and sets the number of columns to return
 * to 100. However, remember that cells are returned in a map and that map
 * is sorted in binary order on the key of each cell and thus you lose the
 * sorting from the database columns. This will be fixed at a later time to
 * offer a vector instead so the sort order from the Cassandra cluster can
 * be preserved.
 *
 * This predicate is expected to be used to read one well defined set of
 * columns. If you need to read more than 'count' columns then you may
 * want to use the index mode instead, which is off by default. To turn
 * it on call the setIndex() function. This requests for the system
 * to automatically update the start column name with the last column red
 * from the database. It will also automatically skip that column on the
 * second and further reads. This works the same way as the row predicate,
 * only by default this feature is turned off because we assume that the
 * range you define is a read-only object (i.e. if you are to use the
 * same range over multiple rows, then having the start and/or end
 * change on you may not be practical!) This means you'll want to
 * reset the start column after each read is complete.
 *
 * In order to make things go faster, don't hesitate to read a large
 * number of columns at once. If you expect to have 1 million rows and
 * each is relatively small (i.e. up to about 64 bytes) then you can
 * as well read 10,000 columns at once (640Kb of data in one packet
 * is not that bad, obviously it very much depends on what you need
 * that data for and whether the ring is built on a local environment
 * or nodes connected over long distances.)
 *
 * \sa setCount()
 */
QCassandraColumnRangePredicate::QCassandraColumnRangePredicate()
    //: QCassandraColumnPredicate(), -- auto-init
    //  f_start_column(), -- auto-init
    //  f_end_column(), -- auto-init
    //  f_reversed(false), -- auto-init
    //  f_index(false), -- auto-init
    //  f_exclude(false), -- auto-init
    //  f_count(100) -- auto-init
{
}

/** \brief Retrieve a copy of the start column name.
 *
 * This function returns a copy of the start column name. If the start column
 * key was defined with binary that is not UTF-8 compatible, this function will
 * raise an exception while converting the buffer to UTF-8.
 *
 * \return The start column name.
 */
QString QCassandraColumnRangePredicate::startColumnName() const
{
    return QString::fromUtf8(f_start_column.data());
}

/** \brief Set the name of the start column.
 *
 * This function defines the name of the start column to retrieve. All the
 * columns defined between the start and end column names/keys inclusive
 * are returned by this predicate.
 *
 * \warning
 * If the start column name is larger than the end column name then Cassandra
 * throws an error.
 *
 * \param[in] column_name  The name of the column to start with.
 */
void QCassandraColumnRangePredicate::setStartColumnName(const QString& column_name)
{
    setStartColumnKey(column_name.toUtf8());
}

/** \brief Retrieve a copy of the start column key.
 *
 * This function returns a constant reference to the start column key.
 *
 * \return A constant reference to the start column key.
 */
const QByteArray& QCassandraColumnRangePredicate::startColumnKey() const
{
    return f_start_column;
}

/** \brief Set the start column key.
 *
 * This function sets the start column key of this row predicate.
 *
 * \warning
 * If the start column name is larger than the end column name then Cassandra
 * throws an error.
 *
 * \param[in] column_key  The new start column key.
 */
void QCassandraColumnRangePredicate::setStartColumnKey(const QByteArray& column_key)
{
    if(column_key.size() > 65535) {
        throw std::runtime_error("the start column key is more than 64Kb");
    }
    f_start_column = column_key;
    f_exclude = false;
}

/** \brief Retrieve the end column name.
 *
 * This function retrieves the column key in the form of a column name.
 * The name is the UTF-8 string that you set using setColumnName().
 *
 * If you used the setColumnKey() and the name was not valid UTF-8, then
 * this function will throw an eror.
 *
 * \return The column string.
 */
QString QCassandraColumnRangePredicate::endColumnName() const
{
    return QString::fromUtf8(f_end_column.data());
}

/** \brief Set the end column name.
 *
 * This function defines the end column key using the UTF-8 string
 * of the specified column name.
 *
 * \warning
 * If the start column name is larger than the end column name then Cassandra
 * throws an error.
 *
 * \param[in] column_name  The name of the end column.
 */
void QCassandraColumnRangePredicate::setEndColumnName(const QString& column_name)
{
    setEndColumnKey(column_name.toUtf8());
}

/** \brief Retrieve a copy of the end column key.
 *
 * This function returns a constant reference to the current end
 * column key.
 *
 * \return A constant reference key to the end column key.
 */
const QByteArray& QCassandraColumnRangePredicate::endColumnKey() const
{
    return f_end_column;
}

/** \brief Define the last row key.
 *
 * This function sets the last key you're interested in. It can safely be
 * set to an empty key to not bound the last key.
 *
 * \warning
 * If the start column name is larger than the end column name then Cassandra
 * throws an error.
 *
 * \param[in] column_key  The binary column key we stop searching.
 *
 * \sa endColumnKey()
 */
void QCassandraColumnRangePredicate::setEndColumnKey(const QByteArray& column_key)
{
    if(column_key.size() > 65535) {
        throw std::runtime_error("the end column key is more than 64Kb");
    }
    f_end_column = column_key;
}

/** \brief Check whether the list of rows is returned from the start or the end.
 *
 * This function retrieves the current state of the reversed flag. By default
 * the flag is false which means rows are listed from the start to the end.
 *
 * It can be changed using the setReversed() function. However, remember that
 * cells are returned in a map and that map is sorted in binary order
 * and thus you lose the sorting from the database columns. This will
 * be fixed at a later time to offer a vector instead so the sort order
 * from the Cassandra cluster can be preserved.
 *
 * To read the map in reverse order, you can use code as follow:
 *
 * \code
 * QMapIterator<QByteArray, QtCassandra::QCassandraCell::pointer_t> c(cells);
 * c.toBack();
 * while(c.hasPrevious())
 * {
 *     c.previous();
 *     qDebug() << c.key() << c.value();
 * }
 * \endcode
 *
 * \return Whether the list of rows is returned in reverse order or not.
 *
 * \sa setReversed()
 */
bool QCassandraColumnRangePredicate::reversed() const
{
    return f_reversed;
}


/** \brief Defines whether the list of rows should start from the end.
 *
 * This function defines whether the list of rows should start from
 * the end of the table. By default, it starts from the beginning.
 *
 * Remember that cells are returned in a map and that map is sorted in
 * binary order and thus you lose the sorting from the database columns.
 * This will be fixed at a later time to offer a vector instead so the
 * sort order from the Cassandra cluster can be preserved.
 *
 * To read the map in reverse order, you can use code as follow:
 *
 * \code
 * QMapIterator<QByteArray, QtCassandra::QCassandraCell::pointer_t> c(cells);
 * c.toBack();
 * while(c.hasPrevious())
 * {
 *     c.previous();
 *     qDebug() << c.key() << c.value();
 * }
 * \endcode

 * \param[in] reversed  Whether it should be normal or reversed.
 *
 * \sa reversed()
 */
void QCassandraColumnRangePredicate::setReversed(bool val)
{
    f_reversed = val;
}


/** \brief Return the maximum number of cells that will be returned.
 *
 * This function retrieves the maximum number of cells that a column
 * slice request will return. By default it is set to 100.
 *
 * \return The maximum number of cells to be returned by requests.
 *
 * \sa setCount()
 */
int32_t QCassandraColumnRangePredicate::count() const
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
void QCassandraColumnRangePredicate::setCount(int32_t val)
{
    f_count = val;
}

/** \brief Retrieve the current status of the index flag.
 *
 * This function returns the status of the index flag as defined
 * by setIndex(). By default the index is false.
 *
 * \return true if the predicate was changed to index mode.
 */
bool QCassandraColumnRangePredicate::index() const
{
    return f_index;
}

/** \brief Transform the column range predicate to an index.
 *
 * This function transforms the column range predicate to an index
 * instead of just a pre-defined range of columns. This allows you
 * to read any number of columns between the start and end columns.
 * In most cases you use such to read an index defined using a large
 * number of columns in a specific row.
 *
 * The number of columns is limited to 2 billion so you can
 * generally make use of a single row to define your entire
 * index.
 *
 * The following is the expected usage of the column predicate
 * to read all the columns that match this range:
 *
 * \code
 *      // define the row
 *      ...
 *      QtCassandra::QCassandraRow::pointer_t row(table->row(key));
 *      ...
 *      // read all the columns with a name that start
 *      // with "links", 10000 at a time
 *      QCassandraColumnPredicate column_predicate;
 *      column_predicate.setStartColumnName("links");
 *      column_predicate.setEndColumnName("linkt");
 *      column_predicate.setCount(10000);
 *      column_predicate.setIndex();
 *      for(;;) {
 *          row->clearCache();
 *          row->readCells(column_predicate);
 *          const QCassandraCells& cells(row->cells());
 *          if(cells.isEmpty()) {
 *              // all the columns were read
 *              break;
 *          }
 *          // process the last 10,000 cells...
 *          ...
 *      }
 * \endcode
 *
 * \warning
 * When you set the index to true the start column is changed
 * under your feet. This is done so we can read the next batch
 * the next time you call readCells(). Remember to reset
 * the start column each time you want to read a new row or
 * you are likely to get an empty set on the second and following
 * rows, for sure not the expected result.
 *
 * \param[in] new_index  The new value of the index flag.
 *
 * \sa readCells()
 */
void QCassandraColumnRangePredicate::setIndex(bool new_index)
{
    f_index = new_index;
}


#if 0
/** \brief Transform to a Thrift predicate.
 *
 * This function is used to transform a QCassandraColumnRangePredicate object to
 * a Cassandra SlicePredicate structure.
 *
 * The input parameter is set to void * because the function is defined in
 * the public header file and thus cannot directly make use of the Thrift
 * type definitions.
 *
 * \note
 * If the f_exclude flag is true the function request the caller to read one
 * more column than what the user expects. That's because the first column
 * read will be ignored, so that's transprent to the caller. This being said,
 * if the cell is really big, the transparency will visibly make a difference
 * in time. At some point I hope to find a better way and save the next start
 * cell as the last read cell + 1. But that depends on the cell type!
 *
 * \param[in] data  The pointer to the SlicePredicate to setup.
 */
void QCassandraColumnRangePredicate::toPredicate(void *data) const
{
    org::apache::cassandra::SlicePredicate *slice_predicate = reinterpret_cast<org::apache::cassandra::SlicePredicate *>(data);

    std::string start(f_start_column.data(), f_start_column.size());
    std::string end(f_end_column.data(), f_end_column.size());
    slice_predicate->slice_range.__set_start(start);
    slice_predicate->slice_range.__set_finish(end);
    slice_predicate->slice_range.__set_reversed(f_reversed);
    slice_predicate->slice_range.__set_count(f_count + (f_exclude ? 1 : 0));
    slice_predicate->__isset.slice_range = true;
    slice_predicate->__isset.column_names = false;
}
#endif

/** \brief Set the last key found in a getColumnSlice() call.
 *
 * This function is called internally by the private implementation
 * to determine the last column read. It is used on the next iteration
 * to avoid returning the same column twice and to start reading the
 * next batch of columns.
 *
 * \param[in] column_key  The key of the column to read.
 */
void QCassandraColumnRangePredicate::setLastKey(const QByteArray& column_key)
{
    if(column_key.size() > 65535) {
        // note that this one comes from Cassandra so it should never happen
        throw std::runtime_error("the last column key is more than 64Kb");
    }
    f_start_column = column_key;
    // the read is inclusive by default, so we flag this key as
    // "exclude on next call"; we don't have to check the index
    // flag here as it is by the getColumnSlice() function
    f_exclude = true;
}

/** \brief Check whether the very first column should be excluded.
 *
 * This flag is set to true whenever the setLastKey() function is called
 * and it signals the QCassandraPrivate::getColumnSlice() function that
 * the first column read on that turn was already returned last time
 * the function was called so this time we want to skip it.
 *
 * The flag is automatically reset when the predicate user sets the
 * start or end column names of this predicate. Note that this flag
 * has no effect if the predicate is not marked as an index with a
 * call to the setIndex() function.
 *
 * \return false if the first column read should not be excluded, true otherwise.
 */
bool QCassandraColumnRangePredicate::excludeFirst() const
{
    return f_exclude;
}

} // namespace QtCassandra
// vim: ts=4 sw=4 et
