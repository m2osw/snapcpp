/** @preserve
 * Name: server-access
 * Version: 0.0.1.7
 * Browsers: all
 * Depends: output (>= 0.1.5)
 * Copyright: Copyright 2013-2014 (c) Made to Order Software Corporation  All rights reverved.
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
// @js plugins/output/output.js
// ==/ClosureCompiler==
//



/** \brief Interface you have to implement to receive the access results.
 *
 * This interface has to be derived from so you receive different callbacks
 * as required by your objects.
 *
 * @return {snapwebsites.ServerAccessCallbacks}
 *
 * @constructor
 * @struct
 */
snapwebsites.ServerAccessCallbacks = function()
{
    return this;
};


/** \brief Mark EditorWidgetTypeBase as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.ServerAccessCallbacks);


/** \brief The type of parameter one can pass to the save functions.
 *
 * The result is saved in a data object which fields are:
 *
 * \li html -- the data being saved, used to change the originalData_
 *               field once successfully saved.
 * \li result -- the data to send to the server.
 *
 * @typedef {{result_status: string,
 *            messages: (Object|null),
 *            error_message: string,
 *            ajax_error_message: string,
 *            jqxhr: (Object|null),
 *            result_data: string,
 *            will_redirect: boolean}}
 */
snapwebsites.ServerAccessCallbacks.ResultData;


/** \brief Function called on success.
 *
 * This function is called if the remote access was successful. The
 * result object includes a reference to the XML document found in the
 * data sent back from the server.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data.
 */
snapwebsites.ServerAccessCallbacks.prototype.serverAccessSuccess = function(result) // virtual
{
};


/** \brief Function called on error.
 *
 * This function is called if the remote access generated an error.
 * In this case errors include I/O errors, server errors, and application
 * errors. All call this function so you do not have to repeat the same
 * code for each type of error.
 *
 * \li I/O errors -- somehow the AJAX command did not work, maybe the
 *                   domain name is wrong or the URI has a syntax error.
 * \li server errors -- the server received the POST but somehow refused
 *                      it (maybe the request generated a crash.)
 * \li application errors -- the server received the POST and returned an
 *                           HTTP 200 result code, but the result includes
 *                           a set of errors (not enough permissions,
 *                           invalid data, etc.)
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.ServerAccessCallbacks.prototype.serverAccessError = function(result) // virtual
{
};


/** \brief Function called on completion.
 *
 * This function is called once the whole process is over. It is most
 * often used to do some cleanup.
 *
 * By default this function does nothing.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The
 *          resulting data with information about the error(s).
 */
snapwebsites.ServerAccessCallbacks.prototype.serverAccessComplete = function(result) // virtual
{
};



/** \brief The ServerAccess constructor.
 *
 * Whenever you want to send data to the server and expect a "standard"
 * XML document (standard for Snap! at least) then you should make use
 * of this type of object.
 *
 * First create an instance with new, then set the different parameters
 * that you are interested in, then call the send() function. Once a
 * response is received, one or more of you callbacks will be called.
 *
 * @param {snapwebsites.ServerAccessCallbacks} that  An object reference,
 *          object that derives from the ServerAccessCallbacks interface.
 *
 * @constructor
 * @struct
 */
snapwebsites.ServerAccess = function(that)
{
    this.that_ = that;
};


/** \brief Mark ServerAccess as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.ServerAccess);


/** \brief The object wanting remote access.
 *
 * This object represents the object that wants to access the Snap!
 * server. It has to be an object that derives from the
 * snapwebsites.ServerAccessCallbacks interface.
 *
 * @type {Object}
 * @private
 */
snapwebsites.ServerAccess.prototype.that_ = null;


/** \brief The URI used to send the request to the server.
 *
 * The ServerAccess object needs a valid URI in order to send a request
 * to the destination server.
 *
 * This parameter is mandatory. However, it is not part of the constructor
 * so one can reuse the same ServerAccess multiple times with different
 * functions.
 *
 * @type {string}
 * @private
 */
snapwebsites.ServerAccess.prototype.uri_ = "";


/** \brief An object if key/value pairs for the query string.
 *
 * The ServerAccess object accepts an object of key and value pairs
 * used to append query strings to the URI before sending the
 * request.
 *
 * This parameter is otional. However, if it is ever set and the
 * ServerAccess is to be used multiple times, then it should be
 * cleared before reusing this object.
 *
 * @type {Object|undefined}
 * @private
 */
