/* string.js -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


class String extends Object
{
    use extended_operators(2);

    function String(var in value: String := 0.0);

    static function fromCharCode(var in ... chars: Number) : String;

    function toString(Void) : String;
    function valueOf(Void) : String;

    //
    // Note: all the following functions are marked as
    //       "is intentionally generic; 'this' does not need to be an Array"
    //       at this time, we keep this here and force our users to convert
    //       their object to a String first
    //
    function charAt(var in value: Number) : String;
    function charCodeAt(var in value: Number) : Number;
    function contact(var in ... value: String) : String;
    function indexOf(var in searchString: String, var in position: Number) : Number;
    function lastIndexOf(var in searchString: String, var in position: Number) : Number;
    function localeCompare(var in that: String) : Number;
    function match(var in regexp: String) : Boolean;
    function match(var in regexp: RegExp) : Boolean;
    function replace(var in searchValue: String, var in replaceValue: String) : String;
    function replace(var in searchValue: RegExp, var in replaceValue: String) : String;
    function search(var in regexp: String) : Number;
    function search(var in regexp: RegExp) : Number;
    function slice(var in start: Number, var in end: Number) : String;
    function split(var in separator: String, var in limit: Number) : Array;
    function split(var in separator: RegExp, var in limit: Number) : Array;
    function substring(var in start: Number, var in end: Number) : String;
    function toLowerCase(Void) : String;
    function toLocaleLowerCase(Void) : String;
    function toUpperCase(Void) : String;
    function toLocaleUpperCase(Void) : String;
    function trim() : String;

    // unary operators
    function +   (Void) : Number;
    function -   (Void) : Number;
    function ++  (Void) : Number; // pre
    function --  (Void) : Number; // pre
    function ++  (var in value: Number) : Number; // post
    function --  (var in value: Number) : Number; // post
    function ~   (Void) : Number;
    function !   (Void) : Boolean;

    // binary operator
    function **  (var in value: Number) : Number;

    function ~=  (var in value: Number) : Number;
    function !~  (var in value: Number) : Number;

    function *   (var in value: Number) : Number;
    function /   (var in value: Number) : Number;
    function %   (var in value: Number) : Number;

    function +   (var in value: Number) : Number;
    function -   (var in value: Number) : Number;

    function <<  (var in value: Number) : Number;
    function >>  (var in value: Number) : Number;
    function >>> (var in value: Number) : Number;
    function <%  (var in value: Number) : Number;
    function >%  (var in value: Number) : Number;

    function <   (var in value: Number) : Boolean;
    function >   (var in value: Number) : Boolean;
    function <=  (var in value: Number) : Boolean;
    function >=  (var in value: Number) : Boolean;

    function ==  (var in value: Number) : Boolean;
    function !=  (var in value: Number) : Boolean;
    function === (var in value: Number) : Boolean;
    function !== (var in value: Number) : Boolean;
    function <=> (var in value: Number) : Object.CompareResult;
    function ~~  (var in value: Number) : Boolean;

    function &   (var in value: Number) : Number;

    function ^   (var in value: Number) : Number;

    function |   (var in value: Number) : Number;

    function &&  (var in value: Number) : Number;

    function ^^  (var in value: Number) : Number;

    function ||  (var in value: Number) : Number;

    function <?  (var in value: Number) : Number;
    function >?  (var in value: Number) : Number;

    // assignment operators
    function :=  (var in value: Number) : Number;

    function **= (var in value: Number) : Number;

    function *=  (var in value: Number) : Number;
    function /=  (var in value: Number) : Number;
    function %=  (var in value: Number) : Number;

    function +=  (var in value: Number) : Number;
    function -=  (var in value: Number) : Number;

    function <<= (var in value: Number) : Number;
    function >>= (var in value: Number) : Number;
    function >>>=(var in value: Number) : Number;
    function <%= (var in value: Number) : Number;
    function >%= (var in value: Number) : Number;

    function &=  (var in value: Number) : Number;

    function ^=  (var in value: Number) : Number;

    function |=  (var in value: Number) : Number;

    function &&= (var in value: Number) : Number;

    function ^^= (var in value: Number) : Number;

    function ||= (var in value: Number) : Number;

    function <?= (var in value: Number) : Number;
    function >?= (var in value: Number) : Number;

    var length;
};


}

// vim: ts=4 sw=4 et
