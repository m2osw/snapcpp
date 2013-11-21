// Snap Websites Server -- all the user content and much of the system content
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

#include "content.h"
#include "plugins.h"
#include "../layout/layout.h"
#include "not_reached.h"
#include "dom_util.h"
#include <iostream>
#include <QFile>


SNAP_PLUGIN_START(content, 1, 0)

/** \brief Get a fixed content name.
 *
 * The content plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char *get_name(name_t name)
{
    switch(name) {
    case SNAP_NAME_CONTENT_TABLE: // pages, tags, comments, etc.
        return "content";

    case SNAP_NAME_CONTENT_CONTENT_TYPES:
        return "Content Types";

    case SNAP_NAME_CONTENT_CONTENT_TYPES_NAME:
        return "content_types";

    case SNAP_NAME_CONTENT_PAGE_CONTENT_TYPE:
        return "page_content_type";

    case SNAP_NAME_CONTENT_TITLE:
        return "content::title";

    case SNAP_NAME_CONTENT_SHORT_TITLE:
        return "content::short_title";

    case SNAP_NAME_CONTENT_LONG_TITLE:
        return "content::long_title";

    case SNAP_NAME_CONTENT_BODY:
        return "content::body";

    case SNAP_NAME_CONTENT_CREATED:
        return "content::created";

    case SNAP_NAME_CONTENT_UPDATED:
        return "content::updated";

    case SNAP_NAME_CONTENT_MODIFIED:
        return "content::modified";

    case SNAP_NAME_CONTENT_ACCEPTED:
        return "content::accepted";

    case SNAP_NAME_CONTENT_SUBMITTED:
        return "content::submitted";

    case SNAP_NAME_CONTENT_SINCE:
        return "content::since";

    case SNAP_NAME_CONTENT_UNTIL:
        return "content::until";

    case SNAP_NAME_CONTENT_COPYRIGHTED:
        return "content::copyrighted";

    case SNAP_NAME_CONTENT_ISSUED:
        return "content::issued";

    default:
        // invalid index
        throw snap_exception();

    }
    NOTREACHED();
}

/** \brief Initialize the content plugin.
 *
 * This function is used to initialize the content plugin object.
 */
content::content()
    //: f_snap(NULL) -- auto-init
{
}

/** \brief Clean up the content plugin.
 *
 * Ensure the content object is clean before it is gone.
 */
content::~content()
{
}

/** \brief Initialize the content.
 *
 * This function terminates the initialization of the content plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void content::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN0(content, "server", server, save_content);
    SNAP_LISTEN(content, "layout", layout::layout, generate_page_content, _1, _2, _3, _4);

    if(plugins::exists("javascript")) {
        javascript::javascript::instance()->register_dynamic_plugin(this);
    }
}

/** \brief Get a pointer to the content plugin.
 *
 * This function returns an instance pointer to the content plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the content plugin.
 */
