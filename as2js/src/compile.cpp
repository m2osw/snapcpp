/* compile.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/compiler.h"


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

int Compiler::Compile(NodePtr& root)
{
#if defined(_DEBUG) || defined(DEBUG)
    fflush(stdout);
#endif

    // all the "use namespace ..." currently in effect
    f_scope.CreateNode(NODE_SCOPE);

    if(root.HasNode()) {
        Data& data = root.GetData();
        if(data.f_type == NODE_PROGRAM) {
            Program(root);
        }
        else if(data.f_type == NODE_ROOT) {
            NodeLock ln(root);
            int max = root.GetChildCount();
            for(int idx = 0; idx < max; ++idx) {
                NodePtr child = root.GetChild(idx);
                if(child.HasNode()) {
                    data = child.GetData();
                    if(data.f_type == NODE_PROGRAM) {
                        Program(child);
                    }
                }
            }
        }
        else {
            f_error_stream->ErrMsg(AS_ERR_INTERNAL_ERROR, root, "the Compiler::Compile() function expected a root or a program node to start with.");
        }
    }

    return f_error_stream->ErrCount();
}






void Compiler::program(Node::pointer_t& program)
{
    // This is the root. Whenever you search to resolve a reference,
    // don't go past that node! What's in the parent of a program is
    // not part of that program...
    f_program = program;

#if 0
program.Display(stderr);
#endif
    // get rid of any declaration marked false
    size_t const org_max(program->get_child_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child(program.get_child(idx));
        if(get_attribute(child, Node::NODE_ATTR_FALSE))
        {
            child->to_unknown();
        }
    }
    program->clean_tree();

    NodeLock ln(program);

    // look for all the labels in this program (for goto's)
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child(program.GetChild(idx));
        if(child)
        {
            if(child->get_type() == Node::NODE_DIRECTIVE_LIST)
            {
                find_labels(program, child);
            }
        }
    }

    // a program is composed of directives (usually just one list)
    // which we want to compile
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child(program->get_child(idx));
        if(child->get_type() == NODE_DIRECTIVE_LIST)
        {
            directive_list(child);
        }
    }
#if 0
if(f_error_stream->ErrCount() != 0)
program.Display(stderr);
#endif
}



NodePtr Compiler::DirectiveList(Node::pointer_t& directive_list)
{
    int p(f_scope->get_child_size());

    // TODO: should we go through the list a first time
    //     so we get the list of namespaces for these
    //     directives at once; so in other words you
    //     could declare the namespaces in use at the
    //     start or the end of this scope and it works
    //     the same way...

    size_t max(directive_list->get_child_size());

    // get rid of any declaration marked false
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child(directive_list->get_child(idx));
        if(child)
        {
            if(get_attribute(child, Node::NODE_ATTR_FALSE))
            {
                directive_list.DeleteChild(idx);
                --idx;
                --max;
            }
        }
    }

    bool no_access = false;
    NodePtr end_list;

    // compile each directive one by one...
    {
        NodeLock ln(directive_list);
        for(int idx = 0; idx < max; ++idx) {
            NodePtr& child = directive_list.GetChild(idx);
            if(!no_access && end_list.HasNode()) {
                // err only once on this one
                no_access = true;
                f_error_stream->ErrMsg(AS_ERR_INACCESSIBLE_STATEMENT, child, "code is not accessible after a break, continue, goto, throw or return statement.");
            }
            if(child.HasNode()) {
#if 0
fprintf(stderr, "Directive at ");
child.DisplayPtr(stderr);
fprintf(stderr, " (%d + 1 of %d)\n", idx, max);
#endif

                Data& data = child.GetData();
                switch(data.f_type) {
                case NODE_PACKAGE:
                    // there is nothing to do on those
                    // until users reference them...
                    break;

                case NODE_DIRECTIVE_LIST:
                    // Recursive!
                    end_list = DirectiveList(child);
                    // TODO: we need a real control flow
                    // information to know whether this
                    // latest list had a break, continue,
                    // goto or return statement which
                    // was (really) breaking us too.
                    break;

                case NODE_LABEL:
                    // labels don't require any
                    // compile whatever...
                    break;

                case NODE_VAR:
                    Var(child);
                    break;

                case NODE_WITH:
                    With(child);
                    break;

                case NODE_USE: // TODO: should that move in a separate loop?
                    UseNamespace(child);
                    break;

                case NODE_GOTO:
                    Goto(child);
                    end_list = child;
                    break;

                case NODE_FOR:
                    for_directive(child);
                    break;

                case NODE_SWITCH:
                    Switch(child);
                    break;

                case NODE_CASE:
                    Case(child);
                    break;

                case NODE_DEFAULT:
                    Default(child);
                    break;

                case NODE_IF:
                    If(child);
                    break;

                case NODE_WHILE:
                    While(child);
                    break;

                case NODE_DO:
                    Do(child);
                    break;

                case NODE_THROW:
                    Throw(child);
                    end_list = child;
                    break;

                case NODE_TRY:
                    Try(child);
                    break;

                case NODE_CATCH:
                    Catch(child);
                    break;

                case NODE_FINALLY:
                    Finally(child);
                    break;

                case NODE_BREAK:
                case NODE_CONTINUE:
                    BreakContinue(child);
                    end_list = child;
                    break;

                case NODE_ENUM:
                    Enum(child);
                    break;

                case NODE_FUNCTION:
                    Function(child);
                    break;

                case NODE_RETURN:
                    end_list = Return(child);
                    break;

                case NODE_CLASS:
                case NODE_INTERFACE:
                    // TODO: any non-intrinsic function or
                    //     variable member referenced in
                    //     a class requires that the
                    //     whole class be assembled.
                    //     (Unless we can just assemble
                    //     what the user accesses.)
                    Class(child);
                    break;

                case NODE_IMPORT:
                    Import(child);
                    break;

                // all the possible expression entries
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
                    expression(child);
                    break;

                default:
                    f_error_stream->ErrMsg(AS_ERR_INTERNAL_ERROR, child, "directive node '%s' not handled yet in Compiler::DirectiveList().", data.GetTypeName());
                    break;

                }

                if(end_list.HasNode()
                && idx + 1 < max)
                {
                    NodePtr& next = directive_list.GetChild(idx + 1);
                    Data& data = next.GetData();
                    if(data.f_type == NODE_CASE
                    || data.f_type == NODE_DEFAULT) {
                        end_list.ClearNode();
                    }
                }
            }
        }
    }

    Data& data = directive_list.GetData();
    if((data.f_int.Get() & NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES) != 0) {
        int max = directive_list.GetVariableCount();
        for(int idx = 0; idx < max; ++idx) {
            NodePtr& variable = directive_list.GetVariable(idx);
            Node::node_pointer_t var(variable.get_parent());
            if(var)
            {
                Data& var_data = var.GetData();
                if((var_data.f_int.Get() & NODE_VAR_FLAG_TOADD) != 0)
                {
                    // TBD: is that just the var declaration and no
                    //      assignment? because the assignment needs to
                    //      happen at the proper time!!!
                    var_data.f_int.Set(var_data.f_int.Get() & ~NODE_VAR_FLAG_TOADD);
                    directive_list.InsertChild(0, var);
                }
            }
        }
        Offsets(directive_list);
        data.f_int.Set(data.f_int.Get() & ~NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES);
    }

    // go through the f_scope list and remove all the use namespace
    // (because those are NOT like in C++, they are standalone
    // instructions... weird!)
    max = f_scope.GetChildCount();
    while(p < max) {
        max--;
        f_scope.DeleteChild(max);
    }

    return end_list;
}



void Compiler::var(Node::pointer_t& var)
{
    // when variables are used, they are initialized
    // here, we initialize them only if they have
    // side effects; this is because a variable can
    // be used as an attribute and it would often
    // end up as an error (i.e. attributes not
    // found as identifier(s) defining another
    // object)
    NodeLock ln(var);
    size_t vcnt(var.get_child_size());
    for(size_t v(0); v < vcnt; ++v)
    {
        Node::pointer_t variable(var->get_child(v));
        variable(variable, true);
    }
}


void Compiler::variable(Node::pointer_t& variable, bool const side_effects_only)
{
    size_t max(variable.get_child_size());

    // if we already have a type, we've been parsed
    Data& data = variable.GetData();
    int flags = data.f_int.Get();
    if((flags & (NODE_VAR_FLAG_DEFINED | NODE_VAR_FLAG_ATTRIBUTES)) != 0)
    {
#if 0
 {
char buf[256];
size_t sz = sizeof(buf);
data.f_str.ToUTF8(buf, sz);
fprintf(stderr, "Querying variable [%s] %08X (%d)\n", buf, flags, side_effects_only);
 }
#endif
        if(!side_effects_only) {
            if((flags & NODE_VAR_FLAG_COMPILED) == 0) {
                for(int idx = 0; idx < max; ++idx) {
                    NodePtr& child = variable.GetChild(idx);
                    if(child.HasNode()) {
                        Data& child_data = child.GetData();
                        if(child_data.f_type == NODE_SET) {
                            NodePtr& expr = child.GetChild(0);
                            Expression(expr);
                            flags |= NODE_VAR_FLAG_COMPILED;
                            break;
                        }
                    }
                }
            }
            data.f_int.Set(flags | NODE_VAR_FLAG_INUSE);
        }
        return;
    }
    data.f_int.Set(flags | NODE_VAR_FLAG_DEFINED | (!side_effects_only ? NODE_VAR_FLAG_INUSE : 0));
    bool constant = (flags & NODE_VAR_FLAG_CONST) != 0;

#if 0
 {
char buf[256];
size_t sz = sizeof(buf);
data.f_str.ToUTF8(buf, sz);
fprintf(stderr, "Parsing variable [%s]\n", buf);
 }
#endif

    // make sure to get the attributes before the node gets locked
    get_attribute(variable, Node::NODE_ATTR_DEFINED);

    NodeLock ln(variable);
    int set(0);

    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = variable.GetChild(idx);
        if(child.HasNode()) {
            Data& child_data = child.GetData();
            if(child_data.f_type == NODE_SET) {
                NodePtr& expr = child.GetChild(0);
                 Data& expr_data = expr.GetData();
                if(expr_data.f_type == NODE_PRIVATE
                || expr_data.f_type == NODE_PUBLIC) {
                    // this is a list of attributes
                    ++set;
                }
                else if((!side_effects_only
                    || expr.HasSideEffects())
                        && set == 0) {
                    Expression(expr);
                    data.f_int.Set(data.f_int.Get() | NODE_VAR_FLAG_COMPILED | NODE_VAR_FLAG_INUSE);
                }
                ++set;
            }
            else {
                // define the variable type in this case
                Expression(child);
                if(!variable.GetLink(NodePtr::LINK_TYPE).HasNode()) {
                    variable.SetLink(NodePtr::LINK_TYPE, child.GetLink(NodePtr::LINK_INSTANCE));
                }
            }
        }
    }

    if(set > 1) {
        Data& data = variable.GetData();
        data.f_type = NODE_VAR_ATTRIBUTES;
        if(!constant) {
            f_error_stream->ErrStrMsg(AS_ERR_NEED_CONST, variable, "a variable cannot be a list of attributes unless it is made constant and '%S' is not constant.", &data.f_str);
        }
    }
    else {
        // read the initializer (we're expecting an expression, but
        // if this is only one identifier or PUBLIC or PRIVATE then
        // we're in a special case...)
        AddVariable(variable);
    }
}


void Compiler::AddVariable(NodePtr& variable)
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
    NodePtr parent = variable;
    bool first = true;
    for(;;) {
        parent = parent.GetParent();
        Data& parent_data = parent.GetData();
        switch(parent_data.f_type) {
        case NODE_DIRECTIVE_LIST:
            if(first) {
                first = false;
                parent.AddVariable(variable);
            }
            break;

        case NODE_FUNCTION:
            // mark the variable as local
        {
            Data& data = variable.GetData();
            data.f_int.Set(data.f_int.Get() | NODE_VAR_FLAG_LOCAL);
            if(first) {
                parent.AddVariable(variable);
            }
        }
            return;

        case NODE_CLASS:
        case NODE_INTERFACE:
        {
            Data& data = variable.GetData();
            data.f_int.Set(data.f_int.Get() | NODE_VAR_FLAG_MEMBER);
            if(first) {
                parent.AddVariable(variable);
            }
        }
            return;

        case NODE_PROGRAM:
        case NODE_PACKAGE:
            if(first) {
                parent.AddVariable(variable);
            }
            return;

        default:
            break;

        }
    }
}



void Compiler::With(NodePtr& with)
{
    int max = with.GetChildCount();
    if(max != 2) {
        return;
    }
    NodeLock ln(with);

    // object name defined in an expression
    // (used to resolve identifiers as members in the following
    // expressions until it gets popped)
    NodePtr& object = with.GetChild(0);

    Data& data = object.GetData();
    if(data.f_type == NODE_THIS) {
        // TODO: could we avoid erring here?!
        f_error_stream->ErrMsg(AS_ERR_INVALID_EXPRESSION, with, "'with' cannot use 'this' as an object.");
    }

//fprintf(stderr, "Resolving WITH object...\n");

    Expression(object);

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

    NodePtr& sub_directives = with.GetChild(1);
    DirectiveList(sub_directives);

    // the effect of this with() ends with the end of its
    // list of directives
    //f_scope.DeleteChild(p);
}



void Compiler::Goto(NodePtr& goto_node)
{
    int        idx, count;
    NodePtr        label;

    count = 0;

    NodePtr parent = goto_node;
    Data& data = goto_node.GetData();

    do {
        ++count;
        parent = parent.GetParent();
        if(!parent.HasNode()) {
            f_error_stream->ErrMsg(AS_ERR_INTERNAL_ERROR, goto_node, "Compiler::Goto(): Out of parent before we find function, program or package parent?!");
            AS_ASSERT(0);
            return;
        }

        Data& parent_data = parent.GetData();
        switch(parent->get_type())
        {
        case Node::NODE_CLASS:
        case Node::NODE_INTERFACE:
            f_error_stream->ErrMsg(AS_ERR_IMPORPER_STATEMENT, goto_node, "cannot have a GOTO instruction in a 'class' or 'interface'.");
            return;

        case Node::NODE_FUNCTION:
        case Node::NODE_PACKAGE:
        case Node::NODE_PROGRAM:
            label = parent.FindLabel(data.f_str);
            if(!label.HasNode())
            {
                f_error_stream->ErrStrMsg(AS_ERR_LABEL_NOT_FOUND, goto_node, "label '%S' for goto instruction not found.", &data.f_str);
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
    } while(!label.HasNode());

    // Now we have to do the hardess part:
    //    find the common parent frame where both, the goto
    //    and the label can be found
    //    for this purpose we create an array with all the
    //    frames and then we search that array with each
    //    parent of the label

#ifdef _MSVC
    // alloca() not available with cl
    class AutoDelete {
    public: AutoDelete(NodePtr *ptr) { f_ptr = ptr; }
        ~AutoDelete() { delete f_ptr; }
    private: NodePtr *f_ptr;
    };
    NodePtr *parents = new NodePtr[count];
    AutoDelete ad_parent(parents);
#else
    NodePtr parents[count];
#endif
    parent = goto_node;
    for(idx = 0; idx < count; ++idx) {
        parent = parent.GetParent();
        parents[idx] = parent;
    }

    goto_node.SetLink(NodePtr::LINK_GOTO_ENTER, label);

    parent = label;
    for(;;) {
        parent = parent.GetParent();
        if(!parent.HasNode()) {
            f_error_stream->ErrMsg(AS_ERR_INTERNAL_ERROR, goto_node, "Compiler::Goto(): Out of parent before we find the common node?!");
            AS_ASSERT(0);
            return;
        }
        for(idx = 0; idx < count; ++idx) {
            if(parents[idx].SameAs(parent)) {
                goto_node.SetLink(NodePtr::LINK_GOTO_EXIT, parent);
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
        case NODE_EMPTY:
            // do nothing
            break;

        case NODE_DIRECTIVE_LIST:
            directive_list(child);
            break;

        case NODE_VAR:
            var(child);
            break;

        default:    // expression
            expression(child);
            break;

        }
    }
}


void Compiler::Switch(NodePtr& switch_node)
{
    int max = switch_node.GetChildCount();
    if(max != 2)
    {
        return;
    }

    NodeLock ln_sn(switch_node);
    Expression(switch_node.GetChild(0));

    // make sure that the list of directive starts
    // with a label [this is a requirements which
    // really makes sense but the parser doesn't
    // enforce it]
    NodePtr& directive_list = switch_node.GetChild(1);
    max = directive_list.GetChildCount();
    if(max > 0)
    {
        NodePtr& child = directive_list.GetChild(0);
        Data& data = child.GetData();
        if(data.f_type != NODE_CASE
        && data.f_type != NODE_DEFAULT)
        {
            f_error_stream->ErrMsg(AS_ERR_INACCESSIBLE_STATEMENT, child, "the list of instructions of a 'switch()' must start with a 'case' or 'default' label.");
        }
    }

    DirectiveList(directive_list);

    // in case we are being compiled a second time
    // (it happens for testing the missing return validity)
    switch_node.set_flag(NODE_SWITCH_FLAG_DEFAULT, false);  // <<-- new scheme!
    //Data& data = switch_node.GetData();
    //data.f_int.Set(data.f_int.Get() & ~NODE_SWITCH_FLAG_DEFAULT);

    // TODO: If EQUAL or STRICTLY EQUAL we may
    //       want to check for duplicates.
    //       (But cases can be dynamic so it
    //       doesn't really make sense, does it?!)
}


void Compiler::Case(NodePtr& case_node)
{
    // make sure it was used inside a switch statement
    // (the parser doesn't enforce it)
    NodePtr parent = case_node.GetParent();
    if(!parent.HasNode())
    {
        // ?!?
        return;
    }
    parent = parent.GetParent();
    if(!parent.HasNode())
    {
        // ?!?
        return;
    }
    Data& data = parent.GetData();
    if(data.f_type != NODE_SWITCH)
    {
        f_error_stream->ErrMsg(AS_ERR_IMPORPER_STATEMENT, case_node, "a 'case' statement can only be used within a 'switch()' block.");
        return;
    }

    int const max = case_node.GetChildCount();
    if(max > 0)
    {
        Expression(case_node.GetChild(0));
        if(max > 1)
        {
            switch(data.f_int.Get() & NODE_MASK)
            {
            case NODE_UNKNOWN:
            case NODE_IN:
                break;

            default:
                f_error_stream->ErrMsg(AS_ERR_INVALID_EXPRESSION, case_node, "a range on a 'case' statement can only be used with the 'in' and 'default' operators.");
                break;

            }
            Expression(case_node.GetChild(1));
        }
    }
}


void Compiler::Default(NodePtr& default_node)
{
    // make sure it was used inside a switch statement
    // (the parser doesn't enforce it)
    NodePtr parent = default_node.GetParent();
    if(!parent.HasNode()) {
        // ?!?
        return;
    }
    parent = parent.GetParent();
    if(!parent.HasNode()) {
        // ?!?
        return;
    }
    Data& data = parent.GetData();
    if(data.f_type != NODE_SWITCH) {
        f_error_stream->ErrMsg(AS_ERR_INACCESSIBLE_STATEMENT, default_node, "a 'default' statement can only be used within a 'switch()' block.");
        return;
    }

    int flags = data.f_int.Get();
    if((flags & NODE_SWITCH_FLAG_DEFAULT) != 0) {
        f_error_stream->ErrMsg(AS_ERR_IMPORPER_STATEMENT, default_node, "only one 'default' statement can be used within one 'switch()'.");
    }
    else {
        data.f_int.Set(flags | NODE_SWITCH_FLAG_DEFAULT);
    }
}


void Compiler::If(NodePtr& if_node)
{
    int max = if_node.GetChildCount();
    if(max < 2) {
        return;
    }
    NodeLock ln(if_node);

    // TODO: check whether the first expression
    //     is a valid boolean?
    Expression(if_node.GetChild(0));
    DirectiveList(if_node.GetChild(1));
    if(max == 3) {
        DirectiveList(if_node.GetChild(2));
    }
}



void Compiler::While(NodePtr& while_node)
{
    int max = while_node.GetChildCount();
    if(max != 2) {
        return;
    }
    NodeLock ln(while_node);

    // If the first expression is a constant boolean,
    // the optimizer will replace the while()
    // loop in a loop forever; or remove it entirely.
    Expression(while_node.GetChild(0));
    DirectiveList(while_node.GetChild(1));
}


void Compiler::Do(NodePtr& do_node)
{
    int max = do_node.GetChildCount();
    if(max != 2) {
        return;
    }
    NodeLock ln(do_node);

    // If the second expression is a constant boolean,
    // the optimizer will replace the do/while()
    // loop in a loop forever; or execute the first
    // list of directives once.
    DirectiveList(do_node.GetChild(0));
    Expression(do_node.GetChild(1));
}


void Compiler::BreakContinue(NodePtr& break_node)
{
    int        offset;
    NodePtr        to_break, parent, p, previous;

    Data& data = break_node.GetData();
    bool no_name = data.f_str.IsEmpty();
    bool accept_switch = !no_name || data.f_type == NODE_BREAK;
    bool found_switch = false;
    parent = break_node;
    for(;;) {
        parent = parent.GetParent();
        Data& parent_data = parent.GetData();
        if(parent_data.f_type == NODE_SWITCH) {
            found_switch = true;
        }
        if((parent_data.f_type == NODE_SWITCH && accept_switch)
        || parent_data.f_type == NODE_FOR
        || parent_data.f_type == NODE_DO
        || parent_data.f_type == NODE_WHILE) {
            if(no_name) {
                // just break the current switch, for,
                // while, do when there isn't a name.
                break;
            }
            // check whether this statement has a label
            // and whether it matches the requested name
            offset = parent.GetOffset();
            if(offset > 0) {
                p = parent.GetParent();
                previous = p.GetChild(offset - 1);
                Data& prev_data = previous.GetData();
                if(prev_data.f_type == NODE_LABEL
                && prev_data.f_str == data.f_str) {
                    break;
                }
            }
        }
        if(parent_data.f_type == NODE_FUNCTION
        || parent_data.f_type == NODE_PROGRAM
        || parent_data.f_type == NODE_CLASS    // ?!
        || parent_data.f_type == NODE_INTERFACE    // ?!
        || parent_data.f_type == NODE_PACKAGE) {
            // not found?! a break/continue outside a loop or
            // switch?! or the name wasn't found
            if(no_name) {
                if(found_switch) {
                    f_error_stream->ErrMsg(AS_ERR_IMPORPER_STATEMENT, break_node, "you cannot use a continue statement outside a loop (and you need a label to make it work with a switch statement).");
                }
                else {
                    f_error_stream->ErrMsg(AS_ERR_IMPORPER_STATEMENT, break_node, "you cannot use a break or continue instruction outside a loop or switch statement.");
                }
            }
            else {
                f_error_stream->ErrStrMsg(AS_ERR_LABEL_NOT_FOUND, break_node, "could not find a loop or switch statement labelled '%s' for this break or continue.", &data.f_str);
            }
            return;
        }
    }

    // We just specify which node needs to be reached
    // on this break/continue.
    //
    // We don't replace these with a simple goto instruction
    // because that way the person using the tree later can
    // program the break and/or continue the way they feel
    // (using a variable, a special set of instructions,
    // etc. so as to be able to unwind all the necessary
    // data in a way specific to the break/continue).
    break_node.SetLink(NodePtr::LINK_GOTO_EXIT, parent);
}



void Compiler::Throw(NodePtr& throw_node)
{
    if(throw_node.GetChildCount() != 1) {
        return;
    }

    Expression(throw_node.GetChild(0));
}


void Compiler::Try(NodePtr& try_node)
{
    if(try_node.GetChildCount() != 1) {
        return;
    }

    // we want to make sure that we are followed
    // by a catch or a finally
    NodePtr& parent = try_node.GetParent();
    bool correct =  false;
    int max = parent.GetChildCount();
    int offset = try_node.GetOffset() + 1;
    if(offset < max) {
        NodePtr& next = parent.GetChild(offset);
        Data& data = next.GetData();
        if(data.f_type == NODE_CATCH
        || data.f_type == NODE_FINALLY) {
            correct = true;
        }
    }
    if(!correct) {
        f_error_stream->ErrMsg(AS_ERR_INVALID_TRY, try_node, "a 'try' statement needs to be followed by at least one catch or a finally.");
    }

    DirectiveList(try_node.GetChild(0));
}


void Compiler::Catch(NodePtr& catch_node)
{
    if(catch_node.GetChildCount() != 2) {
        return;
    }

    // we want to make sure that we are preceded
    // by a try
    NodePtr& parent = catch_node.GetParent();
    bool correct =  false;
    int offset = catch_node.GetOffset() - 1;
    if(offset >= 0) {
        NodePtr& prev = parent.GetChild(offset);
        Data& data = prev.GetData();
        if(data.f_type == NODE_TRY) {
            correct = true;
        }
        else if(data.f_type == NODE_CATCH) {
            correct = true;
            // It is correct syntactically, but we must
            // also have all typed catch()'es first!
            if((data.f_int.Get() & NODE_CATCH_FLAG_TYPED) == 0) {
                f_error_stream->ErrMsg(AS_ERR_INVALID_TYPE, catch_node, "only the last 'catch' statement can have a parameter without a valid type.");
            }
        }
    }
    if(!correct) {
        f_error_stream->ErrMsg(AS_ERR_IMPORPER_STATEMENT, catch_node, "a 'catch' statement needs to be preceded by a 'try' statement.");
    }

    NodePtr& parameters = catch_node.GetChild(0);
    Parameters(parameters);
    if(parameters.GetChildCount() > 0) {
        NodePtr& param = parameters.GetChild(0);
        Data& data = param.GetData();
        data.f_int.Set(data.f_int.Get() | NODE_PARAMETERS_FLAG_CATCH);
    }

    DirectiveList(catch_node.GetChild(1));
}


void Compiler::Finally(NodePtr& finally_node)
{
    if(finally_node.GetChildCount() != 1) {
        return;
    }

    // we want to make sure that we are preceded
    // by a try
    NodePtr& parent = finally_node.GetParent();
    bool correct =  false;
    int offset = finally_node.GetOffset() - 1;
    if(offset >= 0) {
        NodePtr& prev = parent.GetChild(offset);
        Data& data = prev.GetData();
        if(data.f_type == NODE_TRY
        || data.f_type == NODE_CATCH) {
            correct = true;
        }
    }
    if(!correct) {
        f_error_stream->ErrMsg(AS_ERR_IMPORPER_STATEMENT, finally_node, "a 'finally' statement needs to be preceded by a 'try' or 'catch' statement.");
    }

    // At this time we do nothing about the
    // parameter; this is just viewed as a
    // variable at this time

    DirectiveList(finally_node.GetChild(0));
}



void Compiler::enum_directive(Node::pointer_t& enum_node)
{
    NodeLock ln(enum_node);
    size_t const max(enum_node->get_children_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t entry(enum_node->get_child(idx));
        if(entry->get_children_size() != 1)
        {
            // not valid, skip
            continue;
        }
        // compile the expression
        Node::pointer_t set = entry->get_child(0);
        if(set->get_children_size() != 1)
        {
            // not valid, skip
            continue;
        }
        expression(set->get_child(0));
    }
}


/** \brief Check whether that function was not marked as final before.
 *
 * \return true if the function is marked as final in a super definition.
 */