snapwebsites.ServerAccess.prototype.queryString_ = null;


/** \brief An object if key/value pairs to send to the server.
 *
 * The ServerAccess object uses this object to build a set of variables
 * (name=value) to be sent to the server.
 *
 * This object is directly passed to the ajax() function of jQuery.
 *
 * @type {Object}
 * @private
 */
snapwebsites.ServerAccess.prototype.data_ = null;


/** \brief Set the URI used to send the data.
 *
 * This function is used to set the URI and optional query string of
 * the destination object.
 *
 * @param {!string} uri  The URI where the data is to be sent.
 * @param {Object=} queryString_opt  An option set of key/value pairs.
 */
snapwebsites.ServerAccess.prototype.setURI = function(uri, queryString_opt)
{
    this.uri_ = uri ? uri : "/";
    this.queryString_ = queryString_opt;
};


/** \brief Set the data key/value pairs to send to the server.
 *
 * This function takes an object of data to be sent to the server. The
 * key/value pairs form the variable names and values sent to the server
 * using a POST.
 *
 * The object can be anything, although it is safer to keep a single level
 * of key/value pairs (no sub-objects.)
 *
 * @param {!Object} data  The data to send to the server.
 */
snapwebsites.ServerAccess.prototype.setData = function(data)
{
    this.data_ = data;
};


/** \brief Send a POST to the server.
 *
 * This function remotely accesses the Snap! server using AJAX. It expects
 * your ServerAccess object to have been setup properly:
 *
 * \li setURI() -- called to setup the destination URI with an optional
 *                 query string parameter.
 *
 * \li setData() -- called to setup the data to be sent to the server.
 *
 * \note
 * The function returns while the data is still being sent as it is
 * asynchroneous.
 */
