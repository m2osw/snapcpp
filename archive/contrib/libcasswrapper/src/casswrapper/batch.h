/*
 * Text:
 *      src/casswrapper/batch.h
 *
 * Description:
 *      Wrap a Cassandra batch object.
 *
 * Documentation:
 *      See each function below.
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
#pragma once

#include <memory>
#include <vector>


#include <QSharedPointer>


namespace casswrapper
{

class Query;
class batch;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class Batch
    : public std::enable_shared_from_this<Batch>
{
public:
    typedef std::shared_ptr<Batch>  pointer_t;

    void            clear();
    bool            isActive() const;
    void            addQuery( QSharedPointer<Query> query );
    void            run( bool const block = true );

protected:
    friend class Query;

                    Batch();

    std::unique_ptr<batch>              f_batch;  // = std::unique_ptr<batch>(); -- batch is not yet defined
    std::vector<QSharedPointer<Query>>  f_queryList = std::vector<QSharedPointer<Query>>();
};
#pragma GCC diagnostic pop


class LoggedBatch
    : public Batch
{
public:
    LoggedBatch();

    static pointer_t    create();
};


class UnloggedBatch
    : public Batch
{
public:
    UnloggedBatch();

    static pointer_t    create();
};


class CounterBatch
    : public Batch
{
public:
    CounterBatch();

    static pointer_t    create();
};


}
// namespace casswrapper

// vim: ts=4 sw=4 et
