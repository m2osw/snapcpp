// Snap Websites Server -- manage sessions for users, forms, etc.
// Copyright (C) 2012-2014  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#include "dbutils.h"
#include "snap_exception.h"
#include "qstring_stream.h"
#include "log.h"

#include <iostream>

#include <QStringList>

#include "poison.h"


using namespace QtCassandra;

namespace snap
{

namespace
{
    char hex_to_dec(ushort c)
    {
        if(c >= '0' && c <= '9')
        {
            return static_cast<char>(c - '0');
        }
        if(c >= 'a' && c <= 'f')
        {
            return static_cast<char>(c - 'a' + 10);
        }
        if(c >= 'A' && c <= 'F')
        {
            return static_cast<char>(c - 'A' + 10);
        }
        throw snap_exception( "error: invalid hexadecimal digit, it cannot be converted." );
    }
}


dbutils::dbutils( const QString& table_name, const QString& row_name )
    : f_tableName(table_name)
    , f_rowName(row_name)
    , f_displayLen(64)
{
}


/** \brief Copy all the cells from one row to another.
 *
 * This function copies all the cells from one row to another. It does not
 * try to change anything in the process. The destination (source?) should
 * be tweaked as required on return.
 *
 * \warning
 * This function does not delete anything, if other fields already existed
 * in the destination, then it stays there.
 *
 * \param[in] ta  The source table.
 * \param[in] a  The name of the row to copy from.
 * \param[in] tb  The destination table.
 * \param[in] b  The name of the row to copy to.
 */
void dbutils::copy_row(QtCassandra::QCassandraTable::pointer_t ta, QString const& a, // source
                       QtCassandra::QCassandraTable::pointer_t tb, QString const& b) // destination
{
    QtCassandra::QCassandraRow::pointer_t source_row(ta->row(a));
    QtCassandra::QCassandraRow::pointer_t destination_row(tb->row(b));
    QtCassandra::QCassandraColumnRangePredicate column_predicate;
    column_predicate.setCount(1000); // we have to copy everything also it is likely very small (i.e. 10 fields...)
    column_predicate.setIndex(); // behave like an index
    for(;;)
    {
        source_row->clearCache();
        source_row->readCells(column_predicate);
        QtCassandra::QCassandraCells const source_cells(source_row->cells());
        if(source_cells.isEmpty())
        {
            // done
            break;
        }
        // handle one batch
        for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
                nc != source_cells.end();
                ++nc)
        {
            QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
            QByteArray cell_key(source_cell->columnKey());
            destination_row->cell(cell_key)->setValue(source_cell->value());
        }
    }
}



QByteArray dbutils::get_row_key() const
{
    QByteArray row_key;

    if(!f_rowName.isEmpty() && f_tableName == "files")
    {
        // these rows make use of MD5 sums so we have to convert them
        if(f_rowName == "new" || f_rowName == "javascripts" || f_rowName == "css")
        {
            row_key = f_rowName.toAscii();
        }
        else
        {
            QByteArray str(f_rowName.toUtf8());
            char const *s(str.data());
            while(s[0] != '\0' && s[1] != '\0')
            {
                char c(static_cast<char>((hex_to_dec(s[0]) << 4) | hex_to_dec(s[1])));
                row_key.append(c);
                s += 2;
            }
        }
    }
    else
    {
        row_key = f_rowName.toAscii();
    }

    return row_key;
}


/** \brief Transform a byte in an hexadecimal number.
 *
 * This function transforms a byte (a number from 0 to 255) to an ASCII
 * representation using hexadecimal.
 *
 * \param[in] byte  The byt to transform.
 *
 * \return The resulting string.
 */
QString dbutils::byte_to_hex( char const byte )
{
    const QString hex(QString("%1").arg(byte & 255, 2, 16, QChar('0')));
    return hex;
}


/** \brief Transform a binary key to hexadecimal.
 *
 * This function transforms each byte of a binary key to an ASCII string
 * of hexadecimal numbers using the byte_to_hex() function.
 *
 * \param[in] key  The key to transform.
 *
 * \return The resulting string.
 */
QString dbutils::key_to_string( QByteArray const& key )
{
    QString ret;
    int const max_length(key.size());
    for(int i(0); i < max_length; ++i)
    {
        ret += byte_to_hex( key[i] );
    }
    return ret;
}


QByteArray dbutils::string_to_key( const QString& str )
{
    QByteArray ret;
    QStringList numList( str.split(' ') );

    for( auto str_num : numList )
    {
        bool ok( false );
        ret.push_back( static_cast<char>( str_num.toInt( &ok, 16 ) ) );
        if( !ok )
        {
            throw snap_exception( "Cannot convert to num! Not base 16." );
        }
    }
    return ret;
}


