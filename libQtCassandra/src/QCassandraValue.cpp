/*
 * Text:
 *      QCassandraValue.cpp
 *
 * Description:
 *      Handling of a cell value.
 *
 * Documentation:
 *      See each function below.
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

#include "QtCassandra/QCassandraValue.h"
#include <stdexcept>

namespace QtCassandra
{

/** \class QCassandraValue
 * \brief Holds a cell value.
 *
 * This class defines a value that is saved in a cell in the Cassandra
 * database. The class is optimized to with a QByteArray as the main
 * data holder.
 *
 * You can set the value as an integer, a floating point, a string, or
 * directly as a binary buffer. Strings are converted to UTF-8. Integers
 * and floating points are saved in big endian format (i.e. can then
 * be compared with a simple memcmp() call and sorted without magic
 * when saved as a Cassandra BytesType.)
 *
 * At this point, the copy operator is not overloaded meaning that
 * everything is copied as is. This also means the QByteArray copy
 * on write feature is used (i.e. the data buffer itself doesn't
 * get copied until the source of the destination is written to.)
 *
 * On the other hand, this means we need to copy the QByteArray
 * buffer to the std::string of the Column structure defined in
 * the Cassandra thrift code before sending it to Cassandra. In other
 * words, you may end up with one copy that you could otherwise have
 * avoided. On the other hand, many times just the QCassandraValue
 * objects will be copied between each others.
 */

/** \fn void appendBinaryValue(QByteArray &array, const QByteArray &value);
 * \brief Append the array \p value to the array.
 *
 * Append the byte array in \p value to the byte array in \p array. This
 * function keeps all the existing data in place. The result must fit
 * in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendBoolValue(QByteArray &array, bool value);
 * \brief Append the Boolean value to the array.
 *
 * Append the Boolean \p value (0 or 1) to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendCharValue(QByteArray &array, const char value);
 * \brief Append the character to the array.
 *
 * Append the character \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendDoubleValue(QByteArray &array, double value);
 * \brief Append the floating point to the array.
 *
 * Append the double \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \note
 * Floating points are added in Big Endian format.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendFloatValue(QByteArray &array, float value);
 * \brief Append the floating point to the array.
 *
 * Append the float \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \note
 * Floating points are added in Big Endian format.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendInt16Value(QByteArray &array, int16_t value);
 * \brief Append the signed integer to the array.
 *
 * Append the 16 bit signed integers \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \note
 * Integers are added in Big Endian format.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendInt32Value(QByteArray &array, int32_t value);
 * \brief Append the signed integer to the array.
 *
 * Append the 32 bit signed integers \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \note
 * Integers are added in Big Endian format.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendInt64Value(QByteArray &array, int64_t value);
 * \brief Append the signed integer to the array.
 *
 * Append the 64 bit signed integers \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \note
 * Integers are added in Big Endian format.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendSignedCharValue(QByteArray &array, signed char value);
 * \brief Append the signed integer to the array.
 *
 * Append the 8 bit signed byte \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendStringValue(QByteArray &array, const QString &value);
 * \brief Append the string to the array.
 *
 * Append the string \p value to the byte \p array in UTF-8 format.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendUInt16Value(QByteArray &array, uint16_t value);
 * \brief Append the unsigned integer to the array.
 *
 * Append the 16 bit unsigned integers \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \note
 * Integers are added in Big Endian format.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendUInt32Value(QByteArray &array, uint32_t value);
 * \brief Append the unsigned integer to the array.
 *
 * Append the 32 bit unsigned integers \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \note
 * Integers are added in Big Endian format.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendUInt64Value(QByteArray &array, uint64_t value);
 * \brief Append the unsigned integer to the array.
 *
 * Append the 64 bit unsigned integers \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \note
 * Integers are added in Big Endian format.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void appendUnsignedCharValue(QByteArray &array, unsigned char value);
 * \brief Verify that a given size is compatible with QtCassandra.
 *
 * Append the 8 bit unsigned byte \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void checkBufferSize(uint64_t new_size);
 * \brief Verify that a given size is compatible with QtCassandra.
 *
 * This function checks the new size of the buffer against the maximum
 * size that the buffer accepts. If the new size is too large, it
 * raises an exception which in effect prevents the set or append
 * function from happening.
 *
 * If you want to check the size beforehand and avoid an exception,
 * you can compute the new size of your buffer and then check it
 * against QtCassandra::BUFFER_MAX_SIZE
 *
 * \param[in] new_size  The size of the array after an append or copy.
 */

/** \fn void setBinaryValue(QByteArray &array, const QByteArray &value);
 * \brief Copy the specified array to the destination.
 *
 * Replace the current \p array value with the specified \p value.
 * The previous value of \p array is lost.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setBoolValue(QByteArray &array, bool value);
 * \brief Set the array to the specified Boolean value.
 *
 * Set the Boolean \p value (0 or 1) in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \param[in,out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setCharValue(QByteArray &array, const char value);
 * \brief Set the array to the specified character.
 *
 * Set the character \p value in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setDoubleValue(QByteArray &array, double value);
 * \brief Set the array to the specified floating point.
 *
 * Set the double \p value in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \note
 * Doubles are saved in Big Endian format.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setFloatValue(QByteArray &array, float value);
 * \brief Set the array to the specified floating point.
 *
 * Set the float \p value in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \note
 * Floats are saved in Big Endian format.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setInt16Value(QByteArray &array, int16_t value);
 * \brief Set the array to the specified signed integer.
 *
 * Set the 16 bit integer \p value in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \note
 * Integers are saved in Big Endian format.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setInt32Value(QByteArray &array, int32_t value);
 * \brief Set the array to the specified signed integer.
 *
 * Set the 32 bit integer \p value in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \note
 * Integers are saved in Big Endian format.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setInt64Value(QByteArray &array, int64_t value);
 * \brief Set the array to the specified signed integer.
 *
 * Set the 64 bit integer \p value in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \note
 * Integers are saved in Big Endian format.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setNullValue(QByteArray &array);
 * \brief Empty the buffer.
 *
 * Empty the buffer which represents the Null value.
 * The size is then zero.
 *
 * \param[out] array  The array where value is copied.
 */

