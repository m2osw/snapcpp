/* compiler_class.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/compiler.h"

#include    "as2js/exceptions.h"
#include    "as2js/message.h"


namespace as2js
{


bool Compiler::is_dynamic_class(Node::pointer_t class_node)
{
    // can we know?
    if(!class_node)
    {
        return true;
    }

    // already determined?
    if(get_attribute(class_node, Node::attribute_t::NODE_ATTR_DYNAMIC))
    {
        return true;
    }

    size_t const max_children(class_node->get_children_size());
    for(size_t idx = 0; idx < max_children; ++idx)
    {
        Node::pointer_t child(class_node->get_child(idx));
        if(child->get_type() == Node::node_t::NODE_EXTENDS)
        {
            // TODO: once we support multiple extends, work on
            //       the list of them, in which case one instance
            //       is not going to be too good
            Node::pointer_t name(child->get_child(0));
            Node::pointer_t extends(name ? name->get_instance() : name);
            if(extends)
            {
                if(extends->get_string() == "Object")
                {
                    // we ignore the dynamic flag of Object (that is a
                    // hack in the language reference!)
                    return false;
                }
                // continue at the next level (depth increasing)
                return is_dynamic_class(extends); // recursive
            }
            break;
        }
    }

    return false;
}


void Compiler::check_member(Node::pointer_t ref, Node::pointer_t field, Node::pointer_t field_name)
{
    if(!field)
    {
        Node::pointer_t type(ref->get_type_node());
        if(!is_dynamic_class(type))
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_STATIC, ref->get_position());
            msg << "'" << ref->get_string()
                << ": " << type->get_string()
                << "' is not dynamic and thus it cannot be used with unknown member '" << field_name->get_string()
                << "'.";
        }
        return;
    }

    Node::pointer_t obj(ref->get_instance());
    if(!obj)
    {
        return;
    }

    // If the link is directly a class or an interface
    // then the field needs to be a sub-class, sub-interface,
    // static function, static variable or constant variable.
    if(obj->get_type() != Node::node_t::NODE_CLASS
    && obj->get_type() != Node::node_t::NODE_INTERFACE)
    {
        return;
    }

    bool err(false);
    switch(field->get_type())
    {
    case Node::node_t::NODE_CLASS:
    case Node::node_t::NODE_INTERFACE:
        break;

    case Node::node_t::NODE_FUNCTION:
        //
        // note that constructors are considered static, but
        // you cannot just call a constructor...
        //
        // operators are static and thus we will be fine with
        // operators (since you need to call operators with
        // all the required inputs)
        //
        err = !get_attribute(field, Node::attribute_t::NODE_ATTR_STATIC)
           && !field->get_flag(Node::flag_t::NODE_FUNCTION_FLAG_OPERATOR);
        break;

    case Node::node_t::NODE_VARIABLE:
        // static const foo = 123; is fine
        err = !get_attribute(field, Node::attribute_t::NODE_ATTR_STATIC)
           && !field->get_flag(Node::flag_t::NODE_VARIABLE_FLAG_CONST);
        break;

    default:
        err = true;
        break;

    }

    if(err)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INSTANCE_EXPECTED, ref->get_position());
        msg << "you cannot directly access non-static functions and non-static/constant variables in a class ('"
            << field->get_string()
            << "' here); you need to use an instance instead.";
    }
}


/** \brief Check whether a function is a constructor.
 *
 * This function checks a node representing a function to determine whether
 * it represents a constructor or not.
 *
 * By default, if a function is marked as a constructor by the programmer,
 * then this function considers the function as a constructor no matter
 * what (outside of the fact that it has to be a function defined in a
 * class, obviously.)
 *
 * \param[in] function_node  A node representing a function definition.
 * \param[out] the_class  If the function is a constructor, this is the
 *                        class it was defined in.
 *
 * \return true if the function is a constructor and in that case the_class
 *         is set to the class node pointer; otherwise the_class is set to
 *         nullptr.
 */