int dbutils::get_display_len() const
{
    return f_displayLen;
}


void dbutils::set_display_len( const int val )
{
    f_displayLen = val;
}


QString dbutils::get_row_name( QCassandraRow::pointer_t p_r ) const
{
    QString ret;
    if(f_tableName == "files")
    {
        // these are raw MD5 keys
        QByteArray key(p_r->rowKey());
        if(key.size() == 16)
        {
            ret = key_to_string( key );
        }
        else
        {
            ret = key;
        }
    }
    else
    {
        ret = p_r->rowName();
    }

    return ret;
}


QString dbutils::get_column_name( QCassandraCell::pointer_t c ) const
{
    QString const content_attachment_reference( "content::attachment::reference::" );

    QByteArray const key( c->columnKey() );

    QString name;
    if(f_tableName == "files" && f_rowName == "new")
    {
        name = key_to_string( key );
    }
    else if(f_tableName == "list" && f_rowName != "*standalone*")
    {
        uint64_t time(QtCassandra::uint64Value(key, 0));
        char buf[64];
        struct tm t;
        time_t const seconds(time / 1000000);
        gmtime_r(&seconds, &t);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
        name = QString("%1.%2 (%3) %4").arg(buf).arg(time % 1000000, 6, 10, QChar('0')).arg(time).arg(QtCassandra::stringValue(key, sizeof(uint64_t)));
    }
    else if(f_tableName == "data" && (key.startsWith(content_attachment_reference.toAscii())) )
    {
        name = content_attachment_reference;
        name += key_to_string( key.mid( content_attachment_reference.length()+1 ) );
    }
    else if((f_tableName == "files" && f_rowName == "javascripts")
         || (f_tableName == "files" && f_rowName == "css"))
    {
        // this row name is "<name>"_"<browser>"_<version as integers>
        int const max_length(key.size());
        int sep(0);
        int i(0);
        for(; i < max_length && sep < 2; ++i)
        {
            if(key[i] == '_')
            {
                ++sep;
            }
            name += key[i];
        }
        // now we have to add the version
        bool first(true);
        for(; i + 3 < max_length; i += 4)
        {
            if(first)
            {
                first = false;
            }
            else
            {
                name += ".";
            }
            name += QString("%1").arg(QtCassandra::uint32Value(key, i));
        }
    }
    else if((f_tableName == "users"    && f_rowName == "*index_row*")
         || (f_tableName == "shorturl" && f_rowName.endsWith("/*index_row*")))
    {
        // special case where the column key is a 64 bit integer
        //const QByteArray& name(c->columnKey());
        QtCassandra::QCassandraValue const identifier(c->columnKey());
        name = QString("%1").arg(identifier.int64Value());
    }
    else
    {
        name = c->columnName();
    }

    return name;
}


