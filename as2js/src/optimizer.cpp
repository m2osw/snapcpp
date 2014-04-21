/* optimizer.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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

#include "as2js/optimizer.h"



namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  OPTIMIZER CREATOR  **********************************************/
/**********************************************************************/
/**********************************************************************/

Optimizer *Optimizer::CreateOptimizer(void)
{
    return new IntOptimizer();
}


const char *Optimizer::Version(void)
{
    return TO_STR(SSWF_VERSION);
}


/**********************************************************************/
/**********************************************************************/
/***  INTERNAL OPTIMIZER  *********************************************/
/**********************************************************************/
/**********************************************************************/

IntOptimizer::IntOptimizer(void)
{
    f_error_stream = &f_default_error_stream;
    f_options = 0;
    f_label = 0;
    f_errcnt = 0;
}


IntOptimizer::~IntOptimizer()
{
}


void IntOptimizer::SetErrorStream(ErrorStream& error_stream)
{
    f_error_stream = &error_stream;
}


void IntOptimizer::SetOptions(Options& options)
{
    f_options = &options;
}




/**********************************************************************/
/**********************************************************************/
/***  OPTIMIZE  *******************************************************/
/**********************************************************************/
/**********************************************************************/

int IntOptimizer::Optimize(NodePtr& node)
{
    f_errcnt = 0;

    Run(node);

    return f_errcnt;
}



void IntOptimizer::Label(String& label)
{
    char    buf[256];

    snprintf(buf, sizeof(buf), "__optimizer__%d", f_label);
    f_label++;
    label = buf;
}

int IntOptimizer::GetLastLabel(void) const
{
    return f_label;
}

void IntOptimizer::SetFirstLabel(int label)
{
    f_label = label;
}



void IntOptimizer::Run(NodePtr& node)
{
    int        idx, max;

    // accept empty nodes, just ignore them
    if(!node.HasNode())
    {
        return;
    }

    // we need to optimize the child most nodes first
    max = node.GetChildCount();
    for(idx = 0; idx < max; ++idx)
    {
        NodePtr& child = node.GetChild(idx);
        if(child.HasNode())
        {
            Run(child); // recurse

            Data& data = child.GetData();
            if(data.f_type == NODE_UNKNOWN)
            {
                node.DeleteChild(idx);
                idx--;
                max--;
            }
        }
    }

    Data& data = node.GetData();
    switch(data.f_type) {
    case NODE_DIRECTIVE_LIST:
        DirectiveList(node);
        break;

    case NODE_IF:
        If(node);
        break;

    case NODE_WHILE:
        While(node);
        break;

    case NODE_DO:
        Do(node);
        break;

    case NODE_ASSIGNMENT:
        Assignment(node);
        break;

    case NODE_ASSIGNMENT_ADD:
    case NODE_ASSIGNMENT_SUBTRACT:
        AssignmentAdd(node);
        break;

    case NODE_ASSIGNMENT_MULTIPLY:
        AssignmentMultiply(node);
        break;

    case NODE_ASSIGNMENT_DIVIDE:
        AssignmentDivide(node);
        break;

    case NODE_ASSIGNMENT_MODULO:
        AssignmentModulo(node);
        break;

    case NODE_BITWISE_NOT:
        BitwiseNot(node);
        break;

    case NODE_LOGICAL_NOT:
        LogicalNot(node);
        break;

    case NODE_DECREMENT:
        Decrement(node);
        break;

    case NODE_INCREMENT:
        Increment(node);
        break;

    case NODE_POWER:
        Power(node);
        break;

    case NODE_MULTIPLY:
        Multiply(node);
        break;

    case NODE_DIVIDE:
        Divide(node);
        break;

    case NODE_MODULO:
        Modulo(node);
        break;

    case NODE_ADD:
        Add(node);
        break;

    case NODE_SUBTRACT:
        Subtract(node);
        break;

    case NODE_SHIFT_LEFT:
        ShiftLeft(node);
        break;

    case NODE_SHIFT_RIGHT:
        ShiftRight(node);
        break;

    case NODE_SHIFT_RIGHT_UNSIGNED:
        ShiftRightUnsigned(node);
        break;

    case NODE_ROTATE_LEFT:
        RotateLeft(node);
        break;

    case NODE_ROTATE_RIGHT:
        RotateRight(node);
        break;

    case NODE_LESS:
        Less(node);
        break;

    case NODE_LESS_EQUAL:
        LessEqual(node);
        break;

    case NODE_GREATER:
        Greater(node);
        break;

    case NODE_GREATER_EQUAL:
        GreaterEqual(node);
        break;

    case NODE_EQUAL:
        Equality(node, false, false);
        break;

    case NODE_STRICTLY_EQUAL:
        Equality(node, true, false);
        break;

    case NODE_NOT_EQUAL:
        Equality(node, false, true);
        break;

    case NODE_STRICTLY_NOT_EQUAL:
        Equality(node, true, true);
        break;

    case NODE_BITWISE_AND:
        BitwiseAnd(node);
        break;

    case NODE_BITWISE_XOR:
        BitwiseXOr(node);
        break;

    case NODE_BITWISE_OR:
        BitwiseOr(node);
        break;

    case NODE_LOGICAL_AND:
        LogicalAnd(node);
        break;

    case NODE_LOGICAL_XOR:
        LogicalXOr(node);
        break;

    case NODE_LOGICAL_OR:
        LogicalOr(node);
        break;

    case NODE_MAXIMUM:
        Maximum(node);
        break;

    case NODE_MINIMUM:
        Minimum(node);
        break;

    case NODE_CONDITIONAL:
        Conditional(node);
        break;

    default:
        break;

    }
}



void IntOptimizer::DirectiveList(NodePtr& list)
{
    int max = list.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = list.GetChild(idx);
        Data& data = child.GetData();
        if(data.f_type == NODE_IDENTIFIER) {
            NodePtr& instance = child.GetLink(NodePtr::LINK_INSTANCE);
            if(instance.HasNode()) {
                list.DeleteChild(idx);
                --idx;
                --max;
            }
        }
    }
}


void IntOptimizer::If(NodePtr& if_node)
{
    int max = if_node.GetChildCount();
    if(max != 2 && max != 3) {
        return;
    }

    NodePtr& condition = if_node.GetChild(0);
    Data& data = condition.GetData();
    if(data.ToBoolean()) {
        if(data.f_type == NODE_TRUE) {
            NodePtr then = if_node.GetChild(1);
            if_node.DeleteChild(1);
            if_node.ReplaceWith(then);
        }
        else if(max == 3) {
            NodePtr else_node = if_node.GetChild(2);
            if_node.DeleteChild(2);
            if_node.ReplaceWith(else_node);
        }
        else {
            // we want to delete the if_node, it serves
            // no purpose! (another function is in charge
            // of deleting; we just mark the node as unknown)
            Data& data = if_node.GetData();
            data.f_type = NODE_UNKNOWN;
        }
    }
}


