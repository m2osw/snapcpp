/* lexer.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include "as2js/lexer.h"

#include "as2js/message.h"

#include    <iomanip>


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER CREATOR  *************************************************/
/**********************************************************************/
/**********************************************************************/


Lexer::Lexer()
    //: f_char_type(CHAR_NO_FLAGS) -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_input(nullptr) -- auto-init
    //, f_options(nullptr) -- auto-init
{
}



void Lexer::set_input(Input::pointer_t& input)
{
    f_input = input;
}


Input::pointer_t Lexer::get_input() const
{
    return f_input;
}


void Lexer::set_options(Options::pointer_t& options)
{
    f_options = options;
}


Input::char_t Lexer::getc()
{
    Input::char_t c(f_input->getc());

    f_char_type = char_type(c);
    if((f_char_type & (CHAR_LINE_TERMINATOR | CHAR_WHITE_SPACE)) != 0)
    {
        switch(c)
        {
        case '\n':
            // skip '\n\r' as one newline
            do
            {
                f_input->get_position().new_line();
                c = f_input->getc();
            }
            while(c == '\n');
            if(c != '\r')
            {
                ungetc(c);
            }
            c = '\n';
            break;

        case '\r':
            // skip '\r\n' as one newline (?!)
            do
            {
                f_input->get_position().new_line();
                c = f_input->getc();
            }
            while(c == '\r');
            if(c != '\n')
            {
                ungetc(c);
            }
            c = '\n';
            break;

        case '\f':
            // view the form feed as a new page for now...
            f_input->get_position().new_page();
            break;

        case 0x0085:
            // ?
            break;

        case 0x2028:
            f_input->get_position().new_line();
            break;

        case 0x2029:
            f_input->get_position().new_paragraph();
            break;

        }
    }

    return c;
}


void Lexer::ungetc(Input::char_t c)
{
    f_input->ungetc(c);
}


Lexer::char_type_t Lexer::char_type(Input::char_t c)
{
    // TODO: this needs a HUGE improvement to be conformant...
    switch(c) {
    case '\0':
        return CHAR_INVALID;

    case '\n':
    case '\r':
    case 0x0085:
    case 0x2028:
    case 0x2029:
        return CHAR_LINE_TERMINATOR;

    case '\t':
    case '\v':
    case '\f':
    case ' ':
    case 0x00A0:
    case 0x2000: // 0x2000 ... 0x200B
    case 0x2001:
    case 0x2002:
    case 0x2003:
    case 0x2004:
    case 0x2005:
    case 0x2006:
    case 0x2007:
    case 0x2008:
    case 0x2009:
    case 0x200A:
    case 0x200B:
    case 0x3000:
        return CHAR_WHITE_SPACE;

    case '0': // '0' ... '9'
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return CHAR_DIGIT | CHAR_HEXDIGIT;

    case 'a': // 'a' ... 'f'
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'A': // 'A' ... 'F'
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        return CHAR_LETTER | CHAR_HEXDIGIT;

    case '_':
    case '$':
        return CHAR_LETTER;

    default:
        if((c >= 'g' && c <= 'z')
        || (c >= 'G' && c <= 'Z'))
        {
            return CHAR_LETTER;
        }
        if((c & 0x0FFFF) >= 0xFFFE
        || (c >= 0xD800 && c <= 0xDFFF))
        {
            // 0xFFFE and 0xFFFF are invalid in all planes
            return CHAR_INVALID;
        }
        if(c < 0x7F)
        {
            return CHAR_PUNCTUATION;
        }
        // TODO: this will be true in most cases, but not always!
        return CHAR_LETTER;

    }
    /*NOTREACHED*/
}




int64_t Lexer::read_hex(long max)
{
    int64_t result(0);
    Input::char_t c(getc());
    long p(0);
    for(; (f_char_type & CHAR_HEXDIGIT) != 0 && p < max; ++p)
    {
        if(c <= '9')
        {
            result = result * 16 + c - '0';
        }
        else
        {
            result = result * 16 + c - ('A' - 10);
        }
        c = getc();
    }
    ungetc(c);

    if(p == 0)
    {
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE, f_input->get_position());
            msg << "invalid unicode (\\[xXuU]##) escape sequence)";
        }
        return -1;
    }

    // TODO: In strict mode, should we check whether we got p == max?
    // WARNING: this is also used by the ReadNumber() function

    return result;
}


