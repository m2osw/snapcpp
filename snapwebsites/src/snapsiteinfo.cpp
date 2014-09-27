/*
 * Text:
 *      snapsiteinfo.cpp
 *
 * Description:
 *      Command line tool to manipulate the snap "sites" table. This can
 *      also be done from the cassview GUI and the snapmanager tool. This
 *      tool allows you to automate certain setups if you need to have
 *      such for your website.
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

// snapwebsites
//
#include "snapwebsites.h"
#include "qstring_stream.h"
#include "dbutils.h"

// our libs
//
#include <controlled_vars/controlled_vars_need_init.h>
#include <advgetopt/advgetopt.h>
#include <QtCassandra/QCassandra.h>

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
            NULL,
            NULL,
            "Usage: %p [-<opt>] [row [cell [value]]]",
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
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            NULL,
            "show this help output",
            advgetopt::getopt::no_argument
        },
        {
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "host",
            NULL,
            "host IP address or name (defaults to localhost)",
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
            "port",
            NULL,
            "port on the host to connect to (defaults to 9160)",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "table",
            NULL,
            "change the table name (default is \"sites\")",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "show the version of the snapcgi executable",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "[row [cell [value]]]",
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
    QString                         f_cell;
    QString                         f_value;
    getopt_ptr_t                    f_opt;

    void display_tables() const;
    void display_rows() const;
    void display_rows_wildcard() const;
    void display_columns() const;
    void display_cell() const;
    void set_cell() const;
};


snapdb::snapdb(int argc, char *argv[])
    : f_cassandra( QCassandra::create() )
    , f_host("localhost")           // default
    , f_port(9160)                  // default
    , f_count(100)                  // default
    , f_context("snap_websites")    // default
    , f_table("sites")              // forced to "sites" by default
    //, f_row("") -- auto-init
    , f_opt( new advgetopt::getopt( argc, argv, g_snapdb_options, g_configuration_files, NULL ) )
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
    if( f_opt->is_defined( "table" ) )
    {
        f_table = f_opt->get_string( "table" ).c_str();
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

    // finally check for parameters
    if( f_opt->is_defined( "--" ) )
    {
        const int arg_count = f_opt->size( "--" );
        if( arg_count >= 4 )
        {
            std::cerr << "error: one to three parameters ([row [cell [value]]]) can be specified on the command line." << std::endl;
            usage(advgetopt::getopt::error);
        }
        for( int idx = 0; idx < arg_count; ++idx )
        {
            if( idx == 0 )
            {
                f_row = f_opt->get_string( "--", idx ).c_str();
            }
            else if( idx == 1 )
            {
                f_cell = f_opt->get_string("--", idx).c_str();
            }
            else if( idx == 2 )
            {
                f_value = f_opt->get_string("--", idx).c_str();
            }
        }
    }

    if(!f_cell.isEmpty() && (f_row.isEmpty() || f_row.endsWith("%")))
    {
        // it is not likely that a row would need to end with '%'
        std::cerr << "error: when specifying a cell name, the row name cannot be empty nor end with '%'." << std::endl;
        usage(advgetopt::getopt::error);
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


void snapdb::display_cell() const
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
    if(!row->exists(f_cell))
    {
        std::cerr << "error: cell \"" << f_cell << "\" not found in table \"" << f_table << "\" and row \"" << f_row << "\"." << std::endl;
        exit(1);
    }

    QCassandraCell::pointer_t c(row->cell(f_cell));
    std::cout << du.get_column_value( c, true /*display_only*/ ) << std::endl;
}


void snapdb::set_cell() const
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

    QCassandraCell::pointer_t c(row->cell(f_cell));
    du.set_column_value( c, f_value );
}


void snapdb::display()
{
    f_cassandra->connect(f_host, f_port);

    if(f_row.isEmpty())
    {
        display_rows();
    }
    else if(f_row.endsWith("%"))
    {
        display_rows_wildcard();
    }
    else if(f_cell.isEmpty())
    {
        display_columns();
    }
    else if(f_value.isEmpty())
    {
        display_cell();
    }
    else
    {
        set_cell();
    }
}



int main(int argc, char *argv[])
{
    snapdb  s(argc, argv);

    s.display();

    return 0;
}

// vim: ts=4 sw=4 et