void IntOptimizer::While(NodePtr& while_node)
{
    int max = while_node.GetChildCount();
    if(max != 2) {
        return;
    }

// TODO:
//
// if we detect something such as:
//
//    cnt = 3
//    while(cnt > 0) {
//        ...
//    }
//
// we can reduce to:
//
//    cnt = 3;
//    do {
//        ...
//    } while(cnt > 0);
//
// since the first time cnt > 0 will always be true.
//

    NodePtr& condition = while_node.GetChild(0);
    Data& data = condition.GetData();
    if(data.ToBoolean()) {
        if(data.f_type == NODE_TRUE) {
            // TODO:
            // Whenever we detect a forever loop we
            // need to check whether it includes some
            // break. If there isn't a break, we should
            // err about it. It is likely a bad one!
            // [except if the function returns Never]
            //
            // create a forever loop
            NodePtr forever;
            forever.CreateNode(NODE_DIRECTIVE_LIST);
            forever.CopyInputInfo(while_node);

            NodePtr label;
            label.CreateNode(NODE_LABEL);
            label.CopyInputInfo(while_node);
            Data& label_data = label.GetData();
            Label(label_data.f_str);
            forever.AddChild(label);

            NodePtr list = while_node.GetChild(1);
            while_node.DeleteChild(1);
            forever.AddChild(list);

            NodePtr goto_label;
            goto_label.CreateNode(NODE_GOTO);
            goto_label.CopyInputInfo(while_node);
            Data& goto_data = goto_label.GetData();
            goto_data.f_str = label_data.f_str;
            forever.AddChild(goto_label);

            while_node.ReplaceWith(forever);
        }
        else {
            // we want to delete the when_node, it serves
            // not purpose! (another function is in charge
            // of deleting; we just mark the node as unknown)
            Data& data = while_node.GetData();
            data.f_type = NODE_UNKNOWN;
        }
    }
}


void IntOptimizer::Do(NodePtr& do_node)
{
    int max = do_node.GetChildCount();
    if(max != 2) {
        return;
    }

    NodePtr& condition = do_node.GetChild(1);
    Data& data = condition.GetData();
    if(data.ToBoolean()) {
        if(data.f_type == NODE_TRUE) {
            // TODO:
            // Whenever we detect a forever loop we
            // need to check whether it includes some
            // break. If there isn't a break, we should
            // err about it. It is likely a bad one!
            // [except if the function returns Never]
            //
            // create a forever loop
            NodePtr forever;
            forever.CreateNode(NODE_DIRECTIVE_LIST);
            forever.CopyInputInfo(do_node);

            NodePtr label;
            label.CreateNode(NODE_LABEL);
            label.CopyInputInfo(do_node);
            Data& label_data = label.GetData();
            Label(label_data.f_str);
            forever.AddChild(label);

            NodePtr list = do_node.GetChild(0);
            do_node.DeleteChild(0);
            forever.AddChild(list);

            NodePtr goto_label;
            goto_label.CreateNode(NODE_GOTO);
            goto_label.CopyInputInfo(do_node);
            Data& goto_data = goto_label.GetData();
            goto_data.f_str = label_data.f_str;
            forever.AddChild(goto_label);

            do_node.ReplaceWith(forever);
        }
        else {
            // in this case, we simply run the
            // directives once
            NodePtr list = do_node.GetChild(0);
            do_node.DeleteChild(0);
            do_node.ReplaceWith(do_node.GetChild(0));
        }
    }
}


void IntOptimizer::Assignment(NodePtr& assignment)
{
    if(assignment.GetChildCount() != 2) {
        return;
    }

    NodePtr left = assignment.GetChild(0);
    NodePtr& right = assignment.GetChild(1);

    Data& ldata = left.GetData();
    Data& rdata = right.GetData();

    if(ldata.f_type == NODE_IDENTIFIER
    && rdata.f_type == NODE_IDENTIFIER
    && ldata.f_str == rdata.f_str) {
        // TODO: fix the parenting whenever we copy a
        //     node in another! What we really need
        //     is a recurcive function which reparents
        //     all the children once in a while...
        assignment.DeleteChild(0);
        assignment.ReplaceWith(left);
        // The following is valid ONLY if the offset is up to date
        //assignment.GetParent().SetChild(assignment.GetOffset(), left);
    }
}


void IntOptimizer::AssignmentAdd(NodePtr& assignment)
{
    // a += 0 -> a
    // a -= 0 -> a
    if(assignment.GetChildCount() != 2) {
        return;
    }

    NodePtr& right = assignment.GetChild(1);
    Data& data = right.GetData();
    if(data.f_type == NODE_INT64) {
        if(data.f_int.Get() == 0) {
            NodePtr left = assignment.GetChild(0);
            assignment.DeleteChild(0);
            assignment.ReplaceWith(left);
        }
    }
    else if(data.f_type == NODE_FLOAT64) {
        if(data.f_float.Get() == 0) {
            NodePtr left = assignment.GetChild(0);
            assignment.DeleteChild(0);
            assignment.ReplaceWith(left);
        }
    }
}


void IntOptimizer::AssignmentMultiply(NodePtr& assignment)
{
    // a *= 0 -> 0
    // a *= 1 -> a
    if(assignment.GetChildCount() != 2) {
        return;
    }

    NodePtr right = assignment.GetChild(1);
    Data& data = right.GetData();
    if(data.f_type == NODE_INT64) {
        if(data.f_int.Get() == 0) {
            assignment.DeleteChild(1);
            assignment.ReplaceWith(right);
        }
        else if(data.f_int.Get() == 1) {
            NodePtr left = assignment.GetChild(0);
            assignment.DeleteChild(0);
            assignment.ReplaceWith(left);
        }
    }
    else if(data.f_type == NODE_FLOAT64) {
        if(data.f_float.Get() == 0.0) {
            assignment.DeleteChild(1);
            assignment.ReplaceWith(right);
        }
        else if(data.f_float.Get() == 1.0) {
            NodePtr left = assignment.GetChild(0);
            assignment.DeleteChild(0);
            assignment.ReplaceWith(left);
        }
    }
}


