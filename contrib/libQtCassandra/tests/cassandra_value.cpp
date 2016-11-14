/*
 * Text:
 *      cassandra_value.cpp
 *
 * Description:
 *      Test the QCassandraValue object for validity to ensure that the
 *      read matches the write and vice versa. This tests the value by
 *      itself and within a row of a database.
 *
 * Documentation:
 *      Run with no options, although supports the -h to define
 *      Cassandra's host.
 *      Fails if the test cannot connect to the default Cassandra cluster
 *      or if any value cannot be read back the way it was written.
 *
 * License:
 *      Copyright (c) 2013-2016 Made to Order Software Corp.
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

#include <QtCassandra/QCassandra.h>
#include <QtCore/QDebug>

union float_switch_t {
    float f;
    uint32_t v;
};
union double_switch_t {
    double f;
    uint64_t v;
};

uint64_t my_rand()
{
    return
        (static_cast<uint64_t>(rand() & 255) << 56)
      | (static_cast<uint64_t>(rand() & 255) << 48)
      | (static_cast<uint64_t>(rand() & 255) << 40)
      | (static_cast<uint64_t>(rand() & 255) << 32)
      | (static_cast<uint64_t>(rand() & 255) << 24)
      | (static_cast<uint64_t>(rand() & 255) << 16)
      | (static_cast<uint64_t>(rand() & 255) <<  8)
      | (static_cast<uint64_t>(rand() & 255) <<  0)
    ;
}

QString cleanString(const QString& str)
{
    QString result;

    for(int i(0); i < str.length(); ++i) {
        ushort u(str[i].unicode());
        switch(u) {
        case '\0': result += "\\0"; break;
        case '\a': result += "\\a"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        case '\v': result += "\\v"; break;
        case '\\': result += "\\\\"; break;
        case 127: result += "<DEL>"; break;
        default:
            if(u < ' ' || (u >= 0x80 && u <= 0x9F)) {
                result += QString("\\x%1").arg(u, 2, 16, QChar('0'));
            }
            else if(u > 255) {
                result += QString("U+%1").arg(u, 4, 16, QChar('0'));
            }
            else {
                result += str[i];
            }
            break;

        }
    }

    return result;
}


int main(int argc, char *argv[])
{
    // number of errors
    int err(0);

    try
    {
        float_switch_t float_int;
        double_switch_t double_int;
        QtCassandra::QCassandraValue value;

        // always somewhat different "random" values
        srand(time(NULL));

        QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );
        qDebug() << "+ libQtCassandra version" << cassandra->version();

        const char *host("localhost");
        for(int i(1); i < argc; ++i) {
            if(strcmp(argv[i], "--help") == 0) {
                qDebug() << "Usage:" << argv[0] << "[-h <hostname>]";
                exit(1);
            }
            if(strcmp(argv[i], "-h") == 0) {
                ++i;
                if(i >= argc) {
                    qDebug() << "error: -h must be followed by a hostname.";
                    exit(1);
                }
                host = argv[i];
            }
        }

        // before we connect we can test the in memory object validity

        // max. size really limited by Cassandra
        uint64_t const BUFFER_MAX_SIZE( QtCassandra::getBufferMaxSize() );
        qDebug() << "+ Testing size limit";
        if(BUFFER_MAX_SIZE > 0x80000000) {
            qDebug() << "error: the size of a Cassandra's cell is limited to 2Gb";
            ++err;
        }

        for(uint64_t i(0); i <= BUFFER_MAX_SIZE; ++i) {
            try {
                QtCassandra::checkBufferSize(i);
            }
            catch(const std::runtime_error&) {
                qDebug() << "error: checkBufferSize() generated an error with a valid size";
                ++err;
            }
        }

        for(int64_t i(-1024); i < 0; ++i) {
            try {
                QtCassandra::checkBufferSize(i);
                qDebug() << "error: checkBufferSize() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
        }

        for(uint64_t i(BUFFER_MAX_SIZE + 1); i <= BUFFER_MAX_SIZE + 1024; ++i) {
            try {
                QtCassandra::checkBufferSize(i);
                qDebug() << "error: checkBufferSize() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
        }

        qDebug() << "+ Testing arrays";

        // testing all the array set/get/read functions with one/two/three values (no mix yet)
        qDebug() << "++ Empty array";
        QByteArray array;
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }
        try {
            QtCassandra::boolValue(array);
            qDebug() << "error: boolValue() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        try {
            QtCassandra::charValue(array);
            qDebug() << "error: charValue() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        try {
            QtCassandra::signedCharValue(array);
            qDebug() << "error: signedCharValue() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        try {
            QtCassandra::unsignedCharValue(array);
            qDebug() << "error: unsignedCharValue() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        try {
            QtCassandra::int16Value(array);
            qDebug() << "error: int16Value() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        try {
            QtCassandra::uint16Value(array);
            qDebug() << "error: uint16Value() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        try {
            QtCassandra::int32Value(array);
            qDebug() << "error: int32Value() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        try {
            QtCassandra::uint32Value(array);
            qDebug() << "error: uint32Value() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        try {
            QtCassandra::int64Value(array);
            qDebug() << "error: int64Value() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        try {
            QtCassandra::uint64Value(array);
            qDebug() << "error: uint64Value() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        try {
            QtCassandra::floatValue(array);
            qDebug() << "error: floatValue() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        try {
            QtCassandra::doubleValue(array);
            qDebug() << "error: doubleValue() did not generate an error with an invalid size";
            ++err;
        }
        catch(const std::runtime_error&) {
        }
        // test once more with the index
        for(int i(-100); i <= 100; ++i) {
            try {
                QtCassandra::boolValue(array, i);
                qDebug() << "error: boolValue() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
            try {
                QtCassandra::charValue(array, i);
                qDebug() << "error: charValue() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
            try {
                QtCassandra::signedCharValue(array, i);
                qDebug() << "error: signedCharValue() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
            try {
                QtCassandra::unsignedCharValue(array, i);
                qDebug() << "error: unsignedCharValue() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
            try {
                QtCassandra::int16Value(array, i);
                qDebug() << "error: int16Value() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
            try {
                QtCassandra::uint16Value(array, i);
                qDebug() << "error: uint16Value() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
            try {
                QtCassandra::int32Value(array, i);
                qDebug() << "error: int32Value() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
            try {
                QtCassandra::uint32Value(array, i);
                qDebug() << "error: uint32Value() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
            try {
                QtCassandra::int64Value(array, i);
                qDebug() << "error: int64Value() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
            try {
                QtCassandra::uint64Value(array, i);
                qDebug() << "error: uint64Value() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
            try {
                QtCassandra::floatValue(array, i);
                qDebug() << "error: floatValue() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
            try {
                QtCassandra::doubleValue(array, i);
                qDebug() << "error: doubleValue() did not generate an error with an invalid size";
                ++err;
            }
            catch(const std::runtime_error&) {
            }
        }
        QtCassandra::QCassandraValue null_value;
        if(null_value != array) {
            qDebug() << "error: default QCassandraValue() constructor not creating a null value";
            ++err;
        }

        // Boolean
        qDebug() << "++ Boolean";
        QtCassandra::setBoolValue(array, false);
        if(array.size() != sizeof(bool)) {
            qDebug() << "error: the setBoolValue() is not setting exactly one value.";
            ++err;
        }
        if(QtCassandra::boolValue(array) != false) {
            qDebug() << "error: the setBoolValue() did not set false or the boolValue() did not read it back properly.";
            ++err;
        }
        if(QtCassandra::boolValue(array, 0) != false) {
            qDebug() << "error: the setBoolValue() did not set false or the boolValue() did not read it back properly.";
            ++err;
        }
        value.setBoolValue(false);
        if(value.size() != sizeof(bool)) {
            qDebug() << "error: the value.setBoolValue() is not setting exactly one value.";
            ++err;
        }
        if(value.boolValue() != false) {
            qDebug() << "error: the value.setBoolValue() did not set false or the value.boolValue() did not read it back properly.";
            ++err;
        }
        if(value.boolValue(0) != false) {
            qDebug() << "error: the value.setBoolValue() did not set false or the value.boolValue() did not read it back properly.";
            ++err;
        }
        {
            QtCassandra::QCassandraValue v(false);
            if(value != v) {
                qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                ++err;
            }
        }

        QtCassandra::setBoolValue(array, true);
        if(array.size() != sizeof(bool)) {
            qDebug() << "error: the setBoolValue() is not setting exactly one value.";
            ++err;
        }
        if(QtCassandra::boolValue(array) != true) {
            qDebug() << "error: the setBoolValue() did not set false or the boolValue() did not read it back properly.";

        }
        if(QtCassandra::boolValue(array, 0) != true) {
            qDebug() << "error: the setBoolValue() did not set false or the boolValue() did not read it back properly.";
            ++err;
        }
        value.setBoolValue(true);
        if(value.size() != sizeof(bool)) {
            qDebug() << "error: the value.setBoolValue() is not setting exactly one value.";
            ++err;
        }
        if(value.boolValue() != true) {
            qDebug() << "error: the value.setBoolValue() did not set false or the value.boolValue() did not read it back properly.";
            ++err;
        }
        if(value.boolValue(0) != true) {
            qDebug() << "error: the value.setBoolValue() did not set false or the value.boolValue() did not read it back properly.";
            ++err;
        }
        {
            QtCassandra::QCassandraValue v(true);
            if(value != v) {
                qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                ++err;
            }
        }

        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                QtCassandra::boolValue(array, i);
            }
            else {
                try {
                    QtCassandra::boolValue(array, i);
                    qDebug() << "error: boolValue() did not generate an error with an invalid size";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // Char
        qDebug() << "++ Char";
        for(int i(0); i < 256; ++i) {
            QtCassandra::setCharValue(array, i);
            if(array.size() != sizeof(char)) {
                qDebug() << "error: the setCharValue() is not setting exactly one value.";
                ++err;
            }
            if(QtCassandra::charValue(array) != static_cast<char>(i)) {
                qDebug() << "error: the setCharValue() did not set" << i << "or the charValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::charValue(array, 0) != static_cast<char>(i)) {
                qDebug() << "error: the setCharValue() did not set" << i << "or the charValue() did not read it back properly.";
                ++err;
            }
            value.setCharValue(i);
            if(value.size() != sizeof(char)) {
                qDebug() << "error: the value.setCharValue() is not setting exactly one value.";
                ++err;
            }
            if(value.charValue() != static_cast<char>(i)) {
                qDebug() << "error: the value.setCharValue() did not set" << i << "or the value.charValue() did not read it back properly.";
                ++err;
            }
            if(value.charValue(0) != static_cast<char>(i)) {
                qDebug() << "error: the value.setCharValue() did not set" << i << "or the value.charValue() did not read it back properly.";
                ++err;
            }
            {
                QtCassandra::QCassandraValue v(static_cast<char>(i));
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }
        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                QtCassandra::charValue(array, i);
            }
            else {
                try {
                    QtCassandra::charValue(array, i);
                    qDebug() << "error: charValue() did not generate an error with an invalid size";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        char char_buffer[256];
        for(size_t i(0); i < sizeof(char_buffer) / sizeof(char_buffer[0]); ++i) {
            char_buffer[i] = static_cast<char>(my_rand());
            QtCassandra::appendCharValue(array, char_buffer[i]);
            if(static_cast<size_t>(array.size()) != sizeof(char) * (i + 1)) {
                qDebug() << "error: the appendCharValue() generated the wrong array size" << (i + 1) << "/" << array.size() << ".";
                ++err;
            }
            if(QtCassandra::charValue(array, i) != static_cast<char>(char_buffer[i])) {
                qDebug() << "error: the appendCharValue() did not set" << i << "or the charValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::charValue(array, i) != static_cast<char>(char_buffer[i])) {
                qDebug() << "error: the appendCharValue() did not set" << i << "or the charValue() did not read it back properly.";
                ++err;
            }
        }
        QByteArray char_buf(QtCassandra::binaryValue(array, 0, sizeof(char_buffer)));
        if(memcmp(char_buf.data(), char_buffer, sizeof(char_buffer)) != 0) {
            qDebug() << "error: the appendCharValue() did not set the 256 bytes as expected?";
            ++err;
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // Signed Char
        qDebug() << "++ Signed Char";
        for(int i(0); i < 256; ++i) {
            QtCassandra::setSignedCharValue(array, i);
            if(array.size() != sizeof(signed char)) {
                qDebug() << "error: the setSignedCharValue() is not setting exactly one value.";
                ++err;
            }
            if(QtCassandra::signedCharValue(array) != static_cast<signed char>(i)) {
                qDebug() << "error: the setSignedCharValue() did not set" << i << "or the signedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::signedCharValue(array, 0) != static_cast<signed char>(i)) {
                qDebug() << "error: the setSignedCharValue() did not set" << i << "or the signedCharValue() did not read it back properly.";
                ++err;
            }
            value.setSignedCharValue(i);
            if(value.size() != sizeof(signed char)) {
                qDebug() << "error: the value.setSignedCharValue() is not setting exactly one value.";
                ++err;
            }
            if(value.signedCharValue() != static_cast<signed char>(i)) {
                qDebug() << "error: the value.setSignedCharValue() did not set" << i << "or the value.signedCharValue() did not read it back properly.";
                ++err;
            }
            if(value.signedCharValue(0) != static_cast<signed char>(i)) {
                qDebug() << "error: the value.setSignedCharValue() did not set" << i << "or the value.signedCharValue() did not read it back properly.";
                ++err;
            }
            {
                QtCassandra::QCassandraValue v(static_cast<signed char>(i));
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }
        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                QtCassandra::signedCharValue(array, i);
            }
            else {
                try {
                    QtCassandra::signedCharValue(array, i);
                    qDebug() << "error: signedCharValue() did not generate an error with an invalid size";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        signed char signed_char_buffer[256];
        for(size_t i(0); i < sizeof(signed_char_buffer) / sizeof(signed_char_buffer[0]); ++i) {
            signed_char_buffer[i] = static_cast<signed char>(my_rand());
            QtCassandra::appendSignedCharValue(array, signed_char_buffer[i]);
            if(static_cast<size_t>(array.size()) != sizeof(signed char) * (i + 1)) {
                qDebug() << "error: the appendSignedCharValue() generated the wrong array size" << (i + 1) << "/" << array.size() << ".";
                ++err;
            }
            if(QtCassandra::signedCharValue(array, i) != static_cast<signed char>(signed_char_buffer[i])) {
                qDebug() << "error: the appendSignedCharValue() did not set" << i << "or the signedCharValue() did not read it back properly.";
                ++err;
            }
        }
        QByteArray signed_char_buf(QtCassandra::binaryValue(array, 0, sizeof(signed_char_buffer)));
        if(memcmp(signed_char_buf.data(), signed_char_buffer, sizeof(signed_char_buffer)) != 0) {
            qDebug() << "error: the appendSignedCharValue() did not set the 256 bytes as expected?";
            ++err;
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // Unsigned Char
        qDebug() << "++ Unsigned Char";
        for(int i(0); i < 256; ++i) {
            QtCassandra::setUnsignedCharValue(array, i);
            if(array.size() != sizeof(unsigned char)) {
                qDebug() << "error: the setUnsignedCharValue() is not setting exactly one value.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array) != static_cast<unsigned char>(i)) {
                qDebug() << "error: the setUnsignedCharValue() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 0) != static_cast<unsigned char>(i)) {
                qDebug() << "error: the setUnsignedCharValue() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            value.setUnsignedCharValue(i);
            if(value.size() != sizeof(unsigned char)) {
                qDebug() << "error: the value.setUnsignedCharValue() is not setting exactly one value.";
                ++err;
            }
            if(value.unsignedCharValue() != static_cast<unsigned char>(i)) {
                qDebug() << "error: the value.setUnsignedCharValue() did not set" << i << "or the value.unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(value.unsignedCharValue(0) != static_cast<unsigned char>(i)) {
                qDebug() << "error: the value.setUnsignedCharValue() did not set" << i << "or the value.unsignedCharValue() did not read it back properly.";
                ++err;
            }
            {
                QtCassandra::QCassandraValue v(static_cast<unsigned char>(i));
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }
        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                QtCassandra::unsignedCharValue(array, i);
            }
            else {
                try {
                    QtCassandra::unsignedCharValue(array, i);
                    qDebug() << "error: unsignedCharValue() did not generate an error with an invalid size";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        unsigned char unsigned_char_buffer[256];
        for(size_t i(0); i < sizeof(unsigned_char_buffer) / sizeof(unsigned_char_buffer[0]); ++i) {
            unsigned_char_buffer[i] = static_cast<unsigned char>(my_rand());
            QtCassandra::appendUnsignedCharValue(array, unsigned_char_buffer[i]);
            if(static_cast<size_t>(array.size()) != sizeof(unsigned char) * (i + 1)) {
                qDebug() << "error: the appendUnsignedCharValue() generated the wrong array size" << (i + 1) << "/" << array.size() << ".";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, i) != static_cast<unsigned char>(unsigned_char_buffer[i])) {
                qDebug() << "error: the appendUnsignedCharValue() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
        }
        QByteArray unsigned_char_buf(QtCassandra::binaryValue(array, 0, sizeof(unsigned_char_buffer)));
        if(memcmp(unsigned_char_buf.data(), unsigned_char_buffer, sizeof(unsigned_char_buffer)) != 0) {
            qDebug() << "error: the appendUnsignedCharValue() did not set the 256 bytes as expected?";
            ++err;
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // Int16
        qDebug() << "++ Int16";
        for(int i(0); i < 65536; ++i) {
            QtCassandra::setInt16Value(array, i);
            if(array.size() != sizeof(uint16_t)) {
                qDebug() << "error: the setInt16Value() is not setting exactly one value.";
                ++err;
            }
            if(QtCassandra::int16Value(array) != static_cast<int16_t>(i)) {
                qDebug() << "error: the setInt16Value() did not set" << i << "or the int16Value() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::int16Value(array, 0) != static_cast<int16_t>(i)) {
                qDebug() << "error: the setInt16Value() did not set" << i << "or the int16Value() did not read it back properly.";
                ++err;
            }
            // make sure it was saved in big endian
            if(QtCassandra::unsignedCharValue(array, 0) != static_cast<unsigned char>(i >> 8)) {
                qDebug() << "error: the setInt16Value() did not set" << i << "in big end format (high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 1) != static_cast<unsigned char>(i)) {
                qDebug() << "error: the setInt16Value() did not set" << i << "in big end format (low).";
                ++err;
            }
            value.setInt16Value(i);
            if(value.size() != sizeof(int16_t)) {
                qDebug() << "error: the value.setInt16Value() is not setting exactly one value.";
                ++err;
            }
            if(value.int16Value() != static_cast<int16_t>(i)) {
                qDebug() << "error: the value.setInt16Value() did not set" << i << "or the value.int16Value() did not read it back properly.";
                ++err;
            }
            if(value.int16Value(0) != static_cast<int16_t>(i)) {
                qDebug() << "error: the value.setInt16Value() did not set" << i << "or the value.int16Value() did not read it back properly.";
                ++err;
            }
            if(value.unsignedCharValue(0) != static_cast<unsigned char>(i >> 8)) {
                qDebug() << "error: the setInt16Value() did not set" << i << "in big end format (high).";
                ++err;
            }
            if(value.unsignedCharValue(1) != static_cast<unsigned char>(i)) {
                qDebug() << "error: the setInt16Value() did not set" << i << "in big end format (low).";
                ++err;
            }
            {
                QtCassandra::QCassandraValue v(static_cast<int16_t>(i));
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }
        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                // this works and was tested earlier
                QtCassandra::int16Value(array, i);
            }
            else {
                try {
                    QtCassandra::int16Value(array, i);
                    qDebug() << "error: int16Value() did not generate an error with an invalid size (" << i << ")";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        int16_t int16_buffer[256];
        for(size_t i(0); i < sizeof(int16_buffer) / sizeof(int16_buffer[0]); ++i) {
            int16_t r(static_cast<int16_t>(my_rand()));
            int16_buffer[i] = r;
            QtCassandra::appendInt16Value(array, int16_buffer[i]);
            if(static_cast<size_t>(array.size()) != sizeof(int16_t) * (i + 1)) {
                qDebug() << "error: the appendInt16Value() generated the wrong array size" << (i + 1) << "/" << array.size() << ".";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int16_t) * i + 0) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the appendInt16Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int16_t) * i + 1) != static_cast<unsigned char>(r)) {
                qDebug() << "error: the appendInt16Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
        }
        QByteArray int16_buf(QtCassandra::binaryValue(array, 0, sizeof(int16_buffer)));
        for(size_t i(0); i < sizeof(int16_buffer) / sizeof(int16_buffer[0]); ++i) {
            int16_t r((static_cast<unsigned char>(int16_buf.at(i * sizeof(int16_t) + 0)) << 8)
                      + static_cast<unsigned char>(int16_buf.at(i * sizeof(int16_t) + 1)));
            if(r != int16_buffer[i]) {
                qDebug() << "error: the appendInt16Value() did not set the int16_t at position" << i << "as expected?";
                ++err;
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // UInt16
        qDebug() << "++ UInt16";
        for(int i(0); i < 65536; ++i) {
            QtCassandra::setUInt16Value(array, i);
            if(array.size() != sizeof(uint16_t)) {
                qDebug() << "error: the setUInt16Value() is not setting exactly one value.";
                ++err;
            }
            if(QtCassandra::uint16Value(array) != static_cast<uint16_t>(i)) {
                qDebug() << "error: the setUInt16Value() did not set" << i << "or the uint16Value() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::uint16Value(array, 0) != static_cast<uint16_t>(i)) {
                qDebug() << "error: the setUInt16Value() did not set" << i << "or the uint16Value() did not read it back properly.";
                ++err;
            }
            // make sure it was saved in big endian
            if(QtCassandra::unsignedCharValue(array, 0) != static_cast<unsigned char>(i >> 8)) {
                qDebug() << "error: the setUInt16Value() did not set" << i << "in big end format (high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 1) != static_cast<unsigned char>(i)) {
                qDebug() << "error: the setUInt16Value() did not set" << i << "in big end format (low).";
                ++err;
            }
            value.setUInt16Value(i);
            if(value.size() != sizeof(uint16_t)) {
                qDebug() << "error: the value.setUInt16Value() is not setting exactly one value.";
                ++err;
            }
            if(value.uint16Value() != static_cast<uint16_t>(i)) {
                qDebug() << "error: the value.setUInt16Value() did not set" << i << "or the value.uint16Value() did not read it back properly.";
                ++err;
            }
            if(value.uint16Value(0) != static_cast<uint16_t>(i)) {
                qDebug() << "error: the value.setUInt16Value() did not set" << i << "or the value.uint16Value() did not read it back properly.";
                ++err;
            }
            if(value.unsignedCharValue(0) != static_cast<unsigned char>(i >> 8)) {
                qDebug() << "error: the setUInt16Value() did not set" << i << "in big end format (high).";
                ++err;
            }
            if(value.unsignedCharValue(1) != static_cast<unsigned char>(i)) {
                qDebug() << "error: the setUInt16Value() did not set" << i << "in big end format (low).";
                ++err;
            }
            {
                QtCassandra::QCassandraValue v(static_cast<uint16_t>(i));
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }
        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                // this works and was tested earlier
                QtCassandra::uint16Value(array, i);
            }
            else {
                try {
                    QtCassandra::uint16Value(array, i);
                    qDebug() << "error: uint16Value() did not generate an error with an invalid size (" << i << ")";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        uint16_t uint16_buffer[256];
        for(uint i(0); i < sizeof(uint16_buffer) / sizeof(uint16_buffer[0]); ++i) {
            uint16_t r(static_cast<uint16_t>(my_rand()));
            uint16_buffer[i] = r;
            QtCassandra::appendUInt16Value(array, uint16_buffer[i]);
            if(static_cast<uint>(array.size()) != sizeof(uint16_t) * (i + 1)) {
                qDebug() << "error: the appendUInt16Value() generated the wrong array size" << (i + 1) << "/" << array.size() << ".";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint16_t) * i + 0) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the appendUInt16Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint16_t) * i + 1) != static_cast<unsigned char>(r)) {
                qDebug() << "error: the appendUInt16Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
        }
        QByteArray uint16_buf(QtCassandra::binaryValue(array, 0, sizeof(int16_buffer)));
        for(size_t i(0); i < sizeof(uint16_buffer) / sizeof(uint16_buffer[0]); ++i) {
            uint16_t r((static_cast<unsigned char>(uint16_buf.at(i * sizeof(uint16_t) + 0)) << 8)
                      + static_cast<unsigned char>(uint16_buf.at(i * sizeof(uint16_t) + 1)));
            if(r != uint16_buffer[i]) {
                qDebug() << "error: the appendUInt16Value() did not set the uint16_t at position" << i << "as expected?";
                ++err;
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // Int32
        qDebug() << "++ Int32";
        for(int i(0); i < 65536; ++i) {
            int32_t r(my_rand());
            QtCassandra::setInt32Value(array, r);
            if(array.size() != sizeof(int32_t)) {
                qDebug() << "error: the setInt32Value() is not setting exactly one value.";
                ++err;
            }
            if(QtCassandra::int32Value(array) != static_cast<int32_t>(r)) {
                qDebug() << "error: the setInt32Value() did not set" << r << "or the int32Value() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::int32Value(array, 0) != static_cast<int32_t>(r)) {
                qDebug() << "error: the setInt32Value() did not set" << r << "or the int32Value() did not read it back properly.";
                ++err;
            }
            // make sure it was saved in big endian
            if(QtCassandra::unsignedCharValue(array, 0) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the setInt32Value() did not set" << r << "in big end format (high high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 1) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the setInt32Value() did not set" << r << "in big end format (high low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 2) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the setInt32Value() did not set" << r << "in big end format (low high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 3) != static_cast<unsigned char>(r >> 0)) {
                qDebug() << "error: the setInt32Value() did not set" << r << "in big end format (low low).";
                ++err;
            }
            value.setInt32Value(r);
            if(value.size() != sizeof(int32_t)) {
                qDebug() << "error: the value.setInt32Value() is not setting exactly one value.";
                ++err;
            }
            if(value.int32Value() != static_cast<int32_t>(r)) {
                qDebug() << "error: the value.setInt32Value() did not set" << r << "or the value.int32Value() did not read it back properly.";
                ++err;
            }
            if(value.int32Value(0) != static_cast<int32_t>(r)) {
                qDebug() << "error: the value.setInt32Value() did not set" << r << "or the value.int32Value() did not read it back properly.";
                ++err;
            }
            if(value.unsignedCharValue(0) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the setInt32Value() did not set" << r << "in big end format (high high).";
                ++err;
            }
            if(value.unsignedCharValue(1) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the setInt32Value() did not set" << r << "in big end format (high low).";
                ++err;
            }
            if(value.unsignedCharValue(2) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the setInt32Value() did not set" << r << "in big end format (low high).";
                ++err;
            }
            if(value.unsignedCharValue(3) != static_cast<unsigned char>(r >> 0)) {
                qDebug() << "error: the setInt32Value() did not set" << r << "in big end format (low low).";
                ++err;
            }
            {
                QtCassandra::QCassandraValue v(r);
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }
        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                // this works and was tested earlier
                QtCassandra::int32Value(array, i);
            }
            else {
                try {
                    QtCassandra::int32Value(array, i);
                    qDebug() << "error: int32Value() did not generate an error with an invalid size (" << i << ")";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        int32_t int32_buffer[256];
        for(size_t i(0); i < sizeof(int32_buffer) / sizeof(int32_buffer[0]); ++i) {
            int32_t r(static_cast<int32_t>(my_rand()));
            int32_buffer[i] = r;
            QtCassandra::appendInt32Value(array, int32_buffer[i]);
            if(static_cast<size_t>(array.size()) != sizeof(int32_t) * (i + 1)) {
                qDebug() << "error: the appendInt32Value() generated the wrong array size" << (i + 1) << "/" << array.size() << ".";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int32_t) * i + 0) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the appendInt32Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int32_t) * i + 1) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the appendInt32Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int32_t) * i + 2) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the appendInt32Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int32_t) * i + 3) != static_cast<unsigned char>(r)) {
                qDebug() << "error: the appendInt32Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
        }
        QByteArray int32_buf(QtCassandra::binaryValue(array, 0, sizeof(int32_buffer)));
        for(size_t i(0); i < sizeof(int32_buffer) / sizeof(int32_buffer[0]); ++i) {
            int32_t r((static_cast<unsigned char>(int32_buf.at(i * sizeof(int32_t) + 0)) << 24)
                    + (static_cast<unsigned char>(int32_buf.at(i * sizeof(int32_t) + 1)) << 16)
                    + (static_cast<unsigned char>(int32_buf.at(i * sizeof(int32_t) + 2)) << 8)
                    +  static_cast<unsigned char>(int32_buf.at(i * sizeof(int32_t) + 3)));
            if(r != int32_buffer[i]) {
                qDebug() << "error: the appendInt32Value() did not set the int32_t at position" << i << "as expected?";
                ++err;
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // UInt32
        qDebug() << "++ UInt32";
        for(int i(0); i < 65536; ++i) {
            uint32_t r(my_rand());
            QtCassandra::setUInt32Value(array, r);
            if(array.size() != sizeof(uint32_t)) {
                qDebug() << "error: the setUInt32Value() is not setting exactly one value.";
                ++err;
            }
            if(QtCassandra::uint32Value(array) != static_cast<uint32_t>(r)) {
                qDebug() << "error: the setUInt32Value() did not set" << r << "or the uint32Value() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::uint32Value(array, 0) != static_cast<uint32_t>(r)) {
                qDebug() << "error: the setUInt32Value() did not set" << r << "or the uint32Value() did not read it back properly.";
                ++err;
            }
            // make sure it was saved in big endian
            if(QtCassandra::unsignedCharValue(array, 0) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the setUInt32Value() did not set" << r << "in big end format (high high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 1) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the setUInt32Value() did not set" << r << "in big end format (high low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 2) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the setUInt32Value() did not set" << r << "in big end format (low high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 3) != static_cast<unsigned char>(r >> 0)) {
                qDebug() << "error: the setUInt32Value() did not set" << r << "in big end format (low low).";
                ++err;
            }
            value.setUInt32Value(r);
            if(value.size() != sizeof(uint32_t)) {
                qDebug() << "error: the value.setUInt32Value() is not setting exactly one value.";
                ++err;
            }
            if(value.uint32Value() != static_cast<uint32_t>(r)) {
                qDebug() << "error: the value.setUInt32Value() did not set" << r << "or the value.uint32Value() did not read it back properly.";
                ++err;
            }
            if(value.uint32Value(0) != static_cast<uint32_t>(r)) {
                qDebug() << "error: the value.setUInt32Value() did not set" << r << "or the value.uint32Value() did not read it back properly.";
                ++err;
            }
            if(value.unsignedCharValue(0) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the setUInt32Value() did not set" << r << "in big end format (high high).";
                ++err;
            }
            if(value.unsignedCharValue(1) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the setUInt32Value() did not set" << r << "in big end format (high low).";
                ++err;
            }
            if(value.unsignedCharValue(2) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the setUInt32Value() did not set" << r << "in big end format (low high).";
                ++err;
            }
            if(value.unsignedCharValue(3) != static_cast<unsigned char>(r >> 0)) {
                qDebug() << "error: the setUInt32Value() did not set" << r << "in big end format (low low).";
                ++err;
            }
            {
                QtCassandra::QCassandraValue v(r);
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }
        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                // this works and was tested earlier
                QtCassandra::uint32Value(array, i);
            }
            else {
                try {
                    QtCassandra::uint32Value(array, i);
                    qDebug() << "error: uint32Value() did not generate an error with an invalid size (" << i << ")";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        uint32_t uint32_buffer[256];
        for(size_t i(0); i < sizeof(uint32_buffer) / sizeof(uint32_buffer[0]); ++i) {
            uint32_t r(static_cast<uint32_t>(my_rand()));
            uint32_buffer[i] = r;
            QtCassandra::appendUInt32Value(array, uint32_buffer[i]);
            if(static_cast<size_t>(array.size()) != sizeof(uint32_t) * (i + 1)) {
                qDebug() << "error: the appendUInt32Value() generated the wrong array size" << (i + 1) << "/" << array.size() << ".";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint32_t) * i + 0) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the appendUInt32Value() did not set" << i << "or the UInt32Value() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint32_t) * i + 1) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the appendUInt32Value() did not set" << i << "or the UInt32Value() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint32_t) * i + 2) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the appendUInt32Value() did not set" << i << "or the UInt32Value() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint32_t) * i + 3) != static_cast<unsigned char>(r)) {
                qDebug() << "error: the appendUInt32Value() did not set" << i << "or the UInt32Value() did not read it back properly.";
                ++err;
            }
        }
        QByteArray uint32_buf(QtCassandra::binaryValue(array, 0, sizeof(uint32_buffer)));
        for(size_t i(0); i < sizeof(uint32_buffer) / sizeof(uint32_buffer[0]); ++i) {
            uint32_t r((static_cast<unsigned char>(uint32_buf.at(i * sizeof(uint32_t) + 0)) << 24)
                     + (static_cast<unsigned char>(uint32_buf.at(i * sizeof(uint32_t) + 1)) << 16)
                     + (static_cast<unsigned char>(uint32_buf.at(i * sizeof(uint32_t) + 2)) << 8)
                     +  static_cast<unsigned char>(uint32_buf.at(i * sizeof(uint32_t) + 3)));
            if(r != uint32_buffer[i]) {
                qDebug() << "error: the appendUInt32Value() did not set the uint32_t at position" << i << "as expected?";
                ++err;
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // Int64
        qDebug() << "++ Int64";
        for(int i(0); i < 65536; ++i) {
            int64_t r(my_rand());
            QtCassandra::setInt64Value(array, r);
            if(array.size() != sizeof(int64_t)) {
                qDebug() << "error: the setInt64Value() is not setting exactly one value.";
                ++err;
            }
            if(QtCassandra::int64Value(array) != static_cast<int64_t>(r)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "or the int64Value() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::int64Value(array, 0) != static_cast<int64_t>(r)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "or the int64Value() did not read it back properly.";
                ++err;
            }
            // make sure it was saved in big endian
            if(QtCassandra::unsignedCharValue(array, 0) != static_cast<unsigned char>(r >> 56)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (high high high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 1) != static_cast<unsigned char>(r >> 48)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (high high low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 2) != static_cast<unsigned char>(r >> 40)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (high low high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 3) != static_cast<unsigned char>(r >> 32)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (high low low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 4) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (low high high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 5) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (low high low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 6) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (low low high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 7) != static_cast<unsigned char>(r >> 0)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (low low low).";
                ++err;
            }
            value.setInt64Value(r);
            if(value.size() != sizeof(int64_t)) {
                qDebug() << "error: the value.setInt64Value() is not setting exactly one value.";
                ++err;
            }
            if(value.int64Value() != static_cast<int64_t>(r)) {
                qDebug() << "error: the value.setInt64Value() did not set" << r << "or the value.int64Value() did not read it back properly.";
                ++err;
            }
            if(value.int64Value(0) != static_cast<int64_t>(r)) {
                qDebug() << "error: the value.setInt64Value() did not set" << r << "or the value.int64Value() did not read it back properly.";
                ++err;
            }
            if(value.unsignedCharValue(0) != static_cast<unsigned char>(r >> 56)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (high high high).";
                ++err;
            }
            if(value.unsignedCharValue(1) != static_cast<unsigned char>(r >> 48)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (high high low).";
                ++err;
            }
            if(value.unsignedCharValue(2) != static_cast<unsigned char>(r >> 40)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (high low high).";
                ++err;
            }
            if(value.unsignedCharValue(3) != static_cast<unsigned char>(r >> 32)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (high low low).";
                ++err;
            }
            if(value.unsignedCharValue(4) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (low high high).";
                ++err;
            }
            if(value.unsignedCharValue(5) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (low high low).";
                ++err;
            }
            if(value.unsignedCharValue(6) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (low low high).";
                ++err;
            }
            if(value.unsignedCharValue(7) != static_cast<unsigned char>(r >> 0)) {
                qDebug() << "error: the setInt64Value() did not set" << r << "in big end format (low low low).";
                ++err;
            }
            {
                QtCassandra::QCassandraValue v(r);
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }
        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                // this works and was tested earlier
                QtCassandra::int64Value(array, i);
            }
            else {
                try {
                    QtCassandra::int64Value(array, i);
                    qDebug() << "error: int64Value() did not generate an error with an invalid size (" << i << ")";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        int64_t int64_buffer[256];
        for(size_t i(0); i < sizeof(int64_buffer) / sizeof(int64_buffer[0]); ++i) {
            int64_t r(static_cast<int64_t>(my_rand()));
            int64_buffer[i] = r;
            QtCassandra::appendInt64Value(array, int64_buffer[i]);
            if(static_cast<size_t>(array.size()) != sizeof(int64_t) * (i + 1)) {
                qDebug() << "error: the appendInt64Value() generated the wrong array size" << (i + 1) << "/" << array.size() << ".";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int64_t) * i + 0) != static_cast<unsigned char>(r >> 56)) {
                qDebug() << "error: the appendInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int64_t) * i + 1) != static_cast<unsigned char>(r >> 48)) {
                qDebug() << "error: the appendInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int64_t) * i + 2) != static_cast<unsigned char>(r >> 40)) {
                qDebug() << "error: the appendInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int64_t) * i + 3) != static_cast<unsigned char>(r >> 32)) {
                qDebug() << "error: the appendInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int64_t) * i + 4) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the appendInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int64_t) * i + 5) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the appendInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int64_t) * i + 6) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the appendInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(int64_t) * i + 7) != static_cast<unsigned char>(r)) {
                qDebug() << "error: the appendInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
        }
        QByteArray int64_buf(QtCassandra::binaryValue(array, 0, sizeof(int64_buffer)));
        for(size_t i(0); i < sizeof(int64_buffer) / sizeof(int64_buffer[0]); ++i) {
            int64_t r((static_cast<int64_t>(static_cast<unsigned char>(int64_buf.at(i * sizeof(int64_t) + 0))) << 56)
                    + (static_cast<int64_t>(static_cast<unsigned char>(int64_buf.at(i * sizeof(int64_t) + 1))) << 48)
                    + (static_cast<int64_t>(static_cast<unsigned char>(int64_buf.at(i * sizeof(int64_t) + 2))) << 40)
                    + (static_cast<int64_t>(static_cast<unsigned char>(int64_buf.at(i * sizeof(int64_t) + 3))) << 32)
                    + (static_cast<int64_t>(static_cast<unsigned char>(int64_buf.at(i * sizeof(int64_t) + 4))) << 24)
                    + (static_cast<int64_t>(static_cast<unsigned char>(int64_buf.at(i * sizeof(int64_t) + 5))) << 16)
                    + (static_cast<int64_t>(static_cast<unsigned char>(int64_buf.at(i * sizeof(int64_t) + 6))) << 8)
                    +  static_cast<int64_t>(static_cast<unsigned char>(int64_buf.at(i * sizeof(int64_t) + 7))));
            if(r != int64_buffer[i]) {
                qDebug() << "error: the appendInt64Value() did not set the int64_t at position" << i << "as expected?";
                ++err;
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // UInt64
        qDebug() << "++ UInt64";
        for(int i(0); i < 65536; ++i) {
            uint64_t r(my_rand());
            QtCassandra::setUInt64Value(array, r);
            if(array.size() != sizeof(uint64_t)) {
                qDebug() << "error: the setUInt64Value() is not setting exactly one value.";
                ++err;
            }
            if(QtCassandra::uint64Value(array) != static_cast<uint64_t>(r)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "or the uint64Value() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::uint64Value(array, 0) != static_cast<uint64_t>(r)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "or the uint64Value() did not read it back properly.";
                ++err;
            }
            // make sure it was saved in big endian
            if(QtCassandra::unsignedCharValue(array, 0) != static_cast<unsigned char>(r >> 56)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (high high high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 1) != static_cast<unsigned char>(r >> 48)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (high high low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 2) != static_cast<unsigned char>(r >> 40)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (high low high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 3) != static_cast<unsigned char>(r >> 32)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (high low low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 4) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (low high high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 5) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (low high low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 6) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (low low high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 7) != static_cast<unsigned char>(r >> 0)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (low low low).";
                ++err;
            }
            value.setUInt64Value(r);
            if(value.size() != sizeof(uint64_t)) {
                qDebug() << "error: the value.setUInt64Value() is not setting exactly one value.";
                ++err;
            }
            if(value.uint64Value() != static_cast<uint64_t>(r)) {
                qDebug() << "error: the value.setUInt64Value() did not set" << r << "or the value.uint64Value() did not read it back properly.";
                ++err;
            }
            if(value.uint64Value(0) != static_cast<uint64_t>(r)) {
                qDebug() << "error: the value.setUInt64Value() did not set" << r << "or the value.uint64Value() did not read it back properly.";
                ++err;
            }
            if(value.unsignedCharValue(0) != static_cast<unsigned char>(r >> 56)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (high).";
                ++err;
            }
            if(value.unsignedCharValue(1) != static_cast<unsigned char>(r >> 48)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (low).";
                ++err;
            }
            if(value.unsignedCharValue(2) != static_cast<unsigned char>(r >> 40)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (high).";
                ++err;
            }
            if(value.unsignedCharValue(3) != static_cast<unsigned char>(r >> 32)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (low).";
                ++err;
            }
            if(value.unsignedCharValue(4) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (high).";
                ++err;
            }
            if(value.unsignedCharValue(5) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (low).";
                ++err;
            }
            if(value.unsignedCharValue(6) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (high).";
                ++err;
            }
            if(value.unsignedCharValue(7) != static_cast<unsigned char>(r >> 0)) {
                qDebug() << "error: the setUInt64Value() did not set" << r << "in big end format (low).";
                ++err;
            }
            {
                QtCassandra::QCassandraValue v(r);
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }
        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                // this works and was tested earlier
                QtCassandra::uint64Value(array, i);
            }
            else {
                try {
                    QtCassandra::uint64Value(array, i);
                    qDebug() << "error: uint64Value() did not generate an error with an invalid size (" << i << ")";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        uint64_t uint64_buffer[256];
        for(size_t i(0); i < sizeof(uint64_buffer) / sizeof(uint64_buffer[0]); ++i) {
            uint64_t r(static_cast<uint64_t>(my_rand()));
            uint64_buffer[i] = r;
            QtCassandra::appendUInt64Value(array, uint64_buffer[i]);
            if(static_cast<size_t>(array.size()) != sizeof(uint64_t) * (i + 1)) {
                qDebug() << "error: the appendUInt64Value() generated the wrong array size" << (i + 1) << "/" << array.size() << ".";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint64_t) * i + 0) != static_cast<unsigned char>(r >> 56)) {
                qDebug() << "error: the appendUInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint64_t) * i + 1) != static_cast<unsigned char>(r >> 48)) {
                qDebug() << "error: the appendUInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint64_t) * i + 2) != static_cast<unsigned char>(r >> 40)) {
                qDebug() << "error: the appendUInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint64_t) * i + 3) != static_cast<unsigned char>(r >> 32)) {
                qDebug() << "error: the appendUInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint64_t) * i + 4) != static_cast<unsigned char>(r >> 24)) {
                qDebug() << "error: the appendUInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint64_t) * i + 5) != static_cast<unsigned char>(r >> 16)) {
                qDebug() << "error: the appendUInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint64_t) * i + 6) != static_cast<unsigned char>(r >> 8)) {
                qDebug() << "error: the appendUInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, sizeof(uint64_t) * i + 7) != static_cast<unsigned char>(r)) {
                qDebug() << "error: the appendUInt64Value() did not set" << i << "or the unsignedCharValue() did not read it back properly.";
                ++err;
            }
        }
        QByteArray uint64_buf(QtCassandra::binaryValue(array, 0, sizeof(uint64_buffer)));
        for(size_t i(0); i < sizeof(uint64_buffer) / sizeof(uint64_buffer[0]); ++i) {
            uint64_t r((static_cast<int64_t>(static_cast<unsigned char>(uint64_buf.at(i * sizeof(uint64_t) + 0))) << 56)
                     + (static_cast<int64_t>(static_cast<unsigned char>(uint64_buf.at(i * sizeof(uint64_t) + 1))) << 48)
                     + (static_cast<int64_t>(static_cast<unsigned char>(uint64_buf.at(i * sizeof(uint64_t) + 2))) << 40)
                     + (static_cast<int64_t>(static_cast<unsigned char>(uint64_buf.at(i * sizeof(uint64_t) + 3))) << 32)
                     + (static_cast<int64_t>(static_cast<unsigned char>(uint64_buf.at(i * sizeof(uint64_t) + 4))) << 24)
                     + (static_cast<int64_t>(static_cast<unsigned char>(uint64_buf.at(i * sizeof(uint64_t) + 5))) << 16)
                     + (static_cast<int64_t>(static_cast<unsigned char>(uint64_buf.at(i * sizeof(uint64_t) + 6))) << 8)
                     +  static_cast<int64_t>(static_cast<unsigned char>(uint64_buf.at(i * sizeof(uint64_t) + 7))));
            if(r != uint64_buffer[i]) {
                qDebug() << "error: the appendUInt64Value() did not set the uint64_t at position" << i << "as expected?";
                ++err;
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // Float
        qDebug() << "++ Float";
        for(int i(0); i < 65536; ++i) {
            // use a few "special" values, and the rest is randomized
            switch(i) {
            case 0:
                float_int.f = 0.0f;
                break;

            case 1:
                float_int.f = 1.0f;
                break;

            case 2:
                float_int.f = -1.0f;
                break;

            default:
                float_int.f = static_cast<int32_t>(my_rand()) / 66000.0f;
                break;

            }
            QtCassandra::setFloatValue(array, float_int.f);
            if(array.size() != sizeof(float)) {
                qDebug() << "error: the setFloatValue() is not setting exactly one value.";
                ++err;
            }
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfloat-equal"
            if(QtCassandra::floatValue(array) != float_int.f) {
    #pragma GCC diagnostic pop
                qDebug() << "error: the setFloatValue() did not set" << float_int.f << "or the floatValue() did not read it back properly.";
                ++err;
            }
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfloat-equal"
            if(QtCassandra::floatValue(array, 0) != float_int.f) {
    #pragma GCC diagnostic pop
                qDebug() << "error: the setFloatValue() did not set" << float_int.f << "or the floatValue() did not read it back properly.";
                ++err;
            }
            // make sure it was saved in big endian
            if(QtCassandra::unsignedCharValue(array, 0) != static_cast<unsigned char>(float_int.v >> 24)) {
                qDebug() << "error: the setFloatValue() did not set" << float_int.f << "in big end format (high high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 1) != static_cast<unsigned char>(float_int.v >> 16)) {
                qDebug() << "error: the setFloatValue() did not set" << float_int.f << "in big end format (high low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 2) != static_cast<unsigned char>(float_int.v >> 8)) {
                qDebug() << "error: the setFloatValue() did not set" << float_int.f << "in big end format (low high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 3) != static_cast<unsigned char>(float_int.v >> 0)) {
                qDebug() << "error: the setFloatValue() did not set" << float_int.f << "in big end format (low low).";
                ++err;
            }
            value.setFloatValue(float_int.f);
            if(value.size() != sizeof(float)) {
                qDebug() << "error: the value.setFloatValue() is not setting exactly one value.";
                ++err;
            }
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfloat-equal"
            if(value.floatValue() != float_int.f) {
    #pragma GCC diagnostic pop
                qDebug() << "error: the value.setFloatValue() did not set" << float_int.f << "or the value.floatValue() did not read it back properly.";
                ++err;
            }
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfloat-equal"
            if(value.floatValue(0) != float_int.f) {
    #pragma GCC diagnostic pop
                qDebug() << "error: the value.setFloatValue() did not set" << float_int.f << "or the value.floatValue() did not read it back properly.";
                ++err;
            }
            if(value.unsignedCharValue(0) != static_cast<unsigned char>(float_int.v >> 24)) {
                qDebug() << "error: the setFloatValue() did not set" << float_int.f << "in big end format (high high).";
                ++err;
            }
            if(value.unsignedCharValue(1) != static_cast<unsigned char>(float_int.v >> 16)) {
                qDebug() << "error: the setFloatValue() did not set" << float_int.f << "in big end format (high low).";
                ++err;
            }
            if(value.unsignedCharValue(2) != static_cast<unsigned char>(float_int.v >> 8)) {
                qDebug() << "error: the setFloatValue() did not set" << float_int.f << "in big end format (low high).";
                ++err;
            }
            if(value.unsignedCharValue(3) != static_cast<unsigned char>(float_int.v >> 0)) {
                qDebug() << "error: the setFloatValue() did not set" << float_int.f << "in big end format (low low).";
                ++err;
            }
            {
                QtCassandra::QCassandraValue v(float_int.f);
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }
        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                // this works and was tested earlier
                QtCassandra::floatValue(array, i);
            }
            else {
                try {
                    QtCassandra::floatValue(array, i);
                    qDebug() << "error: floatValue() did not generate an error with an invalid size (" << i << ")";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // Double
        qDebug() << "++ Double";
        for(int i(0); i < 65536; ++i) {
            // use a few "special" values, and the rest is randomized
            switch(i) {
            case 0:
                double_int.f = 0.0;
                break;

            case 1:
                double_int.f = 1.0;
                break;

            case 2:
                double_int.f = -1.0;
                break;

            default:
                if(i < 10000) {
                    double_int.f = my_rand();
                }
                else {
                    double_int.f = my_rand() / 66000000.0;
                }
                break;

            }
            QtCassandra::setDoubleValue(array, double_int.f);
            if(array.size() != sizeof(double)) {
                qDebug() << "error: the setDoubleValue() is not setting exactly one value.";
                ++err;
            }
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfloat-equal"
            if(QtCassandra::doubleValue(array) != double_int.f) {
    #pragma GCC diagnostic pop
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "or the doubleValue() did not read it back properly.";
                ++err;
            }
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfloat-equal"
            if(QtCassandra::doubleValue(array, 0) != double_int.f) {
    #pragma GCC diagnostic pop
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "or the doubleValue() did not read it back properly.";
                ++err;
            }
            // make sure it was saved in big endian
            if(QtCassandra::unsignedCharValue(array, 0) != static_cast<unsigned char>(double_int.v >> 56)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (high high high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 1) != static_cast<unsigned char>(double_int.v >> 48)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (high high low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 2) != static_cast<unsigned char>(double_int.v >> 40)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (high low high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 3) != static_cast<unsigned char>(double_int.v >> 32)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (high low low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 4) != static_cast<unsigned char>(double_int.v >> 24)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (low high high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 5) != static_cast<unsigned char>(double_int.v >> 16)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (low high low).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 6) != static_cast<unsigned char>(double_int.v >> 8)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (low low high).";
                ++err;
            }
            if(QtCassandra::unsignedCharValue(array, 7) != static_cast<unsigned char>(double_int.v >> 0)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (low low low).";
                ++err;
            }
            value.setDoubleValue(double_int.f);
            if(value.size() != sizeof(double)) {
                qDebug() << "error: the value.setDoubleValue() is not setting exactly one value.";
                ++err;
            }
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfloat-equal"
            if(value.doubleValue() != double_int.f) {
    #pragma GCC diagnostic pop
                qDebug() << "error: the value.setDoubleValue() did not set" << double_int.f << "or the value.doubleValue() did not read it back properly.";
                ++err;
            }
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfloat-equal"
            if(value.doubleValue(0) != double_int.f) {
    #pragma GCC diagnostic pop
                qDebug() << "error: the value.setDoubleValue() did not set" << double_int.f << "or the value.doubleValue(0) did not read it back properly.";
                ++err;
            }
            if(value.unsignedCharValue(0) != static_cast<unsigned char>(double_int.v >> 56)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (high high high).";
                ++err;
            }
            if(value.unsignedCharValue(1) != static_cast<unsigned char>(double_int.v >> 48)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (high high low).";
                ++err;
            }
            if(value.unsignedCharValue(2) != static_cast<unsigned char>(double_int.v >> 40)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (high low high).";
                ++err;
            }
            if(value.unsignedCharValue(3) != static_cast<unsigned char>(double_int.v >> 32)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (high low low).";
                ++err;
            }
            if(value.unsignedCharValue(4) != static_cast<unsigned char>(double_int.v >> 24)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (low high high).";
                ++err;
            }
            if(value.unsignedCharValue(5) != static_cast<unsigned char>(double_int.v >> 16)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (low high low).";
                ++err;
            }
            if(value.unsignedCharValue(6) != static_cast<unsigned char>(double_int.v >> 8)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (low low high).";
                ++err;
            }
            if(value.unsignedCharValue(7) != static_cast<unsigned char>(double_int.v >> 0)) {
                qDebug() << "error: the setDoubleValue() did not set" << double_int.f << "in big end format (low low low).";
                ++err;
            }
            {
                QtCassandra::QCassandraValue v(double_int.f);
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }
        for(int i(-100); i <= 100; ++i) {
            if(i == 0) {
                // this works and was tested earlier
                QtCassandra::doubleValue(array, i);
            }
            else {
                try {
                    QtCassandra::doubleValue(array, i);
                    qDebug() << "error: doubleValue() did not generate an error with an invalid size (" << i << ")";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
        }
        QtCassandra::setNullValue(array);
        if(array.size() != 0) {
            qDebug() << "error: the setNullValue() is not clearing the buffer properly.";
            ++err;
        }

        // QString
        qDebug() << "++ QString";
        for(int i(0); i < 1000; ++i) {
            int size(rand() % 300);
            //qDebug() << "QString:" << i << "size" << size;
            QString str;
            for(int j(0); j < size; ++j) {
                // strings are UCS-2 so we can add any characters no 2 bytes
                ushort u;
                do {
                    u = static_cast<ushort>(rand());
                    // avoid surrogates and the "invalid" characters in plane 0
                } while(u == 0 || u == 0xFEFF || u == 0xFFFE || u == 0xFFFF || (u >= 0xD800 && u <= 0xDFFF) || (u >= 0xFDD0 && u <= 0xFDDF));
                str += QChar(u);
            }
            size = str.toUtf8().size();
            QtCassandra::setStringValue(array, str);
            if(array.size() != size) {
                qDebug() << "error: the setStringValue() is not setting the expected size.";
                ++err;
            }
            if(QtCassandra::stringValue(array) != str) {
                qDebug() << "error: the setStringValue() did not set the string" << cleanString(str) << "as expected, or the stringValue(array) did not read it back properly" << QtCassandra::stringValue(array);
                ++err;
            }
            if(QtCassandra::stringValue(array, 0) != str) {
                qDebug() << "error: the setStringValue() did not set the string" << cleanString(str) << "as expected, or the stringValue(array, 0) did not read it back properly" << QtCassandra::stringValue(array, 0);
                ++err;
            }
            if(QtCassandra::stringValue(array, 0, -1) != str) {
                qDebug() << "error: the setStringValue() did not set the string" << cleanString(str) << "as expected, or the stringValue(array, 0, -1) did not read it back properly" << QtCassandra::stringValue(array, 0, -1);
                ++err;
            }
            if(QtCassandra::stringValue(array, 0, size) != str) {
                qDebug() << "error: the setStringValue() did not set the string" << cleanString(str) << "as expected, or the stringValue(array, 0, size) did not read it back properly" << QtCassandra::stringValue(array, 0, size);
                ++err;
            }
            // the index and size need special care in case of a string
            if(i == 0) for(int j(-10); j <= size + 10; ++j) {
                for(int k(-10); k <= size + 10; ++k) {
                    // j is the index and k is the size
                    if(j < 0 || k < -1 || k > size || j > size || j + k > size) {
                        try {
                            QtCassandra::stringValue(array, j, k);
                            qDebug() << "error: the stringValue() did not generate an error with an invalid index " << j << " and/or size" << k << " (max. size is" << size << ")";
                            ++err;
                        }
                        catch(const std::runtime_error&) {
                        }
                    }
                    else {
                        k = size - j;
                        // with UTF-8 this is not as easy as it looks
                        //if(QtCassandra::stringValue(array, j, k) != str.mid(j, k)) {
                        //    qDebug() << "error: the stringValue() [" << str << "] did not return the expected string with " << j << " and " << k << ".";
                        //    ++err;
                        //}
                    }
                }
            }
            value.setStringValue(str);
            if(value.size() != size) {
                qDebug() << "error: the value.setStringValue() is not setting the expected size.";
                ++err;
            }
            if(value.stringValue() != str) {
                qDebug() << "error: the value.setStringValue() did not set the string" << cleanString(str) << "as expected, or the value.stringValue() did not read it back properly" << QtCassandra::stringValue(array);
                ++err;
            }
            if(value.stringValue(0) != str) {
                qDebug() << "error: the value.setStringValue() did not set the string" << cleanString(str) << "as expected, or the value.stringValue(0) did not read it back properly" << QtCassandra::stringValue(array, 0);
                ++err;
            }
            if(value.stringValue(0, -1) != str) {
                qDebug() << "error: the value.setStringValue() did not set the string" << cleanString(str) << "as expected, or the value.stringValue(0, -1) did not read it back properly" << QtCassandra::stringValue(array, 0, -1);
                ++err;
            }
            if(value.stringValue(0, size) != str) {
                qDebug() << "error: the value.setStringValue() did not set the string" << cleanString(str) << "as expected, or the value.stringValue(0, size) did not read it back properly" << QtCassandra::stringValue(array, 0, size);
                ++err;
            }
            // test again with a copy
            QtCassandra::QCassandraValue copy(value);
            if(copy.size() != size) {
                qDebug() << "error: the copy.setStringValue() is not setting the expected size.";
                ++err;
            }
            if(copy.stringValue() != str) {
                qDebug() << "error: the copy.setStringValue() did not set the string" << cleanString(str) << "as expected, or the copy.stringValue() did not read it back properly" << QtCassandra::stringValue(array);
                ++err;
            }
            if(copy.stringValue(0) != str) {
                qDebug() << "error: the copy.setStringValue() did not set the string" << cleanString(str) << "as expected, or the copy.stringValue(0) did not read it back properly" << QtCassandra::stringValue(array, 0);
                ++err;
            }
            if(copy.stringValue(0, -1) != str) {
                qDebug() << "error: the copy.setStringValue() did not set the string" << cleanString(str) << "as expected, or the copy.stringValue(0, -1) did not read it back properly" << QtCassandra::stringValue(array, 0, -1);
                ++err;
            }
            if(copy.stringValue(0, size) != str) {
                qDebug() << "error: the copy.setStringValue() did not set the string" << cleanString(str) << "as expected, or the copy.stringValue(0, size) did not read it back properly" << QtCassandra::stringValue(array, 0, size);
                ++err;
            }
            if(!(copy == value)) {
                qDebug() << "error: the copy.setStringValue() does not match the original" << cleanString(str) << "as expected";
                ++err;
            }
            if(copy != value) {
                qDebug() << "error: the copy.setStringValue() does not match the original which is unexpected" << cleanString(str);
                ++err;
            }
            if(copy < value) {
                qDebug() << "error: the copy.setStringValue() is considered smaller than the original which is unexpected" << cleanString(str);
                ++err;
            }
            if(!(copy <= value)) {
                qDebug() << "error: the copy.setStringValue() is not considered smaller or equal to the original which is unexpected" << cleanString(str);
                ++err;
            }
            if(copy > value) {
                qDebug() << "error: the copy.setStringValue() is considered larger than the original which is unexpected" << cleanString(str);
                ++err;
            }
            if(!(copy >= value)) {
                qDebug() << "error: the copy.setStringValue() is not considered larger or equal to the original which is unexpected" << cleanString(str);
                ++err;
            }
            // test again with a copy
            QtCassandra::QCassandraValue copy2;
            copy2 = value;
            if(copy2.size() != size) {
                qDebug() << "error: the copy2.setStringValue() is not setting the expected size.";
                ++err;
            }
            if(copy2.stringValue() != str) {
                qDebug() << "error: the copy2.setStringValue() did not set the string" << cleanString(str) << "as expected, or the copy2.stringValue() did not read it back properly" << QtCassandra::stringValue(array);
                ++err;
            }
            if(copy2.stringValue(0) != str) {
                qDebug() << "error: the copy2.setStringValue() did not set the string" << cleanString(str) << "as expected, or the copy2.stringValue(0) did not read it back properly" << QtCassandra::stringValue(array, 0);
                ++err;
            }
            if(copy2.stringValue(0, -1) != str) {
                qDebug() << "error: the copy2.setStringValue() did not set the string" << cleanString(str) << "as expected, or the copy2.stringValue(0, -1) did not read it back properly" << QtCassandra::stringValue(array, 0, -1);
                ++err;
            }
            if(copy2.stringValue(0, size) != str) {
                qDebug() << "error: the copy2.setStringValue() did not set the string" << cleanString(str) << "as expected, or the copy2.stringValue(0, size) did not read it back properly" << QtCassandra::stringValue(array, 0, size);
                ++err;
            }
            if(!(copy2 == value)) {
                qDebug() << "error: the copy2.setStringValue() does not match the original" << cleanString(str) << "as expected";
                ++err;
            }
            if(copy2 != value) {
                qDebug() << "error: the copy2.setStringValue() does not match the original which is unexpected" << cleanString(str);
                ++err;
            }
            if(copy2 < value) {
                qDebug() << "error: the copy2.setStringValue() is considered smaller than the original which is unexpected" << cleanString(str);
                ++err;
            }
            if(!(copy2 <= value)) {
                qDebug() << "error: the copy2.setStringValue() is not considered smaller or equal to the original which is unexpected" << cleanString(str);
                ++err;
            }
            if(copy2 > value) {
                qDebug() << "error: the copy2.setStringValue() is considered larger than the original which is unexpected" << cleanString(str);
                ++err;
            }
            if(!(copy2 >= value)) {
                qDebug() << "error: the copy2.setStringValue() is not considered larger or equal to the original which is unexpected" << cleanString(str);
                ++err;
            }
            // the index and size need special care in case of a string
            if(i == 0) for(int j(-10); j <= size + 10; ++j) {
                for(int k(-10); k <= size + 10; ++k) {
                    // j is the index and k is the size
                    if(j < 0 || k < -1 || k > size || j > size || j + k > size) {
                        try {
                            value.stringValue(j, k);
                            qDebug() << "error: the value.stringValue() did not generate an error with an invalid index " << j << " and/or size" << k << " (max. size is" << size << ")";
                            ++err;
                        }
                        catch(const std::runtime_error&) {
                        }
                    }
                    else {
                        k = size - j;
                        // with UTF-8 this is not as easy as it looks
                        //if(QtCassandra::stringValue(array, j, k) != str.mid(j, k)) {
                        //    qDebug() << "error: the stringValue() [" << str << "] did not return the expected string with " << j << " and " << k << ".";
                        //    ++err;
                        //}
                    }
                }
            }
            {
                QtCassandra::QCassandraValue v(str);
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }

        // QByteArray
        qDebug() << "++ QByteArray";
        for(int i(0); i < 1000; ++i) {
            int size(rand() % 300);
            // because we do an extensive test if i is zero we make sure we have
            // a respectable size
            while(i == 0 && size < 64) {
                size = rand() % 300;
            }
            //qDebug() << "QString:" << i << "size" << size;
            QByteArray buffer;
            for(int j(0); j < size; ++j) {
                // strings are UCS-2 so we can add any characters no 2 bytes
                buffer.append(static_cast<char>(rand()));
            }
            QtCassandra::setBinaryValue(array, buffer);
            if(QtCassandra::binaryValue(array) != buffer) {
                qDebug() << "error: the setBinaryValue() did not set the buffer as expected, or the binaryValue(array) did not read it back properly.";
                ++err;
            }
            if(QtCassandra::binaryValue(array, 0) != buffer) {
                qDebug() << "error: the setBinaryValue() did not set the buffer as expected, or the binaryValue(array, 0) did not read it back properly.";
                ++err;
            }
            if(QtCassandra::binaryValue(array, 0, -1) != buffer) {
                qDebug() << "error: the setBinaryValue() did not set the buffer as expected, or the binaryValue(array, 0, -1) did not read it back properly.";
                ++err;
            }
            if(QtCassandra::binaryValue(array, 0, size) != buffer) {
                qDebug() << "error: the setBinaryValue() did not set the buffer as expected, or the binaryValue(array, 0, size) did not read it back properly.";
                ++err;
            }
            // the index and size need special care in case of a string
            if(i == 0) for(int j(-10); j <= size + 10; ++j) {
                for(int k(-10); k <= size + 10; ++k) {
                    // j is the index and k is the size
                    int sz(k == -1 ? size - j : k);
                    if(j < 0 || k < -1 || k > size || j + sz > size || j > size) {
                        try {
                            QtCassandra::binaryValue(array, j, k);
                            qDebug() << "error: the binaryValue() did not generate an error with an invalid index " << j << " and/or size" << k << " (max. size is" << size << ")";
                            ++err;
                        }
                        catch(const std::runtime_error&) {
                        }
                    }
                    else {
                        if(QtCassandra::binaryValue(array, j, k) != buffer.mid(j, sz)) {
                            qDebug() << "error: the binaryValue() did not return the expected buffer with" << j << "and" << k << ".";
                            ++err;
                        }
                    }
                }
            }
            value.setBinaryValue(buffer);
            if(value.binaryValue() != buffer) {
                qDebug() << "error: the value.setBinaryValue() did not set the buffer as expected, or the value.binaryValue() did not read it back properly.";
                ++err;
            }
            if(value.binaryValue(0) != buffer) {
                qDebug() << "error: the value.setBinaryValue() did not set the buffer as expected, or the value.binaryValue(0) did not read it back properly.";
                ++err;
            }
            if(value.binaryValue(0, -1) != buffer) {
                qDebug() << "error: the value.setBinaryValue() did not set the buffer as expected, or the value.binaryValue(0, -1) did not read it back properly.";
                ++err;
            }
            if(value.binaryValue(0, size) != buffer) {
                qDebug() << "error: the value.setBinaryValue() did not set the buffer as expected, or the value.binaryValue(0, size) did not read it back properly.";
                ++err;
            }
            // the index and size need special care in case of a string
            if(i == 0) for(int j(-10); j <= size + 10; ++j) {
                for(int k(-10); k <= size + 10; ++k) {
                    // j is the index and k is the size
                    int sz(k == -1 ? size - j : k);
                    if(j < 0 || k < -1 || k > size || j + sz > size || j > size) {
                        try {
                            value.binaryValue(j, k);
                            qDebug() << "error: the value.binaryValue() did not generate an error with an invalid index " << j << " and/or size" << k << " (max. size is" << size << ")";
                            ++err;
                        }
                        catch(const std::runtime_error&) {
                        }
                    }
                    else {
                        if(value.binaryValue(j, k) != buffer.mid(j, sz)) {
                            qDebug() << "error: the value.binaryValue() did not return the expected buffer with" << j << "and" << k << ".";
                            ++err;
                        }
                    }
                }
            }
            {
                QtCassandra::QCassandraValue v(buffer);
                if(value != v) {
                    qDebug() << "error: the QCassandraValue constructor did not set the expected value.";
                    ++err;
                }
            }
        }


        // Comparisons
        qDebug() << "++ Comparisons";
        QtCassandra::QCassandraValue a, b;
        for(int i(0); i < 256; ++i) {
            for(int j(0); j < 256; ++j) {
                a.setUnsignedCharValue(i);
                b.setUnsignedCharValue(j);
                if(i == j) {
                    if(a != b) {
                        qDebug() << "error: the QCassandraValue != operator returned true when it shouldn't have.";
                        ++err;
                    }
                }
                else {
                    if(a == b) {
                        qDebug() << "error: the QCassandraValue == operator returned true when it shouldn't have.";
                        ++err;
                    }
                }
                if(i < j) {
                    if(a > b) {
                        qDebug() << "error: the QCassandraValue > operator returned true when it shouldn't have.";
                        ++err;
                    }
                    if(a >= b) {
                        qDebug() << "error: the QCassandraValue >= operator returned true when it shouldn't have.";
                        ++err;
                    }
                }
                if(i <= j) {
                    if(a > b) {
                        qDebug() << "error: the QCassandraValue > operator returned true when it shouldn't have (i <= j).";
                        ++err;
                    }
                }
                if(i > j) {
                    if(a < b) {
                        qDebug() << "error: the QCassandraValue < operator returned true when it shouldn't have.";
                        ++err;
                    }
                    if(a <= b) {
                        qDebug() << "error: the QCassandraValue <= operator returned true when it shouldn't have.";
                        ++err;
                    }
                }
                if(i >= j) {
                    if(a < b) {
                        qDebug() << "error: the QCassandraValue < operator returned true when it shouldn't have (i >= j).";
                        ++err;
                    }
                }
            }
        }

        for(int i(0); i < 65536; ++i) {
            //int32_t a(my_rand()); // shadow and unused
            //int32_t b(my_rand()); // shadow and unused
            int sza(my_rand() & 3), szb(sza);
            if(sza == 0) {
                sza = my_rand() % 3 + 1;
                do {
                    szb = my_rand() % 3 + 1;
                } while(sza == szb);
            }
            QByteArray data;
            for(int j(0); j < sza; ++j) {
                QtCassandra::appendUnsignedCharValue(data, my_rand());
            }
            QtCassandra::QCassandraValue av(data);
            QtCassandra::setNullValue(data);
            for(int j(0); j < szb; ++j) {
                QtCassandra::appendUnsignedCharValue(data, my_rand());
            }
            QtCassandra::QCassandraValue bv(data);
            int cmp(0);
            for(int j(0); j < sza && j < szb; ++j) {
                unsigned char ac(av.unsignedCharValue(j));
                unsigned char bc(bv.unsignedCharValue(j));
                if(ac != bc) {
                    if(ac < bc) {
                        cmp = -1;
                    }
                    else {
                        cmp = 1;
                    }
                    break;
                }
            }
            if(cmp == 0 && sza != szb) {
                if(sza > szb) {
                    cmp = 1;
                }
                else {
                    cmp = -1;
                }
            }
            bool cmp_eq(av == bv);
            bool cmp_ne(av != bv);
            bool cmp_lt(av < bv);
            bool cmp_le(av <= bv);
            bool cmp_gt(av > bv);
            bool cmp_ge(av >= bv);
            switch(cmp) {
            case -1:
                if(cmp_eq) {
                    qDebug() << "error: the QCassandraValue == operator returned an unexpected Boolean value. (-1)";
                    ++err;
                }
                if(!cmp_ne) {
                    qDebug() << "error: the QCassandraValue != operator returned an unexpected Boolean value. (-1)";
                    ++err;
                }
                if(!cmp_lt) {
                    qDebug() << "error: the QCassandraValue < operator returned an unexpected Boolean value. (-1)";
                    ++err;
                }
                if(!cmp_le) {
                    qDebug() << "error: the QCassandraValue <= operator returned an unexpected Boolean value. (-1)";
                    ++err;
                }
                if(cmp_gt) {
                    qDebug() << "error: the QCassandraValue > operator returned an unexpected Boolean value. (-1)";
                    ++err;
                }
                if(cmp_ge) {
                    qDebug() << "error: the QCassandraValue >= operator returned an unexpected Boolean value. (-1)";
                    ++err;
                }
                break;

            case 0:
                if(!cmp_eq) {
                    qDebug() << "error: the QCassandraValue == operator returned an unexpected Boolean value. (0)";
                    ++err;
                }
                if(cmp_ne) {
                    qDebug() << "error: the QCassandraValue != operator returned an unexpected Boolean value. (0)";
                    ++err;
                }
                if(cmp_lt) {
                    qDebug() << "error: the QCassandraValue < operator returned an unexpected Boolean value. (0)";
                    ++err;
                }
                if(!cmp_le) {
                    qDebug() << "error: the QCassandraValue <= operator returned an unexpected Boolean value. (0)";
                    ++err;
                }
                if(cmp_gt) {
                    qDebug() << "error: the QCassandraValue > operator returned an unexpected Boolean value. (0)";
                    ++err;
                }
                if(!cmp_ge) {
                    qDebug() << "error: the QCassandraValue >= operator returned an unexpected Boolean value. (0)";
                    ++err;
                }
                break;

            case 1:
                if(cmp_eq) {
                    qDebug() << "error: the QCassandraValue == operator returned an unexpected Boolean value. (1)";
                    ++err;
                }
                if(!cmp_ne) {
                    qDebug() << "error: the QCassandraValue != operator returned an unexpected Boolean value. (1)";
                    ++err;
                }
                if(cmp_lt) {
                    qDebug() << "error: the QCassandraValue < operator returned an unexpected Boolean value. (1)";
                    ++err;
                }
                if(cmp_le) {
                    qDebug() << "error: the QCassandraValue <= operator returned an unexpected Boolean value. (1)";
                    ++err;
                }
                if(!cmp_gt) {
                    qDebug() << "error: the QCassandraValue > operator returned an unexpected Boolean value. (1)";
                    ++err;
                }
                if(!cmp_ge) {
                    qDebug() << "error: the QCassandraValue >= operator returned an unexpected Boolean value. (1)";
                    ++err;
                }
                break;

            }
        }


        // check the TTL really quick
        qDebug() << "+ Testing TTL";
        for(int i(0); i < 256; ++i) {
            int32_t r(my_rand());
            if(r < 0) {
                if(static_cast<uint32_t>(r) == 0x80000000) {
                    // because -0x80000000 == 0x80000000 and we cannot use
                    // a negative number here
                    r = 0;
                }
                else {
                    r = -r;
                }
            }
            value.setTtl(r);
            if(value.ttl() != r) {
                qDebug() << "error: the ttl() value does not match" << r;
                ++err;
            }
        }
        // also test the default
        value.setTtl();
        if(value.ttl() != QtCassandra::QCassandraValue::TTL_PERMANENT) {
            qDebug() << "error: the ttl() value does not match" << QtCassandra::QCassandraValue::TTL_PERMANENT;
            ++err;
        }


        // consistency level
        qDebug() << "+ Testing consistency level";
        for(int i(-10); i < QtCassandra::CONSISTENCY_LEVEL_THREE + 10; ++i) {
            if(i < -1 || i == 0 || i > QtCassandra::CONSISTENCY_LEVEL_THREE) {
                try {
                    value.setConsistencyLevel(i);
                    qDebug() << "error: setConsistencyLevel() accepted" << i << "which is an invalid value";
                    ++err;
                }
                catch(const std::runtime_error&) {
                }
            }
            else {
                // Note: -1 is used as our DEFAULT value
                value.setConsistencyLevel(i);
                if(value.consistencyLevel() != i) {
                    qDebug() << "error: setConsistencyLevel(" << i << ") was not read back as" << i;
                    ++err;
                }
            }
        }





        // now test with a cell in the Cassandra database
        cassandra->connect(host);
        QString name = cassandra->clusterName();
        qDebug() << "+ Cassandra Cluster Name is" << name;
    }
    catch( std::overflow_error const & e )
    {
        qDebug() << "std::overflow_error caught -- " << e.what();
        ++err;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
    if(err > 0) {
        if(err == 1) {
            qDebug() << "1 error found.";
        }
        else {
            qDebug() << err << "errors found.";
        }
    }
#pragma GCC diagnostic pop

    exit(err == 0 ? 0 : 1);
}

// vim: ts=4 sw=4 et
