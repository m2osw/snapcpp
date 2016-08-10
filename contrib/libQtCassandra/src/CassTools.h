/*
 * Text:
 *      CassTools.h
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

#pragma once

#include "cassandra.h"

namespace QtCassandra
{

namespace CassTools
{

struct collectionDeleter
{
    void operator()(CassCollection* p) const;
};

struct columnMetaDeleter
{
    void operator()(const CassColumnMeta* /*p*/) const;
};

struct clusterDeleter
{ 
    void operator()(CassCluster* p) const;
};

struct futureDeleter
{ 
    void operator()(CassFuture* p) const;
};

struct iteratorDeleter
{
    void operator()(CassIterator* p) const;
};

struct keyspaceMetaDeleter
{
    void operator()(const CassKeyspaceMeta* /*p*/) const;
};

struct resultDeleter
{
    void operator()(const CassResult* p) const;
};

struct tableMetaDeleter
{ 
    void operator()(const CassTableMeta* /*p*/) const;
};

struct schemaMetaDeleter
{ 
    void operator()(const CassSchemaMeta* p) const;
};

struct sessionDeleter
{ 
    void operator()(CassSession* p) const;
};

struct statementDeleter
{ 
    void operator()(CassStatement* p) const;
};

struct valueDeleter
{ 
    void operator()(CassValue*       ) const {}
    void operator()(const CassValue* ) const {}
};

}
// namespace CassTools

}
//namespace QtCassandra

// vim: ts=4 sw=4 et
