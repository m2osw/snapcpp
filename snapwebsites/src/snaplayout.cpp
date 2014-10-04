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
#include "snap_image.h"
#include "snapwebsites.h"
#include "qstring_stream.h"
#include "snap_cassandra.h"
#include "snap_config.h"

#include <advgetopt/advgetopt.h>
#include <controlled_vars/controlled_vars_need_init.h>
#include <QtCassandra/QCassandra.h>

#include <algorithm>
#include <iostream>
#include <sstream>

#include <sys/stat.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#include <zipios++/zipfile.h>
#pragma GCC diagnostic pop

#include <QDomDocument>
#include <QDomElement>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QXmlInputSource>


namespace
{
    const std::vector<std::string> g_configuration_files; // Empty

    const advgetopt::getopt::option g_snaplayout_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: %p [-<opt>] <layout filename> ...",
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
            '?',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "show this help output",
            advgetopt::getopt::no_argument
        },
        {
            'c',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "config",
            "/etc/snapwebsites/snapserver.conf",
            "Specify the configuration file to load at startup.",
            advgetopt::getopt::optional_argument
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
            0,
            "remove-theme",
            nullptr,
            "remove the specified theme; this remove the entire row and can allow you to reinstall a theme that \"lost\" files",
            advgetopt::getopt::no_argument
        },
        { // at least until we have a way to edit the theme from the website
            't',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "set-theme",
            nullptr,
            "usage: --set-theme URL [theme|layout] ['\"layout name\";']'",
            advgetopt::getopt::no_argument // expect 3 params as filenames
        },
        {
            'v',
            0,
            "verbose",
            nullptr,
            "show what snaplayout is doing",
            advgetopt::getopt::no_argument // expect 3 params as filenames
        },
        {
            '\0',
            0,
            "version",
            nullptr,
            "show the version of the server and exit",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            nullptr,
            nullptr,
            "layout-file1.xsl layout-file2.xsl ... layout-fileN.xsl or layout.zip",
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


    void stream_to_bytearray( std::istream* is, QByteArray& arr )
    {
        arr.clear();
        while( !is->eof() )
        {
            char ch = '\0';
            is->get( ch );
            if( !is->eof() )
            {
                arr.push_back( ch );
            }
        }
    }
}
//namespace

using namespace QtCassandra;
using namespace snap;


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
    bool load_xml_info(QDomDocument& doc, QString const& filename, QString& layout_name, time_t& layout_modified);
    void load_xsl_info(QDomDocument& doc, QString const& filename, QString& layout_name, QString& layout_area, time_t& layout_modified);
    void load_css  (QString const& filename, QByteArray const& content, QString& row_name);
    void load_js   (QString const& filename, QByteArray const& content, QString& row_name);
    void load_image(QString const& filename, QByteArray const& content, QString& row_name);
    void set_theme();
    void remove_theme();

private:
    typedef std::shared_ptr<advgetopt::getopt>    getopt_ptr_t;
    snap_cassandra  f_cassandra;

    // Layout file structure
    //
    struct fileinfo_t
    {
        QString    f_filename;
        QByteArray f_content;
        time_t     f_filetime;

        fileinfo_t() : f_filetime(0) {}
        fileinfo_t( QString const& fn, QByteArray const& content, time_t const time )
            : f_filename(fn), f_content(content), f_filetime(time) {}
    };
    typedef std::vector<fileinfo_t>   fileinfo_list_t;
    fileinfo_list_t                   f_fileinfo_list;

    // Common attributes
    //
    getopt_ptr_t                    f_opt;
    controlled_vars::fbool_t        f_verbose;
    snap_config                     f_parameters;

    // Private methods
    //
    QCassandraContext::pointer_t    get_snap_context();
};


