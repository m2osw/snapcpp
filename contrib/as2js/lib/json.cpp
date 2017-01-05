/* json.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/json.h"

#include    "as2js/exceptions.h"
#include    "as2js/message.h"

#include    <iomanip>


/** \file
 * \brief Implementation of the JSON reader and writer.
 *
 * This file includes the implementation of the as2js::JSON class.
 *
 * The parser makes use of the lexer and an input stream.
 *
 * The writer makes use of an output stream.
 *
 * Note that our JSON parser supports the following extensions that
 * are NOT part of a valid JSON file:
 *
 * \li C-like comments using the standard slash (/) asterisk (*) to
 *     start the comment and asterisk (*) slash (/) to end it.
 * \li C++-like comments using the standard double slash (/) and ending
 *     the line with a newline character.
 * \li The NaN special value.
 * \li The +Infinity value.
 * \li The -Infinity value.
 * \li The +<number> value.
 * \li Decimal numbers are read as decimal numbers and not floating point
 *     numbers. We support full 64 bit integers.
 * \li Strings using single quote (') characters.
 * \li Strings can include \U######## characters (large Unicode, 8 digits.)
 *
 * Note that all comments are discarded while reading a JSON file.
 *
 * The writer, however, generates:
 *
 * \li Strings using double quotes (").
 * \li Only uses the small unicode \u#### encoding. Large Unicode characters
 *     are output as is (in the format used by your output stream.)
 * \li Does not output any comments (although you may include a comment in
 *     the header parameter.)
 *
 * However, it will:
 *
 * \li Generate integers that are 64 bit.
 * \li Output NaN for undefined numbers.
 * \li Output Infinity and -Infinity for number representing infinity.
 *
 * We may later introduce a flag to allow / disallow these values.
 */


