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
 *      Copyright (c) 2012-2016 Made to Order Software Corp.
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

// our lib
//
#include "snapdb.h"
#include "snapwebsites.h"
#include "qstring_stream.h"
#include "dbutils.h"

// 3rd party libs
//
#include <QtCore>
#include <QtSql>
#include <QtCassandra/QCassandra.h>
#include <advgetopt/advgetopt.h>

// system
//
#include <algorithm>
#include <iostream>
#include <sstream>

namespace
{
    const std::vector<std::string> g_configuration_files; // Empty

    const advgetopt::getopt::option g_snapdb_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "Usage: %p [-<opt>] [table [row]]",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "where -<opt> is one or more of:",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "show this help output",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "context",
            nullptr,
            "name of the context from which to read",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "count",
            nullptr,
            "specify the number of rows to display",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "yes-i-know-what-im-doing",
            nullptr,
            "Force the dropping of tables, without warning and stdin prompt. Only use this if you know what you're doing!",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "host",
            nullptr,
            "host IP address or name (defaults to localhost)",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "port",
            nullptr,
            "port on the host to connect to (defaults to 9042)",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "info",
            nullptr,
            "print out the cluster name and protocol version",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "show the version of the %p executable",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "[table [row]]",
            advgetopt::getopt::default_multiple_argument
        },
        {
            '\0',
            0,
            nullptr,
            nullptr,
            nullptr,
            advgetopt::getopt::end_of_options
        }
    };
}
//namespace

using namespace QtCassandra;

snapdb::snapdb(int argc, char * argv[])
    : f_cassandra( QCassandra::create() )
    //, f_host("localhost") -- auto-init
    //, f_port(4042) -- auto-init -- default to connect to snapdbproxy
    //, f_count(100) -- auto-init
    //, f_context("snap_websites") -- auto-init
    //, f_table("") -- auto-init
    //, f_row("") -- auto-init
    , f_opt( new advgetopt::getopt( argc, argv, g_snapdb_options, g_configuration_files, nullptr ) )
{
    if(f_opt->is_defined("version"))
    {
        std::cerr << SNAPWEBSITES_VERSION_STRING << std::endl;
        exit(1);
    }

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

    try
    {
        if( f_opt->is_defined( "info" ) )
        {
            info();
            exit(0);
        }
        if( f_opt->is_defined( "drop-context" ) )
        {
            if( confirm_drop_check() )
            {
                drop_context();
                exit(0);
            }
            exit(1);
        }
    }
    catch( const std::exception& except )
    {
        std::cerr << "Error connecting to the cassandra server! Reason=[" << except.what() << "]" << std::endl;
        exit( 1 );
    }
    catch( ... )
    {
        std::cerr << "Unknown error connecting to the cassandra server!" << std::endl;
        exit( 1 );
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
        std::cout << "Using Cassandra Partitioner \"" << f_cassandra->partitioner() << "\"." << std::endl;

        // At this time the following does not work, we will need CQL support first
        //const QCassandraClusterInformation& cluster_info(f_cassandra->clusterInformation());
        //int max(cluster_info.size());
        //std::cout << "With " << max << " nodes running." << std::endl;
        //for(int idx(0); idx < max; ++idx)
        //{
        //    const QCassandraNode& node(cluster_info.node(idx));
        //    std::cout << "H:" << node.nodeHost() << " R:" << node.nodeRack() << " DC:" << node.nodeDataCenter() << std::endl;
        //}
        exit(0);
    }
    else
    {
        std::cerr << "The connection failed!" << std::endl;
        exit(1);
    }
}


bool snapdb::confirm_drop_check() const
{
    if( f_opt->is_defined("yes-i-know-what-im-doing") )
    {
        return true;
    }

    std::cout << "WARNING! This command is about to drop vital tables from the Snap!" << std::endl
              << "         database and is IRREVERSIBLE!" << std::endl
              << std::endl
              << "Make sure you know what you are doing and have appropriate backups" << std::endl
              << "before proceeding!" << std::endl
              << std::endl
              << "Are you really sure you want to do this?" << std::endl
              << "(type in \"Yes I know what I'm doing\" and press ENTER): "
              ;
    std::string input;
    std::getline( std::cin, input );
    bool const confirm( (input == "Yes I know what I'm doing") );
    if( !confirm )
    {
        std::cerr << "warning: Not dropping tables, so exiting." << std::endl;
    }
    return confirm;
}


void snapdb::drop_context()
{
    f_cassandra->connect(f_host, f_port);
    f_cassandra->dropContext( f_context );
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
    auto row_predicate = std::make_shared<QCassandraRowPredicate>();
    row_predicate->setCount(f_count);
    table->readRows(row_predicate);
    const QCassandraRows& rows(table->rows());
    for( auto const & p_r : rows )
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
    table->clearCache();
    auto row_predicate = std::make_shared<QCassandraRowPredicate>();
    QString row_start(f_row.left(f_row.length() - 1));
    // remember that the start/end on row doesn't work in "alphabetical"
    // order so we cannot use it here...
    //row_predicate->setStartRowName(row_start);
    //row_predicate->setEndRowName(row_start + QCassandraColumnPredicate::first_char);
    row_predicate->setCount(f_count);
    std::stringstream ss;
    for(;;)
    {
        table->readRows(row_predicate);
        const QCassandraRows& rows(table->rows());
        if(rows.isEmpty())
        {
            break;
        }
        for( auto const & p_r : rows )
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
    try
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
        QByteArray const row_key( du.get_row_key() );
        if(!table->exists(row_key))
        {
            std::cerr << "error: row \"" << f_row << "\" not found in table \"" << f_table << "\"." << std::endl;
            exit(1);
        }

        QCassandraRow::pointer_t row(table->row(row_key));
        row->clearCache();
        auto column_predicate(std::make_shared<QCassandraCellRangePredicate>());
        column_predicate->setCount(f_count);
        column_predicate->setIndex();
        for(;;)
        {
            row->readCells(column_predicate);
            QCassandraCells const& cells(row->cells());
            if(cells.isEmpty())
            {
                break;
            }
            for( auto const & c : cells )
            {
                std::cout
                    << du.get_column_name(c)
                    << " = "
                    << du.get_column_value( c, true /*display_only*/ )
                    << std::endl;
            }
        }
    }
    catch(snap::snap_exception const& e)
    {
        // in most cases we get here because of something invalid in
        // the database
        std::cerr << "error: could not properly read row \"" << f_row << "\" in table \"" << f_table << "\". It may not exist or its key is not defined as expected (i.e. not a valid md5sum)" << std::endl;
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

// vim: ts=4 sw=4 et
