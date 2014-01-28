// Snap Websites Server -- handle the theme/layout information
// Copyright (C) 2011-2014  Made to Order Software Corp.
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

#include "../filter/filter.h"
#include "../content/content.h"
#include "../javascript/javascript.h"
#include "../taxonomy/taxonomy.h"

#include "log.h"
#include "qdomreceiver.h"
#include "qhtmlserializer.h"
#include "qstring_stream.h"
//#include "qdomnodemodel.h" -- at this point the DOM Node Model seems bogus.
#include "not_reached.h"

#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QXmlQuery>
#include <QDomDocument>
#include <QFile>
#include <QXmlResultItems>
#pragma GCC diagnostic pop

#include "poison.h"


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
    case SNAP_NAME_LAYOUT_ADMIN_LAYOUTS:
        return "admin/layouts";

    case SNAP_NAME_LAYOUT_BOX:
        return "layout::box";

    case SNAP_NAME_LAYOUT_BOXES:
        return "layout::boxes";

    case SNAP_NAME_LAYOUT_CONTENT:
        return "content";

    case SNAP_NAME_LAYOUT_LAYOUT:
        return "layout::layout";

    case SNAP_NAME_LAYOUT_TABLE:
        return "layout";

    case SNAP_NAME_LAYOUT_THEME:
        return "layout::theme";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_LAYOUT_...");

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

    SNAP_LISTEN(layout, "server", server, load_file, _1, _2);
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
    //SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, content_update); -- content depends on JavaScript so we cannot do a content update here

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief First update to run for the layout plugin.
 *
 * This function is the first update for the layout plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void layout::initial_update(int64_t variables_timestamp)
{
}
#pragma GCC diagnostic pop


/** \brief Initialize the layout table.
 *
 * This function creates the layout table if it doesn't exist yet. Otherwise
 * it simple retrieves it from Cassandra.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \return The shared pointer to the layout table.
 */
QtCassandra::QCassandraTable::pointer_t layout::get_layout_table()
{
    return f_snap->create_table(get_name(SNAP_NAME_LAYOUT_TABLE), "Layouts table.");
}


/** \brief Retrieve the name of a theme or layout.
 *
 * This function checks for the name of a theme or layout in the current object
 * or the specified type and its parent.
 *
 * \param[in] ipath  The path to the content to process.
 * \param[in] column_name  The name of the column to search (layout::theme or layout::layout)
 */
