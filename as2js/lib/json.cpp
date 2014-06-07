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



namespace as2js
{


JSON::JSONValue::JSONValue(Position const &position)
    : f_type(JSON_TYPE_NULL)
    , f_position(position)
{
}


JSON::JSONValue::JSONValue(Position const &position, Int64 integer)
    : f_type(JSON_TYPE_INT64)
    , f_position(position)
    , f_integer(integer)
{
}


JSON::JSONValue::JSONValue(Position const &position, Float64 floating_point)
    : f_type(JSON_TYPE_FLOAT64)
    , f_position(position)
    , f_float(floating_point)
{
}


JSON::JSONValue::JSONValue(Position const &position, String const& string)
    : f_type(JSON_TYPE_STRING)
    , f_position(position)
    , f_string(string)
{
}


JSON::JSONValue::JSONValue(Position const &position, bool boolean)
    : f_type(boolean ? JSON_TYPE_TRUE : JSON_TYPE_FALSE)
    , f_position(position)
{
}


JSON::JSONValue::JSONValue(Position const &position, array_t const& array)
    : f_type(JSON_TYPE_ARRAY)
    , f_position(position)
    , f_array(array)
{
}


JSON::JSONValue::JSONValue(Position const &position, object_t const& object)
    : f_type(JSON_TYPE_OBJECT)
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
    return f_integer;
}


Float64 JSON::JSONValue::get_float64() const
{
    return f_float;
}


String const& JSON::JSONValue::get_string() const
{
    return f_string;
}


JSON::JSONValue::array_t const& JSON::JSONValue::get_array() const
{
    return f_array;
}


