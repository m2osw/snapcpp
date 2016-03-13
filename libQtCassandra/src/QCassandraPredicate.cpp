/*
 * Header:
 *      QCassandraPredicate.cpp
 *
 * Description:
 *      Handling of CQL query string manipulation.
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

#include "QtCassandra/QCassandraPredicate.h"

namespace QtCassandra
{


/// \brief Cell predicate query handlers
void QCassandraCellKeyPredicate::appendQuery( QString& query, int& bind_count )
{
    query += "AND column1 == ?";
    bind_count++;
}

void QCassandraCellKeyPredicate::bindQuery( statement_pointer_t query_stmt, int& bind_num )
{
    cass_statement_bind_string_n( query_stmt.get(), bind_num++, f_cellKey.constData(),  f_cellKey.size() );
}


/// \brief Cell range predicate query handlers
void QCassandraCellRangePredicate::appendQuery( QString& query, int& bind_count )
{
    query += "AND column1 >= ? AND column1 <= ?";
    bind_count += 2;
}

void QCassandraCellRangePredicate::bindQuery( statement_pointer_t query_stmt, int& bind_num )
{
    cass_statement_bind_string_n( query_stmt.get(), bind_num++, f_startCellKey.constData(),  f_startCellKey.size() );
    cass_statement_bind_string_n( query_stmt.get(), bind_num++, f_endCellKey.constData(),    f_endCellKey.size()   );
}


/// \brief Row key predicate query handlers
void QCassandraRowKeyPredicate::appendQuery( QString& query, int& bind_count )
{
    query += "WHERE key == ?";
    bind_count++;
    f_cellPred->appendQuery( query, bind_count );
}

void QCassandraRowKeyPredicate::bindQuery( statement_pointer_t query_stmt, int& bind_num )
{
    cass_statement_bind_string_n( query_stmt.get(), bind_num++, f_rowKey.constData(),  f_rowKey.size() );
    f_cellPred->bindQuery( query_stmt, bind_num );
}


/// \brief Row range predicate query handlers
void QCassandraRowRangePredicate::appendQuery( QString& query, int& bind_count )
{
    query += "WHERE token(key) >= token(?) AND token(key) <= token(?)";
    bind_count += 2;
    f_cellPred->appendQuery( query, bind_count );
}

void QCassandraRowRangePredicate::bindQuery( statement_pointer_t query_stmt, int& bind_num )
{
    cass_statement_bind_string_n( query_stmt.get(), bind_num++, f_startRowKey.constData(),  f_startRowKey.size() );
    cass_statement_bind_string_n( query_stmt.get(), bind_num++, f_endRowKey.constData(),    f_endRowKey.size()   );
    f_cellPred->bindQuery( query_stmt, bind_num );
}


} // namespace QtCassandra

// vim: ts=4 sw=4 et
