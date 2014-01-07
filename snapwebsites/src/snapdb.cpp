/*
 * Text:
 *      snapdb.cpp
 *
 * Description:
 *      Reads and describes a Snap database. This ease checking out the
 *      current content of the database as the cassandra-cli tends to
 *      show everything in hexadecimal number which is quite unpractical.
 *      Now we do it that way for runtime speed which is much more important
 *      than readability by humans, but we still want to see the data in an
 *      easy practical way which this tool offers.
 *
 * License:
 *      Copyright (c) 2012-2014 Made to Order Software Corp.
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
#include <controlled_vars/controlled_vars_need_init.h>
#include <advgetopt/advgetopt.h>
#include <algorithm>
#include <iostream>
#include "qstring_stream.h"

namespace
{
    const std::vector<std::string> g_configuration_files; // Empty

    const advgetopt::getopt::option g_snapdb_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: snapdb [--opts] [table [row]]",
            advgetopt::getopt::help_argument
        },
        // no args
        {
            '\0',
            0,
            NULL,
            NULL,
            "without arguments, all tables are outputted for the current context.",
            advgetopt::getopt::help_argument
        },
        // OPTIONS
        {
            '\0',
            0,
            NULL,
            NULL,
            "options:",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            NULL,
            "show this help output",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "context",
            NULL,
            "name of the context from which to read",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "count",
            NULL,
            "specify the number of rows to display",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "drop-tables",
            NULL,
            "drop all the content tables of the specified context",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "drop-all-tables",
            NULL,
            "drop absolutely all the tables",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "host",
            NULL,
            "host IP address or name (defaults to localhost)",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "port",
            NULL,
            "port on the host to connect to (defaults to 9160)",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "info",
            NULL,
            "print out the cluster name and protocol version",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::default_multiple_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::end_of_options
        }
    };
}
//namespace

using namespace QtCassandra;

/** \brief A class for easy access to all resources.
 *
 * This class is just so we use resource in an object oriented
 * manner rather than having globals, but that's clearly very
 * similar here!
 */
class snapdb
{
public:
    snapdb(int argc, char *argv[]);

    void usage(advgetopt::getopt::status_t status);
    void info();
    void drop_tables(bool all);
    void display();

private:
    typedef std::shared_ptr<advgetopt::getopt>    getopt_ptr_t;

    QCassandra                      f_cassandra;
    QString                         f_host;
    controlled_vars::mint32_t       f_port;
    controlled_vars::mint32_t       f_count;
    QString                         f_context;
    QString                         f_table;
    QString                         f_row;
    QByteArray                      f_row_key;
    getopt_ptr_t                    f_opt;
};

snapdb::snapdb(int argc, char *argv[])
    //: f_cassandra() -- auto-init
    : f_host("localhost") // default
    , f_port(9160) //default
    , f_count(100)
    , f_context("snap_websites")
    //, f_table("") -- auto-init
    //, f_row("") -- auto-init
    , f_opt( new advgetopt::getopt( argc, argv, g_snapdb_options, g_configuration_files, NULL ) )
{
    // first check options
    if( f_opt->is_defined( "count" ) )
    {
        f_count = f_opt->get_long( "count" );
    }
    if( f_opt->is_defined( "host" ) )
    {
        f_host = f_opt->get_string( "host" ).c_str();
    }
    if( f_opt->is_defined( "port" ) )
    {
        f_port = f_opt->get_long( "port" );
    }
    if( f_opt->is_defined( "context" ) )
    {
        f_context = f_opt->get_string( "context" ).c_str();
    }

    // then check commands
    if( f_opt->is_defined( "help" ) )
    {
        usage(advgetopt::getopt::no_error);
    }
    if( f_opt->is_defined( "info" ) )
    {
        info();
        exit(0);
    }
    if( f_opt->is_defined( "drop-tables" ) )
    {
        drop_tables(false);
        exit(0);
    }
    if( f_opt->is_defined( "drop-all-tables" ) )
    {
        drop_tables(true);
        exit(0);
    }

    // finally check for parameters
    if( f_opt->is_defined( "--" ) )
    {
        const int arg_count = f_opt->size( "--" );
        if( arg_count >= 3 )
        {
            std::cerr << "error: only two parameters (table and row) can be specified on the command line." << std::endl;
            usage(advgetopt::getopt::error);
        }
        for( int idx = 0; idx < arg_count; ++idx )
        {
            if( idx == 0 )
            {
                f_table = f_opt->get_string( "--", idx ).c_str();
            }
            else if( idx == 1 )
            {
                f_row = f_opt->get_string( "--", idx ).c_str();
            }
        }
    }
}