snap_layout::snap_layout(int argc, char *argv[])
    //: f_cassandra() )
    //, f_fileinfo_list -- auto-init
    : f_opt( new advgetopt::getopt( argc, argv, g_snaplayout_options, g_configuration_files, "SNAPSERVER_OPTIONS" ) )
{
    if( f_opt->is_defined( "help" ) )
    {
        usage();
    }
    if( f_opt->is_defined( "version" ) )
    {
        std::cerr << SNAPWEBSITES_VERSION_STRING << std::endl;
        exit(1);
    }
    //
    f_parameters.read_config_file( f_opt->get_string( "config" ).c_str() );
    //
    if( !f_opt->is_defined( "--" ) )
    {
        if( f_opt->is_defined( "set-theme" ) )
        {
            std::cerr << "usage: snaplayout --set-theme URL [theme|layout] ['\"layout_name\";']'" << std::endl;
            std::cerr << "note: if layout_name is not specified, the theme/layout is deleted from the database." << std::endl;
            exit(1);
            /*NOTREACHED*/
        }
        if( f_opt->is_defined( "remove-theme" ) )
        {
            std::cerr << "usage: snaplayout --remove-theme <layout name>" << std::endl;
            exit(1);
            /*NOTREACHED*/
        }
        std::cerr << "one or more layout files are required!" << std::endl;
        usage();
        /*NOTREACHED*/
    }
    if(!f_opt->is_defined("set-theme")
    && !f_opt->is_defined("remove-theme"))
    {
        for( int idx(0); idx < f_opt->size( "--" ); ++idx )
        {
            QString const filename ( f_opt->get_string( "--", idx ).c_str() );
            const int e(filename.lastIndexOf("."));
            QString const extension( filename.mid(e) );
            if( extension == ".zip" )
            {
                std::cout << "Unpacking zipfile '" << filename << "':" << std::endl;

                zipios::ZipFile zf( filename.toUtf8().data() );
                if( zf.size() < 0 )
                {
                    std::cerr << "error: could not open zipfile \"" << filename << "\"" << std::endl;
                    exit(1);
                }
                //
                for( auto ent : zf.entries() )
                {
                    if( ent && ent->isValid() && !ent->isDirectory() )
                    {
                        std::cout << "\t" << *ent << std::endl;

                        const std::string fn( ent->getName() );
                        try
                        {
                            std::auto_ptr< std::istream > is( zf.getInputStream( ent ) ) ;

                            QByteArray byte_arr;
                            stream_to_bytearray( is.get(), byte_arr );

                            f_fileinfo_list.push_back( fileinfo_t( fn.c_str(), byte_arr, ent->getUnixTime() ) );
                        }
                        catch( const std::ios_base::failure& except )
                        {
                            std::cerr << "Caught an ios_base::failure when trying to extract file '"
                                << fn << "'." << std::endl
                                << "Explanatory string: " << except.what() << std::endl
                                //<< "Error code: " << except.code() << std::endl
                                ;
                            exit(1);
                        }
                        catch( const std::exception& except )
                        {
                            std::cerr << "Error extracting '" << fn << "': Exception caught: " << except.what() << std::endl;
                            exit(1);
                        }
                        catch( ... )
                        {
                            std::cerr << "Caught unknown exception attempting to extract '" << fn << "'!" << std::endl;
                            exit(1);
                        }
                    }
                }
            }
            else
            {
                std::ifstream ifs( filename.toUtf8().data() );
                if( !ifs.is_open() )
                {
                    std::cerr << "error: could not open layout file named \"" << filename << "\"" << std::endl;
                    exit(1);
                }

                time_t filetime(0);
                struct stat s;
                if( stat(filename.toUtf8().data(), &s) == 0 )
                {
                    filetime = s.st_mtime;
                }
                else
                {
                    std::cerr << "error: could not get mtime from file \"" << filename << "\"." << std::endl;
                    exit(1);
                }

                QByteArray byte_arr;
                stream_to_bytearray( &ifs, byte_arr );
                f_fileinfo_list.push_back( fileinfo_t( filename, byte_arr, filetime ) );
            }
        }
    }
}


void snap_layout::usage()
{
    f_opt->usage( advgetopt::getopt::no_error, "snaplayout" );
    exit(1);
    /*NOTREACHED*/
}