void IntOptimizer::AssignmentDivide(NodePtr& assignment)
{
    // a /= 1 -> a
    // a /= 0 -> ERROR
    if(assignment.GetChildCount() != 2) {
        return;
    }

    NodePtr& right = assignment.GetChild(1);
    Data& data = right.GetData();
    if(data.f_type == NODE_INT64) {
        if(data.f_int.Get() == 0) {
            f_error_stream->ErrMsg(AS_ERR_DIVIDE_BY_ZERO, right, "dividing by zero is illegal");
            f_errcnt++;
        }
        else if(data.f_int.Get() == 1) {
            NodePtr left = assignment.GetChild(0);
            assignment.DeleteChild(0);
            assignment.ReplaceWith(left);
        }
    }
    else if(data.f_type == NODE_FLOAT64) {
        if(data.f_float.Get() == 0.0) {
            f_error_stream->ErrMsg(AS_ERR_DIVIDE_BY_ZERO, right, "dividing by zero is illegal");
            f_errcnt++;
        }
        else if(data.f_float.Get() == 1.0) {
            NodePtr left = assignment.GetChild(0);
            assignment.DeleteChild(0);
            assignment.ReplaceWith(left);
        }
    }
}


void IntOptimizer::AssignmentModulo(NodePtr& assignment)
{
    // a %= 1 -> a
    // a %= 0 -> ERROR
    if(assignment.GetChildCount() != 2) {
        return;
    }

    NodePtr& right = assignment.GetChild(1);
    Data& data = right.GetData();
    if(data.f_type == NODE_INT64) {
        if(data.f_int.Get() == 0) {
            f_error_stream->ErrMsg(AS_ERR_DIVIDE_BY_ZERO, right, "modulo by zero is illegal");
            f_errcnt++;
        }
    }
    else if(data.f_type == NODE_FLOAT64) {
        if(data.f_float.Get() == 0.0) {
            f_error_stream->ErrMsg(AS_ERR_DIVIDE_BY_ZERO, right, "modulo by zero is illegal");
            f_errcnt++;
        }
    }
}



// TODO: all arithmetic operations need to be checked for overflows...
//     toBoolean(NaN) == false

void IntOptimizer::BitwiseNot(NodePtr& bitwise_not)
{
    if(bitwise_not.GetChildCount() != 1) {
        return;
    }
    Data& result = bitwise_not.GetData();

    NodePtr child = bitwise_not.GetChild(0);
    Data data = child.GetData();
    if(data.ToNumber()) {
        result.f_type = data.f_type;
        if(data.f_type == NODE_INT64) {
            result.f_int.Set(~data.f_int.Get());
        }
        else {
            // ECMAScript version 4 says we can do this with floats!
            result.f_float.Set(~((int64_t) data.f_float.Get() & 0x0FFFFFFFF));
        }
    }
    else {
        // We assume that the expression was already
        // compiled and thus identifiers which were
        // constants have been replaced already.
        return;
    }

// we don't need our child anymore
    bitwise_not.DeleteChild(0);
}


void IntOptimizer::LogicalNot(NodePtr& logical_not)
{
    if(logical_not.GetChildCount() != 1) {
        return;
    }
    Data& result = logical_not.GetData();

    NodePtr child = logical_not.GetChild(0);
    Data data = child.GetData();
    if(data.ToBoolean()) {
        if(data.f_type == NODE_TRUE) {
            result.f_type = NODE_FALSE;
        }
        else {
            result.f_type = NODE_TRUE;
        }
    }
    else {
        if(data.f_type == NODE_LOGICAL_NOT) {
            // Reduce !!expr to expr
            // TODO:
            // we lose the convertion of boolean...
            // we may want to reconsider this optimization!
            NodePtr expr = child.GetChild(0);
            child.DeleteChild(0);
            logical_not.ReplaceWith(expr);
        }
        // We assume that the expression was already
        // compiled and thus identifiers which were
        // constants have been replaced already.
        return;
    }

// we don't need our child anymore
    logical_not.DeleteChild(0);
}


void IntOptimizer::Decrement(NodePtr& decrement)
{
    if(decrement.GetChildCount() != 1)
    {
        return;
    }
    Data& result = decrement.GetData();

    NodePtr child = decrement.GetChild(0);
    Data data = child.GetData();
    if(data.ToNumber())
    {
        if(data.f_type == NODE_INT64)
        {
            result.f_int.Set(data.f_int.Get() - 1);
        }
        else
        {
            result.f_float.Set(data.f_float.Get() - 1.0);
        }
    }
    else
    {
        // We assume that the expression was already
        // compiled and thus identifiers which were
        // constants have been replaced already.
        return;
    }

    result.f_type = data.f_type;

    // we don't need our child anymore
    decrement.DeleteChild(0);
}


void IntOptimizer::Increment(NodePtr& increment)
{
    if(increment.GetChildCount() != 1) {
        return;
    }
    Data& result = increment.GetData();

    NodePtr child = increment.GetChild(0);
    Data data = child.GetData();
    if(data.ToNumber()) {
        if(data.f_type == NODE_INT64) {
            result.f_int.Set(data.f_int.Get() + 1);
        }
        else {
            result.f_float.Set(data.f_float.Get() + 1.0);
        }
    }
    else {
        // We assume that the expression was already
        // compiled and thus identifiers which were
        // constants have been replaced already.
        return;
    }

    result.f_type = data.f_type;

// we don't need our child anymore
    increment.DeleteChild(0);
}