void snapdb::usage(advgetopt::getopt::status_t status)
{
    f_opt->usage( status, "snapdb" );
    exit(1);
}

void snapdb::info()
{
    f_cassandra.connect(f_host, f_port);
    if(f_cassandra.isConnected())
    {
        std::cout << "Working on Cassandra Cluster Named \""    << f_cassandra.clusterName()     << "\"." << std::endl;
        std::cout << "Working on Cassandra Protocol Version \"" << f_cassandra.protocolVersion() << "\"." << std::endl;
        exit(0);
    }
    else
    {
        std::cerr << "The connection failed!" << std::endl;
        exit(1);
    }
}


void snapdb::drop_tables(bool all)
{
    f_cassandra.connect(f_host, f_port);
    QSharedPointer<QCassandraContext> context(f_cassandra.context(f_context));

    // there are re-created when we connect and refilled when
    // we access a page; obviously this is VERY dangerous on
    // a live system!
    context->dropTable("content");
    context->dropTable("emails");
    context->dropTable("files");
    context->dropTable("layout");
    context->dropTable("libQtCassandraLockTable");
    context->dropTable("links");
    context->dropTable("shorturl");
    context->dropTable("sites");
    context->dropTable("sessions");
    context->dropTable("users");

    if(all)
    {
        // for those who also want to test the snapmanager work too
        context->dropTable("domains");
        context->dropTable("websites");
    }
}

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
    std::cerr << "error: invalid hexadecimal digit, it cannot be converted." << std::endl;
    exit(1);
}

