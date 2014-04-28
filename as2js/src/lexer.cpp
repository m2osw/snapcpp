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
    //: f_type(CHAR_NO_FLAGS) -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_input(nullptr) -- auto-init
    //, f_options(nullptr) -- auto-init
    //, f_for_in(false)
{
}



void Lexer::set_input(Input::input_pointer_t& input)
{
    f_input = input;
}


Input::input_pointer_t Lexer::get_input() const
{
    return f_input;
}


void Lexer::set_options(Options::options_pointer_t& options)
{
    f_options = options;
}


void Lexer::set_for_in(bool const for_in)
{
    f_for_in = for_in;
}


Input::char_t Lexer::getc()
{
    Input::char_t c(f_input->getc());

    f_type = char_type(c);
    if((f_type & (CHAR_LINE_TERMINATOR | CHAR_WHITE_SPACE)) != 0)
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
        if((c & 0x0FFFF) >= 0xFFFE)
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
    for(; (f_type & CHAR_HEXDIGIT) != 0 && p < max; ++p)
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
        if(f_options
        && f_options->get_option(Options::AS_OPTION_EXTENDED_ESCAPE_SEQUENCES) != 0)
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
        if((f_type & CHAR_INVALID) == 0)
        {
            str += c;
        }
        c = getc();
    }
    while((f_type & flags) != 0 && c >= 0);

    ungetc(c);

    return c;
}



