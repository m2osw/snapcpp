/** @preserve
 * Name: server-access
 * Version: 0.0.1.18
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

/*jslint nomen: true, todo: true, devel: true */
/*global snapwebsites: false, jQuery: false */



/** \brief Interface you have to implement to receive the access results.
 *
 * This interface has to be derived from so you receive different callbacks
 * as required by your objects.
 *
 * \code
 *  interface ServerAccessCallbacks
 *  {
 *  public:
 *      typedef ... ResultData;
 *
 *      function ServerAccessCallbacks();
 *
 *      abstract function serverAccessSuccess(result : ResultData) : Void;
 *      abstract function serverAccessError(result : ResultData) : Void;
 *      abstract function serverAccessComplete(result : ResultData) : Void;
 *  };
 * \endcode
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
 * \li [ESC] result_status -- the AJAX result_status parameter.
 * \li [ES] messages -- XML NodeList object with the error/success messages
 *                      returned in the AJAX response or null.
 * \li [E] error_message -- our own error message, may happen even if the
 *                          AJAX data returned and worked.
 * \li [E] ajax_error_message -- The raw AJAX system error message.
 * \li [ESC] jqxhr -- the original XHR plus a few things adjusted by jQuery.
 * \li [S] result_data -- raw AJAX result string.
 * \li [S] will_redirect -- whether the response includes a redirect request.
 *                          You may set this to false in your callback to
 *                          avoid the redirect.
 * \li [ESC] userdata -- the data passed to the send() function, may be
 *                       set to 'undefined'.
 *
 * The [ESC] letters stand for:
 *
 * \li E -- set when an error occurs,
 * \li S -- set when the AJAX request was successful,
 * \li C -- reset in the complete function before calling your callback.
 *
 * The ajax_error_message may not be set if the error occurs on a successful
 * AJAX request, but the server generated an error. In that case, the
 * result_data and messages are likely defined.
 *
 * @typedef {{result_status: string,
 *            messages: (NodeList|null),
 *            error_message: string,
 *            ajax_error_message: string,
 *            jqxhr: (Object|null),
 *            result_data: string,
 *            will_redirect: boolean,
 *            userdata: (Object|null|undefined)}}
 */
snapwebsites.ServerAccessCallbacks.ResultData;


/*jslint unparam: true */
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
/*jslint unparam: false */


/*jslint unparam: true */
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
/*jslint unparam: false */


/*jslint unparam: true */
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
/*jslint unparam: false */



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
 * \code
 * class ServerAccess
 * {
 * public:
 *     function ServerAccess(callback: ServerAccessCallbacks);
 *     function setURI(uri: string, opt_queryString: Object) : Void;
 *     function setData(data: Object) : Void;
 *     function send() : Void;
 *     static function appendQueryString(uri: string, query_string: Object): string;
 *
 * private:
 *     var callback_: ServerAccessCallbacks = null;
 *     var uri_: string = "";
 *     var queryString_: Object = null;
 *     var data_: Object = null;
 * };
 * \endcode
 *
 * data_ may specifically be an object of type FormData.
 *
 * @param {snapwebsites.ServerAccessCallbacks} callback  An object reference,
 *          object that derives from the ServerAccessCallbacks interface.
 *
 * @constructor
 * @struct
 */
snapwebsites.ServerAccess = function(callback)
{
    this.callback_ = callback;
};


/** \brief Mark ServerAccess as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(snapwebsites.ServerAccess);


/** \brief Type of object in the data_ parameter.
 *
 * This object represents a simple object of key/value pairs.
 *
 * Adding parameters is as simple as setting assigning a value to
 * a new member:
 *
 * \code
 *    this.data_._ajax = 1;
 * \endcode
 *
 * @type {string}
 * @const
 * @private
 */
snapwebsites.ServerAccess.OBJECT_ = "object"; // static const


