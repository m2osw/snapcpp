// Snap Websites Server -- form handling
// Copyright (C) 2012-2013  Made to Order Software Corp.
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

#include "form.h"
#include "not_reached.h"
#include "qdomreceiver.h"
#include "log.h"
#include "../messages/messages.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QXmlQuery>
#include <QFile>
#pragma GCC diagnostic pop


SNAP_PLUGIN_START(form, 1, 0)


/** \brief Get an HTML form from an XML document.
 *
 * The form plugin makes use of a set of XSTL templates to generate HTML
 * forms from simple XML documents.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char *get_name(name_t name)
{
    switch(name) {
    case SNAP_NAME_FORMS_TABLE:
        return "forms";

    default:
        // invalid index
        throw snap_exception();

    }
    NOTREACHED();
}

/** \brief Initialize the form plugin.
 *
 * This function initializes the form plugin.
 */
form::form()
    //: f_snap(NULL) -- auto-init
    //, f_form_initialized(false) -- auto-init
    //: f_form_elements("form-xslt")
{
}

/** \brief Destroy the form plugin.
 *
 * This function cleans up the form plugin.
 */
form::~form()
{
}


/** \brief Get a pointer to the form plugin.
 *
 * This function returns an instance pointer to the form plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the form plugin.
 */
form *form::instance()
{
    return g_plugin_form_factory.instance();
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
QString form::description() const
{
    return "The form plugin is used to generate forms from simple XML"
        " documents. This plugin uses an XSLT template to process"
        " the the XML data. This plugin is a required backend plugin.";
}


/** \brief Bootstrap the form.
 *
 * This function adds the events the form plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void form::on_bootstrap(::snap::snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN0(form, "server", server, init);
    SNAP_LISTEN(form, "server", server, process_post, _1);
}

/** \brief Initialize the form plugin.
 *
 * At this point this function does nothing.
 */
void form::on_init()
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
QSharedPointer<QtCassandra::QCassandraTable> form::get_form_table()
{
    return f_snap->create_table(get_name(SNAP_NAME_FORMS_TABLE), "Forms table.");
}

/** \brief Transform an XML form into an HTML document.
 *
 * Transforms the specified XML document using a set of XSTL templates
 * into an HTML document.
 *
 * Each time this function is called a new form identifier is generated.
 * It is important to understand that this means the same XML form will
 * output different HTML data every time this function is called.
 *
 * \param[in] info  The session information used to register this form
 * \param[in] xml  The XML document to transform to HTML
 *
 * \return The HTML document
 */
QDomDocument form::form_to_html(const sessions::sessions::session_info& info, const QDomDocument& xml)
{
    QDomDocument doc_output("body");
    if(!f_form_initialized)
    {
        QFile file(":/xsl/form/core-form.xsl");
        if(!file.open(QIODevice::ReadOnly))
        {
            SNAP_LOG_FATAL("form::form_to_html() could not open core-form.xsl resource file.");
            return doc_output;
        }
        // WARNING WARNING WARNING
        // Do not turn on the namespaces because otherwise it gets all messed
        // up by the toString() below (it's to wonder how messed up it must be
        // in memory.)
        if(!f_form_elements.setContent(&file, false))
        {
            SNAP_LOG_FATAL("form::form_to_html() could not parse core-form.xsl resource file.");
            return doc_output;
        }
        QDomNode p(f_form_elements.firstChild());
        while(!p.isElement())
        {
            // this can happen if we have comments
            if(p.isNull())
            {
                // well... nothing found?
                SNAP_LOG_FATAL("form::form_to_html() could not find the first element.");
                return doc_output;
            }
            p = p.nextSibling();
        }
        QDomElement stylesheet(p.toElement());
        if(stylesheet.tagName() != "xsl:stylesheet")
        {
            // we only can handle stylesheets
            SNAP_LOG_FATAL("form::form_to_html() the first element is not a stylesheet.");
            return doc_output;
        }
        f_form_stylesheet = stylesheet;

        // give other plugins a chance to add their own widgets
        form_element(this);
        f_form_elements_string = f_form_elements.toString();
//printf("form [%s]\n", f_form_elements_string.toUtf8().data());
        f_form_initialized = true;
    }
    QXmlQuery q(QXmlQuery::XSLT20);
    q.setFocus(xml.toString());
    // somehow the bind works here...
    q.bindVariable("form_session", QVariant(sessions::sessions::instance()->create_session(info)));
    q.setQuery(f_form_elements_string);
    QDomReceiver receiver(q.namePool(), doc_output);
    q.evaluateTo(&receiver);
    return doc_output;
}


/** \brief Default implementation of the form element event.
 *
 * This function loads the default XSTL that transform Core defined
 * elements to HTML. Other plugins can add their own widgets.
 * However, they are not expected to hijack the core widgets (i.e.
 * their widget should be given a type after their module such as
 * "beautifier::dropdown".)
 *
 * If the function fails, the XSLT document will not be valid.
 *
 * \param[in,out] f  A pointer to the form plugin
 *
 * \return true if the core form XSL file was successfully loaded.
 */
bool form::form_element_impl(form *f)
{
    return true;
}


/** \brief Add the templates and parameters defined in 'add'.
 *
 * This function merges the elements defined by 'add' to the
 * elements defined in f_form_elements. The merge is done here
 * for two reasons:
 *
 * 1. We can control what happens;
 *
 * 2. We avoid having to write complicated code all over the place.
 *
 * This function only does the merge. We'll want to have a backend process
 * that checks whether all those additions work properly and do not
 * have different plugins overwrite each others data.
 *
 * \param[in] add  The document to merge to the f_form_elements document.
 */
void form::add_form_elements(QDomDocument& add)
{
    QDomNode p(add.firstChild());
    while(!p.isElement())
    {
        // this can happen if we have comments
        if(p.isNull())
        {
            // well... nothing found?
            return;
        }
        p = p.nextSibling();
    }
    QDomElement stylesheet(p.toElement());
    if(stylesheet.tagName() != "stylesheet")
    {
        // we only can handle stylesheets
        return;
    }
    p = stylesheet.firstChild();
    while(!p.isNull())
    {
        if(p.isElement())
        {
            QDomElement e(p.toElement());
            QString name(e.tagName());
            if(name == "param" || name == "template")
            {
                f_form_stylesheet.appendChild(e);
            }
        }
        p = p.nextSibling();
    }
}


/** \brief Add the templates and parameters of the specified XSL file.
 *
 * This function is a helper function that reads a file and adds it to
 * the f_form_elements.
 *
 * \param[in] filename  The name of the XSL file to read and add to f_form_elements.
 *
 * \sa add_form_elements(QDomDocument& add);
 */
void form::add_form_elements(QString& filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        SNAP_LOG_FATAL("form::add_form_elements() could not open core-form.xsl resource file.");
        return;
    }
    QDomDocument add;
    if(!add.setContent(&file, true))
    {
        SNAP_LOG_FATAL("form::add_form_elements() could not parse core-form.xsl resource file.");
        return;
    }
    add_form_elements(add);
}


