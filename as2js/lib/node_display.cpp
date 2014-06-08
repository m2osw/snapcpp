/* node_display.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

http://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include    "as2js/node.h"

#include    <controlled_vars/controlled_vars_auto_enum_init.h>

#include    <iomanip>


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  NODE DISPLAY  ***************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Display a node.
 *
 * This function prints a node in the \p out stream.
 *
 * The function is smart enough to recognize the different type of nodes
 * and thus know what is saved in them and knows how to display all of
 * that information.
 *
 * This is only to display a node in a technical way. It does not attempt
 * to display things in JavaScript or any other language.
 *
 * \param[in] out  The output stream where the node is displayed.
 */
void Node::display_data(std::ostream& out) const
{
    struct sub_function
    {
        static void display_str(std::ostream& out, String str)
        {
            out << ": '";
            for(as_char_t const *s(str.c_str()); *s != '\0'; ++s)
            {
                if(*s < 0x7f)
                {
                    if(*s == '\'')
                    {
                        out << "\\'";
                    }
                    else
                    {
                        out << static_cast<char>(*s);
                    }
                }
                else
                {
                    out << "\\U+" << std::hex << *s << std::dec;
                }
            }
            out << "'";
        }
    };

    // WARNING: somehow g++ views the node_t type as a Node type and thus
    //          it recursively calls this function until the stack is full
    out << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<node_t>(f_type))
        << std::setfill('\0') << ": " << get_type_name();
    if(static_cast<int>(static_cast<node_t>(f_type)) > ' ' && static_cast<int>(static_cast<node_t>(f_type)) < 0x7F)
    {
        out << " = '" << static_cast<char>(static_cast<node_t>(f_type)) << "'";
    }

    switch(f_type)
    {
    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_VIDENTIFIER:
    case node_t::NODE_STRING:
    case node_t::NODE_GOTO:
    case node_t::NODE_LABEL:
    case node_t::NODE_IMPORT:
    case node_t::NODE_CLASS:
    case node_t::NODE_INTERFACE:
    case node_t::NODE_ENUM:
        sub_function::display_str(out, f_str);
        break;

    case node_t::NODE_PACKAGE:
        sub_function::display_str(out, f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS)])
        {
            out << " FOUND-LABELS";
        }
        break;

    case node_t::NODE_INT64:
        out << ": " << f_int.get() << ", 0x" << std::hex << std::setw(16) << std::setfill('0') << f_int.get() << std::dec << std::setw(0) << std::setfill('\0');
        break;

    case node_t::NODE_FLOAT64:
        out << ": " << f_float.get();
        break;

    case node_t::NODE_FUNCTION:
        sub_function::display_str(out, f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_GETTER)])
        {
            out << " GETTER";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_SETTER)])
        {
            out << " SETTER";
        }
        break;

    case node_t::NODE_PARAM:
        sub_function::display_str(out, f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_CONST)])
        {
            out << " CONST";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_IN)])
        {
            out << " IN";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_OUT)])
        {
            out << " OUT";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_NAMED)])
        {
            out << " NAMED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_REST)])
        {
            out << " REST";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_UNCHECKED)])
        {
            out << " UNCHECKED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_UNPROTOTYPED)])
        {
            out << " UNPROTOTYPED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_REFERENCED)])
        {
            out << " REFERENCED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAMETERS_FLAG_PARAMREF)])
        {
            out << " PARAMREF";
        }
        break;

    case node_t::NODE_PARAM_MATCH:
        out << ":";
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED)])
        {
            out << " UNPROTOTYPED";
        }
        break;

    case node_t::NODE_VARIABLE:
    case node_t::NODE_VAR_ATTRIBUTES:
        sub_function::display_str(out, f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_CONST)])
        {
            out << " CONST";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_LOCAL)])
        {
            out << " LOCAL";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_MEMBER)])
        {
            out << " MEMBER";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_ATTRIBUTES)])
        {
            out << " ATTRIBUTES";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_ENUM)])
        {
            out << " ENUM";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_COMPILED)])
        {
            out << " COMPILED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_INUSE)])
        {
            out << " INUSE";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_ATTRS)])
        {
            out << " ATTRS";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_DEFINED)])
        {
            out << " DEFINED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_DEFINING)])
        {
            out << " DEFINING";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VAR_FLAG_TOADD)])
        {
            out << " TOADD";
        }
        break;

    default:
        break;

    }
}