/** \brief Type of object in the data_ parameter.
 *
 * This object represents a FormData object. Setting parameters in a FormData
 * requires the append() function instead of using the [] operator.
 *
 * \code
 *    this.data_.append("_ajax", "1");
 * \endcode
 *
 * @type {string}
 * @const
 * @private
 */
snapwebsites.ServerAccess.FORM_ = "form"; // static const


/** \brief The object wanting remote access.
 *
 * This object represents the object that wants to access the Snap!
 * server. It has to be an object that derives from the
 * snapwebsites.ServerAccessCallbacks interface.
 *
 * @type {snapwebsites.ServerAccessCallbacks}
 * @private
 */
snapwebsites.ServerAccess.prototype.callback_ = null;


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


/** \brief The type of object defined in the data_ variable member.
 *
 * The ServerAccess object accepts two types of object:
 *
 * \li Object -- a simple key/value based object, which is sent as a
 *               query string (key1=value1&key2=value2&...)
 * \li FormData -- a form data object, which is created using
 *                 "new FormData()" and supports sending files
 *
 * The AJAX request is tweaked depending on the type of object in use.
 *
 * The default is "object". The value is always forced when you call
 * the setData() or setFormData() functions.
 *
 * @type {string}
 * @private
 */
snapwebsites.ServerAccess.prototype.dataType_ = "object";


/** \brief An object if key/value pairs to send to the server.
 *
 * The ServerAccess object uses this object to build a set of variables
 * (name=value) to be sent to the server.
 *
 * This object is directly passed to the ajax() function of jQuery.
 *
 * @type {Object|FormData}
 * @private
 */
snapwebsites.ServerAccess.prototype.data_ = null;


/** \brief Set the URI used to send the data.
 *
 * This function is used to set the URI and optional query string of
 * the destination object.
 *
 * @param {!string} uri  The URI where the data is to be sent.
 * @param {Object=} opt_queryString  An option set of key/value pairs.
 */
snapwebsites.ServerAccess.prototype.setURI = function(uri, opt_queryString)
{
    this.uri_ = uri ? uri : "/";
    this.queryString_ = opt_queryString;
};


/** \brief Set the data key/value pairs to send to the server.
 *
 * This function takes an object of data to be sent to the server. The
 * key/value pairs form the variable names and values sent to the server
 * using a POST.
 *
 * The object can be anything, although it is safer to keep a single level
 * of key/value pairs (no sub-objects, no array.)
 *
 * \note
 * The system always adds the "_ajax" field to your object. This allows
 * the server to know that this specific POST is an AJAX query. This
 * changes your original object since we do not do a deep copy of it.
 *
 * @param {!Object} data  The data to send to the server.
 */
snapwebsites.ServerAccess.prototype.setData = function(data)
{
    if(data)
    {
        // in this case we expect a standard simple field name/value
        // object or a FormData, define the type depending on that
        this.dataType_ = data instanceof FormData
                    ? snapwebsites.ServerAccess.FORM_
                    : snapwebsites.ServerAccess.OBJECT_;

        this.data_ = data;

        // always force the _ajax field to 1
        if(this.dataType_ === snapwebsites.ServerAccess.FORM_)
        {
            this.data_.append("_ajax", "1");
        }
        else
        {
            this.data_._ajax = 1;
        }
    }
};


