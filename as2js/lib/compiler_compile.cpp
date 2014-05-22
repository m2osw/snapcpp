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

#include    "as2js/message.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  COMPILE  ********************************************************/
/**********************************************************************/
/**********************************************************************/

/* The following functions "compile" the code.
 *
 * This mainly means that it (1) tries to resolve all the references
 * that are found in the current tree; (2) load the libraries referenced
 * by the different import instructions which are necessary (or at least
 * seem to be).
 *
 * If you also want to optimize the tree, you will need to call the
 * Optimize() function after you compiled. This will optimize expressions
 * such as 5 + 13 to just 18. This needs to happen at the end because the
 * reference resolution can endup in the replacement of an identifier by
 * a literal which can then be optimized. Trying to optimize too soon
 * would miss a large percentage of possible optimizations.
 */

int Compiler::compile(Node::pointer_t& root)
{
    // all the "use namespace ... / with ..." currently in effect
    f_scope = root->create_replacement(Node::NODE_SCOPE);

    if(root)
    {
        if(root->get_type() == Node::NODE_PROGRAM)
        {
            program(root);
        }
        else if(root->get_type() == Node::NODE_ROOT)
        {
            NodeLock ln(root);
            size_t const max_children(root->get_children_size());
            for(size_t idx(0); idx < max_children; ++idx)
            {
                Node::pointer_t child(root->get_child(idx));
                if(child->get_type() == Node::NODE_PROGRAM)
                {
                    program(child);
                }
            }
        }
        else
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INTERNAL_ERROR, root->get_position());
            msg << "the Compiler::compile() function expected a root or a program node to start with.";
        }
    }

    return Message::error_count();
}






void Compiler::program(Node::pointer_t& program_node)
{
    // This is the root. Whenever you search to resolve a reference,
    // don't go past that node! What's in the parent of a program is
    // not part of that program...
    f_program = program_node;

#if 0
std::cerr << program_node;
#endif
    // get rid of any declaration marked false
    size_t const org_max(program_node->get_children_size());
    for(size_t idx(0); idx < org_max; ++idx)
    {
        Node::pointer_t child(program_node->get_child(idx));
        if(get_attribute(child, Node::NODE_ATTR_FALSE))
        {
            child->to_unknown();
        }
    }
    program_node->clean_tree();

    NodeLock ln(program_node);

    // look for all the labels in this program (for goto's)
    for(size_t idx(0); idx < org_max; ++idx)
    {
        Node::pointer_t child(program_node->get_child(idx));
        if(child->get_type() == Node::NODE_DIRECTIVE_LIST)
        {
            find_labels(program_node, child);
        }
    }

    // a program is composed of directives (usually just one list)
    // which we want to compile
    for(size_t idx(0); idx < org_max; ++idx)
    {
        Node::pointer_t child(program_node->get_child(idx));
        if(child->get_type() == Node::NODE_DIRECTIVE_LIST)
        {
            directive_list(child);
        }
    }

#if 0
if(Message::error_count() > 0)
std::cerr << program_node;
#endif
}



void Compiler::var(Node::pointer_t& var_node)
{
    // when variables are used, they are initialized
    // here, we initialize them only if they have
    // side effects; this is because a variable can
    // be used as an attribute and it would often
    // end up as an error (i.e. attributes not
    // found as identifier(s) defining another
    // object)
    NodeLock ln(var_node);
    size_t const vcnt(var_node->get_children_size());
    for(size_t v(0); v < vcnt; ++v)
    {
        Node::pointer_t variable_node(var_node->get_child(v));
        variable(variable_node, true);
    }
}


void Compiler::variable(Node::pointer_t& variable_node, bool const side_effects_only)
{
    size_t const max_children(variable_node->get_children_size());

    // if we already have a type, we have been parsed
    if(variable_node->get_flag(Node::NODE_VAR_FLAG_DEFINED)
    || variable_node->get_flag(Node::NODE_VAR_FLAG_ATTRIBUTES))
    {
        if(!side_effects_only)
        {
            if(!variable_node->get_flag(Node::NODE_VAR_FLAG_COMPILED))
            {
                for(size_t idx(0); idx < max_children; ++idx)
                {
                    Node::pointer_t child(variable_node->get_child(idx));
                    if(child->get_type() == Node::NODE_SET)
                    {
                        Node::pointer_t expr(child->get_child(0));
                        expression(expr);
                        variable_node->set_flag(Node::NODE_VAR_FLAG_COMPILED, true);
                        break;
                    }
                }
            }
            variable_node->set_flag(Node::NODE_VAR_FLAG_INUSE, true);
        }
        return;
    }

    variable_node->set_flag(Node::NODE_VAR_FLAG_DEFINED, true);
    variable_node->set_flag(Node::NODE_VAR_FLAG_INUSE, !side_effects_only);

    bool const constant(variable_node->get_flag(Node::NODE_VAR_FLAG_CONST));

    // make sure to get the attributes before the node gets locked
    // (we know that the result is true in this case)
    get_attribute(variable_node, Node::NODE_ATTR_DEFINED);

    NodeLock ln(variable_node);
    int set(0);

    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(variable_node->get_child(idx));
        switch(child->get_type())
        {
        case Node::NODE_UNKNOWN:
            break;

        case Node::NODE_SET:
            {
                Node::pointer_t expr(child->get_child(0));
                if(expr->get_type() == Node::NODE_PRIVATE
                || expr->get_type() == Node::NODE_PUBLIC)
                {
                    // this is a list of attributes
                    ++set;
                }
                else if(set == 0
                     && (!side_effects_only || expr->has_side_effects()))
                {
                    expression(expr);
                    expr->set_flag(Node::NODE_VAR_FLAG_COMPILED, true);
                    expr->set_flag(Node::NODE_VAR_FLAG_INUSE, true);
                }
                ++set;
            }
            break;

        default:
            // define the variable type in this case
            expression(child);
            if(!variable_node->get_link(Node::LINK_TYPE))
            {
                variable_node->set_link(Node::LINK_TYPE, child->get_link(Node::LINK_INSTANCE));
            }
            break;

        }
    }

    if(set > 1)
    {
        variable_node->to_var_attributes();
        if(!constant)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_NEED_CONST, variable_node->get_position());
            msg << "a variable cannot be a list of attributes unless it is made constant and '" << variable_node->get_string() << "' is not constant.";
        }
    }
    else
    {
        // read the initializer (we're expecting an expression, but
        // if this is only one identifier or PUBLIC or PRIVATE then
        // we're in a special case...)
        add_variable(variable_node);
    }
}


void Compiler::add_variable(Node::pointer_t& variable_node)
{
    // For variables, we want to save a link in the
    // first directive list; this is used to clear
    // all the variables whenever a frame is left
    // and enables us to declare local variables as
    // such in functions
    //
    // [i.e. local variables defined in a frame are
    // undefined once you quit that frame; we do that
    // because the Flash instructions don't give us
    // correct frame management and a goto inside a
    // frame would otherwise possibly use the wrong
    // variable value!]
    Node::pointer_t parent(variable_node);
    bool first(true);
    for(;;)
    {
        parent = parent->get_parent();
        switch(parent->get_type())
        {
        case Node::NODE_DIRECTIVE_LIST:
            if(first)
            {
                first = false;
                parent->add_variable(variable_node);
            }
            break;

        case Node::NODE_FUNCTION:
            // mark the variable as local
            variable_node->set_flag(Node::NODE_VAR_FLAG_LOCAL, true);
            if(first)
            {
                parent->add_variable(variable_node);
            }
            return;

        case Node::NODE_CLASS:
        case Node::NODE_INTERFACE:
            // mark the variable as a member of this class or interface
            variable_node->set_flag(Node::NODE_VAR_FLAG_MEMBER, true);
            if(first)
            {
                parent->add_variable(variable_node);
            }
            return;

        case Node::NODE_PROGRAM:
        case Node::NODE_PACKAGE:
            // variable is global
            if(first)
            {
                parent->add_variable(variable_node);
            }
            return;

        default:
            break;

        }
    }
}



void Compiler::with(Node::pointer_t& with_node)
{
    size_t const max_children(with_node->get_children_size());
    if(max_children != 2)
    {
        // invalid, ignore
        return;
    }
    NodeLock ln(with_node);

    // object name defined in an expression
    // (used to resolve identifiers as members in the following
    // expressions until it gets popped)
    Node::pointer_t object(with_node->get_child(0));

    if(object->get_type() == Node::NODE_THIS)
    {
        // TODO: could we avoid erring here?!
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_EXPRESSION, object->get_position());
        msg << "'with' cannot use 'this' as an object.";
    }

//fprintf(stderr, "Resolving WITH object...\n");

    expression(object);

    // we create two nodes; one so we know we have a WITH instruction
    // and a child of that node which is the object itself; these are
    // deleted once we return from this function
    //NodePtr t;
    //t.CreateNode();
    //t.SetData(object.GetData());
    //NodePtr w;
    //w.CreateNode(NODE_WITH);
    //w.AddChild(t);
    //int p = f_scope.GetChildCount();
    //f_scope.AddChild(w);

    Node::pointer_t sub_directives(with_node->get_child(1));
    directive_list(sub_directives);

    // the effect of this with() ends with the end of its
    // list of directives
    //f_scope.DeleteChild(p);
}



/** \brief Compile the goto directive.
 *
 * Note that JavaScript in browsers do not support the goto instruction.
 * They have a similar behavior when using while() loop and either a
 * continue (goto at the start) or the break (goto after the while()
 * loop.).
 *
 * This function is kept here, although we are very unlikely to implement
 * the instruction in your browser, it may end up being useful in case
 * we again work on ActionScript.
 *
 * \param[in] goto_node  The node representing the goto statement.
 */
void Compiler::goto_directive(Node::pointer_t& goto_node)
{
    Node::vector_of_pointers_t parents;
    Node::pointer_t label;
    Node::pointer_t parent(goto_node);
    do
    {
        parent = parent->get_parent();
        if(!parent)
        {
            Message msg(MESSAGE_LEVEL_FATAL, AS_ERR_INTERNAL_ERROR, goto_node->get_position());
            msg << "Compiler::goto(): out of parents before we find function, program or package parent?!";
            exit(1);
        }

        switch(parent->get_type())
        {
        case Node::NODE_CLASS:
        case Node::NODE_INTERFACE:
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_IMPROPER_STATEMENT, goto_node->get_position());
                msg << "cannot have a GOTO instruction in a 'class' or 'interface'.";
            }
            return;

        case Node::NODE_FUNCTION:
        case Node::NODE_PACKAGE:
        case Node::NODE_PROGRAM:
            label = parent->find_label(goto_node->get_string());
            if(!label)
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_LABEL_NOT_FOUND, goto_node->get_position());
                msg << "label '" << goto_node->get_string() << "' for goto instruction not found.";
                return;
            }
            break;

        // We most certainly want to test those with some user
        // options to know whether we should accept or refuse
        // inter-frame gotos
        //case Node::NODE_WITH:
        //case Node::NODE_TRY:
        //case Node::NODE_CATCH:
        //case Node::NODE_FINALLY:

        default:
            break;

        }
        parents.push_back(parent);
    }
    while(!label);
    goto_node->set_link(Node::LINK_GOTO_ENTER, label);

    // Now we have to do the hardest part:
    //    find the common parent frame where both, the goto
    //    and the label can be found
    //    for this purpose we created an array with all the
    //    frames (parents) and then we search that array with each
    //    parent of the label

    parent = label;
    for(;;)
    {
        parent = parent->get_parent();
        if(!parent)
        {
            // never found a common parent?!
            Message msg(MESSAGE_LEVEL_FATAL, AS_ERR_INTERNAL_ERROR, goto_node->get_position());
            msg << "Compiler::goto(): out of parent before we find the common node?!";
            exit(1);
        }
        for(size_t idx(0); idx < parents.size(); ++idx)
        {
            if(parents[idx] == parent)
            {
                // found the first common parent
                goto_node->set_link(Node::LINK_GOTO_EXIT, parent);
                return;
            }
        }
    }
}



void Compiler::for_directive(Node::pointer_t& for_node)
{
    // support for the two forms: for(foo in blah) ... and for(a;b;c) ...
    // (Note: first case we have 3 children: foo, blah, directives
    //        second case we have 4 children: a, b, c, directives
    size_t max(for_node->get_children_size());
    if(max < 3)
    {
        return;
    }
    NodeLock ln(for_node);

    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child(for_node->get_child(idx));
        switch(child->get_type())
        {
        case Node::NODE_EMPTY:
            // do nothing
            break;

        case Node::NODE_DIRECTIVE_LIST:
            directive_list(child);
            break;

        case Node::NODE_VAR:
            var(child);
            break;

        default:    // expression
            expression(child);
            break;

        }
    }
}


void Compiler::switch_directive(Node::pointer_t& switch_node)
{
    size_t const max_children(switch_node->get_children_size());
    if(max_children != 2)
    {
        return;
    }

    NodeLock ln_sn(switch_node);
    expression(switch_node->get_child(0));

    // make sure that the list of directive starts
    // with a label [this is a requirement which
    // really makes sense but the parser does not
    // enforce it]
    Node::pointer_t directive_list_node(switch_node->get_child(1));
    size_t const max_directives(directive_list_node->get_children_size());
    if(max_directives > 0)
    {
        Node::pointer_t child(directive_list_node->get_child(0));
        if(child->get_type() != Node::NODE_CASE
        && child->get_type() != Node::NODE_DEFAULT)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INACCESSIBLE_STATEMENT, switch_node->get_position());
            msg << "the list of instructions of a 'switch()' statement must start with a 'case' or 'default' label.";
        }
    }
    // else -- should we warn when empty?

    directive_list(directive_list_node);

    // reset the DEFAULT flag just in case we get compiled a second
    // time (which happens when testing for missing return statements)
    switch_node->set_flag(Node::NODE_SWITCH_FLAG_DEFAULT, false);

    // TODO: If EQUAL or STRICTLY EQUAL we may
    //       want to check for duplicates.
    //       (But cases can be dynamic so it
    //       does not really make sense, does it?!)
}


void Compiler::case_directive(Node::pointer_t& case_node)
{
    // make sure it was used inside a switch statement
    // (the parser doesn't enforce it)
    Node::pointer_t parent(case_node->get_parent());
    if(!parent)
    {
        // ?!?
        return;
    }
    parent = parent->get_parent();
    if(!parent)
    {
        // ?!?
        return;
    }
    if(parent->get_type() != Node::NODE_SWITCH)
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_IMPROPER_STATEMENT, case_node->get_position());
        msg << "a 'case' statement can only be used within a 'switch()' block.";
        return;
    }

    size_t const max_children(case_node->get_children_size());
    if(max_children > 0)
    {
        expression(case_node->get_child(0));
        if(max_children > 1)
        {
            switch(parent->get_switch_operator())
            {
            case Node::NODE_UNKNOWN:
            case Node::NODE_IN:
                break;

            default:
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_EXPRESSION, case_node->get_position());
                    msg << "a range on a 'case' statement can only be used with the 'in' and 'default' switch() operators.";
                }
                break;

            }
            expression(case_node->get_child(1));
        }
    }
}


void Compiler::default_directive(Node::pointer_t& default_node)
{
    // make sure it was used inside a switch statement
    // (the parser doesn't enforce it)
    Node::pointer_t parent(default_node->get_parent());
    if(!parent) {
        // ?!?
        return;
    }
    parent = parent->get_parent();
    if(!parent) {
        // ?!?
        return;
    }
    if(parent->get_type() != Node::NODE_SWITCH)
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INACCESSIBLE_STATEMENT, default_node->get_position());
        msg << "a 'default' statement can only be used within a 'switch()' block.";
        return;
    }

    if(parent->get_flag(Node::NODE_SWITCH_FLAG_DEFAULT))
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_IMPROPER_STATEMENT, default_node->get_position());
        msg << "only one 'default' statement can be used within one 'switch()'.";
    }
    else
    {
        parent->set_flag(Node::NODE_SWITCH_FLAG_DEFAULT, true);
    }
}


void Compiler::if_directive(Node::pointer_t& if_node)
{
    size_t const max_children(if_node->get_children_size());
    if(max_children < 2)
    {
        return;
    }
    NodeLock ln(if_node);

    // TBD: check whether the first expression
    //      is a valid boolean? (for strict mode
    //      maybe, but JavaScript is very lax on
    //      just like C/C++)
    expression(if_node->get_child(0));
    directive_list(if_node->get_child(1));
    if(max_children == 3)
    {
        // else part
        directive_list(if_node->get_child(2));
    }
}



void Compiler::while_directive(Node::pointer_t& while_node)
{
    size_t const max_children(while_node->get_children_size());
    if(max_children != 2)
    {
        return;
    }
    NodeLock ln(while_node);

    // If the first expression is a constant boolean,
    // the optimizer will replace the while()
    // loop in a loop forever; or remove it entirely.
    expression(while_node->get_child(0));
    directive_list(while_node->get_child(1));
}


void Compiler::do_directive(Node::pointer_t& do_node)
{
    size_t const max_children(do_node->get_children_size());
    if(max_children != 2)
    {
        return;
    }
    NodeLock ln(do_node);

    // If the second expression is a constant boolean,
    // the optimizer will replace the do/while()
    // loop in a loop forever; or execute the first
    // list of directives once.
    directive_list(do_node->get_child(0));
    expression(do_node->get_child(1));
}


void Compiler::break_continue(Node::pointer_t& break_node)
{
    bool const no_label(break_node->get_string().empty());
    bool const accept_switch(!no_label || break_node->get_type() == Node::NODE_BREAK);
    bool found_switch(false);
    Node::pointer_t parent(break_node);
    for(;;)
    {
        parent = parent->get_parent();
        if(parent->get_type() == Node::NODE_SWITCH)
        {
            found_switch = true;
        }
        if((parent->get_type() == Node::NODE_SWITCH && accept_switch)
        || parent->get_type() == Node::NODE_FOR
        || parent->get_type() == Node::NODE_DO
        || parent->get_type() == Node::NODE_WHILE)
        {
            if(no_label)
            {
                // just break the current 'switch', 'for',
                // 'while', 'do' when there is no name.
                break;
            }
            // check whether this statement has a label
            // and whether it matches the requested name
            int32_t const offset(parent->get_offset());
            if(offset > 0)
            {
                Node::pointer_t p(parent->get_parent());
                Node::pointer_t previous(p->get_child(offset - 1));
                if(previous->get_type() == Node::NODE_LABEL
                && previous->get_string() == break_node->get_string())
                {
                    // found a match
                    break;
                }
            }
        }
        if(parent->get_type() == Node::NODE_FUNCTION
        || parent->get_type() == Node::NODE_PROGRAM
        || parent->get_type() == Node::NODE_CLASS       // ?!
        || parent->get_type() == Node::NODE_INTERFACE   // ?!
        || parent->get_type() == Node::NODE_PACKAGE)
        {
            // not found?! a break/continue outside a loop or
            // switch?! or the label was not found
            if(no_label)
            {
                if(found_switch)
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_IMPROPER_STATEMENT, break_node->get_position());
                    msg << "you cannot use a continue statement outside a loop (and you need a label to make it work with a switch statement).";
                }
                else
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_IMPROPER_STATEMENT, break_node->get_position());
                    msg << "you cannot use a break or continue instruction outside a loop or switch statement.";
                }
            }
            else
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_LABEL_NOT_FOUND, break_node->get_position());
                msg << "could not find a loop or switch statement labelled '" << break_node->get_string() << "' for this break or continue.";
            }
            return;
        }
    }

    // We just specify which node needs to be reached
    // on this break/continue.
    //
    // We do not replace these with a simple goto instruction
    // because that way the person using the tree later can
    // program the break and/or continue the way they feel
    // (using a variable, a special set of instructions,
    // etc. so as to be able to unwind all the necessary
    // data in a way specific to the break/continue).
    //
    // Also in browsers, JavaScript does not offer a goto.
    break_node->set_link(Node::LINK_GOTO_EXIT, parent);
}



