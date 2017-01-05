/* date.js -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


class Date extends Object
{
    function Date(Void) : Date;
    function Date(var in value: Date) : Date;
    function Date(var in value: String) : Date;
    function Date(var in value: Number) : Date;
    function Date(var in year: Number, var in month: Number, var in date: Number := undefined, var in hours: Number := undefined, var in minutes: Number := undefined, var in seconds: Number := undefined, var in ms: Number := undefined) : Date;

    static function parse(var in string: String) : Date;
    static function UTC(var in year: Number, var in month: Number, var in date: Number := undefined, var in hours: Number := undefined, var in minutes: Number := undefined, var in seconds: Number := undefined, var in ms: Number := undefined) : Date;
    static function now() : Date;

    function toString() : String;
    function toUTCString() : String;
    function toISOString() : String;
    function toDateString() : String;
    function toTimeString() : String;
    function toLoaleString() : String;
    function toLoaleDateString() : String;
    function toLoaleTimeString() : String;
    function valueOf() : Number;
    function getTime() : Number;
    function getFullYear() : Number;
    function getUTCFullYear() : Number;
    function getMonth() : Number;
    function getUTCMonth() : Number;
    function getDate() : Number;
    function getUTCDate() : Number;
    function getDay() : Number;
    function getUTCDay() : Number;
    function getHours() : Number;
    function getUTCHours() : Number;
    function getMinutes() : Number;
    function getUTCMinutes() : Number;
    function getSeconds() : Number;
    function getUTCSeconds() : Number;
    function getMilliseconds() : Number;
    function getUTCMilliseconds() : Number;
    function getTimezoneOffset() : Number;
    function setTime(var in time: Number) : Number;
    function setMilliseconds(var in ms: Number) : Number;
    function setUTCMilliseconds(var in ms: Number) : Number;
    function setSeconds(var in sec: Number, var in ms: Number := undefined) : Number;
    function setUTCSeconds(var in sec: Number, var in ms: Number := undefined) : Number;
    function setMinutes(var in min: Number, var in sec: Number := undefined, var in ms: Number := undefined) : Number;
    function setUTCMinutes(var in min: Number, var in sec: Number := undefined, var in ms: Number := undefined) : Number;
    function setHours(var in hour: Number, var in min: Number := undefined, var in sec: Number := undefined, var in ms: Number := undefined) : Number;
    function setUTCHours(var in hour: Number, var in min: Number := undefined, var in sec: Number := undefined, var in ms: Number := undefined) : Number;
    function setDate(var in date: Number) : Number;
    function setUTCDate(var in date: Number) : Number;
    function setMonth(var in month: Number, var in date: Number := undefined) : Number;
    function setUTCMonth(var in month: Number, var in date: Number := undefined) : Number;
    function setFullYear(var in year: Number, var in month: Number := undefined, var in date: Number := undefined) : Number;
    function setUTCFullYear(var in year: Number, var in month: Number := undefined, var in date: Number := undefined) : Number;
    function toJSON(var in key: Object) : String;
};


}

// vim: ts=4 sw=4 et
