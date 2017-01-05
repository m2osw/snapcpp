/* math.js -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

class Math extends Object
{
    // the Math object cannot be instantiated
    private function Math(var in m: Math := undefined) : Math;

    static function abs(var in x: Number) : Number;
    static function acos(var in x: Number) : Number;
    static function asin(var in x: Number) : Number;
    static function atan(var in x: Number) : Number;
    static function atan2(var in y: Number, var in x: Number) : Number;
    static function ceil(var in x: Number) : Number;
    static function cos(var in x: Number) : Number;
    static function exp(var in x: Number) : Number;
    static function floor(var in x: Number) : Number;
    static function log(var in x: Number) : Number;
    static function max(var in ... x: Number) : Number;
    static function min(var in ... x: Number) : Number;
    static function pow(var in x: Number, var in y: Number) : Number;
    static function random() : Number;
    static function round(var in x: Number) : Number;
    static function sin(var in x: Number) : Number;
    static function sqrt(var in x: Number) : Number;
    static function tan(var in x: Number) : Number;

    const var E;
    const var LN10;
    const var LN2;
    const var LOG2E;
    const var LOG10E;
    const var PI;
    const var SQRT1_2;
    const var SQRT2;
};


}

// vim: ts=4 sw=4 et
