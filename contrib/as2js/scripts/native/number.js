/* number.js -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


class Number extends Object
{
    use extended_operators(2);

    function Number(var in value: Number := 0.0);

    function toString(var in base: Number := undefined) : String;
    function toLocaleString(Void) : String;
    function valueOf(Void) : Number;
    function toFixed(var in fractionDigits: Number := undefined) : String;
    function toExponential(var in fractionDigits: Number := undefined) : String;
    function toPrevision(var in precision: Number := undefined) : String;

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

    function ~=  (var in value: Number) : Boolean;
    function !~  (var in value: Number) : Boolean;

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

    // constants
    const var MIN_VALUE;
    const var MAX_VALUE;
    //const var NaN; -- NaN is 100% internal in as2js
    const var NEGATIVE_INFINITY;
    const var POSITIVE_INFINITY;
};


// snap extension
class Integer extends Number
{
    function Integer(var in value: Number := 0.0) : Void;

    // constants
    const var MIN_VALUE;
    const var MAX_VALUE;
};


// snap extension
class Double extends Number
{
    function Double(var in value: Number := 0.0) : Void;
};


}

// vim: ts=4 sw=4 et
