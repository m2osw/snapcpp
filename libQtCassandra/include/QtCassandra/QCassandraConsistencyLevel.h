/*
 * Header:
 *      QCassandraConsistencyLevel.h
 *
 * Description:
 *      Redefinition of the Cassandra consistency levels in the QtCassandra
 *      library.
 *
 * Documentation:
 *      See the corresponding .cpp file.
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
#ifndef QCASSANDRA_CONSISTENCY_LEVEL_H
#define QCASSANDRA_CONSISTENCY_LEVEL_H

#include <controlled_vars/controlled_vars_auto_init.h>
#include <QString>
#include <QByteArray>
#include <stdint.h>


namespace QtCassandra
{



// redefined from the Thrift definition since we do not want
// to have to include Thirft in our public header files.
typedef int cassandra_consistency_level_t;

extern const cassandra_consistency_level_t CONSISTENCY_LEVEL_DEFAULT;

extern const cassandra_consistency_level_t CONSISTENCY_LEVEL_ONE;
extern const cassandra_consistency_level_t CONSISTENCY_LEVEL_QUORUM;
extern const cassandra_consistency_level_t CONSISTENCY_LEVEL_LOCAL_QUORUM;
extern const cassandra_consistency_level_t CONSISTENCY_LEVEL_EACH_QUORUM;
extern const cassandra_consistency_level_t CONSISTENCY_LEVEL_ALL;
extern const cassandra_consistency_level_t CONSISTENCY_LEVEL_ANY;
extern const cassandra_consistency_level_t CONSISTENCY_LEVEL_TWO;
extern const cassandra_consistency_level_t CONSISTENCY_LEVEL_THREE;

// CONSISTENCY_LEVEL_DEFAULT is -1 (but we need a constant in this template)
// Default converts to CONSISTENCY_LEVEL_ONE unless changed in
// the QCassandra object; see setDefaultConsistencyLevel()
typedef controlled_vars::auto_init<cassandra_consistency_level_t, -1> consistency_level_t;




} // namespace QtCassandra
#endif
//#ifndef QCASSANDRA_CONSISTENCY_LEVEL_H
// vim: ts=4 sw=4 et