namespace as2js
{

/** \brief Private implementation functions.
 *
 * Our JSON implementation makes use of functions that are defined in
 * this unnamed namespace.
 */
namespace
{

/** \brief Append a raw string to a stringified string.
 *
 * This function appends a string (str) to a stringified
 * string (result). In the process, it adds quotes to the
 * resulting string.
 *
 * \param[in,out] result  Where the output string is appended with a
 *                        valid string for a JSON file.
 * \param[in] str  The raw input string which needs to be stringified.
 */
void append_string(String& result, String const& str)
{
    result += '"';
    size_t const max_chars(str.length());
    for(size_t idx(0); idx < max_chars; ++idx)
    {
        switch(str[idx])
        {
        case '\b':
            result += '\\';
            result += 'b';
            break;

        case '\f':
            result += '\\';
            result += 'f';
            break;

        case '\n':
            result += '\\';
            result += 'n';
            break;

        case '\r':
            result += '\\';
            result += 'r';
            break;

        case '\t':
            result += '\\';
            result += 't';
            break;

        case '"':
            result += '\\';
            result += '"';
            break;

        // Escaping a single quote (') is not valid JSON
        //case '\'':
        //    result += '\\';
        //    result += '\'';
        //    break;

        default:
            if(str[idx] < 0x0020)
            {
                // other controls must be escaped using Unicode
                std::stringstream ss;
                ss << std::hex << "\\u" << std::setfill('0') << std::setw(4) << static_cast<int>(str[idx]);
                result += ss.str().c_str();
            }
            else
            {
                result += str[idx];
            }
            break;

        }
    }
    result += '"';
}

}
// no name namespace


/** \brief Initialize a JSONValue saving_t object.
 *
 * This function initializes a JSONValue saving_t object attached to
 * the specified JSONValue \p value.
 *
 * While saving we cannot know whether the JSON is currently cyclical
 * or not. We use this saving_t object to mark all the objects being
 * saved with a flag. If the flag is already set, this constructor
 * fails with an exception.
 *
 * To avoid cyclical JSON trees, make sure to always allocate any
 * new value that you add to your tree.
 *
 * \exception exception_cyclical_structure
 * This exception is raised if the JSONValue about to be saved is
 * already marked as being saved, meaning that a child of JSONValue
 * points back to this JSONValue.
 *
 * \param[in] value  The value being marked as being saved.
 */
JSON::JSONValue::saving_t::saving_t(JSONValue const& value)
    : f_value(const_cast<JSONValue&>(value))
{
    if(f_value.f_saving)
    {
        throw exception_cyclical_structure("JSON cannot stringify a set of objects and arrays which are cyclical");
    }
    f_value.f_saving = true;
}


/** \brief Destroy a JSONValue saving_t object.
 *
 * The destructor of a JSONValue marks the attached JSONValue object
 * as saved. In other words, it allows it to be saved again.
 *
 * It is used to make sure that the same JSON tree can be saved
 * multiple times.
 *
 * Note that since this happens once the value is saved, if it
 * appears multiple times in the tree, but is not cyclical, the
 * save will work.
 */
JSON::JSONValue::saving_t::~saving_t()
{
    f_value.f_saving = false;
}




/** \brief Initialize a JSONValue object.
 *
 * The NULL constructor only accepts a position and it marks this JSON
 * value as a NULL value.
 *
 * The type of this JSONValue will be set to JSON_TYPE_NULL.
 *
 * \param[in] position  The position where this JSONValue was read from.
 */
JSON::JSONValue::JSONValue(Position const &position)
    : f_type(type_t::JSON_TYPE_NULL)
    , f_position(position)
{
}


/** \brief Initialize a JSONValue object.
 *
 * The integer constructor accepts a position and an integer. It creates
 * an integer JSONValue object.
 *
 * The value cannot be modified, however, it can be retrieved using the
 * get_int64() function.
 *
 * The type of this JSONValue will be set to JSON_TYPE_INT64.
 *
 * \param[in] position  The position where this JSONValue was read from.
 * \param[in] integer  The integer to save in this JSONValue.
 */
JSON::JSONValue::JSONValue(Position const &position, Int64 integer)
    : f_type(type_t::JSON_TYPE_INT64)
    , f_position(position)
    , f_integer(integer)
{
}


/** \brief Initialize a JSONValue object.
 *
 * The floating point constructor accepts a position and a floating point
 * number.
 *
 * The value cannot be modified, however, it can be retrieved using
 * the get_float64() function.
 *
 * The type of this JSON will be JSON_TYPE_FLOAT64.
 *
 * \param[in] position  The position where this JSONValue was read from.
 * \param[in] floating_point  The floating point to save in the JSONValue.
 */
JSON::JSONValue::JSONValue(Position const &position, Float64 floating_point)
    : f_type(type_t::JSON_TYPE_FLOAT64)
    , f_position(position)
    , f_float(floating_point)
{
}


/** \brief Initialize a JSONValue object.
 *
 * The string constructor accepts a position and a string parameter.
 *
 * The value cannot be modified, however, it can be retrieved using
 * the get_string() function.
 *
 * The type of this JSONValue will be set to JSON_TYPE_STRING.
 *
 * \param[in] position  The position where this JSONValue was read from.
 * \param[in] string  The string to save in this JSONValue object.
 */
JSON::JSONValue::JSONValue(Position const &position, String const& string)
    : f_type(type_t::JSON_TYPE_STRING)
    , f_position(position)
    , f_string(string)
{
}


/** \brief Initialize a JSONValue object.
 *
 * The Boolean constructor accepts a position and a Boolean value: true
 * or false.
 *
 * The value cannot be modified, however, it can be tested using
 * the get_type() function and check the type of object.
 *
 * The type of this JSONValue will be set to JSON_TYPE_TRUE when the
 * \p boolean parameter is true, and to JSON_TYPE_FALSE when the
 * \p boolean parameter is false.
 *
 * \param[in] position  The position where this JSONValue was read from.
 * \param[in] boolean  The boolean value to save in this JSONValue object.
 */
JSON::JSONValue::JSONValue(Position const &position, bool boolean)
    : f_type(boolean ? type_t::JSON_TYPE_TRUE : type_t::JSON_TYPE_FALSE)
    , f_position(position)
{
}


/** \brief Initialize a JSONValue object.
 *
 * The array constructor accepts a position and an array as parameters.
 *
 * The array in this JSONValue can be modified using the set_item()
 * function. Also, it can be retrieved using the get_array() function.
 *
 * The type of this JSONValue will be set to JSON_TYPE_ARRAY.
 *
 * \param[in] position  The position where this JSONValue was read from.
 * \param[in] array  The array value to save in this JSONValue object.
 */
JSON::JSONValue::JSONValue(Position const &position, array_t const& array)
    : f_type(type_t::JSON_TYPE_ARRAY)
    , f_position(position)
    , f_array(array)
{
}


/** \brief Initialize a JSONValue object.
 *
 * The object constructor accepts a position and an object.
 *
 * The object in this JSONValue can be modified using the set_member()
 * function. Also, it can be retrieved using the get_object() function.
 *
 * The type of this JSONValue will be set to JSON_TYPE_OBJECT.
 *
 * \param[in] position  The position where this JSONValue was read from.
 * \param[in] object  The object value to save in this JSONValue object.
 */
JSON::JSONValue::JSONValue(Position const &position, object_t const& object)
    : f_type(type_t::JSON_TYPE_OBJECT)
    , f_position(position)
    , f_object(object)
{
}


/** \brief Retrieve the type of this JSONValue object.
 *
 * The type of a JSONValue cannot be modified. This value is read-only.
 *
 * The type determines what get_...() and what set_...() (if any)
 * functions can be called against this JSONValue object. If an invalid
 * function is called, then an exception is raised. To know which functions
 * are valid for this object, you need to check out its type.
 *
 * Note that the Boolean JSONValue objects do not have any getter or
 * setter functions. Their type defines their value: JSON_TYPE_TRUE and
 * JSON_TYPE_FALSE.
 *
 * The type is one of:
 *
 * \li JSON_TYPE_ARRAY
 * \li JSON_TYPE_FALSE
 * \li JSON_TYPE_FLOAT64
 * \li JSON_TYPE_INT64
 * \li JSON_TYPE_NULL
 * \li JSON_TYPE_OBJECT
 * \li JSON_TYPE_STRING
 * \li JSON_TYPE_TRUE
 *
 * A JSONValue cannot have the special type JSON_TYPE_UNKNOWN.
 *
 * \return The type of this JSONValue.
 */
JSON::JSONValue::type_t JSON::JSONValue::get_type() const
{
    return f_type;
}


/** \brief Get the integer.
 *
 * This function is used to retrieve the integer from a JSON_TYPE_INT64
 * JSONValue.
 *
 * It is not possible to change the integer value directly. Instead you
 * have to create a new JSONValue with the new value and replace this
 * object with the new one.
 *
 * \exception exception_internal_error
 * This exception is raised if the JSONValue object is not of type
 * JSON_TYPE_INT64.
 *
 * \return An Int64 object.
 */
Int64 JSON::JSONValue::get_int64() const
{
    if(f_type != type_t::JSON_TYPE_INT64)
    {
        throw exception_internal_error("get_int64() called with a non-int64 value type");
    }
    return f_integer;
}


/** \brief Get the floating point.
 *
 * This function is used to retrieve the floating point from a
 * JSON_TYPE_FLOAT64 JSONValue.
 *
 * It is not possible to change the floating point value directly. Instead
 * you have to create a new JSONValue with the new value and replace this
 * object with the new one.
 *
 * \exception exception_internal_error
 * This exception is raised if the JSONValue object is not of type
 * JSON_TYPE_FLOAT64.
 *
 * \return A Float64 object.
 */
Float64 JSON::JSONValue::get_float64() const
{
    if(f_type != type_t::JSON_TYPE_FLOAT64)
    {
        throw exception_internal_error("get_float64() called with a non-float64 value type");
    }
    return f_float;
}


/** \brief Get the string.
 *
 * This function lets you retrieve the string of a JSON_TYPE_STRING object.
 *
 * \exception exception_internal_error
 * This exception is raised if the JSONValue object is not of type
 * JSON_TYPE_STRING.
 *
 * \return The string of this JSONValue object.
 */
String const& JSON::JSONValue::get_string() const
{
    if(f_type != type_t::JSON_TYPE_STRING)
    {
        throw exception_internal_error("get_string() called with a non-string value type");
    }
    return f_string;
}


/** \brief Get a reference to this JSONValue array.
 *
 * This function is used to retrieve a read-only reference to the array
 * of a JSON_TYPE_ARRAY JSONValue.
 *
 * You may change the array using the set_item() function. Note that if
 * you did not make a copy of the array returned by this function, you
 * will see the changes. It also means that iterators are likely not
 * going to work once a call to set_item() was made.
 *
 * \exception exception_internal_error
 * This exception is raised if the JSONValue object is not of type
 * JSON_TYPE_ARRAY.
 *
 * \return A constant reference to a JSONValue array_t object.
 */
JSON::JSONValue::array_t const& JSON::JSONValue::get_array() const
{
    if(f_type != type_t::JSON_TYPE_ARRAY)
    {
        throw exception_internal_error("get_array() called with a non-array value type");
    }
    return f_array;
}


/** \brief Change the value of an array item.
 *
 * This function is used to change the value of an array item. The index
 * (\p idx) defines the position of the item to change. The \p value is
 * the new value to save at that position.
 *
 * Note that the pointer to the value cannot be set to NULL.
 *
 * The index (\p idx) can be any value between 0 and the current size of
 * the array. If idx is larger, then an exception is raised.
 *
 * When \p idx is set to the current size of the array, the \p value is
 * pushed at the end of the array (i.e. a new item is added to the existing
 * array.)
 *
 * \exception exception_internal_error
 * If the JSONValue is not of type JSON_TYPE_ARRAY, then this function
 * raises this exception.
 *
 * \exception exception_index_out_of_range
 * If idx is out of range (larger than the array size) then this exception
 * is raised. Note that idx is unsigned so it cannot be negative.
 *
 * \exception exception_invalid_data
 * If the value pointer is a NULL pointer, then this exception is raised.
 *
 * \param[in] idx  The index where \p value is to be saved.
 * \param[in] value  The new value to be saved.
 */
void JSON::JSONValue::set_item(size_t idx, JSONValue::pointer_t value)
{
    if(f_type != type_t::JSON_TYPE_ARRAY)
    {
        throw exception_internal_error("set_item() called with a non-array value type");
    }
    if(idx > f_array.size())
    {
        throw exception_index_out_of_range("JSON::JSONValue::set_item() called with an index out of bounds");
    }
    if(!value)
    {
        throw exception_invalid_data("JSON::JSONValue::set_item() called with a null pointer as the value");
    }
    if(idx == f_array.size())
    {
        // append value
        f_array.push_back(value);
    }
    else
    {
        // replace previous value
        f_array[idx] = value;
    }
}


/** \brief Get a reference to this JSONValue object.
 *
 * This function is used to retrieve a read-only reference to the object
 * of a JSON_TYPE_OBJECT JSONValue.
 *
 * You may change the object using the set_member() function. Note that
 * if you did not make a copy of the object returned by this function,
 * you will see the changes. It also means that iterators are likely not
 * going to work once a call to set_member() was made.
 *
 * \exception exception_internal_error
 * This exception is raised if the JSONValue object is not of type
 * JSON_TYPE_OBJECT.
 *
 * \return A constant reference to a JSONValue object_t object.
 */
JSON::JSONValue::object_t const& JSON::JSONValue::get_object() const
{
    if(f_type != type_t::JSON_TYPE_OBJECT)
    {
        throw exception_internal_error("get_object() called with a non-object value type");
    }
    return f_object;
}


/** \brief Change the value of an object member.
 *
 * This function is used to change the value of an object member. The
 * \p name defines the member to change. The \p value is the new value
 * to save along that name.
 *
 * The \p name can be any string except the empty string. If name is set
 * to the empty string, then an exception is raised.
 *
 * If a member with the same name already exists, it gets overwritten
 * with this new value. If the name is new, then the object is modified
 * which may affect your copy of the object, if you have one.
 *
 * In order to remove an object member, set it to a null pointer:
 *
 * \code
 *      set_member("clear_this", as2js::JSON::JSONValue::pointer_t());
 * \endcode
 *
 * \exception exception_internal_error
 * If the JSONValue is not of type JSON_TYPE_OBJECT, then this function
 * raises this exception.
 *
 * \exception exception_invalid_index
 * If name is the empty string then this exception is raised.
 *
 * \exception exception_invalid_data
 * If the value pointer is a NULL pointer, then this exception is raised.
 *
 * \param[in] name  The name of the object field.
 * \param[in] value  The new value to be saved.
 */
void JSON::JSONValue::set_member(String const& name, JSONValue::pointer_t value)
{
    if(f_type != type_t::JSON_TYPE_OBJECT)
    {
        throw exception_internal_error("set_member() called with a non-object value type");
    }
    if(name.empty())
    {
        // TBD: is that really not allowed?
        throw exception_invalid_index("JSON::JSONValue::set_member() called with an empty string as the member name");
    }

    // this one is easy enough
    if(value)
    {
        // add/replace
        f_object[name] = value;
    }
    else
    {
        // remove
        f_object.erase(name);
    }
}


/** \brief Get a constant reference to the JSONValue position.
 *
 * This function returns a constant reference to the JSONValue position.
 *
 * This position object is specific to this JSONValue so each one of
 * them can have a different position.
 *
 * The position of a JSONValue cannot be modified. When creating a
 * JSONValue, the position passed in as a parameter is copied in
 * the f_position of the JSONValue.
 *
 * \return The position of the JSONValue object in the source.
 */
Position const& JSON::JSONValue::get_position() const
{
    return f_position;
}


/** \brief Get the JSONValue as a string.
 *
 * This function transforms a JSONValue object into a string.
 *
 * This is used to serialize the JSONValue and output it to a string.
 *
 * This function may raise an exception in the event the JSONValue is
 * cyclic, meaning that a child JSONValue points back at one of
 * its parent JSONValue's.
 *
 * \exception exception_internal_error
 * This exception is raised if a JSONValue object is of type
 * JSON_TYPE_UNKNOWN, which should not happen.
 *
 * \return A string representing the JSONValue.
 */
String JSON::JSONValue::to_string() const
{
    String result;

    switch(f_type)
    {
    case type_t::JSON_TYPE_ARRAY:
        result += "[";
        if(f_array.size() > 0)
        {
            saving_t s(*this);
            result += f_array[0]->to_string(); // recursive
            size_t const max_elements(f_array.size());
            for(size_t i(1); i < max_elements; ++i)
            {
                result += ",";
                result += f_array[i]->to_string(); // recursive
            }
        }
        result += "]";
        break;

    case type_t::JSON_TYPE_FALSE:
        return "false";

    case type_t::JSON_TYPE_FLOAT64:
    {
        Float64 f(f_float.get());
        if(f.is_NaN())
        {
            return "NaN";
        }
        if(f.is_positive_infinity())
        {
            return "Infinity";
        }
        if(f.is_negative_infinity())
        {
            return "-Infinity";
        }
        return std::to_string(f_float.get());
    }

    case type_t::JSON_TYPE_INT64:
        return std::to_string(f_integer.get());

    case type_t::JSON_TYPE_NULL:
        return "null";

    case type_t::JSON_TYPE_OBJECT:
        result += "{";
        if(f_object.size() > 0)
        {
            saving_t s(*this);
            object_t::const_iterator obj(f_object.begin());
            append_string(result, obj->first);
            result += ":";
            result += obj->second->to_string(); // recursive
            for(++obj; obj != f_object.end(); ++obj)
            {
                result += ",";
                append_string(result, obj->first);
                result += ":";
                result += obj->second->to_string(); // recursive
            }
        }
        result += "}";
        break;

    case type_t::JSON_TYPE_STRING:
        append_string(result, f_string);
        break;

    case type_t::JSON_TYPE_TRUE:
        return "true";

    case type_t::JSON_TYPE_UNKNOWN:
        throw exception_internal_error("JSON type \"Unknown\" is not valid and should never be used (it should not be possible to use it to create a JSONValue in the first place!)"); // LCOV_EXCL_LINE

    }

    return result;
}




/** \brief Read a JSON value.
 *
 * This function opens a FileInput stream, setups a default Position
 * and then calls parse() to parse the file in a JSON tree.
 *
 * \param[in] filename  The name of a JSON file.
 */
JSON::JSONValue::pointer_t JSON::load(String const& filename)
{
    Position pos;
    pos.set_filename(filename);

    // we could not find this module, try to load the it
    FileInput::pointer_t in(new FileInput());
    if(!in->open(filename))
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_FOUND, pos);
        msg << "cannot open JSON file \"" << filename << "\".";
        // should we throw here?
        return JSONValue::pointer_t();
    }

    return parse(in);
}