content *content::instance()
{
    return g_plugin_content_factory.instance();
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
QString content::description() const
{
    return "Manage nearly all the content of your website. This plugin handles"
        " your pages, the website taxonomy (tags, categories, permissions...)"
        " and much much more.";
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
int64_t content::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2013, 9, 19, 22, 50, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief First update to run for the content plugin.
 *
 * This function is the first update for the content plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void content::initial_update(int64_t variables_timestamp)
{
    // add the index page! -- not like this anymore! see the content.xml files instead
    //path::path::instance()->add_path("content", "", variables_timestamp);
}

/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void content::content_update(int64_t variables_timestamp)
{
    content::content::instance()->add_xml("content");
}

/** \brief Execute a page: generate the complete output of that page.
 *
 * This function displays the page that the user is trying to view. It is
 * supposed that the page permissions were already checked and thus that
 * its contents can be displayed to the current user.
 *
 * Note that the path was canonicalized by the path plugin and thus it does
 * not require any further corrections.
 *
 * \param[in] cpath  The canonicalized path being managed.
 *
 * \return true if the content is properly generated, false otherwise.
 */
bool content::on_path_execute(const QString& cpath)
{
    f_snap->output(layout::layout::instance()->apply_layout(cpath, this));

    //QtCassandra::QCassandraValue title(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_TITLE)));
    //QtCassandra::QCassandraValue body(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_BODY)));
    //f_snap->output("<html><head><title>" + title.stringValue() + "</title></head><body>" + body.stringValue() + "</body></html>\n");
    return true;
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
 * \param[in] c  The content pointer.
 * \param[in] path  The path being managed.
 * \param[in] header  The header being generated.
 */
//bool content::on_generate_header_content_impl(content *c, const QString& path, QDomElement& header)
//{
//  return true;
//}


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the <page> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in] l  The layout pointer.
 * \param[in] path  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void content::on_generate_main_content(layout::layout *l, const QString& fullpath, QDomElement& page, QDomElement& body)
{
    QDomDocument doc(page.ownerDocument());
    //QDomNodeList bodies(doc.elementsByTagName("body"));
    //if(bodies.count() != 1)
    //{
    //  return;
    //}
    //QDomNode node(bodies[0]);
    //if(!node.isElement())
    //{
    //  return;
    //}
    //QDomElement body(node.toElement());

    // this is easier as some people (like me) will most certainly call this
    // function with a path that starts with a slash once in a while; this
    // way we avoid all sorts of trouble (should we generate a warning in the
    // logs though?)
    const char *s(fullpath.toUtf8().data());
    while(*s == '/')
    {
        ++s;
    }
    const QString path(QString::fromUtf8(s));

    {
        QDomElement created(doc.createElement("created"));
        body.appendChild(created);
        QtCassandra::QCassandraValue v(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_CREATED)));
        QDomText text(doc.createTextNode(f_snap->date_to_string(v.int64Value())));
        created.appendChild(text);
    }

    {
        QDomElement modified(doc.createElement("modified"));
        body.appendChild(modified);
        QtCassandra::QCassandraValue v(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_MODIFIED)));
        QDomText text(doc.createTextNode(f_snap->date_to_string(v.int64Value())));
        modified.appendChild(text);
    }

    {
        QDomElement updated(doc.createElement("updated"));
        body.appendChild(updated);
        QtCassandra::QCassandraValue v(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_UPDATED)));
        QDomText text(doc.createTextNode(f_snap->date_to_string(v.int64Value())));
        updated.appendChild(text);
    }

    {
        QtCassandra::QCassandraValue accepted_date(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_ACCEPTED)));
        if(!accepted_date.nullValue())
        {
            QDomElement accepted(doc.createElement("accepted"));
            body.appendChild(accepted);
            QDomText text(doc.createTextNode(f_snap->date_to_string(accepted_date.int64Value())));
            accepted.appendChild(text);
        }
    }

    {
        QtCassandra::QCassandraValue submitted_date(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_SUBMITTED)));
        if(!submitted_date.nullValue())
        {
            QDomElement submitted(doc.createElement("submitted"));
            body.appendChild(submitted);
            QDomText text(doc.createTextNode(f_snap->date_to_string(submitted_date.int64Value())));
            submitted.appendChild(text);
        }
    }

    {
        QtCassandra::QCassandraValue since_date(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_SINCE)));
        if(!since_date.nullValue())
        {
            QDomElement since(doc.createElement("since"));
            body.appendChild(since);
            QDomText text(doc.createTextNode(f_snap->date_to_string(since_date.int64Value(), true)));
            since.appendChild(text);
        }
    }

    {
        QtCassandra::QCassandraValue until_date(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_UNTIL)));
        if(!until_date.nullValue())
        {
            QDomElement until(doc.createElement("until"));
            body.appendChild(until);
            QDomText text(doc.createTextNode(f_snap->date_to_string(until_date.int64Value(), true)));
            until.appendChild(text);
        }
    }

    {
        QtCassandra::QCassandraValue copyrighted_date(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_COPYRIGHTED)));
        if(!copyrighted_date.nullValue())
        {
            QDomElement copyrighted(doc.createElement("copyrighted"));
            body.appendChild(copyrighted);
            QDomText text(doc.createTextNode(f_snap->date_to_string(copyrighted_date.int64Value())));
            copyrighted.appendChild(text);
        }
    }

    {
        QtCassandra::QCassandraValue issued_date(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_ISSUED)));
        if(!issued_date.nullValue())
        {
            QDomElement issued(doc.createElement("issued"));
            body.appendChild(issued);
            QDomText text(doc.createTextNode(f_snap->date_to_string(issued_date.int64Value())));
            issued.appendChild(text);
        }
    }

    {
        //QCassandraValue content_title();
        QDomElement titles(doc.createElement("titles"));
        body.appendChild(titles);
        QDomElement title(doc.createElement("title"));
        titles.appendChild(title);
        QDomText text(doc.createTextNode(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_TITLE)).stringValue()));
        title.appendChild(text);
        // short title
        QtCassandra::QCassandraValue short_title_text(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_SHORT_TITLE)));
        if(!short_title_text.nullValue())
        {
            QDomElement short_title(doc.createElement("short-title"));
            titles.appendChild(short_title);
            QDomText short_text(doc.createTextNode(short_title_text.stringValue()));
            short_title.appendChild(short_text);
        }
        // long title
        QtCassandra::QCassandraValue long_title_text(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_LONG_TITLE)));
        if(!long_title_text.nullValue())
        {
            QDomElement long_title(doc.createElement("long-title"));
            titles.appendChild(long_title);
            QDomText long_text(doc.createTextNode(long_title_text.stringValue()));
            long_title.appendChild(long_text);
        }
    }

