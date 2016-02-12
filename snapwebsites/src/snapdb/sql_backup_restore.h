/*
 * Text:
 *      snap_table_list.cpp
 *
 * Description:
 *      Reads and describes a Snap database. This ease checking out the
 *      current content of the database as the cassandra-cli tends to
 *      show everything in hexadecimal number which is quite unpractical.
 *      Now we do it that way for runtime speed which is much more important
 *      than readability by humans, but we still want to see the data in an
 *      easy practical way which this tool offers.
 *
 * License:
 *      Copyright (c) 2012-2016 Made to Order Software Corp.
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

// 3rd party libs
//
#include <QtCore>
#include <QtCassandra/QCassandra.h>

class sqlBackupRestore
{
public:
    sqlBackupRestore( const QtCassandra::QCassandra::pointer_t context
                    , const QString& context_name, const QString& sqlDbFile
                    );

    void storeContext();
    void restoreContext();
    
private:
    typedef QMap<QString,QVariant>                         string_to_id_t;
    typedef QList<QtCassandra::QCassandraTable::pointer_t> table_list_t;
    typedef QList<QtCassandra::QCassandraRow::pointer_t>   row_list_t;

    int  getTableId( QtCassandra::QCassandraTable::pointer_t table );
    int  getRowId  ( QtCassandra::QCassandraRow::pointer_t   row   );

    void createSchema( const QString& sqlDbFile );
    void writeContext();
    void storeTables();
    void storeRowsByTable( QtCassandra::QCassandraTable::pointer_t table );
    void storeCellsByRow ( QtCassandra::QCassandraRow::pointer_t row );

    QtCassandra::QCassandra::pointer_t        f_cassandra;
    QtCassandra::QCassandraContext::pointer_t f_context;
    table_list_t                              f_tableList;
    row_list_t                                f_rowList;
    //
    QtCassandra::QCassandraColumnRangePredicate::pointer_t  f_colPred;
    QtCassandra::QCassandraRowPredicate                     f_rowPred;
};

// vim: ts=4 sw=4 et