/** \brief Parse a JSON object.
 *
 * This function is used to read a JSON input stream.
 *
 * If a recoverable error occurs, the function returns with a JSONValue
 * smart pointer. If errors occur, then a message is created and sent,
 * but as much as possible of the input file is read in.
 *
 * Note that the resulting value may be a NULL pointer if too much failed.
 *
 * An empty file is not a valid JSON file. To the minimum you must have:
 *
 * \code
 * null;
 * \endcode
 *
 * \param[in] in  The input stream to be parsed.
 *
 * \return A pointer to a JSONValue tree, it may be a NULL pointer.
 */
JSON::JSONValue::pointer_t JSON::parse(Input::pointer_t in)
{
    // Parse the JSON file
    //
    // Note:
    // We do not allow external options because it does not make sense
    // (i.e. JSON is very simple and no additional options should affect
    // the lexer!)
    Options::pointer_t options(new Options);
    // Make sure it is marked as JSON (line terminators change in this case)
    options->set_option(Options::option_t::OPTION_JSON, 1);
    f_lexer.reset(new Lexer(in, options));
    f_value = read_json_value(f_lexer->get_next_token());

    if(!f_value)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_CANNOT_COMPILE, in->get_position());
        msg << "could not interpret this JSON input \"" << in->get_position().get_filename() << "\".";
        // Alexis: should we throw here?
        // Doug: YES!!!!
        throw exception_invalid_data(msg.str());
    }

    f_lexer.reset(); // release 'in' and 'options' pointers

    return f_value;
}


