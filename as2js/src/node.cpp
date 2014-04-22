/* node.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

/*

Copyright (c) 2005-2009 Made to Order Software Corp.

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


#include "as2js/node.h"
#include "as2js/exceptions.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  NODE  ***********************************************************/
/**********************************************************************/
/**********************************************************************/


struct operator_to_string_t
{
    node_t          f_node;
    const char *    f_name;
};

#if defined(_DEBUG) || defined(DEBUG)
int g_file_line = __LINE__; // for errors
#endif
static operator_to_string_t const g_operator_to_string[] =
{
    // single character -- sorted in ASCII
    { NODE_LOGICAL_NOT,                     "!" },
    { NODE_MODULO,                          "%" },
    { NODE_BITWISE_AND,                     "&" },
    { NODE_MULTIPLY,                        "*" },
    { NODE_ADD,                             "+" },
    { NODE_SUBTRACT,                        "-" },
    { NODE_DIVIDE,                          "/" },
    { NODE_LESS,                            "<" },
    { NODE_ASSIGNMENT,                      "=" },
    { NODE_GREATER,                         ">" },
    { NODE_BITWISE_XOR,                     "^" },
    { NODE_BITWISE_OR,                      "|" },
    { NODE_BITWISE_NOT,                     "~" },

    // two or more characters transformed to an enum only
    { NODE_ASSIGNMENT_ADD,                  "+=" },
    { NODE_ASSIGNMENT_BITWISE_AND,          "&=" },
    { NODE_ASSIGNMENT_BITWISE_OR,           "|=" },
    { NODE_ASSIGNMENT_BITWISE_XOR,          "^=" },
    { NODE_ASSIGNMENT_DIVIDE,               "/=" },
    { NODE_ASSIGNMENT_LOGICAL_AND,          "&&=" },
    { NODE_ASSIGNMENT_LOGICAL_OR,           "||=" },
    { NODE_ASSIGNMENT_LOGICAL_XOR,          "^^=" },
    { NODE_ASSIGNMENT_MAXIMUM,              "?>=" },
    { NODE_ASSIGNMENT_MINIMUM,              "?<=" },
    { NODE_ASSIGNMENT_MODULO,               "%=" },
    { NODE_ASSIGNMENT_MULTIPLY,             "*=" },
    { NODE_ASSIGNMENT_POWER,                "**=" },
    { NODE_ASSIGNMENT_ROTATE_LEFT,          "!<=" },
    { NODE_ASSIGNMENT_ROTATE_RIGHT,         "!>=" },
    { NODE_ASSIGNMENT_SHIFT_LEFT,           "<<=" },
    { NODE_ASSIGNMENT_SHIFT_RIGHT,          ">>=" },
    { NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED, ">>>=" },
    { NODE_ASSIGNMENT_SUBTRACT,             "-=" },
    { NODE_CALL,                            "()" },
    { NODE_DECREMENT,                       "--" },
    { NODE_EQUAL,                           "==" },
    { NODE_GREATER_EQUAL,                   ">=" },
    { NODE_INCREMENT,                       "++" },
    { NODE_LESS_EQUAL,                      "<=" },
    { NODE_LOGICAL_AND,                     "&&" },
    { NODE_LOGICAL_OR,                      "||" },
    { NODE_LOGICAL_XOR,                     "^^" },
    { NODE_MATCH,                           "~=" },
    { NODE_MAXIMUM,                         "?>" },
    { NODE_MINIMUM,                         "?<" },
    { NODE_NOT_EQUAL,                       "!=" },
    { NODE_POST_DECREMENT,                  "--" },
    { NODE_POST_INCREMENT,                  "++" },
    { NODE_POWER,                           "**" },
    { NODE_ROTATE_LEFT,                     "!<" },
    { NODE_ROTATE_RIGHT,                    "!>" },
    { NODE_SHIFT_LEFT,                      "<<" },
    { NODE_SHIFT_RIGHT,                     ">>" },
    { NODE_SHIFT_RIGHT_UNSIGNED,            ">>>" },
    { NODE_STRICTLY_EQUAL,                  "===" },
    { NODE_STRICTLY_NOT_EQUAL,              "!==" }

// the following doesn't make it in user redefinable operators yet
    //{ NODE_CONDITIONAL,                   "" },
    //{ NODE_DELETE,                        "" },
    //{ NODE_FOR_IN,                        "" },
    //{ NODE_IN,                            "" },
    //{ NODE_INSTANCEOF,                    "" },
    //{ NODE_IS,                            "" },
    //{ NODE_LIST,                          "" },
    //{ NODE_NEW,                           "" },
    //{ NODE_RANGE,                         "" },
    //{ NODE_SCOPE,                         "" },
};

