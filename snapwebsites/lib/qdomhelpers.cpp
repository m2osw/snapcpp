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
void insert_html_string_to_xml_doc(QDomElement& child, QString const& xml)
{
    // parsing the XML can be slow, try to avoid that if possible
    if(xml.contains('<'))
    {
        QDomDocument xml_doc("wrapper");
        xml_doc.setContent("<wrapper>" + xml + "</wrapper>", true, nullptr, nullptr, nullptr);

        // copy the result in a fragment of our document
        QDomDocumentFragment frag(child.ownerDocument().createDocumentFragment());
        frag.appendChild(child.ownerDocument().importNode(xml_doc.documentElement(), true));

        // copy the fragment nodes at the right place
        QDomNodeList children(frag.firstChild().childNodes());
        QDomNode previous(children.at(0));
        child.appendChild(children.at(0));
        while(!children.isEmpty())
        {
            QDomNode l(children.at(0));
            child.insertAfter(children.at(0), previous);
            previous = l;
        }
    }
    else
    {
        QDomText text(child.ownerDocument().createTextNode(xml));
        child.appendChild(text);
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




} // namespace snap_dom
} // namespace snap
// vim: ts=4 sw=4 et
