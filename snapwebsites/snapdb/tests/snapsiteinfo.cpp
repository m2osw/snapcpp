/*
 * Text:
 *      snapsiteinfo.cpp
 *
 * Description:
 *      Command line tool to manipulate the snap "sites" table. This can
 *      also be done from the cassview GUI and the snapmanager tool. This
 *      tool allows you to automate certain setups (i.e. write shell scripts)
 *      if you need to have such for your websites.
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

// snapwebsites
//
#include "snapwebsites.h"
#include "qstring_stream.h"
#include "dbutils.h"

// advgetopt libs
//
#include <advgetopt/advgetopt.h>

// Qt lib
//
#include <QtCassandra/QCassandra.h>

// system
//
#include <algorithm>
#include <iostream>
#include <fstream>
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
            "Usage: %p [-<opt>] [row [cell [value]]]",
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
            "create-row",
            nullptr,
            "allows the creation of a row when writing a value",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "drop-cell",
            nullptr,
            "drop the specified cell (specify row and cell)",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "drop-row",
            nullptr,
            "drop the specified row (specify row)",
            advgetopt::getopt::no_argument
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
            "full-cell",
            nullptr,
            "show all the data from that cell, by default large binary cells get truncated for display",
            advgetopt::getopt::no_argument
        },
        {
            'h',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "host",
            nullptr,
            "host IP address or name (defaults to localhost)",
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
            "port",
            nullptr,
            "port on the host to connect to (defaults to 9042)",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "save-cell",
            nullptr,
            "save the specified cell to this file",
            advgetopt::getopt::required_argument
        },
        {
            '\0',
            0,
            "table",
            nullptr,
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
            nullptr,
            nullptr,
            "[row [cell [value]]]",
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

    void display_tables() const;
    void display_rows() const;
    void display_rows_wildcard() const;
    void display_columns() const;
    void display_cell() const;
    void set_cell() const;

    QCassandra::pointer_t           f_cassandra;
    QString                         f_host = "localhost";
    int32_t                         f_port = 4042;
    int32_t                         f_count = 100;
    QString                         f_context = "snap_websites";
    QString                         f_table = "sites";
    QString                         f_row;
    QString                         f_cell;
    QString                         f_value;
    getopt_ptr_t                    f_opt;
};


snapdb::snapdb(int argc, char * argv[])
    : f_cassandra( QCassandra::create() )
    //, f_host("localhost")           // default
    //, f_port(4042)                  // default
    //, f_count(100)                  // default
    //, f_context("snap_websites")    // default
    //, f_table("sites")              // forced to "sites" by default
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
        if( arg_count >= 1 )
        {
            f_row = f_opt->get_string( "--", 0 ).c_str();
        }
        if( arg_count >= 2 )
        {
            f_cell = f_opt->get_string("--", 1).c_str();
        }
        if( arg_count >= 3 )
        {
            f_value = f_opt->get_string("--", 2).c_str();
        }
    }

    if(!f_cell.isEmpty() && (f_row.isEmpty() || f_row.endsWith("%")))
    {
        // it is not likely that a row would need to end with '%'
        std::cerr << "error:snapdb(): when specifying a cell name, the row name cannot be empty nor end with '%'." << std::endl;
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
        std::cerr << "error:display_rows(): table \"" << f_table << "\" not found." << std::endl;
        exit(1);
    }

    snap::dbutils du( f_table, f_row );
    auto row_predicate = std::make_shared<QCassandraRowPredicate>();
    row_predicate->setCount(f_count);
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
        std::cerr << "error:display_rows_wildcard(): table \"" << f_table << "\" not found." << std::endl;
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
        std::cerr << "error:display_columns(): table \"" << f_table << "\" not found." << std::endl;
        exit(1);
    }
    snap::dbutils du( f_table, f_row );
    QByteArray const row_key( du.get_row_key() );
    if(!table->exists(row_key))
    {
        std::cerr << "error:display_columns(): row \"" << f_row << "\" not found in table \"" << f_table << "\"." << std::endl;
        exit(1);
    }

    if(f_opt->is_defined("drop-row"))
    {
        table->dropRow(row_key);
        return;
    }

    QCassandraRow::pointer_t row(table->row(row_key));
    row->clearCache();
    auto column_predicate = std::make_shared<QCassandraCellRangePredicate>();
    column_predicate->setCount(f_count);
    column_predicate->setIndex();
    for(;;)
    {
        row->readCells(column_predicate);
        QCassandraCells const & cells(row->cells());
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
        std::cerr << "error:display_cell(): table \"" << f_table << "\" not found." << std::endl;
        exit(1);
    }
    snap::dbutils du( f_table, f_row );
    const QByteArray row_key( du.get_row_key() );
    if(!table->exists(row_key))
    {
        std::cerr << "error:display_cell(): row \"" << f_row << "\" not found in table \"" << f_table << "\"." << std::endl;
        exit(1);
    }

    QCassandraRow::pointer_t row(table->row(row_key));
    if(!row->exists(f_cell))
    {
        std::cerr << "error:display_cell(): cell \"" << f_cell << "\" not found in table \"" << f_table << "\" and row \"" << f_row << "\"." << std::endl;
        exit(1);
    }

    // drop or display?
    if(f_opt->is_defined("drop-cell"))
    {
        row->dropCell(f_cell);
    }
    else if(f_opt->is_defined("save-cell"))
    {
        QCassandraCell::pointer_t c(row->cell(f_cell));
        std::fstream out;
        out.open(f_opt->get_string( "save-cell" ), std::fstream::out );
        if(out.is_open())
        {
            QtCassandra::QCassandraValue value(c->value());
            out.write(value.binaryValue().data(), value.size());
        }
        else
        {
            std::cerr << "error:display_cell(): could not open \"" << f_opt->get_string( "save-cell" )
                      << "\" to output content of cell \"" << f_cell
                      << "\" in table \"" << f_table
                      << "\" and row \"" << f_row
                      << "\"." << std::endl;
            exit(1);
        }
    }
    else
    {
        QCassandraCell::pointer_t c(row->cell(f_cell));
        std::cout << du.get_column_value( c, !f_opt->is_defined("full-cell") /*display_only*/ ) << std::endl;
    }
}


void snapdb::set_cell() const
{
    QCassandraContext::pointer_t context(f_cassandra->context(f_context));

    // display all the columns of a row
    QCassandraTable::pointer_t table(context->findTable(f_table));
    if(!table)
    {
        std::cerr << "error:set_cell(): table \"" << f_table << "\" not found." << std::endl;
        exit(1);
    }
    snap::dbutils du( f_table, f_row );
    QByteArray const row_key( du.get_row_key() );
    if(!f_opt->is_defined("create-row"))
    {
        if(!table->exists(row_key))
        {
            std::cerr << "error:set_cell(): row \"" << f_row << "\" not found in table \"" << f_table << "\"." << std::endl;
            exit(1);
        }
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



int main(int argc, char * argv[])
{
    try
    {
        snapdb  s(argc, argv);
        s.display();
        return 0;
    }
    catch(std::exception const & e)
    {
        std::cerr << "snapsiteinfo: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
