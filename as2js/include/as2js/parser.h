#ifndef AS2JS_PARSER_H
#define AS2JS_PARSER_H
/* parser.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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


#include    "as2js/as2js.h"

namespace as2js
{




class Lexer
{
public:
    static int const    MAX_UNGET = 16;

                        Lexer();

    void                SetOptions(Options& options);
    void                SetInput(Input& input);
    Input *             GetInput() const
                        {
                            return const_cast<Input *>(f_input);
                        }

    const Data&         GetNextToken();
    void                ErrMsg(err_code_t err_code, const char *format, ...);

    void                SetForIn(bool const for_in)
                        {
                            f_for_in = for_in;
                        }

private:
    static int const    CHAR_LETTER          = 0x0001;
    static int const    CHAR_DIGIT           = 0x0002;
    static int const    CHAR_PUNCTUATION     = 0x0004;
    static int const    CHAR_WHITE_SPACE     = 0x0008;
    static int const    CHAR_LINE_TERMINATOR = 0x0010;
    static int const    CHAR_HEXDIGIT        = 0x0020;
    static int const    CHAR_INVALID         = 0x8000;   // such as 0xFFFE & 0xFFFF

    long                InputGetC();
    long                GetC();
    void                UngetC(long c);
    int64_t             ReadHex(long max);
    int64_t             ReadOctal(long c, long max);
    long                EscapeSequence(void);
    long                CharType(long c);
    long                Read(long c, long flags, String& str);
    void                ReadIdentifier(long c);
    void                ReadNumber(long c);
    void                ReadString(long quote);

    long                f_last;
    long                f_type;        // type of the last character read
    Data                f_data;
    long                f_unget_pos;
    long                f_unget[MAX_UNGET];
    Input *             f_input;
    Options *           f_options;
    bool                f_for_in;    // IN becomes FOR_IN when this is true
};




class IntParser : public Parser
{
public:
    static int const MAX_UNGET = 3;

                        IntParser();
    virtual             ~IntParser();

    virtual void        SetInput(Input& input);
    virtual void        SetOptions(Options& options);
    virtual NodePtr&    Parse();

private:
    void                GetToken();
    void                UngetToken(const Data& data);

    void                AdditiveExpression(NodePtr& node);
    void                AssignmentExpression(NodePtr& node);
    void                Attributes(NodePtr& attr_list);
    void                BitwiseAndExpression(NodePtr& node);
    void                BitwiseOrExpression(NodePtr& node);
    void                BitwiseXOrExpression(NodePtr& node);
    void                Block(NodePtr& node);
    void                BreakContinue(NodePtr& node, node_t type);
    void                Case(NodePtr& node);
    void                Catch(NodePtr& node);
    void                Class(NodePtr& node, node_t type);
    void                ConditionalExpression(NodePtr& node, bool assignment);
    void                Default(NodePtr& node);
    void                Directive(NodePtr& node);
    void                DirectiveList(NodePtr& node);
    void                Do(NodePtr& node);
    void                Enum(NodePtr& node);
    void                EqualityExpression(NodePtr& node);
    void                Expression(NodePtr& node);
    void                Function(NodePtr& node, bool expression);
    void                For(NodePtr& node);
    void                Goto(NodePtr& node);
    void                If(NodePtr& node);
    void                Import(NodePtr& node);
    void                ListExpression(NodePtr& node, bool rest, bool empty);
    void                LogicalAndExpression(NodePtr& node);
    void                LogicalOrExpression(NodePtr& node);
    void                LogicalXOrExpression(NodePtr& node);
    void                MinMaxExpression(NodePtr& node);
    void                MultiplicativeExpression(NodePtr& node);
    void                Namespace(NodePtr& node);
    void                ObjectLiteralExpression(NodePtr& node);
    void                ParameterList(NodePtr& node, bool& has_out);
    void                Pragma();
    void                Program(NodePtr& node);
    void                Package(NodePtr& node);
    void                PostfixExpression(NodePtr& node);
    void                PowerExpression(NodePtr& node);
    void                PrimaryExpression(NodePtr& node);
    void                RelationalExpression(NodePtr& node);
    void                Return(NodePtr& node);
    void                ShiftExpression(NodePtr& node);
    void                Switch(NodePtr& node);
    void                Throw(NodePtr& node);
    void                TryFinally(NodePtr& node, node_t type);
    void                UnaryExpression(NodePtr& node);
    void                UseNamespace(NodePtr& node);
    void                Variable(NodePtr& node, bool const constant);
    void                WithWhile(NodePtr& node, node_t type);

    void                Pragma_Option(option_t option, bool prima, const Data& argument, long value);

    Lexer               f_lexer;
    Options *           f_options;
    NodePtr             f_root;
    Data                f_data;    // last data read by GetNextToken()
    int                 f_unget_pos;
    Data                f_unget[MAX_UNGET];
};





}
// namespace as2js
#endif
// #ifndef AS2JS_PARSER_H

// vim: ts=4 sw=4 et
