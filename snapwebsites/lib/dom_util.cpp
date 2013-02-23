// Snap Websites Server -- XML DOM utilities
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



#include "dom_util.h"

namespace snap
{
namespace dom_util
{

/** \brief Retrieve a tag, create it if it doesn't exist.
 *
 * This function searches for an element which is expected to exist and
 * have one instance. If not found, it creates it (by default, you may
 * prevent the creation by setting the \p create parameter to false.)
 *
 * The result is the tag set to the tag we've found if the function
 * returns true. When false is returned, tag is not modified.
 *
 * \param[in] tag_name  The name of the tag to search or create.
 * \param[in] element  The parent element of the tag to find or create.
 * \param[in] tag  The found tag (i.e. the answer of this function.)
 * \param[in] create  Whether the tag is created if it doesn't exist yet.
 *
 * \return true when the tag was found or created and can be returned.
 */
bool get_tag(const QString& tag_name, QDomElement& element, QDomElement& tag, bool create)
{
	QDomNodeList all_tags(element.elementsByTagName(tag_name));
	switch(all_tags.count())
	{
	case 0:
		if(create)
		{
			// missing, create a new one and retrieve it back out
			tag = element.ownerDocument().createElement(tag_name);
			element.appendChild(tag);
		}
		else
		{
			return false;
		}
		break;

	case 1:
		// we have it already!
		{
			QDomNode node(all_tags.at(0));
			if(!node.isElement())
			{
				return false;
			}
			tag = node.toElement();
		}
		break;

	default:
		// we've got a problem here
		return false;

	}

	return true;
}


}	// namespace dom_util
}	// namespace snap
// vim: ts=4 sw=4
