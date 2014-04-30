/** @preserve
 * Name: output
 * Version: 0.1.5.5
 * Browsers: all
 * Copyright: Copyright 2014 (c) Made to Order Software Corporation  All rights reverved.
 * Depends: jquery-extensions (1.0.1)
 * License: GPL 2.0
 */


//
// Inline "command line" parameters for the Google Closure Compiler
// See output of:
//    java -jar .../google-js-compiler/compiler.jar --help
//
// ==ClosureCompiler==
// @compilation_level ADVANCED_OPTIMIZATIONS
// @externs $CLOSURE_COMPILER/contrib/externs/jquery-1.9.js
// @externs plugins/output/externs/jquery-extensions.js
// ==/ClosureCompiler==
//

/*jslint nomen: true, todo: true, devel: true */
/*global jQuery: false, Uint8Array: true */


/** \brief Defines the snapwebsites namespace in the JavaScript environment.
 *
 * All the JavaScript functions defined by Snap! plugins are defined inside
 * the snapwebsites namespace. For example, the WYSIWYG Snap! editor is
 * defined as:
 *
 * \code
 * snapwebsites.Editor
 * \endcode
 *
 * \note
 * Technically, this is an object.
 *
 * @type {Object}
 */
var snapwebsites = {};


/** \brief Helper function for base classes that can be inherited from.
 *
 * Base classes (those that do not inherit from other classes) can call
 * function function to get initialized so it works as expected.
 *
 * See the example in the snapwebsites.inherit() function.
 *
 * WARNING: the prototype is setup with this function so you cannot
 *          use blah.prototype = { ... };.
 *
 * @param {!function(...)} base_class  The class to initialize.
 *
 * @final
 */
snapwebsites.base = function(base_class)
{
//#ifdef DEBUG
    base_class.snapwebsitesBased_ = true;
//#endif
    base_class.prototype.constructor = base_class;
};


/** \brief Helper function for inheritance support.
 *
 * This function is used by classes that want to inherit from another
 * class. At this point we do not allow multiple inheritance though.
 *
 * The following shows you how it works:
 *
 * \code
 * // class A -- constructor
 * snapwebsites.A = function()
 * {
 *      ...
 * };
 *
 * snapwebsites.base(snapwebsites.A);
 *
 * // class A -- a method
 * snapwebsites.A.prototype.someMethod = function(q)
 * {
 *      ...
 * }
 *
 * // class B -- constructor
 * snapwebsites.B = function()
 * {
 *      snapwebsites.B.superClass_.constructor.call(this);
 *      // with parameters: snapwebsites.B.superClass_.constructor.call(this, a, b, c);
 *      ...
 * };
 *
 * // class B : public A -- define inheritance
 * snapwebsites.inherits(snapwebsites.B, snapwebsites.A);
 *
 * // class B : extend with more functions
 * snapwebsites.B.prototype.moreStuff = function()
 * {
 *      ...
 * };
 *
 * // class B : override function
 * snapwebsites.B.prototype.someMethod = function(p, q)
 * {
 *      ...
 *      // call super class method (optional)
 *      snapwebsites.B.superClass_.someMethod.call(this, q);
 *      ...
 * };
 * \endcode
 *
 * @param {!function(...)} child_class  The constructor of the child
 *                                      class (the on inheriting.)
 * @param {!function(...)} super_class  The constructor of the parent
 *                                      class (the to inherit from.)
 *
 * @final
 */
snapwebsites.inherits = function(child_class, super_class) // static
{
    /** \brief Intermediate constructor.
     *
     * In case Object.create() is not available (IE8 and older) we
     * want to have a function to hold the super class prototypes
     * and be able to do a new() without parameters. This is the
     * function we use for that purpose.
     *
     * @constructor
     */
    function C() {}

//#ifdef DEBUG
    if(!super_class.snapwebsitesBased_
    && !super_class.snapwebsitesInherited_)
    {
        throw new Error("Super class was not based or inherited");
    }
//#endif

    if(Object.create)
    {
        // modern inheriting is probably faster than
        // old browsers (and at some point we will
        // have the preprocessor which will select
        // one or the other part)
        child_class.prototype = Object.create(super_class.prototype);
    }
    else
    {
        // older browsers don't have Object.create and
        // this is how you inherit properly in that case
        C.prototype = super_class.prototype;
        child_class.prototype = new C();
    }
    child_class.prototype.constructor = child_class;
    child_class.superClass_ = super_class.prototype;
    child_class.snapwebsitesInherited_ = true;
};


