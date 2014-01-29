/*
 * Text:
 *      snaplayout.cpp
 *
 * Description:
 *      Save layout files in the Snap database.
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

#include "snap_version.h"
#include "qstring_stream.h"

#include <advgetopt/advgetopt.h>
#include <controlled_vars/controlled_vars_need_init.h>
#include <QtCassandra/QCassandra.h>

#include <algorithm>

#include <QDomDocument>
#include <QDomElement>
#include <QDateTime>
#include <QFile>
#include <QTextStream>


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
        { // at least until we have a way to edit the theme from the website
            't',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "set-theme",
            NULL,
            "set-theme",
            advgetopt::getopt::no_argument, // expect 3 params as filenames
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
    void run();
    void add_files();
    void load_xml_info(QDomDocument& doc, QString const& filename, QString& layout_name, time_t& layout_modified);
    void load_xsl_info(QDomDocument& doc, QString const& filename, QString& layout_name, QString& layout_area, time_t& layout_modified);
    void load_css(QString const& filename, QString& row_name);
    void set_theme();

private:
    typedef std::shared_ptr<advgetopt::getopt>    getopt_ptr_t;

    QCassandra::pointer_t           f_cassandra;
    typedef QVector<QString>        string_array_t;
    string_array_t                  f_layouts;
    QString                         f_host;
    controlled_vars::zint32_t       f_port;
    getopt_ptr_t                    f_opt;
};


snap_layout::snap_layout(int argc, char *argv[])
    : f_cassandra( QCassandra::create() )
    //, f_layouts() -- auto-init
    //, f_host -- auto-init
    //, f_port -- auto-init
    , f_opt( new advgetopt::getopt( argc, argv, g_snaplayout_options, g_configuration_files, NULL ) )
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


void snap_layout::load_xml_info(QDomDocument& doc, QString const& filename, QString& content_name, time_t& content_modified)
{
    content_name.clear();
    content_modified = 0;

    QDomElement snap_tree(doc.documentElement());
    if(snap_tree.isNull())
    {
        std::cerr << "error: the XML document does not have a root element, failed handling \"" << filename << "\"" << std::endl;
        exit(1);
    }
    QString content_modified_date(snap_tree.attribute("content-modified", "0"));

    QDomNodeList content(snap_tree.elementsByTagName("content"));
    int max(content.size());
    for(int idx(0); idx < max; ++idx)
    {
        // all should be elements... but still verify
        QDomNode p(content.at(idx));
        if(!p.isElement())
        {
            continue;
        }
        QDomElement e(p.toElement());
        if(e.isNull())
        {
            continue;
        }
        QString const path(e.attribute("path", ""));
        if(path.isEmpty())
        {
            // this is probably an error
            continue;
        }
        if(path.startsWith("/admin/layouts/"))
        {
            int pos(path.indexOf('/', 15));
           // int const end(path.indexOf('/', pos + 1));
            if(pos < 0)
            {
                pos = path.length();
            }
            QString name(path.mid(15, pos - 15));
            if(name.isEmpty())
            {
                std::cerr << "error: the XML document seems to have an invalid path in \"" << filename << "\"" << std::endl;
                exit(1);
            }
            if(content_name.isEmpty())
            {
                content_name = name;
            }
            else if(content_name != name)
            {
                std::cerr << "error: the XML document includes two different entries with layout paths that differ: \""
                          << content_name << "\" and \"" << name << "\" in \"" << filename << "\"" << std::endl;
                exit(1);
            }
        }
    }

    if(content_name.isEmpty())
    {
        std::cerr << "error: the XML document is missing a path to a layout in \"" << filename << "\"" << std::endl;
        exit(1);
    }
    if(content_modified_date.isEmpty())
    {
        std::cerr << "error: the XML document is missing its content-modified attribute in your XML document \"" << filename << "\"" << std::endl;
        exit(1);
    }

    // now convert the date, we expect a very specific format
    QDateTime t(QDateTime::fromString(content_modified_date, "yyyy-MM-dd HH:mm:ss"));
    if(!t.isValid())
    {
        std::cerr << "error: the date \"" << content_modified_date << "\" doesn't seem valid in \"" << filename << "\", the expected format is \"yyyy-MM-dd HH:mm:ss\"" << std::endl;
        exit(1);
    }
    content_modified = t.toTime_t();
}


void snap_layout::load_xsl_info(QDomDocument& doc, QString const& filename, QString& layout_name, QString& layout_area, time_t& layout_modified)
{
    layout_name.clear();
    layout_area.clear();
    layout_modified = 0;

    QString layout_modified_date;

    QDomNodeList params(doc.elementsByTagNameNS("http://www.w3.org/1999/XSL/Transform", "variable"));
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
        QDomNodeList children(e.childNodes());
        if(children.size() != 1)
        {
            // that's most certainly the wrong name?
            continue;
        }
        QDomNode n(children.at(0));

        QString buffer;
        QTextStream data(&buffer);
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
            // that's to make sure we don't overwrite a newer version
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


void snap_layout::load_css(QString const& filename, QString& row_name)
{
    // TODO: once we have a CSS compressor, use it to verify that the
    //       data is valid; but save the full thing because we want
    //       the original in the database
    QFile css(filename);
    if(!css.open(QIODevice::ReadOnly))
    {
        std::cerr << "error: could not open file named \"" << filename << "\"" << std::endl;
        exit(1);
    }
    QByteArray value(css.readAll());
    snap::snap_version::quick_find_version_in_source fv;
    if(!fv.find_version(value.data(), value.size()))
    {
        std::cerr << "error: the CSS file \"" << filename << "\" does not include a valid introducer comment." << std::endl;
        exit(1);
    }
    // valid comment, but we need to have a name which is not mandatory
    // in the find_version() function.
    row_name = fv.get_name();
    if(row_name.isEmpty())
    {
        std::cerr << "error: the CSS file \"" << filename << "\" does not define the Name: field. We cannot know where to save it." << std::endl;
        exit(1);
    }
}


void snap_layout::add_files()
{
    f_cassandra->connect(f_host, f_port);
    if( !f_cassandra->isConnected() )
    {
        std::cerr << "Error connecting to cassandra server on host='"
            << f_host
            << "', port="
            << f_port
            << "!"
            << std::endl;
        exit(1);
    }

    QCassandraContext::pointer_t context(f_cassandra->context("snap_websites"));

    QCassandraTable::pointer_t table(context->findTable("layout"));
    if(!table)
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
        if(e == -1)
        {
            std::cerr << "error: file \"" << filename << "\" must be an XML file (end with the .xml or .xsl extension.)" << std::endl;
            exit(1);
        }
        QFile xml(filename);
        QString row_name; // == <layout name>
        QString cell_name; // == <layout_area>  or 'content'
        QString const extension(filename.mid(e));
        if(extension == ".xml") // expects the content.xml file
        {
            // the cell name is always "content" in this case
            cell_name = "content";
            if(!xml.open(QIODevice::ReadOnly))
            {
                std::cerr << "error: XML file \"" << filename << "\" could not be opened for reading." << std::endl;
                exit(1);
            }
            QDomDocument doc("content");
            QString error_msg;
            int error_line, error_column;
            if(!doc.setContent(&xml, false, &error_msg, &error_line, &error_column))
            {
                std::cerr << "error: file \"" << filename << "\" parsing failed";
                std::cerr << "detail " << error_line << "[" << error_column << "]: " << error_msg << std::endl;
                exit(1);
            }
            time_t layout_modified;
            load_xml_info(doc, filename, row_name, layout_modified);
        }
        else if(extension == ".css") // expects the style.css file
        {
            // the cell name is always "style" in this case
            cell_name = "style";
            if(!xml.open(QIODevice::ReadOnly))
            {
                std::cerr << "error: CSS file \"" << filename << "\" could not be opened for reading." << std::endl;
                exit(1);
            }
            load_css(filename, row_name);
        }
        else if(extension == ".xsl") // expects the body or theme XSLT files
        {
            if(!xml.open(QIODevice::ReadOnly))
            {
                std::cerr << "error: XSTL file \"" << filename << "\" could not be opened for reading." << std::endl;
                exit(1);
            }
            QDomDocument doc("stylesheet");
            QString error_msg;
            int error_line, error_column;
            if(!doc.setContent(&xml, true, &error_msg, &error_line, &error_column))
            {
                std::cerr << "error: file \"" << filename << "\" parsing failed";
                std::cerr << "detail " << error_line << "[" << error_column << "]: " << error_msg << std::endl;
                exit(1);
            }
            time_t layout_modified;
            load_xsl_info(doc, filename, row_name, cell_name, layout_modified);

            if(table->exists(row_name))
            {
                // the row already exists, try getting the area
                QCassandraValue existing(table->row(row_name)->cell(cell_name)->value());
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
                        // row_name == existing_layout_name && cell_name == existing_layout_area
                        // (since we found that data at that location in the database!)
                        if(layout_modified < existing_layout_modified)
                        {
                            // we refuse older versions
                            // (if necessary we could add a command line option to force such though)
                            std::cerr << "error: existing XSLT data was created more recently than the one specified on the command line: \"" << filename << "\"." << std::endl;
                            exit(1);
                        }
                        else if(layout_modified == existing_layout_modified)
                        {
                            // we accept the exact same date but emit a warning
                            std::cerr << "warning: existing XSLT data has the same date, replacing with content of file \"" << filename << "\"." << std::endl;
                        }
                    }
                }
            }
        }
        else
        {
            std::cerr << "error: file \"" << filename << "\" must be an XML file (end with the .xml or .xsl extension.)" << std::endl;
            exit(1);
        }
        xml.reset();
        QByteArray value(xml.readAll());
        table->row(row_name)->cell(cell_name)->setValue(value);
    }
}


void snap_layout::set_theme()
{
    if(f_layouts.size() != 3)
    {
        std::cerr << "error: the --set-theme command expects exactly three arguments." << std::endl;
        exit(1);
    }

    // the theme for the entire website is set at the top of the
    // page type structure .../types/taxonomy/system/content-types
    f_cassandra->connect(f_host, f_port);
    if( !f_cassandra->isConnected() )
    {
        std::cerr << "error: connecting to cassandra server on host='"
            << f_host
            << "', port="
            << f_port
            << "!"
            << std::endl;
        exit(1);
    }

    QCassandraContext::pointer_t context(f_cassandra->context("snap_websites"));

    QCassandraTable::pointer_t table(context->findTable("content"));
    if(!table)
    {
        std::cerr << "Content table not found. You must run the server once before we can setup the theme." << std::endl;
        exit(1);
    }

    QString uri(f_layouts[0]);
    QString field(f_layouts[1]);
    QString theme(f_layouts[2]);

    if(!uri.endsWith("/"))
    {
        uri += "/";
    }

    if(field == "layout")
    {
        field = "layout::layout";
    }
    else if(field == "theme")
    {
        field = "layout::theme";
    }
    else
    {
        std::cerr << "the name of the field must be \"layout\" or \"theme\"." << std::endl;
        exit(1);
    }

    QString const key(QString("%1types/taxonomy/system/content-types").arg(uri));
    if(!table->exists(key))
    {
        std::cerr << "content-types not found for domain \"" << uri << "\"." << std::endl;
        exit(1);
    }

    if(theme.isEmpty())
    {
        // remove the theme definition
        table->row(key)->dropCell(field, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
    }
    else
    {
        // remember that the layout specification is a JavaScript script
        // and not just plain text
        //
        // TODO: add a test so we can transform a simple string to a valid
        //       JavaScript string
        table->row(key)->cell(field)->setValue(theme);
    }
}


void snap_layout::run()
{
    if( f_opt->is_defined( "set-theme" ) )
    {
        set_theme();
    }
    else
    {
        add_files();
    }
}




int main(int argc, char *argv[])
{
    snap_layout     s(argc, argv);

    s.run();

    return 0;
}

// vim: ts=4 sw=4 et