#if 1
    {
        // we assume that the body content is valid because when we created it
        // we checked the data and if the user data was invalid XML then we
        // saved a place holder warning the user about the fact!
        QDomDocument doc_body("body");
        QString body_data(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_BODY)).stringValue());
        // TBD: it probably doesn't matter but we currently add an extra <div>
        //      tag; to remove it we'd have to append all the children found
        //      in that <div> tag.
        body_data = "<div class=\"snap-body\">" + body_data + "</div>";
        doc_body.setContent(body_data, true, NULL, NULL, NULL);
        //doc_body.setContent(page_content, true, NULL, NULL, NULL);
        QDomElement content_tag(doc.createElement("content"));
        body.appendChild(content_tag);
        content_tag.appendChild(doc.importNode(doc_body.documentElement(), true));
    }
#else
    // using a CDATA section generates pure text (no tags at all, the &lt;
    // characters become &amp;lt;
    {
        // we assume that the body content is valid because when we created it
        // we checked the data and if the user data was invalid XML then we
        // saved a place holder warning the user about the fact!
        QDomDocument doc_body("body");
        QString body_data(get_content_parameter(path, get_name(SNAP_NAME_CONTENT_BODY)).stringValue());
        // TBD: it probably doesn't matter but we currently add an extra <div>
        //      tag; to remove it we'd have to append all the children found
        //      in that <div> tag.
        body_data = "<div class=\"snap-body\">" + body_data + "</div>";
        // add it as a CDATA section to the XML
        QDomElement content_tag(doc.createElement("content"));

        QDomElement div_tag(doc.createElement("div"));

        QDomCDATASection cdata_section(doc.createCDATASection(body_data));
        div_tag.appendChild(cdata_section);

        content_tag.appendChild(div_tag);

        //content_tag.appendChild(doc.importNode(doc_body.documentElement(), true));

        body.appendChild(content_tag);
    }