/** \brief Set the data in the form of a FormData object.
 *
 * This function expects a FormData object (or null). This function
 * is used when you need to send more than just simple field name/value
 * pairs. In most cases this means you are sending a file in a
 * multi-part message.
 *
 * To setup a FormData object, do the following:
 *
 * \code
 * // create the FormData object
 * var form_data = new FormData();
 *
 * // set fields
 * form_data.append("field_name1", "value1");
 * form_data.append("field_name2", "value2");
 *    ...
 * form_data.append("field_nameN", "valueN");
 *
 * // set data in your server access object
 * server_access.setFormData(form_data);
 * \endcode
 *
 * Any of the value1, value2, etc. can be a file:
 *
 * \code
 * form_data.append("my_file", e.originalEvent.dataTransfer.files[0]);
 * \endcode
 *
 * The AJAX request will automatically handle the necessary conversions
 * to send the data.
 *
 * \note
 * The system always adds the "_ajax" field to your object. This allows
 * the server to know that this specific POST is an AJAX query. This
 * changes your original object since we do not do a deep copy of it.
 *
 * @param {!Object} form_data  The data to send to the server.
 */


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
 *
 * \todo
 * Look into a way to allow for serialization of "many" requests (i.e.
 * stack send() requests so that we can process the next one when
 * we get the complete() event.)
 *
 * \todo
 * Allow for a way to prevent the user from closing the window until the
 * request was completed. We have such a thing in the editor, but that
 * ignores the server access, and it means the unload event should
 * be handled by a lower level object commont to the server access and
 * the editor (and both could request to be checked on unload...)
 *
 * @param {Object|null=} opt_userdata  Any userdata that will be attached to
 *                                     the result sent to your callbacks.
 */
