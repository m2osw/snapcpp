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

#include "QtCassandra/QCassandraConsistencyLevel.h"

#include <casswrapper/encoder.h>

#include <QString>
#include <QByteArray>

#include <execinfo.h>
#include <stdint.h>

#include <iostream>
#include <memory>

namespace QtCassandra
{

/*******************************************************************************
 * Light wrappers around the functions and classes in cassawrapper/encoder.h
 *******************************************************************************/

inline uint64_t getBufferMaxSize()
{
    return casswrapper::getBufferMaxSize();
}


inline void checkBufferSize(const uint64_t new_size)
{
    return casswrapper::checkBufferSize(new_size);
}

// Null
inline void setNullValue(QByteArray& array)
{
    casswrapper::setNullValue(array);
}

// Bool
inline void appendBoolValue(QByteArray& array, const bool value)
{
    casswrapper::appendBoolValue(array,value);
}

inline void setBoolValue(QByteArray& array, const bool value)
{
    casswrapper::setBoolValue(array,value);
}

inline bool boolValue(const QByteArray& array, const int index = 0)
{
    return casswrapper::boolValue(array,index);
}

inline bool boolValueOrNull(const QByteArray& array, const int index = 0, const bool default_value = false)
{
    return casswrapper::boolValueOrNull(array,index,default_value);
}

// Char
inline void appendCharValue(QByteArray& array, const char value)
{
    casswrapper::appendCharValue(array,value);
}

inline void setCharValue(QByteArray& array, const char value)
{
    casswrapper::setCharValue(array,value);
}

inline char charValue(const QByteArray& array, const int index = 0)
{
    return casswrapper::charValue(array,index);
}

inline char charValueOrNull(const QByteArray& array, const int index = 0, const char default_value = 0)
{
    return casswrapper::charValueOrNull(array,index,default_value);
}

inline char safeCharValue(const QByteArray& array, const int index = 0, const char default_value = 0)
{
    return casswrapper::safeCharValue(array,index,default_value);
}

inline void appendSignedCharValue(QByteArray& array, const signed char value)
{
    casswrapper::appendCharValue(array, value);
}

inline void setSignedCharValue(QByteArray& array, const signed char value)
{
    casswrapper::setCharValue(array, value);
}

inline signed char signedCharValue(const QByteArray& array, const int index = 0)
{
    return casswrapper::signedCharValue(array,index);
}

inline signed char signedCharValueOrNull(const QByteArray& array, const int index = 0, const signed char default_value = 0)
{
    return casswrapper::signedCharValueOrNull(array,index,default_value);
}

inline signed char safeSignedCharValue(const QByteArray& array, const int index = 0, const signed char default_value = 0)
{
    return casswrapper::safeSignedCharValue(array,index,default_value);
}

inline void appendUnsignedCharValue(QByteArray& array, const unsigned char value)
{
    casswrapper::appendCharValue(array, value);
}

inline void setUnsignedCharValue(QByteArray& array, const unsigned char value)
{
    casswrapper::setCharValue(array, value);
}

inline unsigned char unsignedCharValue(const QByteArray& array, const int index = 0)
{
    return casswrapper::unsignedCharValue(array,index);
}

inline unsigned char unsignedCharValueOrNull(const QByteArray& array, const int index = 0, const unsigned char default_value = 0)
{
    return casswrapper::unsignedCharValueOrNull(array,index,default_value);
}

inline unsigned char safeUnsignedCharValue(const QByteArray& array, const int index = 0, const unsigned char default_value = 0)
{
    return casswrapper::safeUnsignedCharValue(array,index,default_value);
}

// Int16
inline void appendInt16Value(QByteArray& array, const int16_t value)
{
    casswrapper::appendInt16Value(array,value);
}

inline void setInt16Value(QByteArray& array, const int16_t value)
{
    casswrapper::setInt16Value(array,value);
}

inline int16_t int16Value(const QByteArray& array, const int index = 0)
{
    return casswrapper::int16Value(array,index);
}

inline int16_t int16ValueOrNull(const QByteArray& array, const int index = 0, const int16_t default_value = 0)
{
    return casswrapper::int16ValueOrNull(array,index,default_value);
}

inline int16_t safeInt16Value(const QByteArray& array, const int index = 0, const int16_t default_value = 0)
{
    return casswrapper::safeInt16Value(array,index,default_value);
}

inline void appendUInt16Value(QByteArray& array, const uint16_t value)
{
    casswrapper::appendInt16Value(array, value);
}

inline void setUInt16Value(QByteArray& array, const uint16_t value)
{
    casswrapper::setInt16Value(array, value);
}

inline uint16_t uint16Value(const QByteArray& array, const int index = 0)
{
    return casswrapper::uint16Value(array,index);
}

inline uint16_t uint16ValueOrNull(const QByteArray& array, const int index = 0, const uint16_t default_value = 0)
{
    return casswrapper::uint16ValueOrNull(array,index,default_value);
}

inline uint16_t safeUInt16Value(const QByteArray& array, const int index = 0, const uint16_t default_value = 0)
{
    return casswrapper::safeUInt16Value(array,index,default_value);
}

// Int32
inline void appendInt32Value(QByteArray& array, const int32_t value)
{
    casswrapper::appendInt32Value(array,value);
}

inline void setInt32Value(QByteArray& array, const int32_t value)
{
    casswrapper::setInt32Value(array,value);
}

inline void replaceInt32Value(QByteArray& array, const int32_t value, const int index = 0)
{
    return casswrapper::replaceInt32Value(array,value,index);
}

inline int32_t int32Value(const QByteArray& array, const int index = 0)
{
    return casswrapper::int32Value(array,index);
}

inline int32_t int32ValueOrNull(const QByteArray& array, const int index = 0, const int32_t default_value = 0)
{
    return casswrapper::int32ValueOrNull(array,index,default_value);
}

inline int32_t safeInt32Value(const QByteArray& array, const int index = 0, const int32_t default_value = 0)
{
    return casswrapper::safeInt32Value(array,index,default_value);
}

inline void appendUInt32Value(QByteArray& array, const uint32_t value)
{
    casswrapper::appendInt32Value(array, value);
}

inline void setUInt32Value(QByteArray& array, const uint32_t value)
{
    casswrapper::setInt32Value(array, value);
}

inline void replaceUInt32Value(QByteArray& array, const uint32_t value, const int index = 0)
{
    casswrapper::replaceInt32Value(array, value, index);
}

inline uint32_t uint32Value(const QByteArray& array, const int index = 0)
{
    return casswrapper::uint32Value(array,index);
}

inline uint32_t uint32ValueOrNull(const QByteArray& array, const int index = 0, const uint32_t default_value = 0)
{
    return casswrapper::uint32ValueOrNull(array,index,default_value);
}

inline uint32_t safeUInt32Value(const QByteArray& array, const int index = 0, const uint32_t default_value = 0)
{
    return casswrapper::safeUInt32Value(array,index,default_value);
}

// Int64
inline void appendInt64Value(QByteArray& array, const int64_t value)
{
    casswrapper::appendInt64Value(array,value);
}

inline void setInt64Value(QByteArray& array, const int64_t value)
{
    casswrapper::setInt64Value(array,value);
}

inline int64_t int64Value(const QByteArray& array, const int index = 0)
{
    return casswrapper::int64Value(array,index);
}

inline int64_t int64ValueOrNull(const QByteArray& array, const int index = 0, const int64_t default_value = 0)
{
    return casswrapper::int64ValueOrNull(array,index,default_value);
}

inline int64_t safeInt64Value(const QByteArray& array, const int index = 0, const int64_t default_value = 0)
{
    return casswrapper::safeInt64Value(array,index,default_value);
}

inline void appendUInt64Value(QByteArray& array, const uint64_t value)
{
    casswrapper::appendInt64Value(array, value);
}

inline void setUInt64Value(QByteArray& array, const uint64_t value)
{
    casswrapper::setInt64Value(array, value);
}

inline uint64_t uint64Value(const QByteArray& array, const int index = 0)
{
    return casswrapper::uint64Value(array,index);
}

inline uint64_t uint64ValueOrNull(const QByteArray& array, const int index = 0, const uint64_t default_value = 0)
{
    return casswrapper::uint64ValueOrNull(array,index,default_value);
}

inline uint64_t safeUInt64Value(const QByteArray& array, const int index = 0, const uint64_t default_value = 0)
{
    return casswrapper::safeUInt64Value(array,index,default_value);
}

// Float
inline void setFloatValue(QByteArray& array, const float value)
{
    casswrapper::setFloatValue(array,value);
}

inline void appendFloatValue(QByteArray& array, const float value)
{
    casswrapper::appendFloatValue(array,value);
}

inline float floatValue(const QByteArray& array, const int index = 0)
{
    return casswrapper::floatValue(array,index);
}

inline float floatValueOrNull(const QByteArray& array, const int index = 0, const float default_value = 0)
{
    return casswrapper::floatValueOrNull(array,index,default_value);
}

inline float safeFloatValue(const QByteArray& array, const int index = 0, const float default_value = 0)
{
    return casswrapper::safeFloatValue(array,index,default_value);
}

// Double
inline void setDoubleValue(QByteArray& array, const double value)
{
    casswrapper::setDoubleValue(array,value);
}

inline void appendDoubleValue(QByteArray& array, const double value)
{
    casswrapper::appendDoubleValue(array,value);
}

inline double doubleValue(const QByteArray& array, const int index = 0)
{
    return casswrapper::doubleValue(array,index);
}

inline double doubleValueOrNull(const QByteArray& array, const int index = 0, const double default_value = 0)
{
    return casswrapper::doubleValueOrNull(array,index,default_value);
}

inline double safeDoubleValue(const QByteArray& array, const int index = 0, const double default_value = 0.0)
{
    return casswrapper::safeDoubleValue(array,index,default_value);
}

// String
inline void setStringValue(QByteArray& array, const QString& value)
{
    casswrapper::setStringValue(array,value);
}

inline void appendStringValue(QByteArray& array, const QString& value)
{
    casswrapper::appendStringValue(array,value);
}

inline QString stringValue(const QByteArray& array, const int index = 0, int size = -1)
{
    return casswrapper::stringValue(array,index,size);
}

// Binary
inline void setBinaryValue(QByteArray& array, const QByteArray& value)
{
    casswrapper::setBinaryValue(array,value);
}

inline void appendBinaryValue(QByteArray& array, const QByteArray& value)
{
    casswrapper::appendBinaryValue(array,value);
}

inline QByteArray binaryValue(const QByteArray& array, const int index = 0, int size = -1)
{
    return casswrapper::binaryValue(array,index,size);
}




class QCassandraEncoder : public casswrapper::Encoder
{
public:
    QCassandraEncoder(int reserve_size) : casswrapper::Encoder(reserve_size) {}
};


class QCassandraDecoder : public casswrapper::Decoder
{
public:
    QCassandraDecoder(QByteArray const & encoded) : casswrapper::Decoder(encoded) {}
};



/*******************************************************************************
 * QCassandraValue
 *******************************************************************************/


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
