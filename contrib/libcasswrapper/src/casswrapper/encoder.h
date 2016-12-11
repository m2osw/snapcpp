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

#include "casswrapper/exception.h"

#include <QString>
#include <QByteArray>

#include <execinfo.h>
#include <stdint.h>

#include <iostream>
#include <memory>

namespace casswrapper
{

namespace {
    /** \brief Maximum buffer size.
     *
     * This variable represents the maximum buffer size of a QCassandraValue.
     * At this time this is limited to 64Mb. Some users have successfully used
     * Cassandra with 200Mb buffers, however, remember that you need a huge
     * amount of RAM to handle large buffers (your copy is 200Mb, Cassandra's
     * copy is 200Mb, that's at least 0.5Gb of RAM just for that ONE cell!)
     */
    const uint64_t BUFFER_MAX_SIZE = 64 * 1024 * 1024;
}

inline uint64_t getBufferMaxSize()
{
    return BUFFER_MAX_SIZE;
}

// Verify final buffer size against limit
inline void checkBufferSize(const uint64_t new_size)
{
    if(new_size > BUFFER_MAX_SIZE) {
        throw exception_t(QString("resulting value is more than %1 bytes").arg(BUFFER_MAX_SIZE).toUtf8().data());
    }
}

// Null
inline void setNullValue(QByteArray& array)
{
    array.clear();
}

// Bool
inline void appendBoolValue(QByteArray& array, const bool value)
{
    checkBufferSize(array.size() + 1);

    char buf[1];
    buf[0] = value ? 1 : 0;
    array.append(buf, 1);
}

inline void setBoolValue(QByteArray& array, const bool value)
{
    array.clear();
    appendBoolValue(array, value);
}

inline bool boolValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this boolValue");
    }
    return array.at(index) != 0;
}

inline bool boolValueOrNull(const QByteArray& array, const int index = 0, const bool default_value = false)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    return array.at(index) != 0;
}

// Char
inline void appendCharValue(QByteArray& array, const char value)
{
    checkBufferSize(array.size() + 1);

    array.append(&value, 1);
}

inline void setCharValue(QByteArray& array, const char value)
{
    array.clear();
    appendCharValue(array, value);
}

inline char charValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this charValue");
    }
    return array.at(index);
}

inline char charValueOrNull(const QByteArray& array, const int index = 0, const char default_value = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    return array.at(index);
}

inline char safeCharValue(const QByteArray& array, const int index = 0, const char default_value = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    return static_cast<char>(array.at(index));
}

inline void appendSignedCharValue(QByteArray& array, const signed char value)
{
    appendCharValue(array, value);
}

inline void setSignedCharValue(QByteArray& array, const signed char value)
{
    setCharValue(array, value);
}

inline signed char signedCharValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this signedCharValue");
    }
    return static_cast<signed char>(array.at(index));
}

inline signed char signedCharValueOrNull(const QByteArray& array, const int index = 0, const signed char default_value = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    return static_cast<signed char>(array.at(index));
}

inline signed char safeSignedCharValue(const QByteArray& array, const int index = 0, const signed char default_value = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    return static_cast<signed char>(array.at(index));
}

inline void appendUnsignedCharValue(QByteArray& array, const unsigned char value)
{
    appendCharValue(array, value);
}

inline void setUnsignedCharValue(QByteArray& array, const unsigned char value)
{
    setCharValue(array, value);
}

inline unsigned char unsignedCharValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this unsignedCharValue");
    }
    return static_cast<unsigned char>(array.at(index));
}

inline unsigned char unsignedCharValueOrNull(const QByteArray& array, const int index = 0, const unsigned char default_value = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    return array.at(index);
}

inline unsigned char safeUnsignedCharValue(const QByteArray& array, const int index = 0, const unsigned char default_value = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    return static_cast<unsigned char>(array.at(index));
}