/** \brief Analyze the URL and process the POST data accordingly.
 *
 * This function searches for the plugin that generated the form this
 * POST is about. The form session is used to determine the data and
 * the path is used to verify that it does indeed correspond to that
 * plugin.
 *
 * The plugin is expected to handle the results although the form
 * processing includes all the necessary code to verify the post
 * contents (minimum, maximum, filters, required fields, etc.)
 *
 * The HTML used in the form includes an input that has a form
 * session variable with what is required for us to access the
 * session table. The following is the line used in the form-core.xml
 * file:
 *
 * \code
 * <input id="form_session" name="form_session" type="hidden" value="{$form_session}"/>
 * \endcode
 *
 * Therefore, we expect the POST variables to always include a
 * "form_session" entry and that entry to represent a session
 * in the sessions table that has not expired yet.
 *
 * \note
 * This function is a server signal generated by the snap_child
 * execute() function.
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void form::on_process_post(const QString& uri_path)
{
    messages::messages *messages(messages::messages::instance());

    // First we verify the session information
    // <input id="form_session" name="form_session" type="hidden" value="{$form_session}"/>
    sessions::sessions::session_info info;
    QString form_session(f_snap->postenv("form_session"));
    sessions::sessions::instance()->load_session(form_session, info);
    switch(info.get_session_type())
    {
    case sessions::sessions::session_info::SESSION_INFO_VALID:
        // unless we get this value we've got a problem with the session itself
        break;

    case sessions::sessions::session_info::SESSION_INFO_MISSING:
        f_snap->die(410, "Form Session Gone", "It looks like you attempted to submit a form without first loading it.", "User sent a form with a form session identifier that is not available.");
        NOTREACHED();
        return;

    case sessions::sessions::session_info::SESSION_INFO_OUT_OF_DATE:
        messages->set_http_error(410, "Form Timeout", "Sorry! You sent this request back to Snap! way too late. It timed out. Please re-enter your information and re-submit.", "User did not click the submit button soon enough, the server session timed out.", true);
        return;

    case sessions::sessions::session_info::SESSION_INFO_USED_UP:
        messages->set_http_error(409, "Form Already Submitted", "This form was already processed. If you clicked Reload, this error is expected.", "The user submitted the same form more than once.", true);
        return;

    default:
        throw std::logic_error("load_session() returned an unexpected SESSION_INFO_... value");

    }

    // verify that one of the paths is valid
    QString cpath(uri_path);
    snap_child::canonicalize_path(cpath);
    if(info.get_page_path() != cpath
    && info.get_object_path() != cpath)
    {
        // the path was tempered with?
        f_snap->die(406, "Not Acceptable", "The POST request does not correspond to the form it was defined for.", "User POSTed a request against form \"" + cpath + "\" with an incompatible page (" + info.get_page_path() + ") or object (" + info.get_object_path() + ") path.");
        NOTREACHED();
    }

    // get the owner of this form (plugin name)
    const QString& owner(info.get_plugin_owner());
    plugin *p(plugins::get_plugin(owner));
    if(p == NULL)
    {
        // we've got a problem, that plugin doesn't even exist?!
        // (it could happen assuming someone is removing plugins while someone else submits a form)
        f_snap->die(403, "Forbidden", "The POST request is not attached to a currently supported plugin.", "Somehow the user posted a form that has a plugin name which is not currently installed.");
        NOTREACHED();
    }
    form_post *fp(dynamic_cast<form_post *>(p));
    if(fp == NULL)
    {
        // the programmer forgot to derive from form_post?!
        throw std::logic_error("you cannot use your plugin as a supporting forms without also deriving it from form_post");
    }

    // retrieve the XML form information so we can verify the data
    // (i.e. the XML includes ranges, filters, data types, etc.)
    QDomDocument xml_form(fp->on_get_xml_form(cpath));

    QDomNodeList widgets(xml_form.elementsByTagName("widget"));
    int count(widgets.length());
    for(int i(0); i < count; ++i)
    {
        QDomNode w(widgets.item(i));
        if(!w.isElement())
        {
            throw std::logic_error("elementsByTagName() returned a node that is not an element");
        }
        QDomElement widget(w.toElement());

        // retrieve the name and type once; use the name to retrieve the post
        // value: QString value(f_snap->postenv(widget_name));
        QDomNamedNodeMap attributes(widget.attributes());
        QDomNode id(attributes.namedItem("id"));
        QString widget_name(id.nodeValue());
        if(widget_name.isEmpty())
        {
            throw std::runtime_error("All widgets must have an id with its HTML variable form name");
        }

        QDomNode type(attributes.namedItem("type"));
        QString widget_type(type.nodeValue());
        if(widget_type.isEmpty())
        {
            throw std::runtime_error("All widgets must have a type with its HTML variable form name");
        }

        QDomNode secret(attributes.namedItem("secret"));
        bool is_secret(!secret.isNull() && secret.nodeValue() == "secret");

        // now validate using a signal so any plugin can take over
        // the validation process
        sessions::sessions::session_info::session_info_type_t session_type(info.get_session_type());
        // pretend that everything is fine so far...
        info.set_session_type(sessions::sessions::session_info::SESSION_INFO_VALID);
        int errcnt(messages->get_error_count());
        int warncnt(messages->get_warning_count());
        validate_post_for_widget(cpath, info, widget, widget_name, widget_type, is_secret);
        if(info.get_session_type() != sessions::sessions::session_info::SESSION_INFO_VALID)
        {
            // it was not valid mark the widgets as errorneous (i.e. so we
            // can display it with an error message)
            if(messages->get_error_count() == errcnt
            && messages->get_warning_count() == warncnt)
            {
                // the pluing marked that it found an error but did not
                // generate an actual error, do so here with a generic
                // error message
                QString value(f_snap->postenv(widget_name));
                messages->set_error(
                    "Invalid Content",
                    "\"" + html_64max(value, is_secret) + "\" is not valid for \"" + widget_name + "\".",
                    "unspecified error for widget",
                    false
                );
            }
            messages::messages::message msg(messages->get_last_message());

            // Add the following to the widget so we can display the
            // widget as having an error and show the error on request
            //
            // <error>
            //   <title>$title</title>
            //   <message>$message</message>
            // </error>

            QDomElement err_tag(xml_form.createElement("error"));
            err_tag.setAttribute("idref", QString("messages_message_%1").arg(msg.get_id()));
            widget.appendChild(err_tag);
            QDomElement title_tag(xml_form.createElement("title"));
            err_tag.appendChild(title_tag);
            QDomText title_text(xml_form.createTextNode(msg.get_title()));
            title_tag.appendChild(title_text);
            QDomElement message_tag(xml_form.createElement("message"));
            err_tag.appendChild(message_tag);
            QDomText message_text(xml_form.createTextNode(msg.get_body()));
            message_tag.appendChild(message_text);
        }
        else
        {
            // restore the last type
            info.set_session_type(session_type);
        }
    }
    // if the previous loop found 1 or more errors, return now
    // (i.e. we do not want to process the data any further in this case)
    if(info.get_session_type() != sessions::sessions::session_info::SESSION_INFO_VALID)
    {
        return;
    }

    // data looks good, let the plugin process it
    fp->on_process_post(cpath, info);
}


/** \brief Ensure that messages don't display extremely large values.
 *
 * This function truncates a value to at most 64 characters (+3 period
 * for the elipsis.)
 *
 * This function is only to truncate plain text. HTML should use the
 * html_64max() function instead.
 *
 * \todo
 * Move to the filter plugin.
 *
 * \param[in] text  The text string to clip to 64 characters.
 * \param[in] is_secret  If true then change text with asterisks.
 *
 * \return The clipped value as required.
 */
