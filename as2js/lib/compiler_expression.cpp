/* compiler_compile.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/compiler.h"

#include    "as2js/exceptions.h"
#include    "as2js/message.h"


namespace as2js
{





bool Compiler::expression_new(Node::pointer_t new_node)
{
    //
    // handle the special case of:
    //    VAR name := NEW class()
    //

    if(new_node->get_children_size() == 0)
    {
        return false;
    }

//fprintf(stderr, "BEFORE:\n");
//new_node.Display(stderr);
    Node::pointer_t call(new_node->get_child(0));
    if(!call)
    {
        return false;
    }

    if(call->get_type() != Node::node_t::NODE_CALL
    || call->get_children_size() != 2)
    {
        return false;
    }

    // get the function name
    Node::pointer_t id(call->get_child(0));
    if(id->get_type() != Node::node_t::NODE_IDENTIFIER)
    {
        return false;
    }

    // determine the types of the parameters to search a corresponding object or function
    Node::pointer_t params(call->get_child(1));
    size_t const count(params->get_children_size());
//fprintf(stderr, "ResolveCall() with %d expressions\n", count);
//new_node.Display(stderr);
    for(size_t idx(0); idx < count; ++idx)
    {
        expression(params->get_child(idx));
    }

    // resolve what is named
    Node::pointer_t resolution;
    if(!resolve_name(id, id, resolution, params, SEARCH_FLAG_GETTER))
    {
        // an error is generated later if this is a call and no function can be found
        return false;
    }

    // is the name a class or interface?
    if(resolution->get_type() != Node::node_t::NODE_CLASS
    && resolution->get_type() != Node::node_t::NODE_INTERFACE)
    {
        return false;
    }

    // move the nodes under CALL up one level
    Node::pointer_t type(call->get_child(0));
    Node::pointer_t expr(call->get_child(1));
    new_node->delete_child(0);      // remove the CALL
    new_node->append_child(type);   // replace with TYPE + parameters (LIST)
    new_node->append_child(expr);

//fprintf(stderr, "CHANGED TO:\n");
//new_node.Display(stderr);

    return true;
}


bool Compiler::is_function_abstract(Node::pointer_t function_node)
{
    size_t const max_children(function_node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(function_node->get_child(idx));
        if(child->get_type() == Node::node_t::NODE_DIRECTIVE_LIST)
        {
            return false;
        }
    }

    return true;
}


bool Compiler::find_overloaded_function(Node::pointer_t class_node, Node::pointer_t function_node)
{
    size_t const max_children(class_node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(class_node->get_child(idx));
        switch(child->get_type())
        {
        case Node::node_t::NODE_EXTENDS:
        case Node::node_t::NODE_IMPLEMENTS:
        {
            Node::pointer_t names(child->get_child(0));
            if(names->get_type() != Node::node_t::NODE_LIST)
            {
                names = child;
            }
            size_t const max_names(names->get_children_size());
            for(size_t j(0); j < max_names; ++j)
            {
                Node::pointer_t super(names->get_child(j)->get_link(Node::link_t::LINK_INSTANCE));
                if(super)
                {
                    if(is_function_overloaded(super, function_node))
                    {
                        return true;
                    }
                }
            }
        }
            break;

        case Node::node_t::NODE_DIRECTIVE_LIST:
            if(find_overloaded_function(child, function_node))
            {
                return true;
            }
            break;

        case Node::node_t::NODE_FUNCTION:
            if(function_node->get_string() == child->get_string())
            {
                // found a function with the same name
                if(compare_parameters(function_node, child))
                {
                    // yes! it is overloaded!
                    return true;
                }
            }
            break;

        default:
            break;

        }
    }

    return false;
}


bool Compiler::is_function_overloaded(Node::pointer_t class_node, Node::pointer_t function_node)
{
    Node::pointer_t parent(class_of_member(function_node));
    if(!parent)
    {
        throw exception_internal_error("the parent of a function being checked for overload is not defined in a class");
    }
    if(parent->get_type() != Node::node_t::NODE_CLASS
    && parent->get_type() != Node::node_t::NODE_INTERFACE)
    {
        throw exception_internal_error("somehow the class of member is not a class or interface");
    }
    if(parent == class_node)
    {
        return false;
    }

    return find_overloaded_function(class_node, function_node);
}


bool Compiler::has_abstract_functions(Node::pointer_t class_node, Node::pointer_t list, Node::pointer_t& func)
{
    size_t max_children(list->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(list->get_child(idx));
        switch(child->get_type())
        {
        case Node::node_t::NODE_EXTENDS:
        case Node::node_t::NODE_IMPLEMENTS:
        {
            Node::pointer_t names(child->get_child(0));
            if(names->get_type() != Node::node_t::NODE_LIST)
            {
                names = child;
            }
            size_t const max_names(names->get_children_size());
            for(size_t j(0); j < max_names; ++j)
            {
                Node::pointer_t super(names->get_child(j)->get_link(Node::link_t::LINK_INSTANCE));
                if(super)
                {
                    if(has_abstract_functions(class_node, super, func))
                    {
                        return true;
                    }
                }
            }
        }
            break;

        case Node::node_t::NODE_DIRECTIVE_LIST:
            if(has_abstract_functions(class_node, child, func))
            {
                return true;
            }
            break;

        case Node::node_t::NODE_FUNCTION:
            if(is_function_abstract(child))
            {
                // see whether it was overloaded
                if(!is_function_overloaded(class_node, child))
                {
                    // not overloaded, this class cannot
                    // be instantiated!
                    func = child;
                    return true;
                }
            }
            break;

        default:
            break;

        }
    }

    return false;
}


void Compiler::can_instantiate_type(Node::pointer_t expr)
{
    if(expr->get_type() != Node::node_t::NODE_IDENTIFIER)
    {
        // dynamic, cannot test at compile time...
        return;
    }

    Node::pointer_t inst(expr->get_link(Node::link_t::LINK_INSTANCE));
    if(inst->get_type() == Node::node_t::NODE_INTERFACE)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
        msg << "you can only instantiate an object from a class. '" << expr->get_string() << "' is an interface.";
        return;
    }
    if(inst->get_type() != Node::node_t::NODE_CLASS)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
        msg << "you can only instantiate an object from a class. '" << expr->get_string() << "' does not seem to be a class.";
        return;
    }

    // check all the functions and make sure none are [still] abstract
    // in this class...
    Node::pointer_t func;
    if(has_abstract_functions(inst, inst, func))
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_ABSTRACT, expr->get_position());
        msg << "the class '"
            << expr->get_string()
            << "' has an abstract function '"
            << func->get_string()
            << "' in file '"
            << func->get_position().get_filename()
            << "' at line #"
            << func->get_position().get_line()
            << " and cannot be instantiated. (If you have an overloaded version of that function it may have the wrong prototype.)";
        return;
    }

    // we're fine...
}


void Compiler::check_this_validity(Node::pointer_t expr)
{
    Node::pointer_t parent(expr);
    for(;;)
    {
        parent = parent->get_parent();
        if(!parent)
        {
            return;
        }
        switch(parent->get_type())
        {
        case Node::node_t::NODE_FUNCTION:
            // If we are in a static function, then we
            // don't have access to 'this'. Note that
            // it doesn't matter whether we're in a
            // class or not...
            if(parent->get_flag(Node::flag_t::NODE_FUNCTION_FLAG_OPERATOR)
            || get_attribute(parent, Node::attribute_t::NODE_ATTR_STATIC)
            || get_attribute(parent, Node::attribute_t::NODE_ATTR_CONSTRUCTOR)
            || is_constructor(parent))
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_STATIC, expr->get_position());
                msg << "'this' cannot be used in a static function nor a constructor.";
            }
            return;

        case Node::node_t::NODE_CLASS:
        case Node::node_t::NODE_INTERFACE:
        case Node::node_t::NODE_PROGRAM:
        case Node::node_t::NODE_ROOT:
            return;

        default:
            break;

        }
    }
}


void Compiler::unary_operator(Node::pointer_t expr)
{
    if(expr->get_children_size() != 1)
    {
        return;
    }

    char const *op(expr->operator_to_string(expr->get_type()));
    if(!op)
    {
        throw exception_internal_error("operator_to_string() returned an empty string for a unary operator");
    }

    Node::pointer_t left(expr->get_child(0));
    Node::pointer_t type(left->get_link(Node::link_t::LINK_TYPE));
    if(!type)
    {
//fprintf(stderr, "WARNING: operand of unary operator isn't typed.\n");
        return;
    }

    Node::pointer_t l(expr->create_replacement(Node::node_t::NODE_IDENTIFIER));
    l->set_string("left");

    Node::pointer_t params(expr->create_replacement(Node::node_t::NODE_LIST));
    params->append_child(l);

    Node::pointer_t id(expr->create_replacement(Node::node_t::NODE_IDENTIFIER));
    id->set_string(op);
    id->append_child(params);

    size_t const del(expr->get_children_size());
    expr->append_child(id);

    Node::pointer_t resolution;
    int funcs = 0;
    bool result;
    {
        NodeLock ln(expr);
        result = find_field(type, id, funcs, resolution, params, 0);
    }

    expr->delete_child(del);
    if(!result)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_OPERATOR, expr->get_position());
        msg << "cannot apply operator '" << op << "' to this object.";
        return;
    }

//fprintf(stderr, "Found operator!!!\n");

    Node::pointer_t op_type(resolution->get_link(Node::link_t::LINK_TYPE));

    if(get_attribute(resolution, Node::attribute_t::NODE_ATTR_INTRINSIC))
    {
        switch(expr->get_type())
        {
        case Node::node_t::NODE_INCREMENT:
        case Node::node_t::NODE_DECREMENT:
        case Node::node_t::NODE_POST_INCREMENT:
        case Node::node_t::NODE_POST_DECREMENT:
            {
                Node::pointer_t var_node(left->get_link(Node::link_t::LINK_INSTANCE));
                if(var_node)
                {
                    if((var_node->get_type() == Node::node_t::NODE_PARAM || var_node->get_type() == Node::node_t::NODE_VARIABLE)
                    && var_node->get_flag(Node::flag_t::NODE_VAR_FLAG_CONST))
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CANNOT_OVERWRITE_CONST, expr->get_position());
                        msg << "cannot increment or decrement a constant variable or function parameters.";
                    }
                }
            }
            break;

        default:
            break;

        }
        // we keep intrinsic operators as is
//fprintf(stderr, "It is intrinsic...\n");
        expr->set_link(Node::link_t::LINK_INSTANCE, resolution);
        expr->set_link(Node::link_t::LINK_TYPE, op_type);
        return;
    }
//fprintf(stderr, "Not intrinsic...\n");

    id->set_link(Node::link_t::LINK_INSTANCE, resolution);

    // if not intrinsic, we need to transform the code
    // to a CALL instead because the lower layer won't
    // otherwise understand this operator!
    id->delete_child(0);
    id->set_link(Node::link_t::LINK_TYPE, op_type);

    // move operand in the new expression
    expr->delete_child(0);

    // TODO:
    // if the unary operator is post increment or decrement
    // then we need a temporary variable to save the current
    // value of the expression, compute the expression + 1
    // and restore the temporary

    Node::pointer_t post_list;
    Node::pointer_t assignment;
    bool const is_post = expr->get_type() == Node::node_t::NODE_POST_DECREMENT
                      || expr->get_type() == Node::node_t::NODE_POST_INCREMENT;
    if(is_post)
    {
        post_list = expr->create_replacement(Node::node_t::NODE_LIST);
        // TODO: should the list get the input type instead?
        post_list->set_link(Node::link_t::LINK_TYPE, op_type);

        Node::pointer_t temp_var(expr->create_replacement(Node::node_t::NODE_IDENTIFIER));
        temp_var->set_string("#temp_var#");

        // Save that name for next reference!
        assignment = expr->create_replacement(Node::node_t::NODE_ASSIGNMENT);
        assignment->append_child(temp_var);
        assignment->append_child(left);

        post_list->append_child(assignment);
    }

    Node::pointer_t call(expr->create_replacement(Node::node_t::NODE_CALL));
    call->set_link(Node::link_t::LINK_TYPE, op_type);
    Node::pointer_t member(expr->create_replacement(Node::node_t::NODE_MEMBER));
    Node::pointer_t function_node;
    resolve_internal_type(expr, "Function", function_node);
    member->set_link(Node::link_t::LINK_TYPE, function_node);
    call->append_child(member);

    // we need a function to get the name of 'type'
    //Data& type_data = type.GetData();
    //NodePtr object;
    //object.CreateNode(NODE_IDENTIFIER);
    //Data& obj_data = object.GetData();
    //obj_data.f_str = type_data.f_str;
    if(is_post)
    {
        // TODO: we MUST call the object defined
        //       by the left expression and NOT what
        //       I'm doing here; that's all wrong!!!
        //       for that we either need a "clone"
        //       function or a dual (or more)
        //       parenting...
        Node::pointer_t r(expr->create_replacement(Node::node_t::NODE_IDENTIFIER));
        if(left->get_type() == Node::node_t::NODE_IDENTIFIER)
        {
            r->set_string(left->get_string());
            // TODO: copy the links, flags, etc.
        }
        else
        {
            // TODO: use the same "temp var#" name
            r->set_string("#temp_var#");
        }

        member->append_child(r);
    }
    else
    {
        member->append_child(left);
    }
    member->append_child(id);

//fprintf(stderr, "NOTE: add a list (no params)\n");
    Node::pointer_t list(expr->create_replacement(Node::node_t::NODE_LIST));
    list->set_link(Node::link_t::LINK_TYPE, op_type);
    call->append_child(list);

    if(is_post)
    {
        post_list->append_child(call);

        Node::pointer_t temp_var(expr->create_replacement(Node::node_t::NODE_IDENTIFIER));
        // TODO: use the same name as used in the 1st temp_var#
        temp_var->set_string("#temp_var#");
        post_list->append_child(temp_var);

        expr->get_parent()->set_child(expr->get_offset(), post_list);
        //expr = post_list;
    }
    else
    {
        expr->get_parent()->set_child(expr->get_offset(), call);
        //expr = call;
    }

//expr.Display(stderr);
}


void Compiler::binary_operator(Node::pointer_t& expr)
{
    if(expr->get_children_size() != 2)
    {
        return;
    }

    char const *op = expr->operator_to_string(expr->get_type());
    if(!op)
    {
        throw exception_internal_error("operator_to_string() returned an empty string for a binary operator");
    }

    Node::pointer_t left(expr->get_child(0));
    Node::pointer_t ltype(left->get_link(Node::link_t::LINK_TYPE));
    if(!ltype)
    {
        return;
    }

    Node::pointer_t right(expr->get_child(1));
    Node::pointer_t rtype(right->get_link(Node::link_t::LINK_TYPE));
    if(!rtype)
    {
        return;
    }

    Node::pointer_t l(expr->create_replacement(Node::node_t::NODE_IDENTIFIER));
    l->set_string("left");
    Node::pointer_t r(expr->create_replacement(Node::node_t::NODE_IDENTIFIER));
    r->set_string("right");

    l->set_link(Node::link_t::LINK_TYPE, ltype);
    r->set_link(Node::link_t::LINK_TYPE, rtype);

    Node::pointer_t params(expr->create_replacement(Node::node_t::NODE_LIST));
    params->append_child(l);
    params->append_child(r);

    Node::pointer_t id(expr->create_replacement(Node::node_t::NODE_IDENTIFIER));
    id->set_string(op);
    id->append_child(params);

    // temporarily add a child to expr
    size_t const del(expr->get_children_size());
    expr->append_child(id);

    Node::pointer_t resolution;
    int funcs = 0;
    bool result;
    {
        NodeLock ln(expr);
        result = find_field(ltype, id, funcs, resolution, params, 0);
        if(!result)
        {
            result = find_field(rtype, id, funcs, resolution, params, 0);
        }
    }

    expr->delete_child(del);
    if(!result)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_OPERATOR, expr->get_position());
        msg << "cannot apply operator '" << op << "' to these objects.";
        return;
    }

    Node::pointer_t op_type(resolution->get_link(Node::link_t::LINK_TYPE));

    if(get_attribute(resolution, Node::attribute_t::NODE_ATTR_INTRINSIC))
    {
        // we keep intrinsic operators as is
        expr->set_link(Node::link_t::LINK_INSTANCE, resolution);
        expr->set_link(Node::link_t::LINK_TYPE, op_type);
        return;
    }

    id->set_link(Node::link_t::LINK_INSTANCE, resolution);

    // if not intrinsic, we need to transform the code
    // to a CALL instead because the lower layer won't
    // otherwise understand this operator!
//fprintf(stderr, "Not intrinsic...\n");
    id->delete_child(0);
    id->set_link(Node::link_t::LINK_TYPE, op_type);

    // move left and right in the new expression
    expr->delete_child(1);
    expr->delete_child(0);

    Node::pointer_t call(expr->create_replacement(Node::node_t::NODE_CALL));
    call->set_link(Node::link_t::LINK_TYPE, op_type);
    Node::pointer_t member(expr->create_replacement(Node::node_t::NODE_MEMBER));;
    Node::pointer_t function_node;
    resolve_internal_type(expr, "Function", function_node);
    member->set_link(Node::link_t::LINK_TYPE, function_node);
    call->append_child(member);

    // we need a function to get the name of 'type'
    //Data& type_data = type.GetData();
    //NodePtr object;
    //object.CreateNode(NODE_IDENTIFIER);
    //Data& obj_data = object.GetData();
    //obj_data.f_str = type_data.f_str;
    member->append_child(left);
    member->append_child(id);

//fprintf(stderr, "NOTE: add list (right param)\n");
    Node::pointer_t list;
    list->create_replacement(Node::node_t::NODE_LIST);
    list->set_link(Node::link_t::LINK_TYPE, op_type);
    list->append_child(right);
    call->append_child(list);

//call.Display(stderr);

    expr->replace_with(call);
}


bool Compiler::special_identifier(Node::pointer_t expr)
{
    // all special identifiers are defined as "__...__"
    // that means they are at least 5 characters and they need to
    // start with '__'

//fprintf(stderr, "ID [%.*S]\n", data.f_str.GetLength(), data.f_str.Get());
    String id(expr->get_string());
    if(id.length() < 5)
    {
        return false;
    }
    if(id[0] != '_' || id[1] != '_')
    {
        return false;
    }

    // in case an error occurs
    const char *what = "?";

    Node::pointer_t parent(expr);
    String result;
    if(id == "__FUNCTION__")
    {
        what = "a function";
        for(;;)
        {
            parent = parent->get_parent();
            if(!parent)
            {
                break;
            }
            if(parent->get_type() == Node::node_t::NODE_PACKAGE
            || parent->get_type() == Node::node_t::NODE_PROGRAM
            || parent->get_type() == Node::node_t::NODE_ROOT
            || parent->get_type() == Node::node_t::NODE_INTERFACE
            || parent->get_type() == Node::node_t::NODE_CLASS)
            {
                parent.reset();
                break;
            }
            if(parent->get_type() == Node::node_t::NODE_FUNCTION)
            {
                break;
            }
        }
    }
    else if(id == "__CLASS__")
    {
        what = "a class";
        for(;;)
        {
            parent = parent->get_parent();
            if(!parent)
            {
                break;
            }
            if(parent->get_type() == Node::node_t::NODE_PACKAGE
            || parent->get_type() == Node::node_t::NODE_PROGRAM
            || parent->get_type() == Node::node_t::NODE_ROOT)
            {
                parent.reset();
                break;
            }
            if(parent->get_type() == Node::node_t::NODE_CLASS)
            {
                break;
            }
        }
    }
    else if(id == "__INTERFACE__")
    {
        what = "an interface";
        for(;;)
        {
            parent = parent->get_parent();
            if(!parent)
            {
                break;
            }
            if(parent->get_type() == Node::node_t::NODE_PACKAGE
            || parent->get_type() == Node::node_t::NODE_PROGRAM
            || parent->get_type() == Node::node_t::NODE_ROOT)
            {
                parent.reset();
                break;
            }
            if(parent->get_type() == Node::node_t::NODE_INTERFACE)
            {
                break;
            }
        }
    }
    else if(id == "__PACKAGE__")
    {
        what = "a package";
        for(;;)
        {
            parent = parent->get_parent();
            if(!parent)
            {
                break;
            }
            if(parent->get_type() == Node::node_t::NODE_PROGRAM
            || parent->get_type() == Node::node_t::NODE_ROOT)
            {
                parent.reset();
                break;
            }
            if(parent->get_type() == Node::node_t::NODE_PACKAGE)
            {
                break;
            }
        }
    }
    else if(id == "__NAME__")
    {
        what = "any function, class, interface or package";
        for(;;)
        {
            parent = parent->get_parent();
            if(!parent)
            {
                break;
            }
            if(parent->get_type() == Node::node_t::NODE_PROGRAM
            || parent->get_type() == Node::node_t::NODE_ROOT)
            {
                parent.reset();
                break;
            }
            if(parent->get_type() == Node::node_t::NODE_FUNCTION
            || parent->get_type() == Node::node_t::NODE_CLASS
            || parent->get_type() == Node::node_t::NODE_INTERFACE
            || parent->get_type() == Node::node_t::NODE_PACKAGE)
            {
                if(result.empty())
                {
                    result = parent->get_string();
                }
                else
                {
                    // TODO: create the + operator
                    //       on String.
                    String p(parent->get_string());
                    p += ".";
                    p += result;
                    result = p;
                }
                if(parent->get_type() == Node::node_t::NODE_PACKAGE)
                {
                    // we do not really care if we
                    // are yet in another package
                    // at this time...
                    break;
                }
            }
        }
    }
    else if(id == "__TIME__")
    {
        //what = "time";
        char buf[256];
        struct tm *t;
        time_t now(f_time);
        t = localtime(&now);
        strftime(buf, sizeof(buf) - 1, "%T", t);
        buf[sizeof(buf) - 1] = '\0';
        result = buf;
    }
    else if(id == "__DATE__")
    {
        //what = "date";
        char buf[256];
        struct tm *t;
        time_t now(f_time);
        t = localtime(&now);
        strftime(buf, sizeof(buf) - 1, "%Y-%m-%d", t);
        buf[sizeof(buf) - 1] = '\0';
        result = buf;
    }
    else if(id == "__UNIXTIME__")
    {
        expr->to_int64();
        Int64 integer;
        time_t now(f_time);
        integer.set(now);
        expr->set_int64(integer);
        return true;
    }
    else if(id == "__UTCTIME__")
    {
        //what = "utctime";
        char buf[256];
        struct tm *t;
        time_t now(f_time);
        t = gmtime(&now);
        strftime(buf, sizeof(buf) - 1, "%T", t);
        buf[sizeof(buf) - 1] = '\0';
        result = buf;
    }
    else if(id == "__UTCDATE__")
    {
        //what = "utcdate";
        char buf[256];
        struct tm *t;
        time_t now(f_time);
        t = gmtime(&now);
        strftime(buf, sizeof(buf) - 1, "%Y-%m-%d", t);
        buf[sizeof(buf) - 1] = '\0';
        result = buf;
    }
    else if(id == "__DATE822__")
    {
        // Sun, 06 Nov 2005 11:57:59 -0800
        //what = "utcdate";
        char buf[256];
        struct tm *t;
        time_t now(f_time);
        t = localtime(&now);
        strftime(buf, sizeof(buf) - 1, "%a, %d %b %Y %T %z", t);
        buf[sizeof(buf) - 1] = '\0';
        result = buf;
    }
    else
    {
        // not a special identifier
        return false;
    }

    // even if it fails, we convert this expression into a string
    expr->to_string();
    if(!result.empty())
    {
        expr->set_string(result);

//fprintf(stderr, "SpecialIdentifier Result = [%.*S]\n", result.GetLength(), result.Get());

    }
    else if(!parent)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_EXPRESSION, expr->get_position());
        msg << "'" << expr->get_string() << "' was used outside " << what << ".";
        // we keep the string as is!
    }
    else
    {
        expr->set_string(parent->get_string());
    }

    return true;
}


void Compiler::type_expr(Node::pointer_t expr)
{
    // already typed?
    if(expr->get_link(Node::link_t::LINK_TYPE))
    {
        return;
    }

    Node::pointer_t resolution;

    switch(expr->get_type())
    {
    case Node::node_t::NODE_STRING:
        resolve_internal_type(expr, "String", resolution);
        expr->set_link(Node::link_t::LINK_TYPE, resolution);
        break;

    case Node::node_t::NODE_INT64:
        resolve_internal_type(expr, "Integer", resolution);
        expr->set_link(Node::link_t::LINK_TYPE, resolution);
        break;

    case Node::node_t::NODE_FLOAT64:
        resolve_internal_type(expr, "Double", resolution);
        expr->set_link(Node::link_t::LINK_TYPE, resolution);
        break;

    case Node::node_t::NODE_TRUE:
    case Node::node_t::NODE_FALSE:
        resolve_internal_type(expr, "Boolean", resolution);
        expr->set_link(Node::link_t::LINK_TYPE, resolution);
        break;

    case Node::node_t::NODE_OBJECT_LITERAL:
        resolve_internal_type(expr, "Object", resolution);
        expr->set_link(Node::link_t::LINK_TYPE, resolution);
        break;

    case Node::node_t::NODE_ARRAY_LITERAL:
        resolve_internal_type(expr, "Array", resolution);
        expr->set_link(Node::link_t::LINK_TYPE, resolution);
        break;

    default:
    {
        Node::pointer_t node(expr->get_link(Node::link_t::LINK_INSTANCE));
        if(!node)
        {
            break;
        }
        if(node->get_type() != Node::node_t::NODE_VARIABLE
        || node->get_children_size() == 0)
        {
            break;
        }
        Node::pointer_t type(node->get_child(0));
        if(type->get_type() == Node::node_t::NODE_SET)
        {
            break;
        }
        Node::pointer_t instance(type->get_link(Node::link_t::LINK_INSTANCE));
        if(!instance)
        {
            // TODO: resolve that if not done yet (it should
            //     always already be at this time)
            Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, expr->get_position());
            msg << "type is missing when it should not.";
            throw exception_exit(1, "type is missing when it should not.");
        }
        expr->set_link(Node::link_t::LINK_TYPE, instance);
    }
        break;

    }

    return;
}


void Compiler::object_literal(Node::pointer_t expr)
{
    // define the type of the literal (i.e. Object)
    type_expr(expr);

    // go through the list of names and
    //    1) make sure property names are unique
    //    2) make sure property names are proper
    //    3) compile expressions
    size_t const max_children(expr->get_children_size());
    if((max_children & 1) != 0)
    {
        // invalid?!
        // the number of children must be even to support pairs of
        // names and a values
        return;
    }

    for(size_t idx(0); idx < max_children; idx += 2)
    {
        Node::pointer_t name(expr->get_child(idx));
        size_t const cnt(name->get_children_size());
        if(name->get_type() == Node::node_t::NODE_TYPE)
        {
            // the first child is a dynamic name(space)
            expression(name->get_child(0));
            if(cnt == 2)
            {
                // TODO: this is a scope
                //    name.GetChild(0) :: name.GetChild(1)
                // ...
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_SUPPORTED, name->get_position());
                msg << "scopes not supported yet. (1)";
            }
        }
        else if(cnt == 1)
        {
            // TODO: this is a scope
            //    name :: name->get_child(0)
            // Here name is IDENTIFIER, PRIVATE or PUBLIC
            // ...
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_SUPPORTED, name->get_position());
            msg << "scopes not supported yet. (2)";
        }

        // compile the value
        Node::pointer_t value(expr->get_child(idx + 1));
        expression(value);
    }
}


void Compiler::assignment_operator(Node::pointer_t expr)
{
    bool is_var = false;

    Node::pointer_t var_node;    // in case this assignment is also a definition

    Node::pointer_t left(expr->get_child(0));
    if(left->get_type() == Node::node_t::NODE_IDENTIFIER)
    {
        // this may be like a VAR <name> = ...
        Node::pointer_t resolution;
        if(resolve_name(left, left, resolution, Node::pointer_t(), 0))
        {
            bool valid(false);
            if(resolution->get_type() == Node::node_t::NODE_VARIABLE)
            {
                if(resolution->get_flag(Node::flag_t::NODE_VAR_FLAG_CONST))
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CANNOT_OVERWRITE_CONST, left->get_position());
                    msg << "you cannot assign a value to the constant variable '" << resolution->get_string() << "'.";
                }
                else
                {
                    valid = true;
                }
            }
            else if(resolution->get_type() == Node::node_t::NODE_PARAM)
            {
                if(resolution->get_flag(Node::flag_t::NODE_PARAMETERS_FLAG_CONST))
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CANNOT_OVERWRITE_CONST, left->get_position());
                    msg << "you cannot assign a value to the constant function parameter '" << resolution->get_string() << "'.";
                }
                else
                {
                    valid = true;
                }
            }
            else
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_CANNOT_OVERLOAD, left->get_position());
                msg << "you cannot assign but a variable or a function parameter.";
            }
            if(valid)
            {
                left->set_link(Node::link_t::LINK_INSTANCE, resolution);
                left->set_link(Node::link_t::LINK_TYPE, resolution->get_link(Node::link_t::LINK_TYPE));
            }
        }
        else
        {
            // it is a missing VAR!
            is_var = true;

            // we need to put this variable in the function
            // in which it is encapsulated, if there is
            // such a function so it can be marked as local
            // for that we create a var ourselves
            Node::pointer_t variable_node;
            Node::pointer_t set;
            var_node = expr->create_replacement(Node::node_t::NODE_VAR);
            var_node->set_flag(Node::flag_t::NODE_VAR_FLAG_TOADD, true);
            var_node->set_flag(Node::flag_t::NODE_VAR_FLAG_DEFINING, true);
            variable_node = expr->create_replacement(Node::node_t::NODE_VARIABLE);
            var_node->append_child(variable_node);
            variable_node->set_string(left->get_string());
            Node::pointer_t parent(left);
            Node::pointer_t last_directive;
            for(;;)
            {
                parent = parent->get_parent();
                if(parent->get_type() == Node::node_t::NODE_DIRECTIVE_LIST)
                {
                    last_directive = parent;
                }
                else if(parent->get_type() == Node::node_t::NODE_FUNCTION)
                {
                    variable_node->set_flag(Node::flag_t::NODE_VAR_FLAG_LOCAL, true);
                    parent->add_variable(variable_node);
                    break;
                }
                else if(parent->get_type() == Node::node_t::NODE_PROGRAM
                     || parent->get_type() == Node::node_t::NODE_CLASS
                     || parent->get_type() == Node::node_t::NODE_INTERFACE
                     || parent->get_type() == Node::node_t::NODE_PACKAGE)
                {
                    // not found?!
                    break;
                }
            }
            left->set_link(Node::link_t::LINK_INSTANCE, variable_node);

            // We cannot call InsertChild()
            // here since it would be in our
            // locked parent. So instead we
            // only add it to the list of
            // variables of the directive list
            // and later we will also add it
            // at the top of the list
            if(last_directive)
            {
                //parent->insert_child(0, var_node);
                last_directive->add_variable(variable_node);
                last_directive->set_flag(Node::flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES, true);
            }
        }
    }
    else if(left->get_type() == Node::node_t::NODE_MEMBER)
    {
        // we parsed?
        if(!left->get_link(Node::link_t::LINK_TYPE))
        {
            // try to optimize the expression before to compile it
            // (it can make a huge difference!)
            f_optimizer->optimize(left);
            //Node::pointer_t right(expr->get_child(1));

            resolve_member(left, 0, SEARCH_FLAG_SETTER);

            // setters have to be treated here because within ResolveMember()
            // we do not have access to the assignment and that's what needs
            // to change to a call.
            Node::pointer_t resolution(left->get_link(Node::link_t::LINK_INSTANCE));
            if(resolution)
            {
                if(resolution->get_type() == Node::node_t::NODE_FUNCTION
                && resolution->get_flag(Node::flag_t::NODE_FUNCTION_FLAG_SETTER))
                {
                    // TODO: handle setters -- this is an old comment
                    //       maybe it was not deleted? I do not think
                    //       that these work properly yet, but it looks
                    //       like I already started work on those.
std::cerr << "CAUGHT! setter...\n";
                    // so expr is a MEMBER at this time
                    // it has two children
                    //NodePtr left = expr.GetChild(0);
                    Node::pointer_t right(expr->get_child(1));
                    //expr.DeleteChild(0);
                    //expr.DeleteChild(1);    // 1 is now 0

                    // create a new node since we don't want to move the
                    // call (expr) node from its parent.
                    //NodePtr member;
                    //member.CreateNode(NODE_MEMBER);
                    //member.SetLink(NodePtr::LINK_INSTANCE, resolution);
                    //member.AddChild(left);
                    //member.AddChild(right);
                    //member.SetLink(NodePtr::LINK_TYPE, type);

                    //expr.AddChild(left);

                    // we need to change the name to match the getter
                    // NOTE: we know that the field data is an identifier
                    //     a v-identifier or a string so the following
                    //     will always work
                    Node::pointer_t field(left->get_child(1));
                    String getter_name("<-");
                    getter_name += field->get_string();
                    field->set_string(getter_name);

                    // the call needs a list of parameters (1 parameter)
                    Node::pointer_t params(expr->create_replacement(Node::node_t::NODE_LIST));
                    /*
                    NodePtr this_expr;
                    this_expr.CreateNode(NODE_THIS);
                    params.AddChild(this_expr);
                    */
                    expr->set_child(1, params);

                    params->append_child(right);


                    // and finally, we transform the member in a call!
                    expr->to_call();
                }
            }
        }
    }
    else
    {
        // Is this really acceptable?!
        // We can certainly make it work in Macromedia Flash...
        // If the expression is resolved as a string which is
        // also a valid variable name.
        expression(left);
    }

    Node::pointer_t right(expr->get_child(1));
    expression(right);

    if(var_node)
    {
        var_node->set_flag(Node::flag_t::NODE_VAR_FLAG_DEFINING, false);
    }

    Node::pointer_t type(left->get_link(Node::link_t::LINK_TYPE));
    if(type)
    {
        expr->set_link(Node::link_t::LINK_TYPE, type);
        return;
    }

    if(!is_var)
    {
        // if left not typed, use right type!
        // (the assignment is this type of special case...)
        expr->set_link(Node::link_t::LINK_TYPE, right->get_link(Node::link_t::LINK_TYPE));
    }
}


