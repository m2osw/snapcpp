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

//#include    "string.h"
#include    "int64.h"
#include    "float64.h"
#include    "stream.h"

#include    <controlled_vars/controlled_vars_limited_auto_init.h>

#include    <bitset>
#include    <map>
#include    <memory>
#include    <vector>

namespace as2js
{



// NOTE: The attributes (Attrs) are defined in the second pass
//       whenever we transform the identifiers in actual attribute
//       flags. While creating the tree, the attributes are always
//       set to 0.

class Node : public std::enable_shared_from_this<Node>
{
public:
    typedef std::shared_ptr<Node>               node_pointer_t;
    typedef std::map<String, node_pointer_t>    map_of_node_pointers_t;
    typedef std::vector<node_pointer_t>         vector_of_node_pointers_t;

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
        NODE_VAR_FLAG_TOADD,                // to be added in the directive list

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

    typedef std::bitset<NODE_FLAG_ATTRIBUTE_MAX - 1>     flag_attribute_set_t;

    enum link_t
    {
        LINK_INSTANCE = 0,
        LINK_TYPE,
        LINK_ATTRIBUTES,    // this is the list of identifiers

        LINK_max,

        LINK_GOTO_EXIT = LINK_INSTANCE,
        LINK_GOTO_ENTER = LINK_TYPE,

        LINK_end
    };

                            Node(node_t type);
                            Node(node_pointer_t const& source, node_pointer_t& parent);

    char const *            get_type_name() const;

    // basic conversions
    bool                    to_boolean();
    bool                    to_number();
    bool                    to_string();

    // check flags
    bool                    get_flag(flag_attribute_t f) const;
    void                    set_flag(flag_attribute_t f, bool v);

    void                    set_position(Position const& position);
    Position const&         get_position() const;

    bool                    has_side_effects() const;

    bool                    is_locked() const;
    void                    lock();
    void                    unlock();

    void                    set_offset(int32_t offset);
    int32_t                 get_offset() const;

    void                    set_parent(node_pointer_t parent = node_pointer_t(), int index = -1);
    node_pointer_t          get_parent() const;

    size_t                  get_children_size() const;
    //void                    replace_with(node_pointer_t& node); -- this is wrong...
    void                    delete_child(int index);
    void                    append_child(node_pointer_t& child);
    void                    insert_child(int index, node_pointer_t& child);
    void                    set_child(int index, node_pointer_t& child);
    node_pointer_t          get_child(int index) const;

    void                    set_link(link_t index, node_pointer_t& link);
    node_pointer_t          get_link(link_t index);

    void                    add_variable(node_pointer_t& variable);
    size_t                  get_variable_size() const;
    node_pointer_t          get_variable(int index) const;

    void                    add_label(node_pointer_t& label);
    size_t                  get_label_size() const;
    //node_pointer_t          get_label(size_t index) const; -- because of the map and it looks like we're not using this one anyway
    node_pointer_t          find_label(String const& name) const;

    static char const *     operator_to_string(node_t op);
    static node_t           string_to_operator(String const& str);

    void                    display(std::ostream& out, int indent, node_pointer_t const& parent, char c) const;

private:
    // verify that the specified flag correspond to the node type
    void                    verify_flag_attribute(flag_attribute_t f) const;
    void                    modifying() const;
    void                    display_data(std::ostream& out) const;

    // define the node type
    safe_node_t                     f_type;
    flag_attribute_set_t            f_flags_and_attributes;

    // whether this node is currently locked
    controlled_vars::zint32_t       f_lock;

    // location where the node was found (filename, line #, etc.)
    Position                        f_position;

    // data of this node
    Int64                           f_int;
    Float64                         f_float;
    String                          f_str;
    //std::vector<int>                f_user_data;  // TBD -- necessary?!

    // parent children node tree handling
    node_pointer_t                  f_parent;
    controlled_vars::zint32_t       f_offset;        // offset (index) in parent array of children -- set by compiler, should probably be removed...
    vector_of_node_pointers_t       f_children;

    // other connections between nodes
    vector_of_node_pointers_t       f_link;
    vector_of_node_pointers_t       f_variables;
    map_of_node_pointers_t          f_labels;
};

std::ostream& operator << (std::ostream& out, Node const& node);




}
// namespace as2js

#endif
// #ifndef AS2JS_NODE_H

// vim: ts=4 sw=4 et
