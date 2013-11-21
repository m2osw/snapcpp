// Snap Websites Servers -- retrieve a list of nodes from a QDomDocument based on an XPath
// Copyright (C) 2013  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "qdomxpath.h"


class QDomXPathImpl
{
public:
    typedef ushort  char_t;

    /** \brief Structure that holds the token information.
     *
     * This structure is used when parsing a token. By default it is
     * marked as undefined. The token can be tested with the bool
     * operator (i.e. if(token) ...) to know whether it is defined
     * (true) or undefined (false).
     */
    struct token_t
    {
        /** \brief List of tokens.
         *
         * This list of token is very large since The XML Path
         * defines a rather large number of function and other
         * names to be used to query an XML document node.
         */
        enum tok_t
        {
            TOK_UNDEFINED,
            TOK_INVALID,

            TOK_OPEN_PARENTHESIS,
            TOK_CLOSE_PARENTHESIS,
            TOK_OPEN_SQUARE_BRACKET,
            TOK_CLOSE_SQUARE_BRACKET,
            TOK_DOT,
            TOK_DOUBLE_DOT,
            TOK_AT,
            TOK_COMMA,
            TOK_COLON,
            TOK_DOUBLE_COLON,
            TOK_SLASH,
            TOK_DOUBLE_SLASH,
            TOK_PIPE,
            TOK_PLUS,
            TOK_MINUS,
            TOK_EQUAL,
            TOK_NOT_EQUAL,
            TOK_LESS_THAN,
            TOK_LESS_OR_EQUAL,
            TOK_GREATER_THAN,
            TOK_GREATER_OR_EQUAL,
            TOK_ASTERISK,
            TOK_DOLLAR,
            TOK_LITERAL,
            TOK_NUMBER,
            TOK_OPERATOR_AND,
            TOK_OPERATOR_OR,
            TOK_OPERATOR_MOD,
            TOK_OPERATOR_DIV,
            TOK_NODE_TYPE_COMMENT,
            TOK_NODE_TYPE_TEXT,
            TOK_NODE_TYPE_PROCESSING_INSTRUCTION,
            TOK_NODE_TYPE_NODE,
            TOK_AXIS_NAME_ANCESTOR,
            TOK_AXIS_NAME_ANCESTOR_OR_SELF,
            TOK_AXIS_NAME_ATTRIBUTE,
            TOK_AXIS_NAME_CHILD,
            TOK_AXIS_NAME_DESCENDANT,
            TOK_AXIS_NAME_DESCENDANT_OR_SELF,
            TOK_AXIS_NAME_FOLLOWING,
            TOK_AXIS_NAME_FOLLOWING_SIBLING,
            TOK_AXIS_NAME_FOLLOWING_SIBLING,
            TOK_AXIS_NAME_NAMESPACE,
            TOK_AXIS_NAME_PARENT,
            TOK_AXIS_NAME_PRECEDING,
            TOK_AXIS_NAME_PRECEDING_SIBLING,
            TOK_AXIS_NAME_SELF,
            TOK_NCNAME
        };

        /** \brief Initialize the token object.
         *
         * This function initializes the token object to its defaults
         * which is an undefined token.
         */
        token_t()
            : f_token = TOK_UNDEFINED;
            //, f_value("") -- auto-init
        {
        }

        /** \brief Test whether the token is defined.
         *
         * This function checks whether the token is defined. If defined,
         * it returns true.
         *
         * \return true if the token is not TOK_UNDEFINED.
         */
        operator bool ()
        {
            return f_token != TOK_UNDEFINED;
        }

        /** \brief Test whether the token is undefined.
         *
         * This function checks whether the token is undefined. If not defined,
         * it returns true.
         *
         * \return true if the token is TOK_UNDEFINED.
         */
        operator ! ()
        {
            return f_token == TOK_UNDEFINED;
        }

        /** \brief Make the token undefined.
         *
         * This function marks the token as being undefined.
         */
        void reset()
        {
            f_token = TOK_UNDEFINED;
        }