void snapdb::display()
{
    f_cassandra.connect(f_host, f_port);
    QSharedPointer<QCassandraContext> context(f_cassandra.context(f_context));

    if(!f_row.isEmpty() && f_table == "files")
    {
        // these rows make use of MD5 sums so we have to convert them
        if(f_row == "new" || f_row == "javascripts")
        {
            f_row_key = f_row.toAscii();
        }
        else
        {
            QByteArray str(f_row.toUtf8());
            char const *s(str.data());
            while(s[0] != '\0' && s[1] != '\0')
            {
                char c((hex_to_dec(s[0]) << 4) | hex_to_dec(s[1]));
                f_row_key.append(c);
                s += 2;
            }
        }
    }
    else
    {
        f_row_key = f_row.toAscii();
    }

    if(f_table.isEmpty())
    {
        // list of all the tables
        const QCassandraTables& tables(context->tables());
        for(QCassandraTables::const_iterator t(tables.begin());
                    t != tables.end();
                    ++t)
        {
            std::cout << (*t)->tableName() << std::endl;
        }
    }
    else if(f_row.isEmpty())
    {
        // list of rows in that table
        QSharedPointer<QCassandraTable> table(context->findTable(f_table));
        if(table.isNull())
        {
            std::cerr << "error: table \"" << f_table << "\" not found." << std::endl;
            exit(1);
        }
        QCassandraRowPredicate row_predicate;
        row_predicate.setCount(f_count);
        table->readRows(row_predicate);
        const QCassandraRows& rows(table->rows());
        for(QCassandraRows::const_iterator r(rows.begin());
                    r != rows.end();
                    ++r)
        {
            if(f_table == "files")
            {
                // these are raw MD5 keys
                QByteArray key((*r)->rowKey());
                if(key.size() != 16)
                {
                    std::cout << key.data();
                }
                else
                {
                    int const max(key.size());
                    for(int i(0); i < max; ++i)
                    {
                        QString hex(QString("%1").arg(key[i] & 255, 2, 16, QChar('0')));
                        std::cout << hex;
                    }
                }
                std::cout << std::endl;
            }
            else
            {
                std::cout << (*r)->rowName().toUtf8().data() << std::endl;
            }
        }
    }
    else if(f_row.endsWith("%"))
    {
        // list of rows in that table
        QSharedPointer<QCassandraTable> table(context->findTable(f_table));
        if(table.isNull())
        {
            std::cerr << "error: table \"" << f_table << "\" not found." << std::endl;
            exit(1);
        }
        QCassandraRowPredicate row_predicate;
        QString row_start(f_row.left(f_row.length() - 1));
        // remember that the start/end on row doesn't work in "alphabetical"
        // order so we cannot use it here...
        //row_predicate.setStartRowName(row_start);
        //row_predicate.setEndRowName(row_start + QCassandraColumnPredicate::first_char);
        row_predicate.setCount(f_count);
        for(;;)
        {
            table->clearCache();
            table->readRows(row_predicate);
            const QCassandraRows& rows(table->rows());
            if(rows.isEmpty())
            {
                break;
            }
            for(QCassandraRows::const_iterator r(rows.begin());
                        r != rows.end();
                        ++r)
            {
                QString name((*r)->rowName());
                if(name.length() >= row_start.length()
                && row_start == name.mid(0, row_start.length()))
                {
                    std::cout << name << std::endl;
                }
            }
        }
    }
    else
    {
        // display all the columns of a row
        QSharedPointer<QCassandraTable> table(context->findTable(f_table));
        if(table.isNull())
        {
            std::cerr << "error: table \"" << f_table << "\" not found." << std::endl;
            exit(1);
        }
        if(!table->exists(f_row_key))
        {
            std::cerr << "error: row \"" << f_row << "\" not found in table \"" << f_table << "\"." << std::endl;
            exit(1);
        }
        QSharedPointer<QCassandraRow> row(table->row(f_row_key));
        QCassandraColumnRangePredicate column_predicate;
        column_predicate.setCount(f_count);
        column_predicate.setIndex();
        for(;;)
        {
            row->clearCache();
            row->readCells(column_predicate);
            const QCassandraCells& cells(row->cells());
            if(cells.isEmpty())
            {
                break;
            }
            for(QCassandraCells::const_iterator c(cells.begin());
                        c != cells.end();
                        ++c)
            {
                QString n;
                if(f_table == "files" && f_row == "new")
                {
                    // TODO write a function to convert MD5, SHA1, etc.
                    QByteArray key((*c)->columnKey());
                    int const max(key.size());
                    for(int i(0); i < max; ++i)
                    {
                        QString hex(QString("%1").arg(key[i] & 255, 2, 16, QChar('0')));
                        n += hex;
                    }
                }
                else if(f_table == "files" && f_row == "javascripts")
                {
                    // this row name is "<name>"_"<browser>"_<version as integers>
                    QByteArray key((*c)->columnKey());
                    int const max(key.size());
                    int sep(0);
                    int i(0);
                    for(; i < max && sep < 2; ++i)
                    {
                        if(key[i] == '_')
                        {
                            ++sep;
                        }
                        n += key[i];
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
                            n += ".";
                        }
                        n += QString("%1").arg(QtCassandra::uint32Value(key, i));
                    }
                }
                else if((f_table == "users"    && f_row == "*index_row*")
                     || (f_table == "shorturl" && f_row.endsWith("/*index_row*"))
                )
                {
                    // special case where the column key is a 64 bit integer
                    //const QByteArray& name((*c)->columnKey());
                    QtCassandra::QCassandraValue identifier((*c)->columnKey());
                    n = QString("%1").arg(identifier.int64Value());
                }
                else
                {
                    n = (*c)->columnName();
                }
                QString v;
                if(n == "users::identifier"
                || n == "permissions::dynamic"
                || n == "shorturl::identifier"
                )
                {
                    // 64 bit value
                    v = QString("%1").arg((*c)->value().uint64Value());
                }
                else if(n == "content::created"
                     || n == "content::files::created"
                     || n == "content::files::secure::last_check"
                     || n == "content::files::updated"
                     || n == "content::modified"
                     || n == "content::updated"
                     || n.left(18) == "core::last_updated"
                     || n == "core::plugin_threshold"
                     || n == "sessions::date"
                     || n == "shorturl::date"
                     || n == "users::created_time"
                     || n == "users::login_on"
                     || n == "users::logout_on"
                     || n == "users::previous_login_on"
                     || n == "users::start_date"
                     || n == "users::verified_on"
                )
                {
                    // 64 bit value (microseconds)
                    uint64_t time((*c)->value().uint64Value());
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
                    uint64_t time((*c)->value().uint64Value());
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
                    float value((*c)->value().floatValue());
                    v = QString("%1").arg(value);
                }
                else if(n == "content::files::image_height"
                     || n == "content::files::image_width"
                     || n == "content::files::size"
                     || n == "content::files::size::compressed"
                     || n == "sitemapxml::count"
                     || n == "sessions::id"
                     || n == "sessions::time_to_live"
                     || (f_table == "libQtCassandraLockTable" && f_row == "hosts")
                )
                {
                    // 32 bit value
                    v = QString("%1").arg((*c)->value().uint32Value());
                }
                else if(n == "sessions::used_up"
                     || n == "content::final"
                     || n == "favicon::sitewide"
                     || n == "content::files::compressor"
                     || n.startsWith("content::files::reference::")
                     || (f_table == "files" && f_row == "new")
                )
                {
                    // 8 bit value
                    // cast to integer so arg() doesn't take it as a character
                    v = QString("%1").arg(static_cast<int>((*c)->value().unsignedCharValue()));
                }
                else if(n == "sessions::random"
                     || n == "users::password::salt"
                     || n == "users::password"
                )
                {
                    // n bit binary value
                    const QByteArray& buf((*c)->value().binaryValue());
                    int const max(buf.size());
                    v += "(hex) ";
                    for(int i(0); i < max; ++i)
                    {
                        v += QString("%1 ").arg(static_cast<int>(static_cast<unsigned char>(buf.at(i))), 2, 16, QChar('0'));
                    }
                }
                else if(n == "favicon::icon"
                     || n == "content::files::data"
                     || n == "content::files::data::compressed"
                )
                {
                    // n bit binary value
                    // same as previous only this can be huge so we limit it
                    const QByteArray& buf((*c)->value().binaryValue());
                    int const max(std::min(64, buf.size()));
                    v += "(hex) ";
                    for(int i(0); i < max; ++i)
                    {
                        v += QString("%1 ").arg(static_cast<int>(static_cast<unsigned char>(buf.at(i))), 2, 16, QChar('0'));
                    }
                    if(buf.size() > max)
                    {
                        v += "...";
                    }
                }
                else if(n == "content::attachment"
                     || (f_table == "files" && f_row == "javascripts")
                )
                {
                    // md5 in binary
                    const QByteArray& buf((*c)->value().binaryValue());
                    int const max(buf.size());
                    v += "(md5) ";
                    for(int i(0); i < max; ++i)
                    {
                        v += QString("%1").arg(static_cast<int>(static_cast<unsigned char>(buf.at(i))), 2, 16, QChar('0'));
                    }
                }
                else if(n == "content::files::secure")
                {
                    switch((*c)->value().signedCharValue())
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
                        v = QString("unknown secure status (%1)").arg((*c)->value().signedCharValue());
                        break;

                    }
                }
                else
                {
                    // all others viewed as strings
                    v = (*c)->value().stringValue().replace("\n", "\\n");
                }
                //
                std::cout << n << " = " << v << std::endl;
            }
        }
    }
}



int main(int argc, char *argv[])
{
    snapdb  s(argc, argv);

    s.display();

    return 0;
}

// vim: ts=4 sw=4 et
