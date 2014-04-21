#ifndef AS2JS_NODE_H
#define AS2JS_NODE_H
/* node.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

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

//#include    "as2js/as2js.h"
#include    "as2js/string.h"

#include    <controlled_vars/controlled_vars_limited_auto_init.h>

#include    <memory>
#include    <map>

namespace as2js
{



// NOTE: The attributes (Attrs) are defined in the second pass
//       whenever we transform the identifiers in actual attribute
//       flags. While creating the tree, the attributes are always
//       set to 0.

class Node
{
public:
    typedef std::shared_ptr<Node>   node_pointer_t;

    // the node type is often referenced as a token
    enum node_t
    {
        NODE_EOF                    = -1,       // when reading after the end of the file
        NODE_UNKNOWN                = 0,        // node still uninitialized

        // here are all the punctuation as themselves
        // (i.e. '<', '>', '=', '+', '-', etc.)
        NODE_ADD                    = '+',
        NODE_BITWISE_AND            = '&',
        NODE_BITWISE_NOT            = '~',
        NODE_ASSIGNMENT             = '=',
        NODE_BITWISE_OR             = '|',
        NODE_BITWISE_XOR            = '^',
        NODE_CLOSE_CURVLY_BRACKET   = '}',
        NODE_CLOSE_PARENTHESIS      = ')',
        NODE_CLOSE_SQUARE_BRACKET   = ']',
        NODE_COLON                  = ':',
        NODE_COMMA                  = ',',
        NODE_CONDITIONAL            = '?',
        NODE_DIVIDE                 = '/',
        NODE_GREATER                = '>',
        NODE_LESS                   = '<',
        NODE_LOGICAL_NOT            = '!',
        NODE_MODULO                 = '%',
        NODE_MULTIPLY               = '*',
        NODE_OPEN_CURVLY_BRACKET    = '{',
        NODE_OPEN_PARENTHESIS       = '(',
        NODE_OPEN_SQUARE_BRACKET    = '[',
        NODE_MEMBER                 = '.',
        NODE_SEMICOLON              = ';',
        NODE_SUBTRACT               = '-',

        // The following are composed tokens
        // (operators, keywords, strings, numbers...)
        NODE_other = 1000,

        NODE_ARRAY,
        NODE_ARRAY_LITERAL,
        NODE_AS,
        NODE_ASSIGNMENT_ADD,
        NODE_ASSIGNMENT_BITWISE_AND,
        NODE_ASSIGNMENT_BITWISE_OR,
        NODE_ASSIGNMENT_BITWISE_XOR,
        NODE_ASSIGNMENT_DIVIDE,
        NODE_ASSIGNMENT_LOGICAL_AND,
        NODE_ASSIGNMENT_LOGICAL_OR,
        NODE_ASSIGNMENT_LOGICAL_XOR,
        NODE_ASSIGNMENT_MAXIMUM,
        NODE_ASSIGNMENT_MINIMUM,
        NODE_ASSIGNMENT_MODULO,
        NODE_ASSIGNMENT_MULTIPLY,
        NODE_ASSIGNMENT_POWER,
        NODE_ASSIGNMENT_ROTATE_LEFT,
        NODE_ASSIGNMENT_ROTATE_RIGHT,
        NODE_ASSIGNMENT_SHIFT_LEFT,
        NODE_ASSIGNMENT_SHIFT_RIGHT,
        NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED,
        NODE_ASSIGNMENT_SUBTRACT,
        NODE_ATTRIBUTES,
        NODE_AUTO,
        NODE_BREAK,
        NODE_CALL,
        NODE_CASE,
        NODE_CATCH,
        NODE_CLASS,
        NODE_CONST,
        NODE_CONTINUE,
        NODE_DEBUGGER,
        NODE_DECREMENT,
        NODE_DEFAULT,
        NODE_DELETE,
        NODE_DIRECTIVE_LIST,
        NODE_DO,
        NODE_ELSE,
        NODE_EMPTY,
        NODE_ENTRY,
        NODE_ENUM,
        NODE_EQUAL,
        NODE_EXCLUDE,
        NODE_EXTENDS,
        NODE_FALSE,
        NODE_FINALLY,
        NODE_FLOAT64,
        NODE_FOR,
        NODE_FOR_IN,
        NODE_FUNCTION,
        NODE_GOTO,
        NODE_GREATER_EQUAL,
        NODE_IDENTIFIER,
        NODE_IF,
        NODE_IMPLEMENTS,
        NODE_IMPORT,
        NODE_IN,
        NODE_INCLUDE,
        NODE_INCREMENT,
        NODE_INSTANCEOF,
        NODE_INT64,
        NODE_INTERFACE,
        NODE_IS,
        NODE_LABEL,
        NODE_LESS_EQUAL,
        NODE_LIST,
        NODE_LOGICAL_AND,
        NODE_LOGICAL_OR,
        NODE_LOGICAL_XOR,
        NODE_MATCH,
        NODE_MAXIMUM,
        NODE_MINIMUM,
        NODE_NAME,
        NODE_NAMESPACE,
        NODE_NEW,
        NODE_NOT_EQUAL,
        NODE_NULL,
        NODE_OBJECT_LITERAL,
        NODE_PACKAGE,
        NODE_PARAM,
        NODE_PARAMETERS,
        NODE_PARAM_MATCH,
        NODE_POST_DECREMENT,
        NODE_POST_INCREMENT,
        NODE_POWER,
        NODE_PRIVATE,
        NODE_PROGRAM,
        NODE_PUBLIC,
        NODE_RANGE,
        NODE_REGULAR_EXPRESSION,
        NODE_REST,
        NODE_RETURN,
        NODE_ROOT,
        NODE_ROTATE_LEFT,
        NODE_ROTATE_RIGHT,
        NODE_SCOPE,
        NODE_SET,
        NODE_SHIFT_LEFT,
        NODE_SHIFT_RIGHT,
        NODE_SHIFT_RIGHT_UNSIGNED,
        NODE_STRICTLY_EQUAL,
        NODE_STRICTLY_NOT_EQUAL,
        NODE_STRING,
        NODE_SUPER,
        NODE_SWITCH,
        NODE_THIS,
        NODE_THROW,
        NODE_TRUE,
        NODE_TRY,
        NODE_TYPE,
        NODE_TYPEOF,
        NODE_UNDEFINED,
        NODE_USE,
        NODE_VAR,
        NODE_VARIABLE,
        NODE_VAR_ATTRIBUTES,
        NODE_VIDENTIFIER,
        NODE_VOID,
        NODE_WHILE,
        NODE_WITH,

        NODE_max,    // mark the limit

        // used to extract the node type from some integers
        // (used by the SWITCH statement at time of writing)
        // hopefully we can remove that later... at this point
        // I'm not too sure I see whether this really gets set
        NODE_MASK = 0x0FFFF
    };
    typedef controlled_vars::limited_auto_init<node_t, NODE_EOF, NODE_max, NODE_UNKNOWN> safe_node_t;

    // some nodes use flags and attributes, all of which are managed in
    // one bitset
    //
    // (Note that our Nodes are smart and make use of the function named
    // verify_flag_attribute() to make sure that this specific node can
    // indeed be given such flag or attribute)
    enum flag_attribute_t
    {
    //
    // the following is a list of all the possible flags in our system
    //
        // NODE_CATCH
        NODE_CATCH_FLAG_TYPED,

        // NODE_DIRECTIVE_LIST
        NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES,

        // NODE_FOR
        NODE_FOR_FLAG_FOREACH,

        // NODE_FUNCTION
        NODE_FUNCTION_FLAG_GETTER,
        NODE_FUNCTION_FLAG_SETTER,
        NODE_FUNCTION_FLAG_OUT,
        NODE_FUNCTION_FLAG_VOID,
        NODE_FUNCTION_FLAG_NEVER,
        NODE_FUNCTION_FLAG_NOPARAMS,
        NODE_FUNCTION_FLAG_OPERATOR,

        // NODE_IDENTIFIER, NODE_VIDENTIFIER, NODE_STRING
        NODE_IDENTIFIER_FLAG_WITH,
        NODE_IDENTIFIER_FLAG_TYPED,

        // NODE_IMPORT
        NODE_IMPORT_FLAG_IMPLEMENTS,

        // NODE_PACKAGE
        NODE_PACKAGE_FLAG_FOUND_LABELS,
        NODE_PACKAGE_FLAG_REFERENCED,

        // NODE_PARAM_MATCH
        NODE_PARAM_MATCH_FLAG_UNPROTOTYPED,

        // NODE_PARAMETERS
        NODE_PARAMETERS_FLAG_CONST,
        NODE_PARAMETERS_FLAG_IN,
        NODE_PARAMETERS_FLAG_OUT,
        NODE_PARAMETERS_FLAG_NAMED,
        NODE_PARAMETERS_FLAG_REST,
        NODE_PARAMETERS_FLAG_UNCHECKED,
        NODE_PARAMETERS_FLAG_UNPROTOTYPED,
        NODE_PARAMETERS_FLAG_REFERENCED,    // referenced from a parameter or a variable
        NODE_PARAMETERS_FLAG_PARAMREF,      // referenced from another parameter
        NODE_PARAMETERS_FLAG_CATCH,         // a parameter defined in a catch()

        // NODE_SWITCH
        NODE_SWITCH_FLAG_DEFAULT,           // we found a 'default:' label in that switch

        // NODE_VARIABLE (and NODE_VAR, NODE_PARAM)
        NODE_VAR_FLAG_CONST,
        NODE_VAR_FLAG_LOCAL,
        NODE_VAR_FLAG_MEMBER,
        NODE_VAR_FLAG_ATTRIBUTES,
        NODE_VAR_FLAG_ENUM,                 // there is a NODE_SET and it somehow needs to be copied
        NODE_VAR_FLAG_COMPILED,             // Expression() was called on the NODE_SET
        NODE_VAR_FLAG_INUSE,                // this variable was referenced
        NODE_VAR_FLAG_ATTRS,                // currently being read for attributes (to avoid loops)
        NODE_VAR_FLAG_DEFINED,              // was already parsed
        NODE_VAR_FLAG_DEFINING,             // currently defining, can't read
        NODE_VAR_FLAG_TOADD                 // to be added in the directive list

    //
    // the following is a list of all the possible attributes in our system
    //
        // member visibility
        NODE_ATTR_PUBLIC,
        NODE_ATTR_PRIVATE,
        NODE_ATTR_PROTECTED,
        NODE_ATTR_INTERNAL,

        // function member type
        NODE_ATTR_STATIC,
        NODE_ATTR_ABSTRACT,
        NODE_ATTR_VIRTUAL,
        NODE_ATTR_ARRAY,

        // function/variable is defined in your system (execution env.)
        // you won't find a body for these functions; the variables
        // will likely be read-only
        NODE_ATTR_INTRINSIC,

        // operator overload (function member)
        // Contructor -> another way to construct this type of objects
        NODE_ATTR_CONSTRUCTOR,

        // function & member constrains
        // CONST is not currently available as an attribute (see flags instead)
        //NODE_ATTR_CONST,
        NODE_ATTR_FINAL,
        NODE_ATTR_ENUMERABLE,

        // conditional compilation
        NODE_ATTR_TRUE,
        NODE_ATTR_FALSE,
        NODE_ATTR_UNUSED,                      // if definition is used, error!

        // class attribute (whether a class can be enlarged at run time)
        NODE_ATTR_DYNAMIC,

        // switch attributes
        NODE_ATTR_FOREACH,
        NODE_ATTR_NOBREAK,
        NODE_ATTR_AUTOBREAK,

        // The following is to make sure we never define the attributes more
        // than once.
        NODE_ATTR_DEFINED,

        // max used to know the number of entries and define out bitset
        NODE_FLAG_ATTRIBUTE_MAX
    };

    typedef bitset<NODE_FLAG_ATTRIBUTE_MAX - 1>     flag_attribute_set_t;

    class Data
    {
    public:
        void                Clear()
                            {
                                f_type = NODE_UNKNOWN;
                                f_int.set(0);
                                f_float.set(0.0);
                                f_str.clear();
                                f_user_data.clear();
                            }
        void                Display(FILE *out) const;
        char const *        GetTypeName() const;

        // basic conversions
        bool                    ToBoolean();
        bool                    ToNumber();
        bool                    ToString();

        // check flags
        bool                    get_flag(flag_t f) const;
        void                    set_flag(flag_t f, bool v);

    private:

    };


                        Node(node_t type /*, NodePtr *shell*/);
                        Node(Node const& source, Node const& parent);
                        ~Node();

    void                SetInputInfo(Input const *input);
    void                CopyInputInfo(Node const *input);
    long                GetPage() const { return f_page; }
    long                GetPageLine() const { return f_page_line; }
    long                GetParagraph() const { return f_paragraph; }
    long                GetLine() const { return f_line; }
    String const&       GetFilename() const { return f_filename; }

    Data const&         GetData() const
                        {
                            return f_data;
                        }
    void            SetData(const Data& data)
                {
                    f_data = data;
                }

    unsigned long        GetAttrs() const
                {
                    return f_attrs;
                }
    void            SetAttrs(unsigned long attrs)
                {
                    f_attrs = attrs;
                }
    bool            HasSideEffects() const;

    bool            IsLocked() const
                {
                    return f_lock != 0;
                }
    void            Lock()
                {
                    f_lock++;
                }
    void            Unlock()
                {
                    AS_ASSERT(f_lock > 0);
                    f_lock--;
                }
    void            SetOffset(int offset)
                {
                    f_offset = offset;
                }
    int            GetOffset() const
                {
                    return f_offset;
                }
    void            ReplaceWith(Node *node);
    void            DeleteChild(int index);
    void            AddChild(NodePtr& child);
    void            InsertChild(int index, NodePtr& child);
    void            SetChild(int index, NodePtr& child);
    int            GetChildCount() const
                {
                    return f_count;
                }
    NodePtr&        GetChild(int index) const;
    void            SetParent(Node *parent)
                {
                    if(parent != 0) {
                        AS_ASSERT(!f_parent.HasNode());
                        f_parent.SetNode(parent);
                    }
                    else {
                        f_parent.ClearNode();
                    }
                }
    NodePtr&        GetParent(void)
                {
                    return f_parent;
                }
    void            SetLink(NodePtr::link_t index, NodePtr& link)
                {
                    AS_ASSERT(index < NodePtr::LINK_max);
                    if(link.HasNode()) {
                        AS_ASSERT(!f_link[index].HasNode());
                        f_link[index].SetNode(link);
                    }
                    else {
                        f_link[index].ClearNode();
                    }
                }
    NodePtr&        GetLink(NodePtr::link_t index)
                {
                    AS_ASSERT(index < NodePtr::LINK_max);
                    return f_link[index];
                }

    void            AddVariable(NodePtr& variable);
    int            GetVariableCount(void) const
                {
                    return f_var_count;
                }
    NodePtr&        GetVariable(int index) const;

    void            AddLabel(NodePtr& label);
    int             GetLabelCount(void) const
                    {
                        return f_label_count;
                    }
    NodePtr&        GetLabel(int index) const;
    NodePtr&        FindLabel(String const& name) const;

    void            Display(FILE *out, int indent, NodePtr *parent, char c) const;

    static char const * OperatorToString(node_t op);
    static node_t       StringToOperator(String const& str);

private:
    void                            init();

    // verify that the specified flag correspond to the node type
    void                            verify_flag(flag_t f);

    controlled_vars::zint32_t       f_lock;

    controlled_vars::zint32_t       f_page;
    controlled_vars::zint32_t       f_page_line;
    controlled_vars::zint32_t       f_paragraph;
    controlled_vars::zint32_t       f_line;
    String                          f_filename;

    //Data            f_data;
    safe_node_t                     f_type;
    flag_attribute_set_t            f_flags_and_attributes;
    Int64                           f_int;
    Float64                         f_float;
    String                          f_str;
    std::vector<int>                f_user_data;  // TBD -- necessary?!

    NodePtr                         f_parent;
    int                             f_offset;        // offset (index) in parent array of children
    int                             f_count;
    int                             f_max;
    NodePtr *                       f_children;
    NodePtr                         f_link[NodePtr::LINK_max];
    int                             f_var_count;
    int                             f_var_max;
    NodePtr *                       f_variables;
    int                             f_label_count;
    int                             f_label_max;
    NodePtr *                       f_labels;
};






}
// namespace as2js

#endif
// #ifndef AS2JS_NODE_H

// vim: ts=4 sw=4 et