#endif

    if(path != "")
    {
        // simple "up" navigation
        QDomElement navigation;
        dom_util::get_tag("navigation", body, navigation);

        {
            QDomElement link(doc.createElement("link"));
            link.setAttribute("rel", "top");
            link.setAttribute("title", "Index"); // TODO: translate
            link.setAttribute("href", f_snap->get_site_key());
            navigation.appendChild(link);
        }

        QString up(path);
        int p(up.lastIndexOf('/'));
        if(p == -1)
        {
            // in this case it is an equivalent to top
            up = f_snap->get_site_key();
        }
        else
        {
            up = f_snap->get_site_key_with_slash() + path.mid(0, p);
        }
        {
            QDomElement link(doc.createElement("link"));
            link.setAttribute("rel", "up");
            link.setAttribute("title", "Up"); // TODO: translate
            link.setAttribute("href", up);
            navigation.appendChild(link);
        }
    }
}



/** \brief Generate the page common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in] l  The layout pointer.
 * \param[in] path  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void content::on_generate_page_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body)
{
}


/** \brief Initialize the content table.
 *
 * This function creates the content table if it doesn't exist yet. Otherwise
 * it simple initializes the f_content_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \return The pointer to the content table.
 */
QSharedPointer<QtCassandra::QCassandraTable> content::get_content_table()
{
    return f_snap->create_table(get_name(SNAP_NAME_CONTENT_TABLE), "Website content table.");
}

/** \brief Retreive a content page parameter.
 *
 * This function reads a column from the content of the page using the
 * content key as defined by the canonalization process. The function
 * cannot be called before the content::on_path_execute() function is
 * called and the key properly initialized.
 *
 * The table is opened once and remains opened so calling this function
 * many times is not a problem. Also the libQtCassandra library caches
 * all the data. Reading the same field multiple times is not a concern
 * at all.
 *
 * If the value is undefined, the result is a null value.
 *
 * \param[in] path  The canonicalized path being managed.
 * \param[in] name  The name of the parameter to retrieve.
 *
 * \return The content of the row as a Cassandra value.
 */
QtCassandra::QCassandraValue content::get_content_parameter(const QString& path, const QString& param_name)
{
    QString key(f_snap->get_site_key_with_slash() + path);
//printf("get content for [%s] . [%s]\n", key.toUtf8().data(), param_name.toUtf8().data());
    if(!get_content_table()->exists(key))
    {
        // an empty value is considered to be a null value
        QtCassandra::QCassandraValue value;
        return value;
    }
    // I need to fix the libQtCassandra because this creates an empty cell!
    // (whereas the call below doesn't, so it's possible to do it correctly!)
    //if(!f_site_table->row(f_snap->get_site_key())->exists(param_name))
    //{
    //  // an empty value is considered to be a null value
    //  QtCassandra::QCassandraValue value;
    //  return value;
    //}

    return get_content_table()->row(key)->cell(param_name)->value();
}


/** \brief Prepare a set of content to add to the database.
 *
 * In most cases, plugins call this function in one of their do_update()
 * functions to add their content.xml file to the database.
 *
 * This function expects a plugin name as input to add the
 * corresponding content.xml file of that plugin. The data is search in
 * the resources (it is expected to be added there by the plugin).
 * The resource path is built as follow:
 *
 * \code
 * ":/plugins/" + plugin_name + "/content.xml"
 * \endcode
 *
 * The content is not immediately added to the database because
 * of dependency issues. At the time all the content is added
 * using this function, the order in which it is added is not
 * generally proper proper (i.e. the taxonomy "/type" may be
 * added after the content "/type/content-types".)
 *
 * The content plugin saves this data when it receives the
 * save_content signal.
 *
 * To dynamically add content (opposed to adding information
 * from an XML file) you want to call the add_param() and
 * add_link() functions as required.
 *
 * \param[in] plugin_name  The name of the plugin loading this data.
 *
 * \sa on_save_content()
 * \sa add_param()
 * \sa add_link()
 */