/** \fn void setSignedCharValue(QByteArray &array, signed char value);
 * \brief Set the array to the specified character.
 *
 * Set the signed character \p value in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setStringValue(QByteArray &array, const QString &value);
 * \brief Set the array to the specified string.
 *
 * Replace the value of the current \p array with the string in \p value.
 * The string is converted to UTF-8 which in many cases is more
 * compressed than the default UCS-2 defined in a QString.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setUInt16Value(QByteArray &array, uint16_t value);
 * \brief Set the array to the specified unsigned integer.
 *
 * Set the 16 bit unsigned integer \p value in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \note
 * Integers are saved in Big Endian format.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setUInt32Value(QByteArray &array, uint32_t value);
 * \brief Set the array to the specified unsigned integer.
 *
 * Set the 32 bit unsigned integer \p value in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \note
 * Integers are saved in Big Endian format.
 *
 * \param[in,out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setUInt64Value(QByteArray &array, uint64_t value);
 * \brief Set the array to the specified unsigned integer.
 *
 * Set the 64 bit unsigned integer \p value in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \note
 * Integers are saved in Big Endian format.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \fn void setUnsignedCharValue(QByteArray &array, unsigned char value);
 * \brief Set the array to the specified unsigned integer.
 *
 * Set the unsigned character \p value in the byte \p array
 * replacing the previous data in its entirety.
 *
 * \param[out] array  The array where value is copied.
 * \param[in] value  The new value of the array.
 */

/** \var QCassandraValue::TTL_PERMANENT
 * \brief Mark the column as permanent.
 *
 * By default, all the cells are saved with a TLL set to TTL_PERMANENT
 * which means that the cells will persist as long as they are not
 * deleted with a remove, truncate, or drop.
 *
 * This value represents a permanent cell, whereas other values saved
 * in the TTL parameter represent seconds.
 *
 * \sa setTtl()
 * \sa ttl()
 */

/** \typedef QCassandraValue::cassandra_ttl_t
 * \brief A safe TTL variable type.
 *
 * This definition is used to handle the TTL of a cell and is defined
 * in the value of each cell.
 *
 * The type forces the TTL value to TTL_PERMANENT by default. It also
 * bounds the value to positive numbers (plus 0 as TTL_PERMANENT is zero.)
 */

/** \enum QCassandraValue::def_timestamp_mode_t
 * \brief Intermediate definition to support a safe timestamp mode.
 *
 * The intermediate definition of the timestamp is used to declare a
 * limited controlled variable that has bounds checked automatically
 * and initializes variables of type timestamp_mode_t to
 * TIMESTAMP_MODE_AUTO.
 */

/** \var QCassandraValue::TIMESTAMP_MODE_CASSANDRA
 * \brief Mark the timestamp as unused.
 *
 * It is possible to mark the Value timestamp as unused. This means
 * the libQtCassandra library will not set the timestamp leaving to
 * the Cassandra server that task. This is not always possible.
 *
 * TBD -- it looks like the timestamp is mandatory even when marked
 * optional; there may be some cases when it's not necessary but
 * while writing to a row, it's dearly required. So this is not
 * the default. Instead the TIMESTAMP_MODE_AUTO is.
 */

/** \var QCassandraValue::TIMESTAMP_MODE_AUTO
 * \brief Ask the library to define the timestamp.
 *
 * The timestamp will automatically be set using the gettimeofday(2)
 * function. The seconds are multiplied by 1,000,000 and the microseconds
 * added to the result. This fits in 64 bits and will do so for a long
 * while.
 *
 * Note that the last timestamp generated by the system gets saved
 * in the corresponding QCassandraValue f_timestamp variable member.
 *
 * This is the default.
 */

/** \var QCassandraValue::TIMESTAMP_MODE_DEFINED
 * \brief The timestamp was defined by you.
 *
 * The QCassandraValue::f_timestamp will be used as the timestamp of this
 * value.
 *
 * Note that when you read the value of a cell, its timestamp is saved in
 * the QCassandraValue, but the mode is not changed. The result is that
 * you can still retrieve the timestamp with the timestamp() function.
 */

/** \var QCassandraValue::f_consistency_level
 * \brief The consitency level for this value.
 *
 * Whenever saving or retrieving a value, you need to specify the consistency
 * level to use to do so. This parameter defines this consistency level.
 *
 * By default it is set to CONSISTENCY_LEVEL_DEFAULT which means that the
 * system will make use of the default consistency value defined in the
 * QCassandra object.
 *
 * \sa QCassandra::setDefaultConsistencyLevel()
 */