void IntOptimizer::Power(NodePtr& power)
{
    if(power.GetChildCount() != 2) {
        return;
    }

    // in case we reduce, we may use this
    Data& power_data = power.GetData();

    NodePtr lchild = power.GetChild(0);
    Data left = lchild.GetData();

    NodePtr rchild = power.GetChild(1);
    Data right = rchild.GetData();

    if(!right.ToNumber()) {
        // Reduce the following
        //    0 ** b = 0 (we can't garantee that b <> 0, we can't do it)
        //    1 ** b = 1
        if(!left.ToNumber()) {
            return;
        }
        if(left.f_type == NODE_INT64) {
            if(left.f_int.Get() == 1) {
                if(rchild.HasSideEffects()) {
                    power.DeleteChild(0);
                    power.DeleteChild(1);
                    power.AddChild(rchild);
                    power.AddChild(lchild);
                    power_data.f_type = NODE_LIST;
                }
                else {
                    power.DeleteChild(0);
                    power.ReplaceWith(lchild);
                }
                return;
            }
        }
        else {
            if(left.f_float.Get() == 1.0) {
                if(rchild.HasSideEffects()) {
                    power.DeleteChild(0);
                    power.DeleteChild(1);
                    power.AddChild(rchild);
                    power.AddChild(lchild);
                    power_data.f_type = NODE_LIST;
                }
                else {
                    power.DeleteChild(0);
                    power.ReplaceWith(lchild);
                }
                return;
            }
        }
        return;
    }

    //
    // Reduce the following if possible
    //    a ** 0 = 1
    //    a ** 1 = a
    //    a ** 2 = a * a (we don't do this one because 'a' can be a
    //            complex expression which we don't want
    //            to duplicate)
    //
    if(right.f_type == NODE_INT64) {
        if(right.f_int.Get() == 0) {
            Data& right = rchild.GetData();
            right.f_int.Set(1LL);
            // the result is always 1
            if(lchild.HasSideEffects()) {
                power_data.f_type = NODE_LIST;
                return;
            }
            power.DeleteChild(1);
            power.ReplaceWith(rchild);
            return;
        }
        else if(right.f_int.Get() == 1) {
            power.DeleteChild(0);
            power.ReplaceWith(lchild);
            return;
        }
    }
    else {
        if(right.f_float.Get() == 0.0) {
            Data& right = rchild.GetData();
            right.f_float.Set(1.0);
            // the result is always 1
            if(lchild.HasSideEffects()) {
                power_data.f_type = NODE_LIST;
                return;
            }
            power.DeleteChild(1);
            power.ReplaceWith(rchild);
            return;
        }
        else if(right.f_float.Get() == 1.0) {
            power.DeleteChild(0);
            power.ReplaceWith(lchild);
            return;
        }
    }

    if(!left.ToNumber()) {
        return;
    }

    Data& result = power.GetData();

    if(left.f_type == NODE_INT64) {
        if(right.f_type == NODE_INT64) {
            result.f_type = NODE_INT64;
            result.f_int.Set((int64_t) pow((double) left.f_int.Get(), (double) right.f_int.Get()));
        }
        else {
            result.f_type = NODE_FLOAT64;
            result.f_float.Set(pow((double) left.f_int.Get(), right.f_float.Get()));
        }
    }
    else {
        result.f_type = NODE_FLOAT64;
        if(right.f_type == NODE_INT64) {
            result.f_float.Set(pow(left.f_float.Get(), (double) right.f_int.Get()));
        }
        else {
            result.f_float.Set(pow(left.f_float.Get(), right.f_float.Get()));
        }
    }

// we don't need any of these children anymore
    power.DeleteChild(1);
    power.DeleteChild(0);
}



void IntOptimizer::Multiply(NodePtr& multiply)
{
    int64_t        itotal;
    double        ftotal;
    node_t        type;
    int        idx, max;
    bool        constant;
    NodePtr        zero;

    // Reduce
    //    a * 0 = 0
    //    a * 1 = a

    // NOTE: we should also be able to optimize -1 to a simple
    //     negate; this is not very useful for Flash though
    //     since it doesn't even have a negate (we need to
    //     use 0 - expr anyway)

    constant = true;
    max = multiply.GetChildCount();
    for(idx = 0; idx < max && max > 1; ++idx) {
        NodePtr& child = multiply.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(data.f_int.Get() == 0) {
                    // anything else won't matter
                    // (except function calls with
                    // side effects)
                    if(!zero.HasNode()) {
                        zero = child;
                    }
                }
                else if(data.f_int.Get() == 1) {
                    // ignore this child
                    multiply.DeleteChild(idx);
                    --idx;
                    --max;
                }
            }
            else {
                if(data.f_float.Get() == 0.0) {
                    // anything else won't matter
                    // (except function calls with
                    // side effects)
                    zero = child;
                }
                else if(data.f_float.Get() == 1.0) {
                    // ignore this child
                    multiply.DeleteChild(idx);
                    --idx;
                    --max;
                }
            }
        }
        else {
            constant = false;
        }
    }

    if(zero.HasNode() && max > 1) {
        max = multiply.GetChildCount();
        for(idx = 0; idx < max; ++idx) {
            NodePtr& child = multiply.GetChild(idx);
            if(!child.HasSideEffects()
            && !child.SameAs(zero)) {
                multiply.DeleteChild(idx);
                --idx;
                --max;
            }
        }
    }

    if(max == 1) {
        // Ha! We deleted many 1's or everything but one
        // zero; remove the multiplication and just leave
        // the other member or the zero
        NodePtr expr = multiply.GetChild(0);
        multiply.DeleteChild(0);
        multiply.ReplaceWith(expr);
        return;
    }

    if(!constant) {
        return;
    }

    type = NODE_INT64;
    itotal = 1;
    ftotal = 1.0;
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = multiply.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_FLOAT64) {
                    ftotal *= data.f_int.Get();
                }
                else {
                    itotal *= data.f_int.Get();
                }
            }
            else {
                if(type == NODE_INT64) {
                    type = NODE_FLOAT64;
                    ftotal = itotal * data.f_float.Get();
                }
                else {
                    ftotal *= data.f_float.Get();
                }
            }
        }
        else {
            // we should not come here!
            AS_ASSERT(0);
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = multiply.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        multiply.DeleteChild(max);
    }
}


void IntOptimizer::Divide(NodePtr& divide)
{
    int64_t        itotal, idiv;
    double        ftotal, fdiv;
    node_t        type;
    int        idx, max;
    bool        div0, constant;

    // Reduce
    //    a / 1 = a
    // Error
    //    a / 0

    type = NODE_UNKNOWN;
    itotal = 0;
    ftotal = 0.0;
    constant = true;
    max = divide.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        div0 = false;
        NodePtr& child = divide.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                idiv = data.f_int.Get();
                if(idx > 0 && idiv == 1) {
                    divide.DeleteChild(idx);
                    --idx;
                    --max;
                }
                else if(type == NODE_UNKNOWN) {
                    type = NODE_INT64;
                    itotal = idiv;
                }
                else {
                    div0 = idiv == 0;
                    if(!div0) {
                        if(type == NODE_FLOAT64) {
                            ftotal /= idiv;
                        }
                        else {
                            itotal /= idiv;
                        }
                    }
                }
            }
            else {
                fdiv = data.f_float.Get();
                if(idx > 0 && fdiv == 1.0) {
                    divide.DeleteChild(idx);
                    --idx;
                    --max;
                }
                else if(type == NODE_UNKNOWN) {
                    type = NODE_FLOAT64;
                    ftotal = fdiv;
                }
                else {
                    div0 = fdiv == 0.0;
                    if(!div0) {
                        if(type == NODE_INT64) {
                            type = NODE_FLOAT64;
                            ftotal = itotal / fdiv;
                        }
                        else {
                            ftotal /= fdiv;
                        }
                    }
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            constant = false;
        }
        if(div0) {
            f_error_stream->ErrMsg(AS_ERR_DIVIDE_BY_ZERO, divide, "dividing by zero is illegal");
            f_errcnt++;
        }
    }

    if(max == 1) {
        NodePtr expr = divide.GetChild(0);
        divide.DeleteChild(0);
        divide.ReplaceWith(expr);
        return;
    }

    if(!constant) {
        return;
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = divide.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        divide.DeleteChild(max);
    }
}