bool Compiler::is_constructor(Node::pointer_t function_node, Node::pointer_t& the_class)
{
    the_class.reset();

    if(function_node->get_type() != Node::node_t::NODE_FUNCTION)
    {
        throw exception_internal_error(std::string("Compiler::is_constructor() was called with a node which is not a NODE_FUNCTION, it is ") + function_node->get_type_name());
    }

    // search the first NODE_CLASS with the same name
    for(Node::pointer_t parent(function_node->get_parent()); parent; parent = parent->get_parent())
    {
        // Note: here I made a note that sub-functions cannot be
        //       constructors which is true in ActionScript, but
        //       not in JavaScript. We actually make use of a
        //       sub-function to create inheritance that works
        //       in JavaScript (older browsers required a "new Object"
        //       to generate inheritance which was a big problem.)
        //       However, in our language, we probably want people
        //       to make use of the class keyword anyway, so they
        //       could create a sub-class inside a function and
        //       we are back in business!
        //
        switch(parent->get_type())
        {
        case Node::node_t::NODE_PACKAGE:
        case Node::node_t::NODE_PROGRAM:
        case Node::node_t::NODE_FUNCTION:    // sub-functions cannot be constructors
        case Node::node_t::NODE_INTERFACE:
            return false;

        case Node::node_t::NODE_CLASS:
            // we found the class in question

            // user defined constructor?
            if(get_attribute(function_node, Node::attribute_t::NODE_ATTR_CONSTRUCTOR)
            || parent->get_string() == function_node->get_string())
            {
                the_class = parent;
                return true;
            }
            return false;

        default:
            // ignore all the other nodes
            break;

        }
    }

    if(get_attribute(function_node, Node::attribute_t::NODE_ATTR_CONSTRUCTOR))
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_ATTRIBUTES, function_node->get_position());
        msg << "'constructor " << function_node->get_string() << "()' cannot be used outside of a class declaration.";
    }

    return false;
}


void Compiler::check_super_validity(Node::pointer_t expr)
{
    if(!expr)
    {
        return;
    }
    Node::pointer_t parent(expr->get_parent());
    if(!parent)
    {
        return;
    }

    bool const needs_constructor(parent->get_type() == Node::node_t::NODE_CALL);
    bool first_function(true);
    bool continue_testing(true);
    for(; parent && continue_testing; parent = parent->get_parent())
    {
        switch(parent->get_type())
        {
        case Node::node_t::NODE_FUNCTION:
            if(first_function)
            {
                // We have two super's
                // 1) super(params) in constructors
                // 2) super.field(params) in non-static functions
                // case 1 is recognized as having a direct parent
                // of type call (see at start of function!)
                // case 2 is all other cases
                // in both cases we need to be defined in a class
                Node::pointer_t the_class;
                if(needs_constructor)
                {
                    if(!is_constructor(parent, the_class))
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
                        msg << "'super()' cannot be used outside of a constructor function.";
                        return;
                    }
                }
                else
                {
                    if(parent->get_flag(Node::flag_t::NODE_FUNCTION_FLAG_OPERATOR)
                    || get_attribute(parent, Node::attribute_t::NODE_ATTR_STATIC)
                    || get_attribute(parent, Node::attribute_t::NODE_ATTR_CONSTRUCTOR)
                    || is_constructor(parent, the_class))
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
                        msg << "'super.member()' cannot be used in a static function nor a constructor.";
                        return;
                    }
                }
                // TODO: Test that this is correct, it was missing...
                //       Once false, we skip all the tests from this
                //       block.
                first_function = false;
            }
            else
            {
                // Can it be used in sub-functions?
                // If we arrive here then we can err if
                // super and/or this are not available
                // in sub-functions... TBD
            }
            break;

        case Node::node_t::NODE_CLASS:
        case Node::node_t::NODE_INTERFACE:
            return;

        case Node::node_t::NODE_PROGRAM:
        case Node::node_t::NODE_ROOT:
            continue_testing = false;
            break;

        default:
            break;

        }
    }

    if(needs_constructor)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
        msg << "'super()' cannot be used outside a class definition.";
    }
#if 0
    else {
fprintf(stderr, "WARNING: 'super.member()' should only be used in a class function.\n");
    }
#endif
}


void Compiler::link_type(Node::pointer_t type)
{
//std::cerr << "find_type()\n";
    // already linked?
    Node::pointer_t link(type->get_instance());
    if(link)
    {
        return;
    }

    if(type->get_type() != Node::node_t::NODE_IDENTIFIER
    && type->get_type() != Node::node_t::NODE_STRING)
    {
        // we cannot link (determine) the type at compile time
        // if we have a type expression
//std::cerr << "WARNING: dynamic type?!\n";
        return;
    }

    if(type->get_flag(Node::flag_t::NODE_IDENTIFIER_FLAG_TYPED))
    {
        // if it failed already, fail only once...
        return;
    }
    type->set_flag(Node::flag_t::NODE_IDENTIFIER_FLAG_TYPED, true);

    Node::pointer_t object;
    if(!resolve_name(type, type, object, Node::pointer_t(), 0))
    {
        // unknown type?! -- should we return a link to Object?
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, type->get_position());
        msg << "cannot find a class definition for type '" << type->get_string() << "'.";
        return;
    }

    if(object->get_type() != Node::node_t::NODE_CLASS
    && object->get_type() != Node::node_t::NODE_INTERFACE)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, type->get_position());
        msg << "the name '" << type->get_string() << "' is not referencing a class nor an interface.";
        return;
    }

    // it worked.
    type->set_instance(object);
}