        tok_t       f_token;
        QString     f_value;
    };

    /** \brief Initialize the class.
     *
     * This function initializes the class. Once the constructor returns
     * the object parse() function can be called in order to get the
     * XPath transformed to tokens and ready to be applied against nodes.
     */
    QDomXPathImpl(const QString& xpath)
        : f_xpath(xpath)
        , f_start(f_xpath.data())
        , f_in(f_start)
    {
    }

    /** \brief Get the next character.
     *
     * This function returns the next character found in the input string.
     * If the character is invalid, the function throws an exception.
     *
     * Note that the function returns characters encoded in UTF-16, even
     * though XML expects UCS-4 characters. The main reason is because the
     * QString implementation returns those characters in this way. This
     * works because none of the characters with code values larger than
     * 0xFFFF are tested within this parser. All of those are viewed as
     * standard 'Char' and thus they can as well be defined as 0xD800 to
     * 0xDFFF byte codes.
     *
     * \return The next character in the form of an encoded UTF-16 character.
     */
    char_t getc()
    {
        char_t c(*f_in);
        if(c == '\0')
        {
            return EOF;
        }
        // Char ::= #x9
        //        | #xA
        //        | #xD
        //        | [#x20-#xD7FF]
        //        | [#xE000-#xFFFD]
        //        | [#x10000-#x10FFFF]
        // The Qt QChar is a UTF-16 character which means that
        // characters larger then 0xFFFF are defined with codes
        // between 0xD800 and 0xDFFF. These are therefore included
        // although we could check that the characters are correct
        // we do not because we do not have to test for specific
        // characters with codes that larger.)
        if(c != 0x09
        && c != 0x0A
        && c != 0x0D
        && (c < 0x20 || c > 0xFFFD))
        {
            throw QDomXPathException_InvalidCharacter(QString("invalid XML character 0x%1").arg(static_cast<int>(c), 4, 16, '0').toStdString());
        }
        ++f_in;
        return c;
    }

    /** \brief Restore the input character pointer position.
     *
     * This function can be called to restore the character pointer position
     * to a previous position. It can be called as many times as the getc()
     * function was called. However, note that you cannot specify which
     * character is being ungotten. It will always be the character that
     * you got at that time with getc().
     */
    void ungetc()
    {
        if(f_in <= f_start)
        {
            throw QDomXPathException_TooManyUnget("ungetc() called too many times, the algorithm is spurious");
        }
        --f_in;
    }

