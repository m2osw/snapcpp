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

using namespace QtCassandra;

namespace snap
{

namespace
{
    char hex_to_dec(ushort c)
    {
        if(c >= '0' && c <= '9')
        {
            return c - '0';
        }
        if(c >= 'a' && c <= 'f')
        {
            return c - 'a' + 10;
        }
        if(c >= 'A' && c <= 'F')
        {
            return c - 'A' + 10;
        }
        throw snap_exception( "error: invalid hexadecimal digit, it cannot be converted." );
    }
}


dbutils::dbutils( const QString& table_name, const QString& row_name )
    : f_tableName(table_name)
    , f_rowName(row_name)
{
}


QByteArray dbutils::get_row_key() const
{
    QByteArray row_key;

    if(!f_rowName.isEmpty() && f_tableName == "files")
    {
        // these rows make use of MD5 sums so we have to convert them
        if(f_rowName == "new" || f_rowName == "javascripts")
        {
            row_key = f_rowName.toAscii();
        }
        else
        {
            QByteArray str(f_rowName.toUtf8());
            char const *s(str.data());
            while(s[0] != '\0' && s[1] != '\0')
            {
                char c((hex_to_dec(s[0]) << 4) | hex_to_dec(s[1]));
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


QString dbutils::byte_to_hex( const char byte )
{
    const QString hex(QString("%1").arg(byte & 255, 2, 16, QChar('0')));
    return hex;
}


QString dbutils::key_to_string( const QByteArray& key )
{
    QString ret;
    int const max(key.size());
    for(int i(0); i < max; ++i)
    {
        ret += byte_to_hex( key[i] );
    }
    return ret;
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
    else if(f_tableName == "data" && (key.startsWith(content_attachment_reference.toAscii())) )
    {
        name = content_attachment_reference;
        name += key_to_string( key.mid( content_attachment_reference.length()+1 ) );
    }
    else if(f_tableName == "files" && f_rowName == "javascripts")
    {
        // this row name is "<name>"_"<browser>"_<version as integers>
        int const max(key.size());
        int sep(0);
        int i(0);
        for(; i < max && sep < 2; ++i)
        {
            if(key[i] == '_')
            {
                ++sep;
            }
            name += key[i];
        }
        // now we have to add the version
        bool first(true);
        for(; i + 3 < max; i += 4)
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


QString dbutils::get_column_value( QCassandraCell::pointer_t c ) const
{
    QString const n( get_column_name( c ) );

    QString v;
    if(n == "users::identifier"
    || n == "permissions::dynamic"
    || n == "shorturl::identifier"
    )
    {
        // 64 bit value
        v = QString("%1").arg(c->value().uint64Value());
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
        uint64_t time(c->value().uint64Value());
        if(time == 0)
        {
            v = "time not set (0)";
        }
        else
        {
            char buf[64];
            struct tm t;
            time_t seconds(time / 1000000);
            gmtime_r(&seconds, &t);
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
            v = QString("%1.%2 (%3)").arg(buf).arg(time % 1000000, 6, 10, QChar('0')).arg(time);
        }
    }
    else if(n == "sessions::login_limit"
         || n == "sessions::time_limit"
         )
    {
        // 64 bit value (seconds)
        uint64_t time(c->value().uint64Value());
        char buf[64];
        struct tm t;
        time_t seconds(time);
        gmtime_r(&seconds, &t);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
        v = QString("%1 (%2)").arg(buf).arg(time);
    }
    else if(n == "sitemapxml::priority"
         )
    {
        // 32 bit float
        float value(c->value().floatValue());
        v = QString("%1").arg(value);
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
        // 32 bit value
        v = QString("%1").arg(c->value().uint32Value());
    }
    else if(n == "sessions::used_up"
         || n == "content::final"
         || n == "favicon::sitewide"
         || n == "content::files::compressor"
         || n.startsWith("content::files::reference::")
         || (f_tableName == "files" && f_rowName == "new")
         )
    {
        // 8 bit value
        // cast to integer so arg() doesn't take it as a character
        v = QString("%1").arg(static_cast<int>(c->value().unsignedCharValue()));
    }
    else if(n == "sessions::random"
         || n == "users::password::salt"
         || n == "users::password"
         )
    {
        // n bit binary value
        const QByteArray& buf(c->value().binaryValue());
        int const max(buf.size());
        v += "(hex) ";
        for(int i(0); i < max; ++i)
        {
            v += byte_to_hex(buf[i]) + " ";
        }
    }
    else if(n == "favicon::icon"
         || n == "content::files::data"
         || n == "content::files::data::compressed"
         || f_tableName == "layout"
         )
    {
        // n bit binary value
        // same as previous only this can be huge so we limit it
        const QByteArray& buf(c->value().binaryValue());
        int const max(std::min(64, buf.size()));
        v += "(hex) ";
        for(int i(0); i < max; ++i)
        {
            v += byte_to_hex(buf[i]) + " ";
        }
        if(buf.size() > max)
        {
            v += "...";
        }
    }
    else if((f_tableName == "data" && n == "content::attachment")
         || (f_tableName == "files" && f_rowName == "javascripts")
         )
    {
        // md5 in binary
        const QByteArray& buf(c->value().binaryValue());
        int const max(buf.size());
        v += "(md5) ";
        for(int i(0); i < max; ++i)
        {
            v += byte_to_hex(buf[i]);
        }
    }
    else if(n == "content::files::secure")
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
    else
    {
        // all others viewed as strings
        v = c->value().stringValue().replace("\n", "\\n");
    }
    //
    return n + " = " + v;
}

}
// namespace snap

// vim: ts=4 sw=4 et