void JSON::JSONValue::set_item(size_t idx, JSONValue::pointer_t value)
{
    if(idx > f_array.size())
    {
        throw exception_internal_error("JSON::JSONValue::set_item() called with an index out of bounds");
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
    return f_object;
}


void JSON::JSONValue::set_member(String const& name, JSONValue::pointer_t value)
{
    if(name.empty())
    {
        // TBD: is that really not allowed?
        throw exception_internal_error("JSON::JSONValue::set_member() called with an empty string as the member name");
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
    case JSON_TYPE_ARRAY:
        result += "[";
        if(f_array.size() > 0)
        {
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

    case JSON_TYPE_FALSE:
        result += "false";
        break;

    case JSON_TYPE_FLOAT64:
        {
            std::stringstream value;
            value << f_float.get();
            result += value.str();
        }
        break;

    case JSON_TYPE_INT64:
        {
            std::stringstream value;
            value << f_integer.get();
            result += value.str();
        }
        break;

    case JSON_TYPE_NULL:
        result += "null";
        break;

    case JSON_TYPE_OBJECT:
        result += "{";
        if(f_object.size() > 0)
        {
            object_t::const_iterator obj(f_object.begin());
            result += obj->first;
            result += ":";
            result += obj->second->to_string(); // recursive
            for(++obj; obj != f_object.end(); ++obj)
            {
                result += ",";
                result += obj->first;
                result += ":";
                result += obj->second->to_string(); // recursive
            }
        }
        result += "}";
        break;

    case JSON_TYPE_STRING:
        result += f_string;
        break;

    case JSON_TYPE_TRUE:
        result += "true";
        break;

    case JSON_TYPE_UNKNOWN:
        throw exception_internal_error("JSON type \"Unknown\" is not value and should never be used (it should not possible to use it in an element)");

    }

    return result;
}



JSON::JSONValue::pointer_t JSON::load(String const& filename, Options::pointer_t options)
{
    Position pos;
    pos.set_filename(filename);

    // we could not find this module, try to load the it
    FileInput::pointer_t in(new FileInput());
    if(!in->open(filename))
    {
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_NOT_FOUND, pos);
        msg << "cannot open JSON file \"" << filename << "\".";
        return JSONValue::pointer_t();
    }

    return parse(in, options);
}


JSON::JSONValue::pointer_t JSON::parse(Input::pointer_t in, Options::pointer_t options)
{
    // Parse the JSON file
    // make sure it is marked as JSON
    options->set_option(Options::OPTION_JSON, 1);
    f_lexer.reset(new Lexer(in, options));
    f_value = read_json_value();

    if(!f_value)
    {
        Message msg(MESSAGE_LEVEL_FATAL, AS_ERR_CANNOT_COMPILE, in->get_position());
        msg << "could not interpret this JSON input \"" << in->get_position().get_filename() << "\".";
    }

    f_lexer.reset(); // release 'in' and 'options' pointers

    return f_value;
}


JSON::JSONValue::pointer_t JSON::read_json_value()
{
    for(;;)
    {
        Node::pointer_t n(f_lexer->get_next_token());
        if(n->get_type() == Node::node_t::NODE_EOF)
        {
            return JSONValue::pointer_t();
        }
        switch(n->get_type())
        {
        case Node::node_t::NODE_FALSE:
            return JSONValue::pointer_t(new JSONValue(f_lexer->get_input()->get_position(), false));

        case Node::node_t::NODE_FLOAT64:
            return JSONValue::pointer_t(new JSONValue(f_lexer->get_input()->get_position(), n->get_float64()));

        case Node::node_t::NODE_INT64:
            return JSONValue::pointer_t(new JSONValue(f_lexer->get_input()->get_position(), n->get_int64()));

        case Node::node_t::NODE_NULL:
            return JSONValue::pointer_t(new JSONValue(f_lexer->get_input()->get_position()));

        case Node::node_t::NODE_OPEN_CURVLY_BRACKET: // read an object
            {
                JSONValue::object_t obj;

                Position pos(f_lexer->get_input()->get_position());
                for(;;)
                {
                    n = f_lexer->get_next_token();
                    if(n->get_type() == Node::node_t::NODE_CLOSE_CURVLY_BRACKET)
                    {
                        break;
                    }
                    if(n->get_type() != Node::node_t::NODE_STRING)
                    {
                        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_STRING_EXPECTED, f_lexer->get_input()->get_position());
                        msg << "expected a string as the JSON object member name";
                        return JSONValue::pointer_t();
                    }
                    String name(n->get_string());
                    n = f_lexer->get_next_token();
                    if(n->get_type() != Node::node_t::NODE_COLON)
                    {
                        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_COLON_EXPECTED, f_lexer->get_input()->get_position());
                        msg << "expected a colon (:) as the JSON object member name and member value separator";
                        return JSONValue::pointer_t();
                    }
                    JSONValue::pointer_t value(read_json_value()); // recursive
                    if(!value)
                    {
                        // empty values mean we got an error, stop short!
                        return value;
                    }
                    if(obj.find(name) != obj.end())
                    {
                        // TBD: we should verify that JSON indeed forbids such
                        //      nonsense; because we may have it wrong
                        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_OBJECT_MEMBER_DEFINED_TWICE, f_lexer->get_input()->get_position());
                        msg << "the same object member \"" << name << "\" was defined twice, which is not allowed in JSON.";
                        // continue because the element in its is otherwise
                        // valid
                    }
                    else
                    {
                        obj[name] = value;
                    }
                }

                return JSONValue::pointer_t(new JSONValue(pos, obj));
            }
            break;

        case Node::node_t::NODE_OPEN_SQUARE_BRACKET: // read an array
            {
                JSONValue::array_t array;

                Position pos(f_lexer->get_input()->get_position());
                for(;;)
                {
                    n = f_lexer->get_next_token();
                    if(n->get_type() == Node::node_t::NODE_CLOSE_SQUARE_BRACKET)
                    {
                        break;
                    }
                    JSONValue::pointer_t value(read_json_value()); // recursive
                    if(!value)
                    {
                        // empty values mean we got an error, stop short!
                        return value;
                    }
                    array.push_back(value);
                }

                return JSONValue::pointer_t(new JSONValue(pos, array));
            }
            break;

        case Node::node_t::NODE_STRING:
            return JSONValue::pointer_t(new JSONValue(f_lexer->get_input()->get_position(), n->get_string()));

        case Node::node_t::NODE_TRUE:
            return JSONValue::pointer_t(new JSONValue(f_lexer->get_input()->get_position(), true));

        default:
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_UNEXPECTED_TOKEN, f_lexer->get_input()->get_position());
            msg << "unexpected token (" << n->get_type_name() << ") found in a JSON input stream";
            return JSONValue::pointer_t();

        }
    }
}


bool JSON::save(String const& filename, String const& header) const
{
    FileOutput::pointer_t out(new FileOutput());
    if(!out->open(filename))
    {
        Message msg(MESSAGE_LEVEL_FATAL, AS_ERR_CANNOT_COMPILE, out->get_position());
        msg << "could not open output file \"" << filename << "\".";
        return false;
    }

    return output(out, header);
}


bool JSON::output(Output::pointer_t out, String const& header) const
{
    out->write(header);
    out->write("\n");
    out->write(f_value->to_string());
    out->write("\n");

    return true;
}


JSON::JSONValue::pointer_t JSON::get_value() const
{
    return f_value;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