void content::add_xml(const QString& plugin_name)
{
    if(!plugins::verify_plugin_name(plugin_name))
    {
        // invalid plugin name
        throw std::runtime_error(("add_xml() called with an invalid plugin name: \"" + plugin_name + "\"").toUtf8().data());
    }
    QString filename(":/plugins/" + plugin_name + "/content.xml");
    QFile xml_content(filename);
    if(!xml_content.open(QFile::ReadOnly))
    {
        // file not found
        throw std::runtime_error(("add_xml() cannot open file: \"" + filename + "\"").toUtf8().data());
    }
    QDomDocument dom;
    if(!dom.setContent(&xml_content, false))
    {
        // invalid XML
        throw std::runtime_error(("add_xml() cannot read the XML of content file: \"" + filename + "\"").toUtf8().data());
    }
    QDomNodeList content_nodes(dom.elementsByTagName("content"));
    int max(content_nodes.size());
    for(int i(0); i < max; ++i)
    {
        QDomNode content_node(content_nodes.at(i));
        if(!content_node.isElement())
        {
            // we're only interested in elements
            continue;
        }
        QDomElement content_element(content_node.toElement());
        if(content_element.isNull())
        {
            // somehow this is not an element
            continue;
        }

        QString path(content_element.attribute("path"));
        if(path.isEmpty())
        {
            throw std::runtime_error("all <content> tags supplied to add_xml() must include a valid \"path\" attribute");
        }
        f_snap->canonicalize_path(path);
        QString key(f_snap->get_site_key_with_slash() + path);

        // create a new entry for the database
        add_content(key, plugin_name);

        QDomNodeList children(content_element.childNodes());
        int cmax(children.size());
        for(int c(0); c < cmax; ++c)
        {
            // grab <param> and <link> tags
            QDomNode child(children.at(c));
            if(!child.isElement())
            {
                // we're only interested by elements
                continue;
            }
            QDomElement element(child.toElement());
            if(element.isNull())
            {
                // somehow this is not really an element
                continue;
            }

            // <param name=... overwrite=... force-namespace=...> data </param>
            if(element.tagName() == "param")
            {
                QString param_name(element.attribute("name"));
                if(param_name.isEmpty())
                {
                    throw std::runtime_error("all <param> tags supplied to add_xml() must include a valid \"name\" attribute");
                }

                // 1) prepare the buffer
                // the parameter value can include HTML (should be in a [CDATA[...]] in that case)
                QString buffer;
                QTextStream data(&buffer);
                // we have to save all the element children because
                // saving the element itself would save the <param ...> tag
                // also if the whole is a <![CDATA[...]]> entry, remove it
                // (but keep sub-<![CDATA[...]]> if any.)
                QDomNodeList values(element.childNodes());
                int lmax(values.size());
                if(lmax == 1)
                {
                    QDomNode n(values.at(0));
                    if(n.isCDATASection())
                    {
                        QDomCDATASection raw_data(n.toCDATASection());
                        data << raw_data.data();
                    }
                    else
                    {
                        // not a CDATA section, save as is
                        n.save(data, 0);
                    }
                }
                else
                {
                    // save all the children
                    for(int l(0); l < lmax; ++l)
                    {
                        values.at(l).save(data, 0);
                    }
                }

                // 2) prepare the name
                QString fullname;
                // It seems to me that if the developer included any namespace
                // then it was meant to be defined that way
                //if(param_name.left(plugin_name.length() + 2) == plugin_name + "::")
                if(param_name.contains("::")) // already includes a namespace
                {
                    // plugin namespace already defined
                    fullname = param_name;
                }
                else
                {
                    // plugin namespace not defined
                    if(element.attribute("force-namespace") == "no")
                    {
                        // but developer said no namespace needed (?!)
                        fullname = param_name;
                    }
                    else
                    {
                        // this is the default!
                        fullname = plugin_name + "::" + param_name;
                    }
                }

                // add the resulting parameter
                add_param(key, fullname, buffer);

                // check whether we allow overwrites
                if(element.attribute("overwrite") == "yes")
                {
                    set_param_overwrite(key, fullname, true);
                }
            }
            // <link name=... to=...> destination path </link>
            else if(element.tagName() == "link")
            {
                QString link_name(element.attribute("name"));
                if(link_name.isEmpty())
                {
                    throw std::runtime_error("all <link> tags supplied to add_xml() must include a valid \"name\" attribute");
                }
                QString link_to(element.attribute("to"));
                if(link_to.isEmpty())
                {
                    throw std::runtime_error("all <link> tags supplied to add_xml() must include a valid \"to\" attribute");
                }
                bool source_unique(true);
                bool destination_unique(true);
                QString mode(element.attribute("mode"));
                if(!mode.isEmpty() && mode != "1:1")
                {
                    if(mode == "1:*")
                    {
                        destination_unique = false;
                    }
                    else if(mode == "*:1")
                    {
                        source_unique = false;
                    }
                    else if(mode == "*:*")
                    {
                        destination_unique = false;
                        source_unique = false;
                    }
                    else
                    {
                        throw std::runtime_error("<link> tags mode attribute must be one of \"1:1\", \"1:*\", \"*:1\", or \"*:*\"");
                    }
                }
                // the destination URL is defined in the <link> content
                QString destination_path(element.text());
                f_snap->canonicalize_path(destination_path);
                QString destination_key(f_snap->get_site_key_with_slash() + destination_path);
                links::link_info source(link_name, source_unique, key);
                links::link_info destination(link_to, destination_unique, destination_key);
                add_link(key, source, destination);
            }
        }
    }
}