snapwebsites.ServerAccess.prototype.send = function(opt_userdata)
{
    var that = this,
        uri = snapwebsites.ServerAccess.appendQueryString(this.uri_, this.queryString_);

    /** \brief Initialize the result object.
     *
     * The letters below tell you where each member of the result
     * object is modified.
     *
     * \li E -- error
     * \li S -- success
     * \li C -- completion
     *
     * Note that the same object is passed to the completion so the error
     * and success parameters, if not overridden by the completion step,
     * are also available in the completion function (also are fields
     * you added in your success or error callback.)
     *
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
            will_redirect: false,

            // [ESC] A user object
            userdata: opt_userdata
        };

    // TODO: add an AJAX object definition or find the one from jQuery externs
    //       to have closure ensure we don't mess up this object parameters
    var ajax_options =
        {
            type: "POST",
            processData: this.dataType_ === snapwebsites.ServerAccess.OBJECT_,
            data: this.data_,
            error: function(jqxhr, result_status, error_msg)
            {
                result.jqxhr = jqxhr;
                result.result_status = result_status;
                result.ajax_error_message = error_msg;
                that.onError_(result);
            },
            success: function(data, result_status, jqxhr)
            {
                result.jqxhr = jqxhr;
                result.result_data = data;
                result.result_status = result_status;
                that.onSuccess_(result);
            },
            complete: function(jqxhr, result_status)
            {
                result.jqxhr = jqxhr;
                result.result_status = result_status;
                that.onComplete_(result);
            },
            dataType: "xml" // server is expected to return XML only
        };

    // TODO: to the xhr object, add a listener to handle the upload
    //       progress (most certainly will work in Chrome, might not
    //       in other broswers without many more AJAX calls...)
    //       http://stackoverflow.com/questions/166221/how-can-i-upload-files-asynchronously-with-jquery#8758614

    if(this.dataType_ === snapwebsites.ServerAccess.FORM_)
    {
        // prevent jQuery from setting the Content-Type field as that
        // would otherwise clear the boundary string of the request
        //
        // see: http://stackoverflow.com/questions/5392344/sending-multipart-formdata-with-jquery-ajax#5976031
        ajax_options.contentType = false;
    }

    jQuery.ajax(uri, ajax_options);
};


/** \brief Handle the case of a failed AJAX request.
 *
 * This function is called whenever the error callback of the ajax()
 * function gets called.
 *
 * The function builds a result object and calls the user server
 * access callback named serverAccessError().
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The result object to pass to the serverAccessError().
 *
 * @private
 */
snapwebsites.ServerAccess.prototype.onError_ = function(result)
{
    result.error_message = "An error occured while posting AJAX (status: "
                    + result.result_status + " / error: " + result.ajax_error_message + ")";
    this.callback_.serverAccessError(result);
};


/** \brief Handle the case of a successful AJAX request.
 *
 * This function is called whenever the success callback of the ajax()
 * function gets called.
 *
 * The function builds a result object and calls the user server
 * access callback named serverAccessError() or serverAccessSuccess()
 * depending on what is defined in the result request.
 *
 * If we do not get XML in return, or the XML response is not
 * "success", then the function generates an error and calls the
 * serverAccessError() callback.
 *
 * If the response HTTP code is 200, the response is XML, and the
 * XML says "success", then the function calls the serverAccessSuccess()
 * callback.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The result object.
 *
 * @private
 */
snapwebsites.ServerAccess.prototype.onSuccess_ = function(result)
{
    //
    // WARNING: success of the AJAX round trip data does not
    //          mean that the POST was a success.
    //
    var results,        // the XML results (should be 1 element)
        doc,            // the document used to redirect
        redirect,       // the XML redirect element
        redirect_uri,   // the redirect URI
        target,         // the redirect target
        doc_type_start, // the position of <!DOCTYPE in the result
        doc_type_end,   // the position of > of the <!DOCTYPE>
        doc_type;       // the DOCTYPE information

//console.log(jqxhr);

    if(result.jqxhr.status === 200)
    {
        doc_type_start = result.jqxhr.responseText.indexOf("<!DOCTYPE");
        if(doc_type_start !== -1)
        {
            doc_type_start += 9; // skip the <!DOCTYPE part
            doc_type_end = result.jqxhr.responseText.indexOf(">", doc_type_start);
            doc_type = result.jqxhr.responseText.substr(doc_type_start, doc_type_end - doc_type_start);
            if(doc_type.indexOf("html") !== -1)
            {
                // this is definitively wrong, but we want to avoid
                // the following tests in case the HTML includes
                // invalid content (although it looks like jQuery
                // do not convert such documents to an XML tree
                // anyway...)
                result.error_message = "The server replied with HTML instead of XML.";
                this.callback_.serverAccessError(result);
                return;
            }
        }

        // success or error, we may have messages
        // (at this point, only errors, but later...)
        result.messages = result.jqxhr.responseXML.getElementsByTagName("messages");

        // we expect exactly ONE result tag
        results = result.jqxhr.responseXML.getElementsByTagName("result");
        if(results.length === 1 && results[0].childNodes[0].nodeValue === "success")
        {
//alert("The AJAX succeeded (" + result_status + ")");

            // success!
            redirect = result.jqxhr.responseXML.getElementsByTagName("redirect");
            result.will_redirect = redirect.length === 1;

            this.callback_.serverAccessSuccess(result);

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
                if(target === "_parent" && window.parent)
                {
                    // TODO: we probably want to support
                    // multiple levels (i.e. a "_top" kind
                    // of a thing) instead of just one up.
                    doc = window.parent.document;
                }
                else if(target === "_top")
                {
                    doc = window.top.document;
                }
                // else TODO search for a window named 'target'
                //           and do the redirect in there?
                //           it doesn't look good in terms of
                //           API though... we can find frames
                //           but not windows unless we 100%
                //           handle the window.open() calls

                redirect_uri = redirect[0].childNodes[0].nodeValue;
                if(redirect_uri === ".")
                {
                    // just exit the editor (i.e. remove the edit action)
                    //
                    // TODO: the action field name may not be 'a'
                    redirect_uri = doc.location.toString();
                    redirect_uri = redirect_uri.replace(/\?a=edit$/, "")
                                               .replace(/\?a=edit&/, "?")
                                               .replace(/&a=edit&/, "&");
                }
                doc.location = redirect_uri;
                // avoid anything else after a redirect
                return;
            }
        }
        else
        {
            // although it went round trip fine, the application
            // returned an error... report it
            result.error_message = "The server replied with errors.";
            this.callback_.serverAccessError(result);
        }
    }
    else
    {
        result.error_message = "The server replied with HTTP code " + result.jqxhr.status
                             + " while posting AJAX (status: " + result.result_status + ")";
        this.callback_.serverAccessError(result);
    }
};


