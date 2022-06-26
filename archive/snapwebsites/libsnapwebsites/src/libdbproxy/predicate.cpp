/*
 * Text:
 *      libsnapwebsites/src/libdbproxy/predicate.cpp
 *
 * Description:
 *      Handling of CQL query string manipulation.
 *
 * Documentation:
 *      See the corresponding .cpp file.
 *
 * License:
 *      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
 * 
 *      https://snapwebsites.org/
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

#include "libdbproxy/predicate.h"

#include <iostream>

namespace libdbproxy
{

/** \brief Define the first possible character in a column key.
 *
 * This character can be used to define the very first character
 * in a column key. Note though that it is rarely used because
 * the empty string serves the purpose and is more likely what
 * you want.
 *
 * The first character is '\0'.
 */
const QChar cell_predicate::first_char = QChar('\0');


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
const QChar cell_predicate::last_char = QChar(static_cast<uint>(L'\uFFFD'));


/// \brief Cell predicate query handlers
void cell_key_predicate::appendQuery( QString& query, int& bind_count )
{
    query += " AND column1 == ?";
    bind_count++;
}


void cell_key_predicate::bindOrder( order& order )
{
    order.addParameter( f_cellKey );
}


/// \brief Cell range predicate query handlers
void cell_range_predicate::appendQuery( QString& query, int& bind_count )
{
    if(!f_startCellKey.isEmpty())
    {
        query += " AND column1>=?";
        bind_count += 1;
    }

    if(!f_endCellKey.isEmpty())
    {
        // The end boundary is NEVER included in the results
        query += " AND column1<?";
        bind_count += 1;
    }

    if(f_reversed)
    {
        query += " ORDER BY column1 DESC";
    }
}


void cell_range_predicate::bindOrder( order& order )
{
    if(!f_startCellKey.isEmpty())
    {
        order.addParameter( f_startCellKey );
    }

    if(!f_endCellKey.isEmpty())
    {
        order.addParameter( f_endCellKey );
    }
}


/// \brief Row key predicate query handlers
void row_key_predicate::appendQuery( QString& query, int& bind_count )
{
    query += " WHERE key=?";
    ++bind_count;
    f_cell_pred->appendQuery( query, bind_count );
}


void row_key_predicate::bindOrder( order& order)
{
    order.addParameter( f_rowKey );
    f_cell_pred->bindOrder( order );
}


/// \brief Row range predicate query handlers
void row_range_predicate::appendQuery( QString& query, int& bind_count )
{
    query += " WHERE token(key) >= token(?) AND token(key) <= token(?)";
    bind_count += 2;
    f_cell_pred->appendQuery( query, bind_count );
}


void row_range_predicate::bindOrder( order& order )
{
    order.addParameter( f_startRowKey );
    order.addParameter( f_endRowKey );
    f_cell_pred->bindOrder( order );
}


} // namespace libdbproxy

// vim: ts=4 sw=4 et
