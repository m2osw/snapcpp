/*
 * Text:
 *      CassTools.cpp
 *
 * Description:
 *      Handling of the CQL interface.
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

#include "CassTools.h"

namespace QtCassandra
{

namespace CassTools
{


void collection::deleter::operator()(CassCollection* p) const
{
    cass_collection_free(p);
}


collection::collection( CassCollectionType type, size_t item_count )
    : f_ptr( cass_collection_new( type, item_count ), deleter_t )
{
}


collection::~collection()
{
    // Empty
}


CassError collection::append_string( const char* value )
{
    return cass_collection_append_string( f_ptr.get(), value );
}


column_meta::column_meta( const iterator& iter )
{
}


void columnMetaDeleter::operator()(const CassColumnMeta * /*p*/) const
{
    // No need to delete anything
    //cass_column_meta_free(p);
}

void clusterDeleter::operator()(CassCluster * p) const
{
    cass_cluster_free(p);
}

void futureDeleter::operator()(CassFuture * p) const
{
    cass_future_free(p);
}


iterator::iterator( CassIterator* iter )
    : f_ptr(iter)
{
}


iterator::~iterator()
{
    cass_iterator_free( f_ptr );
}


void keyspaceMetaDeleter::operator()(const CassKeyspaceMeta * /*p*/) const
{
    //cass_keyspace_meta_deleter(p);
}

void resultDeleter::operator()(const CassResult* p) const
{
    cass_result_free(p);
}

void tableMetaDeleter::operator()(const CassTableMeta* /*p*/) const
{
    //cass_table_meta_free(p);
}

void schemaMetaDeleter::operator()(const CassSchemaMeta* p) const
{
    cass_schema_meta_free(p);
}

void sessionDeleter::operator()(CassSession* p) const
{
    cass_session_free(p);
}

void sslDeleter::operator()(CassSsl* p) const
{
    cass_ssl_free(p);
}

void statementDeleter::operator()(CassStatement* p) const
{
    cass_statement_free(p);
}

}
// namespace CassTools

}
//namespace QtCassandra

// vim: ts=4 sw=4 et