// Int16
inline void appendInt16Value(QByteArray& array, const int16_t value)
{
    checkBufferSize(array.size() + sizeof(int16_t));

    char buf[2];
    buf[0] = static_cast<char>(value >> 8);
    buf[1] = static_cast<char>(value);
    array.append(buf, 2);
}

inline void setInt16Value(QByteArray& array, const int16_t value)
{
    array.clear();
    appendInt16Value(array, value);
}

inline int16_t int16Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // avoid potential overflow with the +2
    || static_cast<unsigned int>(index + sizeof(int16_t)) > static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this int16Value");
    }
    return static_cast<int16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                               | static_cast<unsigned char>(array.at(index + 1)));
}

inline int16_t int16ValueOrNull(const QByteArray& array, const int index = 0, const int16_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int16_t)) <= static_cast<unsigned int>(array.size())) {
        return static_cast<int16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                                   | static_cast<unsigned char>(array.at(index + 1)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw exception_t("buffer too small for this int16ValueOrNull");
}

inline int16_t safeInt16Value(const QByteArray& array, const int index = 0, const int16_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int16_t)) <= static_cast<unsigned int>(array.size())) {
        return static_cast<int16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                                   | static_cast<unsigned char>(array.at(index + 1)));
    }
    return default_value;
}

inline void appendUInt16Value(QByteArray& array, const uint16_t value)
{
    appendInt16Value(array, value);
}

inline void setUInt16Value(QByteArray& array, const uint16_t value)
{
    setInt16Value(array, value);
}

inline uint16_t uint16Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(uint16_t)) > static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this uint16Value");
    }
    return static_cast<uint16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                                | static_cast<unsigned char>(array.at(index + 1)));
}

inline uint16_t uint16ValueOrNull(const QByteArray& array, const int index = 0, const uint16_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint16_t)) <= static_cast<unsigned int>(array.size())) {
        return static_cast<uint16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                                    | static_cast<unsigned char>(array.at(index + 1)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw exception_t("buffer too small for this uint16ValueOrNull");
}

inline uint16_t safeUInt16Value(const QByteArray& array, const int index = 0, const uint16_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint16_t)) <= static_cast<unsigned int>(array.size())) {
        return static_cast<uint16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                                    | static_cast<unsigned char>(array.at(index + 1)));
    }
    return default_value;
}

// Int32
inline void appendInt32Value(QByteArray& array, const int32_t value)
{
    checkBufferSize(array.size() + sizeof(int32_t));

    char buf[4];
    buf[0] = static_cast<char>(value >> 24);
    buf[1] = static_cast<char>(value >> 16);
    buf[2] = static_cast<char>(value >> 8);
    buf[3] = static_cast<char>(value);
    array.append(buf, 4);
}

inline void setInt32Value(QByteArray& array, const int32_t value)
{
    array.clear();
    appendInt32Value(array, value);
}

inline void replaceInt32Value(QByteArray& array, const int32_t value, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(int32_t)) > static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this int32Value");
    }
    char buf[4];
    buf[0] = static_cast<char>(value >> 24);
    buf[1] = static_cast<char>(value >> 16);
    buf[2] = static_cast<char>(value >> 8);
    buf[3] = static_cast<char>(value);
    array.replace(index, 4, buf, 4);
}

inline int32_t int32Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(int32_t)) > static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this int32Value");
    }
    return (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
         | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
         | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
         | static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 3)));
}

inline int32_t int32ValueOrNull(const QByteArray& array, const int index = 0, const int32_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int32_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
             | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
             | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
             | static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 3)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw exception_t("buffer too small for this int32ValueOrNull");
}

inline int32_t safeInt32Value(const QByteArray& array, const int index = 0, const int32_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int32_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
             | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
             | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
             | static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 3)));
    }
    return default_value;
}

inline void appendUInt32Value(QByteArray& array, const uint32_t value)
{
    appendInt32Value(array, value);
}

