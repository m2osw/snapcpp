// Snap Websites Server -- filter
// Copyright (C) 2011-2012  Made to Order Software Corp.
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
	QString tags(" " + accepted_tags + " ");

	QString attr(" " + accepted_attributes + " ");
	const bool attr_refused = accepted_attributes[0] == '!';
	if(attr_refused) {
		// erase the '!' from the attr string
		attr.remove(1, 1);
	}

	// go through the entire tree
	QDomNode n = node.firstChild();
	while(!n.isNull()) {
		// determine the next pointer so we can delete this node
		QDomNode parent = n.parentNode();
		QDomNode next = n.firstChild();
		if(next.isNull()) {
			next = n.nextSibling();
			if(next.isNull()) {
				next = parent.nextSibling();
			}
		}

		// Is this node a tag? (i.e. an element)
		if(n.isElement()) {
			QDomElement e = n.toElement();
			const QString& name = e.tagName();
			if(tags.indexOf(" " + name.toLower() + " ") == -1) {
//qDebug() << "removing tag: [" << name << "] (" << tags << ")\n";
				// remove this tag, now there are two different type of
				// removal: complete removal (i.e. <script>) and removal
				// of the tag, but not the children (i.e. <b>)
				// the xmp and plaintext are browser extensions
				if(name != "script" && name != "style" && name != "textarea"
				&& name != "xmp" && name != "plaintext") {
					// in this case we can just remove the tag itself but keep
					// its children which we have to move up once
					QDomNode c = n.firstChild();
					while(!c.isNull()) {
						QDomNode next_sibling = c.nextSibling();
						n.removeChild(c);
						parent.insertBefore(c, n);
						c = next_sibling;
					}
				}
				parent.removeChild(n);
				next = parent.nextSibling();
				if(next.isNull()) {
					next = parent;
				}
			}
			else {
				// remove unwanted attributes too
				QDomNamedNodeMap attributes(n.attributes());
				int max = attributes.length();
				for(int i = 0; i < max; ++i) {
					QDomAttr a = attributes.item(i).toAttr();
					const QString& attr_name = a.name();
					if((attr.indexOf(" " + attr_name.toLower() + " ") == -1) ^ attr_refused) {
						e.removeAttribute(attr_name);
//qDebug() << "removing attribute: " << attr_name << " max: " << max << " size: " << attributes.length() << "\n";
					}
				}
			}
		}
		else if(n.isComment() || n.isProcessingInstruction()
		     || n.isNotation() || n.isEntity() || n.isDocument()
			 || n.isDocumentType() || n.isCDATASection()) {
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
			if(next.isNull()) {
				next = parent;
			}
		}
		// the rest is considered to be text
		n = next;
	}
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4