#define    operator_to_string_size    (sizeof(g_operator_to_string) / sizeof(g_operator_to_string[0]))





Node::Node(node_t type)
{
    Init();
    f_data.f_type = type;
}


Node::Node(const Node& source, const Node& parent)
{
#if defined(_DEBUG) || defined(DEBUG)
    switch(source.f_data.f_type)
    {
    case NODE_STRING:
    case NODE_INT64:
    case NODE_FLOAT64:
    case NODE_TRUE:
    case NODE_FALSE:
    case NODE_NULL:
    case NODE_UNDEFINED:
    case NODE_REGULAR_EXPRESSION:
        break;

    default:
        // ERROR: only constants can be cloned at this
        //      time (otherwise we'd need to clone all the
        //      children node too!!!)
        AS_ASSERT(0);
        break;

    }
#endif

    Init();

    f_page = source.f_page;
    f_page_line = source.f_page_line;
    f_paragraph = source.f_paragraph;
    f_line = source.f_line;
    f_filename = source.f_filename;
    f_data = source.f_data;
    f_parent.SetNode(&const_cast<Node&>(parent));
    for(int idx = 0; idx < NodePtr::LINK_max; ++idx)
    {
        f_link[idx] = source.f_link[idx];
    }

    // constants should not have variables nor labels
}


Node::~Node()
{
    delete [] f_variables;
    delete [] f_labels;
}


void Node::init()
{
    //f_lock = 0; -- auto-init
    f_page = 0;
    f_page_line = 0;
    f_paragraph = 0;
    f_line = 0;
    //f_filename -- auto-init
    //f_data -- auto-init
    f_attrs = 0;
    //f_parent -- auto-init
    f_offset = 0x7FFFFFFF;
    //f_children -- auto-init
    //f_link[] -- auto-init
    f_var_count = 0;
    f_var_max = 0;
    f_variables = 0;
    f_label_count = 0;
    f_label_max = 0;
    f_labels = 0;
}


char const *Node::OperatorToString(node_t op)
{
#if defined(_DEBUG) || defined(DEBUG)
    {
        // make sure that the node types are properly sorted
        static bool checked = false;
        if(!checked)
        {
            // check only once
            checked = true;
            for(size_t idx = 1; idx < operator_to_string_size; ++idx)
            {
                if(g_operator_to_string[idx].f_node <= g_operator_to_string[idx - 1].f_node)
                {
                    std::cerr << "INTERNAL ERROR at offset " << idx
                              << " (line ~#" << (idx + g_file_line + 5)
                              << ", node type " << g_operator_to_string[idx].f_node
                              << " vs. " << g_operator_to_string[idx - 1].f_node
                              << "): the g_operator_to_string table isn't sorted properly. We can't binary search it."
                              << std::endl;
                    throw internal_error("INTERNAL ERROR: node types not properly sorted, cannot properly search for operators using a binary search.");
                }
            }
        }
    }
#endif

    size_t i, j, p;
    int    r;

    i = 0;
    j = operator_to_string_size;
    while(i < j)
    {
        p = (j - i) / 2 + i;
        r = g_operator_to_string[p].f_node - op;
        if(r == 0)
        {
            return g_operator_to_string[p].f_name;
        }
        if(r < 0)
        {
            i = p + 1;
        }
        else
        {
            j = p;
        }
    }

    return 0;
}



node_t Node::StringToOperator(String const& str)
{
    operator_to_string_t const  *lst;
    int                         idx;

    lst = g_operator_to_string;
    idx = operator_to_string_count;
    while(idx > 0)
    {
        idx--;
        if(str == lst->f_name)
        {
            return lst->f_node;
        }
    }

    return NODE_UNKNOWN;
}