/** \var QCassandraValue::f_value
 * \brief The actual data of this value object.
 *
 * The f_value holds the binary data of the value.
 *
 * If you set the value as an integer or floating point value, it is saved
 * in the buffer in big endian (i.e. sorts correctly!)
 *
 * Saving a QString in a value transforms the string to UTF-8 before
 * saving it.
 */

/** \var QCassandraValue::f_ttl
 * \brief The TTL of this value.
 *
 * The TTL represents the number of seconds this value will be kept in the
 * Cassandra database. For example, a log could be made to disappear
 * automatically after 3 months.
 *
 * The default value is TTL_PERMANENT which means that the value
 * is permanent.
 */

/** \var QCassandraValue::timestamp_mode_t
 * \brief A timestamp mode.
 *
 * The timestamp can be defined in multiple ways. This mode specifies which
 * way you want to use for this value. The same mode is used by the different
 * remove() functions.
 *
 * The available modes are:
 *
 * \li TIMESTAMP_MODE_CASSANDRA -- the Cassandra server defines the timestamp
 * \li TIMESTAMP_MODE_AUTO -- the libQtCassandra library defines the timestamp
 * \li TIMESTAMP_MODE_DEFINED -- the user defined the timestamp
 *
 * \sa QCassandraValue::f_timestamp_mode
 */

/** \var QCassandraValue::f_timestamp_mode
 * \brief The timestamp mode.
 *
 * This variable member defines how the timestamp value is used. It can
 * be set to any one of the timestamp_mode_t values.
 */

/** \var QCassandraValue::f_timestamp
 * \brief The timestamp for this value.
 *
 * This variable member holds the timestamp of this value, however it is
 * used only if the QCassandraValue::f_timestamp_mode is set to
 * CASSANDRA_VALUE_TIMESTAMP. In all other cases it is ignored.
 */

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraRow object to NULL. This is
 * an equivalent to a BINARY with a size of 0.
 */