void Compiler::throw_directive(Node::pointer_t& throw_node)
{
    if(throw_node->get_children_size() != 1)
    {
        return;
    }

    expression(throw_node->get_child(0));
}


void Compiler::try_directive(Node::pointer_t& try_node)
{
    if(try_node->get_children_size() != 1)
    {
        return;
    }

    // we want to make sure that we are followed
    // by a catch or a finally
    Node::pointer_t parent(try_node->get_parent());
    bool correct(false);
    size_t const max_parent_children(parent->get_children_size());
    size_t const offset(static_cast<size_t>(try_node->get_offset()) + 1);
    if(offset < max_parent_children)
    {
        Node::pointer_t next(parent->get_child(offset));
        if(next->get_type() == Node::NODE_CATCH
        || next->get_type() == Node::NODE_FINALLY)
        {
            correct = true;
        }
    }
    if(!correct)
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_TRY, try_node->get_position());
        msg << "a 'try' statement needs to be followed by at least one of 'catch' or 'finally'.";
    }

    directive_list(try_node->get_child(0));
}


void Compiler::catch_directive(Node::pointer_t& catch_node)
{
    if(catch_node->get_children_size() != 2)
    {
        return;
    }

    // we want to make sure that we are preceded by a try
    Node::pointer_t parent(catch_node->get_parent());
    bool correct(false);
    int32_t const offset(catch_node->get_offset());
    if(offset > 0)
    {
        Node::pointer_t prev(parent->get_child(offset - 1));
        if(prev->get_type() == Node::NODE_TRY)
        {
            correct = true;
        }
        else if(prev->get_type() == Node::NODE_CATCH)
        {
            correct = true;

            // correct syntactically, however, the previous catch
            // must clearly be typed
            if(!prev->get_flag(Node::NODE_CATCH_FLAG_TYPED))
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_TYPE, catch_node->get_position());
                msg << "only the last 'catch' statement can have a parameter without a valid type.";
            }
        }
    }
    if(!correct)
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_IMPROPER_STATEMENT, catch_node->get_position());
        msg << "a 'catch' statement needs to be preceded by a 'try' or another typed 'catch' statement.";
    }

    Node::pointer_t parameters_node(catch_node->get_child(0));
    parameters(parameters_node);
    if(parameters_node->get_children_size() > 0)
    {
        Node::pointer_t param(parameters_node->get_child(0));
        param->set_flag(Node::NODE_PARAMETERS_FLAG_CATCH, true);
    }

    directive_list(catch_node->get_child(1));
}


void Compiler::finally(Node::pointer_t& finally_node)
{
    if(finally_node->get_children_size() != 1)
    {
        return;
    }

    // we want to make sure that we are preceded by a try or a catch
    Node::pointer_t parent(finally_node->get_parent());
    bool correct(false);
    int32_t const offset(finally_node->get_offset());
    if(offset > 0)
    {
        Node::pointer_t prev(parent->get_child(offset - 1));
        if(prev->get_type() == Node::NODE_TRY
        || prev->get_type() == Node::NODE_CATCH)
        {
            correct = true;
        }
    }
    if(!correct)
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_IMPROPER_STATEMENT, finally_node->get_position());
        msg << "a 'finally' statement needs to be preceded by a 'try' or 'catch' statement.";
    }

    directive_list(finally_node->get_child(0));
}



/** \brief Check whether that function was not marked as final before.
 *
 * \return true if the function is marked as final in a super definition.
 */
bool Compiler::find_final_functions(Node::pointer_t& function_node, Node::pointer_t& super)
{
    size_t const max_children(super->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(super->get_child(idx));
        switch(child->get_type())
        {
        case Node::NODE_EXTENDS:
        {
            Node::pointer_t next_super(child->get_link(Node::LINK_INSTANCE));
            if(next_super)
            {
                if(find_final_functions(function_node, next_super)) // recursive
                {
                    return true;
                }
            }
        }
            break;

        case Node::NODE_DIRECTIVE_LIST:
            if(find_final_functions(function_node, child)) // recursive
            {
                return true;
            }
            break;

        case Node::NODE_FUNCTION:
            // TBD: are we not also expected to check the number of
            //      parameter to know that it is the same function?
            //      (see compare_parameters() below)
            if(function_node->get_string() == child->get_string())
            {
                // we found a function of the same name
                if(get_attribute(child, Node::NODE_ATTR_FINAL))
                {
                    // Ooops! it was final...
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


/** \brief Check whether that function was not marked as final before.
 *
 * \return true if the function is marked as final in a super definition.
 */
bool Compiler::check_final_functions(Node::pointer_t& function_node, Node::pointer_t& class_node)
{
    size_t const max_children(class_node->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(class_node->get_child(idx));

        // NOTE: there can be only one 'extends'
        //
        // TODO: we most certainly can support more than one extend in
        //       JavaScript, although it is not 100% clean, but we can
        //       make it work so we will have to enhance this test
        if(child->get_type() == Node::NODE_EXTENDS
        && child->get_children_size() > 0)
        {
            // this points to another class which may define
            // the same function as final
            Node::pointer_t name(child->get_child(0));
            Node::pointer_t super(name->get_link(Node::LINK_INSTANCE));
            if(super)
            {
                return find_final_functions(function_node, super);
            }
            break;
        }
    }

    return false;
}


bool Compiler::compare_parameters(Node::pointer_t& lfunction, Node::pointer_t& rfunction)
{
    // search for the list of parameters in each function
    Node::pointer_t lparams(lfunction->find_first_child(Node::NODE_PARAMETERS));
    Node::pointer_t rparams(rfunction->find_first_child(Node::NODE_PARAMETERS));

    // get the number of parameters in each list
    size_t const lmax(lparams ? lparams->get_children_size() : 0);
    size_t const rmax(rparams ? rparams->get_children_size() : 0);

    // if we do not have the same number of parameters, already, we know it
    // is not the same, even if one has just a rest in addition
    if(lmax != rmax)
    {
        return false;
    }

    // same number, compare the types
    for(size_t idx(0); idx < lmax; ++idx)
    {
        // Get the PARAM
        Node::pointer_t lp(lparams->get_child(idx));
        Node::pointer_t rp(rparams->get_child(idx));
        // Get the type of each PARAM
        // TODO: test that lp and rp have at least one child?
        Node::pointer_t l(lp->get_child(0));
        Node::pointer_t r(rp->get_child(0));
        // We can directly compare strings and identifiers
        // Anything else fails meaning that we consider them equal
        if((l->get_type() != Node::NODE_IDENTIFIER && l->get_type() != Node::NODE_STRING)
        || (r->get_type() != Node::NODE_IDENTIFIER && r->get_type() != Node::NODE_STRING))
        {
            // if we cannot compare at compile time,
            // we consider the types as equal... (i.e. match!)
            continue;
        }
        if(l->get_string() != r->get_string())
        {
            return false;
        }
    }

    return true;
}



bool Compiler::check_unique_functions(Node::pointer_t function_node, Node::pointer_t class_node, bool const all_levels)
{
    size_t const max(class_node->get_children_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child(class_node->get_child(idx));
        switch(child->get_type())
        {
        case Node::NODE_DIRECTIVE_LIST:
            if(all_levels)
            {
                if(check_unique_functions(function_node, child, true)) // recursive
                {
                    return true;
                }
            }
            break;

        case Node::NODE_FUNCTION:
            // TODO: stop recursion properly
            //
            // this condition is not enough to stop this
            // recursive process; but I think it is good
            // enough for most cases; the only problem is
            // anyway that we will eventually get the same
            // error multiple times...
            if(child == function_node)
            {
                return false;
            }

            if(function_node->get_string() == child->get_string())
            {
                if(compare_parameters(function_node, child))
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DUPLICATES, function_node->get_position());
                    msg << "you cannot define two functions with the same name (" << function_node->get_string()
                                << ") and prototype in the same scope, class or interface.";
                    return true;
                }
            }
            break;

        case Node::NODE_VAR:
        {
            size_t const cnt(child->get_children_size());
            for(size_t j(0); j < cnt; ++j)
            {
                Node::pointer_t variable_node(child->get_child(j));
                if(function_node->get_string() == variable_node->get_string())
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DUPLICATES, function_node->get_position());
                    msg << "you cannot define a function and a variable (found at line #"
                            << variable_node->get_position().get_line()
                            << ") with the same name ("
                            << function_node->get_string()
                            << ") in the same scope, class or interface.";
                    return true;
                }
            }
        }
            break;

        default:
            break;

        }
    }

    return false;
}



void Compiler::function(Node::pointer_t& function_node)
{
    int        idx, flags;

    if(get_attribute(function_node, Node::NODE_ATTR_UNUSED)
    || get_attribute(function_node, Node::NODE_ATTR_FALSE))
    {
        return;
    }

    unsigned long attrs = GetAttributes(function);
    Data& data = function.GetData();

    // Here we search for a parent for this function.
    // The parent can be a class, an interface or a package in which
    // case the function is viewed as a member. Otherwise it is
    // just a local or global definition. Different attributes are
    // only valid on members and some attributes have specific
    // effects which need to be tested here (i.e. a function marked
    // final in a class can't be overwritten)

    Node::pointer_t parent(function);
    Node::pointer_t list;
    bool more(true);
    bool member(false);
    bool package(false);
    do
    {
        parent = parent->get_parent();
        if(!parent)
        {
            // more = false; -- not required here
            break;
        }
        switch(parent->get_type())
        {
        case Node::NODE_CLASS:
        case Node::NODE_INTERFACE:
            more = false;
            member = true;
            break;

        case Node::NODE_PACKAGE:
            more = false;
            package = true;
            break;

        case Node::NODE_CATCH:
        case Node::NODE_DO:
        case Node::NODE_ELSE:
        case Node::NODE_FINALLY:
        case Node::NODE_FOR:
        case Node::NODE_FUNCTION:
        case Node::NODE_IF:
        case Node::NODE_PROGRAM:
        case Node::NODE_ROOT:
        case Node::NODE_SWITCH:
        case Node::NODE_TRY:
        case Node::NODE_WHILE:
        case Node::NODE_WITH:
            more = false;
            break;

        case Node::NODE_DIRECTIVE_LIST:
            if(!list)
            {
                list = parent;
            }
            break;

        default:
            break;

        }
    }
    while(more);

    // any one of the following flags implies that the function is
    // defined in a class; check to make sure!
    if(get_attribute(function, Node::NODE_ATTR_ABSTRACT)
    || get_attribute(function, Node::NODE_ATTR_STATIC)
    || get_attribute(function, Node::NODE_ATTR_PROTECTED)
    || get_attribute(function, Node::NODE_ATTR_VIRTUAL)
    || get_attribute(function, Node::NODE_ATTR_CONSTRUCTOR)
    || get_attribute(function, Node::NODE_ATTR_FINAL))
    {
        if(!member)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_ATTRIBUTES, f_lexer->get_input()->get_position());
            msg << "function \"" << function->get_string() << "\" was defined with an attribute which can only be used with a function member inside a class definition.";
        }
    }
    if(function->get_flag(Node::NODE_FUNCTION_FLAG_OPERATOR))
    {
        if(!member)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_OPERATOR, f_lexer->get_input()->get_position());
            msg << "operator \"" << function->get_string() << "\" can only be defined inside a class definition.";
        }
    }

    // any one of the following flags implies that the function is
    // defined in a class or a package; check to make sure!
    if(get_attribute(function, Node::NODE_ATTR_PRIVATE))
    {
        if(!package && !member)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_ATTRIBUTES, f_lexer->get_input()->get_position());
            msg << "function \"" << function->get_string() << "\" was defined with an attribute which can only be used inside a class or package definition.";
        }
    }

    // member functions need to not be defined in a super class
    // as final since that means you cannot overwrite these functions
    if(member)
    {
        if(check_final_functions(function, parent))
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_CANNOT_OVERLOAD, f_lexer->get_input()->get_position());
            msg << "function \"" << function->get_string() << "\" was marked as final in a super class and thus it cannot be defined in class \"" << parent->get_string() << "\".";
        }
        check_unique_functions(function, parent, true);
    }
    else
    {
        check_unique_functions(function, list, false);
    }

    // when the function calls itself (recursive) it would try to
    // add children when it is locked if we do not do this right here!
    if(!define_function_type(function))
    {
        return;
    }

    Node::pointer_t end_list, directive_list;
    NodeLock ln(function);
    size_t const max(function->get_child_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child = function->get_child(idx);
        switch(child->get_type())
        {
        case Node::NODE_PARAMETERS:
            // parse the parameters which have a default value
            parameters(child);
            break;

        case Node::NODE_DIRECTIVE_LIST:
            if(get_attribute(function, Node::NODE_ATTR_ABSTRACT))
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_IMPROPER_STATEMENT, f_lexer->get_input()->get_position());
                msg << "the function \"" << function->get_string() << "\" is marked abstract and cannot have a body.";
            }
            // find all the labels of this function
            find_labels(function, child);
            // parse the function body
            end_list = directive_list(child);
            directive_list = child;
            break;

        default:
            // the expression represents the function return type
            expression(child);
            // constructors only support Void (or should
            // it be the same name as the class?)
            if(is_constructor(function))
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_RETURN_TYPE, f_lexer->get_input()->get_position());
                msg << "a constructor must return \"void\" and nothing else, \"" << function->get_string() << "\" is invalid.";
            }
            break;

        }
    }

    flags = data.f_int.Get();
    if(function->get_flag(Node::NODE_FUNCTION_FLAG_NEVER) && is_constructor(function))
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_RETURN_TYPE, f_lexer->get_input()->get_position());
        msg << "a constructor must return (it cannot be marked Never).";
    }

    // test for a return whenever necessary
    if(!end_list
    && directive_list
    && (get_attribute(function, Node::NODE_ATTR_ABSTRACT) || get_attribute(function, Node::NODE_ATTR_INTRINSIC))
    && (function->get_flag(Node::NODE_FUNCTION_FLAG_VOID) || function->get_flag(Node::NODE_FUNCTION_FLAG_NEVER)))
    {
        f_optimizer->optimize(directive_list);
        find_labels(function, directive_list);
//fprintf(stderr, "ARGH! 2nd call...\n");
        end_list = DirectiveList(directive_list);
        if(!end_list.HasNode())
        {
            // TODO: we need a much better control flow to make
            // sure that this isn't a spurious error (i.e. you
            // don't need to have a return after a loop which
            // never exits)
// This could become annoying...
//fprintf(stderr, "WARNING: function not returning Void nor Never seems to terminate without a 'return' statement.\n");
            // It should be an error
            //f_error_stream->ErrMsg(, , );
        }
    }
}



void Compiler::Parameters(NodePtr& parameters)
{
    int        idx, max, j, jmax, k;
    int64_t        flags;

    flags = 0;
    NodeLock ln(parameters);
    max = parameters.GetChildCount();

    // clear the reference flags
    for(idx = 0; idx < max; ++idx) {
        NodePtr& param = parameters.GetChild(idx);
        Data& param_data = param.GetData();
        param_data.f_int.Set(param_data.f_int.Get() & ~(NODE_PARAMETERS_FLAG_REFERENCED | NODE_PARAMETERS_FLAG_PARAMREF));
    }

    // verify unicity and compute the NODE_SET and parameter type
    for(idx = 0; idx < max; ++idx) {
        NodePtr& param = parameters.GetChild(idx);
        Data& param_data = param.GetData();

        // verify whether it is defined twice or more
        k = idx;
        while(k > 0) {
            k--;
            NodePtr& prev = parameters.GetChild(k);
            Data& prev_data = prev.GetData();
            if(prev_data.f_str == param_data.f_str) {
                // TODO: note that these flags assume
                // that we never will have more than
                // 64 parameters or no double definitions
                if((flags & (1LL << k)) == 0) {
                    f_error_stream->ErrStrMsg(AS_ERR_DUPLICATES, prev, "the named parameter '%S' is defined two or more times in the same list of parameters.", &param_data.f_str);
                }
                flags |= 1LL << idx;
                break;
            }
        }

//param.Display(stderr);

        NodeLock ln(param);
        jmax = param.GetChildCount();
        for(j = 0; j < jmax; ++j) {
            NodePtr& child = param.GetChild(j);
            Data& data = child.GetData();
            if(data.f_type == NODE_SET)
            {
                expression(child.GetChild(0));
            }
            else
            {
                expression(child);
                NodePtr& type = child.GetLink(NodePtr::LINK_INSTANCE);
                if(type.HasNode())
                {
                    Node::pointer_t existing_type(param->get_link(Node::LINK_TYPE));
                    if(!existing_type)
                    {
                        param->set_link(Node::LINK_TYPE, type);
                    }
                    else if(existing_type != type)
                    {
                        Message msg(MESSAGE_LEVEL_FATAL, AS_ERR_INVALID_TYPE, param->get_position());
                        msg << "Existing type is:" << std::endl << existing_type << std::endl << "New type would be:" << std::endl << type;
                    }
                }
            }
        }
    }

    // if some parameter was referenced by another, mark it as such
    for(idx = 0; idx < max; ++idx) {
        NodePtr& param = parameters.GetChild(idx);
        Data& param_data = param.GetData();
        if((param_data.f_int.Get() & NODE_PARAMETERS_FLAG_REFERENCED) != 0) {
            param_data.f_int.Set(param_data.f_int.Get() | NODE_PARAMETERS_FLAG_PARAMREF);
        }
    }
}



// note that we search for labels in functions, programs, packages
// [and maybe someday classes, but for now classes can't have
// code and thus no labels]
void Compiler::FindLabels(NodePtr& function, NodePtr& node)
{
    // NOTE: function may also be a program or a package.
    Data& data = node.GetData();
    switch(data.f_type) {
    case NODE_LABEL:
    {
        NodePtr& label = function.FindLabel(data.f_str);
        if(label.HasNode()) {
            // TODO: test function type
            f_error_stream->ErrMsg(AS_ERR_DUPLICATES, function, "label '%S' defined twice in the same program, package or function.", &data.f_str);
        }
        else {
            function.AddLabel(node);
        }
    }
        return;

    // sub-declarations and expressions are just skipped
    // decls:
    case NODE_FUNCTION:
    case NODE_CLASS:
    case NODE_INTERFACE:
    case NODE_VAR:
    case NODE_PACKAGE:    // ?!
    case NODE_PROGRAM:    // ?!
    // expr:
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
    case NODE_MEMBER:
    case NODE_NEW:
    case NODE_POST_DECREMENT:
    case NODE_POST_INCREMENT:
        return;

    default:
        // other nodes may have children we want to check out
        break;

    }

    NodeLock ln(node);
    int max = node.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = node.GetChild(idx);
        FindLabels(function, child);
    }
}



