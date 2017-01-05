#ifndef AS2JS_PARSER_H
#define AS2JS_PARSER_H
/* as2js/parser.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


#include    "as2js/lexer.h"

namespace as2js
{


// OLD DOCUMENTATION
// We do not have a separate interface for now...
//
// The parser class is mostly hidden to you.
// You can't derive from it. You call the CreateParser() to use it.
// Once you are finished with the parser, delete it.
// Note that deleting the parser doesn't delete the nodes and thus
// you can work with the tree even after you deleted the parser.
//
// You use like this:
//
//    using namespace sswf::as; // a using namespace is not recommended, though
//    MyInput input;
//    Parser *parser = Parser::CreateParser();
//    parser->SetInput(input);
//    // it is optional to set the options
//    parser->SetOptions(options);
//    NodePtr root = parser->Parse();
//
// NOTE: the input and options are NOT copied, a pointer to these
// object is saved in the parser. Delete the Parser() before you
// delete them. Also, this means you can change the options as the
// parsing goes on (i.e. usually this happens in Input::Error().).
class Parser
{
public:
    typedef std::shared_ptr<Parser>     pointer_t;

                        Parser(Input::pointer_t input, Options::pointer_t options);

    Node::pointer_t     parse();

private:
    void                get_token();
    void                unget_token(Node::pointer_t& data);
    bool                has_option_set(Options::option_t option) const;

    void                additive_expression(Node::pointer_t& node);
    void                assignment_expression(Node::pointer_t& node);
    void                attributes(Node::pointer_t& attr_list);
    void                bitwise_and_expression(Node::pointer_t& node);
    void                bitwise_or_expression(Node::pointer_t& node);
    void                bitwise_xor_expression(Node::pointer_t& node);
    void                block(Node::pointer_t& node);
    void                break_continue(Node::pointer_t& node, Node::node_t type);
    void                case_directive(Node::pointer_t& node);
    void                catch_directive(Node::pointer_t& node);
    void                class_declaration(Node::pointer_t& node, Node::node_t type);
    void                conditional_expression(Node::pointer_t& node, bool assignment);
    void                contract_declaration(Node::pointer_t& node, Node::node_t type);
    void                debugger(Node::pointer_t& node);
    void                default_directive(Node::pointer_t& node);
    void                directive(Node::pointer_t& node);
    void                directive_list(Node::pointer_t& node);
    void                do_directive(Node::pointer_t& node);
    void                enum_declaration(Node::pointer_t& node);
    void                equality_expression(Node::pointer_t& node);
    void                expression(Node::pointer_t& node);
    void                function(Node::pointer_t& node, bool const expression);
    void                for_directive(Node::pointer_t& node);
    void                forced_block(Node::pointer_t& node, Node::pointer_t statement);
    void                goto_directive(Node::pointer_t& node);
    void                if_directive(Node::pointer_t& node);
    void                import(Node::pointer_t& node);
    void                list_expression(Node::pointer_t& node, bool rest, bool empty);
    void                logical_and_expression(Node::pointer_t& node);
    void                logical_or_expression(Node::pointer_t& node);
    void                logical_xor_expression(Node::pointer_t& node);
    void                match_expression(Node::pointer_t& node);
    void                min_max_expression(Node::pointer_t& node);
    void                multiplicative_expression(Node::pointer_t& node);
    void                namespace_block(Node::pointer_t& node, Node::pointer_t& attr_list);
    void                numeric_type(Node::pointer_t& numeric_type_node, Node::pointer_t& name);
    void                object_literal_expression(Node::pointer_t& node);
    void                parameter_list(Node::pointer_t& node, bool& has_out);
    void                pragma();
    void                pragma_option(Options::option_t option, bool prima, Node::pointer_t& argument, Options::option_value_t value);
    void                program(Node::pointer_t& node);
    void                package(Node::pointer_t& node);
    void                postfix_expression(Node::pointer_t& node);
    void                power_expression(Node::pointer_t& node);
    void                primary_expression(Node::pointer_t& node);
    void                relational_expression(Node::pointer_t& node);
    void                return_directive(Node::pointer_t& node);
    void                shift_expression(Node::pointer_t& node);
    void                switch_directive(Node::pointer_t& node);
    void                synchronized(Node::pointer_t& node);
    void                throw_directive(Node::pointer_t& node);
    void                try_finally(Node::pointer_t& node, Node::node_t const type);
    void                unary_expression(Node::pointer_t& node);
    void                use_namespace(Node::pointer_t& node);
    void                variable(Node::pointer_t& node, Node::node_t const type);
    void                with_while(Node::pointer_t& node, Node::node_t const type);
    void                yield(Node::pointer_t& node);

    Lexer::pointer_t            f_lexer;
    Options::pointer_t          f_options;
    Node::pointer_t             f_root;
    Node::pointer_t             f_node;    // last data read by get_token()
    Node::vector_of_pointers_t  f_unget;
};





}
// namespace as2js
#endif
// #ifndef AS2JS_PARSER_H

// vim: ts=4 sw=4 et
