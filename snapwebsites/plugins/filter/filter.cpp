// Snap Websites Server -- filter
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

#include "filter.h"
#include "snapwebsites.h"
#include "plugins.h"
#include "qdomxpath.h"
#include "../content/content.h"
#include <iostream>
#include <cctype>
#include <QtCore/QDebug>


SNAP_PLUGIN_START(filter, 1, 0)

/** \brief Initialize the filter plugin.
 *
 * This function is used to initialize the filter plugin object.
 */
filter::filter()
    : f_snap(NULL)
{
//std::cerr << " - Created filter!\n";
}

/** \brief Clean up the filter plugin.
 *
 * Ensure the filter object is clean before it is gone.
 */
filter::~filter()
{
//std::cerr << " - Destroying filter!\n";
}

/** \brief Get a pointer to the filter plugin.
 *
 * This function returns an instance pointer to the filter plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the filter plugin.
 */
filter *filter::instance()
{
    return g_plugin_filter_factory.instance();
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
QString filter::description() const
{
    return "This plugin offers functions to filter XML and HTML data."
        " Especially, it is used to avoid Cross Site Attacks (XSS) from"
        " hackers. XSS is a way for a hacker to gain access to a person's"
        " computer through someone's website.";
}

/** \brief Initialize the filter plugin.
 *
 * This function terminates the initialization of the filter plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void filter::on_bootstrap(::snap::snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(filter, "server", server, xss_filter, _1, _2, _3);
}

/** \brief Filter an DOM node and remove all unwanted tags.
 *
 * This filter accepts:
 *
 * \li A DOM node (QDomNode) which is to be filtered
 * \li A string of tags to be kept
 * \li A string of attributes to be kept (or removed)
 *
 * The \p accepted_tags parameter is a list of tags names separated
 * by spaces (i.e. "a h1 h2 h3 img br".)
 *
 * By default the \p accepted_attributes parameter includes all the
 * attributes to be kept. You can inverse the meaning using a ! character
 * at the beginning of the string (i.e. "!style" instead of
 * "href target title alt src".) At some point we probably want
 * to have a DTD like support where each tag has its list of
 * attributes checked and unknown attributes removed. This list would
 * be limited since only user entered tags need to be checked
 * (i.e. "p div a b img ul dl ol li ...")
 *
 * The ! character does not work in the \p accepted_tags parameter.
 *
 * The filtering makes sure of the following:
 *
 * 1) The strings are valid UTF-8 (Bad UTF-8 can cause problems in IE6)
 *
 * 2) The content does not include a NUL
 *
 * 3) The syntax of the tags (this is done through the use of the htmlcxx tree)
 *
 * 4) Remove entities that look like NT4 entities: &{\<name>};
 *
 * \note
 * The call \c QDomNode::toString().toUft8().data() generates a valid UTF-8
 * string no matter what. Therefore we do not need to filter for illegal
 * characters in this filter function. The output will always be correct.
 *
 * \param[in,out] text  The HTML to be filtered.
 * \param[in] accepted_tags  The list of tags kept in the text.
 * \param[in] accepted_attributes  The list of attributes kept in the tags.
 */
void filter::on_xss_filter(QDomNode& node,
                           const QString& accepted_tags,
                           const QString& accepted_attributes)
{
    // initialize the array of tags so it starts and ends with spaces
    // this allows for much faster searches (i.e. indexOf())
    const QString tags(" " + accepted_tags + " ");

    QString attr(" " + accepted_attributes + " ");
    const bool attr_refused = accepted_attributes[0] == '!';
    if(attr_refused) {
        // erase the '!' from the attr string
        attr.remove(1, 1);
    }

    // go through the entire tree
    QDomNode n = node.firstChild();
    while(!n.isNull())
    {
        // determine the next pointer so we can delete this node
        QDomNode parent = n.parentNode();
        QDomNode next = n.firstChild();
        if(next.isNull())
        {
            next = n.nextSibling();
            if(next.isNull())
            {
                QDomNode p(parent);
                do
                {
                    next = p.nextSibling();
                    p = p.parentNode();
                }
                while(next.isNull() && !p.isNull());
            }
        }

        // Is this node a tag? (i.e. an element)
        if(n.isElement())
        {
            QDomElement e = n.toElement();
            const QString& name = e.tagName();
            if(tags.indexOf(" " + name.toLower() + " ") == -1)
            {
//qDebug() << "removing tag: [" << name << "] (" << tags << ")\n";
                // remove this tag, now there are two different type of
                // removal: complete removal (i.e. <script>) and removal
                // of the tag, but not the children (i.e. <b>)
                // the xmp and plaintext are browser extensions
                if(name != "script" && name != "style" && name != "textarea"
                && name != "xmp" && name != "plaintext")
                {
                    // in this case we can just remove the tag itself but keep
                    // its children which we have to move up once
                    QDomNode c = n.firstChild();
                    while(!c.isNull())
                    {
                        QDomNode next_sibling = c.nextSibling();
                        n.removeChild(c);
                        parent.insertBefore(c, n);
                        c = next_sibling;
                    }
                }
                parent.removeChild(n);
                next = parent.nextSibling();
                if(next.isNull())
                {
                    next = parent;
                }
            }
            else
            {
                // remove unwanted attributes too
                QDomNamedNodeMap attributes(n.attributes());
                int max = attributes.length();
                for(int i = 0; i < max; ++i)
                {
                    QDomAttr a = attributes.item(i).toAttr();
                    const QString& attr_name = a.name();
                    if((attr.indexOf(" " + attr_name.toLower() + " ") == -1) ^ attr_refused)
                    {
                        e.removeAttribute(attr_name);
//qDebug() << "removing attribute: " << attr_name << " max: " << max << " size: " << attributes.length() << "\n";
                    }
                }
            }
        }
        else if(n.isComment() || n.isProcessingInstruction()
             || n.isNotation() || n.isEntity() || n.isDocument()
             || n.isDocumentType() || n.isCDATASection())
        {
            // remove all sorts of unwanted nodes
            // these are not tags, but XML declarations which have nothing
            // to do in clients code that is parsed via the XSS filter
            //
            // to consider:
            // transform a CDATA section to plain text
            //
            // Note: QDomComment derives from QDomCharacterData
            //       QDomCDATASection derives from QDomText which derives from QDomCharacterData
//qDebug() << "removing \"wrong\" node type\n";
            parent.removeChild(n);
            next = parent.nextSibling();
            if(next.isNull())
            {
                next = parent;
            }
        }
        // the rest is considered to be text
        n = next;
    }
}


/** \brief Replace a token with a corresponding value.
 *
 * This function is expected to replace the specified token with a
 * replacement value. For example, the '[year]' token can be replaced
 * with the current year.
 *
 * The default filter function knows how to handle the special token
 * '[test]' which is replaced by the text "The Test Token Worked" written
 * in bold. It does not support parameters although it ignores the fact
 * (i.e. parameters can be used, they will be ignored.)
 *
 * The default filter replace_token event supports the following
 * general tokens:
 *
 * \li [test] -- a simple test token, it inserts "The Test Token Worked"
 *               message, in English.
 * \li [select("\<xpath>")] -- select content from the XML document using
 *                             the specified \<xpath>
 * \li [date(\"format\")] -- date with format as per strftime(); without
 *                           format, use the default which is %m/%d/%Y
 * \li [version] -- version of the Snap! C++ server
 * \li [year] -- the 4-digit year when the request started
 *
 * \param[in] f  The filter object.
 * \param[in] cpath  The canonicalized path linked with this XML document.
 * \param[in,out] xml  The XML document where tokens are being replaced.
 * \param[in,out] token  The token object, with the token name and optional parameters.
 */
bool filter::replace_token_impl(filter *f, QString const& cpath, QDomDocument& xml, token_info_t& token)
{
    if(token.is_token("test"))
    {
        token.f_replacement = "<span style=\"font-weight: bold;\">The Test Token Worked</span>";
    }
    else if(token.is_token("select"))
    {
        if(token.verify_args(1, 1))
        {
            parameter_t param(token.get_arg("xpath", 0));
            if(!token.f_error)
            {
                // in this case the XPath is dynamic so we have to compile now
                QDomXPath dom_xpath;
                dom_xpath.setXPath(param.f_value);
                QDomXPath::node_vector_t result(dom_xpath.apply(xml));
//FILE *o(fopen("/tmp/test.xml", "w"));
//fprintf(o, "%s\n", xml.toString().toUtf8().data());
//fclose(o);
                // at this point we expect the result to be 1 (or 0) entries
                // if more than 1, ignore the following nodes
                if(result.size() > 0)
                {
                    // apply the replacement
                    if(result[0].isElement())
                    {
                        QDomDocument document;
                        QDomNode copy(document.importNode(result[0], true));
                        document.appendChild(copy);
                        token.f_replacement = document.toString();
                    }
                    else if(result[0].isAttr())
                    {
                        token.f_replacement = result[0].toAttr().value();
                    }
                }
            }
        }
    }
    else if(token.is_token("year"))
    {
        time_t now(f_snap->get_start_time());
        struct tm time_info;
        gmtime_r(&now, &time_info);
        token.f_replacement = QString("%1").arg(time_info.tm_year + 1900);
    }
    else if(token.is_token("date"))
    {
        if(token.verify_args(0, 2))
        {
            time_t unix_time(f_snap->get_start_time());
            QString date_format("%m/%d/%Y");
            if(token.f_parameters.size() >= 1)
            {
                parameter_t param(token.get_arg("format", 0, TOK_STRING));
                date_format = param.f_value;
            }
            if(token.f_parameters.size() >= 2)
            {
                parameter_t param(token.get_arg("unixtime", 1, TOK_STRING));
                bool ok(false);
                unix_time = param.f_value.toLongLong(&ok);
            }
            struct tm t;
            gmtime_r(&unix_time, &t);
            char buf[256];
            strftime(buf, sizeof(buf), date_format.toUtf8().data(), &t);
            buf[sizeof(buf) / sizeof(buf[0]) - 1] = '\0'; // make sure there is a NUL
            token.f_replacement = QString::fromUtf8(buf);
        }
    }
    else if(token.is_token("version"))
    {
        token.f_replacement = SNAPWEBSITES_VERSION_STRING;
    }

    return true;
}


/** \brief Read all the XML text and replace its tokens.
 *
 * This function searches all the XML text and replace the tokens it finds
 * in these texts with the corresponding replacement value.
 *
 * The currently supported syntax is:
 *
 * \code
 *   '[' <name> [ '(' [ [ <name> '=' ] <param> ',' ... ] ')' ] ']'
 * \endcode
 *
 * where \<name> is composed of letter, digit, and colon characters.
 *
 * where \<param> is composed of identifiers, numbers, or quoted strings
 * (' or "); parameters are separated by commas and can be named if
 * preceded by a name and an equal sign.
 *
 * Spaces are allowed between parameters and parenthesis. However, no space
 * is allowed after the opening square bracket ([). Spaces are ignored and
 * are not required.
 *
 * \param[in] cpath  The canonicalized path being processed.
 * \param[in,out] xml  The XML document to filter.
 */
void filter::on_token_filter(QString const& cpath, QDomDocument& xml)
{
    class text_t
    {
    public:
        typedef ushort char_t;

        text_t(filter *f, QString const& cpath, QDomDocument& xml, QString const& text)
            : f_filter(f)
            , f_cpath(cpath)
            , f_xml(xml)
            , f_index(0)
            , f_extra_index(0)
            , f_text(text)
            //, f_result("") -- auto-init
            //, f_token("") -- auto-init
            //, f_replacement("") -- auto-init
            //, f_extra_input("") -- auto-init
        {
        }

        bool parse()
        {
            f_result = "";
            f_result.reserve(f_text.size() * 2);

            bool changed(false);
            for(char_t c(0); c = getc(); )
            {
                if(c == '[')
                {
                    if(parse_token())
                    {
                        changed = true;
                    }
                    else
                    {
                        // it failed, add the token content as is
                        // to the result
                        f_result += f_token;
                    }
                }
                else
                {
                    f_result += c;
                }
            }

            return changed;
        }

        const QString& result() const
        {
            return f_result;
        }

    private:
        // it is not yet proven to be a token...
        bool parse_token()
        {
            token_info_t info;

            // reset the token variable
            f_token = "[";
            token_t t(get_token(info.f_name, false));
            f_token += info.f_name;
            if(t != TOK_IDENTIFIER)
            {
                // the '[' must be followed by an identifier, no choice here
                return false;
            }
            QString tok;
            t = get_token(tok);
            f_token += tok;
            if(t != TOK_SEPARATOR || (tok != "]" && tok != "("))
            {
                // we can only have a ']' or '(' separator at this point
                return false;
            }
            if(tok == "(")
            {
                // note: the list of parameters may be empty
                t = get_token(tok);
                f_token += tok;
                if(t != TOK_SEPARATOR || tok != ")")
                {
                    parameter_t param;
                    param.f_type = t;
                    param.f_value = tok;
                    for(;;)
                    {
                        switch(param.f_type)
                        {
                        case TOK_IDENTIFIER:
                            {
                                t = get_token(tok);
                                f_token += tok;
                                if(t == TOK_IDENTIFIER && tok == "=")
                                {
                                    // named parameter; the identifier was the name
                                    // and not the value, swap those
                                    param.f_name = param.f_value;
                                    param.f_type = get_token(param.f_value);
                                    f_token += param.f_value;
                                    switch(param.f_type)
                                    {
                                    case TOK_STRING:
                                    case TOK_INTEGER:
                                    case TOK_REAL:
                                        break;

                                    default:
                                        return false;

                                    }
                                    t = get_token(tok);
                                }
                            }
                            break;

                        case TOK_STRING:
                            // remove the quotes from the parameters
                            param.f_value = param.f_value.mid(1, param.f_value.size() - 2);
                            /*FALLTHROUGH*/
                        case TOK_INTEGER:
                        case TOK_REAL:
                            t = get_token(tok);
                            break;

                        default:
                            // anything else is wrong
                            return false;

                        }
                        info.f_parameters.push_back(param);

                        if(t != TOK_SEPARATOR)
                        {
                            // only commas are accepted here until we find
                            // a closing parenthesis
                            return false;
                        }

                        if(tok == ")")
                        {
                            // we're done reading the list of parameters
                            break;
                        }
                        if(tok != ",")
                        {
                            // we only accept commas between parameters
                            return false;
                        }

                        param.reset();
                        param.f_type = get_token(param.f_value);
                        f_token += param.f_value;
                    }
                }
                t = get_token(tok);
                f_token += tok;
            }
            if(tok != "]")
            {
                // a token must end with ']'
                return false;
            }

            // valid input, now verify that it does exist in the current
            // installation
            f_filter->replace_token(f_filter, f_cpath, f_xml, info);
            if(!info.f_found)
            {
                // the token is not known, that's an error so we do not
                // replace anything
                return false;
            }
            ungets(info.f_replacement);

            return true;
        }

        token_t get_token(QString& tok, bool skip_spaces = true)
        {
            char_t c;
            for(;;)
            {
                c = getc();
                if(c == '[')
                {
                    // recursively parse sub-tokens
                    QString save_token(f_token);
                    if(!parse_token())
                    {
                        f_token = save_token + f_token;
                        return TOK_INVALID;
                    }
                    f_token = save_token;
                }
                else if(c != ' ' || ~skip_spaces)
                {
                    break;
                }
                else
                {
                    // the space is needed in case the whole thing fails
                    f_token += c;
                }
            }
            tok = c;
            switch(c)
            {
            case '"':
            case '\'':
                {
                    char_t quote(c);
                    tok = quote;
                    do
                    {
                        c = getc();
                        if(c == '\0')
                        {
                            return TOK_INVALID;
                        }
                        tok += c;
                        if(c == '\\')
                        {
                            c = getc();
                            if(c == '\0')
                            {
                                return TOK_INVALID;
                            }
                            tok += c;
                            // ignore quotes if escaped
                            c = '\0';
                        }
                    }
                    while(c != quote);
                    tok = tok.mid(0, tok.size() - 1) + quote;
                }
                return TOK_STRING;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                c = getc();
                while(c >= '0' && c <= '9')
                {
                    tok += c;
                    c = getc();
                }
            case '.':
                if(c == '.')
                {
                    tok += c;
                    c = getc();
                    while(c >= '0' && c <= '9')
                    {
                        tok += c;
                        c = getc();
                    }
                    ungetc(c);
                    return TOK_REAL;
                }
                ungetc(c);
                return TOK_INTEGER;

            // separators
            case ']':
            case '(':
            case ')':
            case ',':
            case '=':
                return TOK_SEPARATOR;

            default:
                if((c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z'))
                {
                    // identifier
                    c = getc();
                    while((c >= 'a' && c <= 'z')
                       || (c >= 'A' && c <= 'Z')
                       || (c >= '0' && c <= '9')
                       || c == '_' || c == ':')
                    {
                        tok += c;
                        c = getc();
                    }
                    ungetc(c);
                    return TOK_IDENTIFIER;
                }
                return TOK_INVALID;

            }
        }

        void ungets(const QString& s)
        {
            f_extra_input.remove(0, f_extra_index);
            f_extra_input.insert(0, s);

            // plugins that generate a token  replacement from a QDomDocument
            // start with the <!DOCTYPE ...> tag which we have to remove here
            if(f_extra_input.startsWith("<!DOCTYPE"))
            {
                // note that if the indexOf() fails to find the '>' then
                // it returns -1 which is fine because the + 1 will get
                // it right back to 0 which is what we want in that case
                f_extra_index = f_extra_input.indexOf('>') + 1;
            }
            else
            {
                f_extra_index = 0;
            }
        }

        void ungetc(char_t c)
        {
            f_extra_input.remove(0, f_extra_index);
            f_extra_index = 0;
            f_extra_input.insert(0, c);
        }

        char_t getc()
        {
            if(!f_extra_input.isEmpty())
            {
                if(f_extra_index < f_extra_input.size())
                {
                    char_t wc(f_extra_input[f_extra_index].unicode());
                    ++f_extra_index;
                    return wc;
                }
                f_extra_index = 0;
                f_extra_input = "";
            }
            if(f_index >= f_text.size())
            {
                return '\0';
            }
            else
            {
                char_t wc(f_text[f_index].unicode());
                ++f_index;
                return wc;
            }
        }

        filter *        f_filter;
        QString         f_cpath;
        QDomDocument    f_xml;
        int             f_index;
        int             f_extra_index;
        QString         f_text;
        QString         f_result;
        QString         f_token;
        QString         f_replacement;
        QString         f_extra_input;
        QString         f_token_name;
    };

    QDomNode n = xml.firstChild();
    while(!n.isNull())
    {
        // determine the next pointer so we can delete this node
        QDomNode parent = n.parentNode();
        QDomNode next = n.firstChild();
        if(next.isNull())
        {
            next = n.nextSibling();
            if(next.isNull())
            {
                QDomNode p(parent);
                do
                {
                    next = p.nextSibling();
                    p = p.parentNode();
                }
                while(next.isNull() && !p.isNull());
            }
        }

        // TODO support comments, instructions, etc.

        // we want to transform tokens in text areas and in attributes
        if(n.isCDATASection())
        {
            // this works too, although the final result is still plain text!
            // (it must be xslt that converts the contents of CDATA sections)
            QDomCDATASection cdata_section(n.toCDATASection());
//printf("checking CDATA [%s]\n", cdata_section.data().toUtf8().data());
            text_t t(this, cpath, xml, cdata_section.data());
            if(t.parse())
            {
                // replace the text with its contents
                cdata_section.setData(t.result());
            }
        }
        else if(n.isText())
        {
            QDomText text(n.toText());
//printf("checking text [%s]\n", text.data().toUtf8().data());
            text_t t(this, cpath, xml, text.data());
            if(t.parse())
            {
                // replace the text with its contents
                const QString& result(t.result());
                if(result.contains('<'))
                {
                    // the tokens added HTML... replace the whole text node
                    QDomDocument doc_text("snap");
                    doc_text.setContent("<text>" + result + "</text>", true, NULL, NULL, NULL);
                    QDomDocumentFragment frag(xml.createDocumentFragment());
                    frag.appendChild(xml.importNode(doc_text.documentElement(), true));
                    QDomNodeList children(frag.firstChild().childNodes());
                    const int max(children.size());
                    QDomNode previous(n);
                    for(int i(0); i < max; ++i)
                    {
                        QDomNode l(children.at(0));
                        parent.insertAfter(children.at(0), previous);
                        previous = l;
                    }
                    parent.removeChild(n);
                }
                else
                {
                    text.setData(result);
                }
            }
        }

        // the rest is considered to be text
        n = next;
    }
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
