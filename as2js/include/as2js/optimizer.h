#ifndef AS2JS_NODE_H
#define AS2JS_NODE_H
/* optimizer.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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


#include    "sswf/libsswf_as.h"

namespace sswf
{
namespace as
{




class IntOptimizer : public Optimizer
{
public:
                IntOptimizer(void);
    virtual            ~IntOptimizer();

    virtual void        SetErrorStream(ErrorStream& error_stream);
    virtual void        SetOptions(Options& options);
    virtual int        Optimize(NodePtr& root);
    virtual    int        GetLastLabel(void) const;
    virtual void        SetFirstLabel(int label);

private:
    void            Run(NodePtr& root);
    int            Compare(NodePtr& relational);
    void            Label(String& label);

    void            Add(NodePtr& add);
    void            Assignment(NodePtr& add);
    void            AssignmentAdd(NodePtr& assignment);
    void            AssignmentDivide(NodePtr& assignment);
    void            AssignmentMultiply(NodePtr& assignment);
    void            AssignmentModulo(NodePtr& assignment);
    void            BitwiseAnd(NodePtr& bitwise_and);
    void            BitwiseNot(NodePtr& bitwise_not);
    void            BitwiseOr(NodePtr& bitwise_or);
    void            BitwiseXOr(NodePtr& bitwise_xor);
    void            Conditional(NodePtr& conditional);
    void            Decrement(NodePtr& decrement);
    void            DirectiveList(NodePtr& id);
    void            Divide(NodePtr& divide);
    void            Do(NodePtr& do_node);
    void            Equality(NodePtr& equality, bool strict, bool logical_not);
    void            Greater(NodePtr& logical_and);
    void            GreaterEqual(NodePtr& logical_and);
    void            If(NodePtr& if_node);
    void            Increment(NodePtr& increment);
    void            Less(NodePtr& logical_and);
    void            LessEqual(NodePtr& logical_and);
    void            LogicalAnd(NodePtr& logical_and);
    void            LogicalNot(NodePtr& logical_not);
    void            LogicalOr(NodePtr& logical_or);
    void            LogicalXOr(NodePtr& logical_xor);
    void            Maximum(NodePtr& minmax);
    void            Minimum(NodePtr& minmax);
    void            Modulo(NodePtr& divide);
    void            Multiply(NodePtr& multiply);
    void            Power(NodePtr& multiply);
    void            RotateLeft(NodePtr& rotate_left);
    void            RotateRight(NodePtr& rotate_right);
    void            ShiftLeft(NodePtr& shift_left);
    void            ShiftRight(NodePtr& shift_right);
    void            ShiftRightUnsigned(NodePtr& shift_right_unsigned);
    void            Subtract(NodePtr& subtract);
    void            While(NodePtr& while_node);

    ErrorStream        f_default_error_stream;
    ErrorStream *        f_error_stream;
    Options *        f_options;
    int            f_label;    // for auto-label naming
    int            f_errcnt;
};





}
// namespace as2js

#endif
// #ifndef AS2JS_NODE_H

// vim: ts=4 sw=4 et
