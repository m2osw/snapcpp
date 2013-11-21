// Snap Websites Server -- handle the theme/layout information
// Copyright (C) 2011-2013  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "layout.h"
#include "plugins.h"
#include "log.h"
#include "qdomreceiver.h"
#include "qhtmlserializer.h"
//#include "qdomnodemodel.h" -- at this point the DOM Node Model seems bogus.
#include "../filter/filter.h"
#include "../content/content.h"
#include "../taxonomy/taxonomy.h"
//#include "snap_parser.h"
#include "not_reached.h"
#include <iostream>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QXmlQuery>
#include <QDomDocument>
#include <QFile>
#include <QDate>
#include <QXmlResultItems>
#pragma GCC diagnostic pop


SNAP_PLUGIN_START(layout, 1, 0)

/** \brief Get a fixed layout name.
 *
 * The layout plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_LAYOUT_TABLE:
        return "layout";

    case SNAP_NAME_LAYOUT_THEME:
        return "layout::theme";

    case SNAP_NAME_LAYOUT_LAYOUT:
        return "layout::layout";

    default:
        // invalid index
        throw snap_exception();

    }
    NOTREACHED();
}


/** \brief Initialize the layout plugin.
 *
 * This function is used to initialize the layout plugin object.
 */
layout::layout()
    //: f_snap(NULL) -- auto-init
{
}

/** \brief Clean up the layout plugin.
 *
 * Ensure the layout object is clean before it is gone.
 */
layout::~layout()
{
}

/** \brief Initialize the layout.
 *
 * This function terminates the initialization of the layout plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void layout::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    //if(plugins::exists("javascript"))
    //{
    //    javascript::javascript::instance()->register_dynamic_plugin(this);
    //}
}

/** \brief Get a pointer to the layout plugin.
 *
 * This function returns an instance pointer to the layout plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the layout plugin.
 */
