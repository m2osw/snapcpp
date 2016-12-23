/*
 * Text:
 *      Value.cpp
 *
 * Description:
 *      Handling of a cell value.
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

#include "cassvalue/value.h"

#include <execinfo.h>
#include <stdint.h>

namespace cassvalue
{

/** \class Value
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
 * avoided. On the other hand, many times just the Value
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
 * \brief Verify that a given size is compatible with cassvalue.
 *
 * Append the 8 bit unsigned byte \p value to the byte \p array.
 * This function keeps all the existing data in place. The result must
 * fit in the destination array as defined by BUFFER_MAX_SIZE.
 *
 * \param[in,out] array  The array where the data is to be appended.
 * \param[in] value  The value to append to the array of data.
 */

/** \fn void checkBufferSize(uint64_t new_size);
 * \brief Verify that a given size is compatible with cassvalue.
 *
 * This function checks the new size of the buffer against the maximum
 * size that the buffer accepts. If the new size is too large, it
 * raises an exception which in effect prevents the set or append
 * function from happening.
 *
 * If you want to check the size beforehand and avoid an exception,
 * you can compute the new size of your buffer and then check it
 * against cassvalue::BUFFER_MAX_SIZE
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

/** \var Value::TTL_PERMANENT
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

/** \typedef Value::cassandra_ttl_t
 * \brief A safe TTL variable type.
 *
 * This definition is used to handle the TTL of a cell and is defined
 * in the value of each cell.
 *
 * The type forces the TTL value to TTL_PERMANENT by default. It also
 * bounds the value to positive numbers (plus 0 as TTL_PERMANENT is zero.)
 */

/** \enum Value::def_timestamp_mode_t
 * \brief Intermediate definition to support a safe timestamp mode.
 *
 * The intermediate definition of the timestamp is used to declare a
 * limited controlled variable that has bounds checked automatically
 * and initializes variables of type timestamp_mode_t to
 * TIMESTAMP_MODE_AUTO.
 */

/** \var Value::f_value
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

/** \brief Initialize a Value object.
 *
 * This function initializes a object to NULL. This is
 * an equivalent to a BINARY with a size of 0.
 */