bool Compiler::find_final_functions(Node::pointer_t& function, Node::pointer_t& super)
{
    size_t const max(super.get_child_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child(super.get_child(idx));
        switch(child->get_type())
        {
        case NODE_EXTENDS:
        {
            Node::pointer_t next_super(child.GetLink(NodePtr::LINK_INSTANCE));
            if(next_super)
            {
                if(find_final_functions(function, next_super))
                {
                    return true;
                }
            }
        }
            break;

        case NODE_DIRECTIVE_LIST:
            if(find_final_functions(function, child))
            {
                return true;
            }
            break;

        case NODE_FUNCTION:
            if(function->get_string() == child->get_string())
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
bool Compiler::check_final_functions(Node::pointer_t& function, Node::pointer_t& class_node)
{
    size_t max(class_node->get_child_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child(class_node->get_child(idx));

        // NOTE: there can be only one 'extends'
        //
        // TODO: we most certainly can support more than one extend in
        //       JavaScript, although it is not 100% clean, but we can
        //       make it work so we'll have to enhance this test
        if(child->get_type() == Node::NODE_EXTENDS)
        {
            // this points to another class which may defined
            // the same function as final
            Node::pointer_t name(child->get_child(0));
            Node::pointer_t super(name->get_link(Node::LINK_INSTANCE));
            if(super)
            {
                return find_final_functions(function, super);
            }
            break;
        }
    }

    return false;
}


bool Compiler::compare_parameters(Node::pointer_t& lfunction, Node::pointer_t& rfunction)
{
//Data& d = lfunction.GetData();
//fprintf(stderr, "Comparing params of %.*S\n", d.f_str.GetLength(), d.f_str.Get());

    // search for the list of parameters in each function
    Node::pointer_t lparams(lfunction->get_first_child(Node::NODE_PARAMETERS));
    Node::pointer_t rparams(rfunction->get_first_child(Node::NODE_PARAMETERS));

    // get the number of parameters in each list
    size_t const lmax(lparams ? lparams->get_child_size() : 0);
    size_t const rmax(rparams ? rparams->get_child_size() : 0);

    // if we do not have the same number of parameters, already, we know it
    // is not the same, even if one has just a rest in addition
    if(lmax != rmax)
    {
        return false;
    }

    // same number, compare the types
    bool result = true;
    for(size_t idx(0); idx < lmax; ++idx)
    {
        // Get the PARAM
        NodePtr& lp = lparams.GetChild(idx);
        NodePtr& rp = rparams.GetChild(idx);
        // Get the type of each PARAM
        NodePtr& l = lp.GetChild(0);
        NodePtr& r = rp.GetChild(0);
        // We can directly compare strings and identifiers
        Data& ldata = l.GetData();
        Data& rdata = r.GetData();
        if((ldata.f_type != NODE_IDENTIFIER
                && ldata.f_type != NODE_STRING)
        || (rdata.f_type != NODE_IDENTIFIER
                && rdata.f_type != NODE_STRING)) {
            // if we can't compare at compile time,
            // we consider the types as equal...
            continue;
        }
        if(ldata.f_str != rdata.f_str) {
            return false;
        }
    }

    return result;
}



bool Compiler::CheckUniqueFunctions(NodePtr& function, NodePtr& class_node, bool all_levels)
{
    Data& data = function.GetData();
    int max = class_node.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = class_node.GetChild(idx);
        Data& child_data = child.GetData();
        switch(child_data.f_type) {
        case NODE_DIRECTIVE_LIST:
            if(all_levels) {
                if(CheckUniqueFunctions(function, child, true)) {
                    return true;
                }
            }
            break;

        case NODE_FUNCTION:
            // TODO: stop recursion properly
            //
            // this condition isn't enough to stop this
            // recursive process; but I think it's good
            // enough for most cases; the only problem is
            // anyway that we will eventually get the same
            // error multiple times...
            if(child == function)
            {
                return false;
            }

            if(function->get_string() == child->get_string())
            {
                if(compare_parameters(function, child))
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DUPLICATES, f_lexer->get_input()->get_position());
                    msg << "you cannot define two functions with the same name (" << function->get_string()
                                << ") and prototype in the same scope, class or interface.";
                    return true;
                }
            }
            break;

        case NODE_VAR:
        {
            int cnt = child.GetChildCount();
            for(int j = 0; j < cnt; ++j) {
                NodePtr& variable = child.GetChild(j);
                Data& var_data = variable.GetData();
                if(data.f_str == var_data.f_str) {
                    f_error_stream->ErrStrMsg(AS_ERR_DUPLICATES, function, "you cannot define a function and a variable (found at line #%ld) with the same name (%S) in the same scope, class or interface.", variable.GetLine(), &data.f_str);
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



void Compiler::function(Node::pointer_t& function)
{
    int        idx, flags;

    if(function->get_flag(Node::NODE_ATTR_UNUSED)
    || function->get_flag(Node::NODE_ATTR_FALSE))
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
    if(function->get_flag(Node::NODE_ATTR_ABSTRACT)
    || function->get_flag(Node::NODE_ATTR_STATIC)
    || function->get_flag(Node::NODE_ATTR_PROTECTED)
    || function->get_flag(Node::NODE_ATTR_VIRTUAL)
    || function->get_flag(Node::NODE_ATTR_CONSTRUCTOR)
    || function->get_flag(Node::NODE_ATTR_FINAL))
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
    if(function->get_flag(Node::NODE_ATTR_PRIVATE))
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
            if(function->get_flag(Node::NODE_ATTR_ABSTRACT))
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_IMPORPER_STATEMENT, f_lexer->get_input()->get_position());
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
    && (function->get_flag(Node::NODE_ATTR_ABSTRACT) || function->get_flag(Node::NODE_ATTR_INTRINSIC))
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
                if(type.HasNode()) {
                    NodePtr& existing_type = param.GetLink(NodePtr::LINK_TYPE);
                    if(!existing_type.HasNode()) {
                        param.SetLink(NodePtr::LINK_TYPE, type);
                    }
#if defined(_DEBUG) || defined(DEBUG)
                    else {
                        if(!existing_type.SameAs(type)) {
                            fprintf(stderr, "Existing type is:\n");
                            existing_type.Display(stderr);
                            fprintf(stderr, "New type would be:\n");
                            type.Display(stderr);
                            AS_ASSERT(existing_type.SameAs(type));
                        }
                    }
#endif
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
        f_error_stream->ErrMsg(AS_ERR_IMPORPER_STATEMENT, return_node, "'return' can only be used inside a function.");
    }
    else {
        if((flags & NODE_FUNCTION_FLAG_NEVER) != 0) {
            f_error_stream->ErrStrMsg(AS_ERR_IMPORPER_STATEMENT, return_node, "'return' was used inside '%S', a function Never returning.", &data->f_str);
        }

        int max = return_node.GetChildCount();
        if(max == 1) {
            if((flags & NODE_FUNCTION_FLAG_VOID) != 0
            || IsConstructor(parent)) {
                f_error_stream->ErrStrMsg(AS_ERR_IMPORPER_STATEMENT, return_node, "'return' was used with an expression inside '%S', a function returning Void.", &data->f_str);
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
                f_error_stream->ErrStrMsg(AS_ERR_IMPORPER_STATEMENT, return_node, "'return' was used without an expression inside '%S', a function which expected a value to be returned.", &data->f_str);
            }
        }
    }

    return parent;
}





void Compiler::DeclareClass(NodePtr& class_node)
{
    int max = class_node.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        //NodeLock ln(class_node);
        NodePtr& child = class_node.GetChild(idx);
        Data& data = child.GetData();
        switch(data.f_type) {
        case NODE_DIRECTIVE_LIST:
            DeclareClass(child);
            break;

        case NODE_CLASS:
        case NODE_INTERFACE:
            Class(child);
            break;

        case NODE_ENUM:
            Enum(child);
            break;

        case NODE_FUNCTION:
            Function(child);
            break;

        case NODE_VAR:
            Var(child);
            break;

        default:
            f_error_stream->ErrMsg(AS_ERR_INVALID_NODE, child, "the '%s' token cannot be a class member.", data.GetTypeName());
            break;

        }
    }
}




void Compiler::ExtendClass(NodePtr& class_node, NodePtr& extend_name)
{
    Expression(extend_name);

    NodePtr& super = extend_name.GetLink(NodePtr::LINK_INSTANCE);
    if(super.HasNode()) {
        unsigned long attrs = GetAttributes(super);
        if((attrs & NODE_ATTR_FINAL) != 0) {
            Data& super_data = super.GetData();
            Data& class_data = class_node.GetData();
            f_error_stream->ErrStrMsg(AS_ERR_FINAL, class_node, "the class '%S' is marked final and it cannot be extended by '%S'.", &super_data.f_str, &class_data.f_str);
        }
    }
}


void Compiler::Class(NodePtr& class_node)
{
    int max = class_node.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        //NodeLock ln(class_node);
        NodePtr& child = class_node.GetChild(idx);
        Data& data = child.GetData();
        switch(data.f_type) {
        case NODE_DIRECTIVE_LIST:
            DeclareClass(child);
            break;

        case NODE_EXTENDS:
        case NODE_IMPLEMENTS:
            ExtendClass(class_node, child.GetChild(0));
            break;

        default:
            f_error_stream->ErrMsg(AS_ERR_INTERNAL_ERROR, class_node, "invalid token '%s' in a class definition.", data.GetTypeName());
            break;

        }
    }
}




void Compiler::Import(NodePtr& import)
{
    // If we have the IMPLEMENTS flag set, then we must make sure
    // that the corresponding package is compiled.
    Data& data = import.GetData();
    if((data.f_int.Get() & NODE_IMPORT_FLAG_IMPLEMENTS) == 0) {
        return;
    }

    // find the package
    NodePtr package;

    // search in this program
    package = FindPackage(f_program, data.f_str);
    if(!package.HasNode()) {
        NodePtr program;
        String any_name = "*";
        if(FindExternalPackage(import, any_name, program)) {
            package = FindPackage(program, data.f_str);
        }
        if(!package.HasNode()) {
            f_error_stream->ErrStrMsg(AS_ERR_NOT_FOUND, import, "cannot find package '%S'.", &data.f_str);
            return;
        }
    }

    Data& package_data = package.GetData();

    // make sure it is compiled (once)
    long flags = package_data.f_int.Get();
    package_data.f_int.Set(flags | NODE_PACKAGE_FLAG_REFERENCED);
    if((flags & NODE_PACKAGE_FLAG_REFERENCED) == 0) {
        DirectiveList(package);
    }
}



void Compiler::UseNamespace(NodePtr& use_namespace)
{
    int max = use_namespace.GetChildCount();
    if(max != 1) {
        return;
    }
    NodeLock ln(use_namespace);

    // type/scope name defined in an expression
    // (needs to be resolved in an identifiers, members composed of
    // identifiers or a string representing a valid type name)
    NodePtr& qualifier = use_namespace.GetChild(0);
    Expression(qualifier);

    // we create two nodes; one so we know we have a NAMESPACE instruction
    // and a child of that node which is the type itself; these are
    // deleted once we return from the DirectiveList() function and not
    // this function
    NodePtr q;
    q.CreateNode();
    q.SetData(qualifier.GetData());
    NodePtr n;
    n.CreateNode(NODE_NAMESPACE);
    n.AddChild(q);
    f_scope.AddChild(n);
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

    NodePtr object;
    if(!ResolveName(type, type, object, 0, 0)) {
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

        if(!ResolveName(name, name, object, params, search_flags)) {
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
            if(!ResolveName(t2, id, resolution, 0, 0)) {
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

    if(!tp1.HasNode()) {
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
                Expression(type);
                NodePtr resolution;
                if(ResolveName(type, type, resolution, 0, 0)) {
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




NodePtr Compiler::FindPackage(NodePtr& list, const String& name)
{
    NodeLock ln(list);
    int max = list.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = list.GetChild(idx);
        Data& data = child.GetData();
        if(data.f_type == NODE_DIRECTIVE_LIST) {
            NodePtr package = FindPackage(child, name);
            if(package.HasNode()) {
                return package;
            }
        }
        else if(data.f_type == NODE_PACKAGE) {
            if(data.f_str == name) {
                return child;
            }
        }
    }

    NodePtr not_found;
    return not_found;
}


bool Compiler::FindExternalPackage(NodePtr& import, const String& name, NodePtr& program)
{
    // search a package which has an element named 'name'
    // and has a name which match the identifier specified in 'import'
    Data& data = import.GetData();
    const char *package_info = FindElement(data.f_str, name, 0, 0);
    if(package_info == 0) {
        // not found!
        return false;
    }

    String filename = GetPackageFilename(package_info);

    // found it, let's get a node for it
    FindModule(filename, program);

    // at this time this won't happen because if the FindModule()
    // function fails, it exit(1)...
    if(!program.HasNode()) {
        return false;
    }

    // TODO: we should test whether we already ran Offsets()
    Offsets(program);

    return true;
}



bool Compiler::CheckImport(NodePtr& import, NodePtr& resolution, const String& name, NodePtr *params, int search_flags)
{
//fprintf(stderr, "CheckImport(... [%.*S] ..., %d)\n", name.GetLength(), name.Get(), search_flags);
    // search for a package within this program
    // (I'm not too sure, but according to the spec. you can very well
    // have a package within any script file)
    if(FindPackageItem(f_program, import, resolution, name, params, search_flags)) {
        return true;
    }

//fprintf(stderr, "Search for an external package!\n");
    NodePtr program;
    if(!FindExternalPackage(import, name, program)) {
        return false;
    }

    return FindPackageItem(program, import, resolution, name, params, search_flags | SEARCH_FLAG_PACKAGE_MUST_EXIST);
}


bool Compiler::FindPackageItem(NodePtr& program, NodePtr& import, NodePtr& resolution, const String& name, NodePtr *params, int search_flags)
{
    Data& data = import.GetData();

    NodePtr package;
    package = FindPackage(program, data.f_str);

    if(!package.HasNode()) {
        if((search_flags & SEARCH_FLAG_PACKAGE_MUST_EXIST) != 0) {
            // this is a bad error! we should always find the
            // packages in this case (i.e. when looking using the
            // database.)
            f_error_stream->ErrStrMsg(AS_ERR_INTERNAL_ERROR, import, "cannot find package '%S' in any of the previously registered packages.", &name);
            AS_ASSERT(0);
        }
        return false;
    }

    if(package.GetChildCount() == 0) {
        return false;
    }

    // setup labels (only the first time around)
    Data& package_data = package.GetData();
    if((package_data.f_int.Get() & NODE_PACKAGE_FLAG_FOUND_LABELS) == 0) {
        package_data.f_int.Set(package_data.f_int.Get() | NODE_PACKAGE_FLAG_FOUND_LABELS);
        NodePtr& child = package.GetChild(0);
        FindLabels(package, child);
    }

    // search the name of the class/function/variable we're
    // searching for in this package:

    // TODO: Hmmm... could we have the actual node instead?
    NodePtr id;
    id.CreateNode(NODE_IDENTIFIER);
    Data& id_name = id.GetData();
    id_name.f_str = name;

//fprintf(stderr, "Found package [%.*S], search field [%.*S]\n", data.f_str.GetLength(), data.f_str.Get(), name.GetLength(), name.Get());
    int funcs = 0;
    if(!FindField(package, id, funcs, resolution, params, search_flags)) {
        return false;
    }

    // TODO: Can we have an empty resolution here?!
    if(resolution.HasNode()) {
        unsigned long attrs = resolution.GetAttrs();
        if((attrs & NODE_ATTR_PRIVATE) != 0) {
            // it's private, we can't use this item
            // from outside whether it is in the
            // package or a sub-class
            return false;
        }

        if((attrs & NODE_ATTR_INTERNAL) != 0) {
            // it's internal we can only use it from
            // another package
            NodePtr parent = import;
            for(;;) {
                parent = parent.GetParent();
                if(!parent.HasNode()) {
                    return false;
                }
                Data& data = parent.GetData();
                if(data.f_type == NODE_PACKAGE) {
                    break;
                }
                if(data.f_type == NODE_ROOT
                || data.f_type == NODE_PROGRAM) {
                    return false;
                }
            }
        }
    }

    // make sure it is compiled (once)
    long flags = package_data.f_int.Get();
    package_data.f_int.Set(flags | NODE_PACKAGE_FLAG_REFERENCED);
    if((flags & NODE_PACKAGE_FLAG_REFERENCED) == 0) {
        DirectiveList(package);
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



bool Compiler::ResolveName(NodePtr list, NodePtr& id, NodePtr& resolution, NodePtr *params, int search_flags)
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
            fprintf(stderr, "INTERNAL ERROR: unhandled type in Compiler::ResolveName()\n");
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
        if(ResolveName(id, id, resolution, &params, SEARCH_FLAG_GETTER)) {
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
        r = ResolveName(id, id, resolution, 0, 0);
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



void Compiler::SetAttr(NodePtr& node, unsigned long& list_attrs, unsigned long set, unsigned long exclusive, const char *names)
{
    if((list_attrs & exclusive) != 0) {
        f_error_stream->ErrMsg(AS_ERR_INVALID_ATTRIBUTES, node, "the attributes %s are mutually exclusive.", names);
        return;
    }

    /* We would need the proper name...
     * Also, if we have variables, it isn't unlikely normal that
     * the same attribute would be defined multiple times.
    if((list_attrs & set) != 0) {
fprintf(stderr, "WARNING: %s is defined multiple times.\n", names);
        return;
    }
    */

    list_attrs |= set;
}




void Compiler::VariableToAttrs(NodePtr& node, NodePtr& var, unsigned long& attrs)
{
    Data& var_data = var.GetData();
    if(var_data.f_type != NODE_SET) {
        f_error_stream->ErrMsg(AS_ERR_INVALID_VARIABLE, node, "an attribute variable has to be given a value.");
        return;
    }

    NodePtr& a = var.GetChild(0);
    Data& data = a.GetData();
    switch(data.f_type) {
    case NODE_FALSE:
    case NODE_IDENTIFIER:
    case NODE_PRIVATE:
    case NODE_PUBLIC:
    case NODE_TRUE:
        NodeToAttrs(node, a, attrs);
        return;

    default:
        // expect a full boolean expression in this case
        break;

    }

    // compute the expression
    Expression(a);
    f_optimizer.Optimize(a);

    switch(data.f_type) {
    case NODE_TRUE:
    case NODE_FALSE:
        NodeToAttrs(node, a, attrs);
        return;

    default:
        break;

    }

    f_error_stream->ErrMsg(AS_ERR_INVALID_EXPRESSION, node, "an attribute which is an expression needs to result in a boolean value (true or false).");
}


void Compiler::IdentifierToAttrs(NodePtr& node, NodePtr& a, unsigned long& attrs)
{
    Data& data = a.GetData();

    // an identifier can't be an empty string
    switch(data.f_str.Get()[0]) {
    case 'a':
        if(data.f_str == "abstract") {
            SetAttr(node, attrs,
                NODE_ATTR_ABSTRACT,
                NODE_ATTR_CONSTRUCTOR | NODE_ATTR_STATIC | NODE_ATTR_VIRTUAL,
                "ABSTRACT, CONSTRUCTOR, STATIC and VIRTUAL");
            return;
        }
        if(data.f_str == "array") {
            SetAttr(node, attrs,
                NODE_ATTR_ARRAY,
                0,
                "ARRAY");
            return;
        }
        if(data.f_str == "autobreak") {
            SetAttr(node, attrs,
                NODE_ATTR_AUTOBREAK,
                NODE_ATTR_FOREACH | NODE_ATTR_NOBREAK,
                "AUTOBREAK, FOREACH and NOBREAK");
            return;
        }
        break;

    case 'c':
        if(data.f_str == "constructor") {
            SetAttr(node, attrs,
                NODE_ATTR_CONSTRUCTOR,
                NODE_ATTR_ABSTRACT | NODE_ATTR_STATIC | NODE_ATTR_VIRTUAL,
                "ABSTRACT, CONSTRUCTOR, STATIC and VIRTUAL");
            return;
        }
        break;

    case 'd':
        if(data.f_str == "dynamic") {
            SetAttr(node, attrs,
                NODE_ATTR_DYNAMIC,
                0,
                "DYNAMIC");
            return;
        }
        if(data.f_str == "deprecated") {
            SetAttr(node, attrs,
                NODE_ATTR_DEPRECATED,
                0,
                "DEPRECATED");
            return;
        }
        break;

    case 'e':
        if(data.f_str == "enumerable") {
            SetAttr(node, attrs,
                NODE_ATTR_ENUMERABLE,
                0,
                "ENUMERABLE");
            return;
        }
        break;

    case 'f':
        if(data.f_str == "final") {
            SetAttr(node, attrs,
                NODE_ATTR_FINAL,
                0,
                "FINAL");
            return;
        }
        if(data.f_str == "foreach") {
            SetAttr(node, attrs,
                NODE_ATTR_FOREACH,
                NODE_ATTR_AUTOBREAK | NODE_ATTR_NOBREAK,
                "AUTOBREAK, FOREACH and NOBREAK");
            return;
        }
        break;

    case 'i':
        if(data.f_str == "internal") {
            SetAttr(node, attrs,
                NODE_ATTR_INTERNAL,
                0,
                "INTERNAL");
            return;
        }
        if(data.f_str == "intrinsic") {
            SetAttr(node, attrs,
                NODE_ATTR_INTRINSIC,
                0,
                "INTRINSIC");
            return;
        }
        break;

    case 'n':
        if(data.f_str == "nobreak") {
            SetAttr(node, attrs,
                NODE_ATTR_NOBREAK,
                NODE_ATTR_AUTOBREAK | NODE_ATTR_FOREACH,
                "AUTOBREAK, FOREACH and NOBREAK");
            return;
        }
        break;

    case 'p':
        if(data.f_str == "protected") {
            SetAttr(node, attrs,
                NODE_ATTR_PROTECTED,
                NODE_ATTR_PUBLIC | NODE_ATTR_PRIVATE,
                "PUBLIC, PRIVATE and PROTECTED");
            return;
        }
        break;

    case 's':
        if(data.f_str == "static") {
            SetAttr(node, attrs,
                NODE_ATTR_STATIC,
                NODE_ATTR_ABSTRACT | NODE_ATTR_CONSTRUCTOR | NODE_ATTR_VIRTUAL,
                "ABSTRACT, CONSTRUCTOR, STATIC and VIRTUAL");
            return;
        }
        break;

    case 'u':
        if(data.f_str == "unused") {
            SetAttr(node, attrs,
                NODE_ATTR_UNUSED,
                0,
                "UNUSED");
            return;
        }
        break;

    case 'v':
        if(data.f_str == "virtual") {
            SetAttr(node, attrs,
                NODE_ATTR_VIRTUAL,
                NODE_ATTR_ABSTRACT | NODE_ATTR_CONSTRUCTOR | NODE_ATTR_STATIC,
                "ABSTRACT, CONSTRUCTOR, STATIC and VIRTUAL");
            return;
        }
        break;

    }

    // it could be a user defined variable
    // list of attributes
    NodePtr resolution;
    if(!ResolveName(node, a, resolution, 0, SEARCH_FLAG_NO_PARSING)) {
        f_error_stream->ErrStrMsg(AS_ERR_NOT_FOUND, node, "cannot find a variable named '%S'.", &data.f_str);
        return;
    }
    if(!resolution.HasNode()) {
        // TODO: do we expect an error here?
        return;
    }
    Data& res_data = resolution.GetData();
    if(res_data.f_type != NODE_VARIABLE
    && res_data.f_type != NODE_VAR_ATTRIBUTES) {
        f_error_stream->ErrStrMsg(AS_ERR_DYNAMIC, node, "a dynamic attribute name can only reference a variable and '%S' is not one.", &data.f_str);
        return;
    }

    if((res_data.f_int.Get() & NODE_VAR_FLAG_ATTRS) != 0) {
        f_error_stream->ErrStrMsg(AS_ERR_LOOPING_REFERENCE, node, "the dynamic attribute variable '%S' is used circularly (it loops).", &data.f_str);
        return;
    }

    // it is a variable, go through the list
    // and call ourself recursively with each
    // identifiers
    res_data.f_int.Set(res_data.f_int.Get() | NODE_VAR_FLAG_ATTRS | NODE_VAR_FLAG_ATTRIBUTES);
    NodeLock ln(resolution);
    int max = resolution.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = resolution.GetChild(idx);
        if(child.HasNode()) {
            VariableToAttrs(node, child, attrs);
        }
    }

    res_data.f_int.Set(res_data.f_int.Get() & ~NODE_VAR_FLAG_ATTRS);
}


void Compiler::NodeToAttrs(NodePtr& node, NodePtr& a, unsigned long& attrs)
{
    Data& data = a.GetData();

    switch(data.f_type) {
    case NODE_FALSE:
        SetAttr(node, attrs,
            NODE_ATTR_FALSE,
            NODE_ATTR_TRUE,
            "FALSE and TRUE");
        break;

    case NODE_IDENTIFIER:
        IdentifierToAttrs(node, a, attrs);
        break;

    case NODE_PRIVATE:
        SetAttr(node, attrs,
            NODE_ATTR_PRIVATE,
            NODE_ATTR_PUBLIC | NODE_ATTR_PROTECTED,
            "PUBLIC, PRIVATE and PROTECTED");
        break;

    case NODE_PUBLIC:
        SetAttr(node, attrs,
            NODE_ATTR_PUBLIC,
            NODE_ATTR_PRIVATE | NODE_ATTR_PROTECTED,
            "PUBLIC, PRIVATE and PROTECTED");
        break;

    case NODE_TRUE:
        SetAttr(node, attrs,
            NODE_ATTR_TRUE,
            NODE_ATTR_FALSE,
            "FALSE and TRUE");
        break;

    default:
        // TODO: this is a scope (user defined name)
        // ERROR: unknown attribute type
        // Note that will happen whenever someone references a
        // variable which is an expression which doesn't resolve
        // to a valid attribute and thus we need a valid error here
        f_error_stream->ErrMsg(AS_ERR_NOT_SUPPORTED, node, "unsupported attribute data type, dynamic expressions for attributes need to be resolved as constants.");
        break;

    }
}


bool Compiler::get_attribute(Node::pointer_t& node, Node::flag_attribute_t f)
{
    prepare_attributes(node);
    return node->get_flag(f);
}


void Compiler::prepare_attributes(Node::pointer_t& node)
{
    // done here?
    if(node->get_flag(NODE_ATTR_DEFINED))
    {
        return;
    }

    // mark ourselves as done even if errors occur
    node->set_flag(Node::NODE_ATTR_DEFINED, true);

    if(node->get_type() == Node::NODE_PROGRAM)
    {
        // programs don't get any specific attributes
        return NODE_ATTR_DEFINED;
    }

    Node::pointer_t attr(node->get_link(Node::LINK_ATTRIBUTES));
    if(attr)
    {
        NodeLock ln(attr);
        size_t const max(attr->get_children_size());
        for(size_t idx(0); idx < max; ++idx)
        {
            node_to_attrs(node, attr->get_child(idx), attrs);
        }
    }

    // check whether intrinsic is already set
    // (in which case it is probably an error)
    bool const has_direct_intrinsic(node->get_flag(Node::NODE_ATTR_INTRINSIC));

    // Note: we already returned if it is equal
    //       to program; here it is just documentation
    if(node->get_type() != Node::NODE_PACKAGE
    && node->get_type() != Node::NODE_PROGRAM)
    {
        Node::pointer_t parent(node.get_parent());
        if(parent)
        {
            // recurse against all parents as required
            prepare_flags(parent);

            // child can redefine (ignore parent if any defined)
            // [TODO: should this be an error if conflicting?]
            if(!node->get_flag(Node::NODE_ATTR_PUBLIC)
            && !node->get_flag(Node::NODE_ATTR_PRIVATE)
            && !node->get_flag(Node::NODE_ATTR_PROTECTED))
            {
                node->set_flag(Node::NODE_ATTR_PUBLIC,    parent->get_flag(Node::NODE_ATTR_PUBLIC));
                node->set_flag(Node::NODE_ATTR_PRIVATE,   parent->get_flag(Node::NODE_ATTR_PRIVATE));
                node->set_flag(Node::NODE_ATTR_PROTECTED, parent->get_flag(Node::NODE_ATTR_PROTECTED));
            }
            // child can redefine (ignore parent if defined)
            if(!node->get_flag(Node::NODE_ATTR_STATIC)
            && !node->get_flag(Node::NODE_ATTR_ABSTRACT)
            && !node->get_flag(Node::NODE_ATTR_VIRTUAL))
            {
                node->set_flag(Node::NODE_ATTR_STATIC,   parent->get_flag(Node::NODE_ATTR_STATIC));
                node->set_flag(Node::NODE_ATTR_ABSTRACT, parent->get_flag(Node::NODE_ATTR_ABSTRACT));
                node->set_flag(Node::NODE_ATTR_VIRTUAL,  parent->get_flag(Node::NODE_ATTR_VIRTUAL));
            }
            // inherit
            node->set_flag(Node::NODE_ATTR_INTRINSIC,  parent->get_flag(Node::NODE_ATTR_INTRINSIC));
            node->set_flag(Node::NODE_ATTR_ENUMERABLE, parent->get_flag(Node::NODE_ATTR_ENUMERABLE));
            // false has priority
            if(parent->get_flag(Node::NODE_ATTR_FALSE))
            {
                node->set_flag(Node::NODE_ATTR_FASLE, true);
                node->set_flag(Node::NODE_ATTR_TRUE, false);
            }

            if(parent->get_type() != Node::NODE_CLASS)
            {
                node->set_flag(Node::NODE_ATTR_DYNAMIC, parent->get_flag(Node::NODE_ATTR_DYNAMIC));
                node->set_flag(Node::NODE_ATTR_FINAL,   parent->get_flag(Node::NODE_ATTR_FINAL));
            }
        }
    }

    // a function which has a body cannot be intrinsic
    if(node->get_flag(Node::NODE_ATTR_INTRINSIC)
    && node->get_type() != Node::NODE_FUNCTION)
    {
        NodeLock ln(node);
        size_t const max(node->get_children_size());
        for(size_t idx(0); idx < max; ++idx)
        {
            Node::pointer_t list(node->get_child(idx));
            if(list->gettype() == Node::NODE_DIRECTIVE_LIST)
            {
                // it is an error if the user defined
                // it directly on the function; it is
                // fine if it comes from the parent
                if(has_direct_intrinsic)
                {
                    Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INTRINSIC, f_lexer->get_input()->get_position());
                    msg << "'intrinsic' is not permitted on a function with a body.";
                }
                node->set_flag(NODE_ATTR_INTRINSIC, false);
                break;
            }
        }
    }
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
        if(ResolveName(left, left, resolution, 0, 0)) {
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


void Compiler::CheckThisValidity(NodePtr& expr)
{
    unsigned long attrs;
    NodePtr parent = expr;
    for(;;) {
        parent = parent.GetParent();
        if(!parent.HasNode()) {
            return;
        }
        Data& data = parent.GetData();
        switch(data.f_type) {
        case NODE_FUNCTION:
            // If we are in a static function, then we
            // don't have access to 'this'. Note that
            // it doesn't matter whether we're in a
            // class or not...
            attrs = GetAttributes(parent);
            if((data.f_int.Get() & NODE_FUNCTION_FLAG_OPERATOR) != 0
            || (attrs & (NODE_ATTR_STATIC | NODE_ATTR_CONSTRUCTOR)) != 0
            || IsConstructor(parent)) {
                f_error_stream->ErrMsg(AS_ERR_STATIC, expr, "'this' cannot be used in a static function nor a constructor.");
            }
            return;

        case NODE_CLASS:
        case NODE_INTERFACE:
        case NODE_PROGRAM:
        case NODE_ROOT:
            return;

        default:
            break;

        }
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


void Compiler::CanInstantiateType(NodePtr& expr)
{
    Data& data = expr.GetData();
    if(data.f_type != NODE_IDENTIFIER) {
        // dynamic, can't test at compile time...
        return;
    }

    NodePtr& inst = expr.GetLink(NodePtr::LINK_INSTANCE);
    Data& inst_data = inst.GetData();
    if(inst_data.f_type == NODE_INTERFACE) {
        f_error_stream->ErrStrMsg(AS_ERR_INVALID_EXPRESSION, expr, "you can only instantiate an object from a class. '%S' is an interface.", &data.f_str);
        return;
    }
    if(inst_data.f_type != NODE_CLASS) {
        f_error_stream->ErrStrMsg(AS_ERR_INVALID_EXPRESSION, expr, "you can only instantiate an object from a class. '%S' does not seem to be a class.", &data.f_str);
        return;
    }

    // check all the functions and make sure none are [still] abstract
    // in this class...
    NodePtr func;
    if(HasAbstractFunctions(inst, inst, func)) {
        Data& func_data = func.GetData();
        f_error_stream->ErrStrMsg(AS_ERR_ABSTRACT, expr, "the class '%S' has an abstract function '%S' in file '%S' at line #%ld and cannot be instantiated. (If you have an overloaded version of that function it may have the wrong prototype.)", &data.f_str, &func_data.f_str, &func.GetFilename(), func.GetLine());
        return;
    }

    // we're fine...
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


bool Compiler::expression_new(Node::pointer_t new_node)
{
    //
    // handle the special case of:
    //    VAR name := NEW class()
    //

//fprintf(stderr, "BEFORE:\n");
//new_node.Display(stderr);
    NodePtr& call = new_node.GetChild(0);
    if(!call.HasNode()) {
        return false;
    }

    Data& data = call.GetData();
    if(data.f_type != NODE_CALL) {
        return false;
    }

    // get the function name
    NodePtr& id = call.GetChild(0);
    Data& name = id.GetData();
    if(name.f_type != NODE_IDENTIFIER) {
        return false;
    }

    // determine the types of the parameters to search a corresponding object or function
    NodePtr params;
    params.SetNode(call.GetChild(1));
    int count = params.GetChildCount();
//fprintf(stderr, "ResolveCall() with %d expressions\n", count);
//new_node.Display(stderr);
    for(int idx = 0; idx < count; ++idx) {
        NodePtr& p = params.GetChild(idx);
        Expression(p);
    }

    // resolve what is named
    NodePtr resolution;
    if(!ResolveName(id, id, resolution, &params, SEARCH_FLAG_GETTER)) {
        // an error is generated later if this is a call and no function can be found
        return false;
    }

    // is the name a class or interface?
    Data& res_data = resolution.GetData();
    if(res_data.f_type != NODE_CLASS
    && res_data.f_type != NODE_INTERFACE) {
        return false;
    }

    // move the nodes under CALL up one level
    NodePtr type = call.GetChild(0);
    NodePtr expr = call.GetChild(1);
    call.DeleteChild(0);
    call.DeleteChild(0);    // 1 is now 0
    new_node.DeleteChild(0);    // remove the CALL
    new_node.AddChild(type);    // replace with TYPE + parameters (LIST)
    new_node.AddChild(expr);
//fprintf(stderr, "CHANGED TO:\n");
//new_node.Display(stderr);

    return true;
}


void Compiler::expression(Node::pointer_t expr, Node::pointer_t params)
{
    // we already came here on that one?
    if(expr->get_link(Node::LINK_TYPE))
    {
        return;
    }

    // try to optimize the expression before to compile it
    // (it can make a huge difference!)
    f_optimizer->optimize(expr);

    Data& data = expr.GetData();

    switch(expr->get_type())
    {
    case Node::NODE_STRING:
    case Node::NODE_INT64:
    case Node::NODE_FLOAT64:
    case Node::NODE_TRUE:
    case Node::NODE_FALSE:
        TypeExpr(expr);
        return;

    case Node::NODE_ARRAY_LITERAL:
        TypeExpr(expr);
        break;

    case Node::NODE_OBJECT_LITERAL:
        ObjectLiteral(expr);
        return;

    case Node::NODE_NULL:
    case Node::NODE_PUBLIC:
    case Node::NODE_PRIVATE:
    case Node::NODE_UNDEFINED:
        return;

    case Node::NODE_SUPER:
        CheckSuperValidity(expr);
        return;

    case Node::NODE_THIS:
        CheckThisValidity(expr);
        return;

    case Node::NODE_ADD:
    case Node::NODE_ARRAY:
    case Node::NODE_AS:
    case Node::NODE_ASSIGNMENT_ADD:
    case Node::NODE_ASSIGNMENT_BITWISE_AND:
    case Node::NODE_ASSIGNMENT_BITWISE_OR:
    case Node::NODE_ASSIGNMENT_BITWISE_XOR:
    case Node::NODE_ASSIGNMENT_DIVIDE:
    case Node::NODE_ASSIGNMENT_LOGICAL_AND:
    case Node::NODE_ASSIGNMENT_LOGICAL_OR:
    case Node::NODE_ASSIGNMENT_LOGICAL_XOR:
    case Node::NODE_ASSIGNMENT_MAXIMUM:
    case Node::NODE_ASSIGNMENT_MINIMUM:
    case Node::NODE_ASSIGNMENT_MODULO:
    case Node::NODE_ASSIGNMENT_MULTIPLY:
    case Node::NODE_ASSIGNMENT_POWER:
    case Node::NODE_ASSIGNMENT_ROTATE_LEFT:
    case Node::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case Node::NODE_ASSIGNMENT_SHIFT_LEFT:
    case Node::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case Node::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case Node::NODE_ASSIGNMENT_SUBTRACT:
    case Node::NODE_BITWISE_AND:
    case Node::NODE_BITWISE_NOT:
    case Node::NODE_BITWISE_OR:
    case Node::NODE_BITWISE_XOR:
    case Node::NODE_CONDITIONAL:
    case Node::NODE_DECREMENT:
    case Node::NODE_DELETE:
    case Node::NODE_DIVIDE:
    case Node::NODE_EQUAL:
    case Node::NODE_GREATER:
    case Node::NODE_GREATER_EQUAL:
    case Node::NODE_IN:
    case Node::NODE_INCREMENT:
    case Node::NODE_INSTANCEOF:
    case Node::NODE_TYPEOF:
    case Node::NODE_IS:
    case Node::NODE_LESS:
    case Node::NODE_LESS_EQUAL:
    case Node::NODE_LIST:
    case Node::NODE_LOGICAL_AND:
    case Node::NODE_LOGICAL_NOT:
    case Node::NODE_LOGICAL_OR:
    case Node::NODE_LOGICAL_XOR:
    case Node::NODE_MATCH:
    case Node::NODE_MAXIMUM:
    case Node::NODE_MINIMUM:
    case Node::NODE_MODULO:
    case Node::NODE_MULTIPLY:
    case Node::NODE_NOT_EQUAL:
    case Node::NODE_POST_DECREMENT:
    case Node::NODE_POST_INCREMENT:
    case Node::NODE_POWER:
    case Node::NODE_RANGE:
    case Node::NODE_ROTATE_LEFT:
    case Node::NODE_ROTATE_RIGHT:
    case Node::NODE_SCOPE:
    case Node::NODE_SHIFT_LEFT:
    case Node::NODE_SHIFT_RIGHT:
    case Node::NODE_SHIFT_RIGHT_UNSIGNED:
    case Node::NODE_STRICTLY_EQUAL:
    case Node::NODE_STRICTLY_NOT_EQUAL:
    case Node::NODE_SUBTRACT:
        break;

    case Node::NODE_NEW:
        if(ExpressionNew(expr)) {
            return;
        }
        break;

    case Node::NODE_VOID:
    {
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
        // this is what void returns
        data.f_type = NODE_UNDEFINED;
        // and we don't need to keep the children
        int idx = expr.GetChildCount();
        while(idx > 0)
        {
            idx--;
            expr.DeleteChild(idx);
        }
    }
        return;

    case Node::NODE_ASSIGNMENT:
        AssignmentOperator(expr);
        return;

    case Node::NODE_FUNCTION:
        Function(expr);
        return;

    case Node::NODE_MEMBER:
        ResolveMember(expr, params, SEARCH_FLAG_GETTER);
        return;

    case Node::NODE_IDENTIFIER:
    case Node::NODE_VIDENTIFIER:
    {
        if(SpecialIdentifier(expr)) {
            return;
        }
        NodePtr resolution;
        if(ResolveName(expr, expr, resolution, params, SEARCH_FLAG_GETTER)) {
            if(!ReplaceConstantVariable(expr, resolution)) {
                NodePtr& current = expr.GetLink(NodePtr::LINK_INSTANCE);
                if(!current.HasNode()) {
                    expr.SetLink(NodePtr::LINK_INSTANCE, resolution);
                }
#if defined(_DEBUG) || defined(DEBUG)
                else {
                    AS_ASSERT(current.SameAs(resolution));
                }
#endif
                NodePtr& type = resolution.GetLink(NodePtr::LINK_TYPE);
                if(type.HasNode()) {
                    expr.SetLink(NodePtr::LINK_TYPE, type);
                }
            }
        }
        else {
            f_error_stream->ErrStrMsg(AS_ERR_NOT_FOUND, expr, "cannot find any variable or class declaration for: '%S'.", &data.f_str);
        }
    }
        return;

    case Node::NODE_CALL:
        ResolveCall(expr);
        return;

    default:
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INTERNAL_ERROR, f_lexer->get_input()->get_position());
        msg << "unhandled expression data type \"" << expr->get_type_name() << "\".";
    }
        return;

    }

// When not returned yet, we want that expression to
// compile all the children nodes as expressions.
    int max = expr.GetChildCount();
    {
        NodeLock ln(expr);
        for(int idx = 0; idx < max; ++idx) {
            NodePtr& child = expr.GetChild(idx);
            if(child.HasNode()) {
                Data& data = child.GetData();
                // skip labels
                if(data.f_type != NODE_NAME) {
                    Expression(child);
                }
            }
        }
    }

// Now check for operators to give them a type
    switch(data.f_type) {
    case NODE_ADD:
    case NODE_SUBTRACT:
        if(max == 1) {
            UnaryOperator(expr);
        }
        else {
            BinaryOperator(expr);
        }
        break;

    case NODE_BITWISE_NOT:
    case NODE_DECREMENT:
    case NODE_INCREMENT:
    case NODE_LOGICAL_NOT:
    case NODE_POST_DECREMENT:
    case NODE_POST_INCREMENT:
        UnaryOperator(expr);
        break;

    case NODE_BITWISE_AND:
    case NODE_BITWISE_OR:
    case NODE_BITWISE_XOR:
    case NODE_DIVIDE:
    case NODE_EQUAL:
    case NODE_GREATER:
    case NODE_GREATER_EQUAL:
    case NODE_LESS:
    case NODE_LESS_EQUAL:
    case NODE_LOGICAL_AND:
    case NODE_LOGICAL_OR:
    case NODE_LOGICAL_XOR:
    case NODE_MATCH:
    case NODE_MAXIMUM:
    case NODE_MINIMUM:
    case NODE_MODULO:
    case NODE_MULTIPLY:
    case NODE_NOT_EQUAL:
    case NODE_POWER:
    case NODE_RANGE:
    case NODE_ROTATE_LEFT:
    case NODE_ROTATE_RIGHT:
    case NODE_SCOPE:
    case NODE_SHIFT_LEFT:
    case NODE_SHIFT_RIGHT:
    case NODE_SHIFT_RIGHT_UNSIGNED:
    case NODE_STRICTLY_EQUAL:
    case NODE_STRICTLY_NOT_EQUAL:
        binary_operator(expr);
        break;

    case NODE_IN:
    case NODE_CONDITIONAL:    // cannot be overwritten!
        break;

    case NODE_ARRAY:
    case NODE_ARRAY_LITERAL:
    case NODE_AS:
    case NODE_DELETE:
    case NODE_INSTANCEOF:
    case NODE_IS:
    case NODE_TYPEOF:
    case NODE_VOID:
        // nothing special we can do here...
        break;

    case NODE_NEW:
        CanInstantiateType(expr.GetChild(0));
        break;

    case NODE_LIST:
    {
        // this is the type of the last entry
        NodePtr& child = expr.GetChild(max - 1);
        expr.SetLink(NodePtr::LINK_TYPE, child.GetLink(NodePtr::LINK_TYPE));
    }
        break;

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