snapwebsites.ServerAccess.prototype.send = function()
{
    var that = this,
        uri = this.uri_;

    /**
     * @type {snapwebsites.ServerAccessCallbacks.ResultData}
     */
    var result =
        {
            // [ESC] A string representing the result of the AJAX command
            result_status: "",

            // [ES] Messages in XML (i.e. from the messages plugin)
            messages: null,

            // [E] A string we generate representing this error or ""
            error_message: "",

            // [E] An error message generated by the AJAX implementation
            ajax_error_message: "",

            // [ESC] The jQuery header object
            jqxhr: null,

            // [S] The resulting data (raw format)
            result_data: "",

            // [S] Whether a redirect will be done on success
            will_redirect: false
        };


    jQuery.ajax(snapwebsites.ServerAccess.appendQueryString(uri, this.queryString_), {
        type: "POST",
        processData: true,
        data: this.data_,
        error: function(jqxhr, result_status, error_msg)
        {
            result.jqxhr = jqxhr;
            result.result_status = result_status;
            result.ajax_error_message = error_msg;
            result.error_message = "An error occured while posting AJAX (status: "
                            + result_status + " / error: " + error_msg + ")";
            that.that_.serverAccessError(result);
        },
        success: function(data, result_status, jqxhr)
        {
            var results,        // the XML results (should be 1 element)
                doc,            // the document used to redirect
                redirect,       // the XML redirect element
                uri,            // the redirect URI
                target,         // the redirect target
                doc_type_start, // the position of <!DOCTYPE in the result
                doc_type_end,   // the position of > of the <!DOCTYPE>
                doc_type;       // the DOCTYPE information

            result.result_data = data;
            result.result_status = result_status;
            result.jqxhr = jqxhr;

//console.log(jqxhr);
            if(jqxhr.status == 200)
            {
                doc_type_start = jqxhr.responseText.indexOf("<!DOCTYPE");
                if(doc_type_start != -1)
                {
                    doc_type_start += 9; // skip the <!DOCTYPE part
                    doc_type_end = jqxhr.responseText.indexOf(">", doc_type_start);
                    doc_type = jqxhr.responseText.substr(doc_type_start, doc_type_end - doc_type_start);
                    if(doc_type.indexOf("html") != -1)
                    {
                        // this is definitively wrong, but we want to avoid
                        // the following tests in case the HTML includes
                        // invalid content (although it looks like jQuery
                        // do not convert such documents to an XML tree
                        // anyway...)
                        result.error_message = "The server replied with HTML instead of XML.";
                        that.that_.serverAccessError(result);
                        return;
                    }
                }

                // success or error, we may have messages
                // (at this point, only errors, but later...)
                result.messages = jqxhr.responseXML.getElementsByTagName("messages");

                // we expect exactly ONE result tag
                results = jqxhr.responseXML.getElementsByTagName("result");
                if(results.length == 1 && results[0].childNodes[0].nodeValue == "success")
                {
                    // WARNING: success of the AJAX round trip data does not
                    //          mean that the POST was a success.
                    alert("The AJAX succeeded (" + result_status + ")");

                    // success!
                    redirect = jqxhr.responseXML.getElementsByTagName("redirect");
                    result.will_redirect = redirect.length == 1;

                    that.that_.serverAccessSuccess(result);

                    // test the object flag so the callback could set it to
                    // false if applicable
                    if(result.will_redirect)
                    {
                        // used asked to redirect the user after a
                        // successful save
                        doc = document;

                        // get the target to see whether we need to use the
                        // parent, top, or self...
                        target = redirect[0].getAttribute("target");
                        if(target == "_parent" && window.parent)
                        {
                            // TODO: we probably want to support
                            // multiple levels (i.e. a "_top" kind
                            // of a thing) instead of just one up.
                            doc = window.parent.document;
                        }
                        else if(target == "_top")
                        {
                            doc = window.top.document;
                        }
                        // else TODO search for a window named 'target'
                        //           and do the redirect in there?
                        //           it doesn't look good in terms of
                        //           API though... we can find frames
                        //           but not windows unless we 100%
                        //           handle the window.open() calls

                        uri = redirect[0].childNodes[0].nodeValue;
                        if(uri == ".")
                        {
                            // just exit the editor
                            uri = doc.location.toString();
                            uri = uri.replace(/\?a=edit$/, "")
                                     .replace(/\?a=edit&/, "?")
                                     .replace(/&a=edit&/, "&");
                        }
                        doc.location = uri;
                        // avoid anything else after a redirect
                        return;
                    }
                }
                else
                {
                    // although it went round trip fine, the application
                    // returned an error... report it
                    result.error_message = "The server replied with errors.";
                    that.that_.serverAccessError(result);
                }
            }
            else
            {
                result.error_message = "The server replied with HTTP code " + jqxhr.status
                                     + " while posting AJAX (status: " + result_status + ")";
                that.that_.serverAccessError(result);
            }
        },
        complete: function(jqxhr, result_status)
        {
            result.jqxhr = jqxhr;
            result.result_status = result_status;
            that.that_.serverAccessComplete(result);
        },
        dataType: "xml"
    });
};


/** \brief Append a query string to a URI.
 *
 * This static function appends the list of query strings defined in the
 * \p query_string object. The object is expected to be a simple set of
 * key/pair values. The key is the name of the query string parameter
 * and the value is the value after the equal sign.
 *
 * If the \p query_string parameter is an empty object or null then nothing
 * happens.
 *
 * \todo
 * We may want to add debug checks on the key and value parameters to make
 * sure that we do not accept certain things (i.e. too keys that are too
 * long or not plain ASCII, values that are too long, include rather
 * unacceptable characters.) We may also want to check for the anchor
 * (#) in the URI.
 *
 * @param {!string} uri  The URI to which we append the query string.
 * @param {Object|undefined} query_string  The set of key/value pairs to append.
 *
 * @return {!string} The updated URI.
 */
snapwebsites.ServerAccess.appendQueryString = function(uri, query_string) // static
{
    var o,                      // loop index
        separator;              // the next separator to use (? or &)

    // TBD: we should never have a # in the URI here, correct?
    if(query_string)
    {
        // append the options, if we already have a ?, use & from the start
        separator = uri.indexOf("?") >= 0 ? "&" : "?";
        for(o in query_string)
        {
            if(query_string.hasOwnProperty(o))
            {
                uri += separator + o + "=" + encodeURIComponent(query_string[o]);
                separator = "&";
            }
        }
    }

    return uri;
};


// vim: ts=4 sw=4 et