bool Compiler::find_in_extends(Node::pointer_t link, Node::pointer_t field, int& funcs, Node::pointer_t& resolution, Node::pointer_t params, int const search_flags)
{
    // try to see if we are inheriting that field...
    NodeLock ln(link);
    size_t const max_children(link->get_children_size());
    size_t count(0);
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t extends(link->get_child(idx));
        if(extends->get_type() == Node::node_t::NODE_EXTENDS)
        {
            // TODO: support list of extends (see IMPLEMENTS below!)
            if(extends->get_children_size() == 1)
            {
                Node::pointer_t type(extends->get_child(0));
                link_type(type);
                Node::pointer_t sub_link(type->get_instance());
                if(!sub_link)
                {
                    // we cannot search a field in nothing...
                    Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_TYPE_NOT_LINKED, link->get_position());
                    msg << "type not linked, cannot lookup member.";
                }
                else if(find_any_field(sub_link, field, funcs, resolution, params, search_flags))
                {
                    ++count;
                }
            }
//std::cerr << "Extends existing! (" << extends.GetChildCount() << ")\n";
        }
        else if(extends->get_type() == Node::node_t::NODE_IMPLEMENTS)
        {
            if(extends->get_children_size() == 1)
            {
                Node::pointer_t type(extends->get_child(0));
                if(type->get_type() == Node::node_t::NODE_LIST)
                {
                    size_t cnt(type->get_children_size());
                    for(size_t j(0); j < cnt; ++j)
                    {
                        Node::pointer_t child(type->get_child(j));
                        link_type(child);
                        Node::pointer_t sub_link(child->get_instance());
                        if(!sub_link)
                        {
                            // we cannot search a field in nothing...
                            Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_TYPE_NOT_LINKED, link->get_position());
                            msg << "type not linked, cannot lookup member.";
                        }
                        else if(find_any_field(sub_link, field, funcs, resolution, params, search_flags)) // recursive
                        {
                            ++count;
                        }
                    }
                }
                else
                {
                    link_type(type);
                    Node::pointer_t sub_link(type->get_instance());
                    if(!sub_link)
                    {
                        // we can't search a field in nothing...
                        Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_TYPE_NOT_LINKED, link->get_position());
                        msg << "type not linked, cannot lookup member.";
                    }
                    else if(find_any_field(sub_link, field, funcs, resolution, params, search_flags)) // recursive
                    {
                        ++count;
                    }
                }
            }
        }
    }

    if(count == 1 || funcs != 0)
    {
        return true;
    }

    if(count == 0)
    {
        // NOTE: warning? error? This actually would just turn
        //     on a flag.
        //     As far as I know I now have an error in case
        //     the left hand side expression is a static
        //     class (opposed to a dynamic class which can
        //     have members added at runtime)
//std::cerr << "     field not found...\n";
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DUPLICATES, field->get_position());
        msg << "found more than one match for '" << field->get_string() << "'.";
    }

    return false;
}


bool Compiler::check_field(Node::pointer_t link, Node::pointer_t field, int& funcs, Node::pointer_t& resolution, Node::pointer_t params, int const search_flags)
{
    NodeLock link_ln(link);
    size_t const max_children(link->get_children_size());
//std::cerr << "  +++ compiler_class.cpp: check_field() " << max_children << " +++\n";
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t list(link->get_child(idx));
        if(list->get_type() != Node::node_t::NODE_DIRECTIVE_LIST)
        {
            // extends, implements, empty...
            continue;
        }

        // search in this list!
        NodeLock list_ln(list);
        size_t const max_list_children(list->get_children_size());
        for(size_t j(0); j < max_list_children; ++j)
        {
            // if we have a sub-list, generate a recursive call
            Node::pointer_t child(list->get_child(j));
            if(child->get_type() == Node::node_t::NODE_DIRECTIVE_LIST)
            {
                if(check_field(list, field, funcs, resolution, params, search_flags)) // recursive
                {
                    if(funcs_name(funcs, resolution, false))
                    {
                        return true;
                    }
                }
            }
            else if(child->get_type() != Node::node_t::NODE_EMPTY)
            {
//std::cerr << "  +--> compiler_class.cpp: check_field(): call check_name() as we are searching for a \"class\" field named \"" << field->get_string() << "\" (actually this may be any object that can be given a name, we may be in a package too)\n";
                if(check_name(list, j, resolution, field, params, search_flags))
                {
//std::cerr << "  +++ compiler_class.cpp: check_field(): funcs_name() called too +++\n";
                    if(funcs_name(funcs, resolution))
                    {
                        Node::pointer_t inst(field->get_instance());
                        if(!inst)
                        {
                            field->set_instance(resolution);
                        }
                        else if(inst != resolution)
                        {
                            // if already defined, it should be the same or
                            // we have a real problem
                            throw exception_internal_error("found an instance twice, but it was different each time");
                        }
//std::cerr << "  +++ compiler_class.cpp: check_field(): accept this resolution as the answer! +++\n";
                        return true;
                    }
                }
            }
        }
    }

