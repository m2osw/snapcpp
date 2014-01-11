/*
 * Text:
 *      snaplayout.cpp
 *
 * Description:
 *      Save layout files in the Snap database.
 *
 * License:
 *      Copyright (c) 2012 Made to Order Software Corp.
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

#include <advgetopt/advgetopt.h>
#include <QtCassandra/QCassandra.h>
#include <algorithm>
#include <controlled_vars/controlled_vars_need_init.h>
#include <QDomDocument>
#include <QDomElement>
#include <QDateTime>
#include <QFile>
#include <QTextStream>

// snap library
//
#include "qstring_stream.h"

namespace
{
    const std::vector<std::string> g_configuration_files; // Empty

    const advgetopt::getopt::option g_snaplayout_options[] =
    {
        {
            '?',
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
            "localhost",
            "host IP address or name [default=localhost]",
            advgetopt::getopt::optional_argument
        },
        {
            'p',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "port",
            "9160",
            "port on the host to which to connect [default=9160]",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "layout-file1.xsl layout-file2.xsl ... layout-fileN.xsl",
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
class snap_layout
{
public:
    snap_layout(int argc, char *argv[]);

    void usage();
    void add_files();
    void load_xsl_info(QDomDocument& doc, const QString& filename, QString& layout_name, QString& layout_area, time_t& layout_modified);

private:
    typedef std::shared_ptr<advgetopt::getopt>    getopt_ptr_t;

    QCassandra                      f_cassandra;
    typedef QVector<QString>        string_array_t;
    string_array_t                  f_layouts;
    QString                         f_host;
    controlled_vars::zint32_t       f_port;
    getopt_ptr_t                    f_opt;
};

snap_layout::snap_layout(int argc, char *argv[])
    //: f_cassandra() -- auto-init
    //, f_layouts() -- auto-init
    //, f_host -- auto-init
    //, f_port -- auto-init
    : f_opt( new advgetopt::getopt( argc, argv, g_snaplayout_options, g_configuration_files, NULL ) )
{
    if( f_opt->is_defined( "help" ) )
    {
        usage();
    }
    //
    f_host = f_opt->get_string( "host" ).c_str();
    f_port = f_opt->get_long  ( "port" );
    //
    if( !f_opt->is_defined( "--" ) )
    {
        std::cerr << "one or more layout files are required!" << std::endl;
        usage();
    }
    for( int idx(0); idx < f_opt->size( "--" ); ++idx )
    {
        f_layouts.push_back( f_opt->get_string( "--", idx ).c_str() );
    }
}

void snap_layout::usage()
{
    f_opt->usage( advgetopt::getopt::no_error, "snaplayout" );
    exit(1);
}

void snap_layout::load_xsl_info(QDomDocument& doc, const QString& filename, QString& layout_name, QString& layout_area, time_t& layout_modified)
{
    layout_name.clear();
    layout_area.clear();
    layout_modified = 0;

    QString layout_modified_date;

    QDomNodeList params(doc.elementsByTagNameNS("http://www.w3.org/1999/XSL/Transform", "param"));
    int max(params.size());
    for(int idx(0); idx < max; ++idx)
    {
        // all should be elements... but still verify
        QDomNode p(params.at(idx));
        if(!p.isElement())
        {
            continue;
        }
        QDomElement e(p.toElement());
        if(e.isNull())
        {
            continue;
        }

        // save in buffer, this is not too effective since
        // we convert all the parameters even though that
        // we'll throw away! but that way we don't have to
        // duplicate the code plus this process is not run
        // by the server
        QString buffer;
        QTextStream data(&buffer);
		QDomNodeList children(e.childNodes());
        if(children.size() != 1)
        {
            // that's most certainly the wrong name?
            continue;
        }
		QDomNode n(children.at(0));
        n.save(data, 0);

        // verify the name
        QString name(e.attribute("name"));
//#ifdef DEBUG
//printf("param name [%s] = [%s]\n", name.toUtf8().data(), buffer.toUtf8().data());
//#endif
        if(name == "layout-name")
        {
            // that's the row key
            layout_name = buffer;
        }
        else if(name == "layout-area")
        {
            // that's the name of the column
            layout_area = buffer;
        }
        else if(name == "layout-modified")
        {
            // that's to make sure we don't overwrite with an older version
            layout_modified_date = buffer;
        }
    }

    if(layout_name.isEmpty() || layout_area.isEmpty() || layout_modified_date.isEmpty())
    {
        std::cerr << "error: the layout_name, layout_area, and layout_modified parameters must all three be defined in your XSL document \"" << filename << "\"" << std::endl;
        exit(1);
    }

    // now convert the date, we expect a very specific format
    QDateTime t(QDateTime::fromString(layout_modified_date, "yyyy-MM-dd HH:mm:ss"));
    if(!t.isValid())
    {
        std::cerr << "error: the date \"" << layout_modified_date << "\" doesn't seem valid in \"" << filename << "\", the expected format is \"yyyy-MM-dd HH:mm:ss\"" << std::endl;
        exit(1);
    }
    layout_modified = t.toTime_t();
}

void snap_layout::add_files()
{
    f_cassandra.connect(f_host, f_port);
    if( !f_cassandra.isConnected() )
    {
        std::cerr << "Error connecting to cassandra server on host='"
            << f_host
            << "', port="
            << f_port
            << "!"
            << std::endl;
        exit(1);
    }

    QSharedPointer<QCassandraContext> context(f_cassandra.context("snap_websites"));

    QSharedPointer<QCassandraTable> table(context->findTable("layout"));
    if(table.isNull())
    {
        // TODO: look into whether we could make use of the
        //       server::create_table() function
        //
		// table is not there yet, create it
		table = context->table("layout");
		table->setComment("Table of layouts");
		table->setColumnType("Standard"); // Standard or Super
		table->setKeyValidationClass("BytesType");
		table->setDefaultValidationClass("BytesType");
		table->setComparatorType("BytesType");
		table->setKeyCacheSavePeriodInSeconds(14400);
		table->setMemtableFlushAfterMins(60);
		//table->setMemtableThroughputInMb(247);
		//table->setMemtableOperationsInMillions(1.1578125);
		table->setGcGraceSeconds(864000);
		table->setMinCompactionThreshold(4);
		table->setMaxCompactionThreshold(22);
		table->setReplicateOnWrite(1);
		table->create();
    }

    for(string_array_t::const_iterator it(f_layouts.begin());
                                       it != f_layouts.end();
                                       ++it)
    {
        QString filename(*it);
        int e(filename.lastIndexOf("."));
        if(e == -1 || filename.mid(e) != ".xsl")
        {
            std::cerr << "error: file \"" << filename << "\" must be an XSLT file (end with .xsl extension.)" << std::endl;
            exit(1);
        }
        QFile xsl(filename);
        if(!xsl.open(QIODevice::ReadOnly))
        {
            std::cerr << "error: file \"" << filename << "\" could not be opened for reading." << std::endl;
            exit(1);
        }
        QDomDocument doc("stylesheet");
        QString error_msg;
        int error_line, error_column;
        if(!doc.setContent(&xsl, true, &error_msg, &error_line, &error_column))
        {
            std::cerr << "error: file \"" << filename << "\" parsing failed";
            std::cerr << "detail " << error_line << "[" << error_column << "]: " << error_msg << std::endl;
            exit(1);
        }
        QString layout_name;
        QString layout_area;
        time_t layout_modified;
        load_xsl_info(doc, filename, layout_name, layout_area, layout_modified);

        if(table->exists(layout_name))
        {
            // the row already exists, try getting the area
            QCassandraValue existing(table->row(layout_name)->cell(layout_area)->value());
            if(!existing.nullValue())
            {
                QDomDocument existing_doc("stylesheet");
                if(!existing_doc.setContent(existing.stringValue(), true, &error_msg, &error_line, &error_column))
                {
                    std::cerr << "warning: existing XSLT data parsing failed, it will get replaced";
                    std::cerr << ", detail " << error_line << "[" << error_column << "]: " << error_msg << std::endl;
                    // it failed so we want to replace it with a valid XSLT document instead!
                }
                else
                {
                    QString existing_layout_name;
                    QString existing_layout_area;
                    time_t existing_layout_modified;
                    load_xsl_info(existing_doc, "<existing XSLT data>", existing_layout_name, existing_layout_area, existing_layout_modified);
                    // layout_name == existing_layout_name && layout_area == existing_layout_area
                    // (since we found that data at that location in the database!)
                    if(layout_modified < existing_layout_modified)
                    {
                        // we refuse older versions
                        // (if necessary we could add a command line option to force such though)
                        std::cerr << "error: existing XSLT data was created more recently than the one specified on the command line: \"" << filename << "\"." << std::endl;
                    }
                    else if(layout_modified == existing_layout_modified)
                    {
                        // we accept the exact same date but emit a warning
                        std::cerr << "warning: existing XSLT data has the same date, replacing with content of file \"" << filename << "\"." << std::endl;
                    }
                }
            }
        }
        xsl.reset();
        QByteArray value(xsl.readAll());
        table->row(layout_name)->cell(layout_area)->setValue(value);
    }
}




int main(int argc, char *argv[])
{
    snap_layout     s(argc, argv);

    s.add_files();

    return 0;
}

// vim: ts=4 sw=4 et
