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
 *      Copyright (c) 2012-2013 Made to Order Software Corp.
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
//#include <QtCore>
#include <algorithm>
#include <controlled_vars/controlled_vars_need_init.h>

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

    void usage();
    void info();
    void drop_tables();
    void display();

private:
    QCassandra                      f_cassandra;
    QString                         f_host;
    controlled_vars::mint32_t       f_port;
    controlled_vars::mint32_t       f_count;
    QString                         f_context;
    QString                         f_table;
    QString                         f_row;
};

snapdb::snapdb(int argc, char *argv[])
    //: f_cassandra() -- auto-init
    : f_host("localhost") // default
    , f_port(9160) //default
    , f_count(100)
    , f_context("snap_websites")
    //, f_table("") -- auto-init
    //, f_row("") -- auto-init
{
    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "-h") == 0
        || strcmp(argv[i], "--help") == 0)
        {
            usage();
        }
        else if(strcmp(argv[i], "--count") == 0)
        {
            ++i;
            if(i >= argc)
            {
                fprintf(stderr, "error: --count expects one parameter.\n");
                exit(1);
            }
            char *end;
            f_count = strtol(argv[i], &end, 0);
            if(end == NULL || *end != '\0')
            {
                fprintf(stderr, "error: invalid number for --count (%s, %s)\n", argv[i], end);
                exit(1);
            }
        }
        else if(strcmp(argv[i], "--info") == 0)
        {
            info();
        }
        else if(strcmp(argv[i], "--host") == 0)
        {
            ++i;
            if(i >= argc)
            {
                fprintf(stderr, "error: --host expects one parameter.\n");
                exit(1);
            }
            f_host = argv[i];
        }
        else if(strcmp(argv[i], "--port") == 0)
        {
            ++i;
            if(i >= argc)
            {
                fprintf(stderr, "error: --port expects one parameter.\n");
                exit(1);
            }
            char *end;
            f_port = strtol(argv[i], &end, 0);
            if(end == NULL || *end != '\0')
            {
                fprintf(stderr, "error: invalid number for --port (%s, %s)\n", argv[i], end);
                exit(1);
            }
        }
        else if(strcmp(argv[i], "--context") == 0)
        {
            ++i;
            if(i >= argc)
            {
                fprintf(stderr, "error: --context expects one parameter.\n");
                exit(1);
            }
            f_context = argv[i];
        }
        else if(strcmp(argv[i], "--drop-tables") == 0)
        {
            drop_tables();
        }
        else if(argv[i][0] == '-')
        {
            fprintf(stderr, "error: unknown command line option \"%s\".\n", argv[i]);
            exit(1);
        }
        else
        {
            if(f_table.isEmpty())
            {
                f_table = argv[i];
            }
            else if(f_row.isEmpty())
            {
                f_row = argv[i];
            }
            else
            {
                fprintf(stderr, "error: only two parameters (table and row) can be specified on the command line.\n");
                exit(1);
            }
        }
    }
}

void snapdb::usage()
{
    printf("Usage: snapdb [--opts] [table [row]]\n");
    printf("By default snap db prints out the list of tables (column families) found in Cassandra.\n");
    printf("  -h | --help      print out this help screen.\n");
    printf("  --context <ctxt> name of the context to read from.\n");
    printf("  --count <count>  specify the number of rows to display.\n");
    printf("  --drop-tables    drop all the tables of the specified context.\n");
    printf("  --host <host>    host IP address or name defaults to localhost\n");
    printf("  --info           print out the cluster name and protocol version.\n");
    printf("  [table]          name of a table (column family) to print rows about.\n");
    printf("  [row]            name of a row, may be ending with %% to print all rows that start with that name.\n");
    printf("                   when row is not specified, then up to 100 of the rows of that table are printed.\n");
    exit(1);
}