inline void setUInt32Value(QByteArray& array, const uint32_t value)
{
    setInt32Value(array, value);
}

inline void replaceUInt32Value(QByteArray& array, const uint32_t value, const int index = 0)
{
    replaceInt32Value(array, value, index);
}

inline uint32_t uint32Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(uint32_t)) > static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this uint32Value");
    }
    return (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
         | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
         | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
         | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
}

inline uint32_t uint32ValueOrNull(const QByteArray& array, const int index = 0, const uint32_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint32_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
             | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
             | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
             | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw exception_t("buffer too small for this uint32ValueOrNull");
}

inline uint32_t safeUInt32Value(const QByteArray& array, const int index = 0, const uint32_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint32_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
             | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
             | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
             | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
    }
    return default_value;
}

// Int64
inline void appendInt64Value(QByteArray& array, const int64_t value)
{
    checkBufferSize(array.size() + 8);

    char buf[8];
    buf[0] = static_cast<char>(value >> 56);
    buf[1] = static_cast<char>(value >> 48);
    buf[2] = static_cast<char>(value >> 40);
    buf[3] = static_cast<char>(value >> 32);
    buf[4] = static_cast<char>(value >> 24);
    buf[5] = static_cast<char>(value >> 16);
    buf[6] = static_cast<char>(value >> 8);
    buf[7] = static_cast<char>(value);
    array.append(buf, 8);
}

inline void setInt64Value(QByteArray& array, const int64_t value)
{
    array.clear();
    appendInt64Value(array, value);
}

inline int64_t int64Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(int64_t)) > static_cast<unsigned int>(array.size())) {
        throw exception_t(QString("buffer too small (%1) for this int64Value").arg(array.size()).toStdString());
    }
    return (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
         | static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 7)));
}

inline int64_t int64ValueOrNull(const QByteArray& array, const int index = 0, const int64_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int64_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
             | static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 7)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw exception_t("buffer too small for this int64ValueOrNull");
}

inline int64_t safeInt64Value(const QByteArray& array, const int index = 0, const int64_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int64_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
             | static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 7)));
    }
    return default_value;
}

inline void appendUInt64Value(QByteArray& array, const uint64_t value)
{
    appendInt64Value(array, value);
}

inline void setUInt64Value(QByteArray& array, const uint64_t value)
{
    setInt64Value(array, value);
}

inline uint64_t uint64Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(uint64_t)) > static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this uint64Value");
    }
    return (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
         | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
}

inline uint64_t uint64ValueOrNull(const QByteArray& array, const int index = 0, const uint64_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint64_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
             | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw exception_t("buffer too small for this uint64ValueOrNull");
}

inline uint64_t safeUInt64Value(const QByteArray& array, const int index = 0, const uint64_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint64_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
             | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
    }
    return default_value;
}

// Float
inline void setFloatValue(QByteArray& array, const float value)
{
    union switch_t {
        float f;
        uint32_t v;
    };
    switch_t s;
    s.f = value;
    setInt32Value(array, s.v);
}

inline void appendFloatValue(QByteArray& array, const float value)
{
    union switch_t {
        float f;
        uint32_t v;
    };
    switch_t s;
    s.f = value;
    appendInt32Value(array, s.v);
}

inline float floatValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(float)) > static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this floatValue");
    }
    union switch_t {
        uint32_t    v;
        float       f;
    };
    switch_t s;
    s.v = static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 0))) << 24)
        | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 1))) << 16)
        | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 2))) << 8)
        | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
    return s.f;
}

inline float floatValueOrNull(const QByteArray& array, const int index = 0, const float default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(float)) <= static_cast<unsigned int>(array.size())) {
        union switch_t {
            uint32_t    v;
            float       f;
        };
        switch_t s;
        s.v = static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 0))) << 24)
            | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 1))) << 16)
            | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 2))) << 8)
            | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
        return s.f;
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw exception_t("buffer too small for this floatValueOrNull");
}

