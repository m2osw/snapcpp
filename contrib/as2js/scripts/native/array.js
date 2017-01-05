/* array.js -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

class Array extends Object
{
    function Array(Void) : Void;
    function Array(var in value: Array) : Void;
    function Array(var in len: Number) : Void;
    function Array(var in ... items: Object) : Void;

    static function isArray(var in arg: Object) : Boolean;

    function toString(Void) : String;
    function toLocaleString(Void) : String;

    //
    // Note: all the following functions are marked as
    //       "is intentionally generic; 'this' does not need to be an Array"
    //       at this time, we keep this here and force our users to convert
    //       their object to a String first
    //
    function concat(var in ... items: Object) : Object;
    function join(var in separator: String) : String;
    function pop(Void) : Object;
    function push(var in ... items: Object) : Number;
    function reverse(Void) : Array;
    function shift(Void) : Object;
    function slice(var in start: Number, var in end: Number) : Array;
    function sort(var in comparefn: function(var in j: Number, var in k: Number) : Object.CompareResult := undefined) : Array;
    function splice(var in start: Number, var in deleteCount: Number, var in ... items: Object) : Array;
    function unshift(var in ... items: Object) : Number;
    function indexOf(var in searchElement: Object, var in fromIndex: Number := undefined) : Number;
    function lastIndexOf(var in searchElement: Object, var in fromIndex: Number := undefined) : Number;
    function every(var in callbackfn: function(var in element: Object, var in index: Number, var in obj: Array) : Boolean, var in thisArg: Object := undefined) : Boolean;
    function some(var in callbackfn: function(var in element: Object, var in index: Number, var in obj: Array) : Boolean, var in thisArg: Object := undefined) : Boolean;
    function forEach(var in callbackfn: function(var in element: Object, var in index: Number, var in obj: Array) : Boolean, var in thisArg: Object := undefined) : Void;
    function map(var in callbackfn: function(var in element: Object, var in index: Number, var in obj: Array) : Boolean, var in thisArg: Object := undefined) : Array;
    function filter(var in callbackfn: function(var in element: Object, var in index: Number, var in obj: Array) : Boolean, var in thisArg: Object := undefined) : Array;
    function reduce(var in callbackfn: function(var in previousValue: Object, var in currentValue: Object, var in currentIndex: Number, var in obj: Array) : Void, initialValue: Object := undefined) : Object;
    function reduceRight(var in callbackfn: function(var in previousValue: Object, var in currentValue: Object, var in currentIndex: Number, var in obj: Array) : Void, initialValue: Object := undefined) : Object;

    var length;
};


}

// vim: ts=4 sw=4 et