int64_t Lexer::read_octal(Input::char_t c, long max)
{
    int64_t result(c - '0');
    c = getc();
    for(long p(1); c >= '0' && c <= '7' && p < max; ++p, c = getc())
    {
        result = result * 8 + c - '0';
    }
    ungetc(c);

    return result;
}


Input::char_t Lexer::escape_sequence()
{
    Input::char_t c(getc());
    switch(c)
    {
    case 'u':
        // 4 hex digits
        return read_hex(4);

    case 'U':
        // 8 hex digits
        return read_hex(8);

    case 'x':
    case 'X':
        // 2 hex digits
        return read_hex(2);

    case '\'':
    case '\"':
    case '\\':
        return c;

    case 'b':
        return '\b';

    case 'e':
        if(has_option_set(Options::AS_OPTION_EXTENDED_ESCAPE_SEQUENCES))
        {
            return '\033';
        }
        break;

    case 'f':
        return '\f';

    case 'n':
        return '\n';

    case 'r':
        return '\r';

    case 't':
        return '\t';

    case 'v':
        return '\v';

    default:
        if(c >= '0' && c <= '7')
        {
            return read_octal(c, 3);
        }
        break;

    }

    if(c > ' ' && c < 0x7F)
    {
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_UNKNOWN_ESCAPE_SEQUENCE, f_input->get_position());
            msg << "unknown escape letter '" << static_cast<char>(c) << "'";
        }
    }
    else
    {
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_UNKNOWN_ESCAPE_SEQUENCE, f_input->get_position());
            msg << "unknown escape letter '\\U" << std::hex << std::setw(8) << std::setfill('0') << static_cast<int32_t>(c) << "'";
        }
    }

    return '?';
}


Input::char_t Lexer::read(Input::char_t c, char_type_t flags, String& str)
{
    do
    {
        if(c == '\\')
        {
            c = escape_sequence();
        }
        if((f_char_type & CHAR_INVALID) == 0)
        {
            str += c;
        }
        c = getc();
    }
    while((f_char_type & flags) != 0 && c >= 0);

    ungetc(c);

    return c;
}



