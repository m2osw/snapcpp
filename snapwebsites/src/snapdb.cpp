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

// system
//
#include <QtCassandra/QCassandra.h>
#include <controlled_vars/controlled_vars_need_init.h>
#include <advgetopt/advgetopt.h>
#include <algorithm>
#include <iostream>
#include <sstream>

// lib
//
#include "qstring_stream.h"
#include "dbutils.h"

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
            "Usage: %p [-<opt>] [table [row]]",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "where -<opt> is one or more of:",
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
            0,
            "context",
            NULL,
            "name of the context from which to read",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "count",
            NULL,
            "specify the number of rows to display",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "drop-tables",
            NULL,
            "drop all the content tables of the specified context",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "drop-all-tables",
            NULL,
            "drop absolutely all the tables",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "yes-i-know-what-im-doing",
            NULL,
            "Force the dropping of tables, without warning and stdin prompt. Only use this if you know what you're doing!",
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
            "[table [row]]",
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

    QCassandra::pointer_t           f_cassandra;
    QString                         f_host;
    controlled_vars::mint32_t       f_port;
    controlled_vars::mint32_t       f_count;
    QString                         f_context;
    QString                         f_table;
    QString                         f_row;
    getopt_ptr_t                    f_opt;

    bool confirm_drop_check() const;
    void display_tables() const;
    void display_rows() const;
    void display_rows_wildcard() const;
    void display_columns() const;
};


bool snapdb::confirm_drop_check() const
{
    if( f_opt->is_defined("yes-i-know-what-im-doing") )
    {
        return true;
    }

    std::cout << "WARNING! This will drop vital tables from the database and is IRREVERSABLE!\n"
              << "Make sure you know what you are doing, and have appropriate backups before proceeding!\n"
              << "Are you sure you want to do this? (type in \"Yes I know what I'm doing\" and press ENTER): "
              ;
    std::string input;
    std::getline( std::cin, input );
    bool const confirm( (input == "Yes I know what I'm doing") );
    if( !confirm )
    {
        std::cerr << "Not dropping tables, so exiting." << std::endl;
    }
    return confirm;
}


snapdb::snapdb(int argc, char *argv[])
    : f_cassandra( QCassandra::create() )
    , f_host("localhost") // default
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
        if( confirm_drop_check() )
        {
            drop_tables(false);
            exit(0);
        }
        exit(1);
    }
    if( f_opt->is_defined( "drop-all-tables" ) )
    {
        if( confirm_drop_check() )
        {
            drop_tables(true);
            exit(0);
        }
        exit(1);
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
    f_cassandra->connect(f_host, f_port);
    if(f_cassandra->isConnected())
    {
        std::cout << "Working on Cassandra Cluster Named \""    << f_cassandra->clusterName()     << "\"." << std::endl;
        std::cout << "Working on Cassandra Protocol Version \"" << f_cassandra->protocolVersion() << "\"." << std::endl;
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
    f_cassandra->connect(f_host, f_port);
    QCassandraContext::pointer_t context(f_cassandra->context(f_context));

    // there are re-created when we connect and refilled when
    // we access a page; obviously this is VERY dangerous on
    // a live system!
    context->dropTable("content");
    context->dropTable("data");
    context->dropTable("emails");
    context->dropTable("files");
    context->dropTable("layout");
    context->dropTable("libQtCassandraLockTable");
    context->dropTable("links");
    context->dropTable("list");
    context->dropTable("listref");
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


void snapdb::display_tables() const
{
    QCassandraContext::pointer_t context(f_cassandra->context(f_context));

    // list of all the tables
    const QCassandraTables& tables(context->tables());
    for(QCassandraTables::const_iterator t(tables.begin());
        t != tables.end();
        ++t)
    {
        std::cout << (*t)->tableName() << std::endl;
    }
}


void snapdb::display_rows() const
{
    QCassandraContext::pointer_t context(f_cassandra->context(f_context));

    // list of rows in that table
    QCassandraTable::pointer_t table(context->findTable(f_table));
    if(!table)
    {
        std::cerr << "error: table \"" << f_table << "\" not found." << std::endl;
        exit(1);
    }

    snap::dbutils du( f_table, f_row );
    QCassandraRowPredicate row_predicate;
    row_predicate.setCount(f_count);
    table->readRows(row_predicate);
    const QCassandraRows& rows(table->rows());
    for( auto p_r : rows )
    {
        std::cout << du.get_row_name( p_r ) << std::endl;
    }
}


void snapdb::display_rows_wildcard() const
{
    QCassandraContext::pointer_t context(f_cassandra->context(f_context));

    // list of rows in that table
    QCassandraTable::pointer_t table(context->findTable(f_table));
    if(!table)
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
    std::stringstream ss;
    for(;;)
    {
        table->clearCache();
        table->readRows(row_predicate);
        const QCassandraRows& rows(table->rows());
        if(rows.isEmpty())
        {
            break;
        }
        for( auto p_r : rows )
        {
            const QString name(p_r->rowName());
            if(name.length() >= row_start.length()
                    && row_start == name.mid(0, row_start.length()))
            {
                ss << name << std::endl;
            }
        }
    }

    std::cout << ss.str();
}


void snapdb::display_columns() const
{
    QCassandraContext::pointer_t context(f_cassandra->context(f_context));

    // display all the columns of a row
    QCassandraTable::pointer_t table(context->findTable(f_table));
    if(!table)
    {
        std::cerr << "error: table \"" << f_table << "\" not found." << std::endl;
        exit(1);
    }
    snap::dbutils du( f_table, f_row );
    const QByteArray row_key( du.get_row_key() );
    if(!table->exists(row_key))
    {
        std::cerr << "error: row \"" << f_row << "\" not found in table \"" << f_table << "\"." << std::endl;
        exit(1);
    }

    QCassandraRow::pointer_t row(table->row(row_key));
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
        for( auto c : cells )
        {
            std::cout << du.get_column_name(c) << " = " << du.get_column_value( c, true /*display_only*/ ) << std::endl;
        }
    }
}


void snapdb::display()
{
    f_cassandra->connect(f_host, f_port);

    if(f_table.isEmpty())
    {
        display_tables();
    }
    else if(f_row.isEmpty())
    {
        display_rows();
    }
    else if(f_row.endsWith("%"))
    {
        display_rows_wildcard();
    }
    else
    {
        display_columns();
    }
}



int main(int argc, char *argv[])
{
    snapdb  s(argc, argv);

    s.display();

    return 0;
}

// vim: ts=4 sw=4 et
