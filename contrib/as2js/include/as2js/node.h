#ifndef AS2JS_NODE_H
#define AS2JS_NODE_H
/* node.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

/*

Copyright (c) 2005-2017 Made to Order Software Corp.

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

#include    "string.h"
#include    "int64.h"
#include    "float64.h"
#include    "position.h"

#include    <limits>
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
    typedef std::shared_ptr<Node>               pointer_t;
    typedef std::weak_ptr<Node>                 weak_pointer_t;
    typedef std::map<String, weak_pointer_t>    map_of_weak_pointers_t;
    typedef std::vector<pointer_t>              vector_of_pointers_t;
    typedef std::vector<weak_pointer_t>         vector_of_weak_pointers_t;

    // member related depth parameter
    typedef ssize_t             depth_t;

    static depth_t const        MATCH_NOT_FOUND = 0;
    static depth_t const        MATCH_HIGHEST_DEPTH = 1;
    static depth_t const        MATCH_LOWEST_DEPTH = std::numeric_limits<int>::max() / 2;

    // the node type is often referenced as a token
    enum class node_t
    {
        NODE_EOF                    = -1,       // when reading after the end of the file
        NODE_UNKNOWN                = 0,        // node still uninitialized

        // here are all the punctuation as themselves
        // (i.e. '<', '>', '=', '+', '-', etc.)
        NODE_ADD                    = '+',      // 0x2B
        NODE_ASSIGNMENT             = '=',      // 0x3D
        NODE_BITWISE_AND            = '&',      // 0x26
        NODE_BITWISE_NOT            = '~',      // 0x7E
        NODE_BITWISE_OR             = '|',      // 0x7C
        NODE_BITWISE_XOR            = '^',      // 0x5E
        NODE_CLOSE_CURVLY_BRACKET   = '}',      // 0x7D
        NODE_CLOSE_PARENTHESIS      = ')',      // 0x29
        NODE_CLOSE_SQUARE_BRACKET   = ']',      // 0x5D
        NODE_COLON                  = ':',      // 0x3A
        NODE_COMMA                  = ',',      // 0x2C
        NODE_CONDITIONAL            = '?',      // 0x3F
        NODE_DIVIDE                 = '/',      // 0x2F
        NODE_GREATER                = '>',      // 0x3E
        NODE_LESS                   = '<',      // 0x3C
        NODE_LOGICAL_NOT            = '!',      // 0x21
        NODE_MODULO                 = '%',      // 0x25
        NODE_MULTIPLY               = '*',      // 0x2A
        NODE_OPEN_CURVLY_BRACKET    = '{',      // 0x7B
        NODE_OPEN_PARENTHESIS       = '(',      // 0x28
        NODE_OPEN_SQUARE_BRACKET    = '[',      // 0x5B
        NODE_MEMBER                 = '.',      // 0x2E
        NODE_SEMICOLON              = ';',      // 0x3B
        NODE_SUBTRACT               = '-',      // 0x2D

        // The following are composed tokens
        // (operators, keywords, strings, numbers...)
        NODE_other = 1000,

        NODE_ABSTRACT,
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
        NODE_BOOLEAN,
        NODE_BREAK,
        NODE_BYTE,
        NODE_CALL,
        NODE_CASE,
        NODE_CATCH,
        NODE_CHAR,
        NODE_CLASS,
        NODE_COMPARE,
        NODE_CONST,
        NODE_CONTINUE,
        NODE_DEBUGGER,
        NODE_DECREMENT,
        NODE_DEFAULT,
        NODE_DELETE,
        NODE_DIRECTIVE_LIST,
        NODE_DO,
        NODE_DOUBLE,
        NODE_ELSE,
        NODE_EMPTY,
        NODE_ENSURE,
        NODE_ENUM,
        NODE_EQUAL,
        NODE_EXCLUDE,
        NODE_EXTENDS,
        NODE_EXPORT,
        NODE_FALSE,
        NODE_FINAL,
        NODE_FINALLY,
        NODE_FLOAT,         // "float" keyword
        NODE_FLOAT64,       // a literal float (i.e. 3.14159)
        NODE_FOR,
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
        NODE_INLINE,
        NODE_INSTANCEOF,
        NODE_INT64,         // a literal integer (i.e. 123)
        NODE_INTERFACE,
        NODE_INVARIANT,
        NODE_IS,
        NODE_LABEL,
        NODE_LESS_EQUAL,
        NODE_LIST,
        NODE_LOGICAL_AND,
        NODE_LOGICAL_OR,
        NODE_LOGICAL_XOR,
        NODE_LONG,
        NODE_MATCH,
        NODE_MAXIMUM,
        NODE_MINIMUM,
        NODE_NAME,
        NODE_NAMESPACE,
        NODE_NATIVE,
        NODE_NEW,
        NODE_NOT_EQUAL,
        NODE_NOT_MATCH,
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
        NODE_PROTECTED,
        NODE_PUBLIC,
        NODE_RANGE,
        NODE_REGULAR_EXPRESSION,
        NODE_REQUIRE,
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
        NODE_SHORT,
        NODE_SMART_MATCH,
        NODE_STATIC,
        NODE_STRICTLY_EQUAL,
        NODE_STRICTLY_NOT_EQUAL,
        NODE_STRING,
        NODE_SUPER,
        NODE_SWITCH,
        NODE_SYNCHRONIZED,
        NODE_THEN,
        NODE_THIS,
        NODE_THROW,
        NODE_THROWS,
        NODE_TRANSIENT,
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
        NODE_VOLATILE,
        NODE_WHILE,
        NODE_WITH,
        NODE_YIELD,

        NODE_max     // mark the limit
    };

    // some nodes use flags, all of which are managed in one bitset
    //
    // (Note that our Nodes are smart and make use of the function named
    // verify_flag() to make sure that this specific node can
    // indeed be given such flag)
    enum class flag_t
    {
        // NODE_CATCH
        NODE_CATCH_FLAG_TYPED,

        // NODE_DIRECTIVE_LIST
        NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES,

        // NODE_ENUM
        NODE_ENUM_FLAG_CLASS,
        NODE_ENUM_FLAG_INUSE,

        // NODE_FOR
        NODE_FOR_FLAG_CONST,
        NODE_FOR_FLAG_FOREACH,
        NODE_FOR_FLAG_IN,

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

        // NODE_PARAM
        NODE_PARAM_FLAG_CONST,
        NODE_PARAM_FLAG_IN,
        NODE_PARAM_FLAG_OUT,
        NODE_PARAM_FLAG_NAMED,
        NODE_PARAM_FLAG_REST,
        NODE_PARAM_FLAG_UNCHECKED,
        NODE_PARAM_FLAG_UNPROTOTYPED,
        NODE_PARAM_FLAG_REFERENCED,         // referenced from a parameter or a variable
        NODE_PARAM_FLAG_PARAMREF,           // referenced from another parameter
        NODE_PARAM_FLAG_CATCH,              // a parameter defined in a catch()

        // NODE_PARAM_MATCH
        NODE_PARAM_MATCH_FLAG_UNPROTOTYPED,

        // NODE_SWITCH
        NODE_SWITCH_FLAG_DEFAULT,           // we found a 'default:' label in that switch

        // NODE_TYPE
        NODE_TYPE_FLAG_MODULO,              // modulo numeric type declaration

        // NODE_VARIABLE, NODE_VAR_ATTRIBUTES
        NODE_VARIABLE_FLAG_CONST,
        NODE_VARIABLE_FLAG_FINAL,
        NODE_VARIABLE_FLAG_LOCAL,
        NODE_VARIABLE_FLAG_MEMBER,
        NODE_VARIABLE_FLAG_ATTRIBUTES,
        NODE_VARIABLE_FLAG_ENUM,            // there is a NODE_SET and it somehow needs to be copied
        NODE_VARIABLE_FLAG_COMPILED,        // Expression() was called on the NODE_SET
        NODE_VARIABLE_FLAG_INUSE,           // this variable was referenced
        NODE_VARIABLE_FLAG_ATTRS,           // currently being read for attributes (to avoid loops)
        NODE_VARIABLE_FLAG_DEFINED,         // was already parsed
        NODE_VARIABLE_FLAG_DEFINING,        // currently defining, cannot read
        NODE_VARIABLE_FLAG_TOADD,           // to be added in the directive list

        NODE_FLAG_max
    };

    typedef std::bitset<static_cast<size_t>(flag_t::NODE_FLAG_max)>     flag_set_t;

    // some nodes use flags, all of which are managed in one bitset
    //
    // (Note that our Nodes are smart and make use of the function named
    // verify_flag() to make sure that this specific node can
    // indeed be given such flag)
    enum class attribute_t
    {
        // member visibility
        NODE_ATTR_PUBLIC,
        NODE_ATTR_PRIVATE,
        NODE_ATTR_PROTECTED,
        NODE_ATTR_INTERNAL,
        NODE_ATTR_TRANSIENT, // variables only, skip when serializing a class
        NODE_ATTR_VOLATILE, // variable only

        // function member type
        NODE_ATTR_STATIC,
        NODE_ATTR_ABSTRACT,
        NODE_ATTR_VIRTUAL,
        NODE_ATTR_ARRAY,
        NODE_ATTR_INLINE,

        // function contract
        NODE_ATTR_REQUIRE_ELSE,
        NODE_ATTR_ENSURE_THEN,

        // function/variable is defined in your system (execution env.)
        // you won't find a body for these functions; the variables
        // will likely be read-only
        NODE_ATTR_NATIVE,

        // function/variable is still defined, but should not be used
        // (using generates a "foo deprecated" warning or equivalent)
        NODE_ATTR_DEPRECATED,
        NODE_ATTR_UNSAFE, // i.e. eval()

        // TODO: add a way to mark functions/variables as browser specific
        //       so we can easily tell the user that it should not be used
        //       or with caution (i.e. #ifdef browser-name ...)

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
        NODE_ATTR_UNUSED,                   // if definition is used, error!

        // class attribute (whether a class can be enlarged at run time)
        NODE_ATTR_DYNAMIC,

        // switch attributes
        NODE_ATTR_FOREACH,
        NODE_ATTR_NOBREAK,
        NODE_ATTR_AUTOBREAK,

        // type attribute, to mark all the nodes within a type expression
        NODE_ATTR_TYPE,

        // The following is to make sure we never define the attributes more
        // than once. In itself it is not an attribute.
        NODE_ATTR_DEFINED,

        // max used to know the number of entries and define our bitset
        NODE_ATTR_max
    };

    typedef std::bitset<static_cast<int>(attribute_t::NODE_ATTR_max)>     attribute_set_t;

    //enum class link_t : uint32_t
    //{
    //    LINK_INSTANCE = 0,
    //    LINK_TYPE,
    //    LINK_ATTRIBUTES,    // this is the list of identifiers
    //    LINK_GOTO_EXIT,
    //    LINK_GOTO_ENTER,
    //    LINK_max
    //};

    enum class compare_mode_t
    {
        COMPARE_STRICT,     // ===
        COMPARE_LOOSE,      // ==
        COMPARE_SMART       // ~~
    };

                                Node(node_t type);
    virtual                     ~Node() noexcept(false); // virtual because of shared pointers

    /** \brief Do not allow direct copies of nodes.
     *
     * It is not safe to just copy a node because a node is part of a
     * tree (parent, child, siblings...) and a copy would not work.
     */
                                Node(Node const&) = delete;

    /** \brief Do not allow direct copies of nodes.
     *
     * It is not safe to just copy a node because a node is part of a
     * tree (parent, child, siblings...) and a copy would not work.
     */
    Node&                       operator = (Node const&) = delete;

    node_t                      get_type() const;
    char const *                get_type_name() const;
    static char const *         type_to_string(node_t type);
    void                        set_type_node(Node::pointer_t node);
    pointer_t                   get_type_node() const;

    bool                        is_number() const;
    bool                        is_nan() const;
    bool                        is_int64() const;
    bool                        is_float64() const;
    bool                        is_boolean() const;
    bool                        is_true() const;
    bool                        is_false() const;
    bool                        is_string() const;
    bool                        is_undefined() const;
    bool                        is_null() const;
    bool                        is_identifier() const;
    bool                        is_literal() const;

    // basic conversions
    void                        to_unknown();
    bool                        to_as();
    node_t                      to_boolean_type_only() const;
    bool                        to_boolean();
    bool                        to_call();
    bool                        to_identifier();
    bool                        to_int64();
    bool                        to_float64();
    bool                        to_label();
    bool                        to_number();
    bool                        to_string();
    void                        to_videntifier();
    void                        to_var_attributes();

    void                        set_boolean(bool value);
    void                        set_int64(Int64 value);
    void                        set_float64(Float64 value);
    void                        set_string(String const& value);

    bool                        get_boolean() const;
    Int64                       get_int64() const;
    Float64                     get_float64() const;
    String const&               get_string() const;

    static compare_t            compare(Node::pointer_t const lhs, Node::pointer_t const rhs, compare_mode_t const mode);

    pointer_t                   clone_basic_node() const;
    pointer_t                   create_replacement(node_t type) const;

    // check flags
    bool                        get_flag(flag_t f) const;
    void                        set_flag(flag_t f, bool v);
    bool                        compare_all_flags(flag_set_t const& s) const;

    // check attributes
    void                        set_attribute_node(pointer_t node);
    pointer_t                   get_attribute_node() const;
    bool                        get_attribute(attribute_t const a) const;
    void                        set_attribute(attribute_t const a, bool const v);
    void                        set_attribute_tree(attribute_t const a, bool const v);
    bool                        compare_all_attributes(attribute_set_t const& s) const;
    static char const *         attribute_to_string(attribute_t const attr);

    // various nodes are assigned an "instance" (direct link to actual declaration)
    void                        set_instance(pointer_t node);
    pointer_t                   get_instance() const;

    // switch operator: switch(...) with(<operator>)
    node_t                      get_switch_operator() const;
    void                        set_switch_operator(node_t op);

    // goto / label
    void                        set_goto_enter(pointer_t node);
    void                        set_goto_exit(pointer_t node);
    pointer_t                   get_goto_enter() const;
    pointer_t                   get_goto_exit() const;

    // handle function parameters (reorder and depth)
    void                        set_param_size(size_t size);
    size_t                      get_param_size() const;
    depth_t                     get_param_depth(size_t j) const;
    void                        set_param_depth(size_t j, depth_t depth);
    size_t                      get_param_index(size_t idx) const; // returns 'j'
    void                        set_param_index(size_t idx, size_t j);

    void                        set_position(Position const& position);
    Position const&             get_position() const;

    bool                        has_side_effects() const;

    bool                        is_locked() const;
    void                        lock();
    void                        unlock();

    size_t                      get_offset() const;

    void                        set_parent(pointer_t parent = pointer_t(), int index = -1);
    pointer_t                   get_parent() const;

    size_t                      get_children_size() const;
    void                        replace_with(pointer_t node);
    void                        delete_child(int index);
    void                        append_child(pointer_t child);
    void                        insert_child(int index, pointer_t child);
    void                        set_child(int index, pointer_t child);
    pointer_t                   get_child(int index) const;
    pointer_t                   find_first_child(node_t type) const;
    pointer_t                   find_next_child(pointer_t start, node_t type) const;
    void                        clean_tree();

    void                        add_variable(pointer_t variable);
    size_t                      get_variable_size() const;
    pointer_t                   get_variable(int index) const;

    void                        add_label(pointer_t label);
    pointer_t                   find_label(String const& name) const;

    static char const *         operator_to_string(node_t op);
    static node_t               string_to_operator(String const& str);

    void                        display(std::ostream& out, int indent, char c) const;
    //String                      type_node_to_string() const;