//std::cerr << "  +++ compiler_class.cpp: check_field(): failed -- no resolution yet +++\n";
    return false;
}


bool Compiler::find_any_field(Node::pointer_t link, Node::pointer_t field, int& funcs, Node::pointer_t& resolution, Node::pointer_t params, int const search_flags)
{
//std::cerr << "  *** find_any_field()\n";
    if(check_field(link, field, funcs, resolution, params, search_flags))
    {
//std::cerr << "Check Field true...\n";
        return true;
    }
    if(funcs != 0)
    {
        // TODO: stronger validation of functions
        // this is wrong, we need a depth test on the best
        // functions but we need to test all the functions
        // of inherited fields too
//std::cerr << "funcs != 0 true...\n";
        return true;
    }

//std::cerr << "FindInExtends?!...\n";
    return find_in_extends(link, field, funcs, resolution, params, search_flags); // recursive
}


bool Compiler::find_field(Node::pointer_t link, Node::pointer_t field, int& funcs, Node::pointer_t& resolution, Node::pointer_t params, int const search_flags)
{
    // protect current compiler error flags while searching
    RestoreFlags restore_flags(this);

    bool const r(find_any_field(link, field, funcs, resolution, params, search_flags));
    if(!r)
    {
        print_search_errors(field);
    }

    return r;
}


bool Compiler::resolve_field(Node::pointer_t object, Node::pointer_t field, Node::pointer_t& resolution, Node::pointer_t params, int const search_flags)
{
    // this is to make sure it is optimized, etc.
    //expression(field); -- we cannot have this here or it generates loops

    // just in case the caller is re-using the same node
    resolution.reset();

    Node::pointer_t link;
    Node::pointer_t type;

    // check that the object is indeed an object (i.e. a variable
    // which references a class)
    switch(object->get_type())
    {
    case Node::node_t::NODE_VARIABLE:
    case Node::node_t::NODE_PARAM:
        // it is a variable or a parameter, check for the type
        //NodeLock ln(object);
        {
            size_t const max(object->get_children_size());
            size_t idx;
            for(idx = 0; idx < max; ++idx)
            {
                type = object->get_child(idx);
                if(type->get_type() != Node::node_t::NODE_SET
                && type->get_type() != Node::node_t::NODE_VAR_ATTRIBUTES)
                {
                    // we found the type
                    break;
                }
            }
            if(idx >= max || !type)
            {
                // TODO: should this be an error instead?
                Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INCOMPATIBLE, object->get_position());
                msg << "variables and parameters without a type should not be used with members.";
                return false;
            }
        }

        // we need to have a link to the class
        link_type(type);
        link = type->get_instance();
        if(!link)
        {
            // NOTE: we can't search a field in nothing...
            //     if I'm correct, it will later bite the
            //     user if the class isn't dynamic
//fprintf(stderr, "WARNING: type not linked, cannot lookup member.\n");
            return false;
        }
        break;

    case Node::node_t::NODE_CLASS:
    case Node::node_t::NODE_INTERFACE:
        link = object;
        break;

    default:
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_TYPE, object->get_position());
            msg << "object of type '"
                << object->get_type_name()
                << "' is not known to have members.";
        }
        return false;

    }

    if(field->get_type() != Node::node_t::NODE_IDENTIFIER
    && field->get_type() != Node::node_t::NODE_VIDENTIFIER
    && field->get_type() != Node::node_t::NODE_STRING)
    {
        // we cannot determine at compile time whether a
        // dynamic field is valid...
        //std:cerr << "WARNING: cannot check a dynamic field.\n";
        return false;
    }