void Lexer::read_identifier(Input::char_t c)
{
    String str;
    read(c, CHAR_LETTER | CHAR_DIGIT, str);

    // An identifier can be a keyword, we check that right here!
    size_t l(str.length());
    if(l > 1)
    {
        as_char_t const *s(str.c_str());
        switch(s[0])
        {
        case 'a':
            if(l == 2 && s[1] == 's')
            {
                f_result_type = Node::NODE_AS;
                break;
            }
            break;

        case 'b':
            if(l == 5 && str == "break")
            {
                f_result_type = Node::NODE_BREAK;
                break;
            }
            break;

        case 'c':
            if(l == 4 && str == "case")
            {
                f_result_type = Node::NODE_CASE;
                break;
            }
            if(l == 5 && str == "catch")
            {
                f_result_type = Node::NODE_CATCH;
                break;
            }
            if(l == 5 && str == "class")
            {
                f_result_type = Node::NODE_CLASS;
                break;
            }
            if(l == 5 && str == "const")
            {
                f_result_type = Node::NODE_CONST;
                break;
            }
            if(l == 8 && str == "continue")
            {
                f_result_type = Node::NODE_CONTINUE;
                break;
            }
            break;

        case 'd':
            if(l == 8 && str == "debugger")
            {
                f_result_type = Node::NODE_DEBUGGER;
                break;
            }
            if(l == 7 && str == "default")
            {
                f_result_type = Node::NODE_DEFAULT;
                break;
            }
            if(l == 6 && str == "delete")
            {
                f_result_type = Node::NODE_DELETE;
                break;
            }
            if(l == 2 && s[1] == 'o')
            {
                f_result_type = Node::NODE_DO;
                break;
            }
            break;

        case 'e':
            if(l == 4 && str == "else")
            {
                f_result_type = Node::NODE_ELSE;
                break;
            }
            if(l == 4 && str == "enum")
            {
                f_result_type = Node::NODE_ENUM;
                break;
            }
            if(l == 7 && str == "extends")
            {
                f_result_type = Node::NODE_EXTENDS;
                break;
            }
            break;

        case 'f':
            if(l == 5 && str == "false")
            {
                f_result_type = Node::NODE_FALSE;
                break;
            }
            if(l == 7 && str == "finally")
            {
                f_result_type = Node::NODE_FINALLY;
                break;
            }
            if(l == 3 && s[1] == 'o' && s[2] == 'r')
            {
                f_result_type = Node::NODE_FOR;
                break;
            }
            if(l == 8 && str == "function")
            {
                f_result_type = Node::NODE_FUNCTION;
                break;
            }
            break;

        case 'g':
            if(has_option_set(Options::AS_OPTION_EXTENDED_STATEMENTS))
            {
                if(l == 4 && str == "goto")
                {
                    f_result_type = Node::NODE_GOTO;
                    break;
                }
            }
            break;

        case 'i':
            if(l == 2 && s[1] == 'f')
            {
                f_result_type = Node::NODE_IF;
                break;
            }
            if(l == 10 && str == "implements")
            {
                f_result_type = Node::NODE_IMPLEMENTS;
                break;
            }
            if(l == 6 && str == "import")
            {
                f_result_type = Node::NODE_IMPORT;
                break;
            }
            if(l == 2 && s[1] == 'n')
            {
                f_result_type = Node::NODE_IN;
                break;
            }
            if(l == 10 && str == "instanceof")
            {
                f_result_type = Node::NODE_INSTANCEOF;
                break;
            }
            if(l == 9 && str == "interface")
            {
                f_result_type = Node::NODE_INTERFACE;
                break;
            }
            if(l == 2 && s[1] == 's')
            {
                f_result_type = Node::NODE_IS;
                break;
            }
            break;

        case 'n':
            if(l == 9 && str == "namespace")
            {
                f_result_type = Node::NODE_NAMESPACE;
                break;
            }
            if(l == 3 && s[1] == 'e' && s[2] == 'w')
            {
                f_result_type = Node::NODE_NEW;
                break;
            }
            if(l == 4 && str == "null")
            {
                f_result_type = Node::NODE_NULL;
                break;
            }
            break;

        case 'p':
            if(l == 7 && str == "package")
            {
                f_result_type = Node::NODE_PACKAGE;
                break;
            }
            if(l == 7 && str == "private")
            {
                f_result_type = Node::NODE_PRIVATE;
                break;
            }
            if(l == 6 && str == "public")
            {
                f_result_type = Node::NODE_PUBLIC;
                break;
            }
            break;

        case 'r':
            if(l == 6 && str == "return")
            {
                f_result_type = Node::NODE_RETURN;
                break;
            }
            break;

        case 's':
            if(l == 5 && str == "super")
            {
                f_result_type = Node::NODE_SUPER;
                break;
            }
            if(l == 6 && str == "switch")
            {
                f_result_type = Node::NODE_SWITCH;
                break;
            }
            break;

        case 't':
            if(l == 4 && str == "this")
            {
                f_result_type = Node::NODE_THIS;
                break;
            }
            if(l == 5 && str == "throw")
            {
                f_result_type = Node::NODE_THROW;
                break;
            }
            if(l == 4 && str == "true")
            {
                f_result_type = Node::NODE_TRUE;
                break;
            }
            if(l == 3 && s[1] == 'r' && s[2] == 'y')
            {
                f_result_type = Node::NODE_TRY;
                break;
            }
            if(l == 6 && str == "typeof")
            {
                f_result_type = Node::NODE_TYPEOF;
                break;
            }
            break;

        case 'u':
            if(l == 9 && str == "undefined")
            {
                // Note: undefined is actually not a reserved keyword, but
                //       by reserving it, we avoid stupid mistakes like:
                //
                //       var undefined = 5;
                //
                f_result_type = Node::NODE_UNDEFINED;
                break;
            }
            if(l == 3 && s[1] == 's' && s[2] == 'e')
            {
                f_result_type = Node::NODE_USE;
                break;
            }
            break;

        case 'v':
            if(l == 3 && s[1] == 'a' && s[2] == 'r')
            {
                f_result_type = Node::NODE_VAR;
                break;
            }
            if(l == 4 && str == "void")
            {
                f_result_type = Node::NODE_VOID;
                break;
            }
            break;

        case 'w':
            if(l == 4 && str == "with")
            {
                f_result_type = Node::NODE_WITH;
                break;
            }
            if(l == 5 && str == "while")
            {
                f_result_type = Node::NODE_WHILE;
                break;
            }
            break;

        case '_':
            if(l == 8 && str == "__FILE__")
            {
                f_result_type = Node::NODE_STRING;
                f_result_string = f_input->get_position().get_filename();
                break;
            }
            if(l == 8 && str == "__LINE__")
            {
                f_result_type = Node::NODE_INT64;
                f_result_int64 = f_input->get_position().get_line();
                break;
            }
            break;

        }
    }

    f_result_type = Node::NODE_IDENTIFIER;
    f_result_string = str;
}