    bool get_token()
    {
        // ExprToken ::= '(' | ')'
        //             | '[' | ']'
        //             | '.'
        //             | '..'
        //             | '@'
        //             | ','
        //             | '::'
        //             | NameTest
        //             | NodeType
        //             | Operator
        //             | FunctionName
        //             | AxisName
        //             | Literal
        //             | Number
        //             | VariableReference
        //
        // Number ::= Digits ('.' Digits?)?
        //          | '.' Digits
        //
        // Digits ::= [0-9]+
        //
        // Operator ::= OperatorName
        //            | MultiplyOperator
        //            | '/'
        //            | '//'
        //            | '|'
        //            | '+'
        //            | '-'
        //            | '='
        //            | '!='
        //            | '<'
        //            | '<='
        //            | '>'
        //            | '>='
        //
        // MultiplyOperator ::= '*'
        //
        // Literal ::= '"' [^"]* '"'
        //           | "'" [^']* "'"
        //
        // NameTest ::= '*'
        //            | NCName ':' '*'
        //            | QName
        //
        // NCName ::= Name - (Char* ':' Char*)
        //
        // NameStartChar ::= ':'
        //                 | [A-Z]
        //                 | '_'
        //                 | [a-z]
        //                 | [#xC0-#xD6]
        //                 | [#xD8-#xF6]
        //                 | [#xF8-#x2FF]
        //                 | [#x370-#x37D]
        //                 | [#x37F-#x1FFF]
        //                 | [#x200C-#x200D]
        //                 | [#x2070-#x218F]
        //                 | [#x2C00-#x2FEF]
        //                 | [#x3001-#xD7FF]
        //                 | [#xF900-#xFDCF]
        //                 | [#xFDF0-#xFFFD]
        //                 | [#x10000-#xEFFFF]
        //
        // NameChar ::= NameStartChar
        //            | '-'
        //            | '.'
        //            | [0-9]
        //            | #xB7
        //            | [#x0300-#x036F]
        //            | [#x203F-#x2040]
        //
        // Name ::= NameStartChar (NameChar)*
        //
        // OperatorName ::= 'and'
        //                | 'or'
        //                | 'mod'
        //                | 'div'
        //
        // NodeType ::= 'comment'
        //            | 'text'
        //            | 'processing-instruction'
        //            | 'node'
        //
        // FunctionName ::= QName - NodeType
        //
        // AxisName ::= 'ancestor'
        //            | 'ancestor-or-self'
        //            | 'attribute'
        //            | 'child'
        //            | 'descendant'
        //            | 'descendant-or-self'
        //            | 'following'
        //            | 'following-sibling'
        //            | 'namespace'
        //            | 'parent'
        //            | 'preceding'
        //            | 'preceding-sibling'
        //            | 'self'
        //
        // VariableReference ::= '$' QName
        //
        // QName ::= PrefixedName
        //         | UnprefixedName
        //
        // PrefixedName ::= Prefix ':' LocalPart
        //
        // UnprefixedName ::= LocalPart
        //
        // Prefix ::= NCName
        //
        // LocalPart ::= NCName
        //

        // if we got an ungotten token, return it
        if(!f_unget_token)
        {
            f_last_token = f_unget_token;
            f_unget_token.reset();
            return f_last_token;
        }
        else
        {
            f_last_token.f_value = "";
            char_t c(getc());
            // ignore spaces between tokens
            while(c == 0x20 || c == 0x09 || c == 0x0D || c == 0x0A)
            {
                c = getc();
            }
            switch(c)
            {
            case EOF:
                // EOF reached, return the Undefined token
                f_last_token.reset();
                break;

            case '(':
                f_last_token.f_token = TOK_OPEN_PARENTHESIS;
                break;

            case ')':
                f_last_token.f_token = TOK_CLOSE_PARENTHESIS;
                break;

            case '[':
                f_last_token.f_token = TOK_OPEN_SQUARE_BRACKET;
                break;

            case ']':
                f_last_token.f_token = TOK_CLOSE_SQUARE_BRACKET;
                break;

            case '@':
                f_last_token.f_token = TOK_AT;
                break;

            case ',':
                f_last_token.f_token = TOK_COMMA;
                break;

            case '.':
                c = getc();
                if(c == '.')
                {
                    f_last_token.f_token = TOK_DOUBLE_DOT;
                }
                else
                {
                    ungetc();
                    f_last_token.f_token = TOK_DOT;
                }
                break;

            case ':':
                c = getc();
                if(c == ':')
                {
                    f_last_token.f_token = TOK_DOUBLE_COLON;
                }
                else
                {
                    throw QDomXPathException_InvalidCharacter("found a stand alone ':' character which is not supported at that location");
                }
                break;

            case '/':
                c = getc();
                if(c == '/')
                {
                    f_last_token.f_token = TOK_DOUBLE_SLASH;
                }
                else
                {
                    ungetc();
                    f_last_token.f_token = TOK_SLASH;
                }
                break;

            case '|':
                f_last_token.f_token = TOK_PIPE;
                break;

            case '$':
                f_last_token.f_token = TOK_DOLLAR;
                break;

            case '+':
                f_last_token.f_token = TOK_PLUS;
                break;

            case '-':
                f_last_token.f_token = TOK_MINUS;
                break;

            case '=':
                f_last_token.f_token = TOK_EQUAL;
                break;

            case '!':
                c = getc();
                if(c == '=')
                {
                    f_last_token.f_token = TOK_NOT_EQUAL;
                }
                else
                {
                    throw QDomXPathException_InvalidCharacter("found a stand alone '!' character which is not supported at that location");
                }
                break;

            case '<':
                c = getc();
                if(c == '=')
                {
                    f_last_token.f_token = TOK_LESS_OR_EQUAL;
                }
                else
                {
                    ungetc();
                    f_last_token.f_token = TOK_LESS_THAN;
                }
                break;

            case '>':
                c = getc();
                if(c == '=')
                {
                    f_last_token.f_token = TOK_GREATER_OR_EQUAL;
                }
                else
                {
                    ungetc();
                    f_last_token.f_token = TOK_GREATER_THAN;
                }
                break;

            case '*':
                // '*' can represent a NameTest or the Multiply operator
                // (this is context dependent)
                f_last_token.f_token = TOK_ASTERISK;
                break;

            case '\'':
            case '"':
                f_last_token.f_token = TOK_LITERAL;
                {
                    char_t quote(c);
                    for(;;)
                    {
                        c = getc();
                        if(c == EOF)
                        {
                            throw QDomXPathException_InvalidString("a string that was not properly closed");
                        }
                        if(c == quote)
                        {
                            break;
                        }
                        f_last_token.f_value += c;
                    }
                }
                break;

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
                f_last_token.f_token = TOK_NUMBER;
                f_last_token.f_value += c;
                for(;;)
                {
                    c = getc();
                    if(c < '0' || c > '9')
                    {
                        break;
                    }
                    f_last_token.f_value += c;
                }
                if(c != '.')
                {
                    ungetc();
                    break;
                }
            case '.':
                if(f_last_token.f_value.isEmpty())
                {
                    f_last_token.f_token = TOK_NUMBER;
                    f_last_token.f_value += '0';
                }
                f_last_token.f_value += c;
                for(;;)
                {
                    c = getc();
                    if(c < '0' || c > '9')
                    {
                        break;
                    }
                    f_last_token.f_value += c;
                }
                if(f_last_token.f_value.right(1) == ".")
                {
                    f_last_token.f_value += '0';
                }
                ungetc();
                break;

            default:
                if((c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || (c >= 0x00C0 && c <= 0x00D6)
                || (c >= 0x00D8 && c <= 0x00F6)
                || (c >= 0x00F8 && c <= 0x02FF)
                || (c >= 0x0370 && c <= 0x037D)
                || (c >= 0x037F && c <= 0x1FFF)
                || (c >= 0x200C && c <= 0x200D)
                || (c >= 0x2070 && c <= 0x218F)
                || (c >= 0x2C00 && c <= 0x2FEF)
                || (c >= 0x3001 && c <= 0xDFFF) // includes 0x10000 to 0xEFFFF
                || (c >= 0xF900 && c <= 0xFDCF)
                || (c >= 0xFDF0 && c <= 0xFFFD)
                || c == '_')
                {
                    for(;;)
                    {
                        f_last_token.f_value += c;
                        c = getc();
                        if((c >= 'a' || c <= 'z')
                        && (c >= 'A' || c <= 'Z')
                        && (c >= '0' || c <= '9')
                        && (c >= 0x00C0 || c <= 0x00D6)
                        && (c >= 0x00D8 || c <= 0x00F6)
                        && (c >= 0x00F8 || c <= 0x02FF)
                        && (c >= 0x0300 || c <= 0x037D)
                        && (c >= 0x037F || c <= 0x1FFF)
                        && (c >= 0x200C || c <= 0x200D)
                        && (c >= 0x203F || c <= 0x2040)
                        && (c >= 0x2070 || c <= 0x218F)
                        && (c >= 0x2C00 || c <= 0x2FEF)
                        && (c >= 0x3001 || c <= 0xDFFF) // includes 0x10000 to 0xEFFFF
                        && (c >= 0xF900 || c <= 0xFDCF)
                        && (c >= 0xFDF0 || c <= 0xFFFD)
                        && c != '_' && c != '.' && c != '-' && c != 0xB7)
                        {
                            ungetc();
                            break;
                        }
                    }
                    f_last_token.f_token = TOK_NCNAME;
                }
                else
                {
                    // this won't match anything and thus return and error
                    f_last_token.f_token = TOK_INVALID;
                }
                break;

            }
        }

        return f_last_token;
    }

    bool token_is_operator()
    {
        switch(f_last_token.f_token)
        {
        case TOK_NCNAME:
            if(f_last_token.f_value == "and")
            {
                f_last_token.f_token = TOK_OPERATOR_AND;
            }
            else if(f_last_token.f_value == "or")
            {
                f_last_token.f_token = TOK_OPERATOR_OR;
            }
            else if(f_last_token.f_value == "mod")
            {
                f_last_token.f_token = TOK_OPERATOR_MOD;
            }
            else if(f_last_token.f_value == "div")
            {
                f_last_token.f_token = TOK_OPERATOR_DIV;
            }
            else {
                return false;
            }
            /*FALLTHROUGH*/
         case TOK_OPERATOR_AND;
         case TOK_OPERATOR_OR;
         case TOK_OPERATOR_MOD;
         case TOK_OPERATOR_DIV;
            return true;

        }
        return false;
    }

    bool token_is_node_type()
    {
        switch(f_last_token.f_token)
        {
        case TOK_NCNAME:
            if(f_last_token.f_value == "comment")
            {
                f_last_token.f_token = TOK_NODE_TYPE_COMMENT;
            }
            else if(f_last_token.f_value == "text")
            {
                f_last_token.f_token = TOK_NODE_TYPE_TEXT;
            }
            else if(f_last_token.f_value == "processing-instruction")
            {
                f_last_token.f_token = TOK_NODE_TYPE_PROCESSING_INSTRUCTION;
            }
            else if(f_last_token.f_value == "node")
            {
                f_last_token.f_token = TOK_NODE_TYPE_NODE;
            }
            else
            {
                return false;
            }
            /*FALLTHROUGH*/
        case f_last_token.f_token = TOK_NODE_TYPE_COMMENT;
        case f_last_token.f_token = TOK_NODE_TYPE_TEXT;
        case f_last_token.f_token = TOK_NODE_TYPE_PROCESSING_INSTRUCTION;
        case f_last_token.f_token = TOK_NODE_TYPE_NODE;
            return true;

        }
        return false;
    }

    bool token_is_axis_name()
    {
        switch(f_last_token.f_token)
        {
        case TOK_NCNAME:
            // TODO: add one more level to test the first letter really fast
            if(f_last_token.f_value == "ancestor")
            {
                f_last_token.f_token = TOK_AXIS_NAME_ANCESTOR;
            }
            else if(f_last_token.f_value == "ancestor-or-self")
            {
                f_last_token.f_token = TOK_AXIS_NAME_ANCESTOR_OR_SELF;
            }
            else if(f_last_token.f_value == "attribute")
            {
                f_last_token.f_token = TOK_AXIS_NAME_ATTRIBUTE;
            }
            else if(f_last_token.f_value == "child")
            {
                f_last_token.f_token = TOK_AXIS_NAME_CHILD;
            }
            else if(f_last_token.f_value == "descendant")
            {
                f_last_token.f_token = TOK_AXIS_NAME_DESCENDANT;
            }
            else if(f_last_token.f_value == "descendant-or-self")
            {
                f_last_token.f_token = TOK_AXIS_NAME_DESCENDANT_OR_SELF;
            }
            else if(f_last_token.f_value == "following")
            {
                f_last_token.f_token = TOK_AXIS_NAME_FOLLOWING;
            }
            else if(f_last_token.f_value == "following-sibling")
            {
                f_last_token.f_token = TOK_AXIS_NAME_FOLLOWING_SIBLING;
            }
            else if(f_last_token.f_value == "following-sibling")
            {
                f_last_token.f_token = TOK_AXIS_NAME_FOLLOWING_SIBLING;
            }
            else if(f_last_token.f_value == "namespace")
            {
                f_last_token.f_token = TOK_AXIS_NAME_NAMESPACE;
            }
            else if(f_last_token.f_value == "parent")
            {
                f_last_token.f_token = TOK_AXIS_NAME_PARENT;
            }
            else if(f_last_token.f_value == "preceding")
            {
                f_last_token.f_token = TOK_AXIS_NAME_PRECEDING;
            }
            else if(f_last_token.f_value == "preceding-sibling")
            {
                f_last_token.f_token = TOK_AXIS_NAME_PRECEDING_SIBLING;
            }
            else if(f_last_token.f_value == "self")
            {
                f_last_token.f_token = TOK_AXIS_NAME_SELF;
            }
            /*FALLTHROUGH*/
        case TOK_AXIS_NAME_ANCESTOR;
        case TOK_AXIS_NAME_ANCESTOR_OR_SELF;
        case TOK_AXIS_NAME_ATTRIBUTE;
        case TOK_AXIS_NAME_CHILD;
        case TOK_AXIS_NAME_DESCENDANT;
        case TOK_AXIS_NAME_DESCENDANT_OR_SELF;
        case TOK_AXIS_NAME_FOLLOWING;
        case TOK_AXIS_NAME_FOLLOWING_SIBLING;
        case TOK_AXIS_NAME_FOLLOWING_SIBLING;
        case TOK_AXIS_NAME_NAMESPACE;
        case TOK_AXIS_NAME_PARENT;
        case TOK_AXIS_NAME_PRECEDING;
        case TOK_AXIS_NAME_PRECEDING_SIBLING;
        case TOK_AXIS_NAME_SELF;
            return true;

        }
        return false;
    }

    void                parseXPath();

private:
    const QString       f_xpath;
    const QChar *       f_start;
    const QChar *       f_in;
    token_t             f_unget_token;
    token_t             f_last_token;
};




/** \class QDomXPath
 * \brief A private class used to handle XPath expressions.
 *
 * This class parses the XPath expression and is capable of executing it
 * against a QDomNode.
 *
 * The class is based on the XPath syntax as defined by the W3C consortium:
 *
 * http://www.w3.org/TR/xpath/#section-Expressions
 *
 * In a way, this is a rewrite of the QXmlQuery, except that this
 * implementation can be used against a QDomNode and thus I can avoid all
 * the problems with the QXmlQuery (i.e. having to convert the XML back to
 * text so it can be used with QXmlQuery without crashing.)
 *
 * \note
 * All the expressions are not supported.
 *
 * The following is 'Expr' as defined on the w3c website:
 *
 * Exp ::= OrExpr
 *
 * PrimaryExpr ::= VariableReference
 *               | '(' Expr ')'
 *               | Literal
 *               | Number
 *               | FunctionCall
 *
 * FunctionCall ::= FunctionName '(' ( Argument ( ',' Argument )* )? ')'
 *
 * Argument ::= Expr
 *
 * OrExpr ::= AndExpr
 *          | OrExpr 'or' AndExpr
 *
 * AndExpr ::= EqualityExpr
 *          | AndExpr 'and' EqualityExpr
 *
 * EqualityExpr ::= RelationalExpr
 *          | EqualityExpr '=' RelationalExpr
 *          | EqualityExpr '!=' RelationalExpr
 *
 * RelationalExpr ::= AdditiveExpr
 *          | RelationalExpr '<' AdditiveExpr
 *          | RelationalExpr '>' AdditiveExpr
 *          | RelationalExpr '<=' AdditiveExpr
 *          | RelationalExpr '>=' AdditiveExpr
 *
 * AdditiveExpr ::= MultiplicativeExpr
 *          | AdditiveExpr '+' MultiplicativeExpr
 *          | AdditiveExpr '-' MultiplicativeExpr
 *
 * MultiplicativeExpr ::= UnaryExpr
 *          | MultiplicativeExpr MultiplicativeOperator UnaryExpr
 *          | MultiplicativeExpr 'div' UnaryExpr
 *          | MultiplicativeExpr 'mod' UnaryExpr
 *
 * UnaryExpr ::= UnionExpr
 *          | '-' UnaryExpr
 *
 * UnionExpr ::= PathExpr
 *             | UnionExpr '|' PathExpr
 *
 * PathExpr ::= LocationPath
 *            | FilterExpr
 *            | FilterExpr '/' RelativeLocationPath
 *            | FilterExpr '//' RelativeLocationPath
 *
 * FilterExpr ::= PrimaryExpr
 *              | FilterExpr Predicate
 *
 * LocationPath ::= RelativeLocationPath
 *                | AbsoluteLocationPath
 *
 * AbsoluteLocationPath ::= '/' RelativeLocationPath?
 *                        | AbbreviatedAbsoluteLocationPath
 *
 * RelativeLocationPath ::= Step
 *                        | RelativeLocationPath '/' Step
 *                        | AbbreviatedRelativeLocationPath
 *
 * Step ::= AxisSpecifier NodeTest Predicate*
 *        | AbbreviatedStep
 *
 * AxisSpecifier ::= AxisName '::'
 *                 | AbbreviatedAxisSpecifier
 *
 * AxisName ::= 'ancestor'
 *            | 'ancestor-or-self'
 *            | 'attribute'
 *            | 'child'
 *            | 'descendant'
 *            | 'descendant-or-self'
 *            | 'following'
 *            | 'following-sibling'
 *            | 'namespace'
 *            | 'parent'
 *            | 'preceding'
 *            | 'preceding-sibling'
 *            | 'self'
 *
 * NodeTest ::= NameTest
 *            | NodeType '(' ')'
 *            | 'processing-instruction' '(' Literal ')'
 *
 * Predicate ::= '[' PredicateExpr ']'
 *
 * PredicateExpr ::= Expr
 *
 * AbbreviatedAbsoluteLocationPath ::= '//' RelativeLocationPath
 *
 * AbbreviatedRelativeLocationPath ::= RelativeLocationPath '//' Step
 *
 * AbbreviatedStep ::= '.'
 *                   | '..'
 *
 * AbbreviatedAxisSpecifier ::= '@'?
 *
 * ExprToken ::= '(' | ')'
 *             | '[' | ']'
 *             | '.'
 *             | '..'
 *             | '@'
 *             | ','
 *             | '::'
 *             | NameTest
 *             | NodeType
 *             | Operator
 *             | FunctionName
 *             | AxisName
 *             | Literal
 *             | Number
 *             | VariableReference
 *
 * Literal ::= '"' [^"]* '"'
 *           | "'" [^']* "'"
 *
 * Number ::= Digits ('.' Digits?)?
 *          | '.' Digits
 *
 * Digits ::= [0-9]+
 *
 * Operator ::= OperatorName
 *            | MultiplyOperator
 *            | '/'
 *            | '//'
 *            | '|'
 *            | '+'
 *            | '-'
 *            | '='
 *            | '!='
 *            | '<'
 *            | '<='
 *            | '>'
 *            | '>='
 *
 * OperatorName ::= 'and'
 *                | 'or'
 *                | 'mod'
 *                | 'div'
 *
 * MultiplyOperator ::= '*'
 *
 * FunctionName ::= QName - NodeType
 *
 * VariableReference ::= '$' QName
 *
 * NameTest ::= '*'
 *            | NCName ':' '*'
 *            | QName
 *
 * NodeType ::= 'comment'
 *            | 'text'
 *            | 'processing-instruction'
 *            | 'node'
 *
 * ExprWhitespace ::= S
 *
 * NCName ::= Name - (Char* ':' Char*)
 *
 * S ::= (#x20 | #x9 | #xD | #xA)+
 *
 * Char ::= #x9
 *        | #xA
 *        | #xD
 *        | [#x20-#xD7FF]
 *        | [#xE000-#xFFFD]
 *        | [#x10000-#x10FFFF]
 *
 * NameStartChar ::= ':'
 *                 | [A-Z]
 *                 | '_'
 *                 | [a-z]
 *                 | [#xC0-#xD6]
 *                 | [#xD8-#xF6]
 *                 | [#xF8-#x2FF]
 *                 | [#x370-#x37D]
 *                 | [#x37F-#x1FFF]
 *                 | [#x200C-#x200D]
 *                 | [#x2070-#x218F]
 *                 | [#x2C00-#x2FEF]
 *                 | [#x3001-#xD7FF]
 *                 | [#xF900-#xFDCF]
 *                 | [#xFDF0-#xFFFD]
 *                 | [#x10000-#xEFFFF]
 *
 * NameChar ::= NameStartChar
 *            | '-'
 *            | '.'
 *            | [0-9]
 *            | #xB7
 *            | [#x0300-#x036F]
 *            | [#x203F-#x2040]
 *
 * Name ::= NameStartChar (NameChar)*
 *
 * Names ::= Name (#x20 Name)*
 *
 * Nmtoken ::= (NameChar)+
 *
 * Nmtokens ::= Nmtoken (#x20 Nmtoken)*
 *
 * QName ::= PrefixedName
 *         | UnprefixedName
 *
 * PrefixedName ::= Prefix ':' LocalPart
 *
 * UnprefixedName ::= LocalPart
 *
 * Prefix ::= NCName
 *
 * LocalPart ::= NCName
 */

/** \brief Set the XPath.
 *
 * This function sets the XPath of the QDomXPath object. By default, the
 * XPath is set to "." (i.e. return the current node.)
 *
 * If the XPath is considered invalid, then this function returns false
 * and the internal state is not changed. If considered valid, then the
 * new XPath takes effect and the function returns true.
 *
 * Note that if xpath is set to the empty string or ".", it is always
 * accepted and in both cases it represents the current node.
 *
 * \param[in] xpath  The new XPath to use in this QDomXPath.
 *
 * \return true if the \p xpath is valid, false otherwise.
 */
bool QDomXPath::setXPath(const QString& xpath)
{
    if(xpath.isEmpty() || xpath == ".")
    {
        f_xpath = "";
        f_impl.reset();
        return true;
    }

    QSharedPointer<QDomXPathImpl> impl(new QDomXPathImpl(xpath));
    if(!impl->parse())
    {
        return false;
    }

    f_impl.reset(impl);

    return true;
}


/** \brief Get the current xpath.
 *
 * This function returns the current XPath. If it was never set, then the
 * function returns ".". Note that if the setXPath() function returns false,
 * then the XPath doesn't get changed and thus this function returns the
 * previous XPath.
 */
QString QDomXPath::getXPath() const
{
    if(f_xpath.isEmpty())
    {
        return ".";
    }
    return f_xpath;
}


/** \brief Apply the XPath against the specified node.
 *
 * This function applies (queries) the XPath that was previously set with
 * the setXPath() function agains the input \p node parameter.
 *
 * The function returns a vector of node because it is not possible to
 * add parameters to a QDomNodeList without being within the implementation
 * (i.e. there is no function to add any node to the list.) This may be
 * because a list of nodes is dynamic, it includes a way to remove the
 * node from the list in the event the node gets deleted (just an assumption
 * of course.)
 *
 * \param[in] node  The node to query.
 *
 * \return A list of nodes (maybe empty.)
 */
QVector<QDomNode> QDomXPath::apply(QDomNode node) const
{
    if(f_impl)
    {
        return f_impl->apply(node);
    }
    QVector<QDomNode> result;
    result.push_back(node);
    return result;
}





// vim: ts=4 sw=4 et