void Node::SetInputInfo(Input const *input)
{
    if(input == 0)
    {
        return;
    }

    // copy the input info so we can generate an error on
    // any node at a later time (especially in the compiler)
    f_page      = input->Page();
    f_page_line = input->PageLine();
    f_paragraph = input->Paragraph();
    f_line      = input->Line();
    f_filename  = input->GetFilename();
}


void Node::CopyInputInfo(const Node *node)
{
    f_page      = node->f_page;
    f_page_line = node->f_page_line;
    f_paragraph = node->f_paragraph;
    f_line      = node->f_line;
    f_filename  = node->f_filename;
}


void Node::ReplaceWith(Node *node)
{
    modifying();
    AS_ASSERT(!node->f_parent.HasNode());

    node->f_parent = f_parent;
    f_parent.ClearNode();
}


void Node::DeleteChild(int index)
{
    modifying();

    // disconnect the parent (XXX necessary?)
    f_children[index].set_parent(0);

    f_children.erase(f_children.begin() + index);
}


void Node::append_child(node_pointer_t& child)
{
    modifying();

    f_children.push_back(child);
    child.set_parent(this);
}


void Node::insert_child(int index, node_pointer_t& child)
{
    modifying();

    f_children.insert(f_children.begin() + index, child);
    child.set_parent(this);
}


void Node::modifying() const
{
    if(f_lock != 0)
    {
        throw locked_node("trying to modify a locked node.");
    }
}


void Node::set_child(int index, node_pointer_t& child)
{
    modifying();

    if(f_children[index].HasNode())
    {
        f_children[index].SetParent(0);
    }
    f_children[index] = child;
    child.SetParent(this);
}


NodePtr& Node::get_child(int index) const
{
    return f_children[index];
}


bool Node::has_side_Effects() const
{
    //
    // Well... I'm wondering if we can really
    // trust this current version.
    //
    // Problem I:
    //    some identifiers can be getters and
    //    they can have side effects; though
    //    a getter should be considered constant
    //    toward the object being read and thus
    //    it should be fine in 99% of cases
    //    [imagine a serial number generator...]
    //
    // Problem II:
    //    some operators may not have been
    //     compiled yet and they could have
    //     side effects too; now this is much
    //     less likely a problem because then
    //     the programmer is most certainly
    //     creating a really weird program
    //     with all sorts of side effects that
    //     he wants no one else to know about,
    //     etc. etc. etc.
    //
    // Problem III:
    //    Note that we don't memorize whether
    //    a node has side effects because its
    //    children may change and then the side
    //    effects may disappear
    //

    switch(f_data.f_type) {
    case NODE_ASSIGNMENT:
    case NODE_ASSIGNMENT_ADD:
    case NODE_ASSIGNMENT_BITWISE_AND:
    case NODE_ASSIGNMENT_BITWISE_OR:
    case NODE_ASSIGNMENT_BITWISE_XOR:
    case NODE_ASSIGNMENT_DIVIDE:
    case NODE_ASSIGNMENT_LOGICAL_AND:
    case NODE_ASSIGNMENT_LOGICAL_OR:
    case NODE_ASSIGNMENT_LOGICAL_XOR:
    case NODE_ASSIGNMENT_MAXIMUM:
    case NODE_ASSIGNMENT_MINIMUM:
    case NODE_ASSIGNMENT_MODULO:
    case NODE_ASSIGNMENT_MULTIPLY:
    case NODE_ASSIGNMENT_POWER:
    case NODE_ASSIGNMENT_ROTATE_LEFT:
    case NODE_ASSIGNMENT_ROTATE_RIGHT:
    case NODE_ASSIGNMENT_SHIFT_LEFT:
    case NODE_ASSIGNMENT_SHIFT_RIGHT:
    case NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case NODE_ASSIGNMENT_SUBTRACT:
    case NODE_CALL:
    case NODE_DECREMENT:
    case NODE_DELETE:
    case NODE_INCREMENT:
    case NODE_NEW:
    case NODE_POST_DECREMENT:
    case NODE_POST_INCREMENT:
        return true;

    //case NODE_IDENTIFIER:
    //
    // TODO: Test whether this is a reference to a getter
    //       function (needs to be compiled already...)
    //    
    //    break;

    default:
        break;

    }

    for(size_t idx = 0; idx < f_children.size(); ++idx)
    {
        if(f_children[idx] && f_children[idx].has_side_effects())
        {
            return true;
        }
    }

    return false;
}