/** \brief Helper function: generate hexadecimal number.
 *
 * This function transform byte \p c in a hexadecimal number of
 * exactly two digits.
 *
 * Note that \p c can be larger than a byte, only it should probably
 * not be negative.
 *
 * @param {number} c  The byte to transform (expected to be between 0 and 255)
 *
 * @return {string}  The hexadecimal representation of the number.
 */
snapwebsites.charToHex = function(c) // static
{
    var a, b;

    a = c & 15;
    b = (c >> 4) & 15;
    return String.fromCharCode(b + (b >= 10 ? 55 : 48))
         + String.fromCharCode(a + (a >= 10 ? 55 : 48));
};


/** \brief Make sure a parameter is a string.
 *
 * This function makes sure the parameter is a string, if not it
 * throws.
 *
 * This is useful in situations where a function may return something
 * else than a string.
 *
 * As you can see the function doesn't do anything to the parameter,
 * only the closure compiler sees a "string" coming out.
 *
 * @param {Object|string|number} s  Expects a string as input.
 * @param {string} e  An additional error message in case it fails.
 *
 * @return {string}  The input string after making sure it is a string.
 */
snapwebsites.castToString = function(s, e) // static
{
    if(typeof s !== "string")
    {
        throw new Error("a string was expected, got a \"" + (typeof s) + "\" instead (" + e + ")");
    }
    return s;
};


/** \brief Make sure a parameter is a number.
 *
 * This function makes sure the parameter is a number, if not it
 * throws.
 *
 * This is useful in situations where a function may return something
 * else than a number.
 *
 * As you can see the function doesn't do anything to the parameter,
 * only the closure compiler sees a "number" coming out.
 *
 * @param {Object|string|number} n  Expects a number as input.
 * @param {string} e  An additional error message in case it fails.
 *
 * @return {number}  The input number after making sure it is a number.
 */
snapwebsites.castToNumber = function(n, e) // static
{
    if(typeof n !== "number")
    {
        throw new Error("a number was expected, got a \"" + (typeof n) + "\" instead (" + e + ")");
    }
    return n;
};



/** \brief A template used to define a set of buffer to MIME type scanners.
 *
 * This template class defines a function used by the Output.bufferToMIME()
 * function to determine the type of a file. The function expects a buffer
 * which is converts to a Uint8Array and sends to the
 * BufferToMIMETemplate.bufferToMIME() and overrides of that function to
 * determine the different file types supported (i.e. accepted) by the
 * system.
 *
 * The system inheritance is as follow:
 *
 * \code
 *   +---------------------------+
 *   |                           |
 *   |  BufferToMIMETemplate     |
 *   |                           |
 *   +---------------------------+
 *        ^
 *        | Inherit
 *        |
 *   +---------------------------+
 *   |                           |
 *   |  BufferToMIMESystemImage  |
 *   |                           |
 *   +---------------------------+
 * \endcode
 *
 * @return {!snapwebsites.BufferToMIMETemplate} A reference to this new object.
 *
 * @constructor
 */
snapwebsites.BufferToMIMETemplate = function()
{
    this.constructor = snapwebsites.BufferToMIMETemplate;

    return this;
};


/** \brief Mark the template as a base.
 *
 * This call marks the BufferToMIMETempate a base. This means you
 * can inherit from it.
 */
snapwebsites.base(snapwebsites.BufferToMIMETemplate);


/*jslint unparam: true */
/** \brief Check a buffer magic codes.
 *
 * This function is expected to check the different MIME types supported
 * by your plugin. The function must return a string representing a MIME
 * type. If your function does not recognize that MIME type, then return
 * an empty string.
 *
 * The template function just returns "" so you do not need to call it.
 *
 * @param {!Uint8Array} buf  The buffer of data to be checked.
 *
 * @return {!string}  The MIME type you determined, or the empty string.
 */
snapwebsites.BufferToMIMETemplate.prototype.bufferToMIME = function(buf) // virtual
{
    return "";
};
/*jslint unparam: false */



/** \brief Check for "system" images.
 *
 * This function checks for well known images. The function is generally
 * very fast because it checks only the few very well known image file
 * formats.
 *
 * @return {!snapwebsites.BufferToMIMESystemImages} A reference to this new
 *                                                  object.
 *
 * @extends {snapwebsites.BufferToMIMETemplate}
 * @constructor
 */