NodePtr Compiler::Return(NodePtr& return_node)
{
    // 1. a return is only valid in a function (procedure)
    // 2. a return must return a value in a function
    // 3. a return can't return anything in a procedure
    // 4. you must assume that the function is returning
    //    Void when the function is a constructor and
    //    thus return can't have an expression in this case

    bool more;
    bool bad = false;
    int64_t flags = 0;
    NodePtr parent = return_node;
    Data *data = 0;
    do {
        more = false;
        parent = parent.GetParent();
        if(!parent.HasNode()) {
            bad = true;
            break;
        }
        data = &parent.GetData();
        switch(data->f_type) {
        case NODE_FUNCTION:
            flags = data->f_int.Get();
            break;

        case NODE_CLASS:
        case NODE_INTERFACE:
        case NODE_PACKAGE:
        case NODE_PROGRAM:
        case NODE_ROOT:
            bad = true;
            break;

        default:
            more = true;
            break;

        }
    } while(more);
    if(bad) {
        f_error_stream->ErrMsg(AS_ERR_IMPROPER_STATEMENT, return_node, "'return' can only be used inside a function.");
    }
    else {
        if((flags & NODE_FUNCTION_FLAG_NEVER) != 0) {
            f_error_stream->ErrStrMsg(AS_ERR_IMPROPER_STATEMENT, return_node, "'return' was used inside '%S', a function Never returning.", &data->f_str);
        }

        int max = return_node.GetChildCount();
        if(max == 1) {
            if((flags & NODE_FUNCTION_FLAG_VOID) != 0
            || IsConstructor(parent)) {
                f_error_stream->ErrStrMsg(AS_ERR_IMPROPER_STATEMENT, return_node, "'return' was used with an expression inside '%S', a function returning Void.", &data->f_str);
            }
            Expression(return_node.GetChild(0));
        }
        else {
            // NOTE:
            // This actually needs to be transformed to
            // returning 'undefined' in the execution
            // environment... maybe we will add this
            // here at some point.
            if((flags & NODE_FUNCTION_FLAG_VOID) == 0
            && !IsConstructor(parent)) {
                f_error_stream->ErrStrMsg(AS_ERR_IMPROPER_STATEMENT, return_node, "'return' was used without an expression inside '%S', a function which expected a value to be returned.", &data->f_str);
            }
        }
    }

    return parent;
}







void Compiler::import(Node::pointer_t& import_node)
{
    // If we have the IMPLEMENTS flag set, then we must make sure
    // that the corresponding package is compiled.
    if(!import_node->get_flag(Node::NODE_IMPORT_FLAG_IMPLEMENTS))
    {
        return;
    }

    // find the package
    Node::pointer_t package;

    // search in this program
    package = find_package(f_program, import_node->get_string());
    if(!package)
    {
        // not in this program, search externals
        Node::pointer_t program;
        String any_name("*");
        if(find_external_package(import_node, any_name, program))
        {
            // got externals, search those now
            package = find_package(program, import_node->get_string());
        }
        if(!package)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_NOT_FOUND, f_lexer->get_input()->get_position());
            msg << "cannot find package '" << import_node->get_string() << "'.";
            return;
        }
    }

    Data& package_data = package.GetData();

    // make sure it is compiled (once)
    bool const was_referenced(package->get_flag(Node::NODE_PACKAGE_FLAG_REFERENCED));
    package->set_flag(Node::NODE_PACKAGE_FLAG_REFERENCED, true);
    if(was_referenced)
    {
        directive_list(package);
    }
}



void Compiler::use_namespace(Node::pointer_t& use_namespace_node)
{
    if(use_namespace_node->get_children_count() != 1)
    {
        return;
    }
    NodeLock ln(use_namespace_node);

    // type/scope name defined in an expression
    // (needs to be resolved in an identifiers, members composed of
    // identifiers or a string representing a valid type name)
    Node::pointer_t qualifier(use_namespace_node->get_child(0));
    expression(qualifier);

    // TODO: I'm not too sure what the qualifier can end up being at this
    //       point, but if it is a whole tree of node, we do not know
    //       how to copy it... (because using qualifier directly instead
    //       of using q as defined below would completely break the
    //       existing namespace...)
    if(qualifier->get_type() != Node::NODE_STRING)
    {
        throw exception_internal_error("type qualifier is not just a string, we cannot duplicate it at this point");
    }

    // we create two nodes; one so we know we have a NAMESPACE instruction
    // and a child of that node which is the type itself; these are
    // deleted once we return from the directive_list() function and not
    // this function
    Node::pointer_t q(qualifier->create_replacement(qualifier->get_type()));
    q->set_string(qualifier->get_string());
    Node::pointer_t n(qualifier->create_replacement(Node::NODE_NAMESPACE));
    n->append_child(q);
    f_scope->append_child(n);
}



void Compiler::LinkType(NodePtr& type)
{
    // already linked?
    NodePtr& link = type.GetLink(NodePtr::LINK_INSTANCE);
    if(link.HasNode()) {
        return;
    }

    Data& data = type.GetData();
    if(data.f_type != NODE_IDENTIFIER && data.f_type != NODE_STRING) {
        // we can't link (determine) the type at compile time
        // if we have a type expression
//fprintf(stderr, "WARNING: dynamic type?!\n");
        return;
    }

    int flags = data.f_int.Get();
    if((flags & NODE_IDENTIFIER_FLAG_TYPED) != 0) {
        // if it fails, we fail only once...
        return;
    }
    data.f_int.Set(flags | NODE_IDENTIFIER_FLAG_TYPED);

    Node::pointer_t object;
    if(!resolve_name(type, type, object, Node::pointer_t(), 0)) {
        // unknown type?! -- should we return a link to Object?
        f_error_stream->ErrStrMsg(AS_ERR_INVALID_EXPRESSION, type, "cannot find a class definition for type '%S'.", &data.f_str);
        return;
    }

    Data& object_data = object.GetData();
    if(object_data.f_type != NODE_CLASS
    && object_data.f_type != NODE_INTERFACE) {
        f_error_stream->ErrStrMsg(AS_ERR_INVALID_EXPRESSION, type, "the name '%S' is not referencing a class nor an interface.", &data.f_str);
        return;
    }

    // it worked.
    type.SetLink(NodePtr::LINK_INSTANCE, object);
}