bool snap_layout::load_xml_info(QDomDocument& doc, QString const& filename, QString& content_name, time_t& content_modified)
{
    content_name.clear();
    content_modified = 0;

    QDomElement snap_tree(doc.documentElement());
    if(snap_tree.isNull())
    {
        std::cerr << "error: the XML document does not have a root element, failed handling \"" << filename << "\"" << std::endl;
        exit(1);
    }
    QString const content_modified_date(snap_tree.attribute("content-modified", "0"));

    bool const result(snap_tree.tagName() == "snap-tree");
    if(result)
    {
        QDomNodeList const content(snap_tree.elementsByTagName("content"));
        int const max_nodes(content.size());
        for(int idx(0); idx < max_nodes; ++idx)
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
    }
    else
    {
        content_name = snap_tree.attribute("owner");
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

    return result;
}


void snap_layout::load_xsl_info(QDomDocument& doc, QString const& filename, QString& layout_name, QString& layout_area, time_t& layout_modified)
{
    layout_name.clear();
    layout_area.clear();
    layout_modified = 0;

    QString layout_modified_date;

    QDomNodeList params(doc.elementsByTagNameNS("http://www.w3.org/1999/XSL/Transform", "variable"));
    int const max_nodes(params.size());
    for(int idx(0); idx < max_nodes; ++idx)
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
        std::cerr << "error: the layout-name, layout-area, and layout-modified parameters must all three be defined in your XSL document \"" << filename << "\"" << std::endl;
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


void snap_layout::load_css(QString const& filename, QByteArray const& content, QString& row_name)
{
    snap::snap_version::quick_find_version_in_source fv;
    if(!fv.find_version(content.data(), content.size()))
    {
        std::cerr << "error: the CSS file \"" << filename << "\" does not include a valid introducer comment." << std::endl;
        exit(1);
    }
    // valid comment, but we need to have a name which is not mandatory
    // in the find_version() function.
    if(fv.get_name().isEmpty())
    {
        std::cerr << "error: the CSS file \"" << filename << "\" does not define the Name: field. We cannot know where to save it." << std::endl;
        exit(1);
    }
    // now we force a Layout: field for CSS files defined in a layout
    row_name = fv.get_layout();
    if(row_name.isEmpty())
    {
        std::cerr << "error: the CSS file \"" << filename << "\" does not define the Layout: field. We cannot know where to save it." << std::endl;
        exit(1);
    }
}


void snap_layout::load_js(QString const& filename, QByteArray const& content, QString& row_name)
{
    snap::snap_version::quick_find_version_in_source fv;
    if(!fv.find_version(content.data(), content.size()))
    {
        std::cerr << "error: the JS file \"" << filename << "\" does not include a valid introducer comment." << std::endl;
        exit(1);
    }
    // valid comment, but we need to have a name which is not mandatory
    // in the find_version() function.
    if(fv.get_name().isEmpty())
    {
        std::cerr << "error: the JS file \"" << filename << "\" does not define the Name: field. We cannot know where to save it." << std::endl;
        exit(1);
    }
    // now we force a Layout: field for JavaScript files defined in a layout
    row_name = fv.get_layout();
    if(row_name.isEmpty())
    {
        std::cerr << "error: the JS file \"" << filename << "\" does not define the Layout: field. We cannot know where to save it." << std::endl;
        exit(1);
    }
}


void snap_layout::load_image( QString const& filename, QByteArray const& content, QString& row_name)
{
    row_name = filename;
    int pos(row_name.lastIndexOf('/'));
    if(pos < 0)
    {
        std::cerr << "error: the image file does not include the name of the theme." << std::endl;
        exit(1);
    }
    row_name = row_name.mid(0, pos);
    pos = row_name.lastIndexOf('/');
    if(pos >= 0)
    {
        row_name = row_name.mid(pos + 1);
    }

    snap::snap_image img;
    if(!img.get_info(content))
    {
        std::cerr << "error: \"image\" file named \"" << filename << "\" does not use a recognized image file format." << std::endl;
        exit(1);
    }
}


QCassandraContext::pointer_t snap_layout::get_snap_context()
{
    // Use command line options if they are set...
    //
    if( f_opt->is_defined( "host" ) )
    {
        f_parameters["cassandra_host"] = f_opt->get_string( "host" ).c_str();
    }
    if( f_opt->is_defined( "port" ) )
    {
        f_parameters["cassandra_port"] = f_opt->get_string( "port" ).c_str();
    }

    f_cassandra.connect( f_parameters );
    if( !f_cassandra.is_connected() )
    {
        std::cerr << "error: connecting to cassandra server on host='"
            << f_cassandra.get_cassandra_host()
            << "', port="
            << f_cassandra.get_cassandra_port()
            << "!"
            << std::endl;
        exit(1);
    }

    return f_cassandra.get_snap_context();
}


void snap_layout::add_files()
{
    QCassandraContext::pointer_t context( get_snap_context() );

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

    typedef QMap<QString, time_t> mtimes_t;
    mtimes_t mtimes;
    for( auto info : f_fileinfo_list )
    {
        QString const filename(info.f_filename);
        if(f_verbose)
        {
            std::cout << "info: working on \"" << filename << "\"." << std::endl;
        }
        QByteArray content(info.f_content);
        int const e(filename.lastIndexOf("."));
        if(e == -1)
        {
            std::cerr << "error: file \"" << filename << "\" must be an XML file (end with the .xml, .xsl or .zip extension.)" << std::endl;
            exit(1);
        }
        QString row_name; // == <layout name>
        QString cell_name; // == <layout_area>  or 'content'
        QString const extension(filename.mid(e));
        if(extension == ".xml") // expects the content.xml file
        {
            QDomDocument doc("content");
            QString error_msg;
            int error_line, error_column;
            content.push_back(' ');
            if(!doc.setContent(content, false, &error_msg, &error_line, &error_column))
            {
                std::cerr << "error: file \"" << filename << "\" parsing failed." << std::endl;
                std::cerr << "detail " << error_line << "[" << error_column << "]: " << error_msg << std::endl;
                exit(1);
            }
            time_t layout_modified;
            if(load_xml_info(doc, filename, row_name, layout_modified))
            {
                cell_name = "content";
            }
            else
            {
                cell_name = filename;
                int const last_slash(cell_name.lastIndexOf('/'));
                if(last_slash >= 0)
                {
                    cell_name = cell_name.mid(last_slash + 1);
                }
                int const last_period(cell_name.lastIndexOf('.'));
                if(last_period > 0)
                {
                    cell_name = cell_name.mid(0, last_period);
                }
            }
        }
        else if(extension == ".css") // a CSS file
        {
            // the cell name is the basename
            cell_name = filename;
            int const pos(cell_name.lastIndexOf('/'));
            if(pos >= 0)
            {
                cell_name = cell_name.mid(pos + 1);
            }
            load_css(filename, content, row_name);
        }
        else if(extension == ".js") // a JavaScript file
        {
            // the cell name is the basename with the extension
            cell_name = filename;
            int const pos(cell_name.lastIndexOf('/'));
            if(pos >= 0)
            {
                cell_name = cell_name.mid(pos + 1);
            }
            load_js(filename, content, row_name);
        }
        else if(extension == ".png" || extension == ".gif"
             || extension == ".jpg" || extension == ".jpeg") // expects images
        {
            cell_name = filename;
            int const pos(cell_name.lastIndexOf('/'));
            if(pos >= 0)
            {
                cell_name = cell_name.mid(pos + 1);
            }
            load_image(filename, content, row_name);
        }
        else if(extension == ".xsl") // expects the body or theme XSLT files
        {
            QDomDocument doc("stylesheet");
            QString error_msg;
            int error_line, error_column;
            content.push_back(' ');
            if(!doc.setContent(content, true, &error_msg, &error_line, &error_column))
            {
                std::cerr << "error: file \"" << filename << "\" parsing failed." << std::endl;
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
                        std::cerr << "warning: existing XSLT data parsing failed, it will get replaced." << std::endl;
                        std::cerr << "details: " << error_line << "[" << error_column << "]: " << error_msg << std::endl;
                        // it failed so we want to replace it with a valid XSLT document instead!
                    }
                    else
                    {
                        QString existing_layout_name;
                        QString existing_layout_area;
                        time_t existing_layout_modified;
                        load_xsl_info(existing_doc, QString("<existing XSLT data for %1>").arg(filename), existing_layout_name, existing_layout_area, existing_layout_modified);
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
            std::cerr << "error: file \"" << filename << "\" must be an XML file (end with the .xml or .xsl extension,) a CSS file (end with .css,) a JavaScript file (end with .js,) or be an image (end with .gif, .png, .jpg, .jpeg.)" << std::endl;
            exit(1);
        }

        table->row(row_name)->cell(cell_name)->setValue(content);

        // set last modification time
        if( !mtimes.contains(row_name) || mtimes[row_name] < info.f_filetime )
        {
            mtimes[row_name] = info.f_filetime;
        }
    }

    for( mtimes_t::const_iterator i(mtimes.begin()); i != mtimes.end(); ++i )
    {
        // mtimes holds times in seconds, convert to microseconds
        int64_t last_updated(i.value() * 1000000);
        QtCassandra::QCassandraValue existing_last_updated(table->row(i.key())->cell(snap::get_name(snap::SNAP_NAME_CORE_LAST_UPDATED))->value());
        if(existing_last_updated.nullValue()
        || existing_last_updated.int64Value() < last_updated)
        {
            table->row(i.key())->cell(snap::get_name(snap::SNAP_NAME_CORE_LAST_UPDATED))->setValue(last_updated);
        }
    }
}


void snap_layout::set_theme()
{
    const auto arg_count( f_opt->size("--") );
    if( (arg_count != 2) && (arg_count != 3) )
    {
        std::cerr << "error: the --set-theme command expects 2 or 3 arguments." << std::endl;
        exit(1);
    }

    QCassandraContext::pointer_t context( get_snap_context() );

    QCassandraTable::pointer_t table(context->findTable("content"));
    if(!table)
    {
        std::cerr << "Content table not found. You must run the server once before we can setup the theme." << std::endl;
        exit(1);
    }

    QString uri         ( f_opt->get_string( "--", 0 ).c_str() );
    QString field       ( f_opt->get_string( "--", 1 ).c_str() );
    QString const theme ( (arg_count == 3)? f_opt->get_string( "--", 2 ).c_str(): QString() );

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

    if( theme.isEmpty() )
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


void snap_layout::remove_theme()
{
    const auto arg_count( f_opt->size("--") );
    if( arg_count != 1 )
    {
        std::cerr << "error: the --remove-theme command expects 1 argument." << std::endl;
        exit(1);
    }

    QCassandraContext::pointer_t context( get_snap_context() );

    QCassandraTable::pointer_t table(context->findTable("layout"));
    if(!table)
    {
        std::cerr << "warning: \"layout\" table not found. If you do not yet have a layout table then no theme can be deleted." << std::endl;
        exit(1);
    }

    QString const row_name( f_opt->get_string( "--", 0 ).c_str() );
    if(!table->exists(row_name))
    {
        std::cerr << "warning: \"" << row_name << "\" layout not found." << std::endl;
        exit(1);
    }

    if(!table->row(row_name)->exists("theme"))
    {
        std::cerr << "warning: it looks like the \"" << row_name << "\" layout did not exist (no \"theme\" found)." << std::endl;
    }

    // drop the entire row; however, remember that does not really delete
    // the row itself for a while (it's still visible in the database)
    table->dropRow(row_name);

    if(f_verbose)
    {
        std::cout << "info: theme \"" << row_name << "\" dropped." << std::endl;
    }
}


void snap_layout::run()
{
    f_verbose = f_opt->is_defined("verbose");

    if( f_opt->is_defined( "set-theme" ) )
    {
        set_theme();
    }
    else if( f_opt->is_defined( "remove-theme" ) )
    {
        remove_theme();
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