/** \brief Prepare to add content to the database.
 *
 * This function creates a new block of data to be added to the database.
 * Each time one wants to add content to the database, one must call
 * this function first. At this time the plugin_owner cannot be changed.
 * If that happens (i.e. two plugins trying to create the same piece of
 * content) then the system raises an exception.
 *
 * \exception content_exception_content_already_defined
 * This exception is raised if the block already exists and the owner
 * of the existing block doesn't match the \p plugin_owner parameter.
 *
 * \param[in] path  The path of the content being added.
 * \param[in] plugin_owner  The name of the plugin managing this content.
 */
void content::add_content(const QString& path, const QString& plugin_owner)
{
    if(!plugins::verify_plugin_name(plugin_owner))
    {
        // invalid plugin name
        throw std::runtime_error(("install_content() called with an invalid plugin name: \"" + plugin_owner + "\"").toUtf8().data());
    }

    content_block_map_t::iterator b(f_blocks.find(path));
    if(b != f_blocks.end())
    {
        if(b->f_owner != plugin_owner)
        {
            // cannot change owner!?
            throw content_exception_content_already_defined();
        }
        // it already exists, we're all good
        return;
    }

    // create the new block
    content_block block;
    block.f_path = path;
    block.f_owner = plugin_owner;
    f_blocks.insert(path, block);

    f_snap->new_content();
}

/** \brief Add a parameter to the content to be saved in the database.
 *
 * This function is used to add a parameter to the database.
 * A parameter is composed of a name and a block of data that may be of any
 * type (HTML, XML, picture, etc.)
 *
 * Other parameters can be attached to parameters using set_param_...()
 * functions, however, the add_param() function must be called first to
 * create the parameter.
 *
 * Note that the data added in this way is NOT saved in the database until
 * the save_content signal is sent.
 *
 * \warning
 * This function does NOT save the data immediately (if called after the
 * update, then it is saved after the execute() call returns!) Instead
 * the function prepares the data so it can be saved later. This is useful
 * if you expect many changes and dependencies may not all be available at
 * the time you add the content but will be at a later time. If you already
 * have all the data, you may otherwise directly call the Cassandra function
 * to add the data to the content table.
 *
 * \bug
 * At this time the data of a parameter is silently overwritten if this
 * function is called multiple times with the same path and name.
 *
 * \exception content_exception_parameter_not_defined
 * This exception is raised when this funtion is called before the
 * add_content() is called (i.e. the block of data referenced by
 * \p path is not defined yet.)
 *
 * \param[in] path  The path of this parameter (i.e. /types/taxonomy)
 * \param[in] name  The name of this parameter (i.e. "Website Taxonomy")
 * \param[in] data  The data of this parameter.
 *
 * \sa add_param()
 * \sa add_link()
 * \sa on_save_content()
 */
