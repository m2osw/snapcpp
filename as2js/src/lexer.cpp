/* lexer.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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

#include "as2js/parser.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER CREATOR  *************************************************/
/**********************************************************************/
/**********************************************************************/


Lexer::Lexer(void)
{
    f_last = 0;
    f_type = 0;
    //f_data -- auto-init
    f_unget_pos = 0;
    //f_unget[...] -- don't require initialization
    f_input = 0;
    f_options = 0;
    f_for_in = false;
}



void Lexer::SetInput(Input& input)
{
    f_input = &input;
}


void Lexer::SetOptions(Options& options)
{
    f_options = &options;
}


long Lexer::InputGetC(void)
{
    AS_ASSERT(f_input != 0);

    return f_input->GetC();
}


long Lexer::GetC(void)
{
    long        c;

    // we don't want to re-process these!
    if(f_unget_pos > 0) {
        --f_unget_pos;
        f_last = f_unget[f_unget_pos];
        f_type = CharType(f_last);
//fprintf(stderr, "(reget) ");
        return f_last;
    }

    c = InputGetC();

    f_type = CharType(c);
    if((f_type & (CHAR_LINE_TERMINATOR | CHAR_WHITE_SPACE)) != 0) {
        switch(c) {
        case '\n':
            // skip '\n\r' as one newline
            do {
                f_input->NewLine();
                c = InputGetC();
            } while(c == '\n');
            if(c != '\r') {
                UngetC(c);
            }
            c = '\n';
            break;

        case '\r':
            // skip '\r\n' as one newline (?!)
            do {
                f_input->NewLine();
                c = InputGetC();
            } while(c == '\r');
            if(c != '\n') {
                UngetC(c);
            }
            c = '\n';
            break;

        case '\f':
            // view the form feed as a new page for now...
            f_input->NewPage();
            break;

        case 0x0085:
            // ?
            break;

        case 0x2028:
            f_input->NewLine();
            break;

        case 0x2029:
            f_input->NewParagraph();
            break;

        }
    }

    return f_last = c;
}


void Lexer::UngetC(long c)
{
    AS_ASSERT(f_unget_pos < MAX_UNGET);

    f_unget[f_unget_pos] = c;
    ++f_unget_pos;
}


long Lexer::CharType(long c)
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
    //case 0x2000 ... 0x200B: -- cl doesn't like those
    case 0x2000:
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

    //case '0' ... '9': -- cl doesn't like those
    case '0':
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

    //case 'a' ... 'f': -- cl doesn't like those
    //case 'A' ... 'F':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        return CHAR_LETTER | CHAR_HEXDIGIT;

    case '_':
    case '$':
    //case 'g' ... 'z': -- cl doesn't like those (moved to default:)
    //case 'G' ... 'Z':
        return CHAR_LETTER;

    default:
        if((c >= 'g' && c <= 'z')
        || (c >= 'G' && c <= 'Z')) {
            return CHAR_LETTER;
        }
        if((c & 0x0FFFF) >= 0xFFFE) {
            return CHAR_INVALID;
        }
        if(c < 0x7F) {
            return CHAR_PUNCTUATION;
        }
        // TODO: this will be true in most cases, but not always!
        return CHAR_LETTER;

    }
    /*NOTREACHED*/
}




int64_t Lexer::ReadHex(long max)
{
    long        c, p, result;

    result = 0;
    p = 0;
    c = GetC();
    while((f_type & CHAR_HEXDIGIT) != 0 && p < max) {
        p++;
        if(c <= '9') {
            result = result * 16 + c - '0';
        }
        else {
            result = result * 16 + c - ('A' - 10);
        }
        c = GetC();
    }
    UngetC(c);

    if(p == 0) {
        f_input->ErrMsg(AS_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE, "invalid unicode (\\[xXuU]##) escape sequence)");
        return -1;
    }

    // TODO: In strict mode, should we check whether we got p == max?
    // WARNING: this is also used by the ReadNumber() function

    return result;
}


int64_t Lexer::ReadOctal(long c, long max)
{
    long        p, result;

    result = c - '0';
    p = 1;
    c = GetC();
    while(c >= '0' && c <= '7' && p < max) {
        p++;
        result = result * 8 + c - '0';
        c = GetC();
    }
    UngetC(c);

    return result;
}


