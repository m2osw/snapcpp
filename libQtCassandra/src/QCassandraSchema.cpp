/*
 * Text:
 *      QCassandraSchema.h
 *
 * Description:
 *      Database schema metadata.
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

#include "QtCassandra/QCassandraQuery.h"

#include <memory>
#include <map>
#include <QString>

namespace QtCassandra
{

namespace QCassandraSchema
{


SessionMeta::SessionMeta( QCassandraSession::pointer_t session )
    f_session(session)
{
    // TODO read in full schema
}


SessionMeta::~SessionMeta()
{
}


QCassandraSession::pointer_t SessionMeta::session() const
{
    return f_session;
}

uint32_t   SessionMeta::snapshotVersion() const
{
    return f_version;
}



const SessionMeta::KeyspaceMeta::map_t& getKeyspaces()
{
    return f_keyspaces;
}


QString SessionMeta::KeyspaceMeta::getName() const
{
    return f_name;
}


SessionMeta::qstring_map_t SessionMeta::KeyspaceMeta::getFields() const
{
    return f_fields;
}


const SessionMeta::KeyspaceMeta::TableMeta::map_t& SessionMeta::KeyspaceMeta::getTableMetaMap() const
{
    return f_tables;
}


}
//namespace QCassandraSchema

}
// namespace QtCassandra

// vim: ts=4 sw=4 et
