/*
 * Text:
 *      src/batch.cpp
 *
 * Description:
 *      Wrapper of the batch CQL interface.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2017 Made to Order Software Corp.
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

#include "casswrapper/batch.h"
#include "casswrapper_impl.h"
#include "casswrapper/query.h"

#include <iostream>
#include <stdexcept>
#include <sstream>

#include <QtCore>

/** \class batch
 * \brief Encapulates the cassandra-cpp driver to handle batches of queries.
 *
 * \sa session, query
 */

namespace casswrapper
{


/** \brief Construct a batch object and manage the lifetime of the batch session.
 *
 * \sa batch
 */
Batch::Batch()
{
}


void Batch::clear()
{
    f_batch.reset();
}


bool Batch::isActive() const
{
    return static_cast<bool>(f_batch);
}


void Batch::addQuery( Query::pointer_t query )
{
    // Sanity checks
    //
    if( !f_batch )
    {
        throw libexcept::exception_t( "Batch::addQuery() cannot be called without an active batch!" );
    }

    f_queryList.push_back(query);
}


void Batch::run( bool const block )
{
    // Sanity checks
    //
    if( !f_batch )
    {
        throw libexcept::exception_t( "Batch::start() cannot be called without an active batch!" );
    }
    //
    if( f_queryList.empty() )
    {
        throw libexcept::exception_t( "Batch::start() need at least one query!" );
    }

    for( auto q : f_queryList )
    {
        q->addToBatch( f_batch.get() );
    }

    f_queryList[0]->internalStart( block, f_batch.get() );
}


LoggedBatch::LoggedBatch()
{
    f_batch = std::make_unique<batch>( CASS_BATCH_TYPE_LOGGED );
}


LoggedBatch::pointer_t LoggedBatch::create()
{
    return pointer_t(new LoggedBatch);
}


UnloggedBatch::UnloggedBatch()
{
    f_batch = std::make_unique<batch>( CASS_BATCH_TYPE_UNLOGGED );
}


UnloggedBatch::pointer_t UnloggedBatch::create()
{
    return pointer_t(new UnloggedBatch);
}


CounterBatch::CounterBatch()
{
    f_batch = std::make_unique<batch>( CASS_BATCH_TYPE_UNLOGGED );
}


CounterBatch::pointer_t CounterBatch::create()
{
    return pointer_t(new CounterBatch);
}


} // namespace casswrapper

// vim: ts=4 sw=4 et