layout *layout::instance()
{
    return g_plugin_layout_factory.instance();
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString layout::description() const
{
    return "Determine the layout for a given content and generate the output"
            " for that layout.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t layout::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief First update to run for the layout plugin.
 *
 * This function is the first update for the layout plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void layout::initial_update(int64_t variables_timestamp)
{
}

/** \brief Update the database with our layout references.
 *
 * Send our layout to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void layout::content_update(int64_t variables_timestamp)
{
    content::content::instance()->add_xml("layout");
}

/** \brief Initialize the layout table.
 *
 * This function creates the layout table if it doesn't exist yet. Otherwise
 * it simple retrieves it from Cassandra.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \return The shared pointer to the layout table.
 */
QSharedPointer<QtCassandra::QCassandraTable> layout::get_layout_table()
{
    return f_snap->create_table(get_name(SNAP_NAME_LAYOUT_TABLE), "Layouts table.");
}

/** \brief Retrieve the name of a theme or layout.
 *
 * This function checks for the name of a theme or layout in the current object
 * or the specified type and its parent.
 *
 * \param[in] cpath  The path to the content to process.
 * \param[in] column_name  The name of the column to search (layout::theme or layout::layout)
 */
QString layout::get_layout(const QString& cpath, const QString& column_name)
{
    // get the full key
    QString key(f_snap->get_site_key_with_slash() + cpath);

    // get the content table first
    QtCassandra::QCassandraValue layout_value(content::content::instance()->get_content_table()->row(key)->cell(column_name)->value());
    if(layout_value.nullValue())
    {
        // that very content doesn't define a layout, check its type(s)
        layout_value = taxonomy::taxonomy::instance()->find_type_with(
            cpath,
            content::get_name(content::SNAP_NAME_CONTENT_PAGE_CONTENT_TYPE),
            column_name,
            content::get_name(content::SNAP_NAME_CONTENT_CONTENT_TYPES_NAME)
        );
        if(layout_value.nullValue())
        {
            // user did not define any layout, set the value to "default"
            layout_value = QString("\"default\"");
        }
    }

    QString layout_script(layout_value.stringValue());
    bool run_script(true);
    if(layout_script.startsWith("\"")
    && (layout_script.endsWith("\"") || layout_script.endsWith("\";")))
    {
        run_script = false;
        for(const char *s(layout_script.toUtf8().data() + 1); *s != '\0'; ++s)
        {
            if((*s < 'a' || *s > 'z')
            && (*s < 'A' || *s > 'Z')
            && (*s < '0' || *s > '9')
            && *s != '_')
            {
                run_script = true;
                break;
            }
        }
    }

    QString layout_name;
    if(run_script)
    {
        QVariant v(javascript::javascript::instance()->evaluate_script(layout_script));
        layout_name = v.toString();
    }
    else
    {
        // remove the quotes really quick, we avoid the whole JS deal!
        if(layout_script.endsWith("\";"))
        {
            layout_name = layout_script.mid(1, layout_script.length() - 3);
        }
        else
        {
            layout_name = layout_script.mid(1, layout_script.length() - 2);
        }
    }

    return layout_name;
}

/** \brief Apply the layout to the content defined at \p cpath.
 *
 * This function defines a page content using the data as defined by \p cpath.
 *
 * First it looks for a JavaScript under the column key "layout::theme".
 * If such doesn't exist at cpath, then the function checks the \p cpath
 * content type link. If that type of content has no "layout::theme" then
 * the parent type is checked up to the "Content Types" type.
 *
 * The result is a new document with the data found at cpath and any
 * references as determine by the theme and layouts used by the process.
 * The type of the new document depends on the layout (it could be XHTML,
 * XML, PDF, text, SVG, etc.)
 *
 * \param[in] cpath  The canonalized path of content to be laid out.
 * \param[in] content_plugin  The plugin that will generate the content of the page.
 *
 * \return The result is the output of the layout applied to the data in cpath.
 */
QString layout::apply_layout(const QString& cpath, layout_content *content_plugin)
{
    // Retrieve the theme and layout for this path
    QString theme_name(get_layout(cpath, get_name(SNAP_NAME_LAYOUT_THEME)));
    QString layout_name(get_layout(cpath, get_name(SNAP_NAME_LAYOUT_LAYOUT)));

// TODO: fix the default layout selection!?
theme_name = "bare";
layout_name = "bare";

//printf("Got theme / layout name = [%s] / [%s] (path=%s)\n", theme_name.toUtf8().data(), layout_name.toUtf8().data(), cpath.toUtf8().data());

    // Initialize the XML document
    QDomDocument doc("snap");
    QDomElement root = doc.createElement("snap");
    doc.appendChild(root);
    QDomElement header(doc.createElement("head"));
    root.appendChild(header);
    QDomElement metadata(doc.createElement("metadata"));
    header.appendChild(metadata);
    QDomElement page(doc.createElement("page"));
    root.appendChild(page);
    QDomElement body(doc.createElement("body"));
    page.appendChild(body);
    QVector<QDomElement> boxes;

    generate_header_content(this, cpath, header, metadata);
    content_plugin->on_generate_main_content(this, cpath, page, body);
    generate_page_content(this, cpath, page, body);
    if(plugins::exists("filter"))
    {
        // replace all tokens if filtering is available
        filter::filter::instance()->on_token_filter(doc);
    }
    
    //box = QDomElement();
    //f_boxes.push_back(box);
    // TODO: get the box owner then call the on_generate_box_content() of
    //       that plugin; this is the best way to handle boxes
    //plugin->on_generate_box_content(this, path, box);

//printf("Generated XML is [%s]\n", doc.toString().toUtf8().data());

    QSharedPointer<QtCassandra::QCassandraTable> layout_table(get_layout_table());

    // now we want to transform the XML to HTML or some other format
    {
        QXmlQuery q(QXmlQuery::XSLT20);
#if 0
        QDomNodeModel m(q.namePool(), doc);
        QXmlNodeModelIndex x(m.fromDomNode(doc.documentElement()));
        QXmlItem i(x);
        q.setFocus(i);
#else
        q.setFocus(doc.toString());
#endif
        QString xsl;
        if(layout_name != "default")
        {
            // try to load the layout from the database, if not found
            // we'll switch to the default layout instead
            QtCassandra::QCassandraValue layout_value(layout_table->row(layout_name)->cell(QString("body"))->value());
            if(layout_value.nullValue())
            {
                // note that a layout cannot be empty so the test is correct
                layout_name = "default";
            }
            else
            {
                xsl = layout_value.stringValue();
            }
        }
        if(layout_name == "default")
        {
            QFile file(":/xsl/layout/default-body-parser.xsl");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("layout::apply_layout() could not open default-body-parser.xsl resource file.");
                // TODO: I don't think we just want to return here?
                return "body parser not available";
            }
            QByteArray data(file.readAll());
            xsl = QString::fromUtf8(data.data(), data.size());
        }
        // Somehow binding crashes everything at this point?! (Qt 4.8.1)
        //QDate current_year;
        //QXmlItem year(QVariant(current_year.toString("yyyy")));
        //q.bindVariable("year", QVariant(current_year.toString("yyyy")));
        q.setQuery(xsl);
#if 0
        QXmlResultItems results;
        q.evaluateTo(&results);
        
        QXmlItem item(results.next());
        while(!item.isNull())
        {
            if(item.isNode())
            {
                //printf("Got a node!\n");
                QXmlNodeModelIndex node_index(item.toNodeModelIndex());
                QDomNode node(m.toDomNode(node_index));
                printf("Got a node! [%s]\n", node.localName()/*ownerDocument().toString()*/.toUtf8().data());
            }
            item = results.next();
        }
#elif 1
        // this should be faster since we keep the data in a DOM
        QDomDocument doc_output("body");
        QDomReceiver receiver(q.namePool(), doc_output);
        q.evaluateTo(&receiver);
        body.appendChild(doc.importNode(doc_output.documentElement(), true));
//printf("Body HTML is [%s]\n", doc_output.toString().toUtf8().data());
#else
        //QDomDocument doc_body("body");
        //doc_body.setContent(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_BODY)).stringValue(), true, NULL, NULL, NULL);
        //QDomElement content_tag(doc.createElement("content"));
        //body.appendChild(content_tag);
        //content_tag.appendChild(doc.importNode(doc_body.documentElement(), true));

        // TODO: look into getting XML as output
        QString out;
        q.evaluateTo(&out);
        //QDomElement output(doc.createElement("output"));
        //body.appendChild(output);
        //QDomText text(doc.createTextNode(out));
        //output.appendChild(text);
        QDomDocument doc_output("body");
        doc_output.setContent(out, true, NULL, NULL, NULL);
        body.appendChild(doc.importNode(doc_output.documentElement(), true));
#endif
    }

    // finally apply the theme XSLT to the final XML
    // the output is what we want to return
    {
        QXmlQuery q(QXmlQuery::XSLT20);
        q.setFocus(doc.toString());
        //QFile xsl(":/xsl/layout/default-theme-parser.xsl");
        //if(!xsl.open(QIODevice::ReadOnly))
        //{
        //    SNAP_LOG_FATAL("layout::apply_layout() could not open default-theme-parser.xsl resource file.");
        //    // TODO: I don't think we just want to return here?
        //    return "theme parser not available";
        //}
        QString xsl;
        if(theme_name != "default")
        {
            // try to load the layout from the database, if not found
            // we'll switch to the default layout instead
            QtCassandra::QCassandraValue theme_value(layout_table->row(theme_name)->cell(QString("theme"))->value());
            if(theme_value.nullValue())
            {
                // note that a layout cannot be empty so the test is correct
                theme_name = "default";
            }
            else
            {
                xsl = theme_value.stringValue();
            }
        }
        if(theme_name == "default")
        {
            QFile file(":/xsl/layout/default-theme-parser.xsl");
            if(!file.open(QIODevice::ReadOnly))
            {
                SNAP_LOG_FATAL("layout::apply_layout() could not open default-theme-parser.xsl resource file.");
                // TODO: I don't think we just want to return here?
                return "theme parser not available";
            }
            QByteArray data(file.readAll());
            xsl = QString::fromUtf8(data.data(), data.size());
        }
        q.setQuery(xsl);

        QBuffer output;
        output.open(QBuffer::ReadWrite);
        QHtmlSerializer html(q.namePool(), &output);
        q.evaluateTo(&html);

        QString out(QString::fromUtf8(output.data()));
//printf("Final HTML is [%s]\n", out.toUtf8().data());

        return out;
    }
    // NOT REACHED
}

/** \brief Generate the header of the content.
 *
 * This function generates the main content header information. Other
 * plugins will also receive the event and are invited to add their
 * own information to any header as required by their implementation.
 *
 * Remember that this is not exactly the HTML header, it's the XML
 * header that will be parsed through the theme XSLT file.
 *
 * This function is also often used to setup HTTP headers early on.
 * For example the robots.txt plugin sets up the X-Robots header with
 * a call to the snap_child object:
 *
 * \code
 * f_snap->set_header("X-Robots", f_robots_cache);
 * \endcode
 *
 * \param[in] l  The layout pointer.
 * \param[in] path  The path being managed.
 * \param[in,out] header  The header being generated.
 * \param[in,out] metadata  The metadata being generated.
 */
bool layout::generate_header_content_impl(layout *l, const QString& path, QDomElement& header, QDomElement& metadata)
{
    QDomDocument doc(header.ownerDocument());

    {   // snap/head/metadata/desc[type=website_uri]/data
        QDomElement desc(doc.createElement("desc"));
        desc.setAttribute("type", "website_uri");
        metadata.appendChild(desc);
        QDomElement data(doc.createElement("data"));
        desc.appendChild(data);
        QDomText text(doc.createTextNode(f_snap->get_site_key()));
        data.appendChild(text);
    }

    {   // snap/head/metadata/desc[type=base_uri]/data
        QDomElement desc(header.ownerDocument().createElement("desc"));
        desc.setAttribute("type", "base_uri");
        metadata.appendChild(desc);
        QDomElement data(header.ownerDocument().createElement("data"));
        desc.appendChild(data);
        int p(path.lastIndexOf('/'));
        QString base;
        if(p == -1) {
            base = "";
        }
        else {
            base = path.left(p);
        }
        QDomText text(doc.createTextNode(f_snap->get_site_key_with_slash() + base));
        data.appendChild(text);
    }

    {   // snap/head/metadata/desc[type=page_uri]/data
        QDomElement desc(header.ownerDocument().createElement("desc"));
        desc.setAttribute("type", "page_uri");
        metadata.appendChild(desc);
        QDomElement data(header.ownerDocument().createElement("data"));
        desc.appendChild(data);
        QDomText text(doc.createTextNode(f_snap->get_site_key_with_slash() + path));
        data.appendChild(text);
    }

    {   // snap/head/metadata/desc[type=name]/data
        QDomElement desc(header.ownerDocument().createElement("desc"));
        desc.setAttribute("type", "name");
        metadata.appendChild(desc);
        QDomElement data(header.ownerDocument().createElement("data"));
        desc.appendChild(data);
        // normal name always exists
        QDomText text(doc.createTextNode(f_snap->get_site_parameter(snap::get_name(snap::SNAP_NAME_CORE_SITE_NAME)).stringValue()));
        data.appendChild(text);
        // short name
        QtCassandra::QCassandraValue short_name(f_snap->get_site_parameter(snap::get_name(snap::SNAP_NAME_CORE_SITE_SHORT_NAME)));
        if(!short_name.nullValue())
        {
            QDomElement short_data(header.ownerDocument().createElement("short-data"));
            desc.appendChild(short_data);
            QDomText short_text(doc.createTextNode(short_name.stringValue()));
            short_data.appendChild(short_text);
        }
        // long name
        QtCassandra::QCassandraValue long_name(f_snap->get_site_parameter(snap::get_name(snap::SNAP_NAME_CORE_SITE_LONG_NAME)));
        if(!long_name.nullValue())
        {
            QDomElement long_data(header.ownerDocument().createElement("long-data"));
            desc.appendChild(long_data);
            QDomText long_text(doc.createTextNode(long_name.stringValue()));
            long_data.appendChild(long_text);
        }
    }

    {   // snap/head/metadata/desc[type=email]/data
        QtCassandra::QCassandraValue email(f_snap->get_site_parameter(snap::get_name(snap::SNAP_NAME_CORE_ADMINISTRATOR_EMAIL)));
        if(!email.nullValue())
        {
            QDomElement desc(header.ownerDocument().createElement("desc"));
            desc.setAttribute("type", "email");
            metadata.appendChild(desc);
            QDomElement data(header.ownerDocument().createElement("data"));
            desc.appendChild(data);
            QDomText text(doc.createTextNode(email.stringValue()));
            data.appendChild(text);
        }
    }

    {   // snap/head/metadata/desc[type=remote_ip]/data
        QDomElement desc(header.ownerDocument().createElement("desc"));
        desc.setAttribute("type", "remote_ip");
        metadata.appendChild(desc);
        QDomElement data(header.ownerDocument().createElement("data"));
        desc.appendChild(data);
        QDomText text(doc.createTextNode(f_snap->snapenv("REMOTE_ADDR")));
        data.appendChild(text);
    }

    return true;
}

/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * areas may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the <page> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the intermediate and final output.
 *
 * \param[in] l  The layout pointer.
 * \param[in] path  The path being managed.
 * \param[in] page_content  The main content of the page.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 *
 * \return true if the page content creation can proceed.
 */
bool layout::generate_page_content_impl(layout *l, const QString& path, QDomElement& page, QDomElement& body)
{
    return true;
}


// This was to test, at this point we don't offer anything in the layout itself
//int layout::js_property_count() const
//{
//    return 1;
//}
//
//QVariant layout::js_property_get(const QString& name) const
//{
//    if(name == "name")
//    {
//        return "default_layout";
//    }
//    return QVariant();
//}
//
//QString layout::js_property_name(int index) const
//{
//    return "name";
//}
//
//QVariant layout::js_property_get(int index) const
//{
//    if(index == 0)
//    {
//        return "default_layout";
//    }
//    return QVariant();
//}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