void Lexer::read_identifier(Input::char_t c)
{
    f_data.f_type = NODE_IDENTIFIER;
    QString str;
    c = read(c, CHAR_LETTER | CHAR_DIGIT, str);

    // An identifier can be a keyword, we check that right here!
    long l = f_data.f_str.GetLength();
    if(l > 1)
    {
        const long *s = f_data.f_str.Get();
        switch(s[0]) {
        case 'a':
            if(l == 2 && s[1] == 's') {
                f_data.f_type = NODE_AS;
                break;
            }
            break;

        case 'b':
            if(l == 5 && f_data.f_str == "break") {
                f_data.f_type = NODE_BREAK;
                break;
            }
            break;

        case 'c':
            if(l == 4 && f_data.f_str == "case") {
                f_data.f_type = NODE_CASE;
                break;
            }
            if(l == 5 && f_data.f_str == "catch") {
                f_data.f_type = NODE_CATCH;
                break;
            }
            if(l == 5 && f_data.f_str == "class") {
                f_data.f_type = NODE_CLASS;
                break;
            }
            if(l == 5 && f_data.f_str == "const") {
                f_data.f_type = NODE_CONST;
                break;
            }
            if(l == 8 && f_data.f_str == "continue") {
                f_data.f_type = NODE_CONTINUE;
                break;
            }
            break;

        case 'd':
            if(l == 8 && f_data.f_str == "debugger") {
                f_data.f_type = NODE_DEBUGGER;
                break;
            }
            if(l == 7 && f_data.f_str == "default") {
                f_data.f_type = NODE_DEFAULT;
                break;
            }
            if(l == 6 && f_data.f_str == "delete") {
                f_data.f_type = NODE_DELETE;
                break;
            }
            if(l == 2 && s[1] == 'o') {
                f_data.f_type = NODE_DO;
                break;
            }
            break;

        case 'e':
            if(l == 4 && f_data.f_str == "else") {
                f_data.f_type = NODE_ELSE;
                break;
            }
            if(l == 4 && f_data.f_str == "enum") {
                f_data.f_type = NODE_ENUM;
                break;
            }
            if(l == 7 && f_data.f_str == "extends") {
                f_data.f_type = NODE_EXTENDS;
                break;
            }
            break;

        case 'f':
            if(l == 5 && f_data.f_str == "false") {
                f_data.f_type = NODE_FALSE;
                break;
            }
            if(l == 7 && f_data.f_str == "finally") {
                f_data.f_type = NODE_FINALLY;
                break;
            }
            if(l == 3 && s[1] == 'o' && s[2] == 'r') {
                f_data.f_type = NODE_FOR;
                break;
            }
            if(l == 8 && f_data.f_str == "function") {
                f_data.f_type = NODE_FUNCTION;
                break;
            }
            break;

        case 'g':
            if(f_options != 0
            && f_options->GetOption(AS_OPTION_EXTENDED_STATEMENTS) != 0) {
                if(l == 4 && f_data.f_str == "goto") {
                    f_data.f_type = NODE_GOTO;
                    break;
                }
            }
            break;

        case 'i':
            if(l == 2 && s[1] == 'f') {
                f_data.f_type = NODE_IF;
                break;
            }
            if(l == 10 && f_data.f_str == "implements") {
                f_data.f_type = NODE_IMPLEMENTS;
                break;
            }
            if(l == 6 && f_data.f_str == "import") {
                f_data.f_type = NODE_IMPORT;
                break;
            }
            if(l == 2 && s[1] == 'n') {
                f_data.f_type = f_for_in ? NODE_FOR_IN : NODE_IN;
                break;
            }
            if(l == 10 && f_data.f_str == "instanceof") {
                f_data.f_type = NODE_INSTANCEOF;
                break;
            }
            if(l == 9 && f_data.f_str == "interface") {
                f_data.f_type = NODE_INTERFACE;
                break;
            }
            if(l == 2 && s[1] == 's') {
                f_data.f_type = NODE_IS;
                break;
            }
            break;

        case 'n':
            if(l == 9 && f_data.f_str == "namespace") {
                f_data.f_type = NODE_NAMESPACE;
                break;
            }
            if(l == 3 && s[1] == 'e' && s[2] == 'w') {
                f_data.f_type = NODE_NEW;
                break;
            }
            if(l == 4 && f_data.f_str == "null") {
                f_data.f_type = NODE_NULL;
                break;
            }
            break;

        case 'p':
            if(l == 7 && f_data.f_str == "package") {
                f_data.f_type = NODE_PACKAGE;
                break;
            }
            if(l == 7 && f_data.f_str == "private") {
                f_data.f_type = NODE_PRIVATE;
                break;
            }
            if(l == 6 && f_data.f_str == "public") {
                f_data.f_type = NODE_PUBLIC;
                break;
            }
            break;

        case 'r':
            if(l == 6 && f_data.f_str == "return") {
                f_data.f_type = NODE_RETURN;
                break;
            }
            break;

        case 's':
            if(l == 5 && f_data.f_str == "super") {
                f_data.f_type = NODE_SUPER;
                break;
            }
            if(l == 6 && f_data.f_str == "switch") {
                f_data.f_type = NODE_SWITCH;
                break;
            }
            break;

        case 't':
            if(l == 4 && f_data.f_str == "this") {
                f_data.f_type = NODE_THIS;
                break;
            }
            if(l == 5 && f_data.f_str == "throw") {
                f_data.f_type = NODE_THROW;
                break;
            }
            if(l == 4 && f_data.f_str == "true") {
                f_data.f_type = NODE_TRUE;
                break;
            }
            if(l == 3 && s[1] == 'r' && s[2] == 'y') {
                f_data.f_type = NODE_TRY;
                break;
            }
            if(l == 6 && f_data.f_str == "typeof") {
                f_data.f_type = NODE_TYPEOF;
                break;
            }
            break;

        case 'u':
            if(l == 9 && f_data.f_str == "undefined") {
                f_data.f_type = NODE_UNDEFINED;
                break;
            }
            if(l == 3 && s[1] == 's' && s[2] == 'e') {
                f_data.f_type = NODE_USE;
                break;
            }
            break;

        case 'v':
            if(l == 3 && s[1] == 'a' && s[2] == 'r') {
                f_data.f_type = NODE_VAR;
                break;
            }
            if(l == 4 && f_data.f_str == "void") {
                f_data.f_type = NODE_VOID;
                break;
            }
            break;

        case 'w':
            if(l == 4 && f_data.f_str == "with") {
                f_data.f_type = NODE_WITH;
                break;
            }
            if(l == 5 && f_data.f_str == "while") {
                f_data.f_type = NODE_WHILE;
                break;
            }
            break;

        case '_':
            if(l == 8 && f_data.f_str == "__FILE__") {
                f_data.f_type = NODE_STRING;
                f_data.f_str = f_input->GetFilename();
                break;
            }
            if(l == 8 && f_data.f_str == "__LINE__") {
                f_data.f_type = NODE_INT64;
                f_data.f_int.Set(f_input->Line());
                break;
            }
            break;

        }
    }
}