/** \brief Read only JSON value.
 *
 * This function transform the specified \p n node in a JSONValue object.
 *
 * The type of object is defined from the type of node we just received
 * from the lexer.
 *
 * \li NODE_FALSE -- create a false JSONValue
 * \li NODE_FLOAT64 -- create a floating point JSONValue
 * \li NODE_INT64 -- create an integer JSONValue
 * \li NODE_NULL -- create a null JSONValue
 * \li NODE_STRING -- create a string JSONValue
 * \li NODE_TRUE -- create a true JSONValue
 *
 * If the lexer returned a NODE_SUBTRACT, then we assume we are about to
 * read an integer or a floating point. We do that and then calculate the
 * opposite and save the result as a FLOAT64 or INT64 JSONValue.
 *
 * If the lexer returned a NODE_OPEN_SQUARE_BRACKET then the function
 * enters the mode used to read an array.
 *
 * If the lexer returned a NODE_OPEN_CURVLY_BRACKET then the function
 * enters the mode used to read an object.
 *
 * Note that the function is somewhat weak in regard to error handling.
 * If the input is not valid as per as2js JSON documentation, then an
 * error is emitted and the process stops early.
 *
 * \param[in] n  The node to be transform in a JSON value.
 */
JSON::JSONValue::pointer_t JSON::read_json_value(Node::pointer_t n)
{
    if(n->get_type() == Node::node_t::NODE_EOF)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_EOF, n->get_position());
        msg << "the end of the file was reached while reading JSON data.";
        return JSONValue::pointer_t();
    }
    switch(n->get_type())
    {
    case Node::node_t::NODE_ADD:
        // positive number...
        n = f_lexer->get_next_token();
        switch(n->get_type())
        {
        case Node::node_t::NODE_FLOAT64:
            return JSONValue::pointer_t(new JSONValue(n->get_position(), n->get_float64()));

        case Node::node_t::NODE_INT64:
            return JSONValue::pointer_t(new JSONValue(n->get_position(), n->get_int64()));

        default:
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_TOKEN, n->get_position());
            msg << "unexpected token (" << n->get_type_name() << ") found after a '+' sign, a number was expected.";
            return JSONValue::pointer_t();

        }
        /*NOT_REACHED*/
        break;

    case Node::node_t::NODE_FALSE:
        return JSONValue::pointer_t(new JSONValue(n->get_position(), false));

    case Node::node_t::NODE_FLOAT64:
        return JSONValue::pointer_t(new JSONValue(n->get_position(), n->get_float64()));

    case Node::node_t::NODE_INT64:
        return JSONValue::pointer_t(new JSONValue(n->get_position(), n->get_int64()));

    case Node::node_t::NODE_NULL:
        return JSONValue::pointer_t(new JSONValue(n->get_position()));

    case Node::node_t::NODE_OPEN_CURVLY_BRACKET: // read an object
        {
            JSONValue::object_t obj;

            Position pos(n->get_position());
            n = f_lexer->get_next_token();
            if(n->get_type() != Node::node_t::NODE_CLOSE_CURVLY_BRACKET)
            {
                for(;;)
                {
                    if(n->get_type() != Node::node_t::NODE_STRING)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_STRING_EXPECTED, n->get_position());
                        msg << "expected a string as the JSON object member name.";
                        return JSONValue::pointer_t();
                    }
                    String name(n->get_string());
                    n = f_lexer->get_next_token();
                    if(n->get_type() != Node::node_t::NODE_COLON)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COLON_EXPECTED, n->get_position());
                        msg << "expected a colon (:) as the JSON object member name and member value separator.";
                        return JSONValue::pointer_t();
                    }
                    // skip the colon
                    n = f_lexer->get_next_token();
                    JSONValue::pointer_t value(read_json_value(n)); // recursive
                    if(!value)
                    {
                        // empty values mean we got an error, stop short!
                        return value;
                    }
                    if(obj.find(name) != obj.end())
                    {
                        // TBD: we should verify that JSON indeed forbids such
                        //      nonsense; because we may have it wrong
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_OBJECT_MEMBER_DEFINED_TWICE, n->get_position());
                        msg << "the same object member \"" << name << "\" was defined twice, which is not allowed in JSON.";
                        // continue because (1) the existing element is valid
                        // and (2) the new element is valid
                    }
                    else
                    {
                        obj[name] = value;
                    }
                    n = f_lexer->get_next_token();
                    if(n->get_type() == Node::node_t::NODE_CLOSE_CURVLY_BRACKET)
                    {
                        break;
                    }
                    if(n->get_type() != Node::node_t::NODE_COMMA)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COMMA_EXPECTED, n->get_position());
                        msg << "expected a comma (,) to separate two JSON object members.";
                        return JSONValue::pointer_t();
                    }
                    n = f_lexer->get_next_token();
                }
            }

            return JSONValue::pointer_t(new JSONValue(pos, obj));
        }
        break;

    case Node::node_t::NODE_OPEN_SQUARE_BRACKET: // read an array
        {
            JSONValue::array_t array;

            Position pos(n->get_position());
            n = f_lexer->get_next_token();
            if(n->get_type() != Node::node_t::NODE_CLOSE_SQUARE_BRACKET)
            {
                for(;;)
                {
                    JSONValue::pointer_t value(read_json_value(n)); // recursive
                    if(!value)
                    {
                        // empty values mean we got an error, stop short!
                        return value;
                    }
                    array.push_back(value);
                    n = f_lexer->get_next_token();
                    if(n->get_type() == Node::node_t::NODE_CLOSE_SQUARE_BRACKET)
                    {
                        break;
                    }
                    if(n->get_type() != Node::node_t::NODE_COMMA)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_COMMA_EXPECTED, n->get_position());
                        msg << "expected a comma (,) to separate two JSON array items.";
                        return JSONValue::pointer_t();
                    }
                    n = f_lexer->get_next_token();
                }
            }

            return JSONValue::pointer_t(new JSONValue(pos, array));
        }
        break;

    case Node::node_t::NODE_STRING:
        return JSONValue::pointer_t(new JSONValue(n->get_position(), n->get_string()));

    case Node::node_t::NODE_SUBTRACT:
        // negative number...
        n = f_lexer->get_next_token();
        switch(n->get_type())
        {
        case Node::node_t::NODE_FLOAT64:
            {
                Float64 f(n->get_float64());
                if(!f.is_NaN())
                {
                    f.set(-f.get());
                    n->set_float64(f);
                }
                // else ... should we err about this one?
            }
            return JSONValue::pointer_t(new JSONValue(n->get_position(), n->get_float64()));

        case Node::node_t::NODE_INT64:
            {
                Int64 i(n->get_int64());
                i.set(-i.get());
                n->set_int64(i);
            }
            return JSONValue::pointer_t(new JSONValue(n->get_position(), n->get_int64()));

        default:
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_TOKEN, n->get_position());
            msg << "unexpected token (" << n->get_type_name() << ") found after a '-' sign, a number was expected.";
            return JSONValue::pointer_t();

        }
        /*NOT_REACHED*/
        break;

    case Node::node_t::NODE_TRUE:
        return JSONValue::pointer_t(new JSONValue(n->get_position(), true));

    default:
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_TOKEN, n->get_position());
        msg << "unexpected token (" << n->get_type_name() << ") found in a JSON input stream.";
        return JSONValue::pointer_t();

    }
}