void Lexer::read_number(Input::char_t c)
{
    String      number;

    if(c == '.')
    {
        // in case the strtod() doesn't support a missing 0
        // at the start of the string
        number = "0.";
    }
    else if(c == '0')
    {
        c = getc();
        if(c == 'x' || c == 'X')
        {
            // hexadecimal number
            f_result_type = Node::NODE_INT64;
            f_result_int64 = read_hex(16);
            return;
        }
        // octal is not permitted in ECMAScript version 3+
        if(has_option_set(Options::AS_OPTION_OCTAL)
        && c >= '0' && c <= '7')
        {
            // octal
            f_result_type = Node::NODE_INT64;
            f_result_int64 = read_octal(c, 22);
            return;
        }
        number = "0";
        ungetc(c);
    }
    else
    {
        c = read(c, CHAR_DIGIT, number);
    }

    if(c == '.')
    {
        // TODO: we may want to support 32 bits floats as well
        f_result_type = Node::NODE_FLOAT64;
        c = getc(); // re-read the '.' from f_input

        // TODO:
        // Here we could check to know whether this really
        // represents a decimal number or whether the decimal
        // point is a member operator. This can be very tricky.

        c = read(c, CHAR_DIGIT, number);
        if(c == 'e' || c == 'E')
        {
            number += 'e';
            getc();        // skip the 'e'
            c = getc();    // get the character after!
            if(c == '-' || c == '+' || (c >= '0' && c <= '9'))
            {
                c = read(c, CHAR_DIGIT, number);
            }
        }
        // TODO: detect whether an error was detected in the conversion
        f_result_float64 = strtod(number.to_utf8().c_str(), 0);
    }
    else
    {
        // TODO: Support 8, 16, 32 bits, unsigned thereof
        f_result_type = Node::NODE_INT64;
        // TODO: detect whether an error was detected in the conversion
        f_result_int64 = strtoll(number.to_utf8().c_str(), 0, 10);
    }
}


void Lexer::read_string(Input::char_t quote)
{
    f_result_type = Node::NODE_STRING;
    f_result_string.clear();

    for(Input::char_t c(getc()); c != quote; c = getc())
    {
        if(c < 0)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_UNTERMINTED_STRING, f_input->get_position());
            msg << "the last string was not closed before the end of the input was reached";
            return;
        }
        if((f_char_type & CHAR_LINE_TERMINATOR) != 0)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_UNTERMINTED_STRING, f_input->get_position());
            msg << "a string cannot include a line terminator";
            return;
        }
        if(c == '\\')
        {
            c = escape_sequence();
            // here c can be equal to quote (c == quote)
        }
        f_result_string += c;
    }
}



Node::pointer_t Lexer::get_new_node(Node::node_t type)
{
    Node::pointer_t node(new Node(type));
    node->set_position(f_position);
    // no data by default in this case
    return node;
}


Node::pointer_t Lexer::get_next_token()
{
    // get the info
    get_token();

    // create a node for the result
    Node::pointer_t node(new Node(f_result_type));
    node->set_position(f_position);
    switch(f_result_type)
    {
    case Node::NODE_IDENTIFIER:
    case Node::NODE_STRING:
        node->set_string(f_result_string);
        break;

    case Node::NODE_INT64:
        node->set_int64(f_result_int64);
        break;

    case Node::NODE_FLOAT64:
        node->set_float64(f_result_float64);
        break;

    default:
        // no data attached
        break;

    }
    return node;
}