/** \brief Function called on completion of the AJAX query.
 *
 * After the serverAccessSuccess() or serverAccessError() callbacks were
 * called, this function calls the serverAccessComplete() callback.
 *
 * This callback is always called, whether the AJAX was successful or
 * erroneous.
 *
 * @param {snapwebsites.ServerAccessCallbacks.ResultData} result  The result object.
 *
 * @private
 */
snapwebsites.ServerAccess.prototype.onComplete_ = function(result)
{
    this.callback_.serverAccessComplete(result);
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



/** \brief Interface used to call the function used to send the data.
 *
 * Whenever you call the send() function of the ServerAccessTimer, the
 * request to be sent is not yet available. This is important to be able
 * to minimize the number of connections: i.e. you may get 10 changes to
 * the data to be sent between the first call to send() and the call to
 * this callback function.
 *
 * \code
 *  class ServerAccessTimerCallbacks
 *  {
 *  public:
 *      function ServerAccessTimerCallbacks();
 *
 *      abstract function serverAccessSuccess(result: ResultData) : Void;
 *      abstract function serverAccessError(result: ResultData) : Void;
 *      abstract function serverAccessComplete(result: ResultData) : Void;
 *
 *      abstract function serverAccessTimerReady(request_name: string, server_access: ServerAccess) : Void;
 *  };
 * \endcode
 *
 * @return {snapwebsites.ServerAccessTimerCallbacks}
 *
 * @constructor
 * @struct
 * @extends {snapwebsites.ServerAccessCallbacks}
 */
snapwebsites.ServerAccessTimerCallbacks = function()
{
    return this;
};


/** \brief The ServerAccessTimerCallbacks inherits from the ServerAccessCallbacks.
 *
 * In order to transmit the server access callbacks from the timer to
 * the user, we also inherits from those callbacks.
 */
snapwebsites.inherits(snapwebsites.ServerAccessTimerCallbacks, snapwebsites.ServerAccessCallbacks);


/*jslint unparam: true */
/** \brief Function called whenever the server access timer is ready.
 *
 * This function is called whenever the server access timer times out
 * and thus it is ready for the used to send a new POST to the server.
 *
 * Note that on the first call to the ServerAccessTimer.send() function
 *
 * @param {string} request_name  The request name passed to the send() function.
 * @param {snapwebsites.ServerAccess} server_access  The server access object
 *                          that the callback is expected to setup.
 */
snapwebsites.ServerAccessTimerCallbacks.prototype.serverAccessTimerReady = function(request_name, server_access)
{
    throw new Error("snapwebsites.ServerAccessTimerCallbacks.serverAccessTimerReady() must be overridden, it is abstract");
};
/*jslint unparam: false */


/*jslint unparam: true */
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
snapwebsites.ServerAccessTimerCallbacks.prototype.serverAccessSuccess = function(result) // virtual
{
};
/*jslint unparam: false */


/*jslint unparam: true */
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
snapwebsites.ServerAccessTimerCallbacks.prototype.serverAccessError = function(result) // virtual
{
};
/*jslint unparam: false */


/*jslint unparam: true */
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
snapwebsites.ServerAccessTimerCallbacks.prototype.serverAccessComplete = function(result) // virtual
{
};
/*jslint unparam: false */



/** \brief Timed AJAX requests.
 *
 * This class can be used to avoid sending too many AJAX requests at
 * the server.
 *
 * \code
 *  class ServerAccessTimer extends ServerAccessCallbacks
 *  {
 *      function ServerAccessTimer(request_name: String, timer_callback: ServerAccessTimerCallbacks, interval: Number);
 *
 *      function send();
 *
 *      virtual function serverAccessSuccess(result : ResultData) : Void;
 *      virtual function serverAccessError(result : ResultData) : Void;
 *      virtual function serverAccessComplete(result : ResultData) : Void;
 *
 *  private:
 *      var requestName_: String;
 *      var timerCallback_: ServerAccessTimerCallbacks;
 *      var interval_: Number;
 *      var processing_: Boolean;
 *      var timer_: Number;
 *      var lastRequest_: Number;
 *  };
 * \endcode
 *
 * When a request is sent, it uses the \p request_name to call the
 * ServerAccessTimerCallbacks.serverAccessTimerReady() callback
 * function.
 *
 * The \p opt_interval is defined in milliseconds. So to have at least one
 * second between requests, use 1,000. Note that the minimum interval is
 * 1,000 and the default is 2,000.
 *
 * @param {string} request_name  The name of the this timer.
 * @param {snapwebsites.ServerAccessTimerCallbacks}  timer_callback  The object
 *         that implements the ServerAccessTimerCallbacks interface and gets
 *         called when a request should be sent to the server.
 * @param {number=} opt_interval  The minimum amount of time between requests.
 *
 * @constructor
 * @struct
 * @extends {snapwebsites.ServerAccessCallbacks}
 */
snapwebsites.ServerAccessTimer = function(request_name, timer_callback, opt_interval)
{
    this.requestName_ = request_name;
    this.timerCallback_ = timer_callback;
    if(opt_interval && opt_interval > 0)
    {
        if(opt_interval < 1000)
        {
            opt_interval = 1000;
        }
        this.interval_ = opt_interval;
    }

    return this;
};


/** \brief The ServerAccessTimer is a base class.
 *
 * The ServerAccessTimer is a base class.
 */
snapwebsites.inherits(snapwebsites.ServerAccessTimer, snapwebsites.ServerAccessCallbacks);


/** \brief The name of the request being handled by this timer.
 *
 * This variable represents the name of the request being handled
 * by this timer. If you have multiple ServerAccessTimer objects
 * attached to a single object, you can use that name to distinguish
 * between each request.
 *
 * The request name must be defined when creating the ServerAccessTimer.
 *
 * @type {!string}
 */
snapwebsites.ServerAccessTimer.prototype.requestName_ = "";


/** \brief The reference to the object with the timer callback.
 *
 * This variable is a reference to an object that implements the
 * ServerAccessTimerCallbacks interface.
 *
 * The callback object must be defined when creating the ServerAccessTimer.
 *
 * @type {snapwebsites.ServerAccessTimerCallbacks}
 */
snapwebsites.ServerAccessTimer.prototype.timerCallback_ = null;


/** \brief The reference to the object with the timer callback.
 *
 * This variable is a reference to an object that implements the
 * ServerAccessTimerCallbacks interface.
 *
 * The callback object must be defined when creating the ServerAccessTimer.
 *
 * @type {number}
 */
snapwebsites.ServerAccessTimer.prototype.interval_ = 2000;


/** \brief Whether we are waiting on a POST.
 *
 * This variable is false by default. It gets set to true whenever the
 * ServerAccessTimerCallbacks.serverAccessTimerReady() function is
 * called and back to false when the request returns, whether it was
 * successful or not.
 *
 * @type {boolean}
 */
snapwebsites.ServerAccessTimer.prototype.processing_ = false;


/** \brief Whether we received another call to the send() function.
 *
 * This variable gets set to true if the processing_ flag is true
 * and the send() function gets called. In that situation, we prevent
 * the send() from sending anything until the processing of the
 * current request ends.
 *
 * @type {boolean}
 */
snapwebsites.ServerAccessTimer.prototype.sendAgain_ = false;


/** \brief Timer identifier.
 *
 * The server access timer uses a timer when a new request should be sent
 * to the server, but the server is currently busy. The timer is setup
 * only if it is not already set and if there is a request running now
 * or the last request was sent less than \p interval_ milliseconds ago.
 *
 * @type {number}
 */
snapwebsites.ServerAccessTimer.prototype.timer_ = NaN;


/** \brief The last time a request was sent.
 *
 * The server access timer is required to not send too many requests to
 * the server. This is done by using this lastRequest_ parameter which
 * is the last time a request was sent.
 *
 * @type {number}
 */
snapwebsites.ServerAccessTimer.prototype.lastRequest_ = 0;


/** \brief The server access object.
 *
 * When using the timer, this object has to be the one holding the
 * server access object because it needs to properly handle all the
 * flags. This is the object. The object is also passed down to
 * the object that created the ServerAccessTimer so it can add the
 * data in the POST request (i.e. URL and data).
 *
 * @type {snapwebsites.ServerAccess}
 */
snapwebsites.ServerAccessTimer.prototype.serverAccess_ = null;


/** \brief Request that a POST request be sent.
 *
 * This function registers that the caller wants a request to be sent
 * to the server.
 *
 * If the ServerAccessTimer was not currently handling a request
 * with the specified name and the last time a request with that
 * name was set more than the specified interval, then the post
 * gets sent immediately.
 *
 * Otherwise the request gets registered and a timer started
 * one is not already running.
 */
snapwebsites.ServerAccessTimer.prototype.send = function()
{
    var that = this,
        interval;

    if(this.processing_)
    {
        // as the completion function to send another request
        // (it may require a timer, but we do not know at this point)
        this.sendAgain_ = true;
        // if(isNaN(this.timer_))
        // {
        //     if(this.lastRequest_ != 0)
        //     {
        //         interval = Date.now() - this.lastRequest_;
        //         if(interval > this.interval_)
        //         {
        //             interval = this.interval_;
        //         }
        //     }
        //     else
        //     {
        //         interval = this.interval_;
        //     }
        //     this.timer_ = setTimeout(function(){that.sendRequest_();}, interval);
        //     this.lastRequest_ = Date.now();
        // }
        // // else -- the timer is already set, no need to set it again!
    }
    else
    {
        // if we arrive here, this flag should always be false, but just
        // in case, we reset it here
        this.sendAgain_ = false;
        if(this.lastRequest_ != 0)
        {
            interval = Date.now() - this.lastRequest_;
            if(interval > this.interval_)
            {
                // enough time spent in between, we can run immediately
                this.sendRequest_();
            }
            else
            {
                this.timer_ = setTimeout(function(){that.sendRequest_();}, interval);
                this.lastRequest_ = Date.now();
            }
        }
        else
        {
            this.sendRequest_();
        }
    }
};


/** \brief The timer timed out, so send a request as promised.
 *
 * This function sends a request as the user asked us to do. It sets
 * the processing_ flag to true and reset the timer so if another
 * request to the send() function is made, it will be processed
 * quickly.
 */
snapwebsites.ServerAccessTimer.prototype.sendRequest_ = function() // static
{
    // timer kicked in, clear the ID
    this.timer_ = NaN;

    this.processing_ = true;

    if(!this.serverAccess_)
    {
        this.serverAccess_ = new snapwebsites.ServerAccess(this);
    }

    // we expect the callback to setup these two parameters
    //this.serverAccess_.setURI(...);
    //this.serverAccess_.setData(...);
    this.timerCallback_.serverAccessTimerReady(this.requestName_, this.serverAccess_);

    this.serverAccess_.send();
};


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
snapwebsites.ServerAccessTimer.prototype.serverAccessSuccess = function(result) // virtual
{
    this.timerCallback_.serverAccessSuccess(result);
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
snapwebsites.ServerAccessTimer.prototype.serverAccessError = function(result) // virtual
{
    this.timerCallback_.serverAccessError(result);
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
snapwebsites.ServerAccessTimer.prototype.serverAccessComplete = function(result) // virtual
{
    this.processing_ = false;
    this.timerCallback_.serverAccessComplete(result);
    if(this.sendAgain_)
    {
        this.sendAgain_ = false;
        this.send();
    }
};



// vim: ts=4 sw=4 et