inline float safeFloatValue(const QByteArray& array, const int index = 0, const float default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(float)) <= static_cast<unsigned int>(array.size())) {
        union switch_t {
            uint32_t    v;
            float       f;
        };
        switch_t s;
        s.v = static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 0))) << 24)
            | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 1))) << 16)
            | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 2))) << 8)
            | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
        return s.f;
    }
    return default_value;
}

// Double
inline void setDoubleValue(QByteArray& array, const double value)
{
    union switch_t {
        double d;
        uint64_t v;
    };
    switch_t s;
    s.d = value;
    setInt64Value(array, s.v);
}

inline void appendDoubleValue(QByteArray& array, const double value)
{
    union switch_t {
        double d;
        uint64_t v;
    };
    switch_t s;
    s.d = value;
    appendInt64Value(array, s.v);
}

inline double doubleValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(double)) > static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this doubleValue");
    }
    union switch_t {
        uint64_t    v;
        double      d;
    };
    switch_t s;
    s.v = (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
        | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
    return s.d;
}

inline double doubleValueOrNull(const QByteArray& array, const int index = 0, const double default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(double)) <= static_cast<unsigned int>(array.size())) {
        union switch_t {
            uint64_t    v;
            double      d;
        };
        switch_t s;
        s.v = (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
            | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
        return s.d;
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw exception_t("buffer too small for this doubleValueOrNull");
}

inline double safeDoubleValue(const QByteArray& array, const int index = 0, const double default_value = 0.0)
{
    if(static_cast<unsigned int>(index + sizeof(double)) <= static_cast<unsigned int>(array.size())) {
        union switch_t {
            uint64_t    v;
            double      d;
        };
        switch_t s;
        s.v = (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
            | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
        return s.d;
    }
    return default_value;
}

// String
inline void setStringValue(QByteArray& array, const QString& value)
{
    const QByteArray str(value.toUtf8());
    checkBufferSize(str.size());

    array = str;
}

inline void appendStringValue(QByteArray& array, const QString& value)
{
    const QByteArray str(value.toUtf8());
    checkBufferSize(array.size() + str.size());

    array.append(str);
}

inline QString stringValue(const QByteArray& array, const int index = 0, int size = -1)
{
    if(size == -1) {
        size = array.size() - index;
    }
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size())
    || static_cast<unsigned int>(size) > static_cast<unsigned int>(array.size())
    || static_cast<unsigned int>(index + size) > static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this stringValue");
    }
    return QString::fromUtf8(array.data() + index, size);
}

// Binary
inline void setBinaryValue(QByteArray& array, const QByteArray& value)
{
    checkBufferSize(value.size());

    array = value;
}

inline void appendBinaryValue(QByteArray& array, const QByteArray& value)
{
    checkBufferSize(array.size() + value.size());

    array.append(value);
}

inline QByteArray binaryValue(const QByteArray& array, const int index = 0, int size = -1)
{
    if(size == -1) {
        size = array.size() - index;
    }
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size())
    || static_cast<unsigned int>(size) > static_cast<unsigned int>(array.size())
    || static_cast<unsigned int>(index + size) > static_cast<unsigned int>(array.size())) {
        throw exception_t("buffer too small for this binaryValue() call");
    }
    return QByteArray(array.data() + index, size);
}




class Encoder
{
public:
    Encoder(int reserve_size)
    {
        f_array.reserve(reserve_size);
    }

    void appendSignedCharValue(signed char value)
    {
        casswrapper::appendSignedCharValue(f_array, value);
    }

    void appendUnsignedCharValue(unsigned char value)
    {
        casswrapper::appendUnsignedCharValue(f_array, value);
    }

    void appendInt16Value(int16_t value)
    {
        casswrapper::appendInt16Value(f_array, value);
    }

    void appendUInt16Value(uint16_t value)
    {
        casswrapper::appendUInt16Value(f_array, value);
    }

