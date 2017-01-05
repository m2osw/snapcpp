/* object.js -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

// TBD: what would be the best place to define such things?
enum CompareResult
{
    LESS := -1,
    EQUAL := 0,
    GREATER := 1
};


class Object
{
    function Object();
    function Object(var in value: Object);

    static function getPrototypeOf(var in O: Object) : Object;
    static function getOwnPropertyDescriptor(var in O: Object, var in P: String) : Object;
    static function getOwnPropertyNames(var in O: Object) : Array;
    static function create(var in O: Object, var in ... Properties) : Object;
    static function defineProperty(var in O: Object, var in P: String, var in Attributes: Object) : Object;
    static function defineProperties(var in O: Object, var in Properties: Object) : Object;
    static function seal(var in O: Object) : Object;
    static function freeze(var in O: Object) : Object;
    static function preventExtensions(var in O: Object) : Object;
    static function isSealed(var in O: Object) : Boolean;
    static function isFrozen(var in O: Object) : Boolean;
    static function isExtensible(var in O: Object) : Boolean;
    static function keys(var in O: Object) : Array;

    function toString(Void) : String;
    function toLocaleString(Void) : String;
    function valueOf(Void) : String;
    function hasOwnProperty(var in V: String) : Boolean;
    function isPrototypeOf(var in V: Object) : Boolean;
    function propertyIsEnumerable(var in V: String) : Object;

    var length;
};


}

// vim: ts=4 sw=4 et
