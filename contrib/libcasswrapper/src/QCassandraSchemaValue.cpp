/*
 * Text:
 *      QCassandraSchemaValue.cpp
 *
 * Description:
 *      Handling of the CQL interface.
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

#include "QtCassandra/QCassandraQuery.h"
#include "QtCassandra/QCassandraSchemaValue.h"
#include "CassTools.h"
#include "cassandra.h"

namespace QtCassandra
{

using namespace CassTools;

namespace QCassandraSchema
{


Value::Value()
    : f_type(TypeUnknown)
{
}


Value::Value( const QVariant& var )
    : f_type(TypeVariant)
    , f_variant(var)
{
}


void Value::readValue( iterator iter )
{
    value val( iter->get_meta_field_value() );
    readValue( val );
}


void Value::readValue( value val )
{
    f_value = val;
    parseValue();
}


void Value::encodeValue(QCassandraEncoder& encoder) const
{
    encoder.appendUnsignedCharValue(static_cast<unsigned char>(f_type));

    switch(f_type)
    {
    case TypeUnknown:
        // no data for this one
        break;

    case TypeVariant:
        // the type of a QVariant seems to be a byte however it is
        // not documented as such so we save it as a uint32_t
        encoder.appendUInt32Value(static_cast<uint32_t>(f_variant.type()));
        switch(f_variant.type())
        {
        case QVariant::Bool:
            encoder.appendSignedCharValue(f_variant.toBool() ? 1 : 0);
            break;

        case QVariant::ByteArray:
            encoder.appendBinaryValue(f_variant.toByteArray());
            break;

        case QVariant::String:
            encoder.appendP16StringValue(f_variant.toString());
            break;

        case QVariant::Double:
            encoder.appendDoubleValue(f_variant.toDouble());
            break;

        case QVariant::Int:
            encoder.appendInt32Value(f_variant.toInt());
            break;

        case QVariant::LongLong:
            encoder.appendInt64Value(f_variant.toLongLong());
            break;

        case QVariant::ULongLong:
            encoder.appendUInt64Value(f_variant.toULongLong());
            break;

        default:
            // other types are not supported, no data for those
            // (at this time we throw to make sure we capture
            // invalid data; otherwise the whole thing breaks
            // anyway...)
            throw std::runtime_error("unsupported QVariant type");

        }
        break;

    case TypeMap:
        // a map is a array of named values, first we save the size,
        // the the name / value pairs
        encoder.appendUInt16Value(f_map.size());
        for( auto m : f_map )
        {
            encoder.appendP16StringValue(m.first);
            m.second.encodeValue(encoder);
        }
        break;

    case TypeList:
        // a list is an array of values, first save the size, then
        // save each value
        encoder.appendUInt16Value(f_list.size());
        for(size_t idx(0); idx < f_list.size(); ++idx)
        {
            f_list[idx].encodeValue(encoder);
        }
        break;

    }
}


void Value::decodeValue(const QCassandraDecoder& decoder)
{
    f_type = static_cast<type_t>(decoder.unsignedCharValue());

    switch(f_type)
    {
    case TypeUnknown:
        // no data for this one
        break;

    case TypeVariant:
        // the type of a QVariant seems to be a byte however it is
        // not documented as such so we save it as a uint32_t
        {
            QVariant::Type const variant_type(static_cast<QVariant::Type>(decoder.uint32Value()));
            switch(variant_type)
            {
            case QVariant::Bool:
                f_variant = static_cast<bool>(decoder.signedCharValue());
                break;

            case QVariant::ByteArray:
                f_variant = decoder.binaryValue();
                break;

            case QVariant::String:
                f_variant = decoder.p16StringValue();
                break;

            case QVariant::Double:
                f_variant = decoder.doubleValue();
                break;

            case QVariant::Int:
                f_variant = decoder.int32Value();
                break;

            case QVariant::LongLong:
                f_variant = static_cast<qlonglong>(decoder.int64Value());
                break;

            case QVariant::ULongLong:
                f_variant = static_cast<qulonglong>(decoder.uint64Value());
                break;

            default:
                // other types are not supported, no data for those
                // (at this time we throw to make sure we capture
                // invalid data; otherwise the whole thing breaks
                // anyway...)
                throw std::runtime_error("unsupported QVariant type");

            }
        }
        break;

    case TypeMap:
        // a map is a array of named values, first we save the size,
        // the the name / value pairs
        {
            int const max_items(decoder.uint16Value());
            for(int idx(0); idx < max_items; ++idx)
            {
                QString const name(decoder.p16StringValue());
                Value value;
                value.decodeValue(decoder);
                f_map[name] = value;
            }
        }
        break;

    case TypeList:
        // a list is an array of values, first save the size, then
        // save each value
        {
            int const max_items(decoder.uint16Value());
            for(int idx(0); idx < max_items; ++idx)
            {
                Value value;
                value.decodeValue(decoder);
                f_list.push_back(value);
            }
        }
        break;

    }
}


void Value::parseValue()
{
    f_map.clear();
    f_list.clear();
    f_variant.clear();
    f_stringOutput.clear();

    switch( f_value.get_type() )
    {
        case CASS_VALUE_TYPE_UNKNOWN    :
        case CASS_VALUE_TYPE_CUSTOM     :
        case CASS_VALUE_TYPE_DECIMAL    :
        case CASS_VALUE_TYPE_LAST_ENTRY :
        case CASS_VALUE_TYPE_UDT        :
            f_type = TypeUnknown;
            break;

        case CASS_VALUE_TYPE_LIST       :
        case CASS_VALUE_TYPE_SET        :
            f_type = TypeList;
            parseList();
            break;

        case CASS_VALUE_TYPE_TUPLE      :
            f_type = TypeList;
            parseTuple();
            break;

        case CASS_VALUE_TYPE_MAP        :
            f_type = TypeMap;
            parseMap();
            break;

        case CASS_VALUE_TYPE_BLOB       :
        case CASS_VALUE_TYPE_BOOLEAN    :
        case CASS_VALUE_TYPE_FLOAT      :
        case CASS_VALUE_TYPE_DOUBLE     :
        case CASS_VALUE_TYPE_TINY_INT   :
        case CASS_VALUE_TYPE_SMALL_INT  :
        case CASS_VALUE_TYPE_INT        :
        case CASS_VALUE_TYPE_VARINT     :
        case CASS_VALUE_TYPE_BIGINT     :
        case CASS_VALUE_TYPE_COUNTER    :
        case CASS_VALUE_TYPE_ASCII      :
        case CASS_VALUE_TYPE_DATE       :
        case CASS_VALUE_TYPE_TEXT       :
        case CASS_VALUE_TYPE_TIME       :
        case CASS_VALUE_TYPE_TIMESTAMP  :
        case CASS_VALUE_TYPE_VARCHAR    :
        case CASS_VALUE_TYPE_UUID       :
        case CASS_VALUE_TYPE_TIMEUUID   :
        case CASS_VALUE_TYPE_INET       :
            f_type = TypeVariant;
            parseVariant();
            break;
    }
}


void Value::parseMap()
{
    iterator const iter( f_value.get_iterator_from_map() );
    while( iter.next() )
    {
        Value the_val;
        the_val.readValue( iter.get_map_value() );
        f_map[iter.get_map_key().get_string()] = the_val;
    }
}


void Value::parseList()
{
    iterator const iter( f_value.get_iterator_from_collection() );
    while( iter.next() )
    {
        Value val;
        val.readValue( iter.get_value() );
        f_list.push_back( val );
    }
}


void Value::parseTuple()
{
    iterator const iter( f_value.get_iterator_from_tuple() );
    while( iter.next() )
    {
        Value val;
        val.readValue( iter );
        f_list.push_back( val );
    }
}


void Value::parseVariant()
{
    switch( f_value.get_type() )
    {
        case CASS_VALUE_TYPE_BLOB       :
            f_variant = f_value.get_blob();
            break;

        case CASS_VALUE_TYPE_BOOLEAN    :
            f_variant = f_value.get_bool();
            break;

        case CASS_VALUE_TYPE_FLOAT      :
            f_variant = f_value.get_float();
            break;

        case CASS_VALUE_TYPE_DOUBLE     :
            f_variant = f_value.get_double();
            break;

        case CASS_VALUE_TYPE_TINY_INT  :
            f_variant = f_value.get_int8();
            break;

        case CASS_VALUE_TYPE_SMALL_INT :
            f_variant = f_value.get_int16();
            break;

        case CASS_VALUE_TYPE_VARINT    :
            f_variant = f_value.get_int32();
            break;

        case CASS_VALUE_TYPE_BIGINT     :
        case CASS_VALUE_TYPE_COUNTER    :
            f_variant = f_value.get_int64();
            break;

        case CASS_VALUE_TYPE_ASCII     :
        case CASS_VALUE_TYPE_DATE      :
        case CASS_VALUE_TYPE_TEXT      :
        case CASS_VALUE_TYPE_TIME      :
        case CASS_VALUE_TYPE_TIMESTAMP :
        case CASS_VALUE_TYPE_VARCHAR   :
            f_variant = f_value.get_string();
            break;

        case CASS_VALUE_TYPE_UUID      :
            f_variant = f_value.get_uuid();
            break;

        case CASS_VALUE_TYPE_TIMEUUID  :
            f_variant = f_value.get_uuid_timestamp();
            break;

        case CASS_VALUE_TYPE_INET      :
            f_variant = f_value.get_inet();
            break;

        default:
            throw std::runtime_error( "This type is not a bare type!" );
    }
}


const QString& Value::output() const
{
    if( f_stringOutput.isEmpty() )
    {
        switch( f_type )
        {
            case TypeUnknown:
                f_stringOutput = "''";
                break;

            case TypeVariant:
                if( f_variant.type() == QVariant::String )
                {
                    f_stringOutput = QString("'%1'").arg(f_variant.toString());
                }
                else
                {
                    f_stringOutput = f_variant.toString();
                }
                break;

            case TypeMap:
                {
                    QString content;
                    for( const auto& pair : f_map )
                    {
                        if( !content.isEmpty() )
                        {
                            content += ", ";
                        }
                        content += QString("'%1': %2")
                            .arg(pair.first)
                            .arg(pair.second.output())
                            ;
                    }
                    f_stringOutput = QString("{%1}").arg(content);
                }
                break;

            case TypeList:
                {
                    QString content;
                    for( const auto& entry : f_list )
                    {
                        if( !content.isEmpty() )
                        {
                            content += ", ";
                        }
                        content += entry.output();
                    }
                    f_stringOutput = QString("{%1}").arg(content);
                }
                break;
        }
    }

    return f_stringOutput;
}



} // namespace QCassandraSchema
} //namespace QtCassandra
// vim: ts=4 sw=4 et
