#ifndef AS2JS_DB_H
#define AS2JS_DB_H
/* db.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


namespace as2js
{

// The database uses a JSON object defined as:
//
// {
//   "<package_name>": {
//     "<element name>": {
//       "type": <type>,
//       "filename": <filename>,
//       "line": <line>
//     },
//     // ... repeat for each element ...
//   },
//   // ... repeat for each package ...
// };
//
// The old database was one line per element as in:
// <package name> <element name> <type> <filename> <line>


class Database
{
public:
    typedef std::shared_ptr<Database>   pointer_t;

    class Element
    {
    public:
        typedef std::shared_ptr<Element>      pointer_t;

                                    Element(String const& element_name, JSON::JSONValue::pointer_t element);

        void                        set_type(String const& type);
        void                        set_filename(String const& filename);
        void                        set_line(Position::counter_t line);

        String                      get_element_name() const;
        String                      get_type() const;
        String                      get_filename() const;
        Position::counter_t         get_line() const;

    private:
        String const                f_element_name;
        String                      f_type;
        String                      f_filename;
        Position::counter_t         f_line = Position::DEFAULT_COUNTER;

        JSON::JSONValue::pointer_t  f_element;
    };
    typedef std::map<String, Element::pointer_t>    element_map_t;
    typedef std::vector<Element::pointer_t>         element_vector_t;

    class Package
    {
    public:
        typedef std::shared_ptr<Package>        pointer_t;

                                    Package(String const& package_name, JSON::JSONValue::pointer_t package);

        String                      get_package_name() const;

        element_vector_t            find_elements(String const& pattern) const;
        Element::pointer_t          get_element(String const& element_name) const;
        Element::pointer_t          add_element(String const& element_name);

    private:
        String const                f_package_name;

        JSON::JSONValue::pointer_t  f_package;
        element_map_t               f_elements;
    };
    typedef std::map<String, Package::pointer_t>    package_map_t;
    typedef std::vector<Package::pointer_t>         package_vector_t;

    bool                        load(String const& filename);
    void                        save() const;

    package_vector_t            find_packages(String const& pattern) const;
    Package::pointer_t          get_package(String const& package_name) const;
    Package::pointer_t          add_package(String const& package_name);

    static bool                 match_pattern(String const& name, String const& pattern);

private:
    String                      f_filename;
    JSON::pointer_t             f_json;
    JSON::JSONValue::pointer_t  f_value; // json

    package_map_t               f_packages;
};



}
// namespace as2js
#endif
// #ifndef AS2JS_DB_H

// vim: ts=4 sw=4 et