void content::add_param(const QString& path, const QString& name, const QString& data)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined();
    }

    content_params_t::iterator p(b->f_params.find(name));
    if(p == b->f_params.end())
    {
        content_param param;
        param.f_name = name;
        param.f_data = data;
        b->f_params.insert(name, param);
    }
    else
    {
        // replace the data
        // TBD: should we generate an error because if defined by several
        //      different plugins then we cannot ensure which one is going
        //      to make it to the database! At the same time, we cannot
        //      know whether we're overwriting a default value.
        p->f_data = data;
    }
}

/** \brief Set the overwrite flag to a specific parameter.
 *
 * The parameter must first be added with the add_param() function.
 * By default this is set to false as defined in the DTD of the
 * content XML format. This means if the attribute is not defined
 * then there is no need to call this function.
 *
 * \exception content_exception_parameter_not_defined
 * This exception is raised if the path or the name parameters do
 * not match any block or parameter in that block.
 *
 * \param[in] path  The path of this parameter.
 * \param[in] name  The name of the parameter to modify.
 * \param[in] overwrite  The new value of the overwrite flag.
 *
 * \sa add_param()
 */
void content::set_param_overwrite(const QString& path, const QString& name, bool overwrite)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined();
    }

    content_params_t::iterator p(b->f_params.find(name));
    if(p == b->f_params.end())
    {
        throw content_exception_parameter_not_defined();
    }

    p->f_overwrite = overwrite;
}

/** \brief Add a link to the specified content.
 *
 * This function links the specified content (defined by path) to the
 * specified destination.
 *
 * The source parameter defines the name of the link, the path (has to
 * be the same as path) and whether the link is unique.
 *
 * The path must already represent a block as defined by the add_content()
 * function call otherwise the function raises an exception.
 *
 * Note that the link is not searched. If it is already defined in
 * the array of links, it will simply be written twice to the
 * database.
 *
 * \warning
 * This function does NOT save the data immediately (if called after the
 * update, then it is saved after the execute() call returns!) Instead
 * the function prepares the data so it can be saved later. This is useful
 * if you expect many changes and dependencies may not all be available at
 * the time you add the content but will be at a later time. If you already
 * have all the data, you may otherwise directly call the
 * links::create_link() function.
 *
 * \exception content_exception_parameter_not_defined
 * The add_content() function was not called prior to this call.
 *
 * \param[in] path  The path where the link is added (source URI, site key + path.)
 * \param[in] source  The link definition of the source.
 * \param[in] destination  The link definition of the destination.
 *
 * \sa add_content()
 * \sa add_xml()
 * \sa add_param()
 * \sa create_link()
 */
void content::add_link(const QString& path, const links::link_info& source, const links::link_info& destination)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined();
    }

    content_link link;
    link.f_source = source;
    link.f_destination = destination;
    b->f_links.push_back(link);
}

/** \brief Signal received when the system request that we save content.
 *
 * This function is called by the snap_child() after the update if any one of
 * the plugins requested content to be saved to the database (in most cases
 * from their content.xml file, although it could be created dynamically.)
 *
 * It may be called again after the execute() if anything more was saved
 * while processing the page.
 */
