/*
 * Header:
 *      QCassandraValue.h
 *
 * Description:
 *      Handling of a cell value to access data within the Cassandra database.
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
#pragma once

#include "QtCassandra/QCassandraEncoder.h"
#include "QtCassandra/QCassandraConsistencyLevel.h"

#include <QString>
#include <QByteArray>

#include <execinfo.h>
#include <stdint.h>

#include <iostream>
#include <memory>

namespace QtCassandra
{


class QCassandraValue //: public QObject -- values are copyable and not named
{
public:
    static const int32_t    TTL_PERMANENT = 0;

    // TTL must be positive, although Cassandra allows 0 as "permanent"
    typedef int32_t         cassandra_ttl_t;

    enum timestamp_mode_t {
        TIMESTAMP_MODE_CASSANDRA,
        TIMESTAMP_MODE_AUTO,
        TIMESTAMP_MODE_DEFINED
    };

    // CASSANDRA_VALUE_TYPE_BINARY (empty buffer)
    QCassandraValue();

    // CASSANDRA_VALUE_TYPE_INTEGER
    QCassandraValue(bool value);
    QCassandraValue(char value);
    QCassandraValue(signed char value);
    QCassandraValue(unsigned char value);
    QCassandraValue(int16_t value);
    QCassandraValue(uint16_t value);
    QCassandraValue(int32_t value);
    QCassandraValue(uint32_t value);
    QCassandraValue(int64_t value);
    QCassandraValue(uint64_t value);

    // CASSANDRA_VALUE_TYPE_FLOAT
    QCassandraValue(float value);
    QCassandraValue(double value);

    // CASSANDRA_VALUE_TYPE_STRING
    QCassandraValue(const QString& value);

    // CASSANDRA_VALUE_TYPE_BINARY
    QCassandraValue(const QByteArray& value);
    QCassandraValue(const char *data, int size);

    void setNullValue();
    void setBoolValue(bool value);
    void setCharValue(char value);
    void setSignedCharValue(signed char value);
    void setUnsignedCharValue(unsigned char value);
    void setInt16Value(int16_t value);
    void setUInt16Value(uint16_t value);
    void setInt32Value(int32_t value);
    void setUInt32Value(uint32_t value);
    void setInt64Value(int64_t value);
    void setUInt64Value(uint64_t value);
    void setFloatValue(float value);
    void setDoubleValue(double value);
    void setStringValue(const QString& value);
    void setBinaryValue(const QByteArray& value);
    void setBinaryValue(const char *data, int data_size);

    int size() const;

    // whether size is zero
    bool nullValue() const;

    // bool
    bool boolValue      (int index = 0) const;
    bool boolValueOrNull(int index = 0, const bool default_value = false) const;
    bool safeBoolValue  (int index = 0, const bool default_value = false) const;

    // [[un]signed] char
    char          charValue              (int index = 0) const;
    char          charValueOrNull        (int index = 0, const char default_value = 0) const;
    char          safeCharValue          (int index = 0, const char default_value = 0) const;
    signed char   signedCharValue        (int index = 0) const;
    signed char   signedCharValueOrNull  (int index = 0, const signed char default_value = 0) const;
    signed char   safeSignedCharValue    (int index = 0, const signed char default_value = 0) const;
    unsigned char unsignedCharValue      (int index = 0) const;
    unsigned char unsignedCharValueOrNull(int index = 0, const unsigned char default_value = 0) const;
    unsigned char safeUnsignedCharValue  (int index = 0, const unsigned char default_value = 0) const;

    // [u]int16_t
    int16_t  int16Value       (int index = 0) const;
    int16_t  int16ValueOrNull (int index = 0, const int16_t default_value = 0) const;
    int16_t  safeInt16Value   (int index = 0, const int16_t default_value = 0) const;
    uint16_t uint16Value      (int index = 0) const;
    uint16_t uint16ValueOrNull(int index = 0, const uint16_t default_value = 0) const;
    uint16_t safeUInt16Value  (int index = 0, const uint16_t default_value = 0) const;

    // [u]int32_t
    int32_t  int32Value       (int index = 0) const;
    int32_t  int32ValueOrNull (int index = 0, const int32_t default_value = 0) const;
    int32_t  safeInt32Value   (int index = 0, const int32_t default_value = 0) const;
    uint32_t uint32Value      (int index = 0) const;
    uint32_t uint32ValueOrNull(int index = 0, const uint32_t default_value = 0) const;
    uint32_t safeUInt32Value  (int index = 0, const uint32_t default_value = 0) const;

    // [u]int64_t
    int64_t  int64Value       (int index = 0) const;
    int64_t  int64ValueOrNull (int index = 0, const int64_t default_value = 0) const;
    int64_t  safeInt64Value   (int index = 0, const int64_t default_value = 0) const;
    uint64_t uint64Value      (int index = 0) const;
    uint64_t uint64ValueOrNull(int index = 0, const uint64_t default_value = 0) const;
    uint64_t safeUInt64Value  (int index = 0, const uint64_t default_value = 0) const;

    // floating point
    float  floatValue       (int index = 0) const;
    float  floatValueOrNull (int index = 0, const float default_value = 0.0f) const;
    float  safeFloatValue   (int index = 0, const float default_value = 0.0f) const;
    double doubleValue      (int index = 0) const;
    double doubleValueOrNull(int index = 0, const double default_value = 0.0) const;
    double safeDoubleValue  (int index = 0, const double default_value = 0.0) const;

    // string / binary
    QString stringValue(int index = 0, int size = -1) const;
    const QByteArray& binaryValue() const;
    QByteArray binaryValue(int index, int size = -1) const;

    // There is no toString() because we do not know the type of your
    // cell (value) and therefore we have no means to convert it to
    // a string; you may try stringValue() instead?
    //QString toString() const;

    QCassandraValue& operator = (const char *null_value); // i.e. NULL
    QCassandraValue& operator = (bool value);
    QCassandraValue& operator = (char value);
    QCassandraValue& operator = (signed char value);
    QCassandraValue& operator = (unsigned char value);
    QCassandraValue& operator = (int16_t value);
    QCassandraValue& operator = (uint16_t value);
    QCassandraValue& operator = (int32_t value);
    QCassandraValue& operator = (uint32_t value);
    QCassandraValue& operator = (int64_t value);
    QCassandraValue& operator = (uint64_t value);
    QCassandraValue& operator = (float value);
    QCassandraValue& operator = (double value);
    QCassandraValue& operator = (const QString& value);
    QCassandraValue& operator = (const QByteArray& value);

    bool operator == (const QCassandraValue& rhs);
    bool operator != (const QCassandraValue& rhs);
    bool operator < (const QCassandraValue& rhs);
    bool operator <= (const QCassandraValue& rhs);
    bool operator > (const QCassandraValue& rhs);
    bool operator >= (const QCassandraValue& rhs);

    int32_t ttl() const;
    void setTtl(int32_t ttl = TTL_PERMANENT);

    consistency_level_t consistencyLevel() const;
    void setConsistencyLevel(consistency_level_t level);

private:
    // prevent share pointer assignments (i.e. output of
    // row->cell() instead of row->cell()->value())
    template<class T> QCassandraValue& operator = (std::shared_ptr<T>);

    QByteArray                  f_value;
    cassandra_ttl_t             f_ttl = TTL_PERMANENT;
    consistency_level_t         f_consistency_level = CONSISTENCY_LEVEL_DEFAULT;
    timestamp_mode_t            f_timestamp_mode = TIMESTAMP_MODE_AUTO;
    int64_t                     f_timestamp = 0;
};



} // namespace QtCassandra
// vim: ts=4 sw=4 et
