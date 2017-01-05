/* db.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "db.h"  // 100% private header

#include    "as2js/exceptions.h"
#include    "as2js/message.h"


namespace as2js
{





Database::Element::Element(String const& element_name, JSON::JSONValue::pointer_t element)
    : f_element_name(element_name)
    //, f_type("") -- auto-init
    //, f_filename("") -- auto-init
    //, f_line(1) -- auto-init
    , f_element(element)
{
    // verify the type, but we already tested before creating this object
    JSON::JSONValue::type_t type(f_element->get_type());
    if(type != JSON::JSONValue::type_t::JSON_TYPE_OBJECT)
    {
        throw exception_internal_error("an element cannot be created with a JSON value which has a type other than Object");
    }

    // we got a valid database element object
    JSON::JSONValue::object_t const& obj(f_element->get_object());
    for(JSON::JSONValue::object_t::const_iterator it(obj.begin()); it != obj.end(); ++it)
    {
        JSON::JSONValue::type_t const sub_type(it->second->get_type());
        String const field_name(it->first);
        if(field_name == "type")
        {
            if(sub_type != JSON::JSONValue::type_t::JSON_TYPE_STRING)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, it->second->get_position());
                msg << "The type of an element in the database has to be a string.";
            }
            else
            {
                f_type = it->second->get_string();
            }
        }
        else if(field_name == "filename")
        {
            if(sub_type != JSON::JSONValue::type_t::JSON_TYPE_STRING)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, it->second->get_position());
                msg << "The filename of an element in the database has to be a string.";
            }
            else
            {
                f_filename = it->second->get_string();
            }
        }
        else if(field_name == "line")
        {
            if(sub_type != JSON::JSONValue::type_t::JSON_TYPE_INT64)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, it->second->get_position());
                msg << "The line of an element in the database has to be an integer.";
            }
            else
            {
                f_line = static_cast<Position::counter_t>(it->second->get_int64().get());
            }
        }
        // else -- TBD: should we err on unknown fields?
    }
}


void Database::Element::set_type(String const& type)
{
    f_type = type;
    f_element->set_member("type", JSON::JSONValue::pointer_t(new JSON::JSONValue(f_element->get_position(), f_type)));
}


void Database::Element::set_filename(String const& filename)
{
    f_filename = filename;
    f_element->set_member("filename", JSON::JSONValue::pointer_t(new JSON::JSONValue(f_element->get_position(), f_filename)));
}


void Database::Element::set_line(Position::counter_t line)
{
    f_line = line;
    Int64 integer(f_line);
    f_element->set_member("line", JSON::JSONValue::pointer_t(new JSON::JSONValue(f_element->get_position(), integer)));
}


String Database::Element::get_element_name() const
{
    return f_element_name;
}


String Database::Element::get_type() const
{
    return f_type;
}


String Database::Element::get_filename() const
{
    return f_filename;
}


Position::counter_t Database::Element::get_line() const
{
    return f_line;
}







Database::Package::Package(String const& package_name, JSON::JSONValue::pointer_t package)
    : f_package_name(package_name)
    , f_package(package)
{
    // verify the type, but we already tested before creatin this object
    JSON::JSONValue::type_t type(f_package->get_type());
    if(type != JSON::JSONValue::type_t::JSON_TYPE_OBJECT)
    {
        throw exception_internal_error("a package cannot be created with a JSON value which has a type other than Object");
    }

    // we got a valid database package object
    JSON::JSONValue::object_t const& obj(f_package->get_object());
    for(JSON::JSONValue::object_t::const_iterator it(obj.begin()); it != obj.end(); ++it)
    {
        // the only type of value that we expect are objects within the
        // main object; each one represents a package
        JSON::JSONValue::type_t const sub_type(it->second->get_type());
        if(sub_type != JSON::JSONValue::type_t::JSON_TYPE_OBJECT)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, it->second->get_position());
            msg << "A database is expected to be an object of object packages composed of object elements.";
        }
        else
        {
            String element_name(it->first);
            Element::pointer_t e(new Element(element_name, it->second));
            f_elements[element_name] = e;
        }
    }
}


String Database::Package::get_package_name() const
{
    return f_package_name;
}


Database::element_vector_t Database::Package::find_elements(String const& pattern) const
{
    element_vector_t found;
    for(auto it(f_elements.begin()); it != f_elements.end(); ++it)
    {
        if(match_pattern(it->first, pattern))
        {
            found.push_back(it->second);
        }
    }
    return found;
}


Database::Element::pointer_t Database::Package::get_element(String const& element_name) const
{
    auto it(f_elements.find(element_name));
    if(it == f_elements.end())
    {
        return Element::pointer_t();
    }
    // it exists
    return it->second;
}


Database::Element::pointer_t Database::Package::add_element(String const& element_name)
{
    auto e(get_element(element_name));
    if(!e)
    {
        // some default position object to attach to the new objects
        Position pos(f_package->get_position());

        JSON::JSONValue::object_t obj_element;
        JSON::JSONValue::pointer_t new_element(new JSON::JSONValue(pos, obj_element));
        e.reset(new Element(element_name, new_element));
        f_elements[element_name] = e;

        f_package->set_member(element_name, new_element);
    }
    return e;
}






bool Database::load(String const& filename)
{
    if(f_json)
    {
        // already loaded
        return f_value.operator bool ();
    }
    f_filename = filename;
    f_json.reset(new JSON);

    // test whether the file exists
    FileInput::pointer_t in(new FileInput());
    if(!in->open(filename))
    {
        // no db yet... it is okay
        Position pos;
        pos.set_filename(filename);
        JSON::JSONValue::object_t obj_database;
        f_value.reset(new JSON::JSONValue(pos, obj_database));
        f_json->set_value(f_value);
        return true;
    }

    // there is a db, load it
    f_value = f_json->parse(in);
    if(!f_value)
    {
        return false;
    }

    JSON::JSONValue::type_t type(f_value->get_type());

    // a 'null' is acceptable, it means the database is currently empty
    if(type == JSON::JSONValue::type_t::JSON_TYPE_NULL)
    {
        return true;
    }

    if(type != JSON::JSONValue::type_t::JSON_TYPE_OBJECT)
    {
        Position pos;
        pos.set_filename(filename);
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, pos);
        msg << "A database must be defined as a JSON object, or set to 'null'.";
        return false;
    }

    // we found the database object
    // typedef std::map<String, JSONValue::pointer_t> object_t;
    JSON::JSONValue::object_t const& obj(f_value->get_object());
    for(JSON::JSONValue::object_t::const_iterator it(obj.begin()); it != obj.end(); ++it)
    {
        // the only type of value that we expect are objects within the
        // main object; each one represents a package
        JSON::JSONValue::type_t sub_type(it->second->get_type());
        if(sub_type != JSON::JSONValue::type_t::JSON_TYPE_OBJECT)
        {
            Position pos;
            pos.set_filename(filename);
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_DATABASE, pos);
            msg << "A database is expected to be an object of object packages composed of elements.";
            return false;
        }

        String package_name(it->first);
        Package::pointer_t p(new Package(package_name, it->second));
        f_packages[package_name] = p;
    }

    return true;
}



void Database::save() const
{
    // if it has been loaded, save it
    if(f_json)
    {
        String const header("// Database used by the AS2JS Compiler (as2js)\n"
                            "//\n"
                            "// DO NOT EDIT UNLESS YOU KNOW WHAT YOU ARE DOING\n"
                            "// If you have a problem because of the database, just delete the file\n"
                            "// and the compiler will re-generate it.\n"
                            "//\n"
                            "// Copyright (c) 2005-2017 by Made to Order Software Corp.\n"
                            "// This file is written in UTF-8\n"
                            "// You can safely modify it with an editor supporting UTF-8\n"
                            "// The format is JSON:\n"
                            "//\n"
                            "// {\n"
                            "//   \"package_name\": {\n"
                            "//     \"element_name\": {\n"
                            "//       \"filename\": \"<full path filename>\",\n"
                            "//       \"line\": <line number>,\n"
                            "//       \"type\": \"<type name>\"\n"
                            "//     },\n"
                            "//     <...other elements...>\n"
                            "//   },\n"
                            "//   <...other packages...>\n"
                            "// }\n"
                            "//");
        f_json->save(f_filename, header);
    }
}


Database::package_vector_t Database::find_packages(String const& pattern) const
{
    package_vector_t found;
    for(auto it(f_packages.begin()); it != f_packages.end(); ++it)
    {
        if(match_pattern(it->first, pattern))
        {
            found.push_back(it->second);
        }
    }
    return found;
}


Database::Package::pointer_t Database::get_package(String const& package_name) const
{
    auto p(f_packages.find(package_name));
    if(p == f_packages.end())
    {
        return Database::Package::pointer_t();
    }
    return p->second;
}


Database::Package::pointer_t Database::add_package(String const& package_name)
{
    auto p(get_package(package_name));
    if(!p)
    {
        if(!f_json)
        {
            throw exception_internal_error("attempting to add a package to the database before the database was loaded");
        }

        // some default position object to attach to the new objects
        Position pos;
        pos.set_filename(f_filename);

        // create the database object if not there yet
        if(!f_value)
        {
            JSON::JSONValue::object_t obj_database;
            f_value.reset(new JSON::JSONValue(pos, obj_database));
            f_json->set_value(f_value);
        }

        JSON::JSONValue::object_t obj_package;
        JSON::JSONValue::pointer_t new_package(new JSON::JSONValue(pos, obj_package));
        p.reset(new Package(package_name, new_package));
        f_packages[package_name] = p;

        f_value->set_member(package_name, new_package);
    }
    return p;
}


bool Database::match_pattern(String const& name, String const& pattern)
{
    struct for_sub_function
    {
        static bool do_match(as_char_t const *name, as_char_t const *pattern)
        {
            for(; *pattern != '\0'; ++pattern, ++name)
            {
                if(*pattern == '*')
                {
                    // quick optimization, remove all the '*' if there are
                    // multiple (although that should probably be an error!)
                    do
                    {
                        ++pattern;
                    }
                    while(*pattern == '*');
                    if(*pattern == '\0')
                    {
                        return true;
                    }
                    while(*name != '\0')
                    {
                        if(do_match(name, pattern)) // recursive call
                        {
                            return true;
                        }
                        ++name;
                    }
                    return false;
                }
                if(*name != *pattern)
                {
                    return false;
                }
            }

            // end of name and pattern must match if you did not
            // end the pattern with an asterisk
            return *name == '\0';
        }
    };

    // we want to use a recursive function and use bare pointers
    // because it really simplifies the algorithm...
    return for_sub_function::do_match(name.c_str(), pattern.c_str());
}




}
// namespace as2js

// vim: ts=4 sw=4 et
