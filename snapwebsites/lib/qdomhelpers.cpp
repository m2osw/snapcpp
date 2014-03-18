// Snap Websites Server -- DOM helper functions
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

#include "qdomhelpers.h"
#include "qstring_stream.h"

#include <iostream>

#include <QStringList>

#include "poison.h"

namespace snap
{
namespace snap_dom
{


/** \brief Useful function that transforms a QString to XML.
 *
 * When inserting a string in the XML document and that string may include
 * HTML code, call this function, it will first convert the string to XML
 * then insert the result as children of the \p child element.
 *
 * \param[in,out] child  DOM element receiving the result as children nodes.
 * \param[in] xml  The input XML string.
 */
void insert_html_string_to_xml_doc(QDomNode& child, QString const& xml)
{
    // parsing the XML can be slow, try to avoid that if possible
    if(xml.contains('<'))
    {
        QDomDocument xml_doc("wrapper");
        xml_doc.setContent("<wrapper>" + xml + "</wrapper>", true, nullptr, nullptr, nullptr);
        insert_node_to_xml_doc(child, xml_doc.documentElement());
    }
    else
    {
        QDomText text(child.ownerDocument().createTextNode(xml));
        child.appendChild(text);
    }
}


/** \brief Insert a node's children in a node of another document.
 *
 * This function copies all the children of the specified \p node
 * at the end of the child node.
 *
 * \param[in,out] child  The destination node.
 * \param[in] node  The source element node.
 */
void insert_node_to_xml_doc(QDomNode& child, QDomNode const& node)
{
    // copy the result in a fragment of our document
    QDomDocumentFragment frag(child.ownerDocument().createDocumentFragment());
    frag.appendChild(child.ownerDocument().importNode(node, true));

    // copy the fragment nodes at the right place
    QDomNodeList children(frag.firstChild().childNodes());

    QDomNode previous;
    while(!children.isEmpty())
    {
        QDomNode l(children.at(0));
        if(previous.isNull())
        {
            // the first time append at the end of the existing data
            child.appendChild(l);
        }
        else
        {
            child.insertAfter(l, previous);
        }
        previous = l;
    }
}


/** \brief Useful function that transforms a QString to XML.
 *
 * When inserting a string in the XML document and that string may include
 * HTML code, call this function, it will first convert the string to XML
 * then insert the result as children of the \p child element.
 *
 * \param[in,out] child  DOM element receiving the result as children nodes.
 * \param[in] xml  The input XML string.
 */
void replace_node_with_html_string(QDomNode& replace, QString const& xml)
{
    // parsing the XML can be slow, try to avoid that if possible
    if(xml.contains('<'))
    {
        QDomDocument xml_doc("wrapper");
        xml_doc.setContent("<wrapper>" + xml + "</wrapper>", true, nullptr, nullptr, nullptr);
        replace_node_with_elements(replace, xml_doc.documentElement());
    }
    else
    {
        QDomText text(replace.toText());
        text.setData(xml);
    }
}


/** \brief Replace a node with another.
 *
 * This function replaces the node \p replace with the node \p node.
 *
 * Note that the function creates a copy of \p node as if it were from
 * another document.
 *
 * \param[in,out] replace  The node to be replaced.
 * \param[in] node  The source node to copy in place of \p replace.
 */
void replace_node_with_elements(QDomNode& replace, QDomNode const& node)
{
    QDomNode parent(replace.parentNode());

    // copy the result in a fragment of our document
    QDomDocumentFragment frag(replace.ownerDocument().createDocumentFragment());
    frag.appendChild(replace.ownerDocument().importNode(node, true));

    // copy the fragment nodes at the right place
    QDomNodeList children(frag.firstChild().childNodes());

    QDomNode previous(replace);
    while(!children.isEmpty())
    {
        QDomNode l(children.at(0));
        parent.insertAfter(l, previous);
        previous = l;
    }

    // got replaced, now remove that node
    parent.removeChild(replace);
}


/** \brief Delete all the children of a given element node.
 *
 * This function loops until all the children of a given element node
 * were removed.
 *
 * \param[in,out] parent  The node from which all the children should be
 *                        removed.
 */
void remove_all_children(QDomElement& parent)
{
    for(;;)
    {
        // Note: I use the last child because it is much more likely that
        //       this way we avoid a memmove() of the vector of children
        QDomNode child(parent.lastChild());
        if(child.isNull())
        {
            return;
        }
        parent.removeChild(child);
    }
}


/** \brief Get a specific element from a DOM document.
 *
 * This function returns the first element (tag) with the specified name.
 * In most cases this will represent the tag defined in a layout XML file
 * although it is not required to be.
 *
 * Note that the function could return an element from the HTML or other
 * data found in that XML document if such tags are present as is.
 *
 * \exception snap_logic_exception
 * The logic exception is raised if the tag cannot be found. If the
 * must_exist parameter is set to false, then this exception is not raised.
 *
 * \param[in] doc  The document being searched for the specific element.
 * \param[in] name  The name of the element to retrieve.
 * \param[in] must_exist  If true and the element cannot be found, throw.
 *
 * \return The element found in the document.
 */
QDomElement get_element(QDomDocument& doc, QString const& name, bool must_exist)
{
    QDomNodeList elements(doc.elementsByTagName(name));
    if(elements.isEmpty())
    {
        // this should never happen because we do explicitly create this
        // <page> tag before calling this function
        if(must_exist)
        {
            throw snap_logic_exception(QString("<%1> tag not found in the body DOM").arg(name));
        }
        QDomElement null;
        return null;
    }

    QDomElement element(elements.at(0).toElement());
    if(must_exist && element.isNull())
    {
        // we just got a tag, this is really impossible!?
        throw snap_logic_exception(QString("<%1> tag not a DOM Element???").arg(name));
    }

    return element;
}


/** \brief Create the elements defined by path under parent.
 *
 * Starting from the node \p parent create each child as defined by
 * \p path. The process checks whether each child already exists, if
 * so then it doesn't re-create them (this is important to understand,
 * this function does not append new tags.)
 *
 * This is particularly useful when dealing with XML documents where you
 * have to add many tags at different locations and you do not know whether
 * there is already a tag there.
 *
 * \important
 * The function gets the FIRST of each tag it finds. So if you want to
 * create a child named \<foo\> and there are 3 tags named that way
 * under \p parent, then the first one will be used.
 *
 * \note
 * This function is similar to a get_element() with a path if all the
 * elements in \p path already exist.
 *
 * \note
 * The type of parent is set to QDomNode even though an element is required
 * because that way we do not force the caller to convert the node.
 *
 * \param[in,out] parent  The node from which children are added (i.e. body).
 * \param[in] path  The path representing the childre to create.
 */
QDomElement create_element(QDomNode parent, QString const& path)
{
#ifdef _DEBUG
    if(path.startsWith("/"))
    {
        throw snap_logic_exception(QString("path \"%1\" for create_element cannot start with a slash").arg(path));
    }
#endif

    if(parent.isNull())
    {
        // we cannot add anything starting from a null node
        // (TBD: should we err instead?)
        return parent.toElement();
    }

    QStringList p(path.split('/'));

    QDomDocument doc(parent.ownerDocument());

    int const max_children(p.size());
    for(int i(0); i < max_children; ++i)
    {
        QString const name(p[i]);
        if(name.isEmpty())
        {
            // skip in case of a "//" or starting "/"
            continue;
        }
        QDomNode child(parent.firstChildElement(name));
        if(child.isNull())
        {
            child = doc.createElement(name);
            parent.appendChild(child);
        }
        parent = child;
    }

    // the parent parameter becomes the child most item along
    // the course of this function
    return parent.toElement();
}



} // namespace snap_dom
} // namespace snap
// vim: ts=4 sw=4 et