Value::Value()
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    // an empty f_value() already represents a NULL
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to true or
 * false.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(bool value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setBoolValue(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object an 8 bits
 * value. The value may be signed or not depending on your compiler
 * settings.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(char value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setCharValue(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to a signed
 * value between -128 and +127.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(signed char value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setSignedCharValue(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to an unsigned
 * value between 0 and 255.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(unsigned char value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setUnsignedCharValue(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to a signed
 * value between -32768 and +32768.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(int16_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setInt16Value(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to an unsigned
 * value between 0 and 65535.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(uint16_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setUInt16Value(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to a signed
 * integer of 32 bits.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(int32_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setInt32Value(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to an unsigned
 * integer of 32 bits.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(uint32_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setUInt32Value(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to a signed
 * integer of 64 bits.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(int64_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setInt64Value(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to an unsigned
 * integer of 64 bits.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(uint64_t value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setUInt64Value(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to a floating
 * point value defined on 32 bits.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(float value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setFloatValue(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to a floating
 * point value defined on 64 bits.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(double value)
    //: f_value() -- auto-init
    //  f_ttl(TTL_PERMANENT) -- auto-init
    //  f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
    //  f_timestamp(0) -- auto-init
{
    cassvalue::setDoubleValue(f_value, value);
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to a UTF-8
 * string.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(const QString& value)
    : f_value(value.toUtf8())
      //f_ttl(TTL_PERMANENT) -- auto-init
      //f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
      //f_timestamp(0) -- auto-init
{
    // f_value properly initialized already
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to a buffer.
 *
 * \param[in] value  The start value of the Value object.
 */
Value::Value(const QByteArray& value)
    : f_value(value)
      //f_ttl(TTL_PERMANENT) -- auto-init
      //f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
      //f_timestamp(0) -- auto-init
{
    // f_value properly initialized already
}

/** \brief Initialize a Value object.
 *
 * This function initializes a Value object to a buffer.
 *
 * \param[in] data  The start value of the value object.
 * \param[in] data_size  The number of bytes to copy from the data buffer.
 */
Value::Value(const char *data, int data_size)
    : f_value(data, data_size)
      //f_ttl(TTL_PERMANENT) -- auto-init
      //f_timestamp_mode(TIMESTAMP_MODE_AUTO) -- auto-init
      //f_timestamp(0) -- auto-init
{
    // f_value properly initialized already
}

/** \brief Make the value empty.
 *
 * This function empties the value buffer.
 */
void Value::setNullValue()
{
    cassvalue::setNullValue(f_value);
}

/** \brief Set the value to the bool parameter.
 *
 * This function copies the specified bool in the value buffer.
 * The bool is saved as 0 or 1.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this value.
 */
void Value::setBoolValue(bool value)
{
    cassvalue::setBoolValue(f_value, value);
}

/** \brief Set the value to the char parameter.
 *
 * This function copies the specified char in the value buffer.
 * The char is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this value.
 */
void Value::setCharValue(char value)
{
    cassvalue::setCharValue(f_value, value);
}

/** \brief Set the value to the signed char parameter.
 *
 * This function copies the specified signed char in the value buffer.
 * The signed char is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this value.
 */
void Value::setSignedCharValue(signed char value)
{
    cassvalue::setSignedCharValue(f_value, value);
}

/** \brief Set the value to the unsigned char parameter.
 *
 * This function copies the specified unsigned char in the value buffer.
 * The unsigned char is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this value.
 */
void Value::setUnsignedCharValue(unsigned char value)
{
    cassvalue::setUnsignedCharValue(f_value, value);
}

/** \brief Set the value to the int16_t parameter.
 *
 * This function copies the specified int16_t in the value buffer.
 * The int16_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this value.
 */
void Value::setInt16Value(int16_t value)
{
    cassvalue::setInt16Value(f_value, value);
}

/** \brief Set the value to the uint16_t parameter.
 *
 * This function copies the specified uint16_t in the value buffer.
 * The uint16_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this value.
 */
void Value::setUInt16Value(uint16_t value)
{
    cassvalue::setUInt16Value(f_value, value);
}

/** \brief Set the value to the int32_t parameter.
 *
 * This function copies the specified int32_t in the value buffer.
 * The int32_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void Value::setInt32Value(int32_t value)
{
    cassvalue::setInt32Value(f_value, value);
}

/** \brief Set the value to the uint32_t parameter.
 *
 * This function copies the specified uint32_t in the value buffer.
 * The uint32_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this value.
 */
void Value::setUInt32Value(uint32_t value)
{
    cassvalue::setUInt32Value(f_value, value);
}

/** \brief Set the value to the int64_t parameter.
 *
 * This function copies the specified int64_t in the value buffer.
 * The int64_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void Value::setInt64Value(int64_t value)
{
    cassvalue::setInt64Value(f_value, value);
}

/** \brief Set the value to the uint64_t parameter.
 *
 * This function copies the specified uint64_t in the value buffer.
 * The uint64_t is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void Value::setUInt64Value(uint64_t value)
{
    cassvalue::setUInt64Value(f_value, value);
}

/** \brief Set the value to the float parameter.
 *
 * This function copies the specified float in the value buffer.
 * The float is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void Value::setFloatValue(float value)
{
    cassvalue::setFloatValue(f_value, value);
}

/** \brief Set the value to the double parameter.
 *
 * This function copies the specified double in the value buffer.
 * The double is saved in big endian.
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void Value::setDoubleValue(double value)
{
    cassvalue::setDoubleValue(f_value, value);
}

/** \brief Set the value to the string data.
 *
 * This function copies the specified string data in the value buffer.
 * The string is converted to UTF-8 first.
 *
 * This exception is raised whenever the input binary buffer is
 * larger than 64Mb. Later we may allow you to change the limit,
 * however, we probably will give you a way to save large data
 * sets using multiple cells instead (i.e. blobs.)
 *
 * \param[in] value  The new value to copy in this value buffer.
 */
void Value::setStringValue(const QString& value)
{
    cassvalue::setStringValue(f_value, value);
}

/** \brief Set value to this binary buffer.
 *
 * This function sets the contents of this value object to
 * the specified binary buffer. This is the only case where
 * the input data is saved untouched in the value buffer.
 *
 * \exception exception_t
 * This exception is raised whenever the input binary buffer is
 * larger than 64Mb. Later we may allow you to change the limit,
 * however, we probably will give you a way to save large data
 * sets using multiple cells instead (i.e. blobs.)
 *
 * \param[in] value  The binary buffer to save in this value object.
 */
void Value::setBinaryValue(const QByteArray& value)
{
    cassvalue::setBinaryValue(f_value, value);
}

/** \brief Set value to this binary buffer.
 *
 * This function sets the contents of this value object to
 * the specified binary buffer. This is the only case where
 * the input data is saved untouched in the value buffer.
 *
 * \exception exception_t
 * This exception is raised whenever the input binary buffer is
 * larger than 64Mb. Later we may allow you to change the limit,
 * however, we probably will give you a way to save large data
 * sets using multiple cells instead (i.e. blobs.)
 *
 * \param[in] data  The binary data to save in this value object.
 * \param[in] data_size  The size of the buffer.
 */
void Value::setBinaryValue(const char *data, int data_size)
{
    f_value = QByteArray(data, data_size);
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
int Value::size() const
{
    return f_value.size();
}

/** \brief Determine whether this value is empty.
 *
 * This function returns true if the value has no data (i.e. empty buffer.)
 *
 * \return true if the value is an empty buffer, false otherwise.
 */
bool Value::nullValue() const
{
    return f_value.size() == 0;
}

/** \brief Retrieve a Boolean from the value.
 *
 * This function is used to retrieve the value in the form of a bool.
 *
 * It is assumed that you know what you are doing (i.e. that you created
 * this cell with one byte value at the specified index.)
 *
 * \exception exception_t
 * If the buffer is empty, this function raises an exception.
 *
 * \param[in] index  The index where the Boolean byte is read.
 *
 * \return One bool from the buffer.
 */
bool Value::boolValue(int index) const
{
    return cassvalue::boolValue(f_value, index);
}

/** \brief Retrieve a Boolean from the value.
 *
 * This function is used to retrieve the value in the form of a bool.
 *
 * The function returns the default value (which is 'false' by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the boolValue() function, this function does not throw if
 * no data is available.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One bool from the buffer.
 */
bool Value::boolValueOrNull(int index, const bool default_value) const
{
    return cassvalue::boolValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve a Boolean from the value.
 *
 * This function is used to retrieve the value in the form of a bool.
 *
 * The function returns the default value (which is 'false' by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the boolValue() function, this function does not throw if
 * no data is available.
 *
 * \note
 * For a Boolean value, since it is just 1 byte, this function has the
 * same effect as the boolValueOrNull(). It is available for
 * completeness of this class.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One bool from the buffer.
 */
bool Value::safeBoolValue(int index, const bool default_value) const
{
    return cassvalue::boolValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified byte of the value.
 *
 * This function is used to retrieve the value in the form of a byte.
 * Whether the value is signed depends on your compiler.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a one byte value.)
 *
 * \exception exception_t
 * If the buffer is empty, this function raises an exception.
 *
 * \param[in] index  The index where the char is read.
 *
 * \return One character from the buffer.
 */
char Value::charValue(int index) const
{
    return cassvalue::charValue(f_value, index);
}

/** \brief Retrieve the specified byte from the value.
 *
 * This function is used to retrieve the value in the form of a byte.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the charValue() function, this function does not throw if
 * no data is available.
 *
 * \param[in] index  The index where the byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One character from the buffer.
 */
char Value::charValueOrNull(int index, const char default_value) const
{
    return cassvalue::charValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve a Boolean from the value.
 *
 * This function is used to retrieve the value in the form of a byte.
 *
 * The function returns the default value (which is 'false' by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the charValue() function, this function does not throw if
 * no data is available.
 *
 * \note
 * For a char value, since it is just 1 byte, this function has the
 * same effect as the charValueOrNull(). It is available for
 * completeness of this class.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One character from the buffer.
 */
char Value::safeCharValue(int index, const char default_value) const
{
    return cassvalue::charValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified byte of the value.
 *
 * This function is used to retrieve the value in the form of a signed byte.
 *
 * It is assumed that you know what you are doing (i.e. that you created
 * this cell with a one byte value.)
 *
 * \exception exception_t
 * If the buffer is empty, this function raises an exception.
 *
 * \param[in] index  The index where the signed char is read.
 *
 * \return One character from the buffer.
 */
signed char Value::signedCharValue(int index) const
{
    return cassvalue::signedCharValue(f_value, index);
}

/** \brief Retrieve the specified byte from the value.
 *
 * This function is used to retrieve the value in the form of a byte.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the charValue() function, this function does not throw if
 * no data is available.
 *
 * \param[in] index  The index where the byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One character from the buffer.
 */
signed char Value::signedCharValueOrNull(int index, const signed char default_value) const
{
    return cassvalue::signedCharValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve a Boolean from the value.
 *
 * This function is used to retrieve the value in the form of a byte.
 *
 * The function returns the default value (which is 'false' by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the charValue() function, this function does not throw if
 * no data is available.
 *
 * \note
 * For a char value, since it is just 1 byte, this function has the
 * same effect as the signedCharValueOrNull(). It is available for
 * completeness of this class.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One character from the buffer.
 */
signed char Value::safeSignedCharValue(int index, const signed char default_value) const
{
    return cassvalue::signedCharValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified byte of the value.
 *
 * This function is used to retrieve the value in the form of an
 * unsigned byte.
 *
 * It is assumed that you know what you are doing (i.e. that you created
 * this cell with a one byte value.)
 *
 * \exception exception_t
 * If the buffer is empty, this function raises an exception.
 *
 * \param[in] index  The index where the unsigned char is read.
 *
 * \return One character from the buffer.
 */
unsigned char Value::unsignedCharValue(int index) const
{
    return cassvalue::unsignedCharValue(f_value, index);
}

/** \brief Retrieve the specified byte from the value.
 *
 * This function is used to retrieve the value in the form of a byte.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the unsignedCharValue() function, this function does not
 * throw if no data is available.
 *
 * \param[in] index  The index where the byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One character from the buffer.
 */
unsigned char Value::unsignedCharValueOrNull(int index, const unsigned char default_value) const
{
    return cassvalue::unsignedCharValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve a Boolean from the value.
 *
 * This function is used to retrieve the value in the form of a byte.
 *
 * The function returns the default value (which is 'false' by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the unsignedCharValue() function, this function does not
 * throw if no data is available.
 *
 * \note
 * For an unsigned char value, since it is just 1 byte, this function has
 * the same effect as the unsignedCharValueOrNull(). It is available for
 * completeness of this class.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One character from the buffer.
 */
unsigned char Value::safeUnsignedCharValue(int index, const unsigned char default_value) const
{
    return cassvalue::unsignedCharValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified two bytes from the value.
 *
 * This function is used to retrieve the first two bytes of the value
 * in the form of an integer.
 *
 * It is assumed that you know what you are doing (i.e. that you created
 * this cell with a two byte value.)
 *
 * \exception exception_t
 * If the buffer is less than 2 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the signed short is read.
 *
 * \return Two bytes from the buffer (big endian).
 */
int16_t Value::int16Value(int index) const
{
    return cassvalue::int16Value(f_value, index);
}

/** \brief Retrieve the specified two bytes from the value.
 *
 * This function is used to retrieve the first two bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the int16Value() function, this function does not
 * throw if no data is available. However, it still throws if there
 * is only 1 byte of data since 2 bytes are required.
 *
 * \exception exception_t
 * If the buffer is only 1 byte, this function raises this exception.
 *
 * \param[in] index  The index where the byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Two bytes from the buffer (big endian).
 */
int16_t Value::int16ValueOrNull(int index, const int16_t default_value) const
{
    return cassvalue::int16ValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified two bytes from the value.
 *
 * This function is used to retrieve the first two bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index + 1 >= size).
 *
 * Contrary to the int16Value() and the int16ValueOrNull() functions,
 * this function does not throw if not enough data is available.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Two bytes from the buffer (big endian).
 */
int16_t Value::safeInt16Value(int index, const int16_t default_value) const
{
    return cassvalue::safeInt16Value(f_value, index, default_value);
}

/** \brief Retrieve the specified two bytes of the value.
 *
 * This function is used to retrieve the first two bytes of the value
 * in the form of an unsigned integer.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a two byte value.)
 *
 * \exception exception_t
 * If the buffer is less than 2 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the unsigned short is read.
 *
 * \return One character from the buffer.
 */
uint16_t Value::uint16Value(int index) const
{
    return cassvalue::uint16Value(f_value, index);
}

/** \brief Retrieve the specified two bytes from the value.
 *
 * This function is used to retrieve the first two bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the uint16Value() function, this function does not
 * throw if no data is available. However, it still throws if there
 * is only 1 byte of data since 2 bytes are required.
 *
 * \exception exception_t
 * If the buffer is only 1 byte, this function raises this exception.
 *
 * \param[in] index  The index where the byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Two bytes from the buffer (big endian).
 */
uint16_t Value::uint16ValueOrNull(int index, const uint16_t default_value) const
{
    return cassvalue::uint16ValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified two bytes from the value.
 *
 * This function is used to retrieve the first two bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index + 1 >= size).
 *
 * Contrary to the uint16Value() and the uint16ValueOrNull() functions,
 * this function does not throw if not enough data is available.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Two bytes from the buffer (big endian).
 */
uint16_t Value::safeUInt16Value(int index, const uint16_t default_value) const
{
    return cassvalue::safeUInt16Value(f_value, index, default_value);
}

/** \brief Retrieve the specified four bytes of the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of a signed integer.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a four byte value.)
 *
 * \exception exception_t
 * If the buffer is less than 4 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the 32 bit signed int is read.
 *
 * \return One character from the buffer.
 */
int32_t Value::int32Value(int index) const
{
    return cassvalue::int32Value(f_value, index);
}

/** \brief Retrieve the specified four bytes from the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the int32Value() function, this function does not
 * throw if no data is available. However, it still throws if there
 * is only 1, 2, or 3 bytes of data since 4 bytes are required.
 *
 * \exception exception_t
 * If the buffer is only 1, 2, or 3 bytes, this function raises this
 * exception.
 *
 * \param[in] index  The index where the byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Four bytes from the buffer (big endian).
 */
int32_t Value::int32ValueOrNull(int index, const int32_t default_value) const
{
    return cassvalue::int32ValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified four bytes from the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index + 3 >= size).
 *
 * Contrary to the int32Value() and the int32ValueOrNull() functions,
 * this function does not throw if not enough data is available.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Four bytes from the buffer (big endian).
 */
int32_t Value::safeInt32Value(int index, const int32_t default_value) const
{
    return cassvalue::safeInt32Value(f_value, index, default_value);
}

/** \brief Retrieve the specified four bytes of the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of an unsigned integer.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a four byte value.)
 *
 * \exception exception_t
 * If the buffer is less than 4 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the 32 bit unsigned int is read.
 *
 * \return One character from the buffer.
 */
uint32_t Value::uint32Value(int index) const
{
    return cassvalue::uint32Value(f_value, index);
}

/** \brief Retrieve the specified four bytes from the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the uint32Value() function, this function does not
 * throw if no data is available. However, it still throws if there
 * is only 1, 2, or 3 bytes of data since 4 bytes are required.
 *
 * \exception exception_t
 * If the buffer is only 1, 2, or 3 bytes, this function raises this
 * exception.
 *
 * \param[in] index  The index where the byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Four bytes from the buffer (big endian).
 */
uint32_t Value::uint32ValueOrNull(int index, const uint32_t default_value) const
{
    return cassvalue::uint32ValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified four bytes from the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index + 3 >= size).
 *
 * Contrary to the uint32Value() and the uint32ValueOrNull() functions,
 * this function does not throw if not enough data is available.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Four bytes from the buffer (big endian).
 */
uint32_t Value::safeUInt32Value(int index, const uint32_t default_value) const
{
    return cassvalue::safeUInt32Value(f_value, index, default_value);
}

/** \brief Retrieve the specified eight bytes of the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of a signed integer.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a eight byte value.)
 *
 * \exception exception_t
 * If the buffer is less than 8 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the 64 bit signed int is read.
 *
 * \return One character from the buffer.
 */
int64_t Value::int64Value(int index) const
{
    return cassvalue::int64Value(f_value, index);
}

/** \brief Retrieve the specified eight bytes from the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the int64Value() function, this function does not
 * throw if no data is available. However, it still throws if there
 * is only 1 to 7 bytes of data since 8 bytes are required.
 *
 * \exception exception_t
 * If the buffer is only 1 to 7 bytes, this function raises this
 * exception.
 *
 * \param[in] index  The index where the byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Eight bytes from the buffer (big endian).
 */
int64_t Value::int64ValueOrNull(int index, const int64_t default_value) const
{
    return cassvalue::int64ValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified eight bytes from the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index + 7 >= size).
 *
 * Contrary to the int64Value() and the int64ValueOrNull() functions,
 * this function does not throw if not enough data is available.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Eight bytes from the buffer (big endian).
 */
int64_t Value::safeInt64Value(int index, const int64_t default_value) const
{
    return cassvalue::safeInt64Value(f_value, index, default_value);
}

/** \brief Retrieve the specified eight bytes of the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of an unsigned integer.
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a eight byte value.)
 *
 * \exception exception_t
 * If the buffer is less than 8 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the 64 bit unsigned int is read.
 *
 * \return One character from the buffer.
 */
uint64_t Value::uint64Value(int index) const
{
    return cassvalue::uint64Value(f_value, index);
}

/** \brief Retrieve the specified eight bytes from the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the uint64Value() function, this function does not
 * throw if no data is available. However, it still throws if there
 * is only 1 to 7 bytes of data since 8 bytes are required.
 *
 * \exception exception_t
 * If the buffer is only 1 to 7 bytes, this function raises this
 * exception.
 *
 * \param[in] index  The index where the byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Eight bytes from the buffer (big endian).
 */
uint64_t Value::uint64ValueOrNull(int index, const uint64_t default_value) const
{
    return cassvalue::uint64ValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified eight bytes from the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of an integer.
 *
 * The function returns the default value (which is 0 by default)
 * if this value is not defined (i.e. index + 7 >= size).
 *
 * Contrary to the uint64Value() and the uint64ValueOrNull() functions,
 * this function does not throw if not enough data is available.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return Eight bytes from the buffer (big endian).
 */
uint64_t Value::safeUInt64Value(int index, const uint64_t default_value) const
{
    return cassvalue::safeUInt64Value(f_value, index, default_value);
}

/** \brief Retrieve the specified four bytes of the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of a double floating point (32 bits).
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a four byte floating point value.)
 *
 * \exception exception_t
 * If the buffer is less than 4 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the float is read.
 *
 * \return One character from the buffer.
 */
float Value::floatValue(int index) const
{
    return cassvalue::floatValue(f_value, index);
}

/** \brief Retrieve the specified four bytes from the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of a float.
 *
 * The function returns the default value (which is 0.0f by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the floatValue() function, this function does not
 * throw if no data is available. However, it still throws if there
 * is only 1 to 3 bytes of data since 4 bytes are required.
 *
 * \exception exception_t
 * If the buffer is only 1 to 3 bytes, this function raises this
 * exception.
 *
 * \param[in] index  The index where the byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One float from the buffer (big endian).
 */
float Value::floatValueOrNull(int index, const float default_value) const
{
    return cassvalue::floatValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified four bytes from the value.
 *
 * This function is used to retrieve the first four bytes of the value
 * in the form of a float.
 *
 * The function returns the default value (which is 0.0f by default)
 * if this value is not defined (i.e. index + 3 >= size).
 *
 * Contrary to the floatValue() and the floatValueOrNull() functions,
 * this function does not throw if not enough data is available.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One float from the buffer (big endian).
 */
float Value::safeFloatValue(int index, const float default_value) const
{
    return cassvalue::safeFloatValue(f_value, index, default_value);
}

/** \brief Retrieve the first eight bytes of the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of a double floating point (64 bits).
 *
 * It is assumed that you know what you're doing (i.e. that you created
 * this cell with a eight byte floating point value.)
 *
 * \exception exception_t
 * If the buffer is less than 8 bytes, this function raises an exception.
 *
 * \param[in] index  The index where the double is read.
 *
 * \return One character from the buffer.
 */
double Value::doubleValue(int index) const
{
    return cassvalue::doubleValue(f_value, index);
}

/** \brief Retrieve the specified eight bytes from the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of a double.
 *
 * The function returns the default value (which is 0.0 by default)
 * if this value is not defined (i.e. index >= size).
 *
 * Contrary to the doubleValue() function, this function does not
 * throw if no data is available. However, it still throws if there
 * is only 1 to 7 bytes of data since 8 bytes are required.
 *
 * \exception exception_t
 * If the buffer is only 1 to 7 bytes, this function raises this
 * exception.
 *
 * \param[in] index  The index where the byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One double from the buffer (big endian).
 */
double Value::doubleValueOrNull(int index, const double default_value) const
{
    return cassvalue::doubleValueOrNull(f_value, index, default_value);
}

/** \brief Retrieve the specified eight bytes from the value.
 *
 * This function is used to retrieve the first eight bytes of the value
 * in the form of a double.
 *
 * The function returns the default value (which is 0.0 by default)
 * if this value is not defined (i.e. index + 7 >= size).
 *
 * Contrary to the doubleValue() and the doubleValueOrNull() functions,
 * this function does not throw if not enough data is available.
 *
 * \param[in] index  The index where the Boolean byte is read.
 * \param[in] default_value  The default value if no data is defined at
 *                           that index.
 *
 * \return One double from the buffer (big endian).
 */
double Value::safeDoubleValue(int index, const double default_value) const
{
    return cassvalue::safeDoubleValue(f_value, index, default_value);
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
QString Value::stringValue(int index, int the_size) const
{
    return cassvalue::stringValue(f_value, index, the_size);
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
const QByteArray& Value::binaryValue() const
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
QByteArray Value::binaryValue(int index, int the_size) const
{
    return cassvalue::binaryValue(f_value, index, the_size);
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
Value& Value::operator = (const char * /*null_value*/)
{
    cassvalue::setNullValue(f_value);
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
Value& Value::operator = (bool value)
{
    cassvalue::setBoolValue(f_value, value);
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
Value& Value::operator = (char value)
{
    cassvalue::setCharValue(f_value, value);
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
Value& Value::operator = (signed char value)
{
    cassvalue::setSignedCharValue(f_value, value);
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
Value& Value::operator = (unsigned char value)
{
    cassvalue::setUnsignedCharValue(f_value, value);
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
Value& Value::operator = (int16_t value)
{
    cassvalue::setInt16Value(f_value, value);
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
Value& Value::operator = (uint16_t value)
{
    cassvalue::setUInt16Value(f_value, value);
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
Value& Value::operator = (int32_t value)
{
    cassvalue::setInt32Value(f_value, value);
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
Value& Value::operator = (uint32_t value)
{
    cassvalue::setUInt32Value(f_value, value);
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
Value& Value::operator = (int64_t value)
{
    cassvalue::setInt64Value(f_value, value);
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
Value& Value::operator = (uint64_t value)
{
    cassvalue::setUInt64Value(f_value, value);
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
Value& Value::operator = (float value)
{
    cassvalue::setFloatValue(f_value, value);
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
Value& Value::operator = (double value)
{
    cassvalue::setDoubleValue(f_value, value);
    return *this;
}

/** \brief Set the value to the string data.
 *
 * This function copies the specified string data in the value buffer.
 * The string is converted to UTF-8 first.
 *
 * \exception exception_t
 * A runtime error is raised if the size of the string is more than
 * 64Mb.
 *
 * \param[in] value  The new value to copy in this value buffer.
 *
 * \return A reference to this QCassandra value.
 */
Value& Value::operator = (const QString& value)
{
    cassvalue::setStringValue(f_value, value);
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
Value& Value::operator = (const QByteArray& value)
{
    cassvalue::setBinaryValue(f_value, value);
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
bool Value::operator == (const Value& rhs)
{
    if(f_value.size() != rhs.size()) {
        return false;
    }
    if(f_ttl != rhs.f_ttl) {
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
bool Value::operator != (const Value& rhs)
{
    if(f_value.size() != rhs.size()) {
        return true;
    }
    if(f_ttl != rhs.f_ttl) {
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
bool Value::operator < (const Value& rhs)
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
bool Value::operator <= (const Value& rhs)
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
bool Value::operator > (const Value& rhs)
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
bool Value::operator >= (const Value& rhs)
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


} // namespace cassvalue
// vim: ts=4 sw=4 et