void IntOptimizer::Modulo(NodePtr& modulo)
{
    int64_t        itotal, idiv;
    double        ftotal, fdiv;
    node_t        type;
    int        idx, max;
    bool        div0, constant;

    // Error
    //    a % 0

    type = NODE_UNKNOWN;
    itotal = 0;
    ftotal = 0.0;
    constant = true;
    max = modulo.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        div0 = false;
        NodePtr& child = modulo.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                idiv = data.f_int.Get();
                if(type == NODE_UNKNOWN) {
                    type = NODE_INT64;
                    itotal = idiv;
                }
                else {
                    div0 = idiv == 0;
                    if(!div0) {
                        if(type == NODE_FLOAT64) {
                            ftotal = fmod(ftotal, idiv);
                        }
                        else {
                            itotal %= idiv;
                        }
                    }
                }
            }
            else {
                fdiv = data.f_float.Get();
                if(type == NODE_UNKNOWN) {
                    type = NODE_FLOAT64;
                    ftotal = fdiv;
                }
                else {
                    div0 = fdiv != 0;
                    if(!div0) {
                        if(type == NODE_INT64) {
                            type = NODE_FLOAT64;
                            ftotal = fmod(itotal, fdiv);
                        }
                        else {
                            ftotal = fmod(ftotal, fdiv);
                        }
                    }
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            constant = false;
        }
        if(div0) {
            f_error_stream->ErrMsg(AS_ERR_DIVIDE_BY_ZERO, modulo, "dividing by zero is illegal");
            f_errcnt++;
        }
    }

    if(!constant) {
        return;
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = modulo.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        modulo.DeleteChild(max);
    }
}


void IntOptimizer::Add(NodePtr& add)
{
    int64_t        itotal;
    double        ftotal;
    node_t        type;
    int        idx, max;
    bool        constant;

    // Reduce:
    //    a + 0 = a
    //    0 + a = a

    constant = true;
    type = NODE_INT64;
    itotal = 0;
    ftotal = 0.0;
    max = add.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = add.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(data.f_int.Get() == 0) {
                    add.DeleteChild(idx);
                    --idx;
                    --max;
                }
                else if(type == NODE_FLOAT64) {
                    ftotal += data.f_int.Get();
                }
                else {
                    itotal += data.f_int.Get();
                }
            }
            else {
                // special case where we still want
                // to force the value to a floating
                // point value! (1 + 0.0 -> 1.0)
                if(type == NODE_INT64) {
                    type = NODE_FLOAT64;
                    ftotal = itotal + data.f_float.Get();
                }
                else {
                    ftotal += data.f_float.Get();
                }
                if(data.f_float.Get() == 0.0) {
                    add.DeleteChild(idx);
                    --idx;
                    --max;
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            constant = false;
        }
    }

    // max is 1 when the user used positive or added zero
    if(max == 1) {
        NodePtr expr = add.GetChild(0);
        add.DeleteChild(0);
        add.ReplaceWith(expr);
        return;
    }

    if(!constant) {
        return;
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = add.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        add.DeleteChild(max);
    }
}


void IntOptimizer::Subtract(NodePtr& subtract)
{
    int64_t        itotal;
    double        ftotal;
    node_t        type;
    int        idx, max, start_max;
    bool        constant;

    // Reduce:
    //    a - 0 = a

    type = NODE_UNKNOWN;
    itotal = 0;
    ftotal = 0.0;
    constant = true;
    start_max = max = subtract.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr& child = subtract.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(idx != 0 && data.f_int.Get() == 0) {
                    subtract.DeleteChild(idx);
                    --idx;
                    --max;
                }
                else if(type == NODE_UNKNOWN) {
                    type = NODE_INT64;
                    itotal = data.f_int.Get();
                }
                else if(type == NODE_FLOAT64) {
                    ftotal -= data.f_int.Get();
                }
                else {
                    itotal -= data.f_int.Get();
                }
            }
            else {
                if(idx != 0 && data.f_int.Get() == 0) {
                    subtract.DeleteChild(idx);
                    --idx;
                    --max;
                }
                else if(type == NODE_UNKNOWN) {
                    type = NODE_FLOAT64;
                    ftotal = data.f_float.Get();
                }
                else if(type == NODE_INT64) {
                    type = NODE_FLOAT64;
                    ftotal = itotal - data.f_float.Get();
                }
                else {
                    ftotal -= data.f_float.Get();
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            constant = false;
        }
    }

// cancellation of the operation? (i.e. a - 0)
    if(start_max > 1 && max == 1) {
        NodePtr expr = subtract.GetChild(0);
        subtract.DeleteChild(0);
        subtract.ReplaceWith(expr);
        return;
    }

    if(!constant) {
        return;
    }

// subtraction or negation?
    if(max == 1) {
        if(type == NODE_INT64) {
            itotal = -itotal;
        }
        else {
            ftotal = -ftotal;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = subtract.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        subtract.DeleteChild(max);
    }
}



void IntOptimizer::ShiftLeft(NodePtr& shift_left)
{
    int64_t        itotal;
    node_t        type;
    int        idx, max;

    type = NODE_UNKNOWN;
    itotal = 0;
    max = shift_left.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = shift_left.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_UNKNOWN) {
                    itotal = data.f_int.Get();
                }
                else {
                    itotal <<= data.f_int.Get() & 0x3F;
                }
                type = NODE_INT64;
            }
            else {
                if(type == NODE_UNKNOWN) {
                    itotal = (int32_t) data.f_float.Get();
                }
                else {
                    itotal <<= (int32_t) data.f_float.Get() & 0x1F;
                }
                type = NODE_FLOAT64;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = shift_left.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(itotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        shift_left.DeleteChild(max);
    }
}