dbutils::column_type_t dbutils::get_column_type( QCassandraCell::pointer_t c ) const
{
    QString const n( get_column_name( c ) );

    if(n == "users::identifier"
    || n == "shorturl::identifier"
    )
    {
        return CT_uint64_value;
    }
    else if(n == "content::created"
         || n == "content::files::created"
         || n == "content::files::creation_time"
         || n == "content::files::modification_time"
         || n == "content::files::secure::last_check"
         || n == "content::files::updated"
         || n == "content::modified"
         || n == "content::updated"
         || n.left(18) == "core::last_updated"
         || n == "core::plugin_threshold"
         || n == "list::last_updated"
         || n == "sessions::date"
         || n == "shorturl::date"
         || n == "users::created_time"
         || n == "users::forgot_password_on"
         || n == "users::login_on"
         || n == "users::logout_on"
         || n == "users::previous_login_on"
         || n == "users::start_date"
         || n == "users::verified_on"
         )
    {
        // 64 bit value (microseconds)
        return CT_time_microseconds;
    }
    else if(n == "sessions::login_limit"
         || n == "sessions::time_limit"
         )
    {
        // 64 bit value (seconds)
        return CT_time_seconds;
    }
    else if(n == "sitemapxml::priority"
         )
    {
        // 32 bit float
        return CT_float32_value;
    }
    else if(n.startsWith("content::attachment::reference::")
         || n == "content::attachment::revision_control::last_branch"
         || n.startsWith("content::attachment::revision_control::last_revision::")
         || n == "content::files::image_height"
         || n == "content::files::image_width"
         || n == "content::files::size"
         || n == "content::files::size::compressed"
         || n == "content::revision_control::attachment::current_branch"
         || n == "content::revision_control::attachment::current_working_branch"
         || n == "content::revision_control::current_branch"
         || n == "content::revision_control::current_working_branch"
         || n == "content::revision_control::last_branch"
         || n == "content::revision_control::attachment::last_branch"
         || n.startsWith("content::revision_control::attachment::current_revision::")
         || n.startsWith("content::revision_control::attachment::current_working_revision::")
         || n.startsWith("content::revision_control::current_revision::")
         || n.startsWith("content::revision_control::current_working_revision::")
         || n.startsWith("content::revision_control::last_revision::")
         || n.startsWith("content::revision_control::attachment::last_revision::")
         || n == "sitemapxml::count"
         || n == "sessions::id"
         || n == "sessions::time_to_live"
         || (f_tableName == "libQtCassandraLockTable" && f_rowName == "hosts")
         )
    {
        return CT_uint32_value;
    }
    else if(n == "content::final"
         || n == "content::files::compressor"
         || n.startsWith("content::files::reference::")
         || n == "favicon::sitewide"
         || n == "sessions::used_up"
         || (f_tableName == "files" && f_rowName == "new")
         )
    {
        // unsigned 8 bit value
        // cast to integer so arg() doesn't take it as a character
        return CT_uint8_value;
    }
    else if(n == "content::prevent_delete"
         || n == "permissions::dynamic"
         || n == "finball::read_terms_n_conditions" // TODO -- remove at some point since that a customer's type (we'd need to have an XML file instead)
         || (f_tableName == "list" && f_rowName != "*standalone*")
         )
    {
        // signed 8 bit value
        // cast to integer so arg() doesn't take it as a character
        return CT_int8_value;
    }
    else if(n == "sessions::random"
         || n == "users::password::salt"
         || n == "users::password"
         )
    {
        // n bit binary value
        return CT_hexarray_value;
    }
    else if(n == "favicon::icon"
         || n == "content::files::data"
         || n == "content::files::data::compressed"
         || f_tableName == "layout"
         )
    {
        // n bit binary value
        // same as previous only this can be huge so we limit it
        return CT_hexarray_limited_value;
    }
    else if((f_tableName == "data" && n == "content::attachment")
         || (f_tableName == "files" && f_rowName == "javascripts")
         || (f_tableName == "files" && f_rowName == "css")
         )
    {
        // md5 in binary
        return CT_md5array_value;
    }
    else if(n == "content::files::secure")
    {
        return CT_secure_value;
    }

    // all others viewed as strings
    return CT_string_value;
}


QString dbutils::get_column_value( QCassandraCell::pointer_t c, const bool display_only ) const
{
    QString v;
    try
    {
        switch( column_type_t ct = get_column_type( c ) )
        {
            case CT_uint64_value:
            {
                v = QString("%1").arg(c->value().uint64Value());
            }
            break;

            case CT_time_microseconds:
            {
                // 64 bit value (microseconds)
                uint64_t time(c->value().uint64Value());
                if(time == 0)
                {
                    v = "time not set (0)";
                }
                else
                {
                    char buf[64];
                    struct tm t;
                    time_t const seconds(time / 1000000);
                    gmtime_r(&seconds, &t);
                    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
                    v = QString("%1.%2 (%3)").arg(buf).arg(time % 1000000, 6, 10, QChar('0')).arg(time);
                }
            }
            break;

            case CT_time_seconds:
            {
                // 64 bit value (seconds)
                uint64_t time(c->value().uint64Value());
                char buf[64];
                struct tm t;
                time_t const seconds(time);
                gmtime_r(&seconds, &t);
                strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
                v = display_only
                  ? QString("%1 (%2)").arg(buf).arg(time)
                  : QString("%1").arg(buf);
            }
            break;

            case CT_float32_value:
            {
                // 32 bit float
                float value(c->value().floatValue());
                v = QString("%1").arg(value);
            }
            break;

            case CT_uint32_value:
            {
                // 32 bit value
                v = QString("%1").arg(c->value().uint32Value());
            }
            break;

            case CT_int8_value:
            {
                // signed 8 bit value
                // cast to integer so arg() doesn't take it as a character
                v = QString("%1").arg(static_cast<int>(c->value().signedCharValue()));
            }
            break;

            case CT_uint8_value:
            {
                // unsigned 8 bit value
                // cast to integer so arg() doesn't take it as a character
                v = QString("%1").arg(static_cast<int>(c->value().unsignedCharValue()));
            }
            break;

            case CT_hexarray_value:
            case CT_hexarray_limited_value:
            {
                // n bit binary value
                bool const display_limited( display_only && (ct == CT_hexarray_limited_value) );
                const QByteArray& buf(c->value().binaryValue());
                int const max_length( display_limited? std::min(f_displayLen, buf.size()): buf.size() );
                if( display_only )
                {
                    v += "(hex) ";
                }
                for(int i(0); i < max_length; ++i)
                {
                    v += byte_to_hex(buf[i]) + " ";
                }
                if( display_limited && (buf.size() > max_length) )
                {
                    v += "...";
                }
            }
            break;

            case CT_md5array_value:
            {
                // md5 in binary
                const QByteArray& buf(c->value().binaryValue());
                int const max_length(buf.size());
                if( display_only )
                {
                    v += "(md5) ";
                }
                for(int i(0); i < max_length; ++i)
                {
                    v += byte_to_hex(buf[i]);
                }
            }
            break;

            case CT_secure_value:
            {
                switch(c->value().signedCharValue())
                {
                case -1:
                    v = "not checked (-1)";
                    break;

                case 0:
                    v = "not secure (0)";
                    break;

                case 1:
                    v = "secure (1)";
                    break;

                default:
                    v = QString("unknown secure status (%1)").arg(c->value().signedCharValue());
                    break;

                }
            }
            break;

            case CT_string_value:
            {
                v = c->value().stringValue().replace("\n", "\\n");
            }
            break;

        }
    }
    catch(std::runtime_error const& e)
    {
        SNAP_LOG_ERROR() << "error: caught a runtime exception dealing with \"" << get_column_name(c) << "\" (" << e.what() << ")";
        // TBD: just rethrow?
        //throw;
        v = "ERROR DETECTED";
    }

    return v;
}