QString layout::get_layout(content::path_info_t& ipath, const QString& column_name)
{
    // get the content table first
    QtCassandra::QCassandraValue layout_value(content::content::instance()->get_content_table()->row(ipath.get_key())->cell(column_name)->value());
    if(layout_value.nullValue())
    {
        // that very content doesn't define a layout, check its type(s)
        layout_value = taxonomy::taxonomy::instance()->find_type_with(
            ipath,
            content::get_name(content::SNAP_NAME_CONTENT_PAGE_TYPE),
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
        // TODO: remove dependency on JS with an event on this one!
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
 * This function defines a page content using the data as defined by \p cpath
 * and \p ctemplate. \p ctemplate data is used only if data that is generally
 * required is not currently available in \p cpath.
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
 * You may use the create_body() function directly to gather all the data
 * to be used to create a page. The apply_theme() will then layout the
 * result in a page.
 *
 * \param[in] ipath  The canonalized path of content to be laid out.
 * \param[in] content_plugin  The plugin that will generate the content of the page.
 * \param[in] ctemplate  The path to the template is used to get default data.
 *
 * \return The result is the output of the layout applied to the data in cpath.
 */
QString layout::apply_layout(content::path_info_t& ipath, layout_content *content_plugin, const QString& ctemplate)
{
    QDomDocument doc(create_body(ipath, content_plugin, ctemplate));
    return apply_theme(doc, ipath, content_plugin);
}


/** \brief Create the body XML data.
 *
 * This function creates the entire XML data that will be used by the
 * theme XSLT parser. It first creates an XML document using the
 * different generate functions to create the header and page data,
 * then runs the body XSLT parser to format the specified content
 * in a valid HTML buffer (valid as in, valid HTML tags, as a whole
 * this is not a valid HTML document, only a block of content; in
 * particular, the result does not include the \<head> tag.)
 *
 * This function is often used to generate parts of the content such
 * as boxes on the side of the screen. It can also be used to create
 * content of a page from a template (i.e. the user profile is
 * created from the admin/users/pages/profile template.) In many
 * cases, when the function is used in this way, only the title and
 * body are used. If a block is to generate something that should
 * appear in the header, then it should create it in the header of
 * the main page.
 *
 * The system can now make use of a ctemplate to gather data which are
 * not otherwise defined in the cpath cell. By default ctemplate is set
 * to the empty string which means it does not get used.
 *
 * \param[in,out] ipath  The path being dealt with.
 * \param[in] content_plugin  The plugin handling the content (body/title in general.)
 * \param[in] ctemplate  The path to the template is used to get default data.
 *
 * \return The resulting body in an XML document.
 */
QDomDocument layout::create_body(content::path_info_t& ipath, layout_content *content_plugin, const QString& ctemplate)
{
#ifdef DEBUG
std::cerr << "create body in layout\n";
#endif
    class error_callback : public permission_error_callback
    {
    public:
        error_callback(snap_child *snap)
            : f_snap(snap)
            //, f_error(false) -- auto-init
        {
        }

        virtual void on_error(snap_child::http_code_t err_code, QString const& err_name, QString const& err_description, QString const& err_details)
        {
            // log the error so users know something happened
            SNAP_LOG_ERROR("error #")(static_cast<int>(err_code))(":")(err_name)(": ")(err_description)(" -- ")(err_details);
            f_error = true;
        }

        virtual void on_redirect(
                /* message::set_error() */ QString const& err_name, QString const& err_description, QString const& err_details, bool err_security,
                /* snap_child::page_redirect() */ QString const& path, snap_child::http_code_t http_code)
        {
            (void)err_security;
            SNAP_LOG_ERROR("error #")(static_cast<int>(http_code))(":")(err_name)(": ")(err_description)(" -- ")(err_details)(" (path: ")(path);
            f_error = true;
        }

        void clear_error()
        {
            f_error = false;
        }

        bool has_error() const
        {
            return f_error;
        }

    private:
        zpsnap_child_t              f_snap;
        controlled_vars::fbool_t    f_error;
    } box_error_callback(f_snap);

    // Retrieve the theme and layout for this path
    // XXX should the ctemplate ever be used to retrieve the layout?
    QString layout_name(get_layout(ipath, get_name(SNAP_NAME_LAYOUT_LAYOUT)));

//printf("Got theme / layout name = [%s] / [%s] (path=%s)\n", theme_name.toUtf8().data(), layout_name.toUtf8().data(), cpath.toUtf8().data());

// TODO: fix the default layout selection!?
//       until we can get the theme system working right...
//       actually the theme system works, but we need to have something
//       to allow us to select said theme
layout_name = "bare";

    bool const filter_exists(plugins::exists("filter"));
    QtCassandra::QCassandraTable::pointer_t layout_table(get_layout_table());

    plugin *p(dynamic_cast<plugin *>(content_plugin));

    // now we want to transform the XML to HTML or some other format
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
            f_snap->die(snap_child::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Layout Unavailable",
                    "Somehow no website layout was accessible, not even the internal default.",
                    "layout::create_body() could not open default-body-parser.xsl resource file.");
            NOTREACHED();
        }
        QByteArray data(file.readAll());
        xsl = QString::fromUtf8(data.data(), data.size());
    }
    // TODO: once we got the XSL file we need to handle all the xsl:include
    //       and xsl:import as QXmlQuery does not support those XSLT features
    //       which are important for us because we want to allow for
    //       "internal" features (i.e. avoid duplicating all the code used
    //       to build the <head> tag, for example.)
    //
    // http://www.w3.org/TR/xslt#section-Combining-Stylesheets

    // check whether the layout was defined in this website database
    QtCassandra::QCassandraTable::pointer_t data_table(content::content::instance()->get_data_table());
    content::path_info_t layout_ipath;
    layout_ipath.set_path(QString("%1/%2").arg(get_name(SNAP_NAME_LAYOUT_ADMIN_LAYOUTS)).arg(layout_name));
    // TODO: we'll need to manage updates which is probably going to be done
    //       from the snap_child::update_plugins() with the user of a message
    //       which should be caught by this plugin...
    if(!data_table->exists(layout_ipath.get_branch_key())
    || !data_table->row(layout_ipath.get_branch_key())->exists(get_name(SNAP_NAME_LAYOUT_BOXES)))
    {
        // this layout is missing, create necessary basic info
        // (later users can edit those settings)
        if(!layout_table->row(layout_name)->exists(get_name(SNAP_NAME_LAYOUT_CONTENT)))
        {
            f_snap->die(snap_child::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Layout Unavailable",
                    "Layout \"" + layout_name + "\" content.xml file is missing.",
                    "layout::create_body() could not find the content.xml file in the layout table.");
            NOTREACHED();
        }
        QString const xml_content(layout_table->row(layout_name)->cell(get_name(SNAP_NAME_LAYOUT_CONTENT))->value().stringValue());
        QDomDocument dom;
        if(!dom.setContent(xml_content, false))
        {
            f_snap->die(snap_child::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Layout Unavailable",
                    "Layout \"" + layout_name + "\" content.xml file could not be loaded.",
                    "layout::create_body() could not load the content.xml file from the layout table.");
            NOTREACHED();
        }
        content::content::instance()->add_xml_document(dom, p == NULL ? content::get_name(content::SNAP_NAME_CONTENT_OUTPUT) : p->get_plugin_name());
        f_snap->finish_update();
        if(!data_table->row(layout_ipath.get_branch_key())->exists(get_name(SNAP_NAME_LAYOUT_BOXES)))
        {
            f_snap->die(snap_child::HTTP_CODE_INTERNAL_SERVER_ERROR,
                    "Layout Unavailable",
                    "Layout \"" + layout_name + "\" content.xml file does not define the layout::boxes entry for this layout.",
                    "layout::create_body() the content.xml did not define \"" + layout_ipath.get_branch_key() + "->[layout::boxes]\" as expected.");
            NOTREACHED();
        }
    }

    // Initialize the XML document tree
    // More is done in the generate_header_content_impl() function
    QDomDocument doc("snap");
    QDomElement root = doc.createElement("snap");
    root.setAttribute("path", ipath.get_cpath());
    if(p != NULL)
    {
        root.setAttribute("owner", p->get_plugin_name());
    }
    doc.appendChild(root);
    QDomElement head(doc.createElement("head"));
    root.appendChild(head);
    QDomElement metadata(doc.createElement("metadata"));
    head.appendChild(metadata);
    QDomElement page(doc.createElement("page"));
    root.appendChild(page);
    QDomElement body(doc.createElement("body"));
    page.appendChild(body);

#ifdef DEBUG
std::cerr << "got in layout... cpath = [" << ipath.get_cpath() << "]\n";
#endif
    // other plugins generate defaults
    generate_header_content(this, ipath, head, metadata, ctemplate);

    // concerned (owner) plugin generates content
    content_plugin->on_generate_main_content(this, ipath, page, body, ctemplate);
//std::cout << "Header + Main XML is [" << doc.toString() << "]\n";

    // add boxes content
    // if the "boxes" entry doesn't exist yet then we can create it now
    // (i.e. we're creating a parent if the "boxes" element is not present;
    //       although we should not get called recursively, this makes things
    //       safer!)
    if(page.firstChildElement("boxes").isNull())
    {
        // the list of boxes is defined in the database under
        //    admin/layouts/<layout_name>
        // as one row name per box; for example, the left box would appears as:
        //    admin/layouts/<layout_name>/layout::box::left/layout::boxes
        QDomElement boxes = doc.createElement("boxes");
        page.appendChild(boxes);
        // TODO -- 
        content::field_search::search_result_t box_names;
        FIELD_SEARCH
            (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
            (content::field_search::COMMAND_PATH, get_name(SNAP_NAME_LAYOUT_ADMIN_LAYOUTS) + ("/" + layout_name))
            (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_LAYOUT_BOXES))
            (content::field_search::COMMAND_SELF)
            (content::field_search::COMMAND_RESULT, box_names)

            // retrieve names of all the boxes
            ;
        int const max_names(box_names.size());
        if(max_names != 0)
        {
            if(max_names != 1)
            {
                throw snap_logic_exception("expected zero or one entry from a COMMAND_SELF");
            }
            QStringList names(box_names[0].stringValue().split(","));
            QVector<QDomElement> dom_boxes;
            int const max_boxes(names.size());
            for(int i(0); i < max_boxes; ++i)
            {
                names[i] = names[i].trimmed();
                QDomElement box(doc.createElement(names[i]));
                boxes.appendChild(box);
                dom_boxes.push_back(box); // will be the same offset as names[...]
            }
#ifdef DEBUG
            if(dom_boxes.size() != max_boxes)
            {
                throw snap_logic_exception("somehow the 'DOM boxes' and 'names' vectors do not have the same size.");
            }
#endif
            for(int i(0); i < max_boxes; ++i)
            {
                content::path_info_t ichild;
                ichild.set_path(QString("%1/%2/%3").arg(get_name(SNAP_NAME_LAYOUT_ADMIN_LAYOUTS)).arg(layout_name).arg(names[i]));
                links::link_info info(content::get_name(content::SNAP_NAME_CONTENT_CHILDREN), false, ichild.get_key(), ichild.get_branch());
                QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                links::link_info child_info;
                while(link_ctxt->next_link(child_info))
                {
                    box_error_callback.clear_error();
                    content::path_info_t box_ipath;
                    box_ipath.set_path(child_info.key());
                    plugin *box_plugin(content::content::instance()->get_plugin(box_ipath, box_error_callback));
                    if(!box_error_callback.has_error() && box_plugin)
                    {
                        layout_boxes *lb(dynamic_cast<layout_boxes *>(box_plugin));
                        if(lb != NULL)
                        {
                            // put each box in a filter tag because we have to
                            // specify a different owner and path for each
                            QDomElement filter_box(doc.createElement("filter"));
                            filter_box.setAttribute("path", box_ipath.get_cpath()); // not the full key
                            filter_box.setAttribute("owner", box_plugin->get_plugin_name());
                            dom_boxes[i].appendChild(filter_box);
                            lb->on_generate_boxes_content(this, ipath, box_ipath, page, filter_box, ctemplate);
                        }
                        else
                        {
                            // if this happens a plugin offers a box but not
                            // the handler
                            f_snap->die(snap_child::HTTP_CODE_INTERNAL_SERVER_ERROR,
                                    "Plugin Missing",
                                    "Plugin \"" + box_plugin->get_plugin_name() + "\" does not know how to handle a box assigned to it.",
                                    "layout::create_body() the plugin does not derive from layout::layout_boxes.");
                            NOTREACHED();
                        }
                    }
                }
            }
        }
    }

    // other plugins are allowed to modify the content if so they wish
    generate_page_content(this, ipath, page, body, ctemplate);
//std::cout << "Prepared XML is [" << doc.toString() << "]\n";

    // TODO: the filtering needs to be a lot more generic!
    //       plus the owner of the page should be able to select the
    //       filters he wants to apply agains the page content
    //       (i.e. ultimately we want to have some sort of filter
    //       tagging capability)
    if(filter_exists)
    {
        // replace all tokens if filtering is available
        filter::filter::instance()->on_token_filter(ipath, doc);
    }

//std::cout << "Generated XML is [" << doc.toString() << "]\n";

    // Somehow binding crashes everything at this point?! (Qt 4.8.1)
    QXmlQuery q(QXmlQuery::XSLT20);
#if 0
    QDomNodeModel m(q.namePool(), doc);
    QXmlNodeModelIndex x(m.fromDomNode(doc.documentElement()));
    QXmlItem i(x);
    q.setFocus(i);
#else
    q.setFocus(doc.toString());
#endif
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
    //doc_body.setContent(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_BODY) <<-- that would be wrong now).stringValue(), true, NULL, NULL, NULL);
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

    return doc;
}


/** \brief Apply the theme on an XML document.
 *
 * This function applies the theme to an XML document representing a
 * page. This should only be used against blocks that are themed
 * and final pages.
 *
 * Whenever you create a body from a template, then you should not call
 * this function since it would otherwise pre-theme your result. Instead
 * you'd want to save the title and body elements of the \p doc XML
 * document.
 *
 * \param[in] doc  The XML document to theme.
 * \param[in] cpath  The path of the document being themed.
 * \param[in] content_plugin  The
 *
 * \return The XML document themed in the form of a string.
 */
QString layout::apply_theme(QDomDocument doc, content::path_info_t& ipath, layout_content *content_plugin)
{
    (void)content_plugin; // not yet used

    QString theme_name(get_layout(ipath, get_name(SNAP_NAME_LAYOUT_THEME)));

// TODO: until we can get the theme system working right...
//       actually the theme system works, but we need to have something
//       to allow us to select said theme
theme_name = "bare";

    //QFile xsl(":/xsl/layout/default-theme-parser.xsl");
    //if(!xsl.open(QIODevice::ReadOnly))
    //{
    //    SNAP_LOG_FATAL("layout::apply_theme() could not open default-theme-parser.xsl resource file.");
    //    // TODO: I don't think we just want to return here?
    //    return "theme parser not available";
    //}
    QString xsl;
    if(theme_name != "default")
    {
        // try to load the layout from the database, if not found
        // we'll switch to the default layout instead
        QtCassandra::QCassandraTable::pointer_t layout_table(get_layout_table());
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
            f_snap->die(snap_child::HTTP_CODE_INTERNAL_SERVER_ERROR,
                "Layout Unavailable",
                "Somehow no website layout was accessible, not even the internal default.",
                "layout::apply_theme() could not open default-theme-parser.xsl resource file.");
            NOTREACHED();
        }
        QByteArray data(file.readAll());
        xsl = QString::fromUtf8(data.data(), data.size());
    }
    // TODO: once we got the XSL file we need to handle all the xsl:include
    //       and xsl:import as QXmlQuery does not support those XSLT features
    //       which are important for us because we want to allow for
    //       "internal" features (i.e. avoid duplicating all the code used
    //       to build the <head> tag, for example.)
    //
    // http://www.w3.org/TR/xslt#section-Combining-Stylesheets

    // finally apply the theme XSLT to the final XML
    // the output is what we want to return
    QXmlQuery q(QXmlQuery::XSLT20);
    q.setFocus(doc.toString());
    q.setQuery(xsl);

    QBuffer output;
    output.open(QBuffer::ReadWrite);
    QHtmlSerializer html(q.namePool(), &output);
    q.evaluateTo(&html);

    QString out(QString::fromUtf8(output.data()));

    // HTML5 DOCTYPE is just "html" as follow!
    return "<!DOCTYPE html>" + out;
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
 * \param[in] ctemplate  The template used to generate the page or "".
 *
 * \return true if the signal should go on to all the other plugins.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool layout::generate_header_content_impl(layout *l, content::path_info_t& ipath, QDomElement& header, QDomElement& metadata, QString const& ctemplate)
{
    int const p(ipath.get_cpath().lastIndexOf('/'));
    QString const base(f_snap->get_site_key_with_slash() + (p == -1 ? "" : ipath.get_cpath().left(p)));

    FIELD_SEARCH
        (content::field_search::COMMAND_ELEMENT, metadata)
        (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)

        // snap/head/metadata/desc[@type="website_uri"]/data
        (content::field_search::COMMAND_DEFAULT_VALUE, f_snap->get_site_key())
        (content::field_search::COMMAND_SAVE, "desc[type=website_uri]/data")

        // snap/head/metadata/desc[@type="base_uri"]/data
        (content::field_search::COMMAND_DEFAULT_VALUE, base)
        (content::field_search::COMMAND_SAVE, "desc[type=base_uri]/data")

        // snap/head/metadata/desc[type=page_uri]/data
        (content::field_search::COMMAND_DEFAULT_VALUE, f_snap->get_site_key_with_slash() + ipath.get_cpath())
        (content::field_search::COMMAND_SAVE, "desc[type=page_uri]/data")

        // snap/head/metadata/desc[type=template_uri]/data
        (content::field_search::COMMAND_DEFAULT_VALUE_OR_NULL, ctemplate.isEmpty() ? "" : f_snap->get_site_key_with_slash() + ctemplate)
        (content::field_search::COMMAND_SAVE, "desc[type=template_uri]/data")

        // snap/head/metadata/desc[type=name]/data
        (content::field_search::COMMAND_CHILD_ELEMENT, "desc")
        (content::field_search::COMMAND_ELEMENT_ATTR, "type=name")
        (content::field_search::COMMAND_DEFAULT_VALUE, f_snap->get_site_parameter(snap::get_name(SNAP_NAME_CORE_SITE_NAME)))
        (content::field_search::COMMAND_SAVE, "data")
        // snap/head/metadata/desc[type=name]/short-data
        (content::field_search::COMMAND_DEFAULT_VALUE_OR_NULL, f_snap->get_site_parameter(snap::get_name(SNAP_NAME_CORE_SITE_SHORT_NAME)))
        (content::field_search::COMMAND_SAVE, "short-data")
        // snap/head/metadata/desc[type=name]/long-data
        (content::field_search::COMMAND_DEFAULT_VALUE_OR_NULL, f_snap->get_site_parameter(snap::get_name(SNAP_NAME_CORE_SITE_LONG_NAME)))
        (content::field_search::COMMAND_SAVE, "long-data")
        (content::field_search::COMMAND_PARENT_ELEMENT)

        // snap/head/metadata/desc[type=email]/data
        (content::field_search::COMMAND_DEFAULT_VALUE_OR_NULL, f_snap->get_site_parameter(snap::get_name(SNAP_NAME_CORE_ADMINISTRATOR_EMAIL)))
        (content::field_search::COMMAND_SAVE, "desc[type=email]/data")

        // snap/head/metadata/desc[type=remote_ip]/data
        (content::field_search::COMMAND_DEFAULT_VALUE, f_snap->snapenv("REMOTE_ADDR"))
        (content::field_search::COMMAND_SAVE, "desc[type=remote_ip]/data")

        // generate!
        ;

//printf("layout stuff [%s]\n", header.ownerDocument().toString().toUtf8().data());
    return true;
}
#pragma GCC diagnostic pop

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
 * \param[in] ctemplate  The template used in case some parameters do not
 *                       exist in the specified path
 *
 * \return true if the page content creation can proceed.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool layout::generate_page_content_impl(layout *l, content::path_info_t& path, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    return true;
}
#pragma GCC diagnostic pop


/** \brief Load a file.
 *
 * This function is used to load a file. As additional plugins are added
 * additional protocols can be supported.
 *
 * The file information defaults are kept as is as much as possible. If
 * a plugin returns a file, though, it is advised that any information
 * available to the plugin be set in the file object.
 *
 * The base load_file() function (i.e. this very function) supports the
 * file system protocol (file:) and the Qt resources protocol (qrc:).
 * Including the "file:" protocol is not required. Also, the Qt resources
 * can be indicated simply by adding a colon at the beginning of the
 * filename (":/such/as/this/name").
 *
 * \param[in,out] file  The file name and content.
 * \param[in,out] found  Whether the file was found.
 *
 * \return true if the signal is to be propagated to all the plugins.
 */
void layout::on_load_file(snap_child::post_file_t& file, bool& found)
{
    QString filename(file.get_filename());
    if(filename.startsWith("layout:"))     // Read a layout file
    {
        // remove the protocol
        int i(7);
        for(; i < filename.length() && filename[i] == '/'; ++i);
        filename = filename.mid(i);
        QStringList parts(filename.split('/'));
        if(parts.size() != 2)
        {
            // wrong number of parts...
            SNAP_LOG_ERROR("layout load_file() called with an invalid path: \"")(filename)("\"");
            return;
        }
        if(parts[1].endsWith(".css"))
        {
            parts[1] = parts[1].left(parts[1].length() - 4);
        }
        QtCassandra::QCassandraTable::pointer_t layout_table(get_layout_table());
        if(layout_table->exists(parts[0])
        && layout_table->row(parts[0])->exists(QString(parts[1])))
        {
            QtCassandra::QCassandraValue layout_value(layout_table->row(parts[0])->cell(QString(parts[1]))->value());

            file.set_filename(filename);
            file.set_data(layout_value.binaryValue());
            found = true;
            // return false since we already "found" the file
        }
    }
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


/* sample XML file for a default Snap! website home page --
<!DOCTYPE snap>
<snap>
 <head path="" owner="content">
  <metadata>
   <desc type="website_uri">
    <data>http://csnap.m2osw.com/</data>
   </desc>
   <desc type="base_uri">
    <data>http://csnap.m2osw.com/</data>
   </desc>
   <desc type="page_uri">
    <data>http://csnap.m2osw.com/</data>
   </desc>
   <desc type="name">
    <data>Website Name</data>
   </desc>
   <desc type="remote_ip">
    <data>162.226.130.121</data>
   </desc>
   <desc type="shorturl">
    <data>http://csnap.m2osw.com/s/4</data>
   </desc>
  </metadata>
 </head>
 <page>
  <body>
   <titles>
    <title>Home Page</title>
   </titles>
   <content>
    <p>Welcome to your new Snap! C++ website.</p>
    <p>
     <a href="/login">Log In Now!</a>
    </p>
   </content>
   <created>2014-01-09</created>
   <modified>2014-01-09</modified>
   <updated>2014-01-09</updated>
   <image>
    <shortcut width="16" height="16" type="image/x-icon" href="http://csnap.m2osw.com/favicon.ico"/>
   </image>
   <bookmarks>
    <link title="Search" rel="search" type="text/html" href="http://csnap.m2osw.com/search"/>
   </bookmarks>
  </body>
  <boxes>
   <left>
    <filter path="admin/layouts/bare/left/login" owner="users">
     <titles>
      <title>User Login</title>
     </titles>
     <content>
      <p>The login box is showing!</p>
      <div class="form-wrapper">
       <div class="snap-form">
        <form onkeypress="javascript:if((event.which&amp;&amp;event.which==13)||(event.keyCode&amp;&amp;event.keyCode==13))fire_event(login_34,'click');" method="post" accept-charset="utf-8" id="form_34" autocomplete="off">
         <input type="hidden" value=" " id="form__iehack" name="form__iehack"/>
         <input type="hidden" value="3673b0558e8ad92c" id="form_session" name="form_session"/>
         <div class="form-item fieldset">
          <fieldset class="" id="log_info_34">
           <legend title="Enter your log in information below then click the Log In button." accesskey="l">Log In Form</legend>
           <div class="field-set-content">
            <div class="form-help fieldset-help" style="display: none;">This form allows you to log in your Snap! website. Enter your log in name and password and then click on Log In to get a log in session.</div>
            <div class="form-item line-edit ">
             <label title="Enter your email address to log in your Snap! Website account." class="line-edit-label" for="email_34">
              <span class="line-edit-label-span">Email:</span>
              <span class="form-item-required">*</span>
              <input title="Enter your email address to log in your Snap! Website account." class=" line-edit-input " alt="Enter the email address you used to register with Snap! All the Snap! Websites run by Made to Order Software Corp. allow you to use the same log in credentials." size="20" maxlength="60" accesskey="e" type="text" id="email_34" name="email" tabindex="1"/>
             </label>
             <div class="form-help line-edit-help" style="display: none;">Enter the email address you used to register with Snap! All the Snap! Websites run by Made to Order Software Corp. allow you to use the same log in credentials.</div>
            </div>
            <div class="form-item password ">
             <label title="Enter your password, if you forgot your password, just the link below to request a change." class="password-label" for="password_34">
              <span class="password-label-span">Password:</span>
              <span class="form-item-required">*</span>
              <input title="Enter your password, if you forgot your password, just the link below to request a change." class="password-input " alt="Enter the password you used while registering with Snap! Your password is the same for all the Snap! Websites run by Made to Order Software Corp." size="25" maxlength="256" accesskey="p" type="password" id="password_34" name="password" tabindex="2"/>
             </label>
             <div class="form-help password-help" style="display: none;">Enter the password you used while registering with Snap! Your password is the same for all the Snap! Websites run by Made to Order Software Corp.</div>
            </div>
            <div class="form-item link">
             <a title="Forgot your password? Click on this link to request Snap! to send you a link to change it with a new one." class="link " accesskey="f" href="/forgot-password" id="forgot_password_34" tabindex="6">Forgot Password</a>
             <div class="form-help link-help" style="display: none;">You use so many websites with an account... and each one has to have a different password! So it can be easy to forget the password for a given website. We store passwords in a one way encryption mechanism (i.e. we cannot decrypt it) so if you forget it, we can only offer you to replace it. This is done using the form this link sends you to.</div>
            </div>
            <div class="form-item link">
             <a title="No account yet? Register your own Snap! account now." class="link " accesskey="u" href="/register" id="register_34" tabindex="5">Register</a>
             <div class="form-help link-help" style="display: none;">To log in a Snap! account, you first have to register an account. Click on this link if you don't already have an account. If you are not sure, you can always try the <strong>Forgot Password</strong> link. It will tell you whether we know your email address.</div>
            </div>
            <div class="form-item checkbox">
             <label title="Select this checkbox to let your browser record a long time cookie. This way you can come back to your Snap! account(s) without having to log back in everytime." class="checkbox-label" for="remember_34">
              <input title="Select this checkbox to let your browser record a long time cookie. This way you can come back to your Snap! account(s) without having to log back in everytime." class="checkbox-input remember-me-checkbox" alt="By checking this box you agree to have Snap! save a full session cookie which let you come back to your website over and over again. By not selecting the checkbox, you still get a cookie, but it will only last 2 hours unless your use your website constantly." accesskey="m" type="checkbox" checked="checked" id="remember_34" name="remember" tabindex="3"/>
              <script type="text/javascript">remember_34.checked="checked";</script>
              <span class="checkbox-label-span">Remember Me</span>
             </label>
             <div class="form-help checkbox-help" style="display: none;">By checking this box you agree to have Snap! save a full session cookie which let you come back to your website over and over again. By not selecting the checkbox, you still get a cookie, but it will only last 2 hours unless your use your website constantly.</div>
            </div>
            <div class="form-item submit">
             <input title="Well... we may want to rename this one if we use it as the alternate text of widgets..." class="submit-input my-button-class" alt="Long description that goes in the help box, for example." size="25" accesskey="s" type="submit" value="Log In" disabled="disabled" id="login_34" name="login" tabindex="4"/>
             <div class="form-help submit-help" style="display: none;">Long description that goes in the help box, for example.</div>
            </div>
            <div class="form-item link">
             <a title="Click here to return to the home page" class="link my-cancel-class" accesskey="c" href="/" id="cancel_34" tabindex="7">Cancel</a>
             <div class="form-help link-help" style="display: none;">Long description that goes in the help box explaining why you'd want to click Cancel.</div>
            </div>
           </div>
          </fieldset>
         </div>
         <script type="text/javascript">email_34.focus();email_34.select();</script>
         <script type="text/javascript">function auto_reset_34(){form_34.reset();}window.setInterval(auto_reset_34,1.8E6);</script>
         <script type="text/javascript">
              function fire_event(element, event_type)
              {
                if(element.fireEvent)
                {
                  element.fireEvent('on' + event_type);
                }
                else
                {
                  var event = document.createEvent('Events');
                  event.initEvent(event_type, true, false);
                  element.dispatchEvent(event);
                }
              }
            </script>
        </form>
        <script type="text/javascript">login_34.disabled="";</script>
       </div>
      </div>
     </content>
    </filter>
   </left>
  </boxes>
 </page>
</snap>
*/

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