void IntOptimizer::ShiftRight(NodePtr& shift_right)
{
    int64_t        itotal;
    node_t        type;
    int        idx, max;

    type = NODE_UNKNOWN;
    itotal = 0;
    max = shift_right.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = shift_right.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_UNKNOWN) {
                    itotal = data.f_int.Get();
                }
                else {
                    itotal >>= data.f_int.Get() & 0x3F;
                }
                type = NODE_INT64;
            }
            else {
                if(type == NODE_UNKNOWN) {
                    itotal = (int32_t) data.f_float.Get();
                }
                else {
                    itotal >>= (int32_t) data.f_float.Get() & 0x1F;
                }
                type = NODE_FLOAT64;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = shift_right.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(itotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        shift_right.DeleteChild(max);
    }
}



void IntOptimizer::ShiftRightUnsigned(NodePtr& shift_right_unsigned)
{
    uint64_t    itotal;
    node_t        type;
    int        idx, max;

    type = NODE_UNKNOWN;
    itotal = 0;
    max = shift_right_unsigned.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = shift_right_unsigned.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_UNKNOWN) {
                    itotal = data.f_int.Get();
                }
                else {
                    itotal >>= data.f_int.Get() & 0x3F;
                }
                type = NODE_INT64;
            }
            else {
                if(type == NODE_UNKNOWN) {
                    itotal = (int32_t) data.f_float.Get();
                }
                else {
                    itotal >>= (int32_t) data.f_float.Get() & 0x1F;
                }
                type = NODE_FLOAT64;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = shift_right_unsigned.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(itotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        shift_right_unsigned.DeleteChild(max);
    }
}


void IntOptimizer::RotateLeft(NodePtr& rotate_left)
{
    uint64_t    itotal;
    node_t        type;
    int        idx, max, count;

    type = NODE_UNKNOWN;
    itotal = 0;
    max = rotate_left.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = rotate_left.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_UNKNOWN) {
                    itotal = data.f_int.Get();
                }
                else {
                    count = data.f_int.Get() & 0x3F;
                    if(count != 0) {
                        itotal = (itotal << count) | (itotal >> (64 - count));
                    }
                }
                type = NODE_INT64;
            }
            else {
                if(type == NODE_UNKNOWN) {
                    itotal = (int32_t) data.f_float.Get();
                }
                else {
                    count = (int32_t) data.f_float.Get() & 0x1F;
                    if(count != 0) {
                        itotal = ((itotal << count) | (itotal >> (64 - count)));
                    }
                }
                type = NODE_FLOAT64;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = rotate_left.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(itotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        rotate_left.DeleteChild(max);
    }
}



void IntOptimizer::RotateRight(NodePtr& rotate_right)
{
    uint64_t    itotal;
    node_t        type;
    int        idx, max, count;

    type = NODE_UNKNOWN;
    itotal = 0;
    max = rotate_right.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = rotate_right.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_UNKNOWN) {
                    itotal = data.f_int.Get();
                }
                else {
                    count = data.f_int.Get() & 0x3F;
                    if(count != 0) {
                        itotal = (itotal >> count) | (itotal << (64 - count));
                    }
                }
                type = NODE_INT64;
            }
            else {
                if(type == NODE_UNKNOWN) {
                    itotal = (int32_t) data.f_float.Get();
                }
                else {
                    count = (int32_t) data.f_float.Get() & 0x1F;
                    if(count != 0) {
                        itotal = ((itotal >> count) | (itotal << (64 - count)));
                    }
                }
                type = NODE_FLOAT64;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = rotate_right.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(itotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        rotate_right.DeleteChild(max);
    }
}



// returns:
//     0 - equal
//    -1 - left < right
//     1 - left > right
//     2 - unordered (left or right is a NaN)
//    -2 - error (can't compare)
int IntOptimizer::Compare(NodePtr& relational)
{
    if(relational.GetChildCount() != 2) {
        return -2;
    }

    NodePtr child = relational.GetChild(0);
    Data left = child.GetData();

    child = relational.GetChild(1);
    Data right = child.GetData();

    if(left.f_type == NODE_STRING
    && right.f_type == NODE_STRING) {
        return left.f_str.Compare(right.f_str);
    }

    if(!left.ToNumber()) {
        return -2;
    }
    if(!right.ToNumber()) {
        return -2;
    }

    if(left.f_type == NODE_INT64) {
        if(right.f_type == NODE_INT64) {
            int64_t r = left.f_int.Get() - right.f_int.Get();
            if(r == 0) {
                return 0;
            }
            return r < 0 ? -1 : 1;
        }
        else {
            if(isnan(right.f_float.Get())) {
                return 2;
            }
            int inf = isinf(right.f_float.Get());
            if(inf != 0) {
                return -inf;
            }
            double r = left.f_int.Get() - right.f_float.Get();
            if(r == 0.0) {
                return 0;
            }
            return r < 0.0 ? -1 : 1;
        }
    }
    else {
        if(isnan(left.f_float.Get())) {
            return 2;
        }
        if(right.f_type == NODE_INT64) {
            int inf = isinf(left.f_float.Get());
            if(inf != 0) {
                return inf;
            }
            double r = left.f_float.Get() - right.f_int.Get();
            if(r == 0.0) {
                return 0;
            }
            return r < 0.0 ? -1 : 1;
        }
        else {
            if(isnan(right.f_float.Get())) {
                return 2;
            }
            int linf = isinf(left.f_float.Get());
            int rinf = isinf(right.f_float.Get());
            if(linf != 0 || rinf != 0) {
                if(linf == rinf) {
                    return 0;
                }
                return linf < rinf ? -1 : 1;
            }
            double r = left.f_float.Get() - right.f_float.Get();
            if(r == 0.0) {
                return 0;
            }
            return r < 0.0 ? -1 : 1;
        }
    }
}



void IntOptimizer::Less(NodePtr& less)
{
    long r = Compare(less);
    if(r == -2) {
        return;
    }

    if(r == 2) {
        // ???
        return;
    }

    Data& result = less.GetData();
    result.f_type = r < 0 ? NODE_TRUE : NODE_FALSE;

    less.DeleteChild(1);
    less.DeleteChild(0);
}


void IntOptimizer::LessEqual(NodePtr& less_equal)
{
    long r = Compare(less_equal);
    if(r == -2) {
        return;
    }

    if(r == 2) {
        // ???
        return;
    }

    Data& result = less_equal.GetData();
    result.f_type = r <= 0 ? NODE_TRUE : NODE_FALSE;

    less_equal.DeleteChild(1);
    less_equal.DeleteChild(0);
}


void IntOptimizer::Greater(NodePtr& greater)
{
    long r = Compare(greater);
    if(r == -2) {
        return;
    }

    if(r == 2) {
        // ???
        return;
    }

    Data& result = greater.GetData();
    result.f_type = r > 0 ? NODE_TRUE : NODE_FALSE;

    greater.DeleteChild(1);
    greater.DeleteChild(0);
}