void Compiler::expression(Node::pointer_t expr, Node::pointer_t params)
{
    // we already came here on that one?
    if(expr->get_link(Node::link_t::LINK_TYPE))
    {
        return;
    }

    // try to optimize the expression before to compile it
    // (it can make a huge difference!)
    f_optimizer->optimize(expr);

    switch(expr->get_type())
    {
    case Node::node_t::NODE_STRING:
    case Node::node_t::NODE_INT64:
    case Node::node_t::NODE_FLOAT64:
    case Node::node_t::NODE_TRUE:
    case Node::node_t::NODE_FALSE:
        type_expr(expr);
        return;

    case Node::node_t::NODE_ARRAY_LITERAL:
        type_expr(expr);
        break;

    case Node::node_t::NODE_OBJECT_LITERAL:
        object_literal(expr);
        return;

    case Node::node_t::NODE_NULL:
    case Node::node_t::NODE_PUBLIC:
    case Node::node_t::NODE_PRIVATE:
    case Node::node_t::NODE_UNDEFINED:
        return;

    case Node::node_t::NODE_SUPER:
        check_super_validity(expr);
        return;

    case Node::node_t::NODE_THIS:
        check_this_validity(expr);
        return;

    case Node::node_t::NODE_ADD:
    case Node::node_t::NODE_ARRAY:
    case Node::node_t::NODE_AS:
    case Node::node_t::NODE_ASSIGNMENT_ADD:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_AND:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_OR:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
    case Node::node_t::NODE_ASSIGNMENT_DIVIDE:
    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
    case Node::node_t::NODE_ASSIGNMENT_MAXIMUM:
    case Node::node_t::NODE_ASSIGNMENT_MINIMUM:
    case Node::node_t::NODE_ASSIGNMENT_MODULO:
    case Node::node_t::NODE_ASSIGNMENT_MULTIPLY:
    case Node::node_t::NODE_ASSIGNMENT_POWER:
    case Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case Node::node_t::NODE_ASSIGNMENT_SUBTRACT:
    case Node::node_t::NODE_BITWISE_AND:
    case Node::node_t::NODE_BITWISE_NOT:
    case Node::node_t::NODE_BITWISE_OR:
    case Node::node_t::NODE_BITWISE_XOR:
    case Node::node_t::NODE_CONDITIONAL:
    case Node::node_t::NODE_DECREMENT:
    case Node::node_t::NODE_DELETE:
    case Node::node_t::NODE_DIVIDE:
    case Node::node_t::NODE_EQUAL:
    case Node::node_t::NODE_GREATER:
    case Node::node_t::NODE_GREATER_EQUAL:
    case Node::node_t::NODE_IN:
    case Node::node_t::NODE_INCREMENT:
    case Node::node_t::NODE_INSTANCEOF:
    case Node::node_t::NODE_TYPEOF:
    case Node::node_t::NODE_IS:
    case Node::node_t::NODE_LESS:
    case Node::node_t::NODE_LESS_EQUAL:
    case Node::node_t::NODE_LIST:
    case Node::node_t::NODE_LOGICAL_AND:
    case Node::node_t::NODE_LOGICAL_NOT:
    case Node::node_t::NODE_LOGICAL_OR:
    case Node::node_t::NODE_LOGICAL_XOR:
    case Node::node_t::NODE_MATCH:
    case Node::node_t::NODE_MAXIMUM:
    case Node::node_t::NODE_MINIMUM:
    case Node::node_t::NODE_MODULO:
    case Node::node_t::NODE_MULTIPLY:
    case Node::node_t::NODE_NOT_EQUAL:
    case Node::node_t::NODE_POST_DECREMENT:
    case Node::node_t::NODE_POST_INCREMENT:
    case Node::node_t::NODE_POWER:
    case Node::node_t::NODE_RANGE:
    case Node::node_t::NODE_ROTATE_LEFT:
    case Node::node_t::NODE_ROTATE_RIGHT:
    case Node::node_t::NODE_SCOPE:
    case Node::node_t::NODE_SHIFT_LEFT:
    case Node::node_t::NODE_SHIFT_RIGHT:
    case Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case Node::node_t::NODE_STRICTLY_EQUAL:
    case Node::node_t::NODE_STRICTLY_NOT_EQUAL:
    case Node::node_t::NODE_SUBTRACT:
        break;

    case Node::node_t::NODE_NEW:
        // TBD: we later check whether we can instantiate this 'expr'
        //      object; but if we return here, then that test will
        //      be skipped (unless the return is inapropriate or
        //      we should have if(!expression_new(expr)) ...
        if(expression_new(expr))
        {
            return;
        }
        break;

    case Node::node_t::NODE_VOID:
        // If the expression has no side effect (i.e. doesn't
        // call a function, doesn't use ++ or --, etc.) then
        // we don't even need to keep it! Instead we replace
        // the void by undefined.
        if(expr->has_side_effects())
        {
            // we need to keep some of this expression
            //
            // TODO: we need to optimize better; this
            // should only keep expressions with side
            // effects and not all expressions; for
            // instance:
            //    void (a + b(c));
            // should become:
            //    void b(c);
            // (assuming that 'a' isn't a call to a getter
            // function which could have a side effect)
            break;
        }
        // this is what void returns, assuming the expression
        // had no side effect, that's all we need here
        expr = expr->create_replacement(Node::node_t::NODE_UNDEFINED);
        return;

    case Node::node_t::NODE_ASSIGNMENT:
        assignment_operator(expr);
        return;

    case Node::node_t::NODE_FUNCTION:
        function(expr);
        return;

    case Node::node_t::NODE_MEMBER:
        resolve_member(expr, params, SEARCH_FLAG_GETTER);
        return;

    case Node::node_t::NODE_IDENTIFIER:
    case Node::node_t::NODE_VIDENTIFIER:
        if(!special_identifier(expr))
        {
            Node::pointer_t resolution;
            if(resolve_name(expr, expr, resolution, params, SEARCH_FLAG_GETTER))
            {
                if(!replace_constant_variable(expr, resolution))
                {
                    Node::pointer_t current(expr->get_link(Node::link_t::LINK_INSTANCE));
                    if(current)
                    {
                        // TBD: I'm not exactly sure what this does right now, we
                        //      probably can ameliorate the error message, although
                        //      we should actually never get it!
                        throw exception_internal_error("The link instance of this VIDENTIFIER was already defined...");
                    }
                    expr->set_link(Node::link_t::LINK_INSTANCE, resolution);
                    Node::pointer_t type(resolution->get_link(Node::link_t::LINK_TYPE));
                    if(type)
                    {
                        expr->set_link(Node::link_t::LINK_TYPE, type);
                    }
                }
            }
            else
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_FOUND, expr->get_position());
                msg << "cannot find any variable or class declaration for: '" << expr->get_string() << "'.";
            }
        }
        return;

    case Node::node_t::NODE_CALL:
        resolve_call(expr);
        return;

    default:
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, expr->get_position());
            msg << "unhandled expression data type \"" << expr->get_type_name() << "\".";
        }
        return;

    }