void dbutils::set_column_value( QCassandraCell::pointer_t c, const QString& v )
{
    QCassandraValue cvalue;
    //
    switch( get_column_type( c ) )
    {
        case CT_uint64_value:
        {
            cvalue.setUInt64Value( static_cast<uint64_t>(v.toULongLong()) );
        }
        break;

        case CT_time_microseconds:
        case CT_time_seconds:
        {
            // String will be of the form: "%Y-%m-%d %H:%M:%S"
            //
            const QStringList datetime_split ( v.split(' ') );
            const QStringList date_split     ( datetime_split[0].split('-') );
            const QStringList time_split     ( datetime_split[1].split(':') );
            //
            tm to;
            to.tm_sec  = time_split[2].toInt();
            to.tm_min  = time_split[1].toInt();
            to.tm_hour = time_split[0].toInt();
            to.tm_yday = date_split[2].toInt();
            to.tm_mon  = date_split[1].toInt();
            to.tm_year = date_split[0].toInt();
            //
            const time_t tt( mktime( &to ) );
            cvalue.setUInt64Value( tt );
        }
        break;

        case CT_float32_value:
        {
            cvalue.setFloatValue( v.toFloat() );
        }
        break;

        case CT_uint32_value:
        {
            cvalue.setUInt32Value( static_cast<uint32_t>(v.toULong()) );
        }
        break;

        case CT_int8_value:
        {
            cvalue.setSignedCharValue( static_cast<signed char>(v.toInt()) );
        }
        break;

        case CT_uint8_value:
        {
            cvalue.setUnsignedCharValue( static_cast<unsigned char>(v.toUInt()) );
        }
        break;

        case CT_hexarray_value:
        case CT_hexarray_limited_value:
        {
            cvalue.setBinaryValue( string_to_key( v ) );
        }
        break;

        case CT_md5array_value:
        {
            cvalue.setBinaryValue( string_to_key( v ) );
        }
        break;

        case CT_secure_value:
        {
            signed char cv;
            if( v == "not checked (-1)" || v == "-1" )
            {
                cv = -1;
            }
            else if( v == "not secure (0)" || v == "0" )
            {
                cv = 0;
            }
            else if( v == "secure (1)" || v == "1" )
            {
                cv = 1;
            }
            else
            {
                throw snap_exception( "error: unknown secure value! Must be -1, 0 or 1!" );
            }
            cvalue.setSignedCharValue( cv );
        }
        break;

        case CT_string_value:
        {
            // all others viewed as strings
            //v = c->value().stringValue().replace("\n", "\\n");
            QString convert( v );
            cvalue.setStringValue( convert.replace( "\\n", "\n" ) );
        }
        break;
    }

    c->setValue( cvalue );
}


}
// namespace snap

// vim: ts=4 sw=4 et