#if 0
char buf[256];
size_t sz = sizeof(buf);
data.f_str.ToUTF8(buf, sz);
fprintf(stderr, "DEBUG: resolving field '%s' (%d) Field: ", buf, data.f_type);
field.DisplayPtr(stderr);
fprintf(stderr, "\n");
#endif

    int funcs(0);
    bool const r(find_field(link, field, funcs, resolution, params, search_flags));
    if(!r)
    {
        return false;
    }

    if(funcs != 0)
    {
#if 0
fprintf(stderr, "DEBUG: field ");
field.DisplayPtr(stderr);
fprintf(stderr, " is a function.\n");
#endif
        resolution.reset();
        return select_best_func(params, resolution);
    }

    return true;
}


bool Compiler::find_member(Node::pointer_t member, Node::pointer_t& resolution, Node::pointer_t params, int search_flags)
{
//std::cerr << "find_member()\n";
    // Just in case the caller is re-using the same node
    resolution.reset();

    // Invalid member node? If so don't generate an error because
    // we most certainly already mentioned that to the user
    // (and if not that's a bug earlier than here).
    if(member->get_children_size() != 2)
    {
        return false;
    }
    NodeLock ln(member);

//std::cerr << "Searching for Member...\n";

    bool must_find = false;
    Node::pointer_t object; // our sub-resolution

    Node::pointer_t name(member->get_child(0));
    switch(name->get_type())
    {
    case Node::node_t::NODE_MEMBER:
        // This happens when you have an expression such as:
        //        a.b.c
        // Then the child most MEMBER will be the identifier 'a'
        if(!find_member(name, object, params, search_flags))  // recursive
        {
            return false;
        }
        // If we reach here, the resolution (object variable here)
        // is the node we want to use next to resolve the field(s)
        break;

    case Node::node_t::NODE_SUPER:
    {
        // SUPER cannot be used on the right side of a NODE_MEMBER
        // -- this is not correct, we could access the super of a
        //    child member (a.super.blah represents field blah in
        //    the class a is derived from)
        //Node::pointer_t parent(name->get_parent());
        //if(parent->get_type() == Node::node_t::NODE_MEMBER
        //&& name->get_offset() != 0)
        //{
        //    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, member->get_position());
        //    msg << "you cannot use 'super' after a period (.), it has to be first.";
        //}

        // super should only be used in classes, but we can
        // find standalone functions using this keyword too...
        // here we search for the class and if we find it then
        // we try to get access to the extends. If the object
        // is Object, then we generate an error (i.e. there is
        // no super to Object).
        check_super_validity(name);
        Node::pointer_t class_node(class_of_member(member));
        // NOTE: Interfaces can use super but we cannot
        //       know what it is at compile time.
        if(class_node
        && class_node->get_type() == Node::node_t::NODE_CLASS)
        {
            if(class_node->get_string() == "Object")
            {
                // this should never happen!
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, member->get_position());
                msg << "you cannot use 'super' within the 'Object' class.";
            }
            else
            {
                size_t const max_children(class_node->get_children_size());
                for(size_t idx(0); idx < max_children; ++idx)
                {
                    Node::pointer_t child(class_node->get_child(idx));
                    if(child->get_type() == Node::node_t::NODE_EXTENDS)
                    {
                        if(child->get_children_size() == 1)
                        {
                            Node::pointer_t child_name(child->get_child(0));
                            object = child_name->get_instance();
                        }
                        if(!object)
                        {
                            // there is another error...
                            return false;
                        }
                        break;
                    }
                }
                if(!object)
                {
                    // default to Object if no extends
                    resolve_internal_type(class_node, "Object", object);
                }
                must_find = true;
            }
        }
    }
        break;

    default:
        expression(name);
        break;

    }

    // do the field expression so we possibly detect more errors
    // in the field now instead of the next compile
    Node::pointer_t field(member->get_child(1));
    if(field->get_type() != Node::node_t::NODE_IDENTIFIER)
    {
        expression(field);
    }

    if(!object)
    {
        // TODO: this is totally wrong, what we need is the type, not
        //     just the name; this if we have a string, the type is
        //     the String class.
        if(name->get_type() != Node::node_t::NODE_IDENTIFIER
        && name->get_type() != Node::node_t::NODE_STRING)
        {
            // A dynamic name can't be resolved now; we can only
            // hope that it will be a valid name at run time.
            // However, we still want to resolve everything we
            // can in the list of field names.
            // FYI, this happens in this case:
            //    ("test_" + var).hello
            return true;
        }

        if(!resolve_name(name, name, object, params, search_flags))
        {
            // we cannot even find the first name!
            // we will not search for fields since we need to have
            // an object for that purpose!
            return false;
        }
    }

    // we avoid errors by returning no resolution but 'success'
    if(object)
    {
        bool const result(resolve_field(object, field, resolution, params, search_flags));
        if(!result && must_find)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, member->get_position());
            msg << "'super' must name a valid field of the super class.";
        }
        else
        {
            check_member(name, resolution, field);
        }
        return result;
    }

    return true;
}