void IntOptimizer::GreaterEqual(NodePtr& greater_equal)
{
    long r = Compare(greater_equal);
    if(r == -2) {
        return;
    }

    if(r == 2) {
        // ???
        return;
    }

    Data& result = greater_equal.GetData();
    result.f_type = r >= 0 ? NODE_TRUE : NODE_FALSE;

    greater_equal.DeleteChild(1);
    greater_equal.DeleteChild(0);
}


void IntOptimizer::Equality(NodePtr& equality, bool strict, bool logical_not)
{
    if(equality.GetChildCount() != 2) {
        return;
    }

    bool result = false;

    NodePtr child = equality.GetChild(0);
    Data left = child.GetData();

    child = equality.GetChild(1);
    Data right = child.GetData();

    if(strict) {        // ===
        if((left.f_type  == NODE_TRUE || left.f_type  == NODE_FALSE)
        && (right.f_type == NODE_TRUE || right.f_type == NODE_FALSE)) {
            result = left.f_type == right.f_type;
        }
        else if(left.f_type == NODE_INT64 && right.f_type == NODE_INT64) {
            result = left.f_int.Get() == right.f_int.Get();
        }
        else if(left.f_type == NODE_FLOAT64 && right.f_type == NODE_FLOAT64) {
            result = left.f_float.Get() == right.f_float.Get();
        }
        else if(left.f_type == NODE_INT64 && right.f_type == NODE_FLOAT64) {
            result = left.f_int.Get() == right.f_float.Get();
        }
        else if(left.f_type == NODE_FLOAT64 && right.f_type == NODE_INT64) {
            result = left.f_float.Get() == right.f_int.Get();
        }
        else if(left.f_type == NODE_STRING && right.f_type == NODE_STRING) {
            result = left.f_str == right.f_str;
        }
        else if((left.f_type == NODE_UNDEFINED && right.f_type == NODE_UNDEFINED)
             || (left.f_type == NODE_NULL && right.f_type == NODE_NULL)) {
            result = true;
        }
        else if((left.f_type == NODE_NULL && right.f_type == NODE_UNDEFINED)
             || (left.f_type == NODE_UNDEFINED && right.f_type == NODE_NULL)) {
            result = false;
        }
        else if((left.f_type  == NODE_IDENTIFIER || left.f_type  == NODE_VIDENTIFIER)
             && (right.f_type == NODE_IDENTIFIER || right.f_type == NODE_VIDENTIFIER)) {
            // special case of  a === a
            if(left.f_str != right.f_str) {
                // If not the same, we can't know what the result
                // is at compile time.
                return;
            }
            result = true;
        }
        else {
            switch(left.f_type) {
            case NODE_INT64:
            case NODE_FLOAT64:
            case NODE_STRING:
            case NODE_NULL:
            case NODE_UNDEFINED:
            case NODE_TRUE:
            case NODE_FALSE:
                break;

            default:
                // undefined at compile time
                return;

            }
            switch(right.f_type) {
            case NODE_INT64:
            case NODE_FLOAT64:
            case NODE_STRING:
            case NODE_NULL:
            case NODE_UNDEFINED:
            case NODE_TRUE:
            case NODE_FALSE:
                break;

            default:
                // undefined at compile time
                return;

            }
            // any one of these mix is always false in strict mode
            result = false;
        }
    }
    else {            // ==
        switch(left.f_type) {
        case NODE_UNDEFINED:
        case NODE_NULL:
            switch(right.f_type) {
            case NODE_UNDEFINED:
            case NODE_NULL:
                result = true;
                break;

            case NODE_INT64:
            case NODE_FLOAT64:
            case NODE_STRING:
            case NODE_TRUE:
            case NODE_FALSE:
                result = false;
                break;

            default:
                // undefined at compile time
                return;

            }
            break;

        case NODE_TRUE:
            switch(right.f_type) {
            case NODE_TRUE:
                result = true;
                break;

            case NODE_FALSE:
            case NODE_STRING:
            case NODE_NULL:
            case NODE_UNDEFINED:
                result = false;
                break;

            default:
                if(!right.ToNumber()) {
                    // undefined at compile time
                    return;
                }
                if(right.f_type == NODE_INT64) {
                    result = right.f_int.Get() == 1;
                }
                else {
                    result = right.f_float.Get() == 1.0;
                }
                break;

            }
            break;

        case NODE_FALSE:
            switch(right.f_type) {
            case NODE_TRUE:
            case NODE_STRING:
            case NODE_NULL:
            case NODE_UNDEFINED:
                result = false;
                break;

            case NODE_FALSE:
                result = true;
                break;

            default:
                if(!right.ToNumber()) {
                    // undefined at compile time
                    return;
                }
                if(right.f_type == NODE_INT64) {
                    result = right.f_int.Get() == 0;
                }
                else {
                    result = right.f_float.Get() == 0.0;
                }
                break;

            }
            break;

        case NODE_INT64:
            switch(right.f_type) {
            case NODE_INT64:
                result = left.f_int.Get() == right.f_int.Get();
                break;

            case NODE_FLOAT64:
                result = left.f_int.Get() == right.f_float.Get();
                break;

            case NODE_TRUE:
                result = left.f_int.Get() == 1;
                break;

            case NODE_FALSE:
                result = left.f_int.Get() == 0;
                break;

            //case NODE_STRING: ...

            case NODE_NULL:
            case NODE_UNDEFINED:
                result = false;
                break;

            default:
                return;

            }
            break;

        case NODE_FLOAT64:
            switch(right.f_type) {
            case NODE_FLOAT64:
                result = left.f_float.Get() == right.f_float.Get();
                break;

            case NODE_INT64:
                result = left.f_float.Get() == right.f_int.Get();
                break;

            case NODE_TRUE:
                result = left.f_float.Get() == 1.0;
                break;

            case NODE_FALSE:
                result = left.f_float.Get() == 0.0;
                break;

            //case NODE_STRING: ...

            case NODE_NULL:
            case NODE_UNDEFINED:
                result = false;
                break;

            default:
                return;

            }
            break;

        case NODE_STRING:
            switch(right.f_type) {
            case NODE_STRING:
                result = left.f_str == right.f_str;
                break;

            case NODE_NULL:
            case NODE_UNDEFINED:
                result = false;
                break;

            //case NODE_INT64: ...
            //case NODE_FLOAT64: ...

            default:
                return;

            }
            break;

        case NODE_IDENTIFIER:
        case NODE_VIDENTIFIER:
            if((right.f_type != NODE_IDENTIFIER && right.f_type != NODE_VIDENTIFIER)
            || left.f_str != right.f_str) {
                return;
            }
            // special case of  a == a
            result = true;
            break;

        default:
            // undefined at compile time
            return;

        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = equality.GetData();
    data.f_type = result ^ logical_not ? NODE_TRUE : NODE_FALSE;

// we don't need any of these children anymore
    equality.DeleteChild(1);
    equality.DeleteChild(0);
}



void IntOptimizer::BitwiseAnd(NodePtr& bitwise_and)
{
    int64_t        itotal;
    double        ftotal;
    node_t        type;
    int        idx, max;
    String        result;

    type = NODE_INT64;
    itotal = -1;
    ftotal = -1.0;
    max = bitwise_and.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = bitwise_and.GetChild(idx);
        Data data = child.GetData();
        if(data.f_type == NODE_STRING || type == NODE_STRING) {
            if(type != NODE_STRING && idx != 0) {
                Data value;
                value.f_type = type;
                if(type == NODE_INT64) {
                    value.f_int.Set(itotal);
                }
                else {
                    value.f_float.Set(ftotal);
                }
                value.ToString();
                result = value.f_str;
            }
            if(!data.ToString()) {
                return;
            }
            type = NODE_STRING;
            result += data.f_str;
        }
        else if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_INT64) {
                    itotal &= data.f_int.Get();
                }
                else {
                    ftotal = (int32_t) ftotal & (int32_t) data.f_int.Get();
                    type = NODE_FLOAT64;
                }
            }
            else {
                if(type == NODE_INT64) {
                    type = NODE_FLOAT64;
                    ftotal = (int32_t) itotal & (int32_t) data.f_float.Get();
                }
                else {
                    ftotal = (int32_t) ftotal & (int32_t) data.f_float.Get();
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = bitwise_and.GetData();
    data.f_type = type;
    if(type == NODE_STRING) {
        data.f_str = result;
    }
    else if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        bitwise_and.DeleteChild(max);
    }
}



void IntOptimizer::BitwiseXOr(NodePtr& bitwise_xor)
{
    int64_t        itotal;
    double        ftotal;
    node_t        type;
    int        idx, max;

    type = NODE_INT64;
    itotal = 0;
    ftotal = 0.0;
    max = bitwise_xor.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = bitwise_xor.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_INT64) {
                    itotal ^= data.f_int.Get();
                }
                else {
                    ftotal = (int32_t) ftotal ^ (int32_t) data.f_int.Get();
                }
            }
            else {
                if(type == NODE_INT64) {
                    ftotal = (int32_t) itotal ^ (int32_t) data.f_float.Get();
                }
                else {
                    ftotal = (int32_t) ftotal ^ (int32_t) data.f_float.Get();
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = bitwise_xor.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        bitwise_xor.DeleteChild(max);
    }
}



