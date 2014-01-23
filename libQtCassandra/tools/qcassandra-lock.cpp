/*
 * Text:
 *      cassandra_lock.cpp
 *
 * Description:
 *      Test the QCassandraLock object to make sure that the lock works
 *      as expected when running this test on any number of computers.
 *
 * Documentation:
 *      Start the first instance with the -h option to define the
 *      Cassandra's host (defaults to 127.0.0.1 if undefined) and
 *      the -i to tell the first instance how many instances you
 *      want to run simultaneously. For example, if you have 4
 *      processors, you may want to use -i 4 or -i 8. It also
 *      accepts the number of times the processes will attempt the
 *      lock with -n. By default that count is 60 (1 minute). You
 *      can set it to 86400 for about 1 day test and a multiple
 *      thereof to run the test for multiple days.
 *
 * License:
 *      Copyright (c) 2013 Made to Order Software Corp.
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
#include <QtCassandra/QCassandraLock.h>
#include <QtCassandra/QCassandraValue.h>
#include <QDebug>
#include <QStringList>
#include <QFileInfo>
#include <unistd.h>


namespace
{
    enum cmd
    {
        CMD_UNDEFINED,
        CMD_ADD,
        CMD_HELP,
        CMD_LIST,
        CMD_REMOVE
    };

    QString         g_progname = NULL;
    cmd             g_cmd = CMD_UNDEFINED;
    const char *    g_host = "localhost";
    const char *    g_context_name = NULL;
    const char *    g_computer_name = NULL;

} // no name namespace


void usage()
{
    fprintf(stderr, "Usage: %s <cmd> [<opts>]\n", g_progname.toUtf8().data());
    fprintf(stderr, "  where <cmd> is one of:\n");
    fprintf(stderr, "    --add | -a <name>        add the <name> or comma separated <names> of a host to the specified context\n");
    fprintf(stderr, "    --help                   print out this help screen\n");
    fprintf(stderr, "    --list | -l              list all the host names\n");
    fprintf(stderr, "    --remove | -r <name>     remove the <name> or comma separated <names> of a host from the specified context\n");
    fprintf(stderr, "  where <opts> are:\n");
    fprintf(stderr, "    --context | -c <name>    use the <name>d context as required\n");
    fprintf(stderr, "    --host | -h              host IP address\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "IMPORTANT REMINDER: This tool cannot use the lock since it is used to initialize the\n");
    fprintf(stderr, "                    lock table. You must make sure you're only running one instance\n");
    fprintf(stderr, "                    at a time.\n");
    exit(0);
}



void add_host()
{
    // verify the parameters
    if(g_context_name == NULL) {
        fprintf(stderr, "error: the context name must be specified for the --add option.\n");
        exit(1);
    }

    // initialize the database
    QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );
    cassandra->connect(g_host);
    QtCassandra::QCassandraContext::pointer_t context(cassandra->context(g_context_name));
    if(!context) {
        fprintf(stderr, "error: could not retrieve the \"%s\" context from this Cassandra cluster.\n", g_context_name);
        exit(1);
    }

    QString names(g_computer_name);
    QStringList n(names.split(","));
    int l(n.length());
    for(int i(0); i < l; ++i) {
        context->addLockHost(n.at(i));
    }
}


void remove_host()
{
    // verify the parameters
    if(g_context_name == NULL) {
        fprintf(stderr, "error: the context name must be specified for the --remove option.\n");
        exit(1);
    }

    // initialize the database
    QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );
    cassandra->connect(g_host);
    QtCassandra::QCassandraContext::pointer_t context(cassandra->context(g_context_name));
    if(!context) {
        fprintf(stderr, "error: could not retrieve the \"%s\" context from this Cassandra cluster.\n", g_context_name);
        exit(1);
    }

    QString names(g_computer_name);
    QStringList n(names.split(","));
    int l(n.length());
    for(int i(0); i < l; ++i) {
        context->removeLockHost(n.at(i));
    }
}


void list_hosts()
{
    // verify the parameters
    if(g_context_name == NULL) {
        fprintf(stderr, "error: the context name must be specified for the --list option.\n");
        exit(1);
    }

    // initialize the database
    QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );
    cassandra->connect(g_host);
    QtCassandra::QCassandraContext::pointer_t context(cassandra->context(g_context_name));
    if(!context) {
        fprintf(stderr, "error: could not retrieve the \"%s\" context from this Cassandra cluster.\n", g_context_name);
        exit(1);
    }

    QtCassandra::QCassandraTable::pointer_t locks_table(context->table(context->lockTableName()));
    QtCassandra::QCassandraRow::pointer_t hosts(locks_table->row("hosts"));
    if(!hosts) {
        fprintf(stderr, "warning: there are no computer host names defined in this context.\n");
    }
    else {
        // show all the computer names in this context
        QtCassandra::QCassandraColumnRangePredicate column_predicate;
        column_predicate.setIndex();
        column_predicate.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
        hosts->readCells(column_predicate);
        QtCassandra::QCassandraCells cells(hosts->cells());
        if(cells.count() == 0) {
            fprintf(stderr, "warning: there are no computer host names defined in this context.\n");
        }
        else {
            printf("     ID  Host\n");
            do {
                for(QtCassandra::QCassandraCells::const_iterator c(cells.begin()); c != cells.end(); ++c) {
                    printf("%7d  %s\n",
                        (*c)->value().uint32Value(),
                        (*c)->columnName().toUtf8().data());
                }
                hosts->clearCache();
                hosts->readCells(column_predicate);
                cells = hosts->cells();
            } while(cells.count() != 0);
        }
    }
}


void run_command(cmd c)
{
    switch(c) {
    case CMD_ADD:
        add_host();
        break;

    case CMD_HELP:
        usage();
        break;

    case CMD_LIST:
        list_hosts();
        break;

    case CMD_REMOVE:
        remove_host();
        break;

    default:
        fprintf(stderr, "error: a command must be specified, try --help for more information.\n");
        exit(1);

    }
}


void set_command(cmd c)
{
    if(g_cmd != CMD_UNDEFINED) {
        fprintf(stderr, "error: you cannot use more than one command at a time.\n");
        exit(1);
    }
    g_cmd = c;
}


void check_max(const char *opt, const int i, const int argc)
{
    if(i >= argc) {
        fprintf(stderr, "error: option \"%s\" requires an option.\n", opt);
        exit(1);
    }
}


void parse_arguments(int argc, char *argv[])
{
    QFileInfo info(argv[0]);
    g_progname = info.baseName();

    for(int i(1); i < argc; ++i) {
        if(argv[i][0] == '-') {
            if(argv[i][1] == '-') {
                if(strcmp(argv[i], "--add") == 0) {
                    set_command(CMD_ADD);
                    ++i;
                    check_max("--add", i, argc);
                    g_computer_name = argv[i];
                }
                else if(strcmp(argv[i], "--context") == 0) {
                    ++i;
                    check_max("--context", i, argc);
                    g_context_name = argv[i];
                }
                else if(strcmp(argv[i], "--help") == 0) {
                    set_command(CMD_HELP);
                }
                else if(strcmp(argv[i], "--host") == 0) {
                    ++i;
                    check_max("--host", i, argc);
                    g_host = argv[i];
                }
                else if(strcmp(argv[i], "--list") == 0) {
                    set_command(CMD_LIST);
                }
                else if(strcmp(argv[i], "--remove") == 0) {
                    set_command(CMD_REMOVE);
                    ++i;
                    check_max("--remove", i, argc);
                    g_computer_name = argv[i];
                }
                else {
                    fprintf(stderr, "error: unknown option \"%s\".\n", argv[i]);
                    exit(1);
                }
            }
            else {
                int l(static_cast<int>(strlen(argv[i])));
                for(int j(1); j < l; ++j) {
                    switch(argv[i][j]) {
                    case 'a':
                        set_command(CMD_ADD);
                        ++i;
                        check_max("-a", i, argc);
                        g_computer_name = argv[i];
                        break;

                    case 'c':
                        ++i;
                        check_max("-c", i, argc);
                        g_context_name = argv[i];
                        break;

                    case 'h':
                        ++i;
                        check_max("-h", i, argc);
                        g_host = argv[i];
                        break;

                    case 'l':
                        set_command(CMD_LIST);
                        break;

                    case 'r':
                        set_command(CMD_REMOVE);
                        ++i;
                        check_max("-r", i, argc);
                        g_computer_name = argv[i];
                        break;

                    default:
                        fprintf(stderr, "error: unknown option \"-%c\".\n", argv[i][j]);
                        exit(1);
                        break;

                    }
                }
            }
        }
        else {
            fprintf(stderr, "error: unsupported parameter \"%s\".\n", argv[i]);
            exit(1);
        }
    }
}


int main(int argc, char *argv[])
{
    parse_arguments(argc, argv);
    run_command(g_cmd);
    return 0;
}

// vim: ts=4 sw=4 et