QString form::text_64max(const QString& text, bool is_secret)
{
    if(is_secret && !text.isEmpty())
    {
        return "******";
    }

    if(text.length() > 64)
    {
        return text.mid(0, 64) + "...";
    }
    return text;
}


/** \brief Shorten the specified HTML to 64 characters.
 *
 * \todo
 * Move to the filter plugin.
 *
 * \param[in] html  The HTML string to reduce to a maximum of 64 characters.
 * \param[in] is_secret  If true then change text with asterisks.
 *
 * \return The shorten HTML, although still 100% valid HTML.
 */
QString form::html_64max(const QString& html, bool is_secret)
{
    if(is_secret)
    {
        return "******";
    }

    if(html.indexOf('<') == -1)
    {
        // only text, make it easy on us
        // (also a lot faster)
        return text_64max(html, is_secret);
    }

    // TODO: go through the tree and keep data as long
    //       as the text is shorter than 64 characters
    //       and we have less than 100 tags.
    return html;
}


/** \brief Count the number of lines in a text string.
 *
 * This function goes through a text string and counts the number of
 * new line characters.
 *
 * \todo
 * Should we have a flag to know whether empty lines should be counted?
 * Should we count the last line if it doesn't end with a new line
 * character?
 *
 * \param[in] text  The text to count lines in.
 *
 * \return The number of new line characters (or \r\n) found in \p text.
 */