    void appendInt32Value(int32_t value)
    {
        casswrapper::appendInt32Value(f_array, value);
    }

    void appendUInt32Value(uint32_t value)
    {
        casswrapper::appendUInt32Value(f_array, value);
    }

    void appendInt64Value(int64_t value)
    {
        casswrapper::appendInt64Value(f_array, value);
    }

    void appendUInt64Value(uint64_t value)
    {
        casswrapper::appendUInt64Value(f_array, value);
    }

    void appendDoubleValue(double value)
    {
        casswrapper::appendDoubleValue(f_array, value);
    }

    void appendP16StringValue(const QString& value)
    {
        QByteArray const utf8(value.toUtf8());
        if(utf8.length() >= 64 * 1024)
        {
            // we should look whether there is a smaller limit in Cassandra
            // although we only send very small CQL commands at this point
            // anyway...
            //
            throw exception_t("strings are limited to 64Kb");
        }

        casswrapper::appendUInt16Value(f_array, utf8.length());
        casswrapper::appendBinaryValue(f_array, utf8);
    }

    void appendBinaryValue(const QByteArray& value)
    {
        casswrapper::appendUInt32Value(f_array, value.length());
        casswrapper::appendBinaryValue(f_array, value);
    }

    void replaceUInt32Value(uint32_t value, int index)
    {
        casswrapper::replaceUInt32Value(f_array, value, index);
    }

    size_t size() const
    {
        return f_array.size();
    }

    const QByteArray& result() const
    {
        return f_array;
    }

private:
    QByteArray      f_array;
};


class Decoder
{
public:
    Decoder(QByteArray const & encoded)
        : f_array(encoded)
        //, f_index(0)
    {
    }

    signed char signedCharValue() const
    {
        signed char const result(casswrapper::signedCharValue(f_array, f_index));
        f_index += 1;
        return result;
    }

    unsigned char unsignedCharValue() const
    {
        unsigned char const result(casswrapper::unsignedCharValue(f_array, f_index));
        f_index += 1;
        return result;
    }

    int16_t int16Value() const
    {
        int16_t const result(casswrapper::int16Value(f_array, f_index));
        f_index += 2;
        return result;
    }

    uint16_t uint16Value() const
    {
        uint16_t const result(casswrapper::uint16Value(f_array, f_index));
        f_index += 2;
        return result;
    }

    int32_t int32Value() const
    {
        int32_t result(casswrapper::int32Value(f_array, f_index));
        f_index += 4;
        return result;
    }

    uint32_t uint32Value() const
    {
        uint32_t const result(casswrapper::uint32Value(f_array, f_index));
        f_index += 4;
        return result;
    }

    int64_t int64Value() const
    {
        int64_t const result(casswrapper::int64Value(f_array, f_index));
        f_index += 8;
        return result;
    }

    uint64_t uint64Value() const
    {
        uint64_t const result(casswrapper::uint64Value(f_array, f_index));
        f_index += 8;
        return result;
    }

    double doubleValue() const
    {
        double const result(casswrapper::doubleValue(f_array, f_index));
        f_index += 8;
        return result;
    }

    QString p16StringValue() const
    {
        uint16_t const length(uint16Value());
        QString const result(QString::fromUtf8(casswrapper::binaryValue(f_array, f_index, length)));
        f_index += length;
        return result;
    }

    QString stringValue(int length) const
    {
        QString const result(QString::fromUtf8(casswrapper::binaryValue(f_array, f_index, length)));
        f_index += length;
        return result;
    }

    QByteArray binaryValue() const
    {
        uint32_t const length(uint32Value());
        QByteArray const result(casswrapper::binaryValue(f_array, f_index, length));
        f_index += length;
        return result;
    }

private:
    QByteArray      f_array;
    mutable int     f_index = 0;
};

} // namespace casswrapper

// vim: ts=4 sw=4 et
