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

#include <QtCassandra/QCassandra.h>
//#include <QtCore>
#include <algorithm>
#include <controlled_vars/controlled_vars_need_init.h>
#include <QDomDocument>
#include <QDomElement>
#include <QDateTime>
#include <QFile>
#include <QTextStream>

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
    QCassandra                      f_cassandra;
    typedef QVector<QString>        string_array_t;
    string_array_t                  f_layouts;
};

snap_layout::snap_layout(int argc, char *argv[])
    //: f_cassandra() -- auto-init
    //, f_layouts() -- auto-init
{
    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "-h") == 0
        || strcmp(argv[i], "--help") == 0)
        {
            usage();
        }
        else if(argv[i][0] == '-')
        {
            fprintf(stderr, "error: unknown command line option \"%s\".\n", argv[i]);
            exit(1);
        }
        else
        {
            f_layouts.push_back(argv[i]);
        }
    }
}

void snap_layout::usage()
{
    printf("Usage: snaplayout [--opts] layout-file.xsl ...\n");
    printf("By default snap db prints out the list of tables (column families) found in Cassandra.\n");
    printf("  -h | --help      print out this help screen.\n");
    printf("  layout-file.xsl  one or more XSLT files to add to the layout table.\n");
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
//printf("param name [%s] = [%s]\n", name.toUtf8().data(), buffer.toUtf8().data());
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
        fprintf(stderr, "error: the layout_name, layout_area, and layout_modified parameters must all three be defined in your XSL document \"%s\".\n", filename.toUtf8().data());
        exit(1);
    }

    // now convert the date, we expect a very specific format
    QDateTime t(QDateTime::fromString(layout_modified_date, "yyyy-MM-dd HH:mm:ss"));
    if(!t.isValid())
    {
        fprintf(stderr, "error: the date \"%s\" doesn't seem valid in \"%s\", the expected format is \"yyyy-MM-dd HH:mm:ss\".\n", layout_modified_date.toUtf8().data(), filename.toUtf8().data());
        exit(1);
    }
    layout_modified = t.toTime_t();
}

void snap_layout::add_files()
{
    f_cassandra.connect();
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
            fprintf(stderr, "error: file \"%s\" must be an XSLT file (end with .xsl extension.)\n", filename.toUtf8().data());
            exit(1);
        }
        QFile xsl(filename);
        if(!xsl.open(QIODevice::ReadOnly))
        {
            fprintf(stderr, "error: file \"%s\" could not be opened for reading.\n", filename.toUtf8().data());
            exit(1);
        }
        QDomDocument doc("stylesheet");
        QString error_msg;
        int error_line, error_column;
        if(!doc.setContent(&xsl, true, &error_msg, &error_line, &error_column))
        {
            fprintf(stderr, "error: file \"%s\" parsing failed, detail %d[%d]: %s\n", filename.toUtf8().data(), error_line, error_column, error_msg.toUtf8().data());
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
                    fprintf(stderr, "warning: existing XSLT data parsing failed, it will get replaced, detail %d[%d]: %s\n", error_line, error_column, error_msg.toUtf8().data());
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
                        fprintf(stderr, "error: existing XSLT data was created more recently than the one specified on the command line: \"%s\".\n", filename.toUtf8().data());
                    }
                    else if(layout_modified == existing_layout_modified)
                    {
                        // we accept the exact same date but emit a warning
                        fprintf(stderr, "warning: existing XSLT data has the same date, replacing with content of file \"%s\".\n", filename.toUtf8().data());
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
