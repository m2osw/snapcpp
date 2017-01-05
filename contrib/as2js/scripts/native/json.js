/* json.js -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

native package Native
{


class JSON extends Object
{
    // the JSON object cannot be instantiated
    private function JSON(var in json: JSON := undefined) : JSON;

    static function parse(var in text: String, var in reviver: function(var in element: Object) : Object) : Object;
    static function stringify(var in value: Object, var in replacer: Array := undefined, var in space: String := undefined) : String;
    static function stringify(var in value: Object, var in replacer: Array := undefined, var in space: Number := undefined) : String;
    static function stringify(var in value: Object, var in replacer: function(var in key: String, var in value: Object) : Object := undefined, var in space: String := undefined) : String;
    static function stringify(var in value: Object, var in replacer: function(var in key: String, var in value: Object) : Object := undefined, var in space: Number := undefined) : String;
};


}

// vim: ts=4 sw=4 et