void Compiler::resolve_member(Node::pointer_t expr, Node::pointer_t params, int const search_flags)
{
//std::cerr << "Compiler::resolve_member()\n";
    Node::pointer_t resolution;
    if(!find_member(expr, resolution, params, search_flags))
    {
#if 0
// with dynamic entries, this generates invalid warnings
        NodePtr& parent = expr.GetParent();
        Data& data = parent.GetData();
        if(data.f_type != NODE_ASSIGNMENT
        || parent.GetChildCount() != 2
        || !parent.GetChild(0).SameAs(expr)) {
fprintf(stderr, "WARNING: cannot find field member.\n");
        }
#endif
        return;
    }

    // we got a resolution; but dynamic names
    // cannot be fully resolved at compile time
    if(!resolution)
    {
        return;
    }

    // the name was fully resolved, check it out
    if(replace_constant_variable(expr, resolution))
    {
        // just a constant, we're done
        return;
    }

    // copy the type whenever available
    expr->set_instance(resolution);
    Node::pointer_t type(resolution->get_type_node());
    if(type)
    {
        expr->set_type_node(type);
    }

    // if we have a Getter, transform the MEMBER into a CALL
    // to a MEMBER
    if(resolution->get_type() == Node::node_t::NODE_FUNCTION
    && resolution->get_flag(Node::flag_t::NODE_FUNCTION_FLAG_GETTER))
    {
//std::cerr << "CAUGHT! getter...\n";
        // so expr is a MEMBER at this time
        // it has two children
        Node::pointer_t left(expr->get_child(0));
        Node::pointer_t right(expr->get_child(1));
        expr->delete_child(0);
        expr->delete_child(0);    // 1 is now 0

        // create a new node since we do not want to move the
        // call (expr) node from its parent.
        Node::pointer_t member(expr->create_replacement(Node::node_t::NODE_MEMBER));
        member->set_instance(resolution);
        member->set_type_node(type);
        member->append_child(left);
        member->append_child(right);

        expr->append_child(member);

        // we need to change the name to match the getter
        // NOTE: we know that the right data is an identifier,
        //       a v-identifier, or a string so the following
        //       will always work
        String getter_name("->");
        getter_name += right->get_string();
        right->set_string(getter_name);

        // the call needs a list of parameters (empty)
        Node::pointer_t empty_params(expr->create_replacement(Node::node_t::NODE_LIST));
        expr->append_child(empty_params);

        // and finally, we transform the member in a call!
        expr->to_call();
    }
}


Node::depth_t Compiler::find_class(Node::pointer_t class_type, Node::pointer_t type, Node::depth_t depth)
{
    NodeLock ln(class_type);
    size_t const max_children(class_type->get_children_size());

    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(class_type->get_child(idx));
        if(child->get_type() == Node::node_t::NODE_IMPLEMENTS
        || child->get_type() == Node::node_t::NODE_EXTENDS)
        {
            if(child->get_children_size() == 0)
            {
                // should never happen
                continue;
            }
            NodeLock child_ln(child);
            Node::pointer_t super_name(child->get_child(0));
            Node::pointer_t super(super_name->get_instance());
            if(!super)
            {
                expression(super_name);
                super = super_name->get_instance();
            }
            if(!super)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, class_type->get_position());
                msg << "cannot find the type named in an 'extends' or 'implements' list.";
                continue;
            }
            if(super == type)
            {
                return depth;
            }
        }
    }

    depth += 1;

    Node::depth_t result(Node::MATCH_NOT_FOUND);
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(class_type->get_child(idx));
        if(child->get_type() == Node::node_t::NODE_IMPLEMENTS
        || child->get_type() == Node::node_t::NODE_EXTENDS)
        {
            if(child->get_children_size() == 0)
            {
                // should never happen
                continue;
            }
            NodeLock child_ln(child);
            Node::pointer_t super_name(child->get_child(0));
            Node::pointer_t super(super_name->get_instance());
            if(!super)
            {
                continue;
            }
            Node::depth_t const r(find_class(super, type, depth));  // recursive
            if(r > result)
            {
                result = r;
            }
        }
    }

    return result;
}