void snapdb::info()
{
    f_cassandra.connect(f_host, f_port);
    if(f_cassandra.isConnected())
    {
        printf("Working on Cassandra Cluster Named \"%s\".\n", f_cassandra.clusterName().toUtf8().data());
        printf("Working on Cassandra Protocol Version \"%s\".\n", f_cassandra.protocolVersion().toUtf8().data());
    }
    else
    {
        fprintf(stderr, "The connection failed!\n");
    }
    exit(0);
}

void snapdb::drop_tables()
{
    f_cassandra.connect(f_host, f_port);
    QSharedPointer<QCassandraContext> context(f_cassandra.context(f_context));

    // there are re-created when we connect and refilled when
    // we access a page
    context->dropTable("content");
    context->dropTable("links");
    context->dropTable("sites");
}

void snapdb::display()
{
    f_cassandra.connect(f_host, f_port);
    QSharedPointer<QCassandraContext> context(f_cassandra.context(f_context));

    if(f_table.isEmpty())
    {
        // list of all the tables
        const QCassandraTables& tables(context->tables());
        for(QCassandraTables::const_iterator t(tables.begin());
                    t != tables.end();
                    ++t)
        {
            printf("%s\n", (*t)->tableName().toUtf8().data());
        }
    }
    else if(f_row.isEmpty())
    {
        // list of rows in that table
        QSharedPointer<QCassandraTable> table(context->findTable(f_table));
        if(table.isNull())
        {
            printf("error: table \"%s\" not found.\n", f_table.toUtf8().data());
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
            printf("%s\n", (*r)->rowName().toUtf8().data());
        }
    }
    else if(f_row.endsWith("%"))
    {
        // list of rows in that table
        QSharedPointer<QCassandraTable> table(context->findTable(f_table));
        if(table.isNull())
        {
            printf("error: table \"%s\" not found.\n", f_table.toUtf8().data());
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
                    printf("%s\n", name.toUtf8().data());
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
            printf("error: table \"%s\" not found.\n", f_table.toUtf8().data());
            exit(1);
        }
        if(!table->exists(f_row))
        {
            printf("error: row \"%s\" not found in table \"%s\".\n", f_row.toUtf8().data(), f_table.toUtf8().data());
            exit(1);
        }
        QSharedPointer<QCassandraRow> row(table->row(f_row));
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
                if(f_table == "users" && f_row == "*index_row*")
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
                )
                {
                    // 64 bit value
                    v = QString("%1").arg((*c)->value().uint64Value());
                }
                else if(n == "content::created"
                     || n == "content::modified"
                     || n == "content::updated"
                     || n.left(18) == "core::last_updated"
                     || n == "core::plugin_threshold"
                     || n == "sessions::time_limit"
                     || n == "users::created_time"
                     || n == "users::login_on"
                     || n == "users::logout_on"
                     || n == "users::previous_login_on"
                     || n == "users::start_date"
                     || n == "users::verified_on"
                )
                {
                    // 64 bit value
                    uint64_t time((*c)->value().uint64Value());
                    char buf[64];
                    struct tm t;
                    time_t seconds(time / 1000000);
                    gmtime_r(&seconds, &t);
                    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
                    v = QString("%1.%2").arg(buf).arg(time % 1000000, 6, 10, QChar('0'));
                }
                else if(n == "sitemapxml::count"
                     || n == "sessions::id"
                     || n == "sessions::time_to_live"
                     || (f_table == "libQtCassandraLockTable" && f_row == "hosts")
                )
                {
                    // 32 bit value
                    v = QString("%1").arg((*c)->value().uint32Value());
                }
                else if(n == "sessions::used_up"
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
                    int max(buf.size());
                    v += "(hex) ";
                    for(int i(0); i < max; ++i)
                    {
                        v += QString("%1 ").arg(static_cast<int>(static_cast<unsigned char>(buf.at(i))), 2, 16, QChar('0'));
                    }
                }
                else
                {
                    // all others viewed as strings
                    v = (*c)->value().stringValue().replace("\n", "\\n");
                }
                printf("%s = %s\n", n.toUtf8().data(), v.toUtf8().data());
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