snapwebsites.BufferToMIMESystemImages = function()
{
    snapwebsites.BufferToMIMESystemImages.superClass_.constructor.call(this);

    this.constructor = snapwebsites.BufferToMIMESystemImages;

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and it's super.
 */
snapwebsites.inherits(snapwebsites.BufferToMIMESystemImages, snapwebsites.BufferToMIMETemplate);


/** \brief Check for most of the well known image file formats.
 *
 * This function checks for the well known image file formats that
 * we generally want the system to support. This includes:
 *
 * \li JPEG
 * \li PNG
 * \li GIF
 *
 * Other formats will be added with time:
 *
 * \li SVG
 * \li BMP/ICO
 * \li TIFF
 * \li ...
 *
 * @param {!Uint8Array} buf  The array of data to check for a known magic.
 *
 * @return {!string} The MIME type or the empty string if empty.
 *
 * @override
 */
snapwebsites.BufferToMIMESystemImages.prototype.bufferToMIME = function(buf)
{
    // Image JPEG
    if(buf[0] === 0xFF
    && buf[1] === 0xD8
    && buf[2] === 0xFF
    && buf[3] === 0xE0
    && buf[4] === 0x00
    && buf[5] === 0x10
    && buf[6] === 0x4A  // J
    && buf[7] === 0x46  // F
    && buf[8] === 0x49  // I
    && buf[9] === 0x46) // F
    {
        return "image/jpeg";
    }

    // Image PNG
    if(buf[0] === 0x89
    && buf[1] === 0x50  // P
    && buf[2] === 0x4E  // N
    && buf[3] === 0x47  // G
    && buf[4] === 0x0D  // \r
    && buf[5] === 0x0A) // \n
    {
        return "image/png";
    }

    // Image GIF
    if(buf[0] === 0x47  // G
    && buf[1] === 0x49  // I
    && buf[2] === 0x46  // F
    && buf[3] === 0x38  // 8
    && buf[4] === 0x39  // 9
    && buf[5] === 0x61) // a
    {
        return "image/gif";
    }

    return "";
};



/** \brief Snap Output Manipulations.
 *
 * This class initializes and handles the different output objects.
 *
 * \note
 * The Snap! Output is a singleton and should never be created by you. It
 * gets initialized automatically when this output.js file gets included.
 *
 * @return {!snapwebsites.Output}  This object reference.
 *
 * @constructor
 * @struct
 */
snapwebsites.Output = function()
{
    this.registeredBufferToMIME_ = [];
    this.handleMessages_();
    return this;
};


/** \brief The output is a base class even though it is unlikely derived.
 *
 * This class is marked as a base class, although it is rather unlikely
 * that we'd need to derive from it.
 */
snapwebsites.base(snapwebsites.Output);


/** \brief Holds the array of query string values if any.
 *
 * This variable member is an array of the query string values keyed on
 * their names.
 *
 * By default this parameter is undefined. It gets defined the first time
 * you call the qsParam() function.
 *
 * @type {Object}
 * @private
 */
snapwebsites.Output.prototype.queryString_ = null;


/** \brief Array of objects that know about magic codes.
 *
 * This variable member holds an array of objects that can convert a
 * buffer magic code in a MIME type. The system offers a limited number
 * of types recognition. In most cases that is enough, although at times
 * it would be more than useful to support many more formats and this
 * array is used for that purpose. To register additional supportive
 * classes, use the function:
 *
 * \code
 * registerBufferToMIME(MyBufferToMIMEExtension);
 * \endcode
 *
 * @type {Array.<snapwebsites.BufferToMIMETemplate>}
 * @private
 */
snapwebsites.Output.prototype.registeredBufferToMIME_; // = []; -- initialized in constructor to avoid potential problems


/** \brief Initialize the Query String parameters.
 *
 * This function is called to make sure that the Query String
 * parameters are properly initialized. Functions such as
 * the qsParam() one call this function the first time they
 * are called.
 *
 * Valid parameters are those that have an equal sign and
 * that have valid UTF-8 values (either as such or encoded).
 *
 * This function can be called multiple times, although really
 * it should be called only once since the query string is not
 * expected to change for the duration of a page lifetime.
 *
 * @private
 */
snapwebsites.Output.prototype.initQsParams_ = function()
{
    var v, variables, name_value;

    this.queryString_ = {};

    variables = location.search.replace(/^\?/, "")
                               .replace(/\+/, "%20")
                               .split("&");
    for(v in variables)
    {
        if(variables.hasOwnProperty(v))
        {
            name_value = variables[v].split("=");
            if(name_value.length === 2)
            {
                try
                {
                    this.queryString_[name_value[0]] = decodeURIComponent(name_value[1]);
                }
                catch(ignore)
                {
                    // totally ignore if invalid
                    // (happens if name_value[1] is not valid UTF-8)
                }
            }
        }
    }
};


/** \brief Retrieve a parameter from the query string.
 *
 * This function reads the query string of the current page and retrieves
 * the named parameter.
 *
 * Note that parameters that are not followed by an equal sign or that
 * have "invalid" values (not valid UTF-8) will generally be ignored.
 *
 * @param {!string} name  A valid query string name.
 * @return {string}  The value of that query string if defined, otherwise
 *                   the "undefined" value.
 */
snapwebsites.Output.prototype.qsParam = function(name)
{
    if(this.queryString_ === null)
    {
        this.initQsParams_();
    }

    // if it was not defined, then this returnes "undefined"
    return this.queryString_[name];
};


/** \brief Internal function used to display the error messages.
 *
 * This function is used to display the error messages that occured
 * "recently" (in most cases, this last access, or the one before.)
 *
 * This function is called by the init() function and shows the
 * messages if any were added to the DOM.
 *
 * \note
 * This is here because the messages plugin cannot handle the output
 * of its own messages (it is too low level a plugin.)
 *
 * @private
 */
snapwebsites.Output.prototype.handleMessages_ = function()
{
    // put a little delay() so we see the fadeIn(), eventually
    jQuery("div.user-messages")
        .each(function(){
            var z;

            z = jQuery("div.zordered").maxZIndex() + 1;
            jQuery(this).css("z-index", z);
        })
        .delay(250)
        .fadeIn(300)
        .click(function(e){
            if(!(jQuery(e.target).is("a")))
            {
                jQuery(this).fadeOut(300);
            }
        });
};


/** \brief Determine the MIME type from a buffer of data.
 *
 * Assuming you got a buffer (generally from a file dropped over a widget)
 * you may want to determine what type of file it is. This function uses
 * the Magic information of the file to return the supposed type (it may
 * still be lying to us...)
 *
 * The function returns a MIME string such as "image/png".
 *
 * \todo
 * Add code to support all the magic as defined by the tool "file".
 * (once we find the source of magic, it will be easy, now the file
 * is compiled...) Source files are on ftp://ftp.astron.com/pub/file/
 * (the home page is http://www.darwinsys.com/file/ )
 *
 * @param {ArrayBuffer} buffer  The buffer to check for a Magic.
 *
 * @return {string}  The MIME type of the file or the empty string for any
 *                   unknown (unsupported) file.
 */
snapwebsites.Output.prototype.bufferToMIME = function(buffer)
{
    var buf = new Uint8Array(buffer),   // buffer to be checked
        i,                              // loop index
        max,                            // # of registered MIME parsers
        mime;                           // the resulting MIME type

    // Give other plugins a chance to determine the MIME type
    max = this.registeredBufferToMIME_.length;
    for(i = 0; i < max; ++i)
    {
        mime = this.registeredBufferToMIME_[i].bufferToMIME(buf);
        if(mime)
        {
            return mime;
        }
    }

    // unknown magic
    return "";
};


/** \brief Register an object which is capable of determine a MIME type.
 *
 * This function allows you to register an object that defines a
 * bufferToMIME() function:
 *
 * \code
 * myObject.prototype.bufferToMIME(buf)
 * {
 *    ...
 * }
 * \endcode
 *
 * That function is passed a Uint8Array buffer. The size of the buffer
 * must be checked. It may be very small or even empty.
 *
 * @param {snapwebsites.BufferToMIMETemplate} buffer_to_mime  An object
 *        that derives from the BufferToMIMETemplate definition.
 */
snapwebsites.Output.prototype.registerBufferToMIME = function(buffer_to_mime)
{
    this.registeredBufferToMIME_.push(buffer_to_mime);
};


// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.OutputInstance = new snapwebsites.Output();
        snapwebsites.OutputInstance.registerBufferToMIME(new snapwebsites.BufferToMIMESystemImages());
    }
);

// vim: ts=4 sw=4 et