bool Compiler::is_derived_from(Node::pointer_t derived_class, Node::pointer_t super_class)
{
    if(derived_class == super_class)
    {
        // exact same object, it is "derived from"
        return true;
    }

    size_t const max(derived_class->get_children_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t extends(derived_class->get_child(idx));
        if(!extends)
        {
            continue;
        }
        if(extends->get_type() != Node::node_t::NODE_EXTENDS
        && extends->get_type() != Node::node_t::NODE_IMPLEMENTS)
        {
            continue;
        }
        Node::pointer_t type(extends->get_child(0));
        // TODO: we probably want to accept lists of extends too
        //       because JavaScript gives us the ability to create
        //       objects with multiple derivation (not exactly
        //       100% true, but close enough and it makes a lot
        //       of things MUCH easier.)
        if(type->get_type() == Node::node_t::NODE_LIST
        && extends->get_type() == Node::node_t::NODE_IMPLEMENTS)
        {
            // IMPLEMENTS accepts lists
            size_t const cnt(type->get_children_size());
            for(size_t j(0); j < cnt; ++j)
            {
                Node::pointer_t sub_type(type->get_child(j));
                link_type(sub_type);
                Node::pointer_t instance(sub_type->get_instance());
                if(!instance)
                {
                    continue;
                }
                if(is_derived_from(instance, super_class))
                {
                    return true;
                }
            }
        }
        else
        {
            // TODO: review the "extends ..." implementation so it supports
            //       lists in the parser and then here
            link_type(type);
            Node::pointer_t instance(type->get_instance());
            if(!instance)
            {
                continue;
            }
            if(is_derived_from(instance, super_class))
            {
                return true;
            }
        }
    }

    return false;
}


/** \brief Search for a class or interface node.
 *
 * This function searches for a node of type NODE_CLASS or NODE_INTERFACE
 * starting with \p class_node. The search checks \p class_node and all
 * of its parents.
 *
 * The search stops prematuraly if a NODE_PACKAGE, NODE_PROGRAM, or
 * NODE_ROOT is found first.
 *
 * \param[in] class_node  The object from which a class is to be searched.
 *
 * \return The class or interface, or a null pointer if not found.
 */
Node::pointer_t Compiler::class_of_member(Node::pointer_t class_node)
{
    while(class_node)
    {
        if(class_node->get_type() == Node::node_t::NODE_CLASS
        || class_node->get_type() == Node::node_t::NODE_INTERFACE)
        {
            // got the class/interface definition
            return class_node;
        }
        if(class_node->get_type() == Node::node_t::NODE_PACKAGE
        || class_node->get_type() == Node::node_t::NODE_PROGRAM
        || class_node->get_type() == Node::node_t::NODE_ROOT)
        {
            // not found, we reached one of package/program/root instead
            break;
        }
        class_node = class_node->get_parent();
    }

    return Node::pointer_t();
}


/** \brief Check whether derived_class is extending super_class.
 *
 * This function checks whether the object defined as derived_class
 * has an extends or implements one that includes super_class.
 *
 * The \p the_super_class parameter is set to the class of the
 * super_class object. This can be used to determine different
 * types of errors.
 *
 * Note that if derived_class or super_class are not objects defined
 * in a class, then the function always returns false.
 *
 * \param[in] derived_class  The class which is checked to know whether it
 *                           derives from super_class.
 * \param[in] super_class  The class that is expected to be in the extends
 *                         or implements lists.
 * \param[out] the_super_class  The actual class object in which super_class
 *                              is defined.
 *
 * \return true if derived_class is derived from super_class.
 */
bool Compiler::are_objects_derived_from_one_another(Node::pointer_t derived_class, Node::pointer_t super_class, Node::pointer_t& the_super_class)
{
    the_super_class = class_of_member(super_class);
    if(!the_super_class)
    {
        return false;
    }
    Node::pointer_t the_derived_class(class_of_member(derived_class));
    if(!the_derived_class)
    {
        return false;
    }

    return is_derived_from(the_derived_class, the_super_class);
}