QCassandraValue::QCassandraValue()
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    // an empty f_value() already represents a NULL
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to true or
 * false.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(bool value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setBoolValue(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object an 8 bits
 * value. The value may be signed or not depending on your compiler
 * settings.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(char value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setCharValue(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to a signed
 * value between -128 and +127.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(signed char value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setSignedCharValue(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to an unsigned
 * value between 0 and 255.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(unsigned char value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setUnsignedCharValue(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to a signed
 * value between -32768 and +32768.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(int16_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setInt16Value(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to an unsigned
 * value between 0 and 65535.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(uint16_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setUInt16Value(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to a signed
 * integer of 32 bits.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(int32_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setInt32Value(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to an unsigned
 * integer of 32 bits.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(uint32_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setUInt32Value(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to a signed
 * integer of 64 bits.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(int64_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setInt64Value(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to an unsigned
 * integer of 64 bits.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(uint64_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setUInt64Value(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to a floating
 * point value defined on 32 bits.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(float value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setFloatValue(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to a floating
 * point value defined on 64 bits.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(double value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    QtCassandra::setDoubleValue(f_value, value);
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to a UTF-8
 * string.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(const QString& value)
    : f_value(value.toUtf8())
      //f_ttl(TTL_PERMANENT) -- auto-init
      //f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
      //f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
      //f_timestamp(0) -- auto-init
{
    // f_value properly initialized already
}

/** \brief Initialize a QCassandraValue object.
 *
 * This function initializes a QCassandraValue object to a buffer.
 *
 * \param[in] value  The start value of the QCassandraValue object.
 */
QCassandraValue::QCassandraValue(const QByteArray& value)
    : f_value(value)
      //f_ttl(TTL_PERMANENT) -- auto-init
      //f_consistency_level(CONSISTENCY_LEVEL_DEFAULT) -- auto-init
      //f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
      //f_timestamp(0) -- auto-init
{
    // f_value properly initialized already
}

/** \brief Make the value empty.
 *
 * This function empties the value buffer.
 */
void QCassandraValue::setNullValue()
{
    QtCassandra::setNullValue(f_value);
}

/** \brief Set the value to the bool parameter.
 *
 * This function copies the specified bool in the value buffer.
 * The bool is saved as 0 or 1.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
void QCassandraValue::setBoolValue(bool value)
{
    QtCassandra::setBoolValue(f_value, value);
}

/** \brief Set the value to the char parameter.
 *
 * This function copies the specified char in the value buffer.
 * The char is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
void QCassandraValue::setCharValue(char value)
{
    QtCassandra::setCharValue(f_value, value);
}

/** \brief Set the value to the signed char parameter.
 *
 * This function copies the specified signed char in the value buffer.
 * The signed char is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
void QCassandraValue::setSignedCharValue(signed char value)
{
    QtCassandra::setSignedCharValue(f_value, value);
}

/** \brief Set the value to the unsigned char parameter.
 *
 * This function copies the specified unsigned char in the value buffer.
 * The unsigned char is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
void QCassandraValue::setUnsignedCharValue(unsigned char value)
{
    QtCassandra::setUnsignedCharValue(f_value, value);
}

/** \brief Set the value to the int16_t parameter.
 *
 * This function copies the specified int16_t in the value buffer.
 * The int16_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
void QCassandraValue::setInt16Value(int16_t value)
{
    QtCassandra::setInt16Value(f_value, value);
}

/** \brief Set the value to the uint16_t parameter.
 *
 * This function copies the specified uint16_t in the value buffer.
 * The uint16_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
void QCassandraValue::setUInt16Value(uint16_t value)
{
    QtCassandra::setUInt16Value(f_value, value);
}

/** \brief Set the value to the int32_t parameter.
 *
 * This function copies the specified int32_t in the value buffer.
 * The int32_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void QCassandraValue::setInt32Value(int32_t value)
{
    QtCassandra::setInt32Value(f_value, value);
}

/** \brief Set the value to the uint32_t parameter.
 *
 * This function copies the specified uint32_t in the value buffer.
 * The uint32_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
void QCassandraValue::setUInt32Value(uint32_t value)
{
    QtCassandra::setUInt32Value(f_value, value);
}

/** \brief Set the value to the int64_t parameter.
 *
 * This function copies the specified int64_t in the value buffer.
 * The int64_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void QCassandraValue::setInt64Value(int64_t value)
{
    QtCassandra::setInt64Value(f_value, value);
}

/** \brief Set the value to the uint64_t parameter.
 *
 * This function copies the specified uint64_t in the value buffer.
 * The uint64_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void QCassandraValue::setUInt64Value(uint64_t value)
{
    QtCassandra::setUInt64Value(f_value, value);
}

/** \brief Set the value to the float parameter.
 *
 * This function copies the specified float in the value buffer.
 * The float is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void QCassandraValue::setFloatValue(float value)
{
    QtCassandra::setFloatValue(f_value, value);
}

/** \brief Set the value to the double parameter.
 *
 * This function copies the specified double in the value buffer.
 * The double is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void QCassandraValue::setDoubleValue(double value)
{
    QtCassandra::setDoubleValue(f_value, value);
}

/** \brief Set the value to the string data.
 *
 * This function copies the specified string data in the value buffer.
 * The string is converted to UTF-8 first.
 *
 * \exception std::runtime_error
 * This exception is raised whenever the input binary buffer is
 * larger than 64Mb. Later we may allow you to change the limit,
 * however, we probably will give you a way to save large data
 * sets using multiple cells instead (i.e. blobs.)
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void QCassandraValue::setStringValue(const QString& value)
{
    QtCassandra::setStringValue(f_value, value);
}

/** \brief Set value to this binary buffer.
 *
 * This function sets the contents of this value object to
 * the specified binary buffer. This is the only case where
 * the input data is saved untouched in the value buffer.
 *
 * \exception std::runtime_error
 * This exception is raised whenever the input binary buffer is
 * larger than 64Mb. Later we may allow you to change the limit,
 * however, we probably will give you a way to save large data
 * sets using multiple cells instead (i.e. blobs.)
 *
 * \param[in] value  The binary buffer to save in this value object.
 */
void QCassandraValue::setBinaryValue(const QByteArray& value)
{
    QtCassandra::setBinaryValue(f_value, value);
}

/** \brief Return the size of the value.
 *
 * This function returns the number of bytes that the value uses.
 *
 * The size can be zero in which case the value is considered to be
 * empty.
 *
 * \warning
 * If the data represents a string, this is the byte size of the
 * buffer and not the number of characters in the string as it
 * is saved in UTF-8 format.
 *
 * \return The size of the value buffer.
 */
int QCassandraValue::size() const
{
    return f_value.size();
}

/** \brief Determine whether this value is empty.
 *
 * This function returns true if the value has no data (i.e. empty buffer.)
 *
 * \return true if the value is an empty buffer, false otherwise.
 */
bool QCassandraValue::nullValue() const
{
    return f_value.size() == 0;
}

/** \brief Retrieve a Boolean from the value.
 *
 * This function is used to retrieve the value in the form of a byte.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with one byte value at the specified index.)
 *
 * \exception std::runtime_error
 * If the buffer is empty, this function raises an exception.
 *
 * \param[in] index  The index where the Boolean byte is read.
 *
 * \return One character from the buffer.
 */
bool QCassandraValue::boolValue(int index) const
{
    return QtCassandra::boolValue(f_value, index);
}

/** \brief Retrieve the specified byte of the value.
 *
 * This function is used to retrieve the value in the form of a byte.
 * Whether the value is signed depends on your compiler.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a one byte value.)
 *
 * \exception std::runtime_error
 * If the buffer is empty, this function raises an exception.
 *
 * \param[in] index  The index where the char is read.
 *
 * \return One character from the buffer.
 */
char QCassandraValue::charValue(int index) const
{
    return QtCassandra::charValue(f_value, index);
}

/** \brief Retrieve the specified byte of the value.
 *
 * This function is used to retrieve the value in the form of a signed byte.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a one byte value.)
 *
 * \exception std::runtime_error
 * If the buffer is empty, this function raises an exception.
 *
 * \param[in] index  The index where the signed char is read.
 *
 * \return One character from the buffer.
 */
signed char QCassandraValue::signedCharValue(int index) const
{
    return QtCassandra::signedCharValue(f_value, index);
}

/** \brief Retrieve the specified byte of the value.
 *
 * This function is used to retrieve the value in the form of an
 * unsigned byte.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a one byte value.)
 *
 * \exception std::runtime_error
 * If the buffer is empty, this function raises an exception.
 *
 * \param[in] index  The index where the unsigned char is read.
 *
 * \return One character from the buffer.
 */
unsigned char QCassandraValue::unsignedCharValue(int index) const
{
    return QtCassandra::unsignedCharValue(f_value, index);
}

/** \brief Retrieve the specified two bytes of the value.
 *
 * This function is used to retrieve the first two bytes of the value
 * in the form of an integer.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a two byte value.)
 *
 * \exception std::runtime_error
 * If the buffer is less than 2 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the signed short is read.
 *
 * \return One character from the buffer.
 */
int16_t QCassandraValue::int16Value(int index) const
{
    return QtCassandra::int16Value(f_value, index);
}

/** \brief Retrieve the specified two bytes of the value.
 *
 * This function is used to retrieve the first two bytes of the value
 * in the form of an unsigned integer.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a two byte value.)
 *
 * \exception std::runtime_error
 * If the buffer is less than 2 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the unsigned short is read.
 *
 * \return One character from the buffer.
 */
uint16_t QCassandraValue::uint16Value(int index) const
{
    return QtCassandra::uint16Value(f_value, index);
}

/** \brief Retrieve the specified four bytes of the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of a signed integer.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a four byte value.)
 *
 * \exception std::runtime_error
 * If the buffer is less than 4 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the 32 bit signed int is read.
 *
 * \return One character from the buffer.
 */
int32_t QCassandraValue::int32Value(int index) const
{
    return QtCassandra::int32Value(f_value, index);
}

/** \brief Retrieve the specified four bytes of the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of an unsigned integer.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a four byte value.)
 *
 * \exception std::runtime_error
 * If the buffer is less than 4 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the 32 bit unsigned int is read.
 *
 * \return One character from the buffer.
 */
uint32_t QCassandraValue::uint32Value(int index) const
{
    return QtCassandra::uint32Value(f_value, index);
}

/** \brief Retrieve the specified eight bytes of the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of a signed integer.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a eight byte value.)
 *
 * \exception std::runtime_error
 * If the buffer is less than 8 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the 64 bit signed int is read.
 *
 * \return One character from the buffer.
 */
int64_t QCassandraValue::int64Value(int index) const
{
    return QtCassandra::int64Value(f_value, index);
}

/** \brief Retrieve the specified eight bytes of the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of an unsigned integer.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a eight byte value.)
 *
 * \exception std::runtime_error
 * If the buffer is less than 8 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the 64 bit unsigned int is read.
 *
 * \return One character from the buffer.
 */
uint64_t QCassandraValue::uint64Value(int index) const
{
    return QtCassandra::uint64Value(f_value, index);
}

/** \brief Retrieve the specified four bytes of the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of a double floating point (32 bits).
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a four byte floating point value.)
 *
 * \exception std::runtime_error
 * If the buffer is less than 4 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the float is read.
 *
 * \return One character from the buffer.
 */
float QCassandraValue::floatValue(int index) const
{
    return QtCassandra::floatValue(f_value, index);
}

/** \brief Retrieve the first eight bytes of the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of a double floating point (64 bits).
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a eight byte floating point value.)
 *
 * \exception std::runtime_error
 * If the buffer is less than 8 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the double is read.
 *
 * \return One character from the buffer.
 */
double QCassandraValue::doubleValue(int index) const
{
    return QtCassandra::doubleValue(f_value, index);
}

/** \brief Get a string from the value.
 *
 * This function retrieves the value as a QString, which means it is
 * taken as UTF-8 data.
 *
 * The index and size should be used with care as using the wrong values
 * will break the validity of the UTF-8 data.
 *
 * If size is set to -1, the rest of the buffer is used.
 *
 * \param[in] index  The index where the string is read.
 * \param[in] size  The number of bytes to retrieve for this string.
 *
 * \return The binary value in the form of a QString that was defined in this value.
 */
QString QCassandraValue::stringValue(int index, int size) const
{
    return QtCassandra::stringValue(f_value, index, size);
}

/** \brief Get a reference to the internal buffer value.
 *
 * This function is used to retrieve a direct reference to the internal buffer.
 * (The reference is constant though!)
 *
 * This means you can get direct access to all the data saved in this value
 * without a copy. The buffer may be empty (null value.)
 *
 * \return The binary value held by this value.
 */
const QByteArray& QCassandraValue::binaryValue() const
{
    return f_value;
}

/** \brief Get a copy to the buffer value.
 *
 * This function is used to retrieve a copy of the buffer or part of the
 * buffer. Since this is a copy, modifying the value object is not visible
 * in the copy.
 *
 * If you do not intend to change the value anyway, the binaryValue()
 * function without parameters may be more efficient.
 *
 * \param[in] index  The index where the array is read.
 * \param[in] size  The number of bytes to retrieve for this array.
 *
 * \return The binary value held by this value.
 */
QByteArray QCassandraValue::binaryValue(int index, int size) const
{
    return QtCassandra::binaryValue(f_value, index, size);
}

/** \brief Make the value empty.
 *
 * This function empties the value buffer. The \p null_value
 * parameter can be set to any value, it is ignored.
 *
 * \param[in] null_value  An ignored pointer (should be set to NULL)
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (const char * /*null_value*/)
{
    QtCassandra::setNullValue(f_value);
    return *this;
}

/** \brief Set the value to the bool parameter.
 *
 * This function copies the specified bool in the value buffer.
 * The bool is saved as 0 or 1.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (bool value)
{
    QtCassandra::setBoolValue(f_value, value);
    return *this;
}

/** \brief Set the value to the char parameter.
 *
 * This function copies the specified char in the value buffer.
 * The char is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (char value)
{
    QtCassandra::setCharValue(f_value, value);
    return *this;
}

/** \brief Set the value to the signed char parameter.
 *
 * This function copies the specified signed char in the value buffer.
 * The signed char is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (signed char value)
{
    QtCassandra::setSignedCharValue(f_value, value);
    return *this;
}

/** \brief Set the value to the unsigned char parameter.
 *
 * This function copies the specified unsigned char in the value buffer.
 * The unsigned char is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (unsigned char value)
{
    QtCassandra::setUnsignedCharValue(f_value, value);
    return *this;
}

/** \brief Set the value to the int16_t parameter.
 *
 * This function copies the specified int16_t in the value buffer.
 * The int16_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (int16_t value)
{
    QtCassandra::setInt16Value(f_value, value);
    return *this;
}

/** \brief Set the value to the uint16_t parameter.
 *
 * This function copies the specified uint16_t in the value buffer.
 * The uint16_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (uint16_t value)
{
    QtCassandra::setUInt16Value(f_value, value);
    return *this;
}

/** \brief Set the value to the int32_t parameter.
 *
 * This function copies the specified int32_t in the value buffer.
 * The int32_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (int32_t value)
{
    QtCassandra::setInt32Value(f_value, value);
    return *this;
}

/** \brief Set the value to the uint32_t parameter.
 *
 * This function copies the specified uint32_t in the value buffer.
 * The uint32_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (uint32_t value)
{
    QtCassandra::setUInt32Value(f_value, value);
    return *this;
}

/** \brief Set the value to the int64_t parameter.
 *
 * This function copies the specified int64_t in the value buffer.
 * The int64_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (int64_t value)
{
    QtCassandra::setInt64Value(f_value, value);
    return *this;
}

/** \brief Set the value to the uint64_t parameter.
 *
 * This function copies the specified uint64_t in the value buffer.
 * The uint64_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (uint64_t value)
{
    QtCassandra::setUInt64Value(f_value, value);
    return *this;
}

/** \brief Set the value to the float parameter.
 *
 * This function copies the specified float in the value buffer.
 * The float is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (float value)
{
    QtCassandra::setFloatValue(f_value, value);
    return *this;
}

/** \brief Set the value to the double parameter.
 *
 * This function copies the specified double in the value buffer.
 * The double is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (double value)
{
    QtCassandra::setDoubleValue(f_value, value);
    return *this;
}

/** \brief Set the value to the string data.
 *
 * This function copies the specified string data in the value buffer.
 * The string is converted to UTF-8 first.
 *
 * \exception std::runtime_error
 * A runtime error is raised if the size of the string is more than
 * 64Mb.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (const QString& value)
{
    QtCassandra::setStringValue(f_value, value);
    return *this;
}

/** \brief Set the value to binary data.
 *
 * This function copies the specified binary data in the value buffer.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
QCassandraValue& QCassandraValue::operator = (const QByteArray& value)
{
    QtCassandra::setBinaryValue(f_value, value);
    return *this;
}

/** \brief Compare this and rhs values for equality.
 *
 * This function returns true if this value and the rhs value are considered
 * equal.
 *
 * The equality takes the value buffer content, the TTL and the consistency
 * level in account. All three must be equal for the function to return true.
 *
 * \param[in] rhs  The value to compare against this value.
 *
 * \return true if both values are considered equal, false otherwise.
 */
bool QCassandraValue::operator == (const QCassandraValue& rhs)
{
    if(f_value.size() != rhs.size()) {
        return false;
    }
    if(f_ttl != rhs.f_ttl) {
        return false;
    }
    if(f_consistency_level != rhs.f_consistency_level) {
        return false;
    }
    return memcmp(f_value.data(), rhs.f_value.data(), f_value.size()) == 0;
}

/** \brief Compare this and rhs values for inequality.
 *
 * This function returns true if this value and the rhs value are considered
 * inequal.
 *
 * The inequality takes the value buffer content, the TTL and the consistency
 * level in account. Any of the three must be inequal for the function to
 * return true.
 *
 * \param[in] rhs  The value to compare against this value.
 *
 * \return true if both values are considered inequal, false otherwise.
 */
bool QCassandraValue::operator != (const QCassandraValue& rhs)
{
    if(f_value.size() != rhs.size()) {
        return true;
    }
    if(f_ttl != rhs.f_ttl) {
        return true;
    }
    if(f_consistency_level != rhs.f_consistency_level) {
        return true;
    }
    return memcmp(f_value.data(), rhs.f_value.data(), f_value.size()) != 0;
}

/** \brief Compare this and rhs values for inequality.
 *
 * This function returns true if this value is considered smaller
 * than the rhs value.
 *
 * The inequality comparison only takes the value buffers in account.
 * All the other parameters are ignored (i.e. TTL, timestamp, and
 * consistency level.)
 *
 * \param[in] rhs  The value to compare against this value.
 *
 * \return true if this value is considered smaller than rhs, false otherwise.
 */
bool QCassandraValue::operator < (const QCassandraValue& rhs)
{
    int sz(std::min(f_value.size(), rhs.size()));
    int r(memcmp(f_value.data(), rhs.f_value.data(), sz));
    if(r != 0) {
        return r < 0;
    }
    if(f_value.size() < rhs.size()) {
        return true;
    }
    return false;
}

/** \brief Compare this and rhs values for inequality.
 *
 * This function returns true if this value is considered smaller
 * than or equal to the rhs value.
 *
 * The inequality comparison only takes the value buffers in account.
 * All the other parameters are ignored (i.e. TTL, timestamp, and
 * consistency level.)
 *
 * \param[in] rhs  The value to compare against this value.
 *
 * \return true if this value is considered smaller than or equal to rhs, false otherwise.
 */
bool QCassandraValue::operator <= (const QCassandraValue& rhs)
{
    int sz(std::min(f_value.size(), rhs.size()));
    int r(memcmp(f_value.data(), rhs.f_value.data(), sz));
    if(r != 0) {
        return r < 0;
    }
    if(f_value.size() <= rhs.size()) {
        return true;
    }
    return false;
}

/** \brief Compare this and rhs values for inequality.
 *
 * This function returns true if this value is considered larger
 * than the rhs value.
 *
 * The inequality comparison only takes the value buffers in account.
 * All the other parameters are ignored (i.e. TTL, timestamp, and
 * consistency level.)
 *
 * \param[in] rhs  The value to compare against this value.
 *
 * \return true if this value is considered larger than rhs, false otherwise.
 */
bool QCassandraValue::operator > (const QCassandraValue& rhs)
{
    int sz(std::min(f_value.size(), rhs.size()));
    int r(memcmp(f_value.data(), rhs.f_value.data(), sz));
    if(r != 0) {
        return r > 0;
    }
    if(f_value.size() > rhs.size()) {
        return true;
    }
    return false;
}

/** \brief Compare this and rhs values for inequality.
 *
 * This function returns true if this value is considered larger
 * than or equal to the rhs value.
 *
 * The inequality comparison only takes the value buffers in account.
 * All the other parameters are ignored (i.e. TTL, timestamp, and
 * consistency level.)
 *
 * \param[in] rhs  The value to compare against this value.
 *
 * \return true if this value is considered larger than or equal to rhs, false otherwise.
 */
bool QCassandraValue::operator >= (const QCassandraValue& rhs)
{
    int sz(std::min(f_value.size(), rhs.size()));
    int r(memcmp(f_value.data(), rhs.f_value.data(), sz));
    if(r != 0) {
        return r > 0;
    }
    if(f_value.size() >= rhs.size()) {
        return true;
    }
    return false;
}

/** \brief Retrieve the current time to live value.
 *
 * This function returns the number of seconds defined as the lifetime of this
 * cell. The time to live is useful to create some temporary data. For example,
 * if you create an index of recent posts, you may want the older posts to
 * automatically be dropped after a given amount of time (i.e. 2 weeks.)
 *
 * This value can be set using the setTtl() function.
 *
 * \return The number of seconds the cell will live.
 */
int32_t QCassandraValue::ttl() const
{
    return f_ttl;
}

/** \brief Set the time to live of this cell.
 *
 * Each cell can be defined as permanent (i.e. TTL not defined, or
 * set to TTL_PERMANENT) or can be defined as temporary.
 *
 * This value represents the number of seconds you want this value
 * to remain in the database.
 *
 * Note that if you want to keep values while running and then
 * lose them, you may want to consider creating a context in
 * memory only (i.e. a context on which you never call the
 * create() function.) Then the TTL is completely ignored, but
 * when you quit your application, the data is gone.
 *
 * \param[in] ttl  The new time to live of this value.
 */
void QCassandraValue::setTtl(int32_t ttl)
{
    if(ttl < 0) {
        throw std::runtime_error("the TTL value cannot be negative");
    }

    f_ttl = ttl;
}

/** \brief Retrieve the current consistency level of this value.
 *
 * This function returns the consistency level of this value. By default
 * it is set to one (CONSISTENCY_LEVEL_ONE.)
 *
 * The consistency level can be set using the setConsistencyLevel() function.
 *
 * \return The consistency level of this value.
 *
 * \sa setConsistencyLevel()
 * \sa QCassandraCell::consistencyLevel()
 */
consistency_level_t QCassandraValue::consistencyLevel() const
{
    return f_consistency_level;
}

/** \brief Define the consistency level of this value.
 *
 * This function defines the consistency level of this value. The level is
 * defined as a static value in the QCassandraValue.
 *
 * Note that this value is mandatory so defining the right value is probably
 * often a good idea. The default is set to one which means the data is only
 * saved on that one cluster you are connected to. One of the best value is
 * QUORUM. The default can be changed in your QCassandra object, set it with
 * your QCassandra::setDefaultConsistencyLevel() function.
 *
 * The available values are:
 *
 * \li CONSISTENCY_LEVEL_ONE
 * \li CONSISTENCY_LEVEL_QUORUM
 * \li CONSISTENCY_LEVEL_LOCAL_QUORUM
 * \li CONSISTENCY_LEVEL_EACH_QUORUM
 * \li CONSISTENCY_LEVEL_ALL
 * \li CONSISTENCY_LEVEL_ANY
 * \li CONSISTENCY_LEVEL_TWO
 * \li CONSISTENCY_LEVEL_THREE
 *
 * The consistency level is probably better explained in the Cassandra
 * documentations that here.
 *
 * \param[in] level  The new consistency level for this cell.
 *
 * \sa consistencyLevel()
 * \sa QCassandra::setDefaultConsistencyLevel()
 * \sa QCassandraCell::setConsistencyLevel()
 */
void QCassandraValue::setConsistencyLevel(consistency_level_t level)
{
    // we cannot use a switch because these are not really
    // constants (i.e. these are pointers to values); although
    // we could cast to the Cassandra definition and switch on
    // those...
    if(level != CONSISTENCY_LEVEL_DEFAULT
    && level != CONSISTENCY_LEVEL_ONE
    && level != CONSISTENCY_LEVEL_QUORUM
    && level != CONSISTENCY_LEVEL_LOCAL_QUORUM
    && level != CONSISTENCY_LEVEL_EACH_QUORUM
    && level != CONSISTENCY_LEVEL_ALL
    && level != CONSISTENCY_LEVEL_ANY
    && level != CONSISTENCY_LEVEL_TWO
    && level != CONSISTENCY_LEVEL_THREE) {
        throw std::runtime_error("invalid consistency level");
    }

    f_consistency_level = level;
}

/** \brief Retrieve the current timestamp mode.
 *
 * This function returns the timestamp mode.
 *
 * Please, see the setTimestamp() function and the different timestamp modes
 * for more information.
 *
 * \return The timestamp mode.
 *
 * \sa setTimestamp()
 * \sa TIMESTAMP_MODE_CASSANDRA
 * \sa TIMESTAMP_MODE_AUTO
 * \sa TIMESTAMP_MODE_DEFINED
 */
QCassandraValue::timestamp_mode_t QCassandraValue::timestampMode() const
{
    return f_timestamp_mode;
}

/** \brief Change the timestamp mode.
 *
 * This function is used to change the timestamp mode to one of:
 *
 * \li TIMESTAMP_MODE_CASSANDRA -- let the Cassandra server define the timestamp
 * \li TIMESTAMP_MODE_AUTO -- let the libQtCassandra library define the timestamp
 *
 * Note that you cannot set the mode to TIMESTAMP_MODE_DEFINED with this
 * function. Instead use the setTimestamp() function which has the side
 * effect of setting the mode to defined.
 *
 * \param[in] mode  The new mode for this value's timestamp.
 *
 * \sa setTimestamp()
 */
void QCassandraValue::setTimestampMode(timestamp_mode_t mode)
{
    // some older version of g++ require a cast here
    switch(static_cast<def_timestamp_mode_t>(mode)) {
    case TIMESTAMP_MODE_CASSANDRA:
    case TIMESTAMP_MODE_AUTO:
        break;

    default:
        throw std::runtime_error("invalid timestamp mode in setTimestampMode()");

    }
    f_timestamp_mode = mode;
}

/** \brief Retrieve the current timestamp of this value.
 *
 * This function returns the timestamp of this value.
 *
 * Please, see the setTimestamp() function for more information.
 *
 * Note that when the timestamp mode is not TIMESTAMP_MODE_DEFINED, this
 * value is not used when saving the QCassandraValue in the database.
 * Otherwise it is set to the last value it was set at (i.e. either the
 * timestamp found in the Cassandra database or the last value you set
 * it to with the setTimestamp() function.)
 *
 * \return The timestamp 64bit value.
 *
 * \sa setTimestamp()
 */
int64_t QCassandraValue::timestamp() const
{
    return f_timestamp;
}

/** \brief Define your own timestamp for this value.
 *
 * The timestamp value is used by the Cassandra server to choose which one of
 * the values to keep. The value with the largest timestamp is kept in the
 * database and all the others are removed.
 *
 * The value can be anything you'd like such as a simple counter or the time
 * of day (see gettimeofday(2)).
 *
 * If you use the time of day, you must make sure that all the servers that
 * run your software have the same time (i.e. use ntpd or an equivalent.)
 *
 * By default (TIMESTAMP_MODE_CASSANDRA), we let the Cassandra server define
 * the timestamp for us.
 *
 * The special mode CASSANDRA_VALUE_AUTO_TIMESTAMP can be used to
 * request the QtCassandra library to use the gettimeofday() function to
 * determine the timestamp with as much precision as that function offers
 * (we send the seconds and microseconds.) This is done at the time the
 * value is saved (each time.)
 *
 * In any event, changing the timestamp en route is not a good idea. You may
 * lose data doing so. So make sure that a specific cell always get the same
 * timestamp definition.
 *
 * Note that if you start with a timestamp of 1 and increase it by 1 each
 * time you write the value, it works just the same (assuming that two
 * nodes cannot write to the database at the same time...)
 *
 * \note
 * This call automatically sets the mode to TIMESTAMP_MODE_DEFINED.
 *
 * \note
 * When a QCassandraValue is copied around in memory it completely ignores
 * the timestamp. In other words, you can copy an old value over a new
 * value and it will overwrite the new value.
 *
 * \param[in] timestamp  The time used to mark this value.
 */
void QCassandraValue::setTimestamp(int64_t timestamp)
{
    f_timestamp_mode = TIMESTAMP_MODE_DEFINED;
    f_timestamp = timestamp;
}

/** \brief Assign a new timestamp to this value.
 *
 * This function assigns a new timestamp to this value without changing
 * the timestamp mode. This is used internally whenever a value is read
 * from the Cassandra server.
 *
 * \param[in] timestamp  The new value timestamp.
 */
void QCassandraValue::assignTimestamp(int64_t timestamp)
{
    f_timestamp = timestamp;
}

} // namespace QtCassandra
// vim: ts=4 sw=4 et