int form::count_text_lines(const QString& text)
{
    int lines(0);

    for(char *s(text.toUtf8().data()); *s != '\0'; ++s)
    {
        if(*s == '\r')
        {
            ++lines;
            if(s[1] == '\r')
            {
                // \r\n <=> one line
                ++s;
            }
        }
        else if(*s == '\n')
        {
            ++lines;
        }
    }

    return lines;
}


/** \brief Count the number of lines in an HTML buffer.
 *
 * This function goes through the HTML in \p html and counts the number
 * paragraphs.
 *
 * \param[in] html  The html to count lines in.
 *
 * \return The number of paragraphs in the HTML buffer.
 */
int form::count_html_lines(const QString& html)
{
    int lines(0);

    QDomDocument doc;
    doc.setContent(html);
    QDomElement parent(doc.documentElement());
    
    // go through all the children elements
    for(QDomElement child(parent.firstChildElement()); !child.isNull(); child = child.nextSiblingElement())
    {
        QString name(child.nodeName());
        if(name == "p" || name == "div")
        {
            // <p> and <div> are considered paragraphs
            // (TBD: should we count the number of <p> inside a <div>)
            ++lines;
        }
    }

    return lines;
}


/** \brief Start a widget validation.
 *
 * This function prepares the validation of the specified widget by
 * applying common core validations.
 *
 * The \p info parameter is used for the result. If something is wrong,
 * then the type of the session is changed from SESSION_INFO_VALID to
 * one of the SESSION_INFO_... that represent an error, in most cases we
 * use SESSION_INFO_INCOMPATIBLE.
 *
 * \param[in] cpath  The path where the form is defined
 * \param[in,out] info  The information linked with this form (loaded from the session)
 * \param[in] widget  The widget being tested
 * \param[in] widget_name  The name of the widget (i.e. the id="..." attribute value)
 * \param[in] widget_type  The type of the widget (i.e. the type="..." attribute value)
 */
