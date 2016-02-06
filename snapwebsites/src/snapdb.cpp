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
#include "snapwebsites.h"
#include "qstring_stream.h"
#include "dbutils.h"

// 3rd party libs
//
#include <QtCore>
#include <QtSql>
#include <QtCassandra/QCassandra.h>
#include <controlled_vars/controlled_vars_need_init.h>
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
            "drop-context",
            NULL,
            "drop the snapwebsites context (and ALL of the tables)",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "dump-context",
            NULL,
            "dump the snapwebsites context to text output",
            advgetopt::getopt::required_argument
        },
        {
            '\0',
            0,
            "restore-context",
            NULL,
            "restore the snapwebsites context from text output (required confirmation)",
            advgetopt::getopt::optional_argument
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
            "version",
            nullptr,
            "show the version of the snapdb executable",
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
 * manner rather than having globals, but that is clearly very
 * similar here!
 */
class snapdb
{
public:
    snapdb(int argc, char * argv[]);

    void usage(advgetopt::getopt::status_t status);
    void info();
    void drop_tables();
    void drop_context();
    void dump_context();
    bool restore_context();
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

    std::cout << "WARNING! This command is about to drop vital tables from the Snap!" << std::endl
              << "         database and is IRREVERSABLE!" << std::endl
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


snapdb::snapdb(int argc, char * argv[])
    : f_cassandra( QCassandra::create() )
    , f_host("localhost") // default
    , f_port(9160) //default
    , f_count(100)
    , f_context("snap_websites")
    //, f_table("") -- auto-init
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
        if( f_opt->is_defined( "drop-tables" ) )
        {
            if( confirm_drop_check() )
            {
                drop_tables();
                exit(0);
            }
            exit(1);
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
        if( f_opt->is_defined( "dump-context" ) )
        {
            try
            {
                dump_context();
                exit(0);
            }
            catch( const std::exception& except )
            {
                std::cerr << "Exception caught! what=[" << except.what() << "]" << std::endl;
                exit(1);
            }
        }
        if( f_opt->is_defined( "restore-context" ) )
        {
            exit( restore_context()? 0: 1 );
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
        std::cout << "Using Cassandra Snitch \"" << f_cassandra->snitch() << "\"." << std::endl;

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


void snapdb::drop_tables()
{
    f_cassandra->connect(f_host, f_port);

    QCassandraContext::pointer_t context(f_cassandra->context(f_context));
    //
    // there are re-created when we connect and refilled when
    // we access a page; obviously this is VERY dangerous on
    // a live system!
    //
    context->dropTable("antihammering");
    context->dropTable("backend");
    context->dropTable("branch");
    context->dropTable("cache");
    context->dropTable("content");
    context->dropTable("emails");
    context->dropTable("epayment_paypal");
    context->dropTable("files");
    context->dropTable("firewall");
    context->dropTable("layout");
    context->dropTable("libQtCassandraLockTable");
    context->dropTable("links");
    context->dropTable("list");
    context->dropTable("listref");
    context->dropTable("processing");
    context->dropTable("revision");
    context->dropTable("secret");
    context->dropTable("sessions");
    context->dropTable("shorturl");
    context->dropTable("sites");
    context->dropTable("test_results");
    context->dropTable("tracker");
    context->dropTable("users");

    // wait until all the tables are 100% dropped
    //
    f_cassandra->synchronizeSchemaVersions();
}


void snapdb::drop_context()
{
    f_cassandra->connect(f_host, f_port);
    f_cassandra->dropContext( f_context );
    f_cassandra->synchronizeSchemaVersions();
}


namespace
{
    void do_query( const QString& q_str )
    {
        QSqlQuery query;
        if( !query.exec( q_str ) )
        {
            throw std::runtime_error( query.lastError().text().toUtf8().data() );
        }
    }
}

class sqlBackupRestore
{
public:
    sqlBackupRestore( const QCassandraContext::pointer_t context, const QString& sqlDbFile );

    void storeContext();
    
private:
    void getTableIds();
    void getRowIds();

    void createSchema( const QString& sqlDbFile );
    void storeTables();
    void storeRowsByTable( QCassandraTable::pointer_t table );
    void storeCellsByRow( QCassandraRow::pointer_t row );

    typedef QMap<QString,QVariant>            name_to_id_t;
    typedef QList<QCassandraTable::pointer_t> table_list_t;
    typedef QList<QCassandraRow::pointer_t>   row_list_t;

    QCassandraContext::pointer_t f_context;
    name_to_id_t                 f_tableNameToId;
    name_to_id_t                 f_rowNameToId;
    table_list_t                 f_tableList;
    row_list_t                   f_rowList;
    //
    QCassandraColumnRangePredicate::pointer_t  f_colPred;
    QCassandraRowPredicate                     f_rowPred;
};


sqlBackupRestore::sqlBackupRestore( const QCassandraContext::pointer_t context, const QString& sqlDbFile )
    : f_context( context )
{
    createSchema( sqlDbFile );

    f_colPred.reset( new QCassandraColumnRangePredicate );
    f_colPred->setCount(1000);
    //
    f_rowPred.setStartRowName("");
    f_rowPred.setEndRowName("");
    f_rowPred.setCount(100);
    f_rowPred.setColumnPredicate( f_colPred );
}


void sqlBackupRestore::createSchema( const QString& sqlDbFile )
{
	QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );
	db.setDatabaseName( sqlDbFile );
	if( !db.open() )
    {
        const QString error( QString("Cannot open SQLite database [%1]!").arg(sqlDbFile) );
        throw std::runtime_error( error.toUtf8().data() );
    }

    do_query( "CREATE TABLE IF NOT EXISTS snap_info   (id INTEGER PRIMARY KEY, last_update TEXT);" );
    do_query( "CREATE TABLE IF NOT EXISTS snap_tables (id INTEGER PRIMARY KEY, table_name TEXT UNIQUE, identifier INTEGER, column_type TEXT, comment TEXT);" ); // TODO: add other attributes
    do_query( "CREATE TABLE IF NOT EXISTS snap_rows   (id INTEGER PRIMARY KEY, row_name   TEXT UNIQUE, row_key LONGBLOB  , table_id    INTEGER);" );
    do_query( "CREATE TABLE IF NOT EXISTS snap_cells  (id INTEGER PRIMARY KEY, cell_name  TEXT UNIQUE, col_key LONGBLOB  , row_id      INTEGER, "
        "ttl INTEGER, consistency_level INTEGER, timestamp INTEGER, cell_value LONGBLOB);" ); // TODO: Do we need to have timestamp_mode? There are two timestamps; one for the cell, and one for the value--are they both needed?
}


void sqlBackupRestore::getTableIds()
{
    QSqlQuery q;
    q.prepare( "SELECT id,table_name FROM snap_tables;" );
    if( !q.exec() )
    {
        throw std::runtime_error( q.lastError().text().toUtf8().data() );
    }
    //
    for( q.first(); q.isValid(); q.next() )
    {
        f_tableNameToId[q.value(1).toString()] = q.value(0);
    }
}


void sqlBackupRestore::getRowIds()
{
    QSqlQuery q;
    q.prepare( "SELECT id,row_name FROM snap_rows;" );
    if( !q.exec() )
    {
        throw std::runtime_error( q.lastError().text().toUtf8().data() );
    }
    //
    for( q.first(); q.isValid(); q.next() )
    {
        f_rowNameToId[q.value(1).toString()] = q.value(0);
    }
}


void sqlBackupRestore::storeContext()
{
    QSqlDatabase db( QSqlDatabase::database() );

    // TABLES
    //
    db.transaction();
    storeTables();
    db.commit();
    getTableIds();

    // ROWS
    //
    db.transaction();
    //
    for( auto table : f_tableList )
    {
        storeRowsByTable( table );
    }
    //
    db.commit();
    getRowIds();

    // CELLS
    //
    db.transaction();
    for( auto row : f_rowList )
    {
        storeCellsByRow( row );
    }
    db.commit();
}


void sqlBackupRestore::storeTables()
{
    QVariantList    table_names;
    QVariantList    identifiers;
    QVariantList    column_types;
    QVariantList    comments;

    for( auto table : f_context->tables() )
    {
        std::string table_name( table->tableName().toUtf8().data() );
        std::cout << "Processing table [" << table_name << "]" << std::endl;
        if( table_name == "cache" )
        {
            std::cout << "Skipping the cache table..." << std::endl;
            continue;
        }

        table_names  << table->tableName();
        identifiers  << table->identifier();
        column_types << table->columnType();
        comments     << table->comment();

        f_tableList << table;
    }


    std::cout << std::endl << "Saving tables to SQL database..." << std::endl;

    QSqlQuery q;
    q.prepare( "INSERT OR REPLACE INTO snap_tables (table_name,identifier,column_type,comment) VALUES (?,?,?,?);" );
    q.addBindValue( table_names  );
    q.addBindValue( identifiers  );
    q.addBindValue( column_types );
    q.addBindValue( comments     );
    if( !q.execBatch() )
    {
        throw std::runtime_error( q.lastError().text().toUtf8().data() );
    }
}


void sqlBackupRestore::storeRowsByTable( QCassandraTable::pointer_t table )
{
    QVariantList    row_names;
    QVariantList    table_ids;
    QVariantList    row_keys;

    uint32_t rowsRemaining = table->readRows( f_rowPred );
    while( true )
    {
        for( auto row : table->rows() )
        {
            std::string row_name( row->rowName().toUtf8().data() );
            std::cout << "Processing table [" << table->tableName() << "], row [" << row_name << "]" << std::endl;
            if( table->tableName() == "libQtCassandraLockTable" && row_name == "hosts" )
            {
                std::cout << "Skipping the 'hosts' row of the 'libQtCassandraLockTable' table." << std::endl;
                continue;
            }

            row_names << row->rowName();
            row_keys  << row->rowKey();
            table_ids << f_tableNameToId[table->tableName()];

            f_rowList << row;
        }
        //
        if( rowsRemaining == 0 )
        {
            break;
        }
        //
        rowsRemaining = table->readRows( f_rowPred ); // Next 100 records
    }

    std::cout << std::endl << "Saving rows to SQL database..." << std::endl;
    QSqlQuery q;
    q.prepare( "INSERT OR REPLACE INTO snap_rows (row_name,row_key,table_id) VALUES (?,?,?);" );
    q.addBindValue( row_names );
    q.addBindValue( row_keys  );
    q.addBindValue( table_ids );
    if( !q.execBatch() )
    {
        throw std::runtime_error( q.lastError().text().toUtf8().data() );
    }
}


void sqlBackupRestore::storeCellsByRow( QCassandraRow::pointer_t row )
{
    // This seems to be a bug. The colsRemaining return value never changes with each read.
    //
    /*uint32_t colsRemaining =*/ row->readCells( *f_colPred );
#if 0
    while( true )
    {
#endif
        QVariantList row_ids, cell_names, col_keys, ttls, consistency_levels, timestamps, cell_values;
        for( auto col : row->cells() )
        {
            std::string cell_name( col->columnName().toUtf8().data() );
            std::cout << "Processing cell [" << cell_name << "]" << std::endl;

            auto& val( col->value() );
            row_ids            << f_rowNameToId[row->rowName()]             ;
            cell_names         << col->columnName()                         ;
            col_keys           << col->columnKey()                          ;
            ttls               << val.ttl()                                 ;
            consistency_levels << static_cast<int>(col->consistencyLevel()) ;
            timestamps         << static_cast<qlonglong>(col->timestamp())  ;
            cell_values        << val.binaryValue()                         ;
        }

        std::cout << std::endl << "Saving cells to SQL database..." << std::endl;
        QSqlQuery q;
        q.prepare(
                "INSERT OR REPLACE INTO snap_cells "
                "(cell_name,row_id,col_key,ttl,consistency_level,timestamp,cell_value) "
                "VALUES (?,?,?,?,?,?,?);"
                );
        q.addBindValue( cell_names         );
        q.addBindValue( row_ids            );
        q.addBindValue( col_keys           );
        q.addBindValue( ttls               );
        q.addBindValue( consistency_levels );
        q.addBindValue( timestamps         );
        q.addBindValue( cell_values        );
        if( !q.execBatch() )
        {
            std::cerr << "lastQuery=[" << q.lastQuery().toUtf8().data() << "]" << std::endl;
            throw std::runtime_error( q.lastError().text().toUtf8().data() );
        }

#if 0
        if( colsRemaining == 0 )
        {
            break;
        }
        //
        colsRemaining = row->readCells( *f_colPred ); // Next 100 columns
    }
#endif
}


void snapdb::dump_context()
{
    f_cassandra->connect(f_host, f_port);
    QCassandraContext::pointer_t context(f_cassandra->context(f_context));
    const QString outfile( f_opt->get_string( "dump-context" ).c_str() );

    sqlBackupRestore backup( context, outfile );
    backup.storeContext();
}


bool snapdb::restore_context()
{
    std::cerr << "--restore-context has not been implemented yet for SQLite. Exiting..." << std::endl;
    return false;
#if 0
    f_cassandra->connect(f_host, f_port);

    if( f_cassandra->findContext( f_context ) )
    {
        std::cerr << "The " << f_context.toUtf8().data() << " context already exists! This feature will not overwrite existing data. Please drop the context first." << std::endl;
        return false;
    }

    QCassandraContext::pointer_t context( f_cassandra->context( f_context ) );

    const QString infile( f_opt->get_string( "restore-context" ).c_str() );
    std::shared_ptr<QFile> qif;
    if( infile.isEmpty() )
    {
        qif.reset( new QFile );
        qif->open( stdin, QIODevice::ReadOnly );
    }
    else
    {
        QFileInfo fi( infile );
        if( !fi.exists() )
        {
            std::cerr << "Input file does not exist!" << std::endl;
            return false;
        }

        qif.reset( new QFile( infile ) );
        qif->open( QIODevice::ReadOnly );
    }

    QCassandraTable::pointer_t table;
    QCassandraRow::pointer_t   row;
    QCassandraCell::pointer_t  cell;

    QXmlStreamReader stream( qif.get() );
    while( !stream.atEnd() )
    {
        QXmlStreamReader::TokenType type = stream.readNext();
        if( type == QXmlStreamReader::StartElement )
        {
            QXmlStreamAttributes attribs( stream.attributes() );
            if( stream.name() == "table" )
            {
                Q_ASSERT(context);
                table = context->table( attribs.value("name").toString() );
            }
            else if( stream.name() == "row" )
            {
                Q_ASSERT(table);
                row = table->row( attribs.value("name").toString() );
            }
            else if( stream.name() == "col" )
            {
                Q_ASSERT(row);
                cell = row->cell( attribs.value("name").toString() );
            }
        }
        else if( type == QXmlStreamReader::Characters )
        {
            if( !stream.isWhitespace() )
            {
                Q_ASSERT(cell);
                cell->setValue( stream.text().toString() );
            }
        }
    }

    if( stream.hasError() )
    {
        std::cerr << "Error in XML: [" << stream.errorString().toUtf8().data() << "]!" << std::endl;
        std::cerr << "Failed at line "
                  << stream.lineNumber() << ", token=["
                  << stream.tokenString().toUtf8().data() << "], name=["
                  << stream.name().toUtf8().data() << "]" << std::endl;
        return false;
    }

    return true;
#endif
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
        QCassandraColumnRangePredicate column_predicate;
        column_predicate.setCount(f_count);
        column_predicate.setIndex();
        for(;;)
        {
            row->clearCache();
            row->readCells(column_predicate);
            QCassandraCells const& cells(row->cells());
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



int main(int argc, char *argv[])
{
    try
    {
        snapdb  s(argc, argv);
        s.display();
        return 0;
    }
    catch(std::exception const& e)
    {
        std::cerr << "snapdb: exception: " << e.what() << std::endl;
        return 1;
    }
}

// vim: ts=4 sw=4 et