void Lexer::read_number(char_t c)
{
    String      number;
    char        buf[256];
    size_t      sz;

    buf[sizeof(buf) - 1] = '\0';

    if(c == '.')
    {
        // in case the strtod() doesn't support a missing 0
        // at the start of the string
        number.AppendChar('0');
        number.AppendChar('.');
    }
    else if(c == '0')
    {
        c = GetC();
        if(c == 'x' || c == 'X')
        {
            // hexadecimal number
            f_data.f_type = NODE_INT64;
            f_data.f_int.Set(read_hex(16));
            return;
        }
        // octal is not permitted in ECMAScript version 3+
        if(f_options
        && f_options->get_option(AS_OPTION_OCTAL) != 0
        && c >= '0' && c <= '7')
        {
            // octal
            f_data.f_type = NODE_INT64;
            f_data.f_int.set(read_octal(c, 22));
            return;
        }
        number.AppendChar('0');
        ungetc(c);
    }
    else
    {
        c = read(c, CHAR_DIGIT, number);
    }

    if(c == '.')
    {
        // TODO: we may want to support 32 bits floats as well
        f_data.f_type = NODE_FLOAT64;
        c = getc();

        // TODO:
        // Here we could check to know whether this really
        // represents a decimal number or whether the decimal
        // point is a member operator. This can be very tricky.

        c = Read(c, CHAR_DIGIT, number);
        if(c == 'e' || c == 'E') {
            number.AppendChar('e');
            GetC();        // skip the 'e'
            c = GetC();    // get the character after!
            if(c == '-' || c == '+' || (c >= '0' && c <= '9'))
            {
                c = Read(c, CHAR_DIGIT, number);
            }
        }
        sz = sizeof(buf);
        number.ToUTF8(buf, sz);
        f_data.f_float.Set(strtod(buf, 0));
    }
    else
    {
        // TODO: Support 8, 16, 32 bits, unsigned thereof
        f_data.f_type = NODE_INT64;
        sz = sizeof(buf);
        number.ToUTF8(buf, sz);
        f_data.f_int.Set(strtoll(buf, 0, 10));
    }
}


void Lexer::read_string(Input::char_t quote)
{
    f_data.f_type = NODE_STRING;

    for(Input::char_t c(getc()); c != quote; c = getc())
    {
        if(c < 0)
        {
            f_input->ErrMsg(AS_ERR_UNTERMINTED_STRING, "the last string wasn't closed before the end of the input was reached");
            return;
        }
        if((f_type & CHAR_LINE_TERMINATOR) != 0)
        {
            f_input->ErrMsg(AS_ERR_UNTERMINTED_STRING, "a string can't include a line terminator");
            return;
        }
        if(c == '\\')
        {
            c = escape_sequence();
            // here c can be equal to quote (c == quote)
        }
        f_data.f_str.AppendChar(c);
    }
}