bool form::validate_post_for_widget_impl(const QString& cpath, sessions::sessions::session_info& info, const QDomElement& widget, const QString& widget_name, const QString& widget_type, bool is_secret)
{
    messages::messages *messages(messages::messages::instance());

    // get the value we're going to validate
    QString value(f_snap->postenv(widget_name));
    bool has_minimum(false);

    QDomElement sizes(widget.firstChildElement("sizes"));
    if(!sizes.isNull())
    {
        QDomElement min(sizes.firstChildElement("min"));
        if(!min.isNull())
        {
            has_minimum = true;
            QString m(min.text());
            bool ok;
            int l(m.toInt(&ok));
            if(!ok)
            {
                throw std::runtime_error(("the minimum size \"" + m + "\" must be a valid decimal integer").toUtf8().data());
            }
            if(value.length() < l)
            {
                // length too small
                messages->set_error(
                    "Length Too Small",
                    "\"" + html_64max(value, is_secret) + "\" is too small in \"" + widget_name + "\". The widget requires at least " + m + " characters.",
                    "not enough characters error",
                    false
                );
                info.set_session_type(sessions::sessions::session_info::SESSION_INFO_INCOMPATIBLE);
            }
        }
        QDomElement max(sizes.firstChildElement("max"));
        if(!max.isNull())
        {
            QString m(max.text());
            bool ok;
            int l(m.toInt(&ok));
            if(!ok)
            {
                throw std::runtime_error("the maximum size must be a valid decimal integer");
            }
            if(value.length() > l)
            {
                // not enough characters
                messages->set_error(
                    "Length Too Long",
                    "\"" + html_64max(value, is_secret) + "\" is too long in \"" + widget_name + "\". The widget requires at most " + m + " characters.",
                    "too many characters error",
                    false
                );
                info.set_session_type(sessions::sessions::session_info::SESSION_INFO_INCOMPATIBLE);
            }
        }
        QDomElement lines(sizes.firstChildElement("lines"));
        if(!lines.isNull())
        {
            QString m(lines.text());
            bool ok;
            int l(m.toInt(&ok));
            if(!ok)
            {
                throw std::runtime_error("the number of lines must be a valid decimal integer");
            }
            if(widget_type == "text-edit")
            {
                if(count_text_lines(value) > l)
                {
                    // not enough lines (text)
                    messages->set_error(
                        "Length Too Long",
                        "\"" + html_64max(value, is_secret) + "\" is too long in \"" + widget_name + "\". The widget requires at most " + m + " characters.",
                        "too many characters error",
                        false
                    );
                    info.set_session_type(sessions::sessions::session_info::SESSION_INFO_INCOMPATIBLE);
                }
            }
            else if(widget_type == "html-edit")
            {
                if(count_html_lines(value) < l)
                {
                    // not enough lines (HTML)
                    messages->set_error(
                        "Length Too Long",
                        "\"" + html_64max(value, is_secret) + "\" is too long in \"" + widget_name + "\". The widget requires at most " + m + " characters.",
                        "too many characters error",
                        false
                    );
                    info.set_session_type(sessions::sessions::session_info::SESSION_INFO_INCOMPATIBLE);
                }
            }
        }
    }

    // check whether the field is required, in case of a checkbox required
    // means that the user selects the checkbox ("on")
    if(widget_type == "line-edit" || widget_type == "password" || widget_type == "checkbox")
    {
        QDomElement required(widget.firstChildElement("required"));
        if(!required.isNull())
        {
            if(required.text() == "required")
            {
                // avoid the error if the minimum size error was already applied
                if(!has_minimum && value.isEmpty())
                {
                    messages->set_error(
                        "Value is Invalid",
                        "\"" + widget_name + "\" is a required field.",
                        "no data entered in widget by user",
                        false
                    );
                    info.set_session_type(sessions::sessions::session_info::SESSION_INFO_INCOMPATIBLE);
                }
            }
        }
    }

    QDomElement filters(widget.firstChildElement("filters"));
    if(!filters.isNull())
    {
        QDomElement regex_tag(filters.firstChildElement("regex"));
        if(!regex_tag.isNull())
        {
            QString re;

            QDomNamedNodeMap attributes(regex_tag.attributes());
            QDomNode name(attributes.namedItem("name"));
            if(!name.isNull())
            {
                QString regex_name(name.nodeValue());
                if(!regex_name.isEmpty())
                {
                    switch(regex_name[0].unicode())
                    {
                    case 'd':
                        if(regex_name == "decimal")
                        {
                            re = "^[0-9]+(?:\\.[0-9]+)?$";
                        }
                        break;

                    case 'e':
                        if(regex_name == "email")
                        {
                            // For emails we accept anything except local emails:
                            //     <name>@[<sub-domain>.]<domain>.<tld>
                            re = "/^[a-z0-9_\\-\\.\\+\\^!#\\$%&*+\\/\\=\\?\\`\\|\\{\\}~\\']+@(?:[a-z0-9]|[a-z0-9][a-z0-9\\-]*[a-z0-9])+\\.(?:(?:[a-z0-9]|[a-z0-9][a-z0-9\\-]*[a-z0-9])\\.?)+$/i";
                        }
                        break;

                    case 'f':
                        if(regex_name == "float")
                        {
                            re = "^[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?$";
                        }
                        break;

                    case 'i':
                        if(regex_name == "integer")
                        {
                            re = "^[0-9]+$";
                        }
                        break;

                    }
                    // TBD: offer other plugins to support their own regex?
                }
                // else -- should empty be ignored? TBD
                if(re.isEmpty())
                {
                    // TBD: this can be a problem if we remove a plugin that
                    //      adds some regexes (although right now we don't
                    //      have such a signal...)
                    throw std::runtime_error(("the regular expression named \"" + regex_name + "\" is not supported.").toUtf8().data());
                }
                // Note:
                // We don't test whether there is some text here to avoid
                // wasting time; we could have such a test in a tool of
                // ours used to verify that the form is well defined.
            }
            else
            {
                re = regex_tag.text();
            }

            Qt::CaseSensitivity cs(Qt::CaseSensitive);
            if(!re.isEmpty() && re[0] == '/')
            {
                re = re.mid(1);
            }
            int p(re.lastIndexOf('/'));
            if(p >= 0)
            {
                QString flags(re.mid(p + 1));
                re = re.mid(0, p);
                for(char *s(flags.toUtf8().data()); *s != '\0'; ++s)
                {
                    switch(*s)
                    {
                    case 'i':
                        cs = Qt::CaseInsensitive;
                        break;

                    default:
                        {
                        char buf[2];
                        buf[0] = *s;
                        buf[1] = '\0';
                        throw std::runtime_error(("\"" + QString::fromLatin1(buf) + "\" is not a supported regex flag").toUtf8().data());
                        }

                    }
                }
            }
            QRegExp reg_expr(re, cs, QRegExp::RegExp2);
            if(!reg_expr.isValid())
            {
                throw std::runtime_error(("\"" + re + "\" regular expression is invalid.").toUtf8().data());
            }
            if(reg_expr.indexIn(value) == -1)
            {
                messages->set_error(
                    "Invalid Value",
                    "\"" + html_64max(value, is_secret) + "\" is not valid for \"" + widget_name + "\".",
                    "the value did not match the filter regular expression",
                    false
                );
                info.set_session_type(sessions::sessions::session_info::SESSION_INFO_INCOMPATIBLE);
            }
        }
    }

    // Note:
    // We always return true because errors generated here are first but
    // complimentary errors may be generated by other plugins
    return true;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4