/** \brief Save the JSON in the specified file.
 *
 * This function is used to save this JSON in the specified file.
 *
 * One can also specified a header, in most cases a comment that
 * gives copyright, license information and eventually some information
 * explaining what that file is about.
 *
 * \param[in] filename  The name of the file on disk.
 * \param[in] header  A header to be saved before the JSON data.
 *
 * \return true if the save() succeeded.
 *
 * \sa output()
 */
bool JSON::save(String const& filename, String const& header) const
{
    FileOutput::pointer_t out(new FileOutput());
    if(!out->open(filename))
    {
        Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_CANNOT_COMPILE, out->get_position());
        msg << "could not open output file \"" << filename << "\".";
        return false;
    }

    return output(out, header);
}


/** \brief Output this JSON to the specified output.
 *
 * This function saves this JSON to the specified output object: \p out.
 *
 * If a header is specified (i.e. \p header is not an empty string) then
 * it gets saved before any JSON data.
 *
 * The output file is made a UTF-8 text file as the function first
 * saves a BOM in the file. Note that this means you should NOT
 * save anything in that file before calling this function. You
 * may, however, write more data (a footer) on return.
 *
 * \note
 * Functions called by this function may generate the cyclic exception.
 * This happens if your JSON tree is cyclic which means that a child
 * element points back to one of its parent.
 *
 * \exception exception_invalid_data
 * This exception is raised in the event the JSON does not have
 * any data to be saved. This happens if you create a JSON object
 * and never load or parse a valid JSON or call the set_value()
 * function.
 *
 * \param[in] out  The output stream where the JSON is to be saved.
 * \param[in] header  A string representing a header. It should
 *                    be written in a C or C++ comment for the
 *                    parser to be able to re-read the file seamlessly.
 *
 * \return true if the data was successfully written to \p out.
 */