void content::on_save_content()
{
    // anything to save?
    if(f_blocks.isEmpty())
    {
        return;
    }

    QString site_key(f_snap->get_site_key_with_slash());
    QSharedPointer<QtCassandra::QCassandraTable> content_table(get_content_table());
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        // now do the actual save
        // connect this entry to the corresponding plugin
        // (unless that field is already defined!)
        QString primary_owner(path::get_name(path::SNAP_NAME_PATH_PRIMARY_OWNER));
        if(content_table->row(d->f_path)->cell(primary_owner)->value().nullValue())
        {
            content_table->row(d->f_path)->cell(primary_owner)->setValue(d->f_owner);
        }
        // if != then another plugin took ownership which is fine...
        //else if(content_table->row(d->f_path)->cell(primary_owner)->value().stringValue() != d->f_owner) {
        //}

        // make sure we have our different basic content dates setup
        uint64_t start_date(f_snap->get_uri().option("start_date").toLongLong());
        if(content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_CREATED)))->value().nullValue())
        {
            // do not overwrite the created date
            content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_CREATED)))->setValue(start_date);
        }
        if(content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_UPDATED)))->value().nullValue())
        {
            // updated changes only because of a user action (i.e. Save)
            content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_UPDATED)))->setValue(start_date);
        }
        // always overwrite the modified date
        content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_MODIFIED)))->setValue(start_date);

        // save the parameters (i.e. cells of data defined by the developer)
        for(content_params_t::iterator p(d->f_params.begin());
                p != d->f_params.end(); ++p)
        {
            // make sure no parameter is defined as path::primary_owner
            // because we are 100% in control of that one!
            // (we may want to add more as time passes)
            if(p->f_name == primary_owner)
            {
                throw std::runtime_error("content::on_save_content() cannot accept a parameter named \"path::primary_owner\" as it is reserved");
            }

            // we just saved the path::primary_owner so the row exists now
            //if(content_table->exists(d->f_block.f_path)) ...

            // unless the developer said to overwrite the data, skip
            // the save if the data alerady exists
            if(p->f_overwrite
            || content_table->row(d->f_path)->cell(p->f_name)->value().nullValue())
            {
                content_table->row(d->f_path)->cell(p->f_name)->setValue(p->f_data);
            }
        }

        // link this entry to its parent automatically
        // first we need to remove the site key from the path
        QString path(d->f_path.mid(site_key.length()));
        QStringList parts(path.split('/', QString::SkipEmptyParts));
        while(parts.count() > 0)
        {
            QString src(parts.join("/"));
            src = site_key + src;
            parts.pop_back();
            QString dst(parts.join("/"));
            dst = site_key + dst;
            links::link_info source("parent", true, src);
            links::link_info destination("children", false, dst);
// TODO only repeat if the parent did not exist, otherwise we assume the
//      parent created its own parent/children link already.
//printf("parent/children [%s]/[%s]\n", src.toUtf8().data(), dst.toUtf8().data());
            links::links::instance()->create_link(source, destination);
        }
    }

    // link the nodes together (on top of the parent/child links)
    // this is done as a second step so we're sure that all the source and
    // destination rows exist at the time we create the links
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        for(content_links_t::iterator l(d->f_links.begin());
                l != d->f_links.end(); ++l)
        {
//printf("developer link: [%s]/[%s]\n", l->f_source.key().toUtf8().data(), l->f_destination.key().toUtf8().data());
            links::links::instance()->create_link(l->f_source, l->f_destination);
        }
    }

    // we're done with that set of data
    f_blocks.clear();
}

int content::js_property_count() const
{
    return 1;
}

QVariant content::js_property_get(const QString& name) const
{
    if(name == "modified")
    {
        return "content::modified";
    }
    return QVariant();
}

QString content::js_property_name(int index) const
{
    return "modified";
}

QVariant content::js_property_get(int index) const
{
    if(index == 0)
    {
        return "content::modified";
    }
    return QVariant();
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