Node::node_pointer_t Lexer::GetNextToken()
{
    f_result = Node::node_pointer_t(new Node);

    for(Input::char_t c(getc());; c = getc()) {
        if(c < 0)
        {
            // we're done
            f_data.f_type = NODE_EOF;
            return f_data;
        }

        if((f_type & (CHAR_WHITE_SPACE | CHAR_LINE_TERMINATOR | CHAR_INVALID)) != 0)
        {
            continue;
        }

        if((f_type & CHAR_LETTER) != 0) {
            ReadIdentifier(c);
            return f_data;
        }

        if((f_type & CHAR_DIGIT) != 0) {
            ReadNumber(c);
            return f_data;
        }

        switch(c) {
        case '"':
        case '\'':
        case '`':    // TODO: do we want to support correct regex?
            ReadString(c);
            if(c == '`') {
                f_data.f_type = NODE_REGULAR_EXPRESSION;
            }
            return f_data;

        case '<':
            c = GetC();
            if(c == '<') {
                c = GetC();
                if(c == '=') {
                    f_data.f_type = NODE_ASSIGNMENT_SHIFT_LEFT;
                    return f_data;
                }
                UngetC(c);
                f_data.f_type = NODE_SHIFT_LEFT;
                return f_data;
            }
            if(c == '=') {
                f_data.f_type = NODE_LESS_EQUAL;
                return f_data;
            }
            if(f_options != 0
            && f_options->GetOption(AS_OPTION_EXTENDED_OPERATORS) != 0) {
                if(c == '>') {
                    f_data.f_type = NODE_NOT_EQUAL;
                    return f_data;
                }
            }
            UngetC(c);
            f_data.f_type = NODE_LESS;
            return f_data;

        case '>':
            c = GetC();
            if(c == '>') {
                c = GetC();
                if(c == '>') {
                    c = GetC();
                    if(c == '=') {
                        f_data.f_type = NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED;
                        return f_data;
                    }
                    UngetC(c);
                    f_data.f_type = NODE_SHIFT_RIGHT_UNSIGNED;
                    return f_data;
                }
                if(c == '=') {
                    f_data.f_type = NODE_ASSIGNMENT_SHIFT_RIGHT;
                    return f_data;
                }
                UngetC(c);
                f_data.f_type = NODE_SHIFT_RIGHT;
                return f_data;
            }
            if(c == '=') {
                f_data.f_type = NODE_GREATER_EQUAL;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_GREATER;
            return f_data;

        case '!':
            c = GetC();
            if(f_options != 0
            && f_options->GetOption(AS_OPTION_EXTENDED_OPERATORS) != 0) {
                if(c == '<') {
                    c = GetC();
                    if(c == '=') {
                        f_data.f_type = NODE_ASSIGNMENT_ROTATE_LEFT;
                        return f_data;
                    }
                    UngetC(c);
                    f_data.f_type = NODE_ROTATE_LEFT;
                    return f_data;
                }
                if(c == '>') {
                    c = GetC();
                    if(c == '=') {
                        f_data.f_type = NODE_ASSIGNMENT_ROTATE_RIGHT;
                        return f_data;
                    }
                    UngetC(c);
                    f_data.f_type = NODE_ROTATE_RIGHT;
                    return f_data;
                }
            }
            if(c == '=') {
                c = GetC();
                if(c == '=') {
                    f_data.f_type = NODE_STRICTLY_NOT_EQUAL;
                    return f_data;
                }
                UngetC(c);
                f_data.f_type = NODE_NOT_EQUAL;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_LOGICAL_NOT;
            return f_data;

        case '=':
            c = GetC();
            if(c == '=') {
                c = GetC();
                if(c == '=') {
                    f_data.f_type = NODE_STRICTLY_EQUAL;
                    return f_data;
                }
                UngetC(c);
                f_data.f_type = NODE_EQUAL;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_ASSIGNMENT;
            return f_data;

        case ':':
            c = GetC();
            if(f_options != 0
            && f_options->GetOption(AS_OPTION_EXTENDED_OPERATORS) != 0
            && c == '=') {
                f_data.f_type = NODE_ASSIGNMENT;
                return f_data;
            }
            if(c == ':') {
                f_data.f_type = NODE_SCOPE;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_COLON;
            return f_data;

        case '~':
            c = GetC();
            if(f_options != 0
            && f_options->GetOption(AS_OPTION_EXTENDED_OPERATORS) != 0
            && c == '=') {
                f_data.f_type = NODE_MATCH;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_BITWISE_NOT;
            return f_data;

        case '+':
            c = GetC();
            if(c == '=') {
                f_data.f_type = NODE_ASSIGNMENT_ADD;
                return f_data;
            }
            if(c == '+') {
                f_data.f_type = NODE_INCREMENT;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_ADD;
            return f_data;

        case '-':
            c = GetC();
            if(c == '=') {
                f_data.f_type = NODE_ASSIGNMENT_SUBTRACT;
                return f_data;
            }
            if(c == '-') {
                f_data.f_type = NODE_DECREMENT;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_SUBTRACT;
            return f_data;

        case '*':
            c = GetC();
            if(c == '=') {
                f_data.f_type = NODE_ASSIGNMENT_MULTIPLY;
                return f_data;
            }
            if(f_options != 0
            && f_options->GetOption(AS_OPTION_EXTENDED_OPERATORS) != 0
            && c == '*') {
                c = GetC();
                if(c == '=') {
                    f_data.f_type = NODE_ASSIGNMENT_POWER;
                    return f_data;
                }
                UngetC(c);
                f_data.f_type = NODE_POWER;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_MULTIPLY;
            return f_data;

        case '/':
            c = GetC();
            if(c == '=') {
                f_data.f_type = NODE_ASSIGNMENT_DIVIDE;
                return f_data;
            }
            if(c == '/') {
                // skip comments (to end of line)
                do {
                    c = GetC();
                } while((f_type & CHAR_LINE_TERMINATOR) == 0 && c > 0);
                break;
            }
            if(c == '*') {
                // skip comments (multiline)
                do {
                    c = GetC();
                    while(c == '*') {
                        c = GetC();
                        if(c == '/') {
                            c = -1;
                            break;
                        }
                    }
                } while(c > 0);
                break;
            }
            UngetC(c);
            f_data.f_type = NODE_DIVIDE;
            return f_data;

        case '%':
            c = GetC();
            if(c == '=') {
                f_data.f_type = NODE_ASSIGNMENT_MODULO;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_MODULO;
            return f_data;

        case '?':
            c = GetC();
            if(f_options != 0
            && f_options->GetOption(AS_OPTION_EXTENDED_OPERATORS) != 0) {
                if(c == '<') {
                    c = GetC();
                    if(c == '=') {
                        f_data.f_type = NODE_ASSIGNMENT_MINIMUM;
                        return f_data;
                    }
                    UngetC(c);
                    f_data.f_type = NODE_MINIMUM;
                    return f_data;
                }
                if(c == '>') {
                    c = GetC();
                    if(c == '=') {
                        f_data.f_type = NODE_ASSIGNMENT_MAXIMUM;
                        return f_data;
                    }
                    UngetC(c);
                    f_data.f_type = NODE_MAXIMUM;
                    return f_data;
                }
            }
            UngetC(c);
            f_data.f_type = NODE_CONDITIONAL;
            return f_data;

        case '&':
            c = GetC();
            if(c == '=') {
                f_data.f_type = NODE_ASSIGNMENT_BITWISE_AND;
                return f_data;
            }
            if(c == '&') {
                c = GetC();
                if(c == '=') {
                    f_data.f_type = NODE_ASSIGNMENT_LOGICAL_AND;
                    return f_data;
                }
                UngetC(c);
                f_data.f_type = NODE_LOGICAL_AND;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_BITWISE_AND;
            return f_data;

        case '^':
            c = GetC();
            if(c == '=') {
                f_data.f_type = NODE_ASSIGNMENT_BITWISE_XOR;
                return f_data;
            }
            if(c == '^') {
                c = GetC();
                if(c == '=') {
                    f_data.f_type = NODE_ASSIGNMENT_LOGICAL_XOR;
                    return f_data;
                }
                UngetC(c);
                f_data.f_type = NODE_LOGICAL_XOR;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_BITWISE_XOR;
            return f_data;

        case '|':
            c = GetC();
            if(c == '=') {
                f_data.f_type = NODE_ASSIGNMENT_BITWISE_OR;
                return f_data;
            }
            if(c == '|') {
                c = GetC();
                if(c == '=') {
                    f_data.f_type = NODE_ASSIGNMENT_LOGICAL_OR;
                    return f_data;
                }
                UngetC(c);
                f_data.f_type = NODE_LOGICAL_OR;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_BITWISE_OR;
            return f_data;

        case '.':
            c = GetC();
            if(c >= '0' && c <= '9') {
                // this is a valid fraction
                UngetC(c);
                ReadNumber('.');
                return f_data;
            }
            if(c == '.') {
                c = GetC();
                if(c == '.') {
                    // Elipsis!
                    f_data.f_type = NODE_REST;
                    return f_data;
                }
                UngetC(c);
                // Range (not too sure if this is really used yet
                // and whether it will be called RANGE)
                f_data.f_type = NODE_RANGE;
                return f_data;
            }
            UngetC(c);
            f_data.f_type = NODE_MEMBER;
            return f_data;

        case '[':
            f_data.f_type = NODE_OPEN_SQUARE_BRACKET;
            return f_data;

        case ']':
            f_data.f_type = NODE_CLOSE_SQUARE_BRACKET;
            return f_data;

        case '{':
            f_data.f_type = NODE_OPEN_CURVLY_BRACKET;
            return f_data;

        case '}':
            f_data.f_type = NODE_CLOSE_CURVLY_BRACKET;
            return f_data;

        case '(':
            f_data.f_type = NODE_OPEN_PARENTHESIS;
            return f_data;

        case ')':
            f_data.f_type = NODE_CLOSE_PARENTHESIS;
            return f_data;

        case ';':
            f_data.f_type = NODE_SEMICOLON;
            return f_data;

        case ',':
            f_data.f_type = NODE_COMMA;
            return f_data;

        default:
            if(c > ' ' && c < 0x7F)
            {
                f_input->ErrMsg(AS_ERR_UNEXPECTED_PUNCTUATION, "unexpected punctuation '%c'", (char) c);
            }
            else
            {
                f_input->ErrMsg(AS_ERR_UNEXPECTED_PUNCTUATION, "unexpected punctuation '\\U%08lX'", c);
            }
            break;

        }
    }
}



}
// namespace as2js

// vim: ts=4 sw=4 et