void IntOptimizer::BitwiseOr(NodePtr& bitwise_or)
{
    int64_t        itotal;
    double        ftotal;
    node_t        type;
    int        idx, max;

    type = NODE_INT64;
    itotal = 0;
    ftotal = 0.0;
    max = bitwise_or.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = bitwise_or.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_INT64) {
                    itotal |= data.f_int.Get();
                }
                else {
                    ftotal = (int32_t) ftotal | (int32_t) data.f_int.Get();
                }
            }
            else {
                if(type == NODE_INT64) {
                    ftotal = (int32_t) itotal | (int32_t) data.f_float.Get();
                }
                else {
                    ftotal = (int32_t) ftotal | (int32_t) data.f_float.Get();
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = bitwise_or.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        bitwise_or.DeleteChild(max);
    }
}



void IntOptimizer::LogicalAnd(NodePtr& logical_and)
{
    node_t        type;
    int        idx, max;

    type = NODE_TRUE;
    max = logical_and.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = logical_and.GetChild(idx);
        Data data = child.GetData();
        if(data.ToBoolean()) {
            if(data.f_type == NODE_FALSE) {
                // stop testing as soon as it is known to be false
                type = NODE_FALSE;
                break;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = logical_and.GetData();
    data.f_type = type;

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        logical_and.DeleteChild(max);
    }
}



void IntOptimizer::LogicalXOr(NodePtr& logical_xor)
{
    node_t        type;
    int        idx, max;

    type = NODE_FALSE;
    max = logical_xor.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = logical_xor.GetChild(idx);
        Data data = child.GetData();
        if(data.ToBoolean()) {
            if(data.f_type == NODE_TRUE) {
                type = type == NODE_TRUE ? NODE_FALSE : NODE_TRUE;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = logical_xor.GetData();
    data.f_type = type;

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        logical_xor.DeleteChild(max);
    }
}



void IntOptimizer::LogicalOr(NodePtr& logical_or)
{
    node_t        type;
    int        idx, max;

    type = NODE_FALSE;
    max = logical_or.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = logical_or.GetChild(idx);
        Data data = child.GetData();
        if(data.ToBoolean()) {
            if(data.f_type == NODE_TRUE) {
                type = NODE_TRUE;
                break;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = logical_or.GetData();
    data.f_type = type;

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        logical_or.DeleteChild(max);
    }
}



void IntOptimizer::Minimum(NodePtr& minimum)
{
    int r = Compare(minimum);
    if(r == -2 || r == 2) {
        return;
    }

    if(r <= 0) {
        // in this case we want to keep the left node
        minimum = minimum.GetChild(0);
    }
    else {
        minimum = minimum.GetChild(1);
    }
}



void IntOptimizer::Maximum(NodePtr& maximum)
{
    int r = Compare(maximum);
    if(r == -2 || r == 2) {
        return;
    }

    if(r >= 0) {
        // in this case we want to keep the left node
        maximum = maximum.GetChild(0);
    }
    else {
        maximum = maximum.GetChild(1);
    }
}



void IntOptimizer::Conditional(NodePtr& conditional)
{
    if(conditional.GetChildCount() != 3) {
        return;
    }

    NodePtr child = conditional.GetChild(0);
    Data data = child.GetData();
    if(!data.ToBoolean()) {
        return;
    }

    if(data.f_type == NODE_TRUE) {
        NodePtr expr = conditional.GetChild(1);
        conditional.DeleteChild(1);
        conditional.ReplaceWith(expr);
    }
    else {
        NodePtr expr = conditional.GetChild(2);
        conditional.DeleteChild(2);
        conditional.ReplaceWith(expr);
    }
}





}
// namespace as2js

// vim: ts=4 sw=4 et