// When not returned yet, we want that expression to
// compile all the children nodes as expressions.
    size_t const max_children(expr->get_children_size());
    {
        NodeLock ln(expr);
        for(size_t idx(0); idx < max_children; ++idx)
        {
            Node::pointer_t child(expr->get_child(idx));
            // skip labels
            if(child->get_type() != Node::node_t::NODE_NAME)
            {
                expression(child); // recursive!
            }
        }
    }

// Now check for operators to give them a type
    switch(expr->get_type())
    {
    case Node::node_t::NODE_ADD:
    case Node::node_t::NODE_SUBTRACT:
        if(max_children == 1)
        {
            unary_operator(expr);
        }
        else
        {
            binary_operator(expr);
        }
        break;

    case Node::node_t::NODE_BITWISE_NOT:
    case Node::node_t::NODE_DECREMENT:
    case Node::node_t::NODE_INCREMENT:
    case Node::node_t::NODE_LOGICAL_NOT:
    case Node::node_t::NODE_POST_DECREMENT:
    case Node::node_t::NODE_POST_INCREMENT:
        unary_operator(expr);
        break;

    case Node::node_t::NODE_BITWISE_AND:
    case Node::node_t::NODE_BITWISE_OR:
    case Node::node_t::NODE_BITWISE_XOR:
    case Node::node_t::NODE_DIVIDE:
    case Node::node_t::NODE_EQUAL:
    case Node::node_t::NODE_GREATER:
    case Node::node_t::NODE_GREATER_EQUAL:
    case Node::node_t::NODE_LESS:
    case Node::node_t::NODE_LESS_EQUAL:
    case Node::node_t::NODE_LOGICAL_AND:
    case Node::node_t::NODE_LOGICAL_OR:
    case Node::node_t::NODE_LOGICAL_XOR:
    case Node::node_t::NODE_MATCH:
    case Node::node_t::NODE_MAXIMUM:
    case Node::node_t::NODE_MINIMUM:
    case Node::node_t::NODE_MODULO:
    case Node::node_t::NODE_MULTIPLY:
    case Node::node_t::NODE_NOT_EQUAL:
    case Node::node_t::NODE_POWER:
    case Node::node_t::NODE_RANGE:
    case Node::node_t::NODE_ROTATE_LEFT:
    case Node::node_t::NODE_ROTATE_RIGHT:
    case Node::node_t::NODE_SCOPE:
    case Node::node_t::NODE_SHIFT_LEFT:
    case Node::node_t::NODE_SHIFT_RIGHT:
    case Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case Node::node_t::NODE_STRICTLY_EQUAL:
    case Node::node_t::NODE_STRICTLY_NOT_EQUAL:
        binary_operator(expr);
        break;

    case Node::node_t::NODE_IN:
    case Node::node_t::NODE_CONDITIONAL:    // cannot be overwritten!
        break;

    case Node::node_t::NODE_ARRAY:
    case Node::node_t::NODE_ARRAY_LITERAL:
    case Node::node_t::NODE_AS:
    case Node::node_t::NODE_DELETE:
    case Node::node_t::NODE_INSTANCEOF:
    case Node::node_t::NODE_IS:
    case Node::node_t::NODE_TYPEOF:
    case Node::node_t::NODE_VOID:
        // nothing special we can do here...
        break;

    case Node::node_t::NODE_NEW:
        can_instantiate_type(expr->get_child(0));
        break;

    case Node::node_t::NODE_LIST:
        {
            // this is the type of the last entry
            Node::pointer_t child(expr->get_child(max_children - 1));
            expr->set_link(Node::link_t::LINK_TYPE, child->get_link(Node::link_t::LINK_TYPE));
        }
        break;

    case Node::node_t::NODE_ASSIGNMENT_ADD:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_AND:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_OR:
    case Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
    case Node::node_t::NODE_ASSIGNMENT_DIVIDE:
    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
    case Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
    case Node::node_t::NODE_ASSIGNMENT_MAXIMUM:
    case Node::node_t::NODE_ASSIGNMENT_MINIMUM:
    case Node::node_t::NODE_ASSIGNMENT_MODULO:
    case Node::node_t::NODE_ASSIGNMENT_MULTIPLY:
    case Node::node_t::NODE_ASSIGNMENT_POWER:
    case Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case Node::node_t::NODE_ASSIGNMENT_SUBTRACT:
        // TODO: we need to replace the intrinsic special
        //       assignment ops with a regular assignment
        //       (i.e. a += b becomes a = a + (b))
        binary_operator(expr);
        break;

    default:
        throw exception_internal_error("error: there is a missing entry in the 2nd switch of Compiler::expression()");

    }
}



}
// namespace as2js

// vim: ts=4 sw=4 et