long Lexer::EscapeSequence(void)
{
    long c = f_input->GetC();
    switch(c) {
    case 'u':
        // 4 hex digits
        return ReadHex(4);

    case 'U':
        // 8 hex digits
        return ReadHex(8);

    case 'x':
    case 'X':
        // 2 hex digits
        return ReadHex(2);

    case '\'':
    case '\"':
    case '\\':
        return c;

    case 'b':
        return '\b';

    case 'e':
        if(f_options != 0
        && f_options->GetOption(AS_OPTION_EXTENDED_ESCAPE_SEQUENCES) != 0) {
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

    //case '0' ... '7': -- cl doesn't like those
    default:
        if(c >= '0' && c <= '7') {
            return ReadOctal(c, 3);
        }
        break;

    }

    if(c > ' ' && c < 0x7F) {
        f_input->ErrMsg(AS_ERR_UNKNOWN_ESCAPE_SEQUENCE, "unknown escape letter '%c'", (char) c);
    }
    else {
        f_input->ErrMsg(AS_ERR_UNKNOWN_ESCAPE_SEQUENCE, "unknown escape letter '\\U%08lX'", c);
    }

    return '?';
}






long Lexer::Read(long c, long flags, String& str)
{
    bool        escape;

    do {
        escape = c == '\\';
        if(escape) {
            c = EscapeSequence();
        }
        if((f_type & CHAR_INVALID) == 0) {
            str.AppendChar(c);
        }
        c = GetC();
    } while((f_type & flags) != 0 && c >= 0);

    if(escape) {
        long l, i;
        l = c;
        i = 8;
        while(i > 0) {
            --i;
            long x = l & 15;
            if(x >= 10) {
                x += 'A' - 10;
            }
            else {
                x += '0';
            }
            UngetC(x);
            l >>= 4;
        }
        UngetC('U');
        UngetC('\\');
    }
    else {
        UngetC(c);
    }

    return c;
}



void Lexer::ReadIdentifier(long c)
{
    f_data.f_type = NODE_IDENTIFIER;
    c = Read(c, CHAR_LETTER | CHAR_DIGIT, f_data.f_str);

    // An identifier can be a keyword, we check that right here!
    long l = f_data.f_str.GetLength();
    if(l > 1) {
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


void Lexer::ReadNumber(long c)
{
    String        number;
    char        buf[256];
    size_t        sz;

    buf[sizeof(buf) - 1] = '\0';

    if(c == '.') {
        // in case the strtod() doesn't support a missing 0
        // at the start of the string
        number.AppendChar('0');
        number.AppendChar('.');
    }
    else if(c == '0') {
        c = GetC();
        if(c == 'x' || c == 'X') {
            // hexadecimal number
            f_data.f_type = NODE_INT64;
            f_data.f_int.Set(ReadHex(16));
            return;
        }
        // octal is not permitted in ECMAScript version 3+
        if(f_options != 0
        && f_options->GetOption(AS_OPTION_OCTAL) != 0
        && c >= '0' && c <= '7') {
            // octal
            f_data.f_type = NODE_INT64;
            f_data.f_int.Set(ReadOctal(c, 22));
            return;
        }
        number.AppendChar('0');
        UngetC(c);
    }
    else {
        c = Read(c, CHAR_DIGIT, number);
    }

    if(c == '.') {
        // TODO: we may want to support 32 bits floats as well
        f_data.f_type = NODE_FLOAT64;
        c = GetC();

        // TODO:
        // Here we could check to know whether this really
        // represents a decimal number or whether the decimal
        // point is a member operator. This can be very tricky.

        c = Read(c, CHAR_DIGIT, number);
        if(c == 'e' || c == 'E') {
            number.AppendChar('e');
            GetC();        // skip the 'e'
            c = GetC();    // get the character after!
            if(c == '-' || c == '+' || (c >= '0' && c <= '9')) {
                c = Read(c, CHAR_DIGIT, number);
            }
        }
        sz = sizeof(buf);
        number.ToUTF8(buf, sz);
        f_data.f_float.Set(strtod(buf, 0));
    }
    else {
        // TODO: Support 8, 16, 32 bits, unsigned thereof
        f_data.f_type = NODE_INT64;
        sz = sizeof(buf);
        number.ToUTF8(buf, sz);
        f_data.f_int.Set(strtoll(buf, 0, 10));
    }

    // TODO: Note, we could also support numbers followed by a unit.
    //     (but not too useful in Flash ActionScript at this time
    //     without us doing all the work...)
}


void Lexer::ReadString(long quote)
{
    long        c;

    f_data.f_type = NODE_STRING;

    c = GetC();
    while(c != quote) {
        if(c < 0) { 
            f_input->ErrMsg(AS_ERR_UNTERMINTED_STRING, "the last string wasn't closed before the end of the input was reached");
            return;
        }
        if((f_type & CHAR_LINE_TERMINATOR) != 0) {
            f_input->ErrMsg(AS_ERR_UNTERMINTED_STRING, "a string can't include a line terminator");
            return;
        }
        if(c == '\\') {
            c = EscapeSequence();
            // here c can be equal to quote (c == quote)
        }
        f_data.f_str.AppendChar(c);
        c = GetC();
    }
}



const Data& Lexer::GetNextToken(void)
{
    long        c;

    f_data.Clear();

    for(;;) {
        c = GetC();
        if(c < 0) {
            // we're done
            f_data.f_type = NODE_EOF;
            return f_data;
        }

        if((f_type & (CHAR_WHITE_SPACE | CHAR_LINE_TERMINATOR | CHAR_INVALID)) != 0) {
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
            if(c > ' ' && c < 0x7F) {
                f_input->ErrMsg(AS_ERR_UNEXPECTED_PUNCTUATION, "unexpected punctuation '%c'", (char) c);
            }
            else {
                f_input->ErrMsg(AS_ERR_UNEXPECTED_PUNCTUATION, "unexpected punctuation '\\U%08lX'", c);
            }
            break;

        }
    }
}


void Lexer::ErrMsg(err_code_t err_code, const char *format, ...)
{
    va_list        ap;

    va_start(ap, format);
    f_input->ErrMsg(err_code, format, ap);
    va_end(ap);
}






}
// namespace as2js

// vim: ts=4 sw=4 et