bool Compiler::CheckField(NodePtr& link, NodePtr& field, int& funcs, NodePtr& resolution, NodePtr *params, int search_flags)
{
    int        idx, max, j, m;

    NodeLock ln(link);
    max = link.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr& list = link.GetChild(idx);
        Data& list_data = list.GetData();
        if(list_data.f_type == NODE_DIRECTIVE_LIST) {
            // search in this list!
            NodeLock ln(list);
            m = list.GetChildCount();
            for(j = 0; j < m; ++j) {
                // if we have a sub-list, generate a recursive call
                NodePtr& child = list.GetChild(j);
                Data& child_data = child.GetData();
                if(child_data.f_type == NODE_DIRECTIVE_LIST) {
                    if(CheckField(list, field, funcs, resolution, params, search_flags)) {
                        if(FuncsName(funcs, resolution, false)) {
                            return true;
                        }
                    }
                }
                else {
                    if(CheckName(list, j, resolution, field, params, search_flags)) {
                        if(FuncsName(funcs, resolution)) {
                            NodePtr inst = field.GetLink(NodePtr::LINK_INSTANCE);
                            if(!inst.HasNode()) {
                                field.SetLink(NodePtr::LINK_INSTANCE, resolution);
                            }
                            else {
                                AS_ASSERT(inst.SameAs(resolution));
                            }
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}


bool Compiler::FindField(NodePtr& link, NodePtr& field, int& funcs, NodePtr& resolution, NodePtr *params, int search_flags)
{
    RestoreFlags restore_flags(this);

    bool r = FindAnyField(link, field, funcs, resolution, params, search_flags);
    if(!r) {
        PrintSearchErrors(field);
    }

    return r;
}


bool Compiler::FindAnyField(NodePtr& link, NodePtr& field, int& funcs, NodePtr& resolution, NodePtr *params, int search_flags)
{
//fprintf(stderr, "Find Any Field...\n");

    if(CheckField(link, field, funcs, resolution, params, search_flags)) {
//fprintf(stderr, "Check Field true...\n");
        return true;
    }
    if(funcs != 0) {
        // TODO: stronger validation of functions
        // this is wrong, we need a depth test on the best
        // functions but we need to test all the functions
        // of inherited fields too
//fprintf(stderr, "funcs != 0 true...\n");
        return true;
    }

//fprintf(stderr, "FindInExtends?!...\n");
    return FindInExtends(link, field, funcs, resolution, params, search_flags);
}



bool Compiler::FindInExtends(NodePtr& link, NodePtr& field, int& funcs, NodePtr& resolution, NodePtr *params, int search_flags)
{
    int        idx, max, count;

    // try to see if we are inheriting that field...
    NodeLock ln(link);
    max = link.GetChildCount();
    count = 0;
    for(idx = 0; idx < max; ++idx) {
        NodePtr& extends = link.GetChild(idx);
        Data& extends_data = extends.GetData();
        if(extends_data.f_type == NODE_EXTENDS) {
            if(extends.GetChildCount() == 1) {
                NodePtr& type = extends.GetChild(0);
                LinkType(type);
                NodePtr& sub_link = type.GetLink(NodePtr::LINK_INSTANCE);
                if(!sub_link.HasNode()) {
                    // we can't search a field in nothing...
fprintf(stderr, "WARNING: type not linked, cannot lookup member.\n");
                }
                else if(FindAnyField(sub_link, field, funcs, resolution, params, search_flags)) {
                    count++;
                }
            }
//fprintf(stderr, "Extends existing! (%d)\n", extends.GetChildCount());
        }
        else if(extends_data.f_type == NODE_IMPLEMENTS) {
            if(extends.GetChildCount() == 1) {
                NodePtr& type = extends.GetChild(0);
                Data& data_type = type.GetData();
                if(data_type.f_type == NODE_LIST) {
                    int cnt = type.GetChildCount();
                    for(int j = 0; j < cnt; ++j) {
                        NodePtr& child = type.GetChild(j);
                        LinkType(child);
                        NodePtr& sub_link = child.GetLink(NodePtr::LINK_INSTANCE);
                        if(!sub_link.HasNode()) {
                            // we can't search a field in nothing...
fprintf(stderr, "WARNING: type not linked, cannot lookup member.\n");
                        }
                        else if(FindAnyField(sub_link, field, funcs, resolution, params, search_flags)) {
                            count++;
                        }
                    }
                }
                else {
                    LinkType(type);
                    NodePtr& sub_link = type.GetLink(NodePtr::LINK_INSTANCE);
                    if(!sub_link.HasNode()) {
                        // we can't search a field in nothing...
fprintf(stderr, "WARNING: type not linked, cannot lookup member.\n");
                    }
                    else if(FindAnyField(sub_link, field, funcs, resolution, params, search_flags)) {
                        count++;
                    }
                }
            }
        }
    }

    if(count == 1 || funcs != 0) {
        return true;
    }

    if(count == 0) {
        // NOTE: warning? error? This actually would just turn
        //     on a flag.
        //     As far as I know I now have an error in case
        //     the left hand side expression is a static
        //     class (opposed to a dynamic class which can
        //     have members added at runtime)
//fprintf(stderr, "     field not found...\n");
    }
    else {
        Data& data = field.GetData();
        f_error_stream->ErrStrMsg(AS_ERR_DUPLICATES, field, "found more than one match for '%S'.", &data.f_str);
    }

    return false;
}


bool Compiler::ResolveField(NodePtr& object, NodePtr& field, NodePtr& resolution, NodePtr *params, int search_flags)
{
    unsigned long    idx, max;
    NodePtr        type;

    // this is to make sure it is optimized, etc.
    //Expression(field); -- we can't have this here or it generates loops

    // just in case the caller is re-using the same node
    resolution.ClearNode();

    NodePtr link;

    // check that the object is indeed an object (i.e. a variable
    // which references a class)
    Data& obj_data = object.GetData();
    switch(obj_data.f_type) {
    case NODE_VARIABLE:
    case NODE_PARAM:
        // it's a variable or a parameter, check for the type
        //NodeLock ln(object);
        max = object.GetChildCount();
        for(idx = 0; idx < max; ++idx) {
            type = object.GetChild(idx);
            Data& data = type.GetData();
            if(data.f_type != NODE_SET
            && data.f_type != NODE_VAR_ATTRIBUTES) {
                // we found the type
                break;
            }
        }
        if(idx >= max || !type.HasNode()) {
            // TODO: should this be an error instead?
            fprintf(stderr, "WARNING: variables and parameters without a type should not be used with members.\n");
            return false;
        }

        // we need to have a link to the class
        LinkType(type);
        link = type.GetLink(NodePtr::LINK_INSTANCE);
        if(!link.HasNode()) {
            // NOTE: we can't search a field in nothing...
            //     if I'm correct, it will later bite the
            //     user if the class isn't dynamic
//fprintf(stderr, "WARNING: type not linked, cannot lookup member.\n");
            return false;
        }
        break;

    case NODE_CLASS:
    case NODE_INTERFACE:
        link = object;
        break;

    default:
        f_error_stream->ErrMsg(AS_ERR_INVALID_TYPE, field, "object of type '%s' is not known to have members.", obj_data.GetTypeName());
        return false;

    }

    Data& data = field.GetData();
    if(data.f_type != NODE_IDENTIFIER
    && data.f_type != NODE_VIDENTIFIER
    && data.f_type != NODE_STRING) {
        // we can't determine at compile time whether a
        // dynamic field is valid...
//fprintf(stderr, "WARNING: cannot check a dynamic field.\n");
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

    int funcs = 0;
    bool r = FindField(link, field, funcs, resolution, params, search_flags);
    if(!r) {
        return false;
    }

    if(funcs != 0) {
#if 0
fprintf(stderr, "DEBUG: field ");
field.DisplayPtr(stderr);
fprintf(stderr, " is a function.\n");
#endif
        resolution.ClearNode();
        return SelectBestFunc(params, resolution);
    }

    return true;
}



bool Compiler::IsDynamicClass(NodePtr& class_node)
{
    if(!class_node.HasNode()) {
        // we cannot know, return that it is...
        return true;
    }

    unsigned long attrs = GetAttributes(class_node);
    if((attrs & NODE_ATTR_DYNAMIC) != 0) {
        return true;
    }

    int max = class_node.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = class_node.GetChild(idx);
        Data& data = child.GetData();
        if(data.f_type == NODE_EXTENDS) {
            NodePtr& name = child.GetChild(0);
            NodePtr& extends = name.GetLink(NodePtr::LINK_INSTANCE);
            if(extends.HasNode()) {
                Data& data = extends.GetData();
                if(data.f_str == "Object") {
                    // we ignore the dynamic flag of
                    // Object (that's a hack in the
                    // language reference!)
                    return false;
                }
                return IsDynamicClass(extends);
            }
            break;
        }
    }

    return false;
}



void Compiler::CheckMember(NodePtr& ref, NodePtr& field, NodePtr& field_name)
{
    unsigned long    attrs;
    bool        err;

    if(!field.HasNode()) {
        NodePtr& type = ref.GetLink(NodePtr::LINK_TYPE);
        if(!IsDynamicClass(type)) {
            Data& data = type.GetData();
            Data& name = ref.GetData();
            Data& field = field_name.GetData();
            f_error_stream->ErrStrMsg(AS_ERR_STATIC, ref, "'%S: %S' is not dynamic and thus it cannot be used with unknown member '%S'.", &name.f_str, &data.f_str, &field.f_str);
        }
        return;
    }

    NodePtr& obj = ref.GetLink(NodePtr::LINK_INSTANCE);
    if(!obj.HasNode()) {
        return;
    }

    // If the link is directly a class or an interface
    // then the field needs to be a sub-class, sub-interface,
    // static function, static variable or constant variable.
    Data& obj_data = obj.GetData();
    if(obj_data.f_type != NODE_CLASS
    && obj_data.f_type != NODE_INTERFACE) {
//fprintf(stderr, ":: Checking member Skipped :: %s\n", obj_data.GetTypeName());
        return;
    }

//fprintf(stderr, ":: Checking member :: %s -- %.*S\n", obj_data.GetTypeName(), obj_data.f_str.GetLength(), obj_data.f_str.Get());

    Data& field_data = field.GetData();

//fprintf(stderr, ":: Against :: %s -- %.*S\n", field_data.GetTypeName(), field_data.f_str.GetLength(), field_data.f_str.Get());

    switch(field_data.f_type) {
    case NODE_CLASS:
    case NODE_INTERFACE:
        err = false;
        break;

    case NODE_FUNCTION:
        // note that constructors are considered static, but
        // you can't just call a constructor...
        //
        // operators are static and thus we'll be fine with
        // operators (since you need to call operators with
        // all the required inputs)
        //
        attrs = GetAttributes(field);
        err = (attrs & NODE_ATTR_STATIC) == 0
            && (field_data.f_int.Get() & NODE_FUNCTION_FLAG_OPERATOR) == 0;
        break;

    case NODE_VARIABLE:
        attrs = GetAttributes(field);
        err = (attrs & NODE_ATTR_STATIC) == 0
            && (field_data.f_int.Get() & NODE_VAR_FLAG_CONST) == 0;
        break;

    default:
        err = true;
        break;

    }

    if(err) {
        f_error_stream->ErrStrMsg(AS_ERR_INSTANCE_EXPECTED, ref, "you cannot directly access non-static functions and non-static/constant variables in a class ('%S' here); you need to use an instance instead.", &field_data.f_str);
    }
}



bool Compiler::FindMember(NodePtr& member, NodePtr& resolution, NodePtr *params, int search_flags)
{
    // Just in case the caller is re-using the same node
    resolution.ClearNode();

    // Invalid member node? If so don't generate an error because
    // we most certainly already mentioned that to the user
    // (and if not that's a bug earlier than here).
    if(member.GetChildCount() != 2) {
        return false;
    }
    NodeLock ln(member);

//fprintf(stderr, "Searching for Member...\n");

    bool must_find = false;
    NodePtr object;

    NodePtr& name = member.GetChild(0);
    Data *data = &name.GetData();
    switch(data->f_type) {
    case NODE_MEMBER:
        // This happens when you have an expression such as:
        //        a.b.c
        // Then the child most MEMBER will be the identifier 'a'
        if(!FindMember(name, object, params, search_flags)) {
            return false;
        }
        // If we reach here, the resolution is the object we want
        // to use next to resolve the field(s)
        data = 0;
        break;

    case NODE_SUPER:
    {
        // super should only be used in classes, but we can
        // find standalone functions using that keyword too...
        // here we search for the class and if we find it then
        // we try to get access to the extends. If the object
        // is Object, then we generate an error (i.e. there is
        // no super of Object).
//fprintf(stderr, "Handling super member\n");
        CheckSuperValidity(name);
        NodePtr parent = member;
        Data *parent_data = 0;
        do {
            parent = parent.GetParent();
            if(!parent.HasNode()) {
                break;
            }
            parent_data = &parent.GetData();
        } while(parent_data->f_type != NODE_CLASS
             && parent_data->f_type != NODE_INTERFACE
             && parent_data->f_type != NODE_PACKAGE
             && parent_data->f_type != NODE_PROGRAM
             && parent_data->f_type != NODE_ROOT);
        // NOTE: Interfaces can use super but we can't
        //     know what it is at compile time.
//fprintf(stderr, "Parent is %s\n", parent_data->GetTypeName());
        if(parent_data != 0
        && parent_data->f_type == NODE_CLASS) {
            if(parent_data->f_str == "Object") {
                // this should never happen!
                f_error_stream->ErrMsg(AS_ERR_INVALID_EXPRESSION, name, "you cannot use 'super' within the 'Object' class.");
            }
            else {
                int max = parent.GetChildCount();
//fprintf(stderr, "Search corresponding class (%d members)\n", max);
                for(int idx = 0; idx < max; ++idx) {
                    NodePtr& child = parent.GetChild(idx);
                    Data& data = child.GetData();
                    if(data.f_type == NODE_EXTENDS) {
                        if(child.GetChildCount() == 1) {
                            NodePtr& name = child.GetChild(0);
                            object = name.GetLink(NodePtr::LINK_INSTANCE);
//fprintf(stderr, "Got the object! (%d)\n", idx);
                        }
                        if(!object.HasNode()) {
                            // there is another
                            // error...
                            return false;
                        }
                        break;
                    }
                }
                if(!object.HasNode()) {
                    // default to Object if no extends
                    ResolveInternalType(parent, "Object", object);
                }
                must_find = true;
            }
        }
        data = 0;
    }
        break;

    default:
        Expression(name);
        data = &name.GetData();
        break;

    }

    // do the field expression so we possibly detect more errors
    // in the field now instead of the next compile
    NodePtr& field = member.GetChild(1);
    Data& fdata = field.GetData();
    if(fdata.f_type != NODE_IDENTIFIER) {
        Expression(field);
    }

    if(data != 0) {
        // TODO: this is totally wrong, what we need is the type, not
        //     just the name; this if we have a string, the type is
        //     the String class.
        if(data->f_type != NODE_IDENTIFIER
        && data->f_type != NODE_STRING) {
            // A dynamic name can't be resolved now; we can only
            // hope that it will be a valid name at run time.
            // However, we still want to resolve everything we
            // can in the list of field names.
            // FYI, this happens in this case:
            //    ("test_" + var).hello
            return true;
        }

        if(!resolve_name(name, name, object, params, search_flags)) {
            // we can't even find the first name!
            // we won't search for fields since we need to have
            // an object for that purpose!
            return false;
        }
    }

    // we avoid errors by returning no resolution but 'success'
    if(object.HasNode()) {
        bool result = ResolveField(object, field, resolution, params, search_flags);

//fprintf(stderr, "ResolveField() returned %d\n", result);

        if(!result && must_find) {
            f_error_stream->ErrMsg(AS_ERR_INVALID_EXPRESSION, name, "'super' must name a valid field of the super class.");
        }
        else {
            CheckMember(name, resolution, field);
        }
        return result;
    }

    return true;
}

void Compiler::ResolveMember(NodePtr& expr, NodePtr *params, int search_flags)
{
    NodePtr resolution;

    if(!FindMember(expr, resolution, params, search_flags)) {
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
    // can't be fully resolved at compile time
    if(!resolution.HasNode()) {
        return;
    }

    // the name was fully resolved, check it out

//Data& d = resolution.GetData();
//fprintf(stderr, "Member resolution is %d\n", d.f_type);

    if(ReplaceConstantVariable(expr, resolution)) {
        // just a constant, we're done
        return;
    }

    // copy the type whenever available
    expr.SetLink(NodePtr::LINK_INSTANCE, resolution);
    NodePtr& type = resolution.GetLink(NodePtr::LINK_TYPE);
    if(type.HasNode()) {
        expr.SetLink(NodePtr::LINK_TYPE, type);
    }

    // if we have a Getter, transform the MEMBER into a CALL
    // to a MEMBER
    Data& data = resolution.GetData();
    if(data.f_type == NODE_FUNCTION && (data.f_int.Get() & NODE_FUNCTION_FLAG_GETTER) != 0) {
fprintf(stderr, "CAUGHT! getter...\n");
        // so expr is a MEMBER at this time
        // it has two children
        NodePtr left = expr.GetChild(0);
        NodePtr right = expr.GetChild(1);
        expr.DeleteChild(0);
        expr.DeleteChild(0);    // 1 is now 0

        // create a new node since we don't want to move the
        // call (expr) node from its parent.
        NodePtr member;
        member.CreateNode(NODE_MEMBER);
        member.SetLink(NodePtr::LINK_INSTANCE, resolution);
        member.AddChild(left);
        member.AddChild(right);
        member.SetLink(NodePtr::LINK_TYPE, type);

        expr.AddChild(member);

        // we need to change the name to match the getter
        // NOTE: we know that the right data is an identifier
        //     a v-identifier or a string so the following
        //     will always work
        Data& right_data = right.GetData();
        String getter_name("->");
        getter_name += right_data.f_str;
        right_data.f_str = getter_name;

        // the call needs a list of parameters (empty)
        NodePtr params;
        params.CreateNode(NODE_LIST);

        expr.AddChild(params);

        // and finally, we transform the member in a call!
        Data& expr_data = expr.GetData();
        expr_data.f_type = NODE_CALL;
    }
}



// Check whether t1 matches t2
// When match flag MATCH_ANY_ANCESTOR is set, it will also check
// all the ancestors of t1 to see if any one matches t2
// It is expected that t2 will be a NODE_PARAM in which case
// we accept an empty node or a node without a type definition
// as a 'match any' special type.
// Otherwise we make sure we transform the type expression in
// a usable type and compare it with t1 and its ancestors.
int Compiler::MatchType(NodePtr& t1, NodePtr t2, int match)
{
// Some invalid input?
    if(!t1.HasNode() || !t2.HasNode()) {
        return 0;
    }

    Data& dt2 = t2.GetData();
    if(dt2.f_type == NODE_PARAM) {
        if((dt2.f_int.Get() & NODE_PARAMETERS_FLAG_OUT) != 0) {
            // t1 MUST be an identifier which references
            // a variable which we can set on exit
            Data& dt1 = t1.GetData();
            if(dt1.f_type != NODE_IDENTIFIER) {
                // NOTE: we can't generate an error here
                //     because there could be another
                //     valid function somewhere else...
fprintf(stderr, "WARNING: a variable name is expected for a function parameter flagged as an OUT parameter.\n");
                return 0;
            }
        }
        if(t2.GetChildCount() <= 0) {
            return INT_MAX / 2;
        }
        NodePtr& id = t2.GetChild(0);
        // make sure we have a type definition, if it is
        // only a default set, then it is equal anyway
        Data& data = id.GetData();
        if(data.f_type == NODE_SET) {
            return INT_MAX / 2;
        }
        NodePtr resolution;
        resolution = id.GetLink(NodePtr::LINK_TYPE);
        if(!resolution.HasNode()) {
//fprintf(stderr, "Resolving type for a NODE_PARAM (%.*S)\n", data.f_str.GetLength(), data.f_str.Get());
            if(!resolve_name(t2, id, resolution, Node::pointer_t(), 0)) {
                return 0;
            }
//fprintf(stderr, "   ---> ");
//resolution.DisplayPtr(stderr);
//fprintf(stderr, "\n");
            id.SetLink(NodePtr::LINK_TYPE, resolution);
        }
        t2 = id;
    }

    NodePtr& tp1 = t1.GetLink(NodePtr::LINK_TYPE);
    NodePtr& tp2 = t2.GetLink(NodePtr::LINK_TYPE);

    if(!tp1.HasNode())
    {
        TypeExpr(t1);
        tp1 = t1.GetLink(NodePtr::LINK_TYPE);
        if(!tp1.HasNode()) {
//fprintf(stderr, "WARNING: cannot determine the type of the input parameter.\n");
            return 1;
        }
    }

// The exact same type?
    if(tp1.SameAs(tp2)) {
        return 1;
    }
    // TODO: if we keep the class <id>; definition, then we need
    //     to also check for a full definition

// if one of the types is Object, then that's a match
    NodePtr object;
    ResolveInternalType(t1, "Object", object);
    if(tp1.SameAs(object)) {
        // whatever tp2, we match (bad user practice of
        // untyped variables...)
        return 1;
    }
    if(tp2.SameAs(object)) {
        // this is a "bad" match -- anything else will be better
        return INT_MAX / 2;
    }
    // TODO: if we find a [class Object;] definition
    //     instead of a complete definition

// Okay, still not equal, check ancestors of tp1 if
// permitted (and if tp1 is a class).
    if((match & MATCH_ANY_ANCESTOR) == 0) {
        return 0;
    }
    Data& d1 = tp1.GetData();
#if 0
{
char buf[256];
size_t sz = sizeof(buf);
d1.f_str.ToUTF8(buf, sz);
fprintf(stderr, "Name of tp1 [%s] (type = %d)\n", buf, d1.f_type);
#if 1
Data& d2 = tp2.GetData();
sz = sizeof(buf);
d2.f_str.ToUTF8(buf, sz);
fprintf(stderr, "Name of tp2 [%s] (type = %d)\n", buf, d2.f_type);
#endif
}
#endif
    if(d1.f_type != NODE_CLASS) {
        return 0;
    }

    return FindClass(tp1, tp2, 2);
}



int Compiler::FindClass(NodePtr& class_type, NodePtr& type, int depth)
{
    int    idx, max, r, result;

    NodeLock ln(class_type);
    max = class_type.GetChildCount();

    for(idx = 0; idx < max; ++idx) {
        NodePtr& child = class_type.GetChild(idx);
        Data& data = child.GetData();
        if(data.f_type == NODE_IMPLEMENTS
        || data.f_type == NODE_EXTENDS) {
            if(child.GetChildCount() == 0) {
                // should never happen
                continue;
            }
            NodeLock ln(child);
            NodePtr& super_name = child.GetChild(0);
            NodePtr& super = super_name.GetLink(NodePtr::LINK_INSTANCE);
            if(!super.HasNode()) {
                Expression(super_name);
                super = super_name.GetLink(NodePtr::LINK_INSTANCE);
            }
            if(!super.HasNode()) {
                f_error_stream->ErrMsg(AS_ERR_INVALID_EXPRESSION, class_type, "cannot find the type named in an 'extends' or 'implements' list.");
                continue;
            }
            if(super.SameAs(type)) {
                return depth;
            }
        }
    }

    depth += 1;
    result = 0;
    for(idx = 0; idx < max; ++idx) {
        NodePtr& child = class_type.GetChild(idx);
        Data& data = child.GetData();
        if(data.f_type == NODE_IMPLEMENTS
        || data.f_type == NODE_EXTENDS) {
            if(child.GetChildCount() == 0) {
                // should never happen
                continue;
            }
            NodeLock ln(child);
            NodePtr& super_name = child.GetChild(0);
            NodePtr& super = super_name.GetLink(NodePtr::LINK_INSTANCE);
            if(!super.HasNode()) {
                continue;
            }
            r = FindClass(super, type, depth);
            if(r > result) {
                result = r;
            }
        }
    }

    return result;
}


bool Compiler::DefineFunctionType(NodePtr& func)
{
    int        idx, max;

    // define the type of the function when not available yet
    if(func.GetLink(NodePtr::LINK_TYPE).HasNode()) {
        return true;
    }

    max = func.GetChildCount();
    if(max < 1) {
        Data& data = func.GetData();
        return (data.f_int.Get() & NODE_FUNCTION_FLAG_VOID) != 0;
    }

    {
        NodeLock ln(func);

        for(idx = 0; idx < max; ++idx) {
            NodePtr& type = func.GetChild(idx);
            Data& data = type.GetData();
            if(data.f_type != NODE_PARAMETERS
            && data.f_type != NODE_DIRECTIVE_LIST) {
                // then this is the type definition
                expression(type);
                Node::pointer_t resolution;
                if(resolve_name(type, type, resolution, Node::pointer_t(), 0)) {
#if 0
                    // we may want to have that in
                    // different places for when we
                    // specifically look for a type
                    Data& func_data = resolution.GetData();
                    if(func_data.f_type == NODE_FUNCTION) {
                        NodePtr parent = resolution;
                        for(;;) {
                            parent = parent.GetParent();
                            if(!parent.HasNode()) {
                                break;
                            }
                            Data& parent_data = parent.GetData();
                            if(parent_data.f_type == NODE_CLASS) {
                                if(parent_data.f_str == func_data.f_str) {
                                    // ha! we hit the constructor of a class, use the class instead!
                                    resolution = parent;
                                }
                                break;
                            }
                            if(parent_data.f_type == NODE_INTERFACE
                            || parent_data.f_type == NODE_PACKAGE
                            || parent_data.f_type == NODE_ROOT) {
                                break;
                            }
                        }
                    }
#endif

//fprintf(stderr, "  final function type is:\n");
//resolution.Display(stderr);

                    func.SetLink(NodePtr::LINK_TYPE, resolution);
                }
                break;
            }
        }
    }

    if(idx == max) {
        // if no type defined, put a default of Object
        NodePtr object;
        ResolveInternalType(func, "Object", object);
        func.SetLink(NodePtr::LINK_TYPE, object);
    }

    return true;
}



// check whether the list of input parameters matches the function
// prototype; note that if the function is marked as "no prototype"
// then it matches automatically, but it gets a really low score.
int Compiler::CheckFunctionWithParams(NodePtr& func, NodePtr *params)
{
    int    idx, j, rest;

    // At this time, I'm not too sure what I can do if params is
    // null. Maybe that's when you try to do var a = <funcname>;?
    if(params == 0)
    {
        return 0;
    }

    NodePtr match;
    match.CreateNode(NODE_PARAM_MATCH);
    match.SetLink(NodePtr::LINK_INSTANCE, func);
    Data& matching = match.GetData();

    // define the type of the function when not available yet
    if(!DefineFunctionType(func))
    {
        // error: this function definition is no good
        // (don't report that, we should have had an error in
        // the parser already)
        return -1;
    }

    int count = params->GetChildCount();
    int max = func.GetChildCount();
//fprintf(stderr, "Check %d params versus %d inputs.\n", count, max);
//params->Display(stderr);
    Data& data = func.GetData();
    if(max == 0)
    {
        // no parameters; check whether the user specifically
        // used void or Void as the list of parameters
        if((data.f_int.Get() & NODE_FUNCTION_FLAG_NOPARAMS) == 0)
        {
unprototyped:
            // TODO:
            // this function accepts whatever
            // however, the function wasn't marked as such and
            // therefore we could warn about this...
            matching.f_int.Set(matching.f_int.Get() | NODE_PARAM_MATCH_FLAG_UNPROTOTYPED);
            params->AddChild(match);
            return 0;
        }
        if(count == 0)
        {
            params->AddChild(match);
            return 0;
        }
        // caller has one or more parameters, but function
        // only accepts 0 (i.e. Void)
        return 0;
    }

    NodeLock ln(func);
    NodePtr& parameters = func.GetChild(0);
    Data& param_data = parameters.GetData();
    if(param_data.f_type != NODE_PARAMETERS)
    {
        goto unprototyped;
    }

    // params doesn't get locked, we expect to add to that list
    NodeLock ln2(parameters);
    max = parameters.GetChildCount();
    if(max == 0)
    {
        // this function accepts 0 parameters
        if(count > 0)
        {
            // error: can't accept any parameter
            return -1;
        }
        params->AddChild(match);
        return 0;
    }

    // check whether the user marked the function as unprototyped;
    // if so, then we're done
    NodePtr& unproto = parameters.GetChild(0);
    Data& up_data = unproto.GetData();
    if((up_data.f_int.Get() & NODE_PARAMETERS_FLAG_UNPROTOTYPED) != 0)
    {
        // this function is marked to accept whatever
        matching.f_int.Set(matching.f_int.Get() | NODE_PARAM_MATCH_FLAG_UNPROTOTYPED);
        params->AddChild(match);
        return 0;
    }

    // we can't choose which list to use because the user
    // parameters can be named and thus we want to search
    // the caller parameters in the function parameter list
    // and not the opposite

#if 0
fprintf(stderr, "NOTE: looking for ~%d params in %d inputs in function proto (Node:", count, max);
func.DisplayPtr(stderr);
fprintf(stderr, ")\n");
#endif

    int size = max > count ? max : count;
    matching.f_user_data.New(size * 2);
    int *m = matching.f_user_data.Buffer();
    int min = 0;
    rest = max;
    for(idx = 0; idx < count; ++idx)
    {
        Data param_name;
        NodePtr& p = params->GetChild(idx);
        Data& data = p.GetData();
        if(data.f_type == NODE_PARAM_MATCH)
        {
//fprintf(stderr, "Skipping entry %d since it is a PARAM MATCH\n", idx);
            continue;
        }
        //NodeLock ln(p);
        int cm = p.GetChildCount();
        for(int c = 0; c < cm; ++c) {
            NodePtr& child = p.GetChild(c);
            Data& data = child.GetData();
            if(data.f_type == NODE_NAME) {
                // the parameter name is specified
                if(child.GetChildCount() != 1) {
                    // an error in the parser?
                    f_error_stream->ErrMsg(AS_ERR_INTERNAL_ERROR, func, "the NODE_NAME has no children.");
                    return -1;
                }
                //NodeLock ln(child);
                NodePtr& name = child.GetChild(0);
                param_name = name.GetData();
                if(param_name.f_type != NODE_IDENTIFIER) {
                    f_error_stream->ErrMsg(AS_ERR_INTERNAL_ERROR, func, "the name of a parameter needs to be an identifier.");
                    return -1;
                }
                break;
            }
        }
        // search for the parameter (fp == found parameter)
        // NOTE: because the children aren't deleted, keep a
        //     bare pointer is fine here.
        NodePtr *fp = 0;
        if(param_name.f_type == NODE_IDENTIFIER) {
            // search for a parameter with that name
            for(j = 0; j < max; ++j) {
                NodePtr& p = parameters.GetChild(j);
                Data& data = p.GetData();
                if(data.f_str == param_name.f_str) {
                    fp = &p;
                    break;
                }
            }
            if(j == max) {
                // can't find a parameter with that name...
                f_error_stream->ErrStrMsg(AS_ERR_INVALID_FIELD_NAME, func, "no parameter named '%S' was not found in this function declaration.", &param_name.f_str);
                return -1;
            }
            // if already used, make sure it is a REST node
            if(m[j] != 0)
            {
                Data& data = fp->GetData();
                if((data.f_int.Get() & NODE_PARAMETERS_FLAG_REST) == 0)
                {
                    f_error_stream->ErrStrMsg(AS_ERR_INVALID_FIELD_NAME, func, "function parameter name '%S' already used & not a 'rest' (...).", &param_name.f_str);
                    return -1;
                }
            }
#if 0
 {
char buf[256];
size_t sz = sizeof(buf);
param_name.f_str.ToUTF8(buf, sz);
fprintf(stderr, "Found by name [%s] at position %d\n", buf, j);
 }
#endif
        }
        else
        {
            // search for the first parameter
            // which wasn't used yet
            for(j = min; j < max; ++j)
            {
                if(m[j] == 0)
                {
                    fp = &parameters.GetChild(j);
                    break;
                }
            }
            min = j;
            if(j == max)
            {
                // all parameters are already taken
                // check whether the last parameter
                // is of type REST
                j = max - 1;
                fp = &parameters.GetChild(j);
                Data& data = fp->GetData();
                if((data.f_int.Get() & NODE_PARAMETERS_FLAG_REST) == 0)
                {
                    // parameters in the function list
                    // of params are all used up!
// TODO: we can't err here yet; we need to do it only if none of the
//     entries are valid!
//fprintf(stderr, "WARNING: all the function parameters are already assigned and the last one isn't a rest (...)\n");
#if 0
fprintf(stderr, "*** Parameters = ");
parameters.DisplayPtr(stderr);
fprintf(stderr, " child %d = ", j);
fp->DisplayPtr(stderr);
fprintf(stderr, "\n");
#endif
                    return -1;
                }
                // ha! we accept this one!
                j = rest;
                rest++;
            }
#if 0
fprintf(stderr, "Found space at default position %d\n", j);
#endif
        }
        // We reach here only if we find a parameter
        // now we need to check the type to make sure
        // it really is valid
        int depth = MatchType(p, *fp, MATCH_ANY_ANCESTOR);
        if(depth == 0)
        {
            // type doesn't match
#if 0
func.Display(stderr);
if(params)
params->Display(stderr);
#endif
//fprintf(stderr, "++++ Invalid type?!\n");
            return -1;
        }
//fprintf(stderr, "++++ Got it?!?!?! (depth: 0x%08X at %d)\n", depth, j);
        m[j] = depth;
        m[idx + size] = j;
    }

    // if some parameters are not defined, then we need to
    // either have a default value (initializer) or they
    // need to be marked as optional (unchecked)
    // a rest is viewed as an optional parameter
    for(j = min; j < max; ++j)
    {
        if(m[j] == 0)
        {
//fprintf(stderr, "Auto-Setting %d?\n", idx + size);
            m[idx + size] = j;
            idx++;
            NodePtr& param = parameters.GetChild(j);
            Data& data = param.GetData();
            if((data.f_int.Get() & (NODE_PARAMETERS_FLAG_UNCHECKED | NODE_PARAMETERS_FLAG_REST)) == 0)
            {
                NodePtr set;
                int cnt = param.GetChildCount();
                for(int k = 0; k < cnt; ++k)
                {
                    NodePtr& child = param.GetChild(k);
                    Data& data = child.GetData();
                    if(data.f_type == NODE_SET)
                    {
                        set = child;
                        break;
                    }
                }
                if(!set.HasNode())
                {
// TODO: we can't warn here, instead we need to register this function
//     as a possible candidate for that call in case no function does
//     match (and even so, in ECMAScript, we can't really know until
//     run time...)
//fprintf(stderr, "WARNING: missing parameters to call function.\n");
                    return -1;
                }
            }
        }
    }

//fprintf(stderr, "Child added to params\n");
    params->AddChild(match);

    return 0;
}




Node::pointer_t Compiler::find_package(Node::pointer_t& list, String const& name)
{
    NodeLock ln(list);
    size_t const max(list->get_children_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child(list->get_child(idx));
        if(child->get_type() == Node::NODE_DIRECTIVE_LIST)
        {
            Node::pointer_t package(find_package(child, name));  // recursive
            if(package)
            {
                return package;
            }
        }
        else if(child->get_type() == Node::NODE_PACKAGE)
        {
            if(child->get_string() == name)
            {
                // found it!
                return child;
            }
        }
    }

    // not found
    return Node::pointer_t();
}


bool Compiler::find_external_package(Node::pointer_t& import, String const& name, Node::pointer_t& program)
{
    // search a package which has an element named 'name'
    // and has a name which match the identifier specified in 'import'
    Node::pointer_t element;  // ignored
    char const *package_info(find_element(import->get_string(), name, element, nullptr));
    if(package_info == nullptr)
    {
        // not found!
        return false;
    }

    String filename = get_package_filename(package_info);

    // found it, let's get a node for it
    find_module(filename, program);

    // at this time this won't happen because if the find_module()
    // function fails, it exit(1)...
    if(!program.HasNode()) {
        return false;
    }

    // TODO: we should test whether we already ran Offsets()
    Offsets(program);

    return true;
}



bool Compiler::check_import(Node::pointer_t& import, Node::pointer_t& resolution, String const& name, Node::pointer_t params, int search_flags)
{
//fprintf(stderr, "CheckImport(... [%.*S] ..., %d)\n", name.GetLength(), name.Get(), search_flags);
    // search for a package within this program
    // (I'm not too sure, but according to the spec. you can very well
    // have a package within any script file)
    if(find_package_item(f_program, import, resolution, name, params, search_flags))
    {
        return true;
    }

//fprintf(stderr, "Search for an external package instead.\n");
    Node::pointer_t program;
    if(!find_external_package(import, name, program))
    {
        return false;
    }

    return find_package_item(program, import, resolution, name, params, search_flags | SEARCH_FLAG_PACKAGE_MUST_EXIST);
}


bool Compiler::find_package_item(Node::pointer_t& program, Node::pointer_t& import, Node::pointer_t& resolution, String const& name, Node::pointer_t params, int search_flags)
{
    Data& data = import.GetData();

    Node::pointer_t package;
    package = find_package(program, import->get_string());

    if(!package)
    {
        if((search_flags & SEARCH_FLAG_PACKAGE_MUST_EXIST) != 0)
        {
            // this is a bad error! we should always find the
            // packages in this case (i.e. when looking using the
            // database.)
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INTERNAL_ERROR, import->get_position());
            msg << "cannot find package '" << import->get_string() << "' in any of the previously registered packages."
            exit(1);
        }
        return false;
    }

    if(package->get_children_size() == 0)
    {
        return false;
    }

    // setup labels (only the first time around)
    Data& package_data = package.GetData();
    if((package->get_flag(Node::NODE_PACKAGE_FLAG_FOUND_LABELS) == 0)
    {
        package->set_flag(Node::NODE_PACKAGE_FLAG_FOUND_LABELS, true);
        Node::pointer_t child(package->get_child(0));
        find_labels(package, child);
    }

    // search the name of the class/function/variable we're
    // searching for in this package:

    // TODO: Hmmm... could we have the actual node instead?
    Node::pointer_t id(package->create_replacement(Node::NODE_IDENTIFIER));
    id->set_string(name);

//fprintf(stderr, "Found package [%.*S], search field [%.*S]\n", data.f_str.GetLength(), data.f_str.Get(), name.GetLength(), name.Get());
    int funcs = 0;
    if(!find_field(package, id, funcs, resolution, params, search_flags))
    {
        return false;
    }

    // TODO: Can we have an empty resolution here?!
    if(resolution)
    {
        unsigned long attrs = resolution.GetAttrs();
        if(resolution->get_flag(Node::NODE_ATTR_PRIVATE))
        {
            // it is private, we cannot use this item
            // from outside whether it is in the
            // package or a sub-class
            return false;
        }

        if(resolution->get_flag(Node::NODE_ATTR_INTERNAL))
        {
            // it is internal we can only use it from
            // another package
            Node::pointer_t parent(import);
            for(;;)
            {
                parent = parent->get_parent();
                if(!parent)
                {
                    return false;
                }
                switch(parent->get_type())
                {
                case Node::NODE_PACKAGE:
                    break;

                case Node::NODE_ROOT:
                case Node::NODE_PROGRAM:
                    return false;

                }
            }
        }
    }

    // make sure it is compiled (once)
    bool was_referenced(package->get_flag(Node::NODE_PACKAGE_FLAG_REFERENCED));
    package->set_flag(Node::NODE_PACKAGE_FLAG_REFERENCED, true);
    if(was_referenced)
    {
        directive_list(package);
    }

    return true;
}


bool Compiler::IsConstructor(NodePtr& func)
{
    unsigned long attrs = GetAttributes(func);
    // user defined constructor
    if((attrs & NODE_ATTR_CONSTRUCTOR) != 0) {
        return true;
    }

    Data& name = func.GetData();
    NodePtr parent = func;
    for(;;) {
        parent = parent.GetParent();
        if(!parent.HasNode()) {
            return false;
        }
        Data& data = parent.GetData();
        switch(data.f_type) {
        case NODE_PACKAGE:
        case NODE_PROGRAM:
        case NODE_FUNCTION:    // sub-functions can't be constructors
        case NODE_INTERFACE:
            return false;

        case NODE_CLASS:
            // we found the class in question
            return data.f_str == name.f_str;

        default:
            // ignore all the other nodes
            break;

        }
    }
}



bool Compiler::CheckFunction(NodePtr& func, NodePtr& resolution, const String& name, NodePtr *params, int search_flags)
{
    // The fact that a function is marked UNUSED should
    // be an error, but overloading prevents us from
    // generating an error here...
    unsigned long attrs = GetAttributes(func);
    if((attrs & NODE_ATTR_UNUSED) != 0) {
        return false;
    }

    Data& data = func.GetData();
    unsigned long flags = data.f_int.Get();
    if((flags & NODE_FUNCTION_FLAG_GETTER) != 0 && (search_flags & SEARCH_FLAG_GETTER) != 0) {
        String getter("->");
        getter += name;
        if(data.f_str != getter) {
            return false;
        }
    }
    else if((flags & NODE_FUNCTION_FLAG_SETTER) != 0 && (search_flags & SEARCH_FLAG_SETTER) != 0) {
        String setter("<-");
        setter += name;
        if(data.f_str != setter) {
            return false;
        }
    }
    else if(data.f_str != name) {
        return false;
    }

    // That's a function!
    // Find the perfect match (testing prototypes)

#if 0
fprintf(stderr, "***\n*** FUNCTION %p [%.*S] at line %d in %.*S\n***\n",
        params, name.GetLength(), name.Get(), func.GetLine(),
        func.GetFilename().GetLength(), func.GetFilename().Get());
func.Display(stderr);
#endif

    if(params == 0) {
        // getters and setters do not have parameters
        if((flags & (NODE_FUNCTION_FLAG_GETTER | NODE_FUNCTION_FLAG_SETTER)) == 0) {
            // warning: we've got to check whether we've hit a constructor
            //        before to generate an error
            if(!IsConstructor(func)) {
                f_error_stream->ErrStrMsg(AS_ERR_MISMATCH_FUNC_VAR, func, "a variable name was expected, we found the function '%S' instead.", &data.f_str);
            }
            return false;
        }
        DefineFunctionType(func);
    }

    resolution = func;

    return true;
}



bool Compiler::IsDerivedFrom(NodePtr& derived_class, NodePtr& super_class)
{
    if(derived_class.SameAs(super_class)) {
        return true;
    }

    int max = derived_class.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& extends = derived_class.GetChild(idx);
        if(!extends.HasNode()) {
            continue;
        }
        Data& data = extends.GetData();
        if(data.f_type != NODE_EXTENDS
        && data.f_type != NODE_IMPLEMENTS) {
            continue;
        }
        NodePtr& type = extends.GetChild(0);
        Data& data_type = type.GetData();
        if(data_type.f_type == NODE_LIST
        && data.f_type == NODE_IMPLEMENTS) {
            // IMPLEMENTS accepts lists
            int cnt = type.GetChildCount();
            for(int j = 0; j < cnt; ++j) {
                NodePtr& sub_type = type.GetChild(j);
                LinkType(sub_type);
                NodePtr& link = sub_type.GetLink(NodePtr::LINK_INSTANCE);
                if(!link.HasNode()) {
                    continue;
                }
                if(IsDerivedFrom(link, super_class)) {
                    return true;
                }
            }
        }
        else {
            LinkType(type);
            NodePtr& link = type.GetLink(NodePtr::LINK_INSTANCE);
            if(!link.HasNode()) {
                continue;
            }
            if(IsDerivedFrom(link, super_class)) {
                return true;
            }
        }
    }

    return false;
}



NodePtr Compiler::ClassOfMember(NodePtr parent, Data *& data)
{
    for(;;) {
        data = &parent.GetData();
        if(data->f_type == NODE_CLASS
        || data->f_type == NODE_INTERFACE) {
            return parent;
        }
        if(data->f_type == NODE_PACKAGE
        || data->f_type == NODE_PROGRAM
        || data->f_type == NODE_ROOT) {
            parent.ClearNode();
            return parent;
        }
        parent = parent.GetParent();
        if(!parent.HasNode()) {
            return parent;
        }
    }
}



bool Compiler::AreObjectsDerivedFromOneAnother(NodePtr& derived_class, NodePtr& super_class, Data *& data)
{
    NodePtr the_super_class = ClassOfMember(super_class, data);
    if(!the_super_class.HasNode()) {
        return false;
    }
    NodePtr the_derived_class = ClassOfMember(derived_class, data);
    data = 0;
    if(!the_derived_class.HasNode()) {
        return false;
    }

    return IsDerivedFrom(the_derived_class, the_super_class);
}


bool Compiler::CheckName(NodePtr& list, int idx, NodePtr& resolution, NodePtr& id, NodePtr *params, int search_flags)
{
    NodePtr& child = list.GetChild(idx);
    Data& id_data = id.GetData();

    // turned off?
    //unsigned long attrs = GetAttributes(child);
    //if((attrs & NODE_ATTR_FALSE) != 0) {
    //    return false;
    //}

    Data *d = &child.GetData();
    bool result = false;
    switch(d->f_type) {
    case NODE_VAR:    // a VAR is composed of VARIABLEs
    {
        NodeLock ln(child);
        int max = child.GetChildCount();
        for(int idx = 0; idx < max; ++idx) {
            NodePtr& variable = child.GetChild(idx);
            Data& var_name = variable.GetData();

#if 0
 {
char buf[256], str[256];
size_t sz = sizeof(buf);
var_name.f_str.ToUTF8(buf, sz);
sz = sizeof(str);
id_data.f_str.ToUTF8(str, sz);
fprintf(stderr, ">>>> found?! [%s] == [%s]\n", buf, str);
 }
#endif

            if(var_name.f_str == id_data.f_str) {
                // that's a variable!
                // make sure it was parsed
                if((search_flags & SEARCH_FLAG_NO_PARSING) == 0) {
                    Variable(variable, false);
                }
                if(params != 0) {
                    // check whether we're in a call
                    // because if we are the resolution
                    // is the "()" operator instead
                }
                resolution = variable;
                result = true;

//fprintf(stderr, "DEBUG: >>> found a variable of that name.\n");

                break;
            }
        }
    }
        break;

    case NODE_PARAM:
        if(d->f_str == id_data.f_str) {
            resolution = child;
            Data& data = child.GetData();
            data.f_int.Set(data.f_int.Get() | NODE_PARAMETERS_FLAG_REFERENCED);
            return true;
        }
        break;

    case NODE_FUNCTION:
        result = CheckFunction(child, resolution, id_data.f_str, params, search_flags);
        break;

    case NODE_CLASS:
    case NODE_INTERFACE:
        if(d->f_str == id_data.f_str) {
            // That's a class name! (good for a typedef, etc.)
            resolution = child;
            result = true;
        }
        break;

    case NODE_ENUM:
    {
        // first we check whether the name of the enum is what
        // is being referenced (i.e. the type)
        if(id_data.f_str == d->f_str) {
            resolution = list;
            Data& data = resolution.GetData();
            data.f_int.Set(data.f_int.Get() | NODE_VAR_FLAG_INUSE);
            return true;
        }

        // inside an enum we have references to other
        // identifiers of that enum and these need to be
        // checked here
        int max = child.GetChildCount();
        for(int idx = 0; idx < max; ++idx) {
            NodePtr& entry = child.GetChild(idx);
            Data& entry_data = entry.GetData();
            if(id_data.f_str == entry_data.f_str) {
                // this can't be a function, right?
                resolution = entry;
                Data& data = resolution.GetData();
                data.f_int.Set(data.f_int.Get() | NODE_VAR_FLAG_INUSE);
                return true;
            }
        }
    }
        break;

    case NODE_PACKAGE:
        if(d->f_str == id_data.f_str) {
            // That's a package... we have to see packages
            // like classes, to search for more, you need
            // to search inside this package and none other.
            resolution = child;
            return true;
        }
#if 0
        // TODO: auto-import? this works, but I don't think we
        //     want an automatic import of even internal packages?
        //     do we?
        //
        // if it isn't the package itself, it could be an
        // element inside the package
        {
            int funcs = 0;
            if(!FindField(child, id, funcs, resolution, params, search_flags)) {
                break;
            }
        }
        result = true;
fprintf(stderr, "Found inside package! [%.*S]\n", id_data.f_str.GetLength(), id_data.f_str.Get());
        if((d->f_int.Get() & NODE_PACKAGE_FLAG_REFERENCED) == 0) {
fprintf(stderr, "Compile package now!\n");
            DirectiveList(child);
            d->f_int.Set(d->f_int.Get() | NODE_PACKAGE_FLAG_REFERENCED);
        }
#endif
        break;

    case NODE_IMPORT:
        return CheckImport(child, resolution, id_data.f_str, params, search_flags);

    default:
        // ignore anything else for now
        break;

    }

    if(!result) {
        return false;
    }

    if(!resolution.HasNode()) {
        // this is kind of bad since we can't test for
        // the scope...
        return true;
    }

    unsigned long attrs = GetAttributes(resolution);

    if((attrs & NODE_ATTR_PRIVATE) != 0) {
        // Note that an interface and a package
        // can also have private members
        Data *data;
        NodePtr the_resolution_class = ClassOfMember(resolution, data);
        if(!the_resolution_class.HasNode()) {
            f_err_flags |= SEARCH_ERROR_PRIVATE;
            resolution.ClearNode();
            return false;
        }
        if(data->f_type == NODE_PACKAGE) {
            f_err_flags |= SEARCH_ERROR_PRIVATE_PACKAGE;
            resolution.ClearNode();
            return false;
        }
        if(data->f_type != NODE_CLASS
        && data->f_type != NODE_INTERFACE) {
            f_err_flags |= SEARCH_ERROR_WRONG_PRIVATE;
            resolution.ClearNode();
            return false;
        }
        NodePtr the_id_class = ClassOfMember(id, data);
        if(!the_id_class.HasNode()) {
            f_err_flags |= SEARCH_ERROR_PRIVATE;
            resolution.ClearNode();
            return false;
        }
        if(!the_id_class.SameAs(the_resolution_class)) {
            f_err_flags |= SEARCH_ERROR_PRIVATE;
            resolution.ClearNode();
            return false;
        }
    }

    if((attrs & NODE_ATTR_PROTECTED) != 0) {
        // Note that an interface can also have protected members
        Data *data;
        if(!AreObjectsDerivedFromOneAnother(id, resolution, data)) {
            if(data != 0
            && data->f_type != NODE_CLASS
            && data->f_type != NODE_INTERFACE) {
                f_err_flags |= SEARCH_ERROR_WRONG_PROTECTED;
                resolution.ClearNode();
                return false;
            }
            f_err_flags |= SEARCH_ERROR_PROTECTED;
            resolution.ClearNode();
            return false;
        }
    }

    if(d->f_type == NODE_FUNCTION && params != 0) {
        if(CheckFunctionWithParams(child, params) < 0) {
            return false;
        }
    }

    return true;
}




bool Compiler::FuncsName(int& funcs, NodePtr& resolution, bool increment)
{
    if(!resolution.HasNode()) {
        return true;
    }
    GetAttributes(resolution);

    Data& data = resolution.GetData();
    if(data.f_type != NODE_FUNCTION) {
        // TODO: do we really ignore those?!
        return funcs == 0;
    }
    if((data.f_int.Get() & (NODE_FUNCTION_FLAG_GETTER | NODE_FUNCTION_FLAG_SETTER)) != 0) {
        // this is viewed as a variable; also, there is no
        // parameters to a getter and thus no way to overload
        // these; the setter has a parameter though but you
        // cannot decide what it is going to be
        return funcs == 0;
    }

    if(increment) {
        funcs++;
    }

    return false;
}




bool Compiler::BestParamMatchDerivedFrom(NodePtr& best, NodePtr& match)
{
    Data *data;

//fprintf(stderr, "NOTE: checking best <- match.\n");
    if(AreObjectsDerivedFromOneAnother(best, match, data)) {
        // if best is in a class derived from
        // the class where we found match, then
        // this isn't an error, we just keep best
//fprintf(stderr, "RESULT! checking best <- match is true.\n");
        return true;
    }

//fprintf(stderr, "NOTE: checking match <- best.\n");
    if(AreObjectsDerivedFromOneAnother(match, best, data)) {
        // if match is in a class derived from
        // the class where we found best, then
        // this isn't an error, we just keep match
        best = match;
//fprintf(stderr, "RESULT! checking match <- best is true.\n");
        return true;
    }

    Data& best_data = best.GetLink(NodePtr::LINK_INSTANCE).GetData();
    f_error_stream->ErrStrMsg(AS_ERR_DUPLICATES, best, "found two functions named '%S' and both have the same prototype. Cannot determine which one to use.", &best_data.f_str);

    return false;
}



bool Compiler::BestParamMatch(NodePtr& best, NodePtr& match)
{
    Data& b_data = best.GetData();
    Data& m_data = match.GetData();

    // unprototyped?
    int b_sz = b_data.f_user_data.Size();
    int m_sz = m_data.f_user_data.Size();
    if(b_sz == 0)
    {
        if(m_sz == 0)
        {
            return BestParamMatchDerivedFrom(best, match);
        }
        // best had no prototype, but match has one, so we keep match
        best = match;
        return true;
    }

    if(m_sz == 0) {
        // we keep best in this case since it has a prototype
        // and not match
        return true;
    }

    int b_more = 0;
    int m_more = 0;
    for(int idx = 0; idx < b_sz && idx < m_sz; ++idx)
    {
        int const r(b_data.f_user_data[idx] - m_data.f_user_data[idx]);
        if(r < 0)
        {
            b_more++;
        }
        else if(r > 0)
        {
            m_more++;
        }
    }

    // if both are 0 or both not 0 then we can't decide
    if((b_more != 0) ^ (m_more == 0)) {
        return BestParamMatchDerivedFrom(best, match);
    }

    // match's better!
    if(m_more != 0) {
        best = match;
    }

    return true;
}



bool Compiler::SelectBestFunc(NodePtr *params, NodePtr& resolution)
{
    // We found one or more function which matched the name
    AS_ASSERT(params != 0);
    bool found = true;

    // search for the best match
    //NodeLock ln(directive_list); -- we're managing this list here
    int cnt = params->GetChildCount();
    NodePtr best;
    int idx = 0, prev = -1;
    while(idx < cnt) {
        NodePtr& match = params->GetChild(idx);
        Data& data = match.GetData();
        if(data.f_type == NODE_PARAM_MATCH) {
            if(best.HasNode()) {
                // compare best & match
                if(!BestParamMatch(best, match)) {
                    found = false;
                }
                if(best.SameAs(match)) {
                    params->DeleteChild(prev);
                    prev = idx;
                }
                else {
                    params->DeleteChild(idx);
                }
                --cnt;
            }
            else {
                prev = idx;
                best = match;
                ++idx;
            }
        }
        else {
            ++idx;
        }
    }
    // we should always have a best node
    AS_ASSERT(best.HasNode());

    if(!best.HasNode()) {
        found = false;
    }
    if(found) {
        // we found a better one! and no error occured
        resolution = best.GetLink(NodePtr::LINK_INSTANCE);
    }

    return found;
}



bool Compiler::resolve_name(Node::pointer_t list, Node::pointer_t id, Node::pointer_t& resolution, Node::pointer_t params, int const search_flags)
{
    RestoreFlags restore_flags(this);

    // just in case the caller is reusing the same node
    resolution.ClearNode();

    Data& data = id.GetData();

    // in some cases we may want to resolve a name specified in a string
    // (i.e. test["me"])
    AS_ASSERT( data.f_type == NODE_IDENTIFIER
        || data.f_type == NODE_VIDENTIFIER
        || data.f_type == NODE_STRING);

#if 0
char buf[256];
size_t sz = sizeof(buf);
data.f_str.ToUTF8(buf, sz);
fprintf(stderr, "DEBUG: resolving identifier '%s' (Parent: ", buf);
id.GetParent().DisplayPtr(stderr);
fprintf(stderr, ").\n");
#endif

    //
    // Search for the parent list of directives; in that list, search
    // for the identifier; if not found, try again with the parent
    // of that list of directives (unless we find an import in which
    // case we first try the import)
    //
    // Note that the currently effective with()'s and use namespace's
    // are defined in the f_scope variable. This is used here to know
    // whether the name matches an entry or not.
    //

    // a list of functions whenever the name resolves to a function
    int funcs = 0;

    //NodePtr list = id;
    NodePtr& parent = list.GetParent();
    Data *d = &parent.GetData();
    if(d->f_type == NODE_WITH) {
        // we're currently defining the WITH object, skip the
        // WITH itself!
        list = parent;
    }
    int module = 0;        // 0 is user module being compiled
    for(;;) {
        // we will start searching at this offset; first backward
        // and then forward
        int offset = 0;

        // This function should never be called from the Program()
        // also, 'id' can't be a directive list (it has to be an
        // identifier, a member or a string!)
        // For these reasons, we can start the following loop with
        // a GetParent() in all cases.
        if(module == 0) {
            // when we were inside the function parameter
            // list we don't want to check out the function
            // otherwise we could have a forward search of
            // the parameters which we disallow (only backward
            // search is allowed in that list)
            d = &list.GetData();
            if(d->f_type == NODE_PARAMETERS) {
//fprintf(stderr, "Skipping parameters?!\n");
                list = list.GetParent();
            }

            do {
                offset = list.GetOffset();
                list = list.GetParent();
                d = &list.GetData();
                if(d->f_type == NODE_EXTENDS
                || d->f_type == NODE_IMPLEMENTS) {
                    list = list.GetParent();
                    continue;
                }
            } while(d->f_type != NODE_DIRECTIVE_LIST
                && d->f_type != NODE_FOR
                && d->f_type != NODE_WITH
                //&& d->f_type != NODE_PACKAGE -- not necessary, the first item is a NODE_DIRECTIVE_LIST
                && d->f_type != NODE_PROGRAM
                && d->f_type != NODE_FUNCTION
                && d->f_type != NODE_PARAMETERS
                && d->f_type != NODE_ENUM
                && d->f_type != NODE_CATCH
                && d->f_type != NODE_CLASS
                && d->f_type != NODE_INTERFACE);
        }

        if(d->f_type == NODE_PROGRAM || module != 0) {
            // not resolved
            switch(module) {
            case 0:
                module = 1;
                if(g_global_import.HasNode()
                && g_global_import.GetChildCount() > 0) {
                    list = g_global_import.GetChild(0);
                    d = &list.GetData();
                    break;
                }
                /*FALLTHROUGH*/
            case 1:
                module = 2;
                if(g_system_import.HasNode()
                && g_system_import.GetChildCount() > 0) {
                    list = g_system_import.GetChild(0);
                    d = &list.GetData();
                    break;
                }
                /*FALLTHROUGH*/
            case 2:
                module = 3;
                if(g_native_import.HasNode()
                && g_native_import.GetChildCount() > 0) {
                    list = g_native_import.GetChild(0);
                    d = &list.GetData();
                    break;
                }
                /*FALLTHROUGH*/
            case 3:
                // no more default list of directives...
                module = 4;
                break;

            }
        }
        if(module == 4) {
            // didn't find a variable and such, but
            // we may have found a function (see below
            // after the forever loop breaking here)
            break;
        }

        NodeLock ln(list);
        int max = list.GetChildCount();
        switch(d->f_type) {
        case Node::NODE_DIRECTIVE_LIST:
        {
            // okay! we've got a list of directives
            // backward loop up first since in 99% of cases that
            // will be enough...
            AS_ASSERT(offset < max);
            int idx = offset;
            while(idx > 0) {
                idx--;
                if(CheckName(list, idx, resolution, id, params, search_flags)) {
                    if(FuncsName(funcs, resolution)) {
                        return true;
                    }
                }
            }

            // forward look up is also available in ECMAScript...
            // (necessary in case function A calls function B
            // and function B calls function A).
            for(idx = offset; idx < max; ++idx) {
                if(CheckName(list, idx, resolution, id, params, search_flags)) {
                    // TODO: if it is a variable it needs
                    //     to be a constant...
                    if(FuncsName(funcs, resolution)) {
#if 0
fprintf(stderr, "DEBUG: in a DIRECTIVE_LIST resolution is = ");
resolution.DisplayPtr(stderr);
fprintf(stderr, "\n");
#endif
                        return true;
                    }
                }
            }
        }
            break;

        case NODE_FOR:
        {
            // the first member of a for can include variable
            // definitions

#if 0
NodePtr& var = list.GetChild(0);
Data& d = var.GetData();
fprintf(stderr, "FOR vars in ");
var.DisplayPtr(stderr);
fprintf(stderr, " [Type = %d]\n", d.f_type);
#endif

            if(max > 0 && check_name(list, 0, resolution, id, params, search_flags))
            {
                if(funcs_name(funcs, resolution))
                {
                    return true;
                }
            }
        }
            break;

#if 0
        case NODE_PACKAGE:
        {
            // From inside a package, we have an implicit
            //    IMPORT <package name>;
            //
            // This is required to enable a multiple files
            // package definition which ease the development
            // of really large packages.
            if(CheckImport(list, resolution, data.f_str, params, search_flags)) {
                return true;
            }
        }
            break;
#endif

        case NODE_WITH:
        {
            if(max != 2) {
                break;
            }
            // ha! we found a valid WITH instruction, let's
            // search for this name in the corresponding
            // object type instead (i.e. a field of the object)
            NodePtr& type = list.GetChild(0);
            if(type.HasNode()) {
                NodePtr& link = type.GetLink(NodePtr::LINK_INSTANCE);
                if(link.HasNode()) {
                    if(ResolveField(link, id, resolution, params, search_flags)) {
                        // Mark this identifier as a
                        // reference to a WITH object
                        data.f_int.Set(data.f_int.Get() | NODE_IDENTIFIER_FLAG_WITH);
                        // TODO: we certainly want to compare
                        //     all the field functions and the
                        //     other functions... at this time,
                        //     err if we get a field function
                        //     and others are ignored!
                        AS_ASSERT(funcs == 0);
                        return true;
                    }
                }
            }
        }
            break;

        case NODE_FUNCTION:
        {
            // search the list of parameters for a
            // corresponding name
            for(int idx = 0; idx < max; ++idx) {
                NodePtr& parameters = list.GetChild(idx);
                Data& parameters_data = parameters.GetData();
                if(parameters_data.f_type == NODE_PARAMETERS) {
                    NodeLock ln(parameters);
                    int cnt = parameters.GetChildCount();
                    for(int j = 0; j < cnt; ++j) {
                        if(CheckName(parameters, j, resolution, id, params, search_flags)) {
                            if(FuncsName(funcs, resolution)) {
#if 0
fprintf(stderr, "DEBUG: in a FUNCTION resolution is = ");
resolution.DisplayPtr(stderr);
fprintf(stderr, "\n");
#endif
                                return true;
                            }
                        }
                    }
                    break;
                }
            }
        }
            break;

        case NODE_PARAMETERS:
        {
            // Wow! I can't believe I'm implementing this...
            // So we will be able to reference the previous
            // parameters in the default value of the following
            // parameters; and that makes sense, it's available
            // in C++ templates, right?!
            // And guess what, that's just this little loop.
            // That's it. Big deal, hey?! 8-)
            AS_ASSERT(offset < max);
            int idx = offset;
            while(idx > 0) {
                idx--;
                if(CheckName(list, idx, resolution, id, params, search_flags)) {
                    if(FuncsName(funcs, resolution)) {
                        return true;
                    }
                }
            }
        }
            break;

        case NODE_CATCH:
        {
            // a catch can have a parameter of its own
            NodePtr& parameters = list.GetChild(0);
            if(parameters.GetChildCount() > 0) {
                if(CheckName(parameters, 0, resolution, id, params, search_flags)) {
                    if(FuncsName(funcs, resolution)) {
                        return true;
                    }
                }
            }
        }
            break;

        case NODE_ENUM:
            // first we check whether the name of the enum is what
            // is being referenced (i.e. the type)
            if(data.f_str == d->f_str) {
                resolution = list;
                Data& data = resolution.GetData();
                data.f_int.Set(data.f_int.Get() | NODE_VAR_FLAG_INUSE);
                return true;
            }

            // inside an enum we have references to other
            // identifiers of that enum and these need to be
            // checked here
            //
            // And note that these are not in any way affected
            // by scope attributes
            for(int idx = 0; idx < max; ++idx) {
                NodePtr& entry = list.GetChild(idx);
                Data& entry_data = entry.GetData();
                if(data.f_str == entry_data.f_str) {
                    // this can't be a function, right?
                    resolution = entry;
                    if(FuncsName(funcs, resolution)) {
                        Data& data = resolution.GetData();
                        data.f_int.Set(data.f_int.Get() | NODE_VAR_FLAG_INUSE);
                        return true;
                    }
                }
            }
            break;

        case NODE_CLASS:
        case NODE_INTERFACE:
            // We need to search the extends and implements
            if(FindInExtends(list, id, funcs, resolution, params, search_flags)) {
                if(FuncsName(funcs, resolution)) {
                    return true;
                }
            }
            break;

        default:
            fprintf(stderr, "INTERNAL ERROR: unhandled type in Compiler::resolve_name()\n");
            AS_ASSERT(0);
            break;

        }
    }

    resolution.ClearNode();

    if(funcs != 0) {
        if(SelectBestFunc(params, resolution)) {
            return true;
        }
    }

    PrintSearchErrors(id);

    return false;
}



void Compiler::PrintSearchErrors(const NodePtr& name)
{
    // all failed, check whether we have errors...
    if(f_err_flags == 0) {
        return;
    }

    Data& data = name.GetData();
    f_error_stream->ErrStrMsg(AS_ERR_CANNOT_MATCH, name, "the name '%S' could not be resolved because:", &data.f_str);
    if((f_err_flags & SEARCH_ERROR_PRIVATE) != 0) {
        f_error_stream->ErrMsg(AS_ERR_CANNOT_MATCH, name, "   You cannot access a private class member from outside that very class.");
    }
    if((f_err_flags & SEARCH_ERROR_PROTECTED) != 0) {
        f_error_stream->ErrMsg(AS_ERR_CANNOT_MATCH, name, "   You cannot access a protected class member from outside a class or its derived classes.");
    }
    if((f_err_flags & SEARCH_ERROR_PROTOTYPE) != 0) {
        f_error_stream->ErrMsg(AS_ERR_CANNOT_MATCH, name, "   One or more functions were found, but none matched the input parameters.");
    }
    if((f_err_flags & SEARCH_ERROR_WRONG_PRIVATE) != 0) {
        f_error_stream->ErrMsg(AS_ERR_CANNOT_MATCH, name, "   You cannot use the private attribute outside of a package or a class.");
    }
    if((f_err_flags & SEARCH_ERROR_WRONG_PROTECTED) != 0) {
        f_error_stream->ErrMsg(AS_ERR_CANNOT_MATCH, name, "   You cannot use the protected attribute outside of a class.");
    }
    if((f_err_flags & SEARCH_ERROR_PRIVATE_PACKAGE) != 0) {
        f_error_stream->ErrMsg(AS_ERR_CANNOT_MATCH, name, "   You cannot access a package private declaration from outside of that package.");
    }
}



void Compiler::CallAddMissingParams(NodePtr& call, NodePtr& params)
{
    // any children?
    int idx = params.GetChildCount();
    if(idx <= 0) {
        return;
    }

    // if we have a parameter match, it has to be at the end
    --idx;
    NodePtr& match = params.GetChild(idx);
    Data& match_data = match.GetData();
    if(match_data.f_type != NODE_PARAM_MATCH) {
        // ERROR: not a param match with a valid best match?!
        AS_ASSERT(0);
        return;
    }

    // found it, now we want to copy the array of indices to the
    // call instruction
    //int size = match_data.f_user_data.size() / 2;
    Data& data = call.GetData();
    data.f_user_data = match_data.f_user_data;
    //data.f_user_data.New(size);
    //int *indices = 0;
    // size is zero for functions with no parameters
    //if(size > 0)
    //{
        //indices = data.f_user_data.Buffer();
        //memcpy(indices, match_data.f_user_data.Buffer() + size,
                        //sizeof(int) * size);
    //}
    params.DeleteChild(idx);

#if 0
fprintf(stderr, "         FIRST ON DATA:\n");
call.Display(stderr);
fprintf(stderr, "\n\n");
#endif

    if(idx < size) {
        // get the list of parameters of the function
        NodePtr parameters;
        NodePtr& function = call.GetLink(NodePtr::LINK_INSTANCE);
        int max = function.GetChildCount();
        for(int j = 0; j < max; ++j) {
            NodePtr& child = function.GetChild(j);
            Data& data = child.GetData();
            if(data.f_type == NODE_PARAMETERS) {
                parameters = child;
                break;
            }
        }
        // Functions with no parameters just have no parameters node
        //AS_ASSERT(parameters.HasNode());
        if(parameters.HasNode()) {
            max = parameters.GetChildCount();
            while(idx < size) {
                AS_ASSERT(data.f_user_data[idx] < max);
                if(data.f_user_data[idx] < max) {
                    NodePtr& param = parameters.GetChild(data.f_user_data[idx]);
                    bool has_set = false;
                    int cnt = param.GetChildCount();
                    for(int k = 0; k < cnt; ++k) {
                        NodePtr& set = param.GetChild(k);
                        Data& data = set.GetData();
                        if(data.f_type == NODE_SET) {
                            has_set = true;
                            NodePtr auto_param;
                            auto_param.CreateNode(NODE_AUTO);
                            auto_param.CopyInputInfo(set);
                            auto_param.SetLink(NodePtr::LINK_INSTANCE, set.GetChild(0));
                            params.AddChild(auto_param);
                            break;
                        }
                    }
                    if(!has_set) {
                        // thought it should be
                        // automatic we actually force
                        // the undefined value here
                        NodePtr undefined;
                        undefined.CreateNode(NODE_UNDEFINED);
                        undefined.CopyInputInfo(call);
                        params.AddChild(undefined);
                    }
                }
                idx++;
            }
        }
    }
}



bool Compiler::ResolveCall(NodePtr& call)
{
    int        max;

    Data& data = call.GetData();

    AS_ASSERT(data.f_type == NODE_CALL);

    max = call.GetChildCount();
    if(max != 2) {
        return false;
    }
    NodeLock ln(call);

    // resolve all the parameters' expressions first
    // the parameters are always in a NODE_LIST
    // and no parameters is equivalent to an empty NODE_LIST
    // and that is an expression, but we don't want to type
    // that expression since it isn't necessary so we go
    // through the list here instead
    NodePtr params;
    params.SetNode(call.GetChild(1));
    int count = params.GetChildCount();
//fprintf(stderr, "ResolveCall() with %d expressions\n", count);
//call.Display(stderr);
    for(int idx = 0; idx < count; ++idx) {
        NodePtr& child = params.GetChild(idx);
        Expression(child);
    }

    // check the name expression
    NodePtr& id = call.GetChild(0);

    // if possible, resolve the function name
    Data& name = id.GetData();
    if(name.f_type == NODE_IDENTIFIER) {
        // straight identifiers can be resolved at compile time;
        // these need to be function names
        NodePtr resolution;
        int errcnt = f_error_stream->ErrCount();
        if(resolve_name(id, id, resolution, params, SEARCH_FLAG_GETTER)) {
//fprintf(stderr, "Cool! identifier found [%s]\n", name.f_str.GetUTF8());
            Data& res_data = resolution.GetData();
            if(res_data.f_type == NODE_CLASS
            || res_data.f_type == NODE_INTERFACE) {
                // this looks like a cast, but if the parent is
                // the NEW operator, then it is really a call!
                // yet that is caught in ExpressionNew()
//fprintf(stderr, "This is not a call, it is a cast instead! [%s]\n", name.f_str.GetUTF8());
                ln.Unlock();
                NodePtr type = call.GetChild(0);
                NodePtr expr = call.GetChild(1);
                call.DeleteChild(0);
                call.DeleteChild(0);    // 1 is now 0
                call.AddChild(expr);
                call.AddChild(type);
                type.SetLink(NodePtr::LINK_INSTANCE, resolution);
                Data& data = call.GetData();
                data.f_type = NODE_AS;
                return true;
            }
            else if(res_data.f_type == NODE_VARIABLE) {
                // if it is a variable, we need to test
                // the type for a "()" operator
//fprintf(stderr, "Looking for a '()' operator\n");
                NodePtr& var_class = resolution.GetLink(NodePtr::LINK_TYPE);
                if(var_class.HasNode()) {
                    id.SetLink(NodePtr::LINK_INSTANCE, var_class);
                    // search for a function named "()"
                    //NodePtr l;
                    //l.CreateNode(NODE_IDENTIFIER);
                    //Data& lname = l.GetData();
                    //lname.f_str = "left";
                    ln.Unlock();
                    NodePtr all_params = call.GetChild(1);
                    call.DeleteChild(1);
                    //NodePtr op_params;
                    //op_params.CreateNode(NODE_LIST);
                    //op_params.AddChild(l);
                    NodePtr op;
                    op.CreateNode(NODE_IDENTIFIER);
                    Data& op_data = op.GetData();
                    op_data.f_str = "()";
                    op.AddChild(all_params);
                    Offsets(op);
                    NodePtr func;
                    int del = call.GetChildCount();
                    call.AddChild(op);
                    int funcs = 0;
//fprintf(stderr, "Find the field now... for a '()' operator\n");
                    bool result = FindField(var_class, op, funcs, func, &params, 0);
                    call.DeleteChild(del);
                    if(result) {
                        resolution = func;
                        NodePtr identifier = id;
                        NodePtr member;
                        member.CreateNode(NODE_MEMBER);
                        call.SetChild(0, member);
                        op.DeleteChild(0);
                        if(call.GetChildCount() > 1) {
                            call.SetChild(1, all_params);
                        }
                        else {
                            call.AddChild(all_params);
                        }
                        member.AddChild(identifier);
                        member.AddChild(op);
                    }
                    else {
                        Data& data = var_class.GetData();
                        f_error_stream->ErrStrMsg(AS_ERR_UNKNOWN_OPERATOR, call, "no '()' operators found in '%S'.", &data.f_str);
                        return false;
                    }
                }
                else {
                    f_error_stream->ErrMsg(AS_ERR_INTERNAL_ERROR, call, "getters and setters not supported yet.");
                }
            }
            else if(res_data.f_type != NODE_FUNCTION) {
                f_error_stream->ErrStrMsg(AS_ERR_INVALID_TYPE, call, "'%S' was expected to be a type, a variable or a function.", &name.f_str);
                return false;
            }
            //
            // If the resolution is in a class that means it is in 'this'
            // class and thus we want to change the call to a member call:
            //
            //    this.<name>(params);
            //
            // This is important for at least Flash 7 which doesn't get it
            // otherwise, I don't think it would be required otherwise (i.e Flash
            // 7.x searches for a global function on that name!)
            //
            Data *d;
            NodePtr res_class = ClassOfMember(resolution, d);
            if(res_class.HasNode()) {
                ln.Unlock();
                NodePtr identifier = id;
                NodePtr member;
                member.CreateNode(NODE_MEMBER);
                call.SetChild(0, member);
                NodePtr this_expr;
                this_expr.CreateNode(NODE_THIS);
                member.AddChild(this_expr);
                member.AddChild(identifier);
            }
            call.SetLink(NodePtr::LINK_INSTANCE, resolution);
            NodePtr& type = resolution.GetLink(NodePtr::LINK_TYPE);
            if(type.HasNode()) {
                call.SetLink(NodePtr::LINK_TYPE, type);
            }
            CallAddMissingParams(call, params);
            return true;
        }
        if(errcnt == f_error_stream->ErrCount()) {
            f_error_stream->ErrStrMsg(AS_ERR_NOT_FOUND, call, "function named '%S' not found.", &name.f_str);
            return false;
        }
    }
    else {
        // a dynamic expression can't always be
        // resolved at compile time
        Expression(id, &params);

        int count = params.GetChildCount();
        if(count > 0) {
            NodePtr& last = params.GetChild(count - 1);
            Data& data = last.GetData();
            if(data.f_type == NODE_PARAM_MATCH) {
                params.DeleteChild(count - 1);
            }
        }

        NodePtr& type = id.GetLink(NodePtr::LINK_TYPE);
        call.SetLink(NodePtr::LINK_TYPE, type);
    }

    return false;
}



// we can simplify constant variables with their content whenever that's
// a string, number or other non-dynamic constant
bool Compiler::ReplaceConstantVariable(NodePtr& replace, NodePtr& resolution)
{
    int        idx, max;

    Data& data = resolution.GetData();
    if(data.f_type != NODE_VARIABLE) {
        return false;
    }

    if((data.f_int.Get() & NODE_VAR_FLAG_CONST) == 0) {
        return false;
    }

    NodeLock ln(resolution);
    max = resolution.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr& set = resolution.GetChild(idx);
        Data& data = set.GetData();
        if(data.f_type != NODE_SET) {
            continue;
        }

        f_optimizer.Optimize(set);

        if(set.GetChildCount() != 1) {
            return false;
        }
        NodeLock ln(set);

        NodePtr& value = set.GetChild(0);
        TypeExpr(value);

        Data& value_data = value.GetData();
        switch(value_data.f_type) {
        case NODE_STRING:
        case NODE_INT64:
        case NODE_FLOAT64:
        case NODE_TRUE:
        case NODE_FALSE:
        case NODE_NULL:
        case NODE_UNDEFINED:
        case NODE_REGULAR_EXPRESSION:

#if 0
{
Data& d = replace.GetData();
fprintf(stderr, "Replace is of type %d\n", d.f_type);
replace.Display(stderr, 2);
}
#endif

            replace.Clone(value);
            return true;

        default:
            // dynamic expression, can't
            // be resolved at compile time...
            return false;

        }
        /*NOTREACHED*/
    }

    return false;
}



void Compiler::ResolveInternalType(NodePtr& parent, const char *type, NodePtr& resolution)
{
    NodePtr id;

    // create a temporary identifier
    id.CreateNode(NODE_IDENTIFIER);
    int idx = parent.GetChildCount();
    parent.AddChild(id);
    Data& data = id.GetData();
    data.f_str = type;

    Offsets(parent);

    // search for the identifier which is an internal type name
    bool r;
    {
        NodeLock ln(parent);
        r = resolve_name(id, id, resolution, Node::pointer_t(), 0);
    }

    // get rid of the temporary identifier
    parent.DeleteChild(idx);

    if(!r) {
        // if the compiler can't find an internal type, that's really bad!
        fprintf(stderr, "INTERNAL ERROR in " __FILE__ " at line %d: cannot find internal type '%s'.\n", __LINE__, type);
        AS_ASSERT(0);
        exit(1);
    }

    return;
}



void Compiler::TypeExpr(NodePtr& expr)
{
    NodePtr resolution;

    // already typed?
    if(expr.GetLink(NodePtr::LINK_TYPE).HasNode()) {
        return;
    }

    Data& data = expr.GetData();

    switch(data.f_type) {
    case NODE_STRING:
        ResolveInternalType(expr, "String", resolution);
        expr.SetLink(NodePtr::LINK_TYPE, resolution);
        break;

    case NODE_INT64:
        ResolveInternalType(expr, "Integer", resolution);
        expr.SetLink(NodePtr::LINK_TYPE, resolution);
        break;

    case NODE_FLOAT64:
        ResolveInternalType(expr, "Double", resolution);
        expr.SetLink(NodePtr::LINK_TYPE, resolution);
        break;

    case NODE_TRUE:
    case NODE_FALSE:
        ResolveInternalType(expr, "Boolean", resolution);
        expr.SetLink(NodePtr::LINK_TYPE, resolution);
        break;

    case NODE_OBJECT_LITERAL:
        ResolveInternalType(expr, "Object", resolution);
        expr.SetLink(NodePtr::LINK_TYPE, resolution);
        break;

    case NODE_ARRAY_LITERAL:
        ResolveInternalType(expr, "Array", resolution);
        expr.SetLink(NodePtr::LINK_TYPE, resolution);
        break;

    default:
    {
        NodePtr& node = expr.GetLink(NodePtr::LINK_INSTANCE);
        if(!node.HasNode()) {
            break;
        }
        Data& data = node.GetData();
        if(data.f_type != NODE_VARIABLE || node.GetChildCount() <= 0) {
            break;
        }
        NodePtr& type = node.GetChild(0);
        Data& type_data = type.GetData();
        if(type_data.f_type == NODE_SET) {
            break;
        }
        NodePtr& instance = type.GetLink(NodePtr::LINK_INSTANCE);
        if(!instance.HasNode()) {
            // TODO: resolve that if not done yet (it should
            //     always already be at this time)
            fprintf(stderr, "Type missing?!\n");
            AS_ASSERT(0);
        }
        expr.SetLink(NodePtr::LINK_TYPE, instance);
    }
        break;

    }

    return;
}






void Compiler::AssignmentOperator(NodePtr& expr)
{
    bool    is_var = false;

    NodePtr var;    // in case this assignment is also a definition

    NodePtr& left = expr.GetChild(0);
    Data& data = left.GetData();
    if(data.f_type == NODE_IDENTIFIER) {
        // this may be like a VAR <name> = ...
        NodePtr resolution;
        if(resolve_name(left, left, resolution, Node::pointer_t(), 0)) {
            Data& data = resolution.GetData();
            bool valid = false;
            if(data.f_type == NODE_VARIABLE) {
                if((data.f_int.Get() & NODE_VAR_FLAG_CONST) != 0) {
                    f_error_stream->ErrStrMsg(AS_ERR_CANNOT_OVERWRITE_CONST, left, "you cannot assign a value to the constant variable '%S'.", &data.f_str);
                }
                else {
                    valid = true;
                }
            }
            else if(data.f_type == NODE_PARAM) {
                if((data.f_int.Get() & NODE_PARAMETERS_FLAG_CONST) != 0) {
                    f_error_stream->ErrStrMsg(AS_ERR_CANNOT_OVERWRITE_CONST, left, "you cannot assign a value to the constant function parameter '%S'.", &data.f_str);
                }
                else {
                    valid = true;
                }
            }
            else {
                f_error_stream->ErrMsg(AS_ERR_CANNOT_OVERLOAD, left, "you cannot assign but a variable or a function parameter.");
            }
            if(valid) {
                left.SetLink(NodePtr::LINK_INSTANCE, resolution);
                left.SetLink(NodePtr::LINK_TYPE, resolution.GetLink(NodePtr::LINK_TYPE));
            }
        }
        else { // it is a missing VAR!
            is_var = true;

            // we need to put this variable in the function
            // in which it is encapsulated, if there is
            // such a function so it can be marked as local
            // for that we create a var ourselves
            NodePtr variable, set;
            var.CreateNode(NODE_VAR);
            var.CopyInputInfo(left);
            Data& var_data = var.GetData();
            var_data.f_int.Set(NODE_VAR_FLAG_TOADD | NODE_VAR_FLAG_DEFINING);
            variable.CreateNode(NODE_VARIABLE);
            variable.CopyInputInfo(left);
            var.AddChild(variable);
            Data& variable_data = variable.GetData();
            variable_data.f_str = data.f_str;
            NodePtr parent = left;
            NodePtr last_directive;
            for(;;) {
                parent = parent.GetParent();
                Data& parent_data = parent.GetData();
                if(parent_data.f_type == NODE_DIRECTIVE_LIST) {
                    last_directive = parent;
                }
                else if(parent_data.f_type == NODE_FUNCTION) {
                    variable_data.f_int.Set(variable_data.f_int.Get() | NODE_VAR_FLAG_LOCAL);
                    parent.AddVariable(variable);
                    break;
                }
                else if(parent_data.f_type == NODE_PROGRAM
                || parent_data.f_type == NODE_CLASS
                || parent_data.f_type == NODE_INTERFACE
                || parent_data.f_type == NODE_PACKAGE) {
                    // not found?!
                    break;
                }
            }
            left.SetLink(NodePtr::LINK_INSTANCE, variable);

            // We cannot call InsertChild()
            // here since it would be in our
            // locked parent. So instead we
            // only add it to the list of
            // variables of the directive list
            // and later we will also add it
            // at the top of the list
            if(last_directive.HasNode()) {
                //parent.InsertChild(0, var);
                last_directive.AddVariable(variable);
                Data& data = last_directive.GetData();
                data.f_int.Set(data.f_int.Get() | NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES);
            }
        }
    }
    // TODO: handle setters
    else if(data.f_type == NODE_MEMBER) {
        // we parsed?
        if(!left.GetLink(NodePtr::LINK_TYPE).HasNode()) {
            // try to optimize the expression before to compile it
            // (it can make a huge difference!)
            f_optimizer.Optimize(left);
            //NodePtr& right = expr.GetChild(1);

            ResolveMember(left, 0, SEARCH_FLAG_SETTER);

            // setters have to be treated here because within ResolveMember()
            // we do not have access to the assignment and that's what needs
            // to change to a call.
            NodePtr& resolution = left.GetLink(NodePtr::LINK_INSTANCE);
            if(resolution.HasNode()) {
                Data& res_data = resolution.GetData();
                if(res_data.f_type == NODE_FUNCTION
                && (res_data.f_int.Get() & NODE_FUNCTION_FLAG_SETTER) != 0) {
fprintf(stderr, "CAUGHT! setter...\n");
                    // so expr is a MEMBER at this time
                    // it has two children
                    //NodePtr left = expr.GetChild(0);
                    NodePtr right = expr.GetChild(1);
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
                    NodePtr field = left.GetChild(1);
                    Data& field_data = field.GetData();
                    String getter_name("<-");
                    getter_name += field_data.f_str;
                    field_data.f_str = getter_name;

                    // the call needs a list of parameters (1 parameter)
                    NodePtr params;
                    params.CreateNode(NODE_LIST);
                    /*
                    NodePtr this_expr;
                    this_expr.CreateNode(NODE_THIS);
                    params.AddChild(this_expr);
                    */
                    expr.SetChild(1, params);

                    params.AddChild(right);


                    // and finally, we transform the member in a call!
                    Data& expr_data = expr.GetData();
                    expr_data.f_type = NODE_CALL;
                }
            }
        }
    }
    else {
        // Is this really acceptable?!
        // We can certainly make it work in Macromedia Flash...
        // If the expression is resolved as a string which is
        // also a valid variable name.
        Expression(left);
    }

    NodePtr& right = expr.GetChild(1);
    Expression(right);

    if(var.HasNode()) {
        Data& var_data = var.GetData();
        var_data.f_int.Set(var_data.f_int.Get() & ~NODE_VAR_FLAG_DEFINING);
    }

    NodePtr& type = left.GetLink(NodePtr::LINK_TYPE);
    if(type.HasNode()) {
        expr.SetLink(NodePtr::LINK_TYPE, type);
        return;
    }

    if(!is_var) {
        // if left not typed, use right type!
        // (the assignment is this type of special case...)
        expr.SetLink(NodePtr::LINK_TYPE, right.GetLink(NodePtr::LINK_TYPE));
    }
}


void Compiler::UnaryOperator(NodePtr& expr)
{
    const char *op;

    op = expr.OperatorToString();
    AS_ASSERT(op != 0);

//fprintf(stderr, "Una - Operator [%s]\n", op);

    NodePtr left = expr.GetChild(0);
    NodePtr& type = left.GetLink(NodePtr::LINK_TYPE);
    if(!type.HasNode())
    {
//fprintf(stderr, "WARNING: operand of unary operator isn't typed.\n");
        return;
    }

    NodePtr l;
    l.CreateNode(NODE_IDENTIFIER);
    Data& lname = l.GetData();
    lname.f_str = "left";

    NodePtr params;
    params.CreateNode(NODE_LIST);
    params.AddChild(l);

    NodePtr id;
    id.CreateNode(NODE_IDENTIFIER);
    Data& name = id.GetData();
    name.f_str = op;
    id.AddChild(params);

    Offsets(id);

    int del = expr.GetChildCount();
    expr.AddChild(id);

    NodePtr resolution;
    int funcs = 0;
    bool result;
    {
        NodeLock ln(expr);
        result = FindField(type, id, funcs, resolution, &params, 0);
    }

    expr.DeleteChild(del);
    if(!result)
    {
        f_error_stream->ErrMsg(AS_ERR_INVALID_OPERATOR, expr, "cannot apply operator '%s' to this object.", op);
        return;
    }

//fprintf(stderr, "Found operator!!!\n");

    Node::pointer_t op_type(resolution->get_link(Node::LINK_TYPE));

    if(get_attribute(resolution, Node::NODE_ATTR_INTRINSIC))
    {
        Data& data = expr.GetData();
        switch(data.f_type) {
        case NODE_INCREMENT:
        case NODE_DECREMENT:
        case NODE_POST_INCREMENT:
        case NODE_POST_DECREMENT:
        {
            NodePtr& var = left.GetLink(NodePtr::LINK_INSTANCE);
            if(var.HasNode()) {
                Data& data = var.GetData();
                if((data.f_type == NODE_PARAM
                    || data.f_type == NODE_VARIABLE)
                && (data.f_int.Get() & NODE_VAR_FLAG_CONST) != 0) {
                    f_error_stream->ErrMsg(AS_ERR_CANNOT_OVERWRITE_CONST, expr, "cannot increment or decrement a constant variable or function parameters.");
                }
            }
        }
            break;

        default:
            break;

        }
        // we keep intrinsic operators as is
//fprintf(stderr, "It is intrinsic...\n");
        expr.SetLink(NodePtr::LINK_INSTANCE, resolution);
        expr.SetLink(NodePtr::LINK_TYPE, op_type);
        return;
    }
//fprintf(stderr, "Not intrinsic...\n");

    id.SetLink(NodePtr::LINK_INSTANCE, resolution);

    // if not intrinsic, we need to transform the code
    // to a CALL instead because the lower layer won't
    // otherwise understand this operator!
    id.DeleteChild(0);
    id.SetLink(NodePtr::LINK_TYPE, op_type);

    // move operand in the new expression
    expr.DeleteChild(0);

    // TODO:
    // if the unary operator is post increment or decrement
    // then we need a temporary variable to save the current
    // value of the expression, compute the expression + 1
    // and restore the temporary

    Data& expr_data = expr.GetData();
    NodePtr post_list;
    NodePtr assignment;
    bool is_post = expr_data.f_type == NODE_POST_DECREMENT
            || expr_data.f_type == NODE_POST_INCREMENT;
    if(is_post) {
        post_list.CreateNode(NODE_LIST);
        // TODO: should the list get the input type instead?
        post_list.SetLink(NodePtr::LINK_TYPE, op_type);

        NodePtr temp_var;
        temp_var.CreateNode(NODE_IDENTIFIER);
        Data& tv_data = temp_var.GetData();
        // TODO: create a temporary variable name generator?
        tv_data.f_str = "#temp_var#";
        // Save that name for next reference!
        assignment.CreateNode(NODE_ASSIGNMENT);
        assignment.AddChild(temp_var);
        assignment.AddChild(left);

        post_list.AddChild(assignment);
    }

    NodePtr call;
    call.CreateNode(NODE_CALL);
    call.SetLink(NodePtr::LINK_TYPE, op_type);
    NodePtr member;
    member.CreateNode(NODE_MEMBER);
    NodePtr function;
    ResolveInternalType(expr, "Function", function);
    member.SetLink(NodePtr::LINK_TYPE, function);
    call.AddChild(member);

    // we need a function to get the name of 'type'
    //Data& type_data = type.GetData();
    //NodePtr object;
    //object.CreateNode(NODE_IDENTIFIER);
    //Data& obj_data = object.GetData();
    //obj_data.f_str = type_data.f_str;
    if(is_post) {
        // TODO: we MUST call the object defined
        //     by the left expression and NOT what
        //     I'm doing here; that's all wrong!!!
        //     for that we either need a "clone"
        //     function or a dual (or more)
        //     parenting...
        NodePtr l;
        Data& left_data = left.GetData();
        if(left_data.f_type == NODE_IDENTIFIER) {
            l.CreateNode(NODE_IDENTIFIER);
            Data& data = l.GetData();
            data.f_str = left_data.f_str;
            // TODO: copy the links, flags, etc.
        }
        else {
            l.CreateNode(NODE_IDENTIFIER);
            Data& data = l.GetData();
            // TODO: use the same "temp var#" name
            data.f_str = "#temp_var#";
        }

        member.AddChild(l);
    }
    else {
        member.AddChild(left);
    }
    member.AddChild(id);

//fprintf(stderr, "NOTE: add a list (no params)\n");
    NodePtr list;
    list.CreateNode(NODE_LIST);
    list.SetLink(NodePtr::LINK_TYPE, op_type);
    call.AddChild(list);

    if(is_post)
    {
        post_list.AddChild(call);

        NodePtr temp_var;
        temp_var.CreateNode(NODE_IDENTIFIER);
        Data& tv_data = temp_var.GetData();
        // TODO: use the same name as used in the 1st temp_var#
        tv_data.f_str = "#temp_var#";
        post_list.AddChild(temp_var);

        expr.GetParent().SetChild(expr.GetOffset(), post_list);
        //expr = post_list;
    }
    else
    {
        expr.GetParent().SetChild(expr.GetOffset(), call);
        //expr = call;
    }

    Offsets(expr);
//expr.Display(stderr);
}




void Compiler::binary_operator(Node::pointer_t& expr)
{
    char const *op;

    op = expr->operator_to_string();
    if(!op)
    {
        throw exception_internal_error("operator_to_string() returned an empty string for a binary operator");
    }

    Node::poiner_t left(expr->get_child(0));
    Node::pointer_t ltype(left->get_link(Node::LINK_TYPE));
    if(!ltype)
    {
        return;
    }

    Node::pointer_t right(expr->get_child(1));
    Node::pointer_t rtype(right->get_link(Node::LINK_TYPE);
    if(!rtype)
    {
        return;
    }

    Node::pointer_t l(expr->create_replacement(Node::NODE_IDENTIFIER));
    l->set_string("left");
    Node::pointer_t r(expr->create_replacement(Node::NODE_IDENTIFIER));
    r->set_string("right");

    l->set_link(Node::LINK_TYPE, ltype);
    r->set_link(Node::LINK_TYPE, rtype);

    Node::pointer_t params(expr->create_replacement(Node::NODE_LIST));
    params->append_child(l);
    params->append_child(r);

    Node::pointer_t id(expr->create_replacement(Node::NODE_IDENTIFIER));
    id->set_string(op);
    id->append_child(params);

    int const del(expr.GetChildCount());
    expr->add_child(id);

    offsets(expr);

    Node::pointer_t resolution;
    int funcs = 0;
    bool result;
    {
        NodeLock ln(expr);
        result = find_field(ltype, id, funcs, resolution, &params, 0);
        if(!result)
        {
            result = find_field(rtype, id, funcs, resolution, &params, 0);
        }
    }

    expr->delete_child(del);
    if(!result)
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_OPERATOR, expr->get_position());
        msg << "cannot apply operator '" << op << "' to these objects.";
        return;
    }

    NodePtr& op_type = resolution.GetLink(NodePtr::LINK_TYPE);

    unsigned long attrs = GetAttributes(resolution);
    if((attrs & NODE_ATTR_INTRINSIC) != 0)
    {
        // we keep intrinsic operators as is
        expr.SetLink(NodePtr::LINK_INSTANCE, resolution);
        expr.SetLink(NodePtr::LINK_TYPE, op_type);
        return;
    }

    id.SetLink(NodePtr::LINK_INSTANCE, resolution);

    // if not intrinsic, we need to transform the code
    // to a CALL instead because the lower layer won't
    // otherwise understand this operator!
//fprintf(stderr, "Not intrinsic...\n");
    id.DeleteChild(0);
    id.SetLink(NodePtr::LINK_TYPE, op_type);

    // move left and right in the new expression
    expr.DeleteChild(1);
    expr.DeleteChild(0);

    NodePtr call;
    call.CreateNode(NODE_CALL);
    call.SetLink(NodePtr::LINK_TYPE, op_type);
    NodePtr member;
    member.CreateNode(NODE_MEMBER);
    NodePtr function;
    ResolveInternalType(expr, "Function", function);
    member.SetLink(NodePtr::LINK_TYPE, function);
    call.AddChild(member);

    // we need a function to get the name of 'type'
    //Data& type_data = type.GetData();
    //NodePtr object;
    //object.CreateNode(NODE_IDENTIFIER);
    //Data& obj_data = object.GetData();
    //obj_data.f_str = type_data.f_str;
    member.AddChild(left);
    member.AddChild(id);

//fprintf(stderr, "NOTE: add list (right param)\n");
    NodePtr list;
    list.CreateNode(NODE_LIST);
    list.SetLink(NodePtr::LINK_TYPE, op_type);
    list.AddChild(right);
    call.AddChild(list);

//call.Display(stderr);

    expr.ReplaceWith(call);
    Offsets(expr);
}




void Compiler::ObjectLiteral(NodePtr& expr)
{
    // define the type of the literal (i.e. Object)
    TypeExpr(expr);

    // go through the list of names and
    //    1) make sure property names are unique
    //    2) make sure property names are proper
    //    3) compiler expressions
    int max = expr.GetChildCount();
    if((max & 1) != 0) {
        // invalid?!
        return;
    }

    for(int idx = 0; idx < max; idx += 2) {
        NodePtr& name = expr.GetChild(idx);
        Data& name_data = name.GetData();
        int cnt = name.GetChildCount();
        if(name_data.f_type == NODE_TYPE) {
            // the first child is a dynamic name(space)
            Expression(name.GetChild(0));
            if(cnt == 2) {
                // TODO: (Flash doesn't support those)
                // this is a scope
                //    name.GetChild(0) :: name.GetChild(1)
                // ...
                f_error_stream->ErrMsg(AS_ERR_NOT_SUPPORTED, name, "scope not support yet. (1)");
            }
        }
        else if(cnt == 1) {
            // TODO: (Flash doesn't support those)
            // this is a scope
            //    name :: name.GetChild(0)
            // Here name is IDENTIFIER, PRIVATE or PUBLIC
            // ...
            f_error_stream->ErrMsg(AS_ERR_NOT_SUPPORTED, name, "scope not support yet. (2)");
        }

        // compile the value
        NodePtr& value = expr.GetChild(idx + 1);
        Expression(value);
    }
}



void Compiler::CheckSuperValidity(NodePtr& expr)
{
    NodePtr parent = expr.GetParent();
    Data& data = parent.GetData();
    bool needs_constructor = data.f_type == NODE_CALL;
    bool first_function = true;
    while(parent.HasNode()) {
        Data& data = parent.GetData();
        switch(data.f_type) {
        case NODE_FUNCTION:
            if(first_function) {
                // We have two super's
                // 1) super(params) in constructors
                // 2) super.field(params) in non-static functions
                // case 1 is recognized as having a direct parent
                // of type call (see at start of function!)
                // case 2 is all other cases
                // in both cases we need to be defined in a class
                unsigned long attrs = GetAttributes(parent);
                if(needs_constructor) {
                    if(!IsConstructor(parent)) {
                        f_error_stream->ErrMsg(AS_ERR_INVALID_EXPRESSION, expr, "'super()' cannot be used outside of a constructor function.");
                        return;
                    }
                }
                else {
                    if((data.f_int.Get() & NODE_FUNCTION_FLAG_OPERATOR) != 0
                    || (attrs & (NODE_ATTR_STATIC | NODE_ATTR_CONSTRUCTOR)) != 0
                    || IsConstructor(parent)) {
                        f_error_stream->ErrMsg(AS_ERR_INVALID_EXPRESSION, expr, "'super.member()' cannot be used in a static function nor a constructor.");
                        return;
                    }
                }
            }
            else {
                // Can it be used in sub-functions?
                // If we arrive here then we can err if
                // super and/or this aren't available
                // in sub-functions... TBD
            }
            break;

        case NODE_CLASS:
        case NODE_INTERFACE:
            return;

        case NODE_PROGRAM:
        case NODE_ROOT:
            parent.ClearNode();
            break;

        default:
            break;

        }
        parent = parent.GetParent();
    }

    if(needs_constructor) {
        f_error_stream->ErrMsg(AS_ERR_INVALID_EXPRESSION, expr, "'super()' cannot be used outside a class definition.");
    }
#if 0
    else {
fprintf(stderr, "WARNING: 'super.member()' should only be used in a class function.\n");
    }
#endif
}



bool Compiler::IsFunctionAbstract(NodePtr& function)
{
    int max = function.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = function.GetChild(idx);
        Data& data = child.GetData();
        if(data.f_type == NODE_DIRECTIVE_LIST) {
            return false;
        }
    }

    return true;
}


bool Compiler::FindOverloadedFunction(NodePtr& class_node, NodePtr& function)
{
    Data& func_data = function.GetData();
    int max = class_node.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = class_node.GetChild(idx);
        Data& data = child.GetData();
        switch(data.f_type) {
        case NODE_EXTENDS:
        case NODE_IMPLEMENTS:
        {
            NodePtr names = child.GetChild(0);
            Data& data = names.GetData();
            if(data.f_type != NODE_LIST) {
                names = child;
            }
            int max = names.GetChildCount();
            for(int idx = 0; idx < max; ++idx) {
                NodePtr& super = names.GetChild(idx).GetLink(NodePtr::LINK_INSTANCE);
                if(super.HasNode()) {
                    if(IsFunctionOverloaded(super, function)) {
                        return true;
                    }
                }
            }
        }
            break;

        case NODE_DIRECTIVE_LIST:
            if(FindOverloadedFunction(child, function)) {
                return true;
            }
            break;

        case NODE_FUNCTION:
            if(func_data.f_str == data.f_str) {
                // found a function with the same name
                if(compare_parameters(function, child))
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


bool Compiler::IsFunctionOverloaded(NodePtr& class_node, NodePtr& function)
{
    Data *d;
    NodePtr parent = ClassOfMember(function, d);
    AS_ASSERT(parent.HasNode());
    AS_ASSERT(d->f_type == NODE_CLASS || d->f_type == NODE_INTERFACE);
    if(parent.SameAs(class_node)) {
        return false;
    }

    return FindOverloadedFunction(class_node, function);
}


bool Compiler::HasAbstractFunctions(NodePtr& class_node, NodePtr& list, NodePtr& func)
{
    int max = list.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = list.GetChild(idx);
        Data& data = child.GetData();
        switch(data.f_type) {
        case NODE_IMPLEMENTS:
        case NODE_EXTENDS:
        {
            NodePtr names = child.GetChild(0);
            Data& data = names.GetData();
            if(data.f_type != NODE_LIST) {
                names = child;
            }
            int max = names.GetChildCount();
            for(int idx = 0; idx < max; ++idx) {
                NodePtr& super = names.GetChild(idx).GetLink(NodePtr::LINK_INSTANCE);
                if(super.HasNode()) {
                    if(HasAbstractFunctions(class_node, super, func)) {
                        return true;
                    }
                }
            }
        }
            break;

        case NODE_DIRECTIVE_LIST:
            if(HasAbstractFunctions(class_node, child, func)) {
                return true;
            }
            break;

        case NODE_FUNCTION:
            if(IsFunctionAbstract(child)) {
                // see whether it was overloaded
                if(!IsFunctionOverloaded(class_node, child)) {
                    // not overloaded, this class can't
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


bool Compiler::SpecialIdentifier(NodePtr& expr)
{
    // all special identifier are defined as "__...__"
    // that means they are at least 5 characters and they need to
    // start with '__'
    Data& data = expr.GetData();
//fprintf(stderr, "ID [%.*S]\n", data.f_str.GetLength(), data.f_str.Get());
    if(data.f_str.GetLength() < 5) {
        return false;
    }
    const long *s = data.f_str.Get();
    if(s[0] != '_' || s[1] != '_') {
        return false;
    }

    const char *what = "?";
    NodePtr parent = expr;
    String result;
    Data *parent_data = 0;
    if(data.f_str == "__FUNCTION__") {
        what = "a function";
        for(;;) {
            parent = parent.GetParent();
            if(!parent.HasNode()) {
                parent_data = 0;
                break;
            }
            parent_data = &parent.GetData();
            if(parent_data->f_type == NODE_PACKAGE
            || parent_data->f_type == NODE_PROGRAM
            || parent_data->f_type == NODE_ROOT
            || parent_data->f_type == NODE_INTERFACE
            || parent_data->f_type == NODE_CLASS) {
                parent_data = 0;
                break;
            }
            if(parent_data->f_type == NODE_FUNCTION) {
                break;
            }
        }
    }
    else if(data.f_str == "__CLASS__") {
        what = "a class";
        for(;;) {
            parent = parent.GetParent();
            if(!parent.HasNode()) {
                parent_data = 0;
                break;
            }
            parent_data = &parent.GetData();
            if(parent_data->f_type == NODE_PACKAGE
            || parent_data->f_type == NODE_PROGRAM
            || parent_data->f_type == NODE_ROOT) {
                parent_data = 0;
                break;
            }
            if(parent_data->f_type == NODE_CLASS) {
                break;
            }
        }
    }
    else if(data.f_str == "__INTERFACE__") {
        what = "an interface";
        for(;;) {
            parent = parent.GetParent();
            if(!parent.HasNode()) {
                parent_data = 0;
                break;
            }
            parent_data = &parent.GetData();
            if(parent_data->f_type == NODE_PACKAGE
            || parent_data->f_type == NODE_PROGRAM
            || parent_data->f_type == NODE_ROOT) {
                parent_data = 0;
                break;
            }
            if(parent_data->f_type == NODE_INTERFACE) {
                break;
            }
        }
    }
    else if(data.f_str == "__PACKAGE__") {
        what = "a package";
        for(;;) {
            parent = parent.GetParent();
            if(!parent.HasNode()) {
                parent_data = 0;
                break;
            }
            parent_data = &parent.GetData();
            if(parent_data->f_type == NODE_PROGRAM
            || parent_data->f_type == NODE_ROOT) {
                parent_data = 0;
                break;
            }
            if(parent_data->f_type == NODE_PACKAGE) {
                break;
            }
        }
    }
    else if(data.f_str == "__NAME__") {
        what = "any function, class, interface or package";
        for(;;) {
            parent = parent.GetParent();
            if(!parent.HasNode()) {
                break;
            }
            as::Data& data = parent.GetData();
            if(data.f_type == as::NODE_PROGRAM
            || data.f_type == as::NODE_ROOT) {
                break;
            }
            if(data.f_type == as::NODE_FUNCTION
            || data.f_type == as::NODE_CLASS
            || data.f_type == as::NODE_INTERFACE
            || data.f_type == as::NODE_PACKAGE) {
                if(result.IsEmpty()) {
                    result = data.f_str;
                }
                else {
                    // TODO: create the + operator
                    //     on String.
                    as::String p = data.f_str;
                    p += ".";
                    p += result;
                    result = p;
                }
                if(data.f_type == as::NODE_PACKAGE) {
                    // we don't really care if we
                    // are yet in another package
                    // at this time...
                    break;
                }
            }
        }
    }
    else if(data.f_str == "__TIME__") {
        //what = "time";
        char buf[256];
        struct tm *t;
        t = localtime(&f_time);
        strftime(buf, sizeof(buf) - 1, "%T", t);
        result = buf;
    }
    else if(data.f_str == "__DATE__") {
        //what = "date";
        char buf[256];
        struct tm *t;
        t = localtime(&f_time);
        strftime(buf, sizeof(buf) - 1, "%Y-%m-%d", t);
        result = buf;
    }
    else if(data.f_str == "__UNIXTIME__") {
        data.f_type = NODE_INT64;
        data.f_int.Set(f_time);
        return true;
    }
    else if(data.f_str == "__UTCTIME__") {
        //what = "utctime";
        char buf[256];
        struct tm *t;
        t = gmtime(&f_time);
        strftime(buf, sizeof(buf) - 1, "%T", t);
        result = buf;
    }
    else if(data.f_str == "__UTCDATE__") {
        //what = "utcdate";
        char buf[256];
        struct tm *t;
        t = gmtime(&f_time);
        strftime(buf, sizeof(buf) - 1, "%Y-%m-%d", t);
        result = buf;
    }
    else if(data.f_str == "__DATE822__") {
        // Sun, 06 Nov 2005 11:57:59 -0800
        char buf[256];
        struct tm *t;
        t = localtime(&f_time);
        strftime(buf, sizeof(buf) - 1, "%a, %d %b %Y %T %z", t);
        result = buf;
    }
    else {
        // not a special identifier
        return false;
    }

    // even if it fails, we convert this expression into a string
    data.f_type = NODE_STRING;
    if(!result.IsEmpty()) {
        data.f_str = result;

//fprintf(stderr, "SpecialIdentifier Result = [%.*S]\n", result.GetLength(), result.Get());

    }
    else if(parent_data == 0) {
        f_error_stream->ErrStrMsg(AS_ERR_INVALID_EXPRESSION, expr, "'%S' was used outside %s.",
                    &data.f_str, what);
        // we keep the string as is!
    }
    else {
        data.f_str = parent_data->f_str;
    }

    return true;
}




}
// namespace as2js

// vim: ts=4 sw=4 et