void Node::AddVariable(NodePtr& variable)
{
#if defined(_DEBUG) || defined(DEBUG)
    Data& data = variable.GetData();
    AS_ASSERT(data.f_type == NODE_VARIABLE);
#endif
    if(f_var_max == 0)
    {
        f_var_max = 10;
        f_variables = new NodePtr[10];
    }
    if(f_var_count >= f_var_max)
    {
        f_var_max += 10;
        NodePtr *extra = new NodePtr[f_var_max];
        for(int i = 0; i < f_var_count; ++i) {
            extra[i] = f_variables[i];
        }
        delete [] f_variables;
        f_variables = extra;
    }

    f_variables[f_var_count] = variable;
    ++f_var_count;
}




NodePtr& Node::GetVariable(int index) const
{
    AS_ASSERT(index < f_var_max);
    return f_variables[index];
}




void Node::AddLabel(NodePtr& label)
{
#if defined(_DEBUG) || defined(DEBUG)
    Data& data = label.GetData();
    AS_ASSERT(data.f_type == NODE_LABEL);
#endif
    if(f_label_max == 0) {
        f_label_max = 5;
        f_labels = new NodePtr[5];
    }
    if(f_label_count >= f_label_max) {
        f_label_max += 5;
        NodePtr *extra = new NodePtr[f_label_max];
        for(int i = 0; i < f_label_count; ++i) {
            extra[i] = f_labels[i];
        }
        delete [] f_labels;
        f_labels = extra;
    }

    // we should sort those
    f_labels[f_label_count] = label;
    ++f_label_count;
}




NodePtr& Node::GetLabel(int index) const
{
    AS_ASSERT(index < f_label_max);
    return f_labels[index];
}



NodePtr& Node::FindLabel(const String& name) const
{
    static NodePtr not_found;

    AS_ASSERT(!not_found.HasNode());

    // once sorted, convert to a binary search
    for(int idx = 0; idx < f_label_count; ++idx) {
        Data& data = f_labels[idx].GetData();
        if(data.f_str == name) {
            return f_labels[idx];
        }
    }

    return not_found;
}











/**********************************************************************/
/**********************************************************************/
/***  NODE DISPLAY  ***************************************************/
/**********************************************************************/
/**********************************************************************/

/* This function is mainly for debug purposes.
 * It is used by the asc tool to display the final
 * tree.
 */