bool JSON::output(Output::pointer_t out, String const& header) const
{
    if(!f_value)
    {
        // should we instead output "null"?
        throw exception_invalid_data("this JSON has no value to output");
    }

    if( std::dynamic_pointer_cast<FileOutput>(out) )
    {
        // Only do this if we are outputting to a file!
        // start with a BOM so the file is clearly marked as being UTF-8
        //
        as2js::String bom;
        bom += String::STRING_BOM;
        out->write(bom);
    }

    if(!header.empty())
    {
        out->write(header);
        out->write("\n");
    }

    out->write(f_value->to_string());

    return true;
}


/** \brief Set the value of this JSON object.
 *
 * This function is used to define the value of this JSON object. This
 * is used whenever you create a JSON in memory and want to save it
 * on disk or send it to a client.
 *
 * \param[in] value  The JSONValue to save in this JSON object.
 */
void JSON::set_value(JSON::JSONValue::pointer_t value)
{
    f_value = value;
}


/** \brief Retrieve the value of the JSON object.
 *
 * This function returns the current value of this JSON object. This
 * is the function you need to call after a call to the load() or
 * parse() functions used to read a JSON file from an input stream.
 *
 * Note that this function gives you the root JSONValue object of
 * the JSON object. You can then read the data or modify it as
 * required. If you make changes, you may later call the save()
 * or output() functions to save the changes to a file or
 * an output stream.
 *
 * \return A pointer to the JSONValue of this JSON object.
 */
JSON::JSONValue::pointer_t JSON::get_value() const
{
    return f_value;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