void Lexer::get_token()
{
    for(Input::char_t c(getc());; c = getc())
    {
        f_position = f_input->get_position();
        if(c < 0)
        {
            // we're done
            f_result_type = Node::NODE_EOF;
            return;
        }

        if((f_char_type & (CHAR_WHITE_SPACE | CHAR_LINE_TERMINATOR | CHAR_INVALID)) != 0)
        {
            continue;
        }

        if((f_char_type & CHAR_LETTER) != 0)
        {
            read_identifier(c);
            return;
        }

        if((f_char_type & CHAR_DIGIT) != 0)
        {
            read_number(c);
            return;
        }

        switch(c) {
        case '"':
        case '\'':
        case '`':    // TODO: do we want to support the correct regex syntax?
            read_string(c);
            if(c == '`')
            {
                f_result_type = Node::NODE_REGULAR_EXPRESSION;
            }
            return;

        case '<':
            c = getc();
            if(c == '<')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::NODE_ASSIGNMENT_SHIFT_LEFT;
                    return;
                }
                ungetc(c);
                f_result_type = Node::NODE_SHIFT_LEFT;
                return;
            }
            if(c == '=')
            {
                f_result_type = Node::NODE_LESS_EQUAL;
                return;
            }
            if(has_option_set(Options::AS_OPTION_EXTENDED_OPERATORS))
            {
                if(c == '>')
                {
                    f_result_type = Node::NODE_NOT_EQUAL;
                    return;
                }
            }
            ungetc(c);
            f_result_type = Node::NODE_LESS;
            return;

        case '>':
            c = getc();
            if(c == '>')
            {
                c = getc();
                if(c == '>')
                {
                    c = getc();
                    if(c == '=')
                    {
                        f_result_type = Node::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED;
                        return;
                    }
                    ungetc(c);
                    f_result_type = Node::NODE_SHIFT_RIGHT_UNSIGNED;
                    return;
                }
                if(c == '=')
                {
                    f_result_type = Node::NODE_ASSIGNMENT_SHIFT_RIGHT;
                    return;
                }
                ungetc(c);
                f_result_type = Node::NODE_SHIFT_RIGHT;
                return;
            }
            if(c == '=')
            {
                f_result_type = Node::NODE_GREATER_EQUAL;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_GREATER;
            return;

        case '!':
            c = getc();
            if(has_option_set(Options::AS_OPTION_EXTENDED_OPERATORS))
            {
                if(c == '<')
                {
                    c = getc();
                    if(c == '=')
                    {
                        f_result_type = Node::NODE_ASSIGNMENT_ROTATE_LEFT;
                        return;
                    }
                    ungetc(c);
                    f_result_type = Node::NODE_ROTATE_LEFT;
                    return;
                }
                if(c == '>')
                {
                    c = getc();
                    if(c == '=')
                    {
                        f_result_type = Node::NODE_ASSIGNMENT_ROTATE_RIGHT;
                        return;
                    }
                    ungetc(c);
                    f_result_type = Node::NODE_ROTATE_RIGHT;
                    return;
                }
            }
            if(c == '=')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::NODE_STRICTLY_NOT_EQUAL;
                    return;
                }
                ungetc(c);
                f_result_type = Node::NODE_NOT_EQUAL;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_LOGICAL_NOT;
            return;

        case '=':
            c = getc();
            if(c == '=')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::NODE_STRICTLY_EQUAL;
                    return;
                }
                ungetc(c);
                f_result_type = Node::NODE_EQUAL;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_ASSIGNMENT;
            return;

        case ':':
            c = getc();
            if(has_option_set(Options::AS_OPTION_EXTENDED_OPERATORS)
            && c == '=')
            {
                f_result_type = Node::NODE_ASSIGNMENT;
                return;
            }
            if(c == ':')
            {
                f_result_type = Node::NODE_SCOPE;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_COLON;
            return;

        case '~':
            c = getc();
            if(has_option_set(Options::AS_OPTION_EXTENDED_OPERATORS)
            && c == '=')
            {
                f_result_type = Node::NODE_MATCH;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_BITWISE_NOT;
            return;

        case '+':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::NODE_ASSIGNMENT_ADD;
                return;
            }
            if(c == '+')
            {
                f_result_type = Node::NODE_INCREMENT;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_ADD;
            return;

        case '-':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::NODE_ASSIGNMENT_SUBTRACT;
                return;
            }
            if(c == '-')
            {
                f_result_type = Node::NODE_DECREMENT;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_SUBTRACT;
            return;

        case '*':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::NODE_ASSIGNMENT_MULTIPLY;
                return;
            }
            if(has_option_set(Options::AS_OPTION_EXTENDED_OPERATORS)
            && c == '*')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::NODE_ASSIGNMENT_POWER;
                    return;
                }
                ungetc(c);
                f_result_type = Node::NODE_POWER;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_MULTIPLY;
            return;

        case '/':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::NODE_ASSIGNMENT_DIVIDE;
                return;
            }
            if(c == '/')
            {
                // skip comments (to end of line)
                do
                {
                    c = getc();
                }
                while((f_char_type & CHAR_LINE_TERMINATOR) == 0 && c > 0);
                break;
            }
            if(c == '*')
            {
                // skip comments (multiline)
                do
                {
                    c = getc();
                    while(c == '*')
                    {
                        c = getc();
                        if(c == '/')
                        {
                            c = -1;
                            break;
                        }
                    }
                }
                while(c > 0);
                break;
            }
            ungetc(c);
            f_result_type = Node::NODE_DIVIDE;
            return;

        case '%':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::NODE_ASSIGNMENT_MODULO;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_MODULO;
            return;

        case '?':
            c = getc();
            if(has_option_set(Options::AS_OPTION_EXTENDED_OPERATORS))
            {
                if(c == '<')
                {
                    c = getc();
                    if(c == '=')
                    {
                        f_result_type = Node::NODE_ASSIGNMENT_MINIMUM;
                        return;
                    }
                    ungetc(c);
                    f_result_type = Node::NODE_MINIMUM;
                    return;
                }
                if(c == '>')
                {
                    c = getc();
                    if(c == '=')
                    {
                        f_result_type = Node::NODE_ASSIGNMENT_MAXIMUM;
                        return;
                    }
                    ungetc(c);
                    f_result_type = Node::NODE_MAXIMUM;
                    return;
                }
            }
            ungetc(c);
            f_result_type = Node::NODE_CONDITIONAL;
            return;

        case '&':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::NODE_ASSIGNMENT_BITWISE_AND;
                return;
            }
            if(c == '&')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::NODE_ASSIGNMENT_LOGICAL_AND;
                    return;
                }
                ungetc(c);
                f_result_type = Node::NODE_LOGICAL_AND;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_BITWISE_AND;
            return;

        case '^':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::NODE_ASSIGNMENT_BITWISE_XOR;
                return;
            }
            if(c == '^')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::NODE_ASSIGNMENT_LOGICAL_XOR;
                    return;
                }
                ungetc(c);
                f_result_type = Node::NODE_LOGICAL_XOR;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_BITWISE_XOR;
            return;

        case '|':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::NODE_ASSIGNMENT_BITWISE_OR;
                return;
            }
            if(c == '|')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::NODE_ASSIGNMENT_LOGICAL_OR;
                    return;
                }
                ungetc(c);
                f_result_type = Node::NODE_LOGICAL_OR;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_BITWISE_OR;
            return;

        case '.':
            c = getc();
            if(c >= '0' && c <= '9')
            {
                // this is a valid float
                ungetc(c);
                read_number('.');
                return;
            }
            if(c == '.')
            {
                c = getc();
                if(c == '.')
                {
                    // Elipsis!
                    f_result_type = Node::NODE_REST;
                    return;
                }
                ungetc(c);

                // Range (not too sure if this is really used yet
                // and whether it will be called RANGE)
                f_result_type = Node::NODE_RANGE;
                return;
            }
            ungetc(c);
            f_result_type = Node::NODE_MEMBER;
            return;

        case '[':
            f_result_type = Node::NODE_OPEN_SQUARE_BRACKET;
            return;

        case ']':
            f_result_type = Node::NODE_CLOSE_SQUARE_BRACKET;
            return;

        case '{':
            f_result_type = Node::NODE_OPEN_CURVLY_BRACKET;
            return;

        case '}':
            f_result_type = Node::NODE_CLOSE_CURVLY_BRACKET;
            return;

        case '(':
            f_result_type = Node::NODE_OPEN_PARENTHESIS;
            return;

        case ')':
            f_result_type = Node::NODE_CLOSE_PARENTHESIS;
            return;

        case ';':
            f_result_type = Node::NODE_SEMICOLON;
            return;

        case ',':
            f_result_type = Node::NODE_COMMA;
            return;

        default:
            if(c > ' ' && c < 0x7F)
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_UNEXPECTED_PUNCTUATION, f_input->get_position());
                msg << "unexpected punctuation '" << static_cast<char>(c) << "'";
            }
            else
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_UNEXPECTED_PUNCTUATION, f_input->get_position());
                msg << "unexpected punctuation '\\U" << c << "'";
            }
            break;

        }
    }
    /*NOTREACHED*/
}


bool Lexer::has_option_set(Options::option_t option) const
{
    if(f_options)
    {
        return f_options->get_option(option);
    }

    return false;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
