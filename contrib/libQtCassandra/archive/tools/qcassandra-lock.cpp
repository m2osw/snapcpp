/*
 * Text:
 *      tools/qcassandra_lock.cpp
 *
 * Description:
 *      Tool used to setup the lock functionality coming with the
 *      libQtCassandra library.
 *
 * Documentation:
 *      This tool offers you a way to add and remove hosts from the list
 *      of hosts defined in the lock_Table table. Each
 *      named host has the ability to lock something in the
 *      Cassandra cluster with the help of the QCassandraLock object.
 *      You must add the name in this way (or your own application software)
 *      before you can lock from that specific host.
 *
 *      The tool has three main functions:
 *
 *      --add <host>     Add a new host to the cluster.
 *      --remove <host>  Remove an existing host from the cluster.
 *      --list           List hosts with their identifier.
 *
 *      Use the --help comment for additional details.
 *
 * License:
 *      Copyright (c) 2013-2016 Made to Order Software Corp.
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
#include <iostream>
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
    std::cerr << "Usage: " << g_progname.toUtf8().data() << " <cmd> [<opts>]" << std::endl;
    std::cerr << "  where <cmd> is one of:" << std::endl;
    std::cerr << "    --add | -a <name>        add the <name> or comma separated <names> of hosts to the specified context" << std::endl;
    std::cerr << "    --help                   print out this help screen" << std::endl;
    std::cerr << "    --list | -l              list all the host names" << std::endl;
    std::cerr << "    --remove | -r <name>     remove the <name> or comma separated <names> of hosts from the specified context" << std::endl;
    std::cerr << "    --version                display the software version" << std::endl;
    std::cerr << "  where <opts> are:" << std::endl;
    std::cerr << "    --context | -c <name>    use the <name>d context as required" << std::endl;
    std::cerr << "    --host | -h              host IP address" << std::endl;
    std::cerr << "" << std::endl;
    std::cerr << "IMPORTANT REMINDER: This tool cannot use the lock since it is used to initialize the" << std::endl;
    std::cerr << "                    lock table. You must make sure you are only running one instance" << std::endl;
    std::cerr << "                    at a time." << std::endl;
    exit(0);
}



void add_host()
{
    // verify the parameters
    if(g_context_name == NULL) {
        std::cerr << "error: the context name must be specified for the --add option." << std::endl;
        exit(1);
    }

    // initialize the database
    QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );
    cassandra->connect(g_host);
    QtCassandra::QCassandraContext::pointer_t context(cassandra->context(g_context_name));
    if(!context) {
        std::cerr << "error: could not retrieve the \"" << g_context_name << "\" context from this Cassandra cluster." << std::endl;
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
        std::cerr << "error: the context name must be specified for the --remove option." << std::endl;
        exit(1);
    }

    // initialize the database
    QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );
    cassandra->connect(g_host);
    QtCassandra::QCassandraContext::pointer_t context(cassandra->context(g_context_name));
    if(!context) {
        std::cerr << "error: could not retrieve the \"" << g_context_name << "\" context from this Cassandra cluster." << std::endl;
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
        std::cerr << "error: the context name must be specified for the --list option." << std::endl;
        exit(1);
    }

    // initialize the database
    QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );
    cassandra->connect(g_host);
    QtCassandra::QCassandraContext::pointer_t context(cassandra->context(g_context_name));
    if(!context) {
        std::cerr << "error: could not retrieve the \"" << g_context_name << "\" context from this Cassandra cluster." << std::endl;
        exit(1);
    }

    QtCassandra::QCassandraTable::pointer_t locks_table(context->table(context->lockTableName()));
    QtCassandra::QCassandraRow::pointer_t hosts(locks_table->row("hosts"));
    if(!hosts) {
        std::cerr << "warning: there are no computer host names defined in this context." << std::endl;
    }
    else {
        // show all the computer names in this context
        auto column_predicate( std::make_shared<QtCassandra::QCassandraCellRangePredicate>() );
        column_predicate->setIndex();
        hosts->readCells(column_predicate);
        QtCassandra::QCassandraCells cells(hosts->cells());
        if(cells.count() == 0) {
            std::cerr << "warning: there are no computer host names defined in this context." << std::endl;
        }
        else {
            std::cout << "     ID  Host" << std::endl;
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
        std::cerr << "error: a command must be specified, try --help for more information." << std::endl;
        exit(1);

    }
}


void set_command(cmd c)
{
    if(g_cmd != CMD_UNDEFINED) {
        std::cerr << "error: you cannot use more than one command at a time." << std::endl;
        exit(1);
    }
    g_cmd = c;
}


void check_max(const char *opt, const int i, const int argc)
{
    if(i >= argc) {
        std::cerr << "error: option \"" << opt << "\" requires an option.\n" << std::endl;
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
                    std::cerr << "error: unknown option \"" <<  argv[i] << "\"." << std::endl;
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
                        std::cerr << "error: unknown option \"-" << argv[i][j] << "\"." << std::endl;
                        exit(1);
                        break;

                    }
                }
            }
        }
        else {
            std::cerr << "error: unsupported parameter \"" << argv[i] << "\"." << std::endl;
            exit(1);
        }
    }
}


int main(int argc, char *argv[])
{
    try
    {
        parse_arguments(argc, argv);
        run_command(g_cmd);
    }
    catch(std::exception const& e)
    {
        // manage an exception just like a "standard error"
        std::cerr << "error:exception: \"" << e.what() << "\"" << std::endl;
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 et
