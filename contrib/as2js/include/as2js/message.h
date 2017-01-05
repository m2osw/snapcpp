#ifndef AS2JS_MESSAGE_H
#define AS2JS_MESSAGE_H
/* message.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "position.h"
#include    "int64.h"
#include    "float64.h"

#include    <sstream>


namespace as2js
{


enum class message_level_t
{
    MESSAGE_LEVEL_OFF,
    MESSAGE_LEVEL_FATAL,
    MESSAGE_LEVEL_ERROR,
    MESSAGE_LEVEL_WARNING,
    MESSAGE_LEVEL_INFO,
    MESSAGE_LEVEL_DEBUG,
    MESSAGE_LEVEL_TRACE
};


enum class err_code_t
{
    AS_ERR_NONE = 0,

    AS_ERR_ABSTRACT,
    AS_ERR_BAD_NUMERIC_TYPE,
    AS_ERR_BAD_PRAGMA,
    AS_ERR_CANNOT_COMPILE,
    AS_ERR_CANNOT_MATCH,
    AS_ERR_CANNOT_OVERLOAD,
    AS_ERR_CANNOT_OVERWRITE_CONST,
    AS_ERR_CASE_LABEL,
    AS_ERR_COLON_EXPECTED,
    AS_ERR_COMMA_EXPECTED,
    AS_ERR_CURVLY_BRACKETS_EXPECTED,
    AS_ERR_DEFAULT_LABEL,
    AS_ERR_DIVIDE_BY_ZERO,
    AS_ERR_DUPLICATES,
    AS_ERR_DYNAMIC,
    AS_ERR_EXPRESSION_EXPECTED,
    AS_ERR_FINAL,
    AS_ERR_IMPROPER_STATEMENT,
    AS_ERR_INACCESSIBLE_STATEMENT,
    AS_ERR_INCOMPATIBLE,
    AS_ERR_INCOMPATIBLE_PRAGMA_ARGUMENT,
    AS_ERR_INSTALLATION,
    AS_ERR_INSTANCE_EXPECTED,
    AS_ERR_INTERNAL_ERROR,
    AS_ERR_NATIVE,
    AS_ERR_INVALID_ARRAY_FUNCTION,
    AS_ERR_INVALID_ATTRIBUTES,
    AS_ERR_INVALID_CATCH,
    AS_ERR_INVALID_CLASS,
    AS_ERR_INVALID_CONDITIONAL,
    AS_ERR_INVALID_DEFINITION,
    AS_ERR_INVALID_DO,
    AS_ERR_INVALID_ENUM,
    AS_ERR_INVALID_EXPRESSION,
    AS_ERR_INVALID_FIELD,
    AS_ERR_INVALID_FIELD_NAME,
    AS_ERR_INVALID_FRAME,
    AS_ERR_INVALID_FUNCTION,
    AS_ERR_INVALID_GOTO,
    AS_ERR_INVALID_IMPORT,
    AS_ERR_INVALID_INPUT_STREAM,
    AS_ERR_INVALID_KEYWORD,
    AS_ERR_INVALID_LABEL,
    AS_ERR_INVALID_NAMESPACE,
    AS_ERR_INVALID_NODE,
    AS_ERR_INVALID_NUMBER,
    AS_ERR_INVALID_OPERATOR,
    AS_ERR_INVALID_PACKAGE_NAME,
    AS_ERR_INVALID_PARAMETERS,
    AS_ERR_INVALID_REST,
    AS_ERR_INVALID_RETURN_TYPE,
    AS_ERR_INVALID_SCOPE,
    AS_ERR_INVALID_TRY,
    AS_ERR_INVALID_TYPE,
    AS_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE,
    AS_ERR_INVALID_VARIABLE,
    AS_ERR_IO_ERROR,
    AS_ERR_LABEL_NOT_FOUND,
    AS_ERR_LOOPING_REFERENCE,
    AS_ERR_MISMATCH_FUNC_VAR,
    AS_ERR_MISSSING_VARIABLE_NAME,
    AS_ERR_NEED_CONST,
    AS_ERR_NOT_ALLOWED,
    AS_ERR_NOT_ALLOWED_IN_STRICT_MODE,
    AS_ERR_NOT_FOUND,
    AS_ERR_NOT_SUPPORTED,
    AS_ERR_OBJECT_MEMBER_DEFINED_TWICE,
    AS_ERR_PARENTHESIS_EXPECTED,
    AS_ERR_PRAGMA_FAILED,
    AS_ERR_SEMICOLON_EXPECTED,
    AS_ERR_SQUARE_BRACKETS_EXPECTED,
    AS_ERR_STRING_EXPECTED,
    AS_ERR_STATIC,
    AS_ERR_TYPE_NOT_LINKED,
    AS_ERR_UNKNOWN_ESCAPE_SEQUENCE,
    AS_ERR_UNKNOWN_OPERATOR,
    AS_ERR_UNTERMINATED_STRING,
    AS_ERR_UNEXPECTED_EOF,
    AS_ERR_UNEXPECTED_PUNCTUATION,
    AS_ERR_UNEXPECTED_TOKEN,
    AS_ERR_UNEXPECTED_DATABASE,
    AS_ERR_UNEXPECTED_RC,

    AS_ERR_max
};


class MessageCallback
{
public:
    virtual             ~MessageCallback() {}

    virtual void        output(message_level_t message_level, err_code_t error_code, Position const& pos, std::string const& message) = 0;
};


// Note: avoid copies because with such you'd get the Message two or more times
class Message : public std::stringstream
{
public:
                        Message(message_level_t message_level, err_code_t error_code, Position const& pos);
                        Message(message_level_t message_level, err_code_t error_code);
                        Message(Message const& rhs) = delete;
                        ~Message();

    Message&            operator = (Message const& rhs) = delete;

    template<typename T>
    Message&            operator << (T const& data)
                        {
                            static_cast<std::stringstream&>(*this) << data;
                            return *this;
                        }

    // internal types; you can add your own types with
    // Message& operator << (Message& os, <my-type>);
    Message&            operator << (char const *s);
    Message&            operator << (wchar_t const *s);
    Message&            operator << (std::string const& s);
    Message&            operator << (std::wstring const& s);
    Message&            operator << (String const& s);
    Message&            operator << (char const v);
    Message&            operator << (signed char const v);
    Message&            operator << (unsigned char const v);
    Message&            operator << (signed short const v);
    Message&            operator << (unsigned short const v);
    Message&            operator << (signed int const v);
    Message&            operator << (unsigned int const v);
    Message&            operator << (signed long const v);
    Message&            operator << (unsigned long const v);
    Message&            operator << (signed long long const v);
    Message&            operator << (unsigned long long const v);
    Message&            operator << (Int64 const v);
    Message&            operator << (float const v);
    Message&            operator << (double const v);
    Message&            operator << (Float64 const v);
    Message&            operator << (bool const v);

    static void         set_message_callback(MessageCallback *callback);
    static void         set_message_level(message_level_t min_level);
    static int          warning_count();
    static int          error_count();

private:
    message_level_t     f_message_level = message_level_t::MESSAGE_LEVEL_OFF;
    err_code_t          f_error_code = err_code_t::AS_ERR_NONE;
    Position            f_position;
};



}
// namespace as2js
#endif
//#ifndef AS2JS_MESSAGE_H

// vim: ts=4 sw=4 et