void Node::Display(FILE *out, int indent, NodePtr *parent, char c) const
{
    fprintf(out, "%08lX:%02d%c %*s", (long) this, indent, c, indent, "");
    if(parent != 0 && !f_parent.SameAs(*parent)) {
        fprintf(out, ">>WRONG PARENT: ");
        f_parent.DisplayPtr(out);
        fprintf(out, "<< ");
    }
    f_data.Display(out);
    bool first = true;
    for(int lnk = 0; lnk < NodePtr::LINK_max; ++lnk) {
        if(f_link[lnk].HasNode()) {
            if(first) {
                first = false;
                fprintf(out, " Lnk:");
            }
            fprintf(out, " [%d]=", lnk);
            f_link[lnk].DisplayPtr(out);
        }
    }
    unsigned long attrs = GetAttrs();
    if(attrs != 0) {
        fprintf(out, " Attrs:");
        if((attrs & NODE_ATTR_PUBLIC) != 0) {
            attrs &= ~NODE_ATTR_PUBLIC;
            fprintf(out, " PUBLIC");
        }
        if((attrs & NODE_ATTR_PRIVATE) != 0) {
            attrs &= ~NODE_ATTR_PRIVATE;
            fprintf(out, " PRIVATE");
        }
        if((attrs & NODE_ATTR_PROTECTED) != 0) {
            attrs &= ~NODE_ATTR_PROTECTED;
            fprintf(out, " PROTECTED");
        }
        if((attrs & NODE_ATTR_STATIC) != 0) {
            attrs &= ~NODE_ATTR_STATIC;
            fprintf(out, " STATIC");
        }
        if((attrs & NODE_ATTR_ABSTRACT) != 0) {
            attrs &= ~NODE_ATTR_ABSTRACT;
            fprintf(out, " ABSTRACT");
        }
        if((attrs & NODE_ATTR_VIRTUAL) != 0) {
            attrs &= ~NODE_ATTR_VIRTUAL;
            fprintf(out, " VIRTUAL");
        }
        if((attrs & NODE_ATTR_INTERNAL) != 0) {
            attrs &= ~NODE_ATTR_INTERNAL;
            fprintf(out, " INTERNAL");
        }
        if((attrs & NODE_ATTR_INTRINSIC) != 0) {
            attrs &= ~NODE_ATTR_INTRINSIC;
            fprintf(out, " INTRINSIC");
        }
        if((attrs & NODE_ATTR_CONSTRUCTOR) != 0) {
            attrs &= ~NODE_ATTR_CONSTRUCTOR;
            fprintf(out, " CONSTRUCTOR");
        }
        if((attrs & NODE_ATTR_FINAL) != 0) {
            attrs &= ~NODE_ATTR_FINAL;
            fprintf(out, " FINAL");
        }
        if((attrs & NODE_ATTR_ENUMERABLE) != 0) {
            attrs &= ~NODE_ATTR_ENUMERABLE;
            fprintf(out, " ENUMERABLE");
        }
        if((attrs & NODE_ATTR_TRUE) != 0) {
            attrs &= ~NODE_ATTR_TRUE;
            fprintf(out, " TRUE");
        }
        if((attrs & NODE_ATTR_FALSE) != 0) {
            attrs &= ~NODE_ATTR_FALSE;
            fprintf(out, " FALSE");
        }
        if((attrs & NODE_ATTR_UNUSED) != 0) {
            attrs &= ~NODE_ATTR_UNUSED;
            fprintf(out, " UNUSED");
        }
        if((attrs & NODE_ATTR_DYNAMIC) != 0) {
            attrs &= ~NODE_ATTR_DYNAMIC;
            fprintf(out, " DYNAMIC");
        }
        if((attrs & NODE_ATTR_FOREACH) != 0) {
            attrs &= ~NODE_ATTR_FOREACH;
            fprintf(out, " FOREACH");
        }
        if((attrs & NODE_ATTR_NOBREAK) != 0) {
            attrs &= ~NODE_ATTR_NOBREAK;
            fprintf(out, " NOBREAK");
        }
        if((attrs & NODE_ATTR_AUTOBREAK) != 0) {
            attrs &= ~NODE_ATTR_AUTOBREAK;
            fprintf(out, " AUTOBREAK");
        }
        if((attrs & NODE_ATTR_DEFINED) != 0) {
            attrs &= ~NODE_ATTR_DEFINED;
            fprintf(out, " DEFINED");
        }

        if(attrs != 0) {
            fprintf(out, " <unamed flags: %08lX>", attrs);
        }
    }

#if 1
// to be very verbose... print also the filename and line info
    char n[256];
    size_t sz = sizeof(n);
    f_filename.ToUTF8(n, sz);
    fprintf(out, " %s:%ld", n, f_line);
#endif

    fprintf(out, "\n");
    NodePtr me;
    me.SetNode(this);
    for(size_t idx = 0; idx < f_children.size(); ++idx)
    {
        f_children[idx].Display(out, indent + 1, &me, '-');
    }
    for(int idx = 0; idx < f_var_count; ++idx)
    {
        f_variables[idx].Display(out, indent + 1, 0, '=');
    }
    for(int idx = 0; idx < f_label_count; ++idx)
    {
        f_labels[idx].Display(out, indent + 1, 0, ':');
    }
}







}
// namespace as2js

// vim: ts=4 sw=4 et