void Compiler::declare_class(Node::pointer_t class_node)
{
    size_t const max_children(class_node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        //NodeLock ln(class_node);
        Node::pointer_t child(class_node->get_child(idx));
        switch(child->get_type())
        {
        case Node::node_t::NODE_DIRECTIVE_LIST:
            declare_class(child); // recursive!
            break;

        case Node::node_t::NODE_CLASS:
        case Node::node_t::NODE_INTERFACE:
            class_directive(child);
            break;

        case Node::node_t::NODE_ENUM:
            enum_directive(child);
            break;

        case Node::node_t::NODE_FUNCTION:
//std::cerr << "Got a function member in that class...\n";
            function(child);
            break;

        case Node::node_t::NODE_VAR:
            var(child);
            break;

        default:
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NODE, child->get_position());
                msg << "the '" << child->get_type_name() << "' token cannot be a class member.";
            }
            break;

        }
    }
}


void Compiler::extend_class(Node::pointer_t class_node, bool const extend, Node::pointer_t extend_name)
{
    expression(extend_name);

    Node::pointer_t super(extend_name->get_instance());
    if(super)
    {
        switch(super->get_type())
        {
        case Node::node_t::NODE_CLASS:
            if(class_node->get_type() == Node::node_t::NODE_INTERFACE)
            {
                // (super) 'class A', 'interface B extends A'
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CLASS, class_node->get_position());
                msg << "class '" << super->get_string() << "' cannot extend interface '" << class_node->get_string() << "'.";
            }
            else if(!extend)
            {
                // (super) 'class A', '... implements A'
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CLASS, class_node->get_position());
                msg << "class '" << super->get_string() << "' cannot implement class '" << class_node->get_string() << "'. Use 'extends' instead.";
            }
            else if(get_attribute(super, Node::attribute_t::NODE_ATTR_FINAL))
            {
                // (super) 'final class A', 'class B extends A'
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_FINAL, class_node->get_position());
                msg << "class '" << super->get_string() << "' is marked final and it cannot be extended by '" << class_node->get_string() << "'.";
            }
            break;

        case Node::node_t::NODE_INTERFACE:
            if(class_node->get_type() == Node::node_t::NODE_INTERFACE && !extend)
            {
                // (super) 'interface A', 'interface B implements A'
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_CLASS, class_node->get_position());
                msg << "interface '" << super->get_string() << "' cannot implement interface '" << class_node->get_string() << "'. Use 'extends' instead.";
            }
            else if(get_attribute(super, Node::attribute_t::NODE_ATTR_FINAL))
            {
                // TODO: prove that this error happens earlier and thus that
                //       we do not need to generate it here
                //
                // (super) 'final interface A'
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_FINAL, class_node->get_position());
                msg << "interface '" << super->get_string() << "' is marked final, which is not legal.";
            }
            break;

        default:
            // this should never happen
            throw exception_internal_error("found a LINK_INSTANCE which is neither a class nor an interface.");

        }
    }
    //else -- TBD: should already have gotten an error by now?
}


void Compiler::class_directive(Node::pointer_t& class_node)
{
    // TBD: Should we instead of looping check nodes in order to
    //      enforce order? Or do we trust that the parser already
    //      did that properly?
    size_t const max(class_node->get_children_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        //NodeLock ln(class_node);
        Node::pointer_t child(class_node->get_child(idx));
        switch(child->get_type())
        {
        case Node::node_t::NODE_DIRECTIVE_LIST:
            declare_class(child);
            break;

        case Node::node_t::NODE_EXTENDS:
            extend_class(class_node, true, child->get_child(0));
            break;

        case Node::node_t::NODE_IMPLEMENTS:
            extend_class(class_node, false, child->get_child(0));
            break;

        case Node::node_t::NODE_EMPTY:
            break;

        default:
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, class_node->get_position());
                msg << "invalid token '" << child->get_type_name() << "' in a class definition.";
            }
            break;

        }
    }
}


/** \brief Enum directive.
 *
 * Enumerations are like classes defining a list of constant values.
 *
 * \param[in] enum_node  The enumeration node to work on.
 */
void Compiler::enum_directive(Node::pointer_t& enum_node)
{
    NodeLock ln(enum_node);
    size_t const max_children(enum_node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t entry(enum_node->get_child(idx));
        if(entry->get_children_size() != 1)
        {
            // this happens in case of an empty enumeration
            // entry type should be NODE_EMPTY
            continue;
        }
        Node::pointer_t set(entry->get_child(0));
        if(set->get_type() != Node::node_t::NODE_SET
        || set->get_children_size() != 1)
        {
            // not valid, skip
            //
            // TODO: for test purposes we could create an invalid tree to hit
            //       this line and have coverage
            //
            continue; // LCOV_EXCL_LINE
        }
        // compile the expression
        expression(set->get_child(0));
    }
}




}
// namespace as2js

// vim: ts=4 sw=4 et