/** \brief Display a node tree.
 *
 * This function displays this node, its children, its children's children,
 * etc. until all the nodes in the tree were displayed.
 *
 * Note that the function knows about the node links, variables, and labels
 * which also get displayed.
 *
 * Because the tree cannot generate loops (the set_parent() function
 * prevents such), we do not have anything that would break the
 * recursivity of the function.
 *
 * The character used to start the string (\p c) changes depending on what
 * we are showing to the user. That way we know whether it is the root (.),
 * a child (-), a variable (=), or a label (:).
 *
 * \todo
 * We probably want to remove the \p parent parameter. I had it here because
 * I wanted to verify it as the old code had many problems with the tree
 * which would break while we were optimizing or compiling the code. The
 * set_parent() function is now working fine so the parent is not useful.
 *
 * \param[in,out] out  The output stream.
 * \param[in] indent  The current indentation. We start with 2.
 * \param[in] c  A character to start each line of output.
 */
void Node::display(std::ostream& out, int indent, char c) const
{
    // this pointer
    out << this << ": " << std::setfill('\0') << std::setw(2) << indent << std::setfill(' ') << c << std::setw(indent) << "";

    // display node data (integer, string, float, etc.)
    display_data(out);

    // display information about the links
    bool first = true;
    for(size_t lnk(0); lnk < f_link.size(); ++lnk)
    {
        if(f_link[lnk])
        {
            if(first)
            {
                first = false;
                out << " Lnk:";
            }
            out << " [" << lnk << "]=" << f_link[lnk].get();
        }
    }

    // display the different attributes if any
    struct display_attributes
    {
        display_attributes(std::ostream& out, attribute_set_t attrs)
            : f_out(out)
            , f_attributes(attrs)
        {
            display_attribute(attribute_t::NODE_ATTR_PUBLIC,         "PUBLIC"        );
            display_attribute(attribute_t::NODE_ATTR_PRIVATE,        "PRIVATE"       );
            display_attribute(attribute_t::NODE_ATTR_PROTECTED,      "PROTECTED"     );
            display_attribute(attribute_t::NODE_ATTR_STATIC,         "STATIC"        );
            display_attribute(attribute_t::NODE_ATTR_ABSTRACT,       "ABSTRACT"      );
            display_attribute(attribute_t::NODE_ATTR_VIRTUAL,        "VIRTUAL"       );
            display_attribute(attribute_t::NODE_ATTR_INTERNAL,       "INTERNAL"      );
            display_attribute(attribute_t::NODE_ATTR_INTRINSIC,      "INTRINSIC"     );
            display_attribute(attribute_t::NODE_ATTR_DEPRECATED,     "DEPRECATED"    );
            display_attribute(attribute_t::NODE_ATTR_UNSAFE,         "UNSAFE"        );
            display_attribute(attribute_t::NODE_ATTR_CONSTRUCTOR,    "CONSTRUCTOR"   );
            display_attribute(attribute_t::NODE_ATTR_FINAL,          "FINAL"         );
            display_attribute(attribute_t::NODE_ATTR_ENUMERABLE,     "ENUMERABLE"    );
            display_attribute(attribute_t::NODE_ATTR_TRUE,           "TRUE"          );
            display_attribute(attribute_t::NODE_ATTR_FALSE,          "FALSE"         );
            display_attribute(attribute_t::NODE_ATTR_UNUSED,         "UNUSED"        );
            display_attribute(attribute_t::NODE_ATTR_DYNAMIC,        "DYNAMIC"       );
            display_attribute(attribute_t::NODE_ATTR_FOREACH,        "FOREACH"       );
            display_attribute(attribute_t::NODE_ATTR_NOBREAK,        "NOBREAK"       );
            display_attribute(attribute_t::NODE_ATTR_AUTOBREAK,      "AUTOBREAK"     );
            display_attribute(attribute_t::NODE_ATTR_DEFINED,        "DEFINED"       );
        }

        void display_attribute(attribute_t a, char const *n)
        {
            if(f_attributes[static_cast<size_t>(a)])
            {
                f_out << " " << n;
            }
        }

        std::ostream&               f_out;
        controlled_vars::fbool_t    f_first;
        attribute_set_t             f_attributes;
    } display_attr(out, f_attributes);

    // end the line with our position
    out << " (" << f_position << ")" << std::endl;

    // now print children
    for(size_t idx(0); idx < f_children.size(); ++idx)
    {
        f_children[idx]->display(out, indent + 1, '-');
    }

    // now print variables
    for(size_t idx(0); idx < f_variables.size(); ++idx)
    {
        f_variables[idx]->display(out, indent + 1, '=');
    }

    // now print labels
    for(map_of_pointers_t::const_iterator it(f_labels.begin());
                                          it != f_labels.end();
                                          ++it)
    {
        it->second->display(out, indent + 1, ':');
    }
}


/** \brief Send a node to the specified output stream.
 *
 * This function prints a node to the output stream. The printing is very
 * technical and mainly used to debug the node tree while parsing,
 * compiling, optimizing, and generating the final output.
 *
 * \param[in,out] out  The output stream.
 * \param[in] node  The node to print in the output stream.
 *
 * \return A reference to the output stream.
 */
std::ostream& operator << (std::ostream& out, Node const& node)
{
    node.display(out, 2, '.');
    return out;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