private:
    typedef std::vector<int32_t>    param_depth_t;
    typedef std::vector<uint32_t>   param_index_t;

    // verify different parameters
    void                        verify_flag(flag_t f) const;
    void                        verify_attribute(attribute_t const f) const;
    bool                        verify_exclusive_attributes(attribute_t f) const;
    void                        modifying() const;

    // output a node to out (on your end, use the << operator)
    void                        display_data(std::ostream& out) const;

    // define the node type
    node_t                      f_type = node_t::NODE_UNKNOWN;
    weak_pointer_t              f_type_node;
    flag_set_t                  f_flags;
    pointer_t                   f_attribute_node;
    attribute_set_t             f_attributes;
    node_t                      f_switch_operator = node_t::NODE_UNKNOWN;

    // whether this node is currently locked
    int32_t                     f_lock = 0;

    // location where the node was found (filename, line #, etc.)
    Position                    f_position;

    // data of this node
    Int64                       f_int;
    Float64                     f_float;
    String                      f_str;

    // function parameters
    param_depth_t               f_param_depth;
    param_index_t               f_param_index;

    // parent children node tree handling
    weak_pointer_t              f_parent;
    int32_t                     f_offset = 0;   // offset (index) in parent array of children -- set by compiler, should probably be removed...
    vector_of_pointers_t        f_children;
    weak_pointer_t              f_instance;

    // goto nodes
    weak_pointer_t              f_goto_enter;
    weak_pointer_t              f_goto_exit;

    // other connections between nodes
    vector_of_weak_pointers_t   f_variables;
    map_of_weak_pointers_t      f_labels;
};

typedef std::vector<Node::pointer_t>    node_pointer_vector_t;

std::ostream& operator << (std::ostream& out, Node const& node);



// Stack based locking of nodes
class NodeLock
{
public:
                NodeLock(Node::pointer_t node);
                ~NodeLock();

    // premature unlock
    void        unlock();

private:
    Node::pointer_t f_node;
};


}
// namespace as2js

#endif
// #ifndef AS2JS_NODE_H

// vim: ts=4 sw=4 et
