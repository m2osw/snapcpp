/* json.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

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


namespace as2js
{

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

        case '\'':
            result += '\\';
            result += '\'';
            break;

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


JSON::JSONValue::saving_t::saving_t(JSONValue const& value)
    : f_value(const_cast<JSONValue&>(value))
{
    if(f_value.f_saving)
    {
        throw exception_cyclical_structure("JSON cannot stringify a set of objects and arrays which are cyclical");
    }
    f_value.f_saving = true;
}


JSON::JSONValue::saving_t::~saving_t()
{
    f_value.f_saving = false;
}




JSON::JSONValue::JSONValue(Position const &position)
    : f_type(type_t::JSON_TYPE_NULL)
    , f_position(position)
{
}


JSON::JSONValue::JSONValue(Position const &position, Int64 integer)
    : f_type(type_t::JSON_TYPE_INT64)
    , f_position(position)
    , f_integer(integer)
{
}


JSON::JSONValue::JSONValue(Position const &position, Float64 floating_point)
    : f_type(type_t::JSON_TYPE_FLOAT64)
    , f_position(position)
    , f_float(floating_point)
{
}


JSON::JSONValue::JSONValue(Position const &position, String const& string)
    : f_type(type_t::JSON_TYPE_STRING)
    , f_position(position)
    , f_string(string)
{
}


JSON::JSONValue::JSONValue(Position const &position, bool boolean)
    : f_type(boolean ? type_t::JSON_TYPE_TRUE : type_t::JSON_TYPE_FALSE)
    , f_position(position)
{
}


JSON::JSONValue::JSONValue(Position const &position, array_t const& array)
    : f_type(type_t::JSON_TYPE_ARRAY)
    , f_position(position)
    , f_array(array)
{
}


JSON::JSONValue::JSONValue(Position const &position, object_t const& object)
    : f_type(type_t::JSON_TYPE_OBJECT)
    , f_position(position)
    , f_object(object)
{
}


JSON::JSONValue::type_t JSON::JSONValue::get_type() const
{
    return f_type;
}


Int64 JSON::JSONValue::get_int64() const
{
    if(f_type != type_t::JSON_TYPE_INT64)
    {
        throw exception_internal_error("get_int64() called with a non-int64 value type");
    }
    return f_integer;
}


Float64 JSON::JSONValue::get_float64() const
{
    if(f_type != type_t::JSON_TYPE_FLOAT64)
    {
        throw exception_internal_error("get_float64() called with a non-float64 value type");
    }
    return f_float;
}


String const& JSON::JSONValue::get_string() const
{
    if(f_type != type_t::JSON_TYPE_STRING)
    {
        throw exception_internal_error("get_string() called with a non-string value type");
    }
    return f_string;
}


JSON::JSONValue::array_t const& JSON::JSONValue::get_array() const
{
    if(f_type != type_t::JSON_TYPE_ARRAY)
    {
        throw exception_internal_error("get_array() called with a non-array value type");
    }
    return f_array;
}


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


JSON::JSONValue::object_t const& JSON::JSONValue::get_object() const
{
    if(f_type != type_t::JSON_TYPE_OBJECT)
    {
        throw exception_internal_error("get_object() called with a non-object value type");
    }
    return f_object;
}


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
    if(!value)
    {
        throw exception_invalid_data("JSON::JSONValue::set_member() called with a null pointer as the value");
    }

    // this one is easy enough
    f_object[name] = value;
}


Position const& JSON::JSONValue::get_position() const
{
    return f_position;
}


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
        return std::to_string(f_float.get());

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
        // should we throw here?
    }

    f_lexer.reset(); // release 'in' and 'options' pointers

    return f_value;
}


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
                f.set(-f.get());
                n->set_float64(f);
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


bool JSON::output(Output::pointer_t out, String const& header) const
{
    if(!f_value)
    {
        // should we instead output "null"?
        throw exception_invalid_data("this JSON has no value to output");
    }

    if(!header.empty())
    {
        out->write(header);
        out->write("\n");
    }
    out->write(f_value->to_string());

    return true;
}


void JSON::set_value(JSON::JSONValue::pointer_t value)
{
    f_value = value;
}


JSON::JSONValue::pointer_t JSON::get_value() const
{
    return f_value;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
