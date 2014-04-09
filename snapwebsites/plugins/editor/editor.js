/** @preserve
 * Name: editor
 * Version: 0.0.2.89
 * Browsers: all
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
// @js plugins/output/popup.js
// ==/ClosureCompiler==
//





// This editor is based on the execCommand() function available in the
// JavaScript environment of browsers.
//
// This version makes use of jQuery 1.11+ to access objects. It probably is
// compatible with a little bit older versions of jQuery.
//
// Source: https://developer.mozilla.org/en-US/docs/Rich-Text_Editing_in_Mozilla
// Source: http://msdn.microsoft.com/en-us/library/ie/ms536419%28v=vs.85%29.aspx

// Other sources for the different parts:
// Source: http://stackoverflow.com/questions/10041433/how-to-detect-when-certain-div-is-out-of-view
// Source: http://stackoverflow.com/questions/5605401/insert-link-in-contenteditable-element
// Source: http://stackoverflow.com/questions/6867519/javascript-regex-to-count-whitespace-characters

// FileReader replacement under IE9:
// Source: https://github.com/MrSwitch/dropfile

// Code verification with Google Closure Compiler:
// documentation: http://code.google.com/closure/compiler/
// Source: http://code.google.com/p/closure-compiler/
// Source: https://developers.google.com/closure/compiler/docs/error-ref
// Source: http://www.jslint.com/
//
// Command line one can use to verify the editor (jquery-for-closure.js
// may not yet be complete):
//
//   java -jar .../google-js-closure-compiler/compiler.jar \
//        --warning_level VERBOSE \
//        --js_output_file /dev/null \
//        tests/jquery-for-closure.js \
//        plugins/output/output.js \
//        plugins/editor/editor.js
//
// WARNING: output of that command is garbage as far as we're concerned
//          only the warnings are of interest.
//
// To test the compression of the script by the Google closure compiler
// (i.e. generate the .min. version) use the following parameters:
//
//   java -jar .../google-js-closure-compiler/compiler.jar \
//        --js_output_file editor.min.js
//        plugins/editor/editor.js
//
// Remember that the bare output of the closure compiler is not compatible
// with Snap!
//

/** \file
 * \brief The inline Editor of Snap!
 *
 * This file defines a set of editor features using advance JavaScript
 * classes. The resulting environment looks like this:
 *
 * \code
 * +-------------------+              +------------------+
 * |                   |              |                  |
 * |  EditorSelection  |              | EditorLinkDialog |
 * |  (all static)     |<----+        |                  |<---+
 * |                   |     |        |                  |    |
 * +-------------------+     |        +------------------+    |
 *                           |              ^                 |
 *                           | Reference    |                 |
 *                           +-----------+  | Reference       |
 *                                       |  |                 |
 * +-------------------+              +--+--+------------+    |
 * |                   |              |                  |    |
 * |   EditorBase      |              |   EditorToolbar  |    |
 * |                   |  Reference   |                  |<---+-------------------+
 * |                   |<----+--------+                  |                        |
 * +--------------+----+     |        +------------------+                        |
 *   ^            |          |                                                    |
 *   | Inherit    |          +-----------------+                                  |
 *   |            |                            |                                  |
 *   |            | Register (1,n)             |                                  |
 *   |            v                            |                                  |
 *   |     +--------------------+              |                                  |
 *   |     |                    |  Inherit     |                                  |
 *   |     |  EditorWidgetType  |<--+          |                                  |
 *   |     |                    |   |          ^                                  |
 *   |     |                    |   |          |                                  |
 *   |     +--------------------+   |          |                                  |
 *   |            ^                 |          |   +------------------+           |
 *   |            | Reference       |          |   |                  |           |
 *   |            |                 |          +---|  EditorFormBase  |           |
 *   |            |   +-------------+------+   |   |                  |           |
 *   |            |   |                    |   |   |                  |           ^
 *   |            |   | EditorWidgetType...|   |   +------------------+           |
 *   |            |   | (i.e. LineEdit,    |   |     ^            ^               |
 *   |            |   | Button, Checkmark) |   |     | Inherit    | Reference     |
 *   |            |   +--------------------+   |     |            |               |
 *   |            |                            |     |            |               |
 *   |     +------+-------------+              |     |            |               |
 *   |     |                    +--------------+     |     +------+-----------+   |
 *   |     |    EditorWidget    |                    |     |                  |   |
 *   |     |                    | Create (1,n)       |     | EditorSaveDialog |   |
 *   |     |                    |<----------+        |     |                  |   |
 *   |     +--------------------+           |        |     |                  |   |
 *   |                                      |        |     +------------------+   |
 * +-+-----------------+              +-----+--------+---+       ^                |
 * |                   |              |                  |       |                |
 * |   Editor          |              |   EditorForm     +-------+                |
 * |                   | Create (1,n) |                  | Create (1,1)           |
 * |                   +------------->|                  |                        |
 * +-----------------+-+              +--------+---------+                        |
 *       ^           |                         |                                  |
 *       |           |    Create (1,1)         |                                  |
 *       |           +-------------------------+----------------->----------------+
 *       |
 *       | Create(1,1)
 *  +----+-------------------+
 *  |                        |
 *  |   jQuery()             |
 *  |   Initialization       |
 *  +------------------------+
 * \endcode
 *
 * Note that the reference of the Toolbar from the EditorForm works
 * through the getToolbar() function of the EditorBase since the
 * toolbar object is not otherwise directly accessible.
 */


/** \brief Snap EditorSelection constructor.
 *
 * The EditorSelection is a set of functions used to deal with the selection
 * property of the different browsers (each browser has a different API at
 * the moment...)
 *
 * The functions are pretty much all considered static functions as they
 * do not need any data. Therefore this is not an object, just a set of
 * functions.
 *
 * @struct
 */
snapwebsites.EditorSelection =
{
    /** \brief Trim the selection.
     *
     * This function checks whether the selection starts and ends with
     * spaces. If so, those spaces are removed from the selection. This
     * makes for much cleaner links.
     *
     * The function takes no parameter since there can be only one current
     * selection and the system (document) knows about it.
     */
    trimSelectionText: function() // static
    {
        var range, sel, text, trimStart, trimEnd;

        if(document.selection)
        {
            range = document.selection.createRange();
            text = range.text;
            trimStart = text.match(/^\s*/)[0].length;
            if(trimStart)
            {
                range.moveStart("character", trimStart);
            }
            trimEnd = text.match(/\s*$/)[0].length;
            if(trimEnd)
            {
                range.moveEnd("character", -trimEnd);
            }
            range.select();
        }
        else
        {
            sel = window.getSelection();
            if(sel.getRangeAt)
            {
                range = sel.getRangeAt(0);
                text = range.toString();
                trimStart = text.match(/^\s*/)[0].length;
                if(trimStart && range.startContainer)
                {
                    range.setStart(range.startContainer, range.startOffset + trimStart);
                }
                trimEnd = text.match(/\s*$/)[0].length;
                if(trimEnd && range.endContainer)
                {
                    range.setEnd(range.endContainer, range.endOffset - trimEnd);
                }
            }
        }
    },

    /** \brief Retrieve the selection boundary element.
     *
     * This function determines the tag that encompasses the current
     * selection.
     *
     * This is particularly useful when editing a link and making sure that
     * the entire link is selected before you edit anything.
     *
     * \note
     * This function does not modify the selection.
     *
     * @param {boolean} isStart  Whether the start end end container should be used.
     *
     * @return {Element}  The object representing the selection boundaries.
     */
    getSelectionBoundaryElement: function(isStart) // static
    {
        var range, sel, container;

        if(document.selection)
        {
            // Note that IE offers a RangeText here, not a Range
            range = document.selection.createRange();
            if(range)
            {
                range.collapse(isStart);
                return range.parentElement();
            }
        }
        else
        {
            sel = window.getSelection();
            if(sel.getRangeAt)
            {
                if(sel.rangeCount > 0)
                {
                    range = sel.getRangeAt(0);
                }
            }
            else
            {
                // Old WebKit
                range = document.createRange();
                range.setStart(sel.anchorNode, sel.anchorOffset);
                range.setEnd(sel.focusNode, sel.focusOffset);

                // Handle the case when the selection was selected backwards (from the end to the start in the document)
                if(range.collapsed !== sel.isCollapsed)
                {
                    range.setStart(sel.focusNode, sel.focusOffset);
                    range.setEnd(sel.anchorNode, sel.anchorOffset);
                }
            }

            if(range)
            {
               container = range[isStart ? "startContainer" : "endContainer"];

               // Check if the container is a text node and return its parent if so
               return container.nodeType === 3 ? container.parentNode : container;
            }
        }

        return null;
    },

    /** \brief Set the boundary to the element.
     *
     * This function changes the selection so it matches the start and end
     * point of the specified element (tag).
     *
     * @param {Element} tag  The tag to which the selection is adjusted.
     */
    setSelectionBoundaryElement: function(tag) // static
    {
        // This works since IE9
        var range = document.createRange();
        range.setStartBefore(tag);
        range.setEndAfter(tag);
        var sel = window.getSelection();
        sel.removeAllRanges();
        sel.addRange(range);
    },

    /** \brief Retrieve the current selection.
     *
     * This function retrieves the current selection and returns it. The
     * caller is responsible to save the selection information and restore
     * it later with a call to restoreSelection().
     *
     * In general this function is used to save the selection before doing
     * an action that will mess with the current selection (i.e. open a
     * dialog which has widgets and thus a new selection.)
     *
     * @return {Object}  An object representing the current selection.
     */
    saveSelection: function() // static
    {
        var sel;

        if(document.selection)
        {
            return document.selection.createRange();
        }
        else
        {
            sel = window.getSelection();
            if(sel.getRangeAt && sel.rangeCount > 0)
            {
                return sel.getRangeAt(0);
            }
            else
            {
                return null;
            }
        }
        //NOTREACHED
    },

    /** \brief Restore a selection previously saved with the saveSelection().
     *
     * This function expects a selection as a parameter as returned by the
     * saveSelection() function.
     *
     * The caller is responsible for only restoring selections that make
     * sense. If the selection represents something that already disappeared,
     * then it should not be applied.
     *
     * \todo
     * Ameliorate the input type using the possible Range types.
     *
     * @param {Object} range  The range describing the selection to restore.
     */
    restoreSelection: function(range) // static
    {
        var sel;

        if(document.selection)
        {
            range.select();
        }
        else
        {
            sel = window.getSelection();
            sel.removeAllRanges();
            sel.addRange(/** @type {Range} */ (range));
        }
    },

    /** \brief Search for links in the selection.
     *
     * This function searches the selection for links and returns an array
     * of all the links. It may return an empty array.
     *
     * @return {Array.<string>}  The list of links found in the current selection.
     */
    getLinksInSelection: function() // static
    {
        var i, r, sel, selectedLinks = [];
        var range, containerEl, links, linkRange;
        if(window.getSelection)
        {
            sel = window.getSelection();
            if(sel.getRangeAt && sel.rangeCount)
            {
                linkRange = document.createRange();
                for(r = 0; r < sel.rangeCount; ++r)
                {
                    range = sel.getRangeAt(r);
                    containerEl = range.commonAncestorContainer;
                    if(containerEl.nodeType != 1)
                    {
                        containerEl = containerEl.parentNode;
                    }
                    if(containerEl.nodeName.toLowerCase() == "a")
                    {
                        selectedLinks.push(containerEl);
                    }
                    else
                    {
                        links = containerEl.getElementsByTagName("a");
                        for(i = 0; i < links.length; ++i)
                        {
                            linkRange.selectNodeContents(links[i]);
                            if(linkRange.compareBoundaryPoints(range.END_TO_START, range) < 1
                            && linkRange.compareBoundaryPoints(range.START_TO_END, range) > -1)
                            {
                                selectedLinks.push(links[i]);
                            }
                        }
                    }
                }
                linkRange.detach();
            }
        }
        else if(document.selection && document.selection.type != "Control")
        {
            range = document.selection.createRange();
            containerEl = range.parentElement();
            if(containerEl.nodeName.toLowerCase() == "a")
            {
                selectedLinks.push(containerEl);
            }
            else
            {
                links = containerEl.getElementsByTagName("a");
                linkRange = document.body.createTextRange();
                for(i = 0; i < links.length; ++i)
                {
                    linkRange.moveToElementText(links[i]);
                    if(linkRange.compareEndPoints("StartToEnd", range) > -1 && linkRange.compareEndPoints("EndToStart", range) < 1)
                    {
                        selectedLinks.push(links[i]);
                    }
                }
            }
        }
        return selectedLinks;
    },

    /** \brief Get the text currently selected.
     *
     * This function gets the text currently selected.
     *
     * \note
     * This function does not verify that the text that is currently
     * selected corresponds to the element currently marked as the
     * current widget in the editor. However, if everything goes
     * according to plan, it will always be the case.
     *
     * \todo
     * Ameliorate the return type using the proper Range definitions.
     *
     * @return {string}  The text currently selected.
     */
    getSelectionText: function() // static
    {
        var range;
        if(document.selection)
        {
            range = document.selection.createRange();
            return range.text;
        }

        var sel = window.getSelection();
        if(sel.getRangeAt)
        {
            range = sel.getRangeAt(0);
            return range.toString();
        }

        return '';
    }

//pasteHtmlAtCaret: function(html, selectPastedContent)
//{
//    var sel, range;
//    if(window.getSelection)
//    {
//        // IE9 and non-IE
//        sel = window.getSelection();
//        if(sel.getRangeAt && sel.rangeCount)
//        {
//            range = sel.getRangeAt(0);
//            range.deleteContents();
//
//            // Range.createContextualFragment() would be useful here but is
//            // only relatively recently standardized and is not supported in
//            // some browsers (IE9, for one)
//            var el = document.createElement("div");
//            el.innerHTML = html;
//            var frag = document.createDocumentFragment(), node, lastNode;
//            while((node = el.firstChild))
//            {
//                lastNode = frag.appendChild(node);
//            }
//            var firstNode = frag.firstChild;
//            range.insertNode(frag);
//
//            // Preserve the selection
//            if(lastNode)
//            {
//                range = range.cloneRange();
//                range.setStartAfter(lastNode);
//                if(selectPastedContent)
//                {
//                    range.setStartBefore(firstNode);
//                }
//                else
//                {
//                    range.collapse(true);
//                }
//                sel.removeAllRanges();
//                sel.addRange(range);
//            }
//        }
//    }
//    else if((sel = document.selection) && sel.type != "Control")
//    {
//        // IE < 9
//        var originalRange = sel.createRange();
//        originalRange.collapse(true);
//        sel.createRange().pasteHTML(html);
//        if(selectPastedContent)
//        {
//            range = sel.createRange();
//            range.setEndPoint("StartToStart", originalRange);
//            range.select();
//        }
//    }
//},

//    findFirstVisibleTextNode_: function(p)
//    {
//        var textNodes, nonWhitespaceMatcher = /\S/;
//
//        function findFirstVisibleTextNode__(p)
//        {
//            var i, max, result, child;
//
//            // text node?
//            if(p.nodeType == 3) // Node.TEXT_NODE == 3
//            {
//                return p;
//            }
//
//            max = p.childNodes.length;
//            for(i = 0; i < max; ++i)
//            {
//                child = p.childNodes[i];
//
//                // verify visibility first (avoid walking the tree of hidden nodes)
////console.log("node " + child + " has display = [" + jQuery(child).css("display") + "]");
////                if(jQuery(child).css("display") == "none")
////                {
////                    return result;
////                }
//
//                result = findFirstVisibleTextNode__(child);
//                if(result)
//                {
//                    return result;
//                }
//            }
//        }
//
//        return findFirstVisibleTextNode__(p);
//    },

//    resetSelectionText: function()
//    {
//        var range, selection;
//        if(document.selection)
//        {
//            range = document.selection.createRange();
//            range.moveStart("character", 0);
//            range.moveEnd("character", 0);
//            range.select();
//            return;
//        }
//
//console.log("active is ["+this.activeElement_+"]");
//        var elem=this.findFirstVisibleTextNode_(this.activeElement_);//jQuery(this.activeElement_).filter(":visible:text:first");
//console.log("first elem = ["+this.activeElement_+"/"+jQuery(elem).length+"] asis:["+elem+"] text:["+elem.nodeValue+"] node:["+jQuery(elem).prop("nodeType")+"]");
////jQuery(this.activeElement_).find("*").filter(":visible").each(function(i,e){
////console.log(" + child = ["+this+"] ["+jQuery(this).prop("tagName")+"] node:["+jQuery(this).prop("nodeType")+"]");
////});
//
//        range = document.createRange();//Create a range (a range is a like the selection but invisible)
//        range.selectNodeContents(elem);//this.activeElement_);//Select the entire contents of the element with the range
//        //range.collapse(true);//collapse the range to the end point. false means collapse to end rather than the start
//        selection = window.getSelection();//get the selection object (allows you to change selection)
//        selection.removeAllRanges();//remove any selections already made
//        selection.addRange(range);//make the range you have just created the visible selection
//        return;
//
//
//        var sel = window.getSelection();
//        if(sel.getRangeAt)
//        {
//            range = sel.getRangeAt(0);
//console.log('Current position: ['+range.startOffset+'] inside ['+jQuery(range.startContainer).attr("class")+']');
//            range.setStart(this.activeElement_, 0);
//            range.setEnd(this.activeElement_, 0);
//        }
//    },

};



/** \brief Snap EditorWidgetTypeBase constructor.
 *
 * The editor works with widgets that are based on this class. Each widget
 * is given a type such as "line-edit".
 *
 * To make it fully dynamic, we define a base class here and let other
 * programmers add new widget types in their own .js files by extending
 * this class.
 *
 * Classes must be registered with the EditorBase class function:
 *
 * \code
 *    snapwebsites.EditorInstance.registerWidgetType(your_widget_type);
 * \endcode
 *
 * This base class already implements a few things that are common to
 * all widgets.
 *
 * @constructor
 * @struct
 */
snapwebsites.EditorWidgetTypeBase = function()
{
    this.constructor = snapwebsites.EditorWidgetTypeBase;

    // TBD
    // Maybe at some point we'd want to create yet another layer
    // so we can have an auto-register, but I'm not totally sure
    // that would really work right in all cases...
    //snapwebsites.EditorBase.registerWidgetType(this);

    return this;
};


/** \brief The type of parameter one can pass to the save functions.
 *
 * The result is saved in a data object which fields are:
 *
 * \li html -- the data being saved, used to change the originalData_
 *               field once successfully saved.
 * \li result -- the data to send to the server.
 *
 * @typedef {{html: string, result: string}}
 */
snapwebsites.EditorWidgetTypeBase.SaveData;


/** \brief The prototype of the EditorWidgetTypeBase base class.
 *
 * This object defines the fields and functions offered in the editor
 * widget type object.
 *
 * It cannot reference any of the other objects (not "legally" at least).
 *
 * @struct
 */
snapwebsites.EditorWidgetTypeBase.prototype =
{
    /** \brief The constructor of this widget.
     *
     * The constructor one can use to build this type of object.
     *
     * @type {!function()}
     */
    constructor: snapwebsites.EditorWidgetTypeBase,

    /** \brief Retrieve the name of this widget type.
     *
     * This function returns the widget name. It is used whenever you
     * register the type in the snap editor object.
     *
     * \return The type of this editor widget type as a string.
     */
    getType: function() // virtual
    {
        throw Error("snapwebsites.EditorWidgetTypeBase.getType() was not overloaded.");
    },

    /** \brief Initialize a widget of this type.
     *
     * The parameter is really a snapwebsites.EditorWidget object,
     * but at this point we did not yet define that object.
     *
     * @param {!Object} editor_widget  The widget being initialized.
     */
    initializeWidget: function(editor_widget) // virtual
    {
        throw Error("snapwebsites.EditorWidgetTypeBase.initializeWidget() doesn't do anything (yet)");
    },

    /** \brief Allow for special handling of the widget data when saving.
     *
     * This function is called whenever the data of a widget is to be
     * sent to the server.
     *
     * @param {!Object} editor_widget  The concerned widget
     * @param {!Object.<snapwebsites.EditorWidgetTypeBase.SaveData>} data  The data to be saved.
     */
    saving: function(editor_widget, data)
    {
        // by default we do nothing (the defaults created by the
        // snapwebsites.EditorWidget.saving() function are good enough.)
    }
};



/** \brief Snap EditorBase constructor.
 *
 * The base editor includes everything that the toolbar and the form objects
 * need to reference in the editor although some of those parameters are
 * created by the main editor object (i.e. the toolbar and forms are created
 * by the main editor object.)
 *
 * \return The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.EditorBase = function()
{
//#ifdef DEBUG
    if(jQuery("body").hasClass("snap-editor-initialized"))
    {
        throw Error("Only one editor singleton can be created.");
    }
    jQuery("body").addClass("snap-editor-initialized");
//#endif

    this.constructor = snapwebsites.EditorBase;

    return this;
};


/** \brief The prototype of the EditorBase.
 *
 * This object defines the fields and functions offered in the editor
 * base object.
 *
 * It cannot reference most of the other objects (not "legally" at least).
 * It knows about widget types though.
 *
 * @struct
 */
snapwebsites.EditorBase.prototype =
{
    /** \brief The constructor of this widget.
     *
     * The constructor one can use to build this type of object.
     *
     * @type {!function()}
     */
    constructor: snapwebsites.EditorBase,

    /** \brief The currently active element.
     *
     * The element considered active in the editor is the very element
     * that gets the focus.
     *
     * When no elements are focused, it is expected to be null. However,
     * our current code may not always properly reset it.
     *
     * @private
     */
    activeElement_: null,

    /** \brief The list of widget types understood by the editor.
     *
     * This array is used to save the widget types when you call the
     * registerWidgetType() function.
     *
     * @type {Array.<snapwebsites.EditorWidgetType>}
     * @private
     */
    widgetTypes_: [],

    /** \brief Retrieve the toolbar object.
     *
     * This function returns a reference to the toolbar object.
     *
     * \exception
     * This function raises an error exception if called directly (i.e. the
     * funtion is virtual).
     *
     * \return The toolbar object.
     */
    getToolbar: function() // virtual
    {
        throw new Error("getToolbar() cannot directly be called on the EditorBase class.");
    },

    /** \brief Define the active element.
     *
     * Whenever an editor object receives the focus, this function is called
     * with that widget. The element is expected to be a valid jQuery object.
     *
     * Note that the blur() event cannot directly set the active element to
     * \em null because at that point the toolbar may have been clicked and
     * in that case we don't want to really lose the focus...
     *
     * \param element  A jQuery element representing the focused object.
     */
    setActiveElement: function(element)
    {
//#ifdef DEBUG
        if(!(element instanceof jQuery))
        {
            throw new Error("setActiveElement() must be called with a jQuery object.");
        }
//#endif
        this.activeElement_ = element;
    },

    /** \brief Retrieve the currently active element.
     *
     * This function returns a reference to the currently focused element.
     *
     * @return {jQuery} The DOM element with the focus.
     */
    getActiveElement: function()
    {
        //jQuery(snapwebsites.EditorInstance.activeElement_)
        return this.activeElement_;
    },

    /** \brief Refocus the active element.
     *
     * This function is used by the toolbar to refocus the currently
     * active element because when one clicks on the toolbar, the focus
     * is lost from the active element.
     */
    refocus: function()
    {
        if(this.activeElement_)
        {
            this.activeElement_.focus();
        }
    },

    /** \brief Check whether a field was modified.
     *
     * When something may have changed (a character may have been inserted
     * or deleted, or a text replaced) then you are expected to call this
     * function in order to see whether something was indeed modified.
     *
     * When the process detects that something was modified, it calls the
     * necessary functions to open the Save Dialog.
     *
     * As a side effect it also lets the toolbar know so if it needs to be
     * moved, it happens.
     *
     * @throws {Error}  The base class throws.
     */
    checkModified: function() // virtual
    {
        throw new Error("checkModified() cannot directly be called on the EditorBase class.");
    },

    /** \brief Retrieve the link dialog.
     *
     * This function creates an instance of the link dialog and returns it.
     * If the function gets called more than once, then the same reference
     * is returned.
     *
     * \return The link dialog reference.
     *
     * @throws {Error}  The base class implementation just throws.
     */
    getLinkDialog: function()
    {
        throw new Error("getLinkDialog() cannot directly be called on the EditorBase class.");
    },

    /** \brief Register a widget type.
     *
     * This function is used to register a widget type in the editor.
     * This allows for any number of extensions and thus any number of
     * cool advanced features that do not all need to be defined in the
     * core of the editor.
     *
     * @param {snapwebsites.EditorWidgetType} widget_type  The widget type to register.
     */
    registerWidgetType: function(widget_type)
    {
        var name = widget_type.getType();
        this.widgetTypes_[name] = widget_type;
    },

    /** \brief Check whether a type exists.
     *
     * This function checks the list of widget types to see whether
     * \p type_name exists.
     *
     * @param {!string} type_name  The name of the type to check.
     *
     * @return {!boolean}  true if the type is defined.
     */
    hasWidgetType: function(type_name)
    {
        return this.widgetTypes_[type_name] instanceof snapwebsites.EditorWidgetTypeBase;
    },

    /** \brief This function is used to get a widget type.
     *
     * Widget types get registered with the registerWidgetType() function.
     * Later you may retrieve them using this function.
     *
     * @throws {Error} If the named \p type was not yet registered, then this
     *                 function throws.
     *
     * @param {string} type_name  The name of the widget type to retrieve.
     *
     * @return {snapwebsites.EditorWidgetType} The widget type object.
     */
    getWidgetType: function(type_name)
    {
        if(!(this.widgetTypes_[type_name] instanceof snapwebsites.EditorWidgetTypeBase))
        {
            throw new Error("getWidgetType() of type \"" + type_name + "\" is not yet defined, you cannot get it now.");
        }
        return this.widgetTypes_[type_name];
    }
};


/** \brief Check whether a block of HTML is empty or not.
 *
 * This function checks the HTML in the specified \p html parameter
 * and after cleaning up the string, returns true if it ends up
 * being empty.
 *
 * @param {string|jQuery} html
 *
 * @return {boolean}  true if the string can be considered empty.
 */
snapwebsites.EditorBase.isEmptyBlock = function(html) // static
{
//#ifdef DEBUG
    if(typeof html != "string")
    {
        throw Error("snapwebsites.EditorBase.isEmptyBlock() called with a parameter which is not a string (" + (typeof html) + ")");
    }
//#endif

    //
    // replace all the nothingness by ""
    // and see whether the result is the empty string
    //
    // WARNING: in the following, if html is empty on entry then
    //          the result is still true but just a match against
    //          the regex would return false on the empty string
    //
    return html.replace(/^(<br *\/?>| |\t|\n|\r|&nbsp;)+$/, "").length == 0;
};



/** \brief Snap EditorLinkDialog constructor.
 *
 * The editor link dialog is a popup window that is used to let the user
 * enter a link (URI, anchor, and some other parameters of the anchor.)
 *
 * @param {snapwebsites.EditorBase} editor_base  A reference to the editor
 *                                               base object.
 *
 * @return {snapwebsites.EditorLinkDialog} A reference to the object being
 *                                         initialized.
 * @constructor
 * @struct
 */
snapwebsites.EditorLinkDialog = function(editor_base)
{
    this.constructor = snapwebsites.EditorLinkDialog;
    this.editorBase_ = editor_base;

    // TODO: add support for a close without saving the changes!
    var html = "<div id='snap_editor_link_dialog'>"
             + "<div class='title'>Link Administration</div>"
             + "<div id='snap_editor_link_page'>"
             + "<div class='line'><label class='limited' for='snap_editor_link_text'>Text:</label> <input id='snap_editor_link_text' name='text' title='Enter the text representing the link. If none, the link will appear as itself.'/></div>"
             + "<div class='line'><label class='limited' for='snap_editor_link_url'>Link:</label> <input id='snap_editor_link_url' name='url' title='Enter a URL.'/></div>"
             + "<div class='line'><label class='limited' for='snap_editor_link_title'>Tooltip:</label> <input id='snap_editor_link_title' name='title' title='The tooltip which appears when a user hovers the mouse cursor over the link.'/></div>"
             + "<div class='line'><label class='limited'>&nbsp;</label><input id='snap_editor_link_new_window' type='checkbox' value='' title='Click to save your changes.'/> <label for='snap_editor_link_new_window'>New Window</label></div>"
             + "<div class='line'><label class='limited'>&nbsp;</label><input id='snap_editor_link_ok' type='button' value='OK' title='Click to save your changes.'/></div>"
             + "<div style='clear:both;padding:0;'></div></div></div>";

    jQuery(html).appendTo("body");
    this.linkDialogPopup_ = jQuery("#snap_editor_link_dialog");

    jQuery("#snap_editor_link_ok").click(this.close);

    return this;
};


/** \brief Popup dialog to let users enter a link.
 *
 * This object is created whenever a user wants to create a link. It
 * creates a popup window and put a simple form in it. The user can
 * then enter the URI, the anchor text, and whether the clicked
 * item should open in a new window.
 *
 * @struct
 */
snapwebsites.EditorLinkDialog.prototype =
{
    /** \brief The constructor of this widget.
     *
     * The constructor one can use to build this type of object.
     *
     * @type {!function(snapwebsites.EditorBase)}
     */
    constructor: snapwebsites.EditorLinkDialog,

    /** \brief The editor base to access the active widget.
     *
     * A reference to the EditorBase object. This allows us to access the
     * currently active widget and apply different functions that are
     * useful to get the editor to work (actually, all the commands... so
     * that's a great deal of the editor!)
     *
     * This parameter is always initialized as it is set when the
     * EditorLinkDialog is constructed.
     *
     * @type {snapwebsites.EditorBase}
     * @private
     */
    editorBase_: null,

    /** \brief The jQuery Link Dialog object.
     *
     * This variable holds the jQuery object representing the Link Dialog
     * DOM object. The object is created at the time the EditorLinkDialog
     * is created so as far as this object is concerned, it is pretty much
     * always available.
     *
     * @type {jQuery}
     * @private
     */
    linkDialogPopup_: null,

    /** \brief The selection range when opening a dialog.
     *
     * The selection needs to be preserved whenever we open a popup dialog
     * in the editor. This selection is saved in this variable. The
     * selection itself is gathered using the saveSelection() and later
     * restored with the restoreSelection() functions.
     *
     * @type {Object}
     * @private
     */
    selectionRange_: null,

    /** \brief Open the link dialog.
     *
     * This function opens (i.e. shows and positions) the link dialog.
     *
     * A strong side effect of this function is to darken anything
     * else in the background.
     */
    open: function()
    {
        this.selectionRange_ = snapwebsites.EditorSelection.saveSelection();

        var jtag;
        var selectionText = snapwebsites.EditorSelection.getSelectionText();
        var links = snapwebsites.EditorSelection.getLinksInSelection();
        var new_window = true;

        if(links.length > 0)
        {
            jtag = jQuery(links[0]);
            // it is already the anchor, we can use the text here
            // in this case we also have a URL and possibly a title
            jQuery("#snap_editor_link_url").val(snapwebsites.Output.castToString(jtag.attr("href")));
            jQuery("#snap_editor_link_title").val(snapwebsites.Output.castToString(jtag.attr("title")));
            new_window = jtag.attr("target") == "_blank";
        }
        else
        {
            jtag = jQuery(snapwebsites.EditorSelection.getSelectionBoundaryElement(true));

            // this is not yet the anchor, we need to retrieve the selection
            //
            // TODO detect email addresses
            if(selectionText.substr(0, 7) == "http://"
            || selectionText.substr(0, 8) == "https://"
            || selectionText.substr(0, 6) == "ftp://")
            {
                // selection is a URL so make use of it
                jQuery("#snap_editor_link_url").val(selectionText);
            }
            else
            {
                jQuery("#snap_editor_link_url").val("");
            }
            jQuery("#snap_editor_link_title").val("");
        }
        jQuery("#snap_editor_link_text").val(selectionText);
        jQuery("#snap_editor_link_new_window").prop('checked', new_window);
        var focusItem;
        if(selectionText.length == 0)
        {
            focusItem = "#snap_editor_link_text";
        }
        else
        {
            focusItem = "#snap_editor_link_url";
        }
        var pos = jtag.position();
        var height = jtag.outerHeight(true);
        this.linkDialogPopup_.css("top", pos.top + height);
        var left = pos.left - 5;
        if(left < 10)
        {
            left = 10;
        }
        this.linkDialogPopup_.css("left", left);
        this.linkDialogPopup_.fadeIn(300,function(){jQuery(focusItem).focus();});
        snapwebsites.PopupInstance.darkenPage(150);
    },

    /** \brief Close the editor link dialog.
     *
     * This function copies the link to the widget being edited and
     * closes the popup.
     */
    close: function()
    {
        snapwebsites.EditorInstance.linkDialogPopup_.fadeOut(150);
        snapwebsites.PopupInstance.darkenPage(-150);

        this.editorBase_.refocus();
        snapwebsites.EditorSelection.restoreSelection(this.selectionRange_);
        var url = jQuery("#snap_editor_link_url");
        document.execCommand("createLink", false, url.val());
        var links = snapwebsites.EditorSelection.getLinksInSelection();
        if(links.length > 0)
        {
            var jtag = jQuery(links[0]);
            var text = jQuery("#snap_editor_link_text");
            if(text.length > 0)
            {
                jtag.text(snapwebsites.Output.castToString(text.val()));
            }
            // do NOT erase the existing text if the user OKed
            // without any text

            var title = jQuery("#snap_editor_link_title");
            if(title.length > 0)
            {
                jtag.attr("title", snapwebsites.Output.castToString(title.val()));
            }
            else
            {
                jtag.removeAttr("title");
            }

            var new_window = jQuery("#snap_editor_link_new_window");
            if(new_window.prop("checked"))
            {
                jtag.attr("target", "_blank");
            }
            else
            {
                jtag.removeAttr("target");
            }
        }
    }
};



/** \brief Snap EditorToolbar constructor.
 *
 * Whenever creating the Snap! Editor a toolbar comes with it (although
 * it can be hidden, the Ctrl-T key can be used to show/hide the bar.)
 *
 * The toolbar is created as a separate object and a reference is saved
 * in each EditorForm. The bar can only be opened once for the widget
 * with the focus.
 *
 * @param {snapwebsites.EditorBase} editor_base  A reference to the editor base object.
 *
 * @return {snapwebsites.EditorToolbar} The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.EditorToolbar = function(editor_base)
{
    this.constructor = snapwebsites.EditorToolbar;

    // save the reference to the editor base
    this.editorBase_ = editor_base;

    return this;
};


/** \brief The prototype of the EditorToolbar.
 *
 * This object defines the fields and functions offered along the
 * EditorToolbar.
 *
 * @struct
 */
snapwebsites.EditorToolbar.prototype =
{
    /** \brief The constructor of this widget.
     *
     * The constructor one can use to build this type of object.
     *
     * @type {!function(snapwebsites.EditorBase)}
     */
    constructor: snapwebsites.EditorToolbar,

    /** \brief the list of buttons in the Toolbar.
     *
     * This array defines the list of buttons available in the toolbar.
     * The JavaScript code makes use of this information to generate the
     * toolbar and later to handle key presses that the editor understands.
     *
     * \note
     * This array should be a constant but actually we want to fix a few
     * things here and there depending on the browser and whatever other
     * feature. Also at some point we should offer a way for other
     * plugins to add their own buttons.
     *
     * @type {Array.<Array.<(Object|string|number|function(Array))>>}
     * @private
     */
    toolbarButtons_: [
        // TODO: support for translations, still need to determine how we
        //       want to do that, at this point I think it should be
        //       separate .js files and depending on the language, the user
        //       includes the right file... (i.e. editor-1.2.3-en.js)
        //
        // TODO: support a way for other plugins to add (remove?) functions
        //       in a clean way, at this point inserting in this array would
        //       not be clean at all...

        //
        // WARNING: Some control keys cannot be used under different browsers
        //          (especially Internet Explorer which does not care much
        //          whether you try to capture those controls.)
        //
        //    . Ctrl-O -- open a new URL
        //

        // Structure:
        //
        //    command -- the exact command you use with
        //               document.execDocument(), if "|" it is a group
        //               separator; if "*" it is an internal command
        //               (not compatible with document.execDocument().)
        //
        //    title -- the title of the command, this is displayed as
        //             buttons are hovered with the mouse pointer; this
        //             entry can be null if no title is available and in
        //             that case the button is not added to the DOM toolbar
        //
        //    key & flags -- this field includes the key that activates
        //                   the command; (i.e. "Bold (Ctrl-B)"); the key
        //                   is expected to be 16 bits; the higher bits are
        //                   used as flags:
        //
        //                   0x0001:0000 -- requires the shift key
        //                   0x0002:0000 -- fix the selection so it better
        //                                  matches a link selection
        //                   0x0004:0000 -- run a dynamic callback such as
        //                                  the '_linkDialog'
        //
        //    parameter -- a parameter for the command, it may be for the
        //                 execDocument() or dynamic callback as defined
        //                 by flag 0x0004:0000.
        //
        //    parameter -- another parameter for the command, for example
        //                 the name of dynamic callback when flag
        //                 0x0004:0000 is set
        //

        ["bold", "Bold (Ctrl-B)", 0x42],
        ["italic", "Italic (Ctrl-I)", 0x49],
        ["underline", "Underline (Ctrl-U)", 0x55],
        ["strikeThrough", "Strike Through (Ctrl-Shift--)", 0x100AD],
        ["removeFormat", "Remove Format (Ctlr-Shift-Delete)", 0x1002E],
        ["|", "|"],
        ["subscript", "Subscript (Ctrl-Shift-B)", 0x10042],
        ["superscript", "Superscript (Ctrl-Shift-P)", 0x10050],
        ["|", "|"],
        ["createLink", "Manage Link (Ctrl-L)", 0x6004C, "http://snapwebsites.org/", snapwebsites.EditorToolbar.prototype.callbackLinkDialog_],
        ["unlink", "Remove Link (Ctrl-K)", 0x2004B],
        ["|", "-"],
        ["insertUnorderedList", "Bulleted List (Ctrl-Q)", 0x51],
        ["insertOrderedList", "Numbered List (Ctrl-Shift-O)", 0x1004F],
        ["|", "|"],
        ["outdent", "Decrease Indent (Ctrl-Up)", 0x26],
        ["indent", "Increase Indent (Ctrl-Down)", 0x28],
        ["formatBlock", "Block Quote (Ctrl-Shift-Q)", 0x10051, "<blockquote>"],
        ["insertHorizontalRule", "Horizontal Line (Ctrl-H)", 0x48],
        ["insertFieldset", "Fieldset (Ctrl-Shift-S)", 0x10053],
        ["|", "|"],
        ["justifyLeft", "Left Align (Ctrl-Shift-L)", 0x1004C],
        ["justifyCenter", "Centered (Ctrl-E)", 0x45],
        ["justifyRight", "Right Align (Ctrl-Shift-R)", 0x10052],
        ["justifyFull", "Justified (Ctrl-J)", 0x4A],

        // no buttons,  just the keyboard at this point
        ["formatBlock", null, 0x31, "<h1>"],        // Ctrl-1
        ["formatBlock", null, 0x32, "<h2>"],        // Ctrl-2
        ["formatBlock", null, 0x33, "<h3>"],        // Ctrl-3
        ["formatBlock", null, 0x34, "<h4>"],        // Ctrl-4
        ["formatBlock", null, 0x35, "<h5>"],        // Ctrl-5
        ["formatBlock", null, 0x36, "<h6>"],        // Ctrl-6
        ["fontSize", null, 0x10031, "1"],           // Ctrl-Shift-1
        ["fontSize", null, 0x10032, "2"],           // Ctrl-Shift-2
        ["fontSize", null, 0x10033, "3"],           // Ctrl-Shift-3
        ["fontSize", null, 0x10034, "4"],           // Ctrl-Shift-4
        ["fontSize", null, 0x10035, "5"],           // Ctrl-Shift-5
        ["fontSize", null, 0x10036, "6"],           // Ctrl-Shift-6
        ["fontSize", null, 0x10037, "7"],           // Ctrl-Shift-7
        ["fontName", null, 0x10046, "Arial"],       // Ctrl-Shift-F -- TODO add font selector
        ["foreColor", null, 0x52, "red"],           // Ctrl-R -- TODO add color selector
        ["hiliteColor", null, 0x10048, "#ffff00"],  // Ctrl-Shift-H -- TODO add color selector
        ["*", null, 0x54, "", snapwebsites.EditorToolbar.prototype.callbackToggleToolbar_]            // Ctrl-T
    ],

    /** \brief The editor base to access the active widget.
     *
     * A reference to the EditorBase object. This allows us to access the
     * currently active widget and apply different functions that are
     * useful to get the editor to work (actually, all the commands... so
     * that's a great deal of the editor!)
     *
     * This parameter is always initialized as it is set when the
     * EditorToolbar is constructed.
     *
     * @type {snapwebsites.EditorBase}
     * @private
     */
    editorBase_: null,

    /** \brief The list of keys understood by the toolbar.
     *
     * Each toolbar button (and non-buttons) can have a key assigned to it.
     * The keys_ variable is a map of those keys to very quickly find the
     * command that corresponds to a key. This table is generated at the
     * time the toolbar is initialized.
     *
     * @type {Array.<number>}
     * @private
     */
    keys_: [],

    /** \brief The jQuery toolbar
     *
     * This is the jQuery object representing the toolbar (a \<div\>
     * tag in the DOM.)
     *
     * @type {jQuery}
     * @private
     */
    toolbar_: null,

    /** \brief Whether the toolbar is currently visible.
     *
     * The toolbar can be shown with:
     *
     * \code
     * toggleToolbar(true);
     * \endcode
     *
     * The toolbar can be hidden with:
     *
     * \code
     * toggleToolbar(false);
     * \endcode
     *
     * The toolbar can be toggled from visible to not visible with:
     *
     * \code
     * toggleToolbar();
     * \endcode
     *
     * This flag represents the current state.
     *
     * Note, however, that the toolbar may be fading in or out. In
     * that case this flag already reflects the final state.
     *
     * @type {boolean}
     * @private
     */
    toolbarVisible_: false,

    /** \brief Whether the toolbar is shown at the bottom of the widget.
     *
     * When there isn't enough room to put the toolbar at the top, this
     * flag is set to true. The default is rather meaningful.
     *
     * @type {boolean}
     * @private
     */
    bottomToolbar_: false,

    /** \brief Current height of the widget the toolbar is open for.
     *
     * This value represents the height of the widget currently having
     * the focus. The height is saved as it is a lot faster to access
     * it in this way than retrieving it each time. It also gives us
     * the ability to detect that the height changed and adjust the
     * position of the toolbar if necessary.
     *
     * @type {number}
     * @private
     */
    height_: -1,

    /** \brief The identifier of the last timer created.
     *
     * Whenever the current widget loses focus, a timer is used to
     * eventually hides the toolbar. The problem is that when someone
     * clicks on the toolbar, the focus gets lost, so just closing
     * the toolbar at that point is not a good idea.
     *
     * @type {number}
     */
    toolbarTimeoutID_: -1,

    /** \brief Call this function whenever the toolbar is about to be accessed.
     *
     * This function is called whenever the EditorToolbar object is about to
     * access the toolbar DOM. This allows the system to create the toolbar
     * only once required. This is whenever an editor key (i.e. Ctrl-B) is
     * hit or if the toolbar is to be shown.
     *
     * @private
     */
    createToolbar_: function()
    {
        var that = this, msie, html, originalName, isGroup, idx, max;

        if(!this.toolbar_)
        {
            msie = /msie/.exec(navigator.userAgent.toLowerCase()); // IE?
            html = "<div id=\"toolbar\">";
            max = this.toolbarButtons_.length;

            for(idx = 0; idx < max; ++idx)
            {
                // the name of the image always uses the original name
                originalName = this.toolbarButtons_[idx][0];
                if(msie)
                {
                    if(this.toolbarButtons_[idx][0] == "hiliteColor")
                    {
                        this.toolbarButtons_[idx][0] = "backColor";
                    }
                }
                else
                {
                    if(this.toolbarButtons_[idx][0] == "insertFieldset")
                    {
                        this.toolbarButtons_[idx][0] = "insertHTML";
                        this.toolbarButtons_[idx][3] = "<fieldset><legend>Fieldset</legend><p>&nbsp;</p></fieldset>";
                    }
                }
                isGroup = this.toolbarButtons_[idx][0] == "|";
                if(!isGroup)
                {
                    this.keys_[this.toolbarButtons_[idx][2] & 0x1FFFF] = idx;
                }
                if(this.toolbarButtons_[idx][1] != null)
                {
                    if(isGroup)
                    {
                        if(this.toolbarButtons_[idx][1] == "-")
                        {
                            // horizontal separator, create a new line
                            html += "<div class=\"horizontal-separator\"></div>";
                        }
                        else
                        {
                            // vertical separator, show a small vertical bar
                            html += "<div class=\"group\"></div>";
                        }
                    }
                    else
                    {
                        // Buttons
                        html += "<div unselectable=\"on\" class=\"button "
                                + originalName
                                + "\" button-id=\"" + idx + "\" title=\""
                                + this.toolbarButtons_[idx][1] + "\">"
                                + "<span class=\"image\"></span></div>";
                    }
                }
            }
            html += "</div>";
            jQuery(html).appendTo("body");
            this.toolbar_ = jQuery("#toolbar");

            this.toolbar_
                .click(function(e){
                    that.editorBase_.refocus();
                    e.preventDefault();
                })
                .mousedown(function(e){
                    // XXX: this needs to be handled through a form of callback
                    snapwebsites.EditorInstance.cancelToolbarHide();
                    e.preventDefault();
                })
                .find(":any(.horizontal-separator .group)")
                    .click(function(){
                        that.editorBase_.refocus();
                    });
            this.toolbar_
                .find(".button")
                    .click(function(){
                        var idx = this.attr("button-id");
                        that.editorBase_.refocus();
                        that.command(idx);
                    });
        }
    },

    /** \brief Execute a toolbar command.
     *
     * This function expects the index of the command to execute. The
     * command index is the index of the command in the toolbarButtons_
     * array.
     *
     * @throws {Error} In debug version, raise this error if something invalid
     *                 is detected at runtime.
     *
     * @param {number} idx  The index of the command to execute.
     *
     * @return {boolean}  Return true to indicate that the command was
     *                    processed.
     */
    command: function(idx)
    {
        var tag, callback;

        // command is defined?
        if(!this.toolbarButtons_[idx])
        {
            return false;
        }

console.log("run command "+idx+" "+this.toolbarButtons_[idx][2]+"!!!");

        // require better selection for a link? (i.e. full link)
        if(this.toolbarButtons_[idx][2] & 0x20000)
        {
            snapwebsites.EditorSelection.trimSelectionText();
            tag = snapwebsites.EditorSelection.getSelectionBoundaryElement(true);
            if(jQuery(tag).prop("tagName") == "A")
            {
                // if we get here the whole tag was not selected,
                // select it now
                snapwebsites.EditorSelection.setSelectionBoundaryElement(tag);
            }
        }

        if(this.toolbarButtons_[idx][0] == "*"
        || this.toolbarButtons_[idx][2] & 0x40000)
        {
            // "internal command" which does not return immediately
            callback = this.toolbarButtons_[idx][4];
//#ifdef DEBUG
            if(typeof callback != "function")
            {
                throw Error("snapwebsites.Editor.command() callback function \"" + callback + "\" is not a function.");
            }
//#endif
            callback.apply(this, this.toolbarButtons_[idx]);

            if(this.toolbarButtons_[idx][0] == "*")
            {
                // the dialog OK button will do the rest of the work as
                // required by this specific command
                return true;
            }
        }
        else
        {
            // TODO: We probably want to transform all of those calls
            //       as callbacks just so it is uniform and clean...
            //       (even if the callbacks end up using execCommand()
            //       pretty much as is.)
            //
            // if there is a toolbar parameter, make sure to pass it along
            if(this.toolbarButtons_[idx][3])
            {
                // TODO: need to define the toolbar parameter
                //       (i.e. a color, font name, size, etc.)
                document.execCommand(snapwebsites.Output.castToString(this.toolbarButtons_[idx][0]), false, this.toolbarButtons_[idx][3]);
            }
            else
            {
                document.execCommand(snapwebsites.Output.castToString(this.toolbarButtons_[idx][0]), false, null);
            }
        }
        this.editorBase_.checkModified();

        return true;
    },

    /** \brief Process a keydown event.
     *
     * This function checks whether the key that was just pressed represents
     * a command, if so execute it.
     *
     * The function returns true if the key was used by the toolbar, which
     * means it should not be used by anything else afterward.
     *
     * \note
     * The function only makes use of keys if the Control key was pushed
     * down.
     *
     * @param {Event} e  The jQuery event object.
     *
     * @return {boolean}  If true, the key was used up by the toolbar process.
     */
    keydown: function(e)
    {
        if(e.ctrlKey)
        {
            return this.command(this.keys_[e.which + (e.shiftKey ? 0x10000 : 0)]);
        }
        return false;
    },

    /** \brief Check whether the toolbar should be moved.
     *
     * As the user enters data in the focused widget, the toolbar may need to
     * be adjusted.
     *
     * There are several possibilities as defined here:
     *
     * 1. The toolbar fits above the widget and is visible, place it there;
     *
     * 2. The toolbar doesn't fit above, place it at the bottom;
     *
     * 3. If the widget grows, make sure that the toolbar remains at the
     *    bottom if it doesn't fit at the top;
     *
     * 4. As the page is scrolled up and down, make sure that the toolbar
     *    remains visible as required for proper editing.
     *
     * \todo
     * Currently our positioning is extremely limited and does not work in
     * all cases. We'll have to do updates once we have the time to fix
     * all the problems. Also, we want to look into creating one function
     * to calculate the position, and then this and the toggleToolbar()
     * functions can make sure to place th object at the right location.
     * Also we may want to offer the user ways to select the toolbar
     * location (and shape) so on long pages it could be at the top, the
     * bottom or a side.
     */
    checkPosition: function()
    {
        var newHeight, pos;

        if(snapwebsites.EditorInstance.bottomToolbar_)
        {
            newHeight = jQuery(this).outerHeight();
            if(newHeight != snapwebsites.EditorInstance.height_)
            {
                snapwebsites.EditorInstance.height_ = newHeight;
                pos = jQuery(this).position();
                snapwebsites.EditorInstance.toolbar_.animate({top: pos.top + jQuery(this).height() + 3}, 200);
            }
        }
    },

    /** \brief Show, hide, or toogle the toolbar visibility.
     *
     * This function is used to show (true), hide (false), or toggle
     * (undefined, do not specify a parameter) the visibility of the
     * toolbar.
     *
     * The function is responsible for placing the toolbar at the right
     * place so it is visible and does not obstruct the field being
     * edited.
     *
     * \todo
     * Ameliorate the positioning of the toolbar, and especially, make sure
     * it remains visible even when editing very tall or wide widgets (i.e.
     * the body of a page can span many \em visible pages.) See the
     * checkPosition() function and the fact that we want ONE common function
     * to calculate the position requirements.
     *
     * @param {boolean=} force  Whether to show (true), hide (false),
     *                         or toggle (undefined) the visibility.
     */
    toggleToolbar: function(force)
    {
        var toolbarHeight, snap_editor_element, pos, widget;

        if(force === true || force === false)
        {
            this.toolbarVisible_ = force;
        }
        else
        {
            this.toolbarVisible_ = !this.toolbarVisible_;
        }

        if(this.toolbarVisible_)
        {
            // make sure to cancel the fade out in case it is active
            this.cancelToolbarHide();

            // in this case we definitvely need to have a toolbar, create it
            this.createToolbar_();

            // start showing the bar, then place it (when display is none,
            // the size is incorrect, calling fadeIn() first fixes that
            // problem.)
            this.toolbar_.fadeIn(300);
            widget = this.editorBase_.getActiveElement();
            snap_editor_element = widget.parents('.snap-editor');
            this.height_ = snapwebsites.Output.castToNumber(snap_editor_element.height());
            toolbarHeight = this.toolbar_.outerHeight();
            pos = snap_editor_element.position();
            this.bottomToolbar_ = pos.top < toolbarHeight + 5;
            if(this.bottomToolbar_)
            {
                // too low, put the toolbar at the bottom
                this.toolbar_.css("top", (pos.top + this.height_ + 3) + "px");
            }
            else
            {
                this.toolbar_.css("top", (pos.top - toolbarHeight - 3) + "px");
            }
            this.toolbar_.css("left", (pos.left + 5) + "px");
        }
        else if(this.toolbar_) // if no toolbar anyway, exit
        {
            // since we're hiding it now, cancel the toolbar timer
            this.cancelToolbarHide();

            // make sure it is gone
            this.toolbar_.fadeOut(150);
        }
    },

    /** \brief ToggleToobar callback to support the Ctrl-T key.
     *
     * This function is called when the user hits the Ctrl-T key. It toggles
     * the visibility of the toolbar.
     *
     * @param {string} cmd  The command that generated this call.
     * @param {?string} title  The command that generated this call.
     * @param {number} key_n_flags  The command that generated this call.
     * @param {string|number|null} param_opt  The command that generated this call.
     * @param {function(this: snapwebsites.EditorToolbar, string, ?string, number, (string|number|null), function())} func_opt  The command that generated this call.
     */
    callbackToggleToolbar_: function(cmd, title, key_n_flags, param_opt, func_opt)
    {
        this.toggleToolbar();
    },

    /** \brief Callback to define a link.
     *
     * This function is the callback that opens a popup to edit a link in
     * the editor. It makes use of the EditorLinkDialog.
     *
     * @param {string} cmd  The command that generated this call.
     * @param {?string} title  The command that generated this call.
     * @param {number} key_n_flags  The command that generated this call.
     * @param {string|number|null} param_opt  The command that generated this call.
     * @param {function(this: snapwebsites.EditorToolbar, string, ?string, number, (string|number|null), function())} func_opt  The command that generated this call.
     */
    callbackLinkDialog_: function(cmd, title, key_n_flags, param_opt, func_opt)
    {
        // for now we can ignore cmd

        this.editorBase_.getLinkDialog().open();
    },

    /** \brief Cancel the toolbar hide timer.
     *
     * When a widget loses focus we start a timer to be used to hide
     * the toolbar. This widget may get the focus again before the
     * toolbar gets completely hidden. This function can be called
     * to cancel that timer.
     */
    cancelToolbarHide: function()
    {
        if(this.toolbarTimeoutID_ != -1)
        {
            // prevent hiding of the toolbar
            clearTimeout(this.toolbarTimeoutID_);
            this.toolbarTimeoutID_ = -1;
        }
    },

    /** \brief Start the toolbar hide feature.
     *
     * This function starts a timer which, if not cancelled in time, will
     * close the toolbar.
     *
     * It is used whenever a widget loses focus.
     */
    startToolbarHide: function()
    {
        var that = this;

        this.toolbarTimeoutID_ = setTimeout(
            function()
            {
                that.toggleToolbar(false);
            },
            200);
    }
};



/** \brief Snap EditorWidget constructor.
 *
 * For each of the widget you add to your editor forms, one of these
 * is created on the client system.
 *
 * @param {snapwebsites.EditorBase} editor_base  A reference to the editor base object.
 * @param {snapwebsites.EditorForm} editor_form  A reference to the editor form object that owns this widget.
 * @param {jQuery} widget  The jQuery object representing this editor toolbar.
 *
 * @return {snapwebsites.EditorWidget} The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.EditorWidget = function(editor_base, editor_form, widget)
{
    var type = snapwebsites.Output.castToString(widget.attr("field_type"));

    // back reference for places were we cannot otherwise have this object
    widget.data("snapEditorWidget", this);

    this.constructor = snapwebsites.EditorWidget;
    this.editorBase_ = editor_base;
    this.editorForm_ = editor_form;
    this.widget_ = widget; // this is the jQuery widget (.snap-editor)
    this.widgetContent_ = widget.children(".snap-content");
    this.name_ = snapwebsites.Output.castToString(widget.attr("field_name"));
    this.originalData_ = snapwebsites.Output.castToString(this.widgetContent_.html());
    this.widgetType_ = editor_base.getWidgetType(type);
    this.checkForBackgroundValue();

    // this should be last
    this.widgetType_.initializeWidget(this);

    return this;
};


/** \brief The prototype of the EditorWidget.
 *
 * This object defines the fields and functions offered along the
 * EditorWidget.
 *
 * @struct
 */
snapwebsites.EditorWidget.prototype =
{
    /** \brief The constructor of this widget.
     *
     * The constructor one can use to build this type of object.
     *
     * @type {!function(snapwebsites.EditorBase, snapwebsites.EditorForm, jQuery)}
     */
    constructor: snapwebsites.EditorWidget,

    /** \brief The editor base object.
     *
     * The a reference back to the editor base object. This is pretty much
     * the same as the snapwebsites.EditorInstance reference.
     *
     * @type {snapwebsites.EditorBase}
     * @private
     */
    editorBase_: null,

    /** \brief The form that owns this widget.
     *
     * This a reference back to the editor form object.
     *
     * @type {snapwebsites.EditorForm}
     * @private
     */
    editorForm_: null,

    /** \brief The jQuery widget.
     *
     * This parameter represents the jQuery widget. The actual visible
     * editor widget.
     *
     * @type {jQuery}
     * @private
     */
    widget_: null,

    /** \brief The jQuery widget representing the content of the widget.
     *
     * This parameter is the DIV under the widget which is marked as the
     * "editor-content" (i.e. with the class named "editor-content").
     * This is the part that we manage in the different widget
     * implementations.
     *
     * @type {jQuery}
     * @private
     */
    widgetContent_: null,

    /** \brief The name of the widget.
     *
     * Whenever the widget is created this field is set to the name
     * of the widget (the "field_name" attribute.)
     *
     * @const
     * @type {!string}
     * @private
     */
    name_: "",

    /** \brief The original data of the widget.
     *
     * Until saved, this data is taken from the existing widget at
     * initialization time.
     *
     * @type {string}
     * @private
     */
    originalData_: "",

    /** \brief Whether the system detected that the widget was modified.
     *
     * To avoid comparing possibly very large buffers (originalData_ and
     * the widget html() data) we save the last result in this variable.
     * Once it is true, we just return true when calling the wasModified()
     * function.
     *
     * @type {boolean}
     * @private
     */
    modified_: false,

    /** \brief The type of this widget.
     *
     * The object representing the type of this widget. It is used to
     * finish the initialization of the widget (i.e. connect to the
     * signals, etc.)
     *
     * @type {snapwebsites.EditorWidgetTypeBase}
     * @private
     */
    widgetType_: null,

    /** \brief Get the name of this widget.
     *
     * On creation the widget retrieves the attribute named "field_name"
     * which contains the name of the widget (used to communicate
     * with the server.) This function returns that name.
     *
     * @return {string}  The name of the widget.
     */
    getName: function()
    {
        return this.name_;
    },

    /** \brief Check whether the widget was modified.
     *
     * This function compares the widget old and current data to see
     * whether it changed.
     *
     * @param {?boolean} recheck_opt  Whether we want to force a check of
     *                                the current HTML data with the original.
     *
     * @return {boolean}  Whether the widget was modified (true) or not
     *                    (false).
     */
    wasModified: function(recheck_opt)
    {
        if(recheck_opt || !this.modified_)
        {
            this.modified_ = this.originalData_ != this.widgetContent_.html();
        }
        return this.modified_;
    },

    /** \brief Get the data to be saved.
     *
     * This function is called whenever the widget is marked as modified
     * or the form as "always save all". It retrieves the data being
     * saved and the result to be sent to the server. The result may
     * be quite optimized, for example, in case of radio buttons, just
     * one (small) value is sent to the server.
     *
     * By default the data.result gets cleaned up so no starting or
     * ending blanks are kept. It also removes \<br\> tags that have
     * no attributes.
     *
     * When you override this function, make sure to call it first,
     * then change the data.result field only as appropriate for you
     * widget type.
     *
     * @return {snapwebsites.EditorWidgetTypeBase.SaveData}  The data to be saved.
     */
    saving: function() // virtual
    {
        var value,          // in case the widget defines a value attribute
            data = {};      // the data to be returned

        data.html = snapwebsites.Output.castToString(this.widgetContent_.html());

        value = this.widgetContent_.attr("value");
        if(typeof value !== "undefined")
        {
            // some widget save their current state as the "value" attribute
            // so we can quickly retrieve that and return it as the result
            // (note that in this case we even skip calling the widget type
            // saving() function since the value is already the final result.)
            data.result = snapwebsites.Output.castToString(value);
        }
        else
        {
            // by default the result (what is sent to the server) is the
            // same as the HTML data, but widgets are free to chnage that
            // value (i.e. checkmark widgets set it to 0 or 1).
            //
            // TBD: should we also remove <br> tags with attributes?
            // TBD: should we remove <hr> tags at the start/end too?
            data.result = snapwebsites.Output.castToString(
                              data.html.replace(/^(<br *\/?>| |\t|\n|\r|\v|\f|&nbsp;|&#160;|&#xA0;)+/, "")
                                       .replace(/(<br *\/?>| |\t|\n|\r|\v|\f|&nbsp;|&#160;|&#xA0;)+$/, "")
                          );

            this.widgetType_.saving(this, data);
        }

        return data;
    },

    /** \brief Call after a widget was saved.
     *
     * This function reset the original data and modified flags when called.
     * It is expected to be called on a successful save.
     *
     * \todo
     * Fix as we want to set the originalData_ value to what was saved
     * which may not be 100% equivalent to what is currently defined in
     * the DOM.
     *
     * @param {snapwebsites.EditorWidgetTypeBase.SaveData} data  The data that was saved.
     */
    saved: function(data)
    {
        this.originalData_ = data.html;
        this.modified_ = false;
    },

    /** \brief Retrieve the editor form object.
     *
     * This function returns the editor form object.
     *
     * @return {snapwebsites.EditorForm} The editor form object.
     */
    getEditorForm: function()
    {
        return this.editorForm_;
    },

    /** \brief Retrieve the editor base object.
     *
     * This function returns the editor base object as passed to
     * the constructor.
     *
     * @return {snapwebsites.EditorBase} The editor base object.
     */
    getEditorBase: function()
    {
        return this.editorBase_;
    },

    /** \brief Retrieve the jQuery widget.
     *
     * This function returns the jQuery widget attached to this
     * editor widget.
     *
     * @return {jQuery} The jQuery widget as passed to the constructor.
     */
    getWidget: function()
    {
        return this.widget_;
    },

    /** \brief Retrieve the jQuery widget content.
     *
     * This function returns the jQuery widget attached to this
     * editor widget which holds the content of the field represented
     * by this widget.
     *
     * This is the child of the widget with class "snap-content".
     *
     * @return {jQuery} The jQuery widget content.
     */
    getWidgetContent: function()
    {
        return this.widgetContent_;
    },

    /** \brief Check whether the background value should be shown.
     *
     * This function checks whether the background value of this widget
     * should be shown which happens when the widget is empty.
     */
    checkForBackgroundValue: function()
    {
        this.widget_.children(".snap-editor-background").toggle(snapwebsites.EditorBase.isEmptyBlock(this.widget_.html()));
    }
};



/** \brief Snap EditorFormBase constructor.
 *
 * The EditorForm inherits the EditorFormBase.
 *
 * @param {snapwebsites.EditorBase} editor_base  The base editor object.
 * @param {jQuery} form_widget  The editor form DOM in a jQuery object.
 *
 * @return {!snapwebsites.EditorFormBase}  The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.EditorFormBase = function(editor_base, form_widget)
{
    this.constructor = snapwebsites.EditorFormBase;

    this.editorBase_ = editor_base;
    this.formWidget_ = form_widget;

    return this;
};


/** \brief The prototype of the EditorForm.
 *
 * This object defines one "Editor" form. This means a form handled by the
 * editor plugin. It knows how to handle the form as a whole (i.e. it
 * different fields / widgets, and sending the changes of the data to
 * the server.)
 *
 * @struct
 */
snapwebsites.EditorFormBase.prototype =
{
    /** \brief The constructor of this widget.
     *
     * The constructor one can use to build this type of object.
     *
     * @type {!function(new: snapwebsites.EditorFormBase, snapwebsites.EditorBase, jQuery): !snapwebsites.EditorFormBase}
     */
    constructor: snapwebsites.EditorFormBase,

    /** \brief Save the data and then publish it.
     *
     * When saving data to the database, it creates a new revision nearly
     * every single time (there is one case where it does not, but that's
     * probably a bug...)
     *
     * When the user publishes, that new revision becomes the current
     * revision. Otherwise the current revision does not change. This
     * allows for revisions to be checked and eventually corrected
     * before publication of the content.
     *
     * @type {string}
     * @const
     */
    SAVE_MODE_PUBLISH: "publish",

    /** \brief Save the data in a new revision.
     *
     * Save the data entered in the page to a new revision entry. The
     * new entry is available for review, but it does not get shown
     * to people who could not edit the page in some way.
     *
     * Once everyone approaved of a page, the page can get published by
     * asking the system to show that specific revision.
     *
     * @type {string}
     * @const
     */
    SAVE_MODE_SAVE: "save",

    /** \brief Create a new branch.
     *
     * This function saves the current data in a new branch. One can
     * create a new branch to make changes to the page such as adding
     * or removing tags and yet leave the current branch as the publicly
     * visible data.
     *
     * This gives the user time to work on his new content until he's
     * ready to publish it.
     *
     * @type {string}
     * @const
     */
    SAVE_MODE_SAVE_NEW_BRANCH: "save-new-branch",

    /** \brief Save the data as a draft.
     *
     * Snap supports drafts to save the data being typed (in case the
     * browser or even the whole client computer crashes, the electricity
     * goes out, etc.)
     *
     * Drafts are special in that they get saved in a special location
     * instead of the normal revision.
     *
     * @type {string}
     * @const
     */
    SAVE_MODE_SAVE_DRAFT: "save-draft",

    /** \brief A reference to the base editor object.
     *
     * This value is a reference to the base editor object so the
     * EditorForm objects can access it.
     *
     * @type {snapwebsites.EditorBase}
     * @private
     */
    editorBase_: null,

    /** \brief The jQuery object representing this form.
     *
     * This variable represents the DOM form as a jQuery object. It is
     * given to the EditorForm on creation.
     *
     * @type {jQuery}
     * @private
     */
    formWidget_: null,

    /** \brief Retrieve the editor base object.
     *
     * This function returns the editor base object as passed to
     * the constructor.
     *
     * @return {snapwebsites.EditorBase} The editor base object.
     * @final
     */
    getEditorBase: function()
    {
        return this.editorBase_;
    },

    /** \brief Retrieve the jQuery form widget.
     *
     * This function returns the form widget.
     *
     * @return {jQuery}
     * @final
     */
    getFormWidget: function()
    {
        return this.formWidget_;
    },

    /** \brief Check whether this form is currently saving data.
     *
     * To avoid problems we prevent the editor from saving more than once
     * simultaneously.
     *
     * @return {boolean}  true if the form is currently saving data.
     * @final
     */
    isSaving: function()
    {
        return this.formWidget_.is(".editor-saving");
    },

    /** \brief Set the current saving state of the form.
     *
     * This function set the saving state to "true" or "false". This is
     * marked using the "editor-saving" class in the editor form widget.
     * That can be used to show that the form is being saved by changing
     * a color or a border.
     *
     * @param {boolean} new_status  The saving status of this editor form.
     */
    setSaving: function(new_status)
    {
        this.formWidget_.toggleClass("editor-saving", new_status);
    },

    /** \brief Save the form data.
     *
     * This function is called to save the data. It generally happens in
     * two cases:
     *
     * \li When the user clicks on of the submit button (Publish, Save,
     *     Save New Branch, and Save Draft);
     * \li When a timer times out and the auto-save to draft feature is
     *     allowed.
     *
     * The supported modes are defined as constants:
     *
     * \li SAVE_MODE_PUBLISH
     * \li SAVE_MODE_SAVE
     * \li SAVE_MODE_SAVE_NEW_BRANCH
     * \li SAVE_MODE_SAVE_DRAFT
     *
     * @throws {Error} The function throws an error if the base class
     *                 version is called (i.e. not implemented.)
     *
     * @param {string} mode  The mode used to save the data.
     */
    saveData: function(mode)
    {
        throw Error("snapwebsites.EditorFormBase.saveData() was called which means it was not properly overridden");
    }
};



/** \brief Dialog used to show different Save options.
 *
 * By default the editor offers a simple save dialog which includes
 * several buttons used to save the changes to a page. The offered
 * buttons are:
 *
 * \li Publish -- save the current data and make it an official revision
 * \li Save -- save the changes, but do not make it the official revision
 * \li Save New Branch -- save the changes in a new branch
 * \li Save Draft -- save the data as a draft (no revision in this case)
 *
 * Editor forms used as standard form most often override the default
 * DOM buttons with just a Save button which pretty much acts like
 * the Publish button.
 *
 * \todo
 * The branch the user decided to edit (i.e. with the query string
 * ...?a=edit&revision=1.2) needs to be taken in account as well.
 *
 * @param {snapwebsites.EditorFormBase}  editor_form The editor form that
 *                                       created this editor save dialog.
 *
 * @constructor
 */
snapwebsites.EditorSaveDialog = function(editor_form)
{
    this.constructor = snapwebsites.EditorSaveDialog;

    this.editorForm_ = editor_form;

    return this;
};


/** \brief The prototype of the EditorSaveDialog.
 *
 * This object defines a form that is shown as soon as some data is modified
 * in the form. By default, this object creates a DOM with 4 buttons. It
 * can, however, be over written with your own DOM in case you want other
 * a different set of buttons (or less buttons).
 *
 * @struct
 */
snapwebsites.EditorSaveDialog.prototype =
{
    /** \brief The constructor of this widget.
     *
     * The constructor one can use to build this type of object.
     *
     * @type {!function(new: snapwebsites.EditorSaveDialog, snapwebsites.EditorFormBase): !snapwebsites.EditorSaveDialog}
     */
    constructor: snapwebsites.EditorSaveDialog,

    /** \brief An editor form reference.
     *
     * This parameter holds the editor form that created this save dialog.
     * It is used to call the saveData() function on the correct editor
     * form.
     *
     * @type {snapwebsites.EditorFormBase}
     */
    editorForm_: null,

    /** \brief The jQuery of the DOM representing the save dialog.
     *
     * This parameter holds the jQuery object referencing the DOM representing
     * the save dialog.
     *
     * @type {jQuery}
     * @private
     */
    saveDialogPopup_: null,

    /** \brief Create the default save dialog DOM.
     *
     * This function creates the default dialog DOM parameters. If you
     * call the setDialogDOM() function before the dialog needs to
     * be opened, then the default DOM will never get used.
     *
     * @private
     */
    create_: function()
    {
        var that = this;
        var html = "<div id='snap_editor_save_dialog'>"
                + "<h3 class='title'>Editor</h3>"
                + "<div id='snap_editor_save_dialog_page'>"
                + "<p class='description'>You made changes to your page. Make sure to save your modifications.</p>"
                // this is wrong at this point because the current branch
                // management is more complicated...
                // (i.e. if you are editing a new branch that is not
                //       public then Publish would make that branch
                //       public and the Save would make that too?!)
                + "<p class='snap_editor_publish_p'><a class='button' id='snap_editor_publish' href='#'>Publish</a></p>"
                + "<p class='snap_editor_save_p'><a class='button' id='snap_editor_save' href='#'>Save</a></p>"
                + "<p class='snap_editor_save_new_branch_p'><a class='button' id='snap_editor_save_new_branch' href='#'>Save New Branch</a></p>"
                + "<p class='snap_editor_save_draft_p'><a class='button' id='snap_editor_save_draft' href='#'>Save Draft</a></p>"
                + "</div></div>";

        jQuery(html).appendTo("body");

        this.saveDialogPopup_ = jQuery("#snap_editor_save_dialog");

        // very simple positioning at this point
        this.saveDialogPopup_.css("left", jQuery(window).outerWidth(true) - 190);

        jQuery("#snap_editor_publish")
            .click(function(){
                that.editorForm_.saveData(snapwebsites.EditorFormBase.prototype.SAVE_MODE_PUBLISH);
            });
        jQuery("#snap_editor_save")
            .click(function(){
                alert("Save!");
            });
        jQuery("#snap_editor_save_new_branch")
            .click(function(){
                alert("Save New Branch!");
            });
        jQuery("#snap_editor_save_draft")
            .click(function(){
                that.editorForm_.saveData(snapwebsites.EditorFormBase.prototype.SAVE_MODE_SAVE_DRAFT);
            });

        // while creating a new page, the page is kept under "admin/drafts"
        // and in that case "save" and "save new branch" do not make sense
        if(jQuery("meta[name='path']").attr("content") == "admin/drafts")
        {
            jQuery(".snap_editor_save_p").hide();
            jQuery(".snap_editor_save_new_branch_p").hide();
        }
    },

    /** \brief Define the save dialog popup to use for this dialog.
     *
     * In many cases you will define your own buttons for a dialog.
     * This function let you do that. Note that the editor expects those
     * to be hidden by default. It will show them whenever necessary
     * (i.e. when something changed.)
     *
     * \todo
     * Offer ways to just disable buttons (instead of hiding them)
     * and to keep the buttons "active" but generate errors if clicked
     * while currently saving.
     */
    setPopup: function(widget)
    {
        this.saveDialogPopup_ = jQuery(widget);
    },

    /** \brief Open the save dialog.
     *
     * This function is generally called whenever the user makes a change
     * that needs to be sent to the server.
     *
     * First the function makes sure that a dialog is defined, if not it
     * initializes the default dialog.
     *
     * Then it shows the dialog.
     *
     * \note
     * This function does not change the dialog buttons status. The
     * save function is expected to call the saveDialogStatus() function
     * for that purpose.
     *
     * \todo
     * The positioning of the dialog is done at creation time so it can be
     * a problem if it is expected (for example) to not overlay the area
     * being edited.
     */
    open: function()
    {
        if(!this.saveDialogPopup_)
        {
            this.create_();
        }
        this.saveDialogPopup_.fadeIn(300).css("display", "block");
    },

    /** \brief Close the save dialog popup.
     *
     * Once a save completed, the editor form checks whether other
     * modifications were performed while saving, if not, then the
     * save dialog gets closed.
     *
     * \note
     * This function does not change the dialog buttons status. The
     * save function is expected to call the setStatus() function
     * for that purpose.
     */
    close: function()
    {
        if(this.saveDialogPopup_)
        {
            this.saveDialogPopup_.fadeOut(300);
        }
        // else throw?
    },

    /** \brief Setup the save dialog status.
     *
     * when a user clicks on a save dialog button, you should call this
     * function to disable the dialog
     *
     * \warning
     * The function throws if the popup is not yet defined. You should not
     * be able to save without the dialog having been created so that
     * should not happen. That being said, it means we cannot call this
     * function before we called the open() function.
     *
     * @param {boolean} new_status  Whether the widget is enabled (true)
     *                              or disabled (false).
     */
    setStatus: function(new_status)
    {
//#ifdef DEBUG
        // dialog even exists?
        if(!this.saveDialogPopup_)
        {
            throw Error("setStatus_() called without the dialog defined.");
        }
//#endif

        if(new_status)
        {
            this.saveDialogPopup_.parent().children("a").removeClass("disabled");
        }
        else
        {
            this.saveDialogPopup_.parent().children("a").addClass("disabled");
        }
    }
};



/** \brief Snap EditorForm constructor.
 *
 * \note
 * The Snap! EditorForm objects are created as required based on the DOM.
 * If the DOM is dynamically updated to add more forms, then it may require
 * special handling (TBD at this point) to make sure that the new forms
 * are handled by the editor.
 *
 * @param {snapwebsites.EditorBase} editor_base  The base editor object.
 * @param {jQuery} form_widget  The editor form DOM in a jQuery object.
 *
 * @return {!snapwebsites.EditorForm}  The newly created object.
 *
 * @extends {snapwebsites.EditorFormBase}
 * @constructor
 * @struct
 */
snapwebsites.EditorForm = function(editor_base, form_widget)
{
    snapwebsites.EditorForm.prototype = new snapwebsites.EditorFormBase(editor_base, form_widget);

    this.constructor = snapwebsites.EditorForm;
    this.readyWidgets_();

    return this;
};


/** \brief The constructor of this widget.
 *
 * The constructor one can use to build this type of object.
 *
 * @type {!function(new: snapwebsites.EditorForm, snapwebsites.EditorBase, jQuery): !snapwebsites.EditorForm}
 */
snapwebsites.EditorForm.prototype.constructor = snapwebsites.EditorForm;


/** \brief A map of widget types used by this form.
 *
 * This object represents a list of types that this form uses to handle
 * its widgets. Each type defines functions to handle that widget
 * as it needs to be handled and allows for easy extensions, etc.
 *
 * The map uses the name of the type as the index and true or false
 * as the value. true is used when the type is not known to be
 * provided by the system, false once it is known to be defined.
 *
 * @type {!Object}
 * @private
 */
snapwebsites.EditorForm.prototype.usedTypes_ = {};


/** \brief A jQuery array of widgets found in this form.
 *
 * This parameter is the jQuery array of widgets defined in this
 * form.
 *
 * \bug
 * At this time, dynamically adding or removing widgets is not supported.
 *
 * @type {jQuery}
 * @private
 */
snapwebsites.EditorForm.prototype.widgets_ = null;


/** \brief A jQuery array of widgets found in this form.
 *
 * This parameter is the jQuery array of widgets defined in this
 * form.
 *
 * \bug
 * At this time, dynamically adding or removing widgets is not supported.
 *
 * @type {Object.<snapwebsites.EditorWidget>}
 * @private
 */
snapwebsites.EditorForm.prototype.editorWidgets_ = null;


/** \brief Flag to know whether the widgets got initialized.
 *
 * We initialize the widgets as soon as we know that all the widget
 * types were loaded. At that point we know that the entire form
 * can be properly initialized.
 *
 * Once the flag is true, the form is fully initialized and
 * functional.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorForm.prototype.widgetInitialized_ = false;


/** \brief The popup dialog object.
 *
 * This member is the save dialog widget. It is an object
 * handling a few DOM widgets representing buttons used to
 * let the user send his work to the server.
 *
 * @type {snapwebsites.EditorSaveDialog}
 * @private
 */
snapwebsites.EditorForm.prototype.saveDialog_ = null;


/** \brief Whether all the fields should be saved.
 *
 * Whenever the save function is called, it is possible to request the
 * system to save all the fields. This is used by forms that are not
 * used to just edit data but to create forms such as a search form.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorForm.prototype.saveAll_ = false;


/** \brief Whether the toolbar is shown immediately on focus.
 *
 * This flag is used to know whether the toolbar should be shown on
 * focus. If so, the value is true (the default). You may turn this
 * value off using the setToolbarAutoVisible() function.
 *
 * @type {boolean}
 * @private
 */
snapwebsites.EditorForm.prototype.toolbarAutoVisible_ = true;


//inPopup_: false,
//lastFormId_: 0,
//lastItemId_: 0,
//uniqueId_: 0, // for images at this point
//modified_: [],
//linkDialogPopup_: null,
//savedTextRange_: null,
//savedRange_: null,
//openDropdown_: null,


/** \brief Return whether the toolbar should automatically be shown.
 *
 * When clicking in a widget, a form can automatically show the
 * toolbar corresponding to that widget. This is true by default.
 * The toolbar can be shown / hidden using the Ctlr-T key as well.
 *
 * \todo
 * We'll also want to add a close button at some point
 *
 * @return {boolean} true if the toolbar should be shown on focus.
 */
snapwebsites.EditorForm.prototype.getToolbarAutoVisible = function()
{
    return this.toolbarAutoVisible_;
};


/** \brief Change whether the toolbar should automatically be shown.
 *
 * Whenever a widget gets the focus we can automatically have the
 * toolbar popup. By default, this is true. It can be changed to
 * false using this function.
 *
 * @param {boolean} toolbar_auto_visible  Whether the toolbar should be
 *                                        shown on widget focus.
 */
snapwebsites.EditorForm.prototype.setToolbarAutoVisible = function(toolbar_auto_visible)
{
    this.toolbarAutoVisible_ = toolbar_auto_visible;
};


/** \brief Massage the title to make it a URI.
 *
 * This function transforms the characters in \p title so it can be
 * used as a segment of the URI of this page. This is quite important
 * since we use the URI to save the page.
 *
 * @param {string} title  The title to tweak.
 *
 * @return {string}  The tweaked title. It may be an empty string.
 * @private
 */
snapwebsites.EditorForm.prototype.titleToURI_ = function(title)
{
    // force all lower case
    title = title.toLowerCase();
    // replace spaces with dashes
    title = title.replace(/ +/g, "-");
    // remove all characters other than letters and digits
    title = title.replace(/[^-a-z0-9_]+/g, "");
    // remove duplicate dashes
    title = title.replace(/--+/g, "-");
    // remove dashes at the start & end
    title = title.replace(/^-+/, "");
    title = title.replace(/-+$/, "");

    return title;
};


/** \brief Save the data.
 *
 * This function checks whether the data changed or not. If it was
 * modified, or the form was setup to save all the widgets content
 * each time, then the function builds an object and sends the
 * data to the server using AJAX.
 *
 * @param {!string} mode  The mode used to save the data.
 */
snapwebsites.EditorForm.prototype.saveData = function(mode)
{
    var that = this;

    if(this.isSaving())
    {
        // TODO: translation support
        alert("You already clicked one of these buttons. Please wait until the save is over.");
        return;
    }

    // mark the form as saving, it may use CSS to show the new status
    this.setSaving(false);

    // disable buttons
    this.saveDialog_.setStatus(false);

    // TODO: add a condition coming from the DOM (i.e. we don't want
    //       to gray out the screen if the user is expected to be
    //       able to continue editing while saving)
    //       the class is nearly there (see header trying to assign body
    //       attributes), we will then need to test it here
    //       WARNING: this needs to be moved to the editor-form object
    //                instead of the body!
    snapwebsites.PopupInstance.darkenPage(150);

    var i,                              // loop index
        max = this.widgets_.length,     // max. index
        w,                              // widget being managed
        saved_data = {},                // "array" of data objects
        obj = {},                       // object to send via AJAX

        saved = [],
        edit_area,
        url,
        name,
        keep_darken_page = false,
        value;

    for(i = 0; i < max; ++i)
    {
        w = this.widgets_[i];
        if(this.saveAll_
        || w.wasModified(true))
        {
            saved_data[i] = w.saving();
            obj[w.getName()] = saved_data[i].result;
        }
    }

    // this test is not 100% correct for the Publish or Create Branch
    // buttons...
    if(!jQuery.isEmptyObject(obj))
    {
        obj["editor_save_mode"] = mode;
        obj["editor_session"] = this.formWidget_.attr("session");
        obj["editor_uri"] = this.titleToURI_( /** @type {string} */ (jQuery("[field_name='title'] .editor-content").text()));
        url = jQuery("link[rel='canonical']").attr("href");
        url = url ? url : "/";
        keep_darken_page = true;
        jQuery.ajax(url, {
            type: "POST",
            processData: true,
            data: obj,
            error: function(jqxhr, result_status, error_msg)
            {
                // TODO get the messages in the HTML and display those
                //      in a popup; later we should have one error per
                //      widget whenever a widget is specified
                alert("An error occured while posting AJAX (status: " + result_status + " / error: " + error_msg + ")");
                snapwebsites.PopupInstance.darkenPage(-150);
            },
            success: function(data, result_status, jqxhr)
            {
                var i, j, modified, element_modified, results, doc, redirect, uri;

//console.log(jqxhr);
                if(jqxhr.status == 200)
                {
                    // WARNING: success of the AJAX round trip data does not
                    //          mean that the POST was a success.
                    alert("The AJAX succeeded (" + result_status + ")");

                    // we expect exactly ONE result tag
                    results = jqxhr.responseXML.getElementsByTagName("result");
                    if(results.length == 1 && results[0].childNodes[0].nodeValue == "success")
                    {
                        // success! so it was saved and now that's the new
                        // original value and next "Save" doesn't do anything
                        modified = false;
                        for(j = 0; j < saved.length; j++)
                        {
                            i = saved[j];
                            // WARNING: DO NOT COPY edit_area.html() BECAUSE IT MAY
                            //          HAVE CHANGED SINCE THE SAVE HAPPENED
                            edit_area = jQuery("#editor-area-" + i);
                            name = edit_area.parent().attr("field_name");
                            // specialized types save their HTML in saved_data
                            that.originalData_[i] = saved_data[name];
                            element_modified = saved_data[name] != edit_area.html();
                            that.modified_[i] = element_modified;

                            modified = modified || element_modified;
                        }
                        // if not modified while processing the POST, hide
                        // those buttons
                        if(!modified)
                        {
                            that.saveDialogPopup_.fadeOut(300);
                        }

                        redirect = jqxhr.responseXML.getElementsByTagName("redirect");
                        if(redirect.length == 1)
                        {
                            // redirect the user after a successful save
                            doc = document;
                            if(that.inPopup_)
                            {
                                // TODO: we probably want to support
                                // multiple levels (i.e. a "_top" kind
                                // of a thing) instead of just one up.
                                doc = window.parent.document;
                            }
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
                        // an error occured... report it
                    }
                }
                else
                {
                    alert("The server replied with HTTP code " + jqxhr.status + " while posting AJAX (status: " + result_status + ")");
                }
                snapwebsites.PopupInstance.darkenPage(-150);
            },
            complete: function(jqxhr, result_status){
                // TODO: avoid this one if we're fading out since that
                //       would mean the user can click in these 300ms
                //       (probably generally unlikely... and it should
                //       not have any effect because we re-compare the
                //       data and do nothing if no modifications happened)
                that.saveDialogStatus_(true); // enable buttons
            },
            dataType: "xml"
        });
    }
    else
    {
        snapwebsites.PopupInstance.darkenPage(-150);
    }
};


/** \brief Let the form know that one of its widgets changed.
 *
 * This signal implementation is called whenever the form detects
 * that one of its widgets changed.
 *
 * In most cases it will show the "Save Dialog" although it may just
 * want to ignore the fact. (i.e. some forms do not care about changes
 * as their "save" button is always shown, for example, or it may
 * reacted on this event directly and not give the user a choice to
 * save the changes.)
 *
 * \todo
 * Support a way to tell others to react to this change.
 */
snapwebsites.EditorForm.prototype.changed = function()
{
    // tell others that something changed in the editor form
    var e = jQuery.Event("formchange", {
            form: this
        });
    this.formWidget_.trigger(e);

    if(!this.formWidget_.is(".no-save"))
    {
        this.getSaveDialog().open();
    }
};


/** \brief Retrieve the save dialog.
 *
 * This functio retrieves a reference to the save dialog. It is a
 * function because the save dialog doesn't get created until
 * requested.
 *
 * \note
 * Unfortunately, if you want to define your own save buttons,
 * then you'll be creating the save dialog up front.
 *
 * @return {snapwebsites.EditorSaveDialog}  The save dialog reference.
 */
snapwebsites.EditorForm.prototype.getSaveDialog = function()
{
    if(!this.saveDialog_)
    {
        this.saveDialog_ = new snapwebsites.EditorSaveDialog();
    }
    return this.saveDialog_;
};


/** \brief Ready the widget for attachment.
 *
 * This function goes through the widgets and prepare them to be
 * attached to their respective handlers. Because we want to have
 * an extensible editor, this means we need to allow for "callbacks"
 * to be defined on the extension and not on some internal object.
 * This works by getting the list of widgets, retrieving their types,
 * and looking for a class of that name. If the class does not exist
 * (yet) then the form is not yet complete and the loader goes on.
 * Once all the types are properly defined, the form is ready and
 * the user can start editing.
 *
 * \note
 * The widgets themselves are marked with ".editor-content". The
 * editor always encapsulate those inside a div marked with
 * ".snap-editor". So there is a one to one correlation between
 * both of those.
 *
 * @private
 */
snapwebsites.EditorForm.prototype.readyWidgets_ = function()
{
    var that = this, key;

    // retrieve the widgets defined in that form
    this.widgets_ = this.formWidget_.find(".snap-editor");

    // retrieve the field types for all the widgets
    this.widgets_.each(function(idx, w){
            var type = jQuery(w).attr("field_type");
            that.usedTypes_[type] = true;
        });

    // check whether all the types are available
    // if so then the function finishes the initialization of the form
    this.newTypeRegistered_();
};


/** \brief Check whether all types were registered and if so initialize everything.
 *
 * The function checks whether all the types necessary to initialize
 * the widgets are available. If so, then all the form widgets get
 * initialized.
 *
 * @private
 */
snapwebsites.EditorForm.prototype.newTypeRegistered_ = function()
{
    var that = this, key;

    if(this.widgetInitialized_)
    {
        return;
    }

    // check whether all the types are available
    for(key in this.usedTypes_)
    {
        if(this.usedTypes_.hasOwnProperty(key)
        && this.usedTypes_[key])
        {
            if(!this.editorBase_.hasWidgetType(key))
            {
                // some types are missing, we're not ready
                // (this happens to forms using external widgets)
                return;
            }

            // this one was defined
            this.usedTypes_[key] = false;
        }
    }

    // if we reach here, all the types are available so we can
    // properly initialize the form now
    this.widgets_.each(function(idx, w){
            var widget = jQuery(w);
            var name = widget.attr("field_name");
            that.editorWidgets_[name] = new snapwebsites.EditorWidget(this.editorBase_, this, widget);
        });

    this.widgetInitialized_ = true;
};


snapwebsites.EditorForm.prototype.attach_ = function()
{
    var snap_editor, immediate, auto_focus, that = this;

    // user has to click Edit to activate the editor
    snap_editor = jQuery(".snap-editor");
    jQuery("<div class='editor-tooltip'><a class='activate-editor' href='#'>Edit</a></div>").prependTo(".snap-editor:not(.immediate)");
    snap_editor.not(".immediate").not(".checkmark").children(".editor-tooltip").children(".activate-editor").click(function(){
        jQuery(this).parent().parent().mouseleave().off("mouseenter mouseleave")
                .children(".editor-content").attr("contenteditable", "true").focus();
    });
    // this adds the mouseenter and mouseleave events
    snap_editor.filter(":not(.immediate)")
        .hover(function(){// in
            jQuery(this).children(".editor-tooltip").fadeIn(150);
        },function(){// out
            jQuery(this).children(".editor-tooltip").fadeOut(150);
        });

    // editor is immediately made available
    immediate = snap_editor.filter(".immediate");
    immediate.not(".checkmark").children(".editor-content").attr("contenteditable", "true");

    immediate.filter(".auto-focus").children(".editor-content").focus();

    // backgrounds are positioned as "absolute" so their width
    // is "broken" and we cannot center them in their parent
    // which is something we want to do for image-box objects
    jQuery(".snap-editor.image-box").each(function(){
        var that_element = jQuery(this);
        var background = that_element.children(".snap-editor-background");
        background.css("width", snapwebsites.Output.castToNumber(that_element.width()))
                  .css("margin-top", (snapwebsites.Output.castToNumber(that_element.height()) - snapwebsites.Output.castToNumber(background.height())) / 2);
    });

    jQuery(".editable.dropdown")
        .each(function(){
           jQuery(this).children(".dropdown-items").css("position", "absolute");
        })
        .click(function(e){
            var that_element = jQuery(this), visible, z;

            // avoid default browser behavior
            e.preventDefault();
            e.stopPropagation();

            visible = that_element.is(".disabled") || that_element.children(".dropdown-items").is(":visible");
            if(snapwebsites.EditorInstance.openDropdown_)
            {
                that.openDropdown_.fadeOut(150);
                that.openDropdown_ = null;
            }
            if(!visible)
            {
                that.openDropdown_ = that_element.children(".dropdown-items");

                // setup z-index
                // (reset itself first so we don't just +1 each time)
                that.openDropdown_.css("z-index", 0);
                z = jQuery("div.zordered").maxZIndex() + 1;
                that.openDropdown_.css("z-index", z);

                that.openDropdown_.fadeIn(150);
            }
        })
        .find(".dropdown-items .dropdown-item").click(function(e){
            var that_element = jQuery(this), content, items;

            // avoid default browser behavior
            e.preventDefault();
            e.stopPropagation();

            items = that_element.parents(".dropdown-items");
            items.toggle();
            items.find(".dropdown-item").removeClass("selected");
            that_element.addClass("selected");
            content = items.siblings(".editor-content");
            content.empty();
            content.append(that_element.html());
            if(that_element.attr("value") != "")
            {
                content.attr("value", /** @type {string} */ (that_element.attr("value")));
            }
            else
            {
                content.removeAttr("value");
            }

            this.editorBase_.checkModified();
        });
    jQuery(".editable.dropdown .editor-content")
        .blur(function(e){
            if(that.openDropdown_)
            {
                that.openDropdown_.fadeOut(150);
                that.openDropdown_ = null;
            }
        });

    // Make labels focus the corresponding editable box
    jQuery("label[for!='']").click(function(e){
        // the default may recapture the focus, so avoid it!
        e.preventDefault();
        e.stopPropagation();
        jQuery("div[name='"+jQuery(this).attr("for")+"']").focus();
    });
};


/** \brief Check whether a widget was modified.
 *
 * This function checks whether a widget was modified. If so, the
 * function returns true.
 *
 * Note that after a successful AJAX Save the modified flag is reset
 * (not right here) so this function returns false again.
 *
 * @param {jQuery} widget  The widget to check for modifications.
 *
 * @return {boolean}  true if the widget was modified from its last
 *                    saved value.
 */
snapwebsites.EditorForm.prototype.isModified = function(widget)
{
    // check whether this active element was modified
    if(!this.modified_[widget.objId])
    {
        this.modified_[widget.objId] = this.originalData_[widget.objId] != jQuery(widget).html();
        return this.modified_[widget.objId];
    }
    return false;
};


/** \brief This function checks whether the form was modified.
 *
 * This function checks whether the form was modified. First it
 * checks whether the form is to be saved. If not then the function
 * just returns false.
 *
 * @param {boolean} recheck  Whether to check each widget for modification
 *                           instead of just the modified flags.
 *
 * @return {boolean} true if the form was modified.
 */
snapwebsites.EditorForm.prototype.wasModified = function(recheck)
{
    return true;
};



/** \brief Snap Editor constructor.
 *
 * \note
 * The Snap! Editor is a singleton and should never be created by you. It
 * gets initialized automatically when this editor.js file gets included.
 *
 * \return The newly created object.
 *
 * @constructor
 * @extends {snapwebsites.EditorBase}
 * @struct
 */
snapwebsites.Editor = function()
{
    snapwebsites.EditorBase.apply(this);

    this.constructor = snapwebsites.Editor;
    this.attachToForms_();
    this.initUnload_();

    return this;
};


/** \brief Derive from EditorBase.
 *
 * The Editor derives from the EditorBase object and define some
 * function that are otherwise considered to be virtual in the base.
 */
snapwebsites.Editor.prototype = new snapwebsites.EditorBase();


/** \brief The constructor of this object.
 *
 * Make sure to declare the constructor for proper inheritance
 * support.
 *
 * @type {!function()}
 */
snapwebsites.Editor.prototype.constructor = snapwebsites.Editor;


/** \brief The toolbar object.
 *
 * This variable represents the toolbar used by the editor.
 *
 * Note that this is the toolbar object, not the DOM. The DOM is
 * defined within the toolbar object and is considered private.
 *
 * @type {snapwebsites.EditorToolbar}
 * @private
 */
snapwebsites.Editor.prototype.toolbar_ = null;


/** \brief List of EditorForm sessions.
 *
 * This variable member holds the array of EditorForm sessions as found
 * in the DOM. Later additional entries can be added dynamically and
 * existing entries can be removed (TBD.)
 *
 * The "session" attribute is expected to be defined in each tag marked as
 * an "editor-form" (i.e. with class including the name "editor-form".)
 *
 * By default the list is empty, although if this editor scripts get loaded
 * it is very unlikely that it will stay that way.
 *
 * @type {Array.<string>}
 * @private
 */
snapwebsites.Editor.prototype.editorSessions_ = [];


/** \brief List of EditorForm objects.
 *
 * This variable member holds the map of EditorForm objects indexed by
 * their sessions number (although those numbers are managed as strings).
 * The numbers are listed in the editorSessions_ array.
 *
 * @type {Object}
 * @private
 */
snapwebsites.Editor.prototype.editorForms_ = {};


/** \brief Whether unload is being processed.
 *
 * There is a "bug" in Firefox and derivative browsers (a.k.a. SeaMonkey)
 * where the browser calls the unload function twice. According to the
 * developers, "it is normal". To avoid that "bug" we use this flag and
 * a timer. If this flag is true, we avoid showing a second prompt to the
 * users.
 *
 * @type {!boolean}
 * @private
 */
snapwebsites.Editor.prototype.unloadCalled_ = false;


/** \brief The link dialog.
 *
 * The get_link_dialog() function creates this link dialog the first
 * time it is called.
 *
 * @type {snapwebsites.EditorLinkDialog}
 * @private
 */
snapwebsites.Editor.prototype.linkDialog_ = null;


/** \brief Attach to the EditorForms defined in the DOM.
 *
 * This function attaches the editor to the existing editor forms
 * as defined in the DOM. Editor forms are detected by the fact
 * that a \<div\> tag has class ".editor-form".
 *
 * The function is expected to be called only once.
 *
 * @private
 */
snapwebsites.Editor.prototype.attachToForms_ = function()
{
    var i, max, that = this;

    // retrieve the list of forms using their sessions
    jQuery(".editor-form")
        .each(function(){
            var that_element = jQuery(this);
            var session = snapwebsites.Output.castToString(that_element.attr("session"));
            that.editorSessions_.push(session);
            that.editorForms_[session] = new snapwebsites.EditorForm(that, that_element);
        });
};


/** \brief Capture the unload event.
 *
 * This function adds the necessary code to handle the unload event.
 * This is used to make sure that users will save their data before
 * they actually close their browser window or tab.
 *
 * @private
 */
snapwebsites.Editor.prototype.initUnload_ = function()
{
    jQuery(window).bind("beforeunload", this.unload_);
};


/** \brief Handle the unload event.
 *
 * This function is called whenever the user is about to close their
 * browser window or tab. The function checks whether anything needs
 * to be saved. If so, then make sure to ask the user whether he
 * wants to save his changes before closing the window.
 *
 * @return {?string} A message saying that something wasn't saved if it
 *                   applies, null otherwise.
 *
 * @private
 */
snapwebsites.Editor.prototype.unload_ = function()
{
    var i,                  // loop index
        max,                // number of sessions
        that = this;        // this pointer in the closure function

    if(!this.unloadCalled_)
    {
        max = this.editorSessions_.length;
        for(i = 0; i < max; ++i)
        {
            if(this.editorForms_[this.editorSessions_[i]].wasModified(true))
            {
                // add this flag and timeout to avoid a double
                // "are you sure?" under Firefox browsers
                this.unloadCalled_ = true;
                setTimeout(function(){
                        that.unloadCalled_ = false;
                    }, 20);

                // TODO: translation
                return "You made changes to this page! Click Cancel to avoid closing the window and Save your changes first.";
            }
        }
    }

    return null;
};


/** \brief Retrieve the toolbar object.
 *
 * This function returns a reference to the toolbar object.
 *
 * @return {snapwebsites.EditorToolbar} The toolbar object.
 * @override
 */
snapwebsites.Editor.prototype.getToolbar = function() // virtual
{
    if(!this.toolbar_)
    {
        this.toolbar_ = new snapwebsites.EditorToolbar(this);
    }
    return this.toolbar_;
};


/** \brief Check whether a field was modified.
 *
 * When something may have changed (a character may have been inserted
 * or deleted, or a text replaced) then you are expected to call this
 * function in order to see whether something was indeed modified.
 *
 * When the process detects that something was modified, it calls the
 * necessary functions to open the Save Dialog.
 *
 * As a side effect it also lets the toolbar know so if it needs to be
 * moved, it happens.
 *
 * @override
 */
snapwebsites.Editor.prototype.checkModified = function() // virtual
{
    var active_element = this.getActiveElement(),
        active_form = this.getActiveEditorForm(),
        widget;

    // checkMofified only applies to the active element so make sure
    // there is one when called
    if(active_form)
    {
        // allow the toolbar to adjust itself (move to a new location)
        this.toolbar_.checkPosition();

        // get the form in which this element is defined
        // and call the isModified() function on it
        if(active_form.wasModified(false))
        {
            active_form.changed();
        }

        // replace nothingness by "background" value
        widget = /** @type {snapwebsites.EditorWidget} */ (active_element.data("snapEditorWidget"));
        widget.checkForBackgroundValue();
    }
};


/** \brief Retrieve the active editor form.
 *
 * This function gets the editor form that includes the currently
 * active element.
 *
 * @return {snapwebsites.EditorForm}  The active editor form or null.
 */
snapwebsites.Editor.prototype.getActiveEditorForm = function()
{
    var active_element = this.getActiveElement(),
        editor_element,
        editor_form = null,
        session;

    if(active_element)
    {
        editor_element = active_element.parents(".editor-form");
        if(editor_element)
        {
            session = editor_element.attr("session");
            editor_form = this.editorForms_[session];
        }
//#ifdef DEBUG
        if(!editor_form)
        {
            // should not happen or it means we whacked the session element
            throw Error("There is an active element but no corresponding editor form.");
        }
//#endif
    }

    return null;
};


/** \brief Retrieve the link dialog.
 *
 * This function creates an instance of the link dialog and returns it.
 * If the function gets called more than once, then the same reference
 * is returned.
 *
 * The function takes settings defined in an object, the following are
 * the supported names:
 *
 * \li close -- The function called whenever the user clicks the OK button.
 *
 * \param[in] settings  An object defining settings.
 *
 * @return {snapwebsites.EditorLinkDialog} The link dialog reference.
 * @override
 */
snapwebsites.Editor.prototype.getLinkDialog = function()
{
    if(!this.linkDialog_)
    {
        this.linkDialog_ = new snapwebsites.EditorLinkDialog(this);
    }
    return this.linkDialog_;
};



/** \brief Snap EditorWidgetType constructor.
 *
 * The editor works with widgets that are based on this class. Each widget
 * is given a type such as "line-edit".
 *
 * To make it fully dynamic, we define a base class here and let other
 * programmers add new widget types in their own .js files by extending
 * this class.
 *
 * Classes must be registered with the EditorBase class function:
 *
 * \code
 *    snapwebsites.EditorInstance.registerWidgetType(your_widget_type);
 * \endcode
 *
 * This base class already implements a few things that are common to
 * all widgets.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeBase}
 * @struct
 */
snapwebsites.EditorWidgetType = function()
{
    snapwebsites.EditorWidgetTypeBase.apply(this);

    this.constructor = snapwebsites.EditorWidgetType;

    // TBD
    // Maybe at some point we'd want to create yet another layer
    // so we can have an auto-register, but I'm not totally sure
    // that would work...
    //snapwebsites.EditorBase.registerWidgetType(this);

    return this;
};


/** \brief Inherit the snapwebsites.EditorWidgetTypeBase class.
 *
 * This statement makes our EditorWidgetType inherits the
 * EditorWidgetTypeBase class.
 */
snapwebsites.EditorWidgetType.prototype = new snapwebsites.EditorWidgetType;


/** \brief Initialize a widget of this type.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetType.prototype.initializeWidget = function(widget) // virtual
{
    var that = this;
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget);
    var w = editor_widget.getWidget();
    var c = w.children(".editor-content");

    // widget was possibly modified, so make sure we stay on top
    c.on("keyup bind cut copy paste", function()
        {
            editor_widget.getEditorBase().checkModified();
        });

    // widget gets the focus, make it the active widget
    c.focus(function()
        {
            editor_widget.getEditorBase().setActiveElement(w);

            if(!jQuery(this).is(".no-toolbar"))
            {
                if(editor_widget.getEditorForm().getToolbarAutoVisible())
                {
                    editor_widget.getEditorBase().getToolbar().toggleToolbar(true);
                }
            }
        });

    // widget loses the focus, lose the toolbar in a few ms...
    //
    // (note: the activeElement_ parameter is NOT reset and this is important
    //        because the toolbar may use it to restore it once the command
    //        is processed.)
    //
    c.blur(function()
        {
            // don't blur the toolbar immediately because if the user just
            // clicked on it, it would break it
            editor_widget.getEditorBase().getToolbar().startToolbarHide();
        });

    // a key was pressed in the focused widget
    c.keydown(function(e)
        {
//console.log("ctrl "+(e.shiftKey?"+ shift ":"")+"= "+e.which+", idx = "+(snapwebsites.EditorInstance.keys_[e.which + (e.shiftKey ? 0x10000 : 0)]));
            if(editor_widget.getEditorBase().getToolbar().keydown(e))
            {
                // toolbar used that key stroke
                e.preventDefault();
                e.stopPropagation();
            }
        });

    // the user just moved over a widget while dragging something
    c.on("dragenter",function(e)
        {
            e.preventDefault();
            e.stopPropagation();
            jQuery(this).parent().addClass("dragging-over");
        });

    // the user is dragging something over a widget
    c.on("dragover",function(e)
        {
            // TBD this is said to make things work better in some browsers...
            e.preventDefault();
            e.stopPropagation();
        });

    // the user just moved out a widget while dragging something
    c.on("dragleave",function(e)
        {
            e.preventDefault();
            e.stopPropagation();
            jQuery(this).parent().removeClass("dragging-over");
        });

    // the user actually dropped a file on this widget
    //
    // Note: we handle the drop at this level, other widget types
    //       should only override the the droppedImage()
    //       and droppedAttachment() functions instead
    //
    c.on("drop",function(e)
        {
            var i,                      // loop index
                r,                      // file reader object
                accept_images,          // boolean, true if element accepts images
                accept_files,           // boolean, true if element accepts attachments
                that_element = jQuery(this);    // this element as a jQuery object

            //
            // TODO:
            // At this point this code breaks the normal behavior that
            // properly places the image where the user wants it; I'm
            // not too sure how we can follow up on the "go ahead and
            // do a normal instead" without propagating the event, but
            // I'll just ask on StackOverflow for now...
            //
            // http://stackoverflow.com/questions/22318243/how-to-apply-the-default-image-drop-behavior-after-testing-that-image-is-valid
            //
            // That said, I did not get any answer but thinking about
            // it, it seems pretty easy to me: the answer is to use
            // the jQuery().trigger() command which processes the event
            // as if nothing had happened. Then we just need to ignore
            // that event if it calls this "drop" event handler again.
            //

            // remove the dragging-over class on a drop because we
            // do not always get the dragleave event in that case
            that_element.parent().removeClass("dragging-over");

            // always prevent the default dropping mechanism
            // we handle the file manually all the way
            e.preventDefault();
            e.stopPropagation();

            // anything transferred on widget that accepts files?
            if(e.originalEvent.dataTransfer
            && e.originalEvent.dataTransfer.files.length)
            {
                accept_images = that_element.hasClass("image");
                accept_files = that_element.hasClass("attachment");
                if(accept_images || accept_files)
                {
                    for(i = 0; i < e.originalEvent.dataTransfer.files.length; ++i)
                    {
                        // For images we do not really care about that info, for uploads we will
                        // use it so I keep that here for now to not have to re-research it...
                        //console.log("  filename = [" + e.originalEvent.dataTransfer.files[i].name
                        //          + "] + size = " + e.originalEvent.dataTransfer.files[i].size
                        //          + " + type = " + e.originalEvent.dataTransfer.files[i].type
                        //          + "\n");

                        // read the image so we can make sure it is indeed an
                        // image and ignore any other type of files
                        r = new FileReader;
                        r.snapEditorElement = that_element;
                        r.snapEditorFile = e.originalEvent.dataTransfer.files[i];
                        r.snapEditorIndex = i;
                        r.snapEditorAcceptImages = accept_images;
                        r.snapEditorAcceptFiles = accept_files;
                        r.onload = that.droppedFile_;

                        //
                        // TBD: right now we only check the first few bytes
                        //      but we may want to increase that size later
                        //      to allow for JPEG that have the width and
                        //      height defined (much) further in the stream
                        //      (at times at the end!?)
                        //
                        r.readAsArrayBuffer(r.snapEditorFile.slice(0, 64));
                    }
                }
            }

            return false;
        });
};


/** \brief Got the content of a dropped file.
 *
 * This function analyze the dropped file content. If recognized then we
 * proceed with the onimagedrop or onattachmentdrop as required.
 *
 * @param {ProgressEvent} e  The file reader structure.
 *
 * @private
 */
snapwebsites.EditorWidgetType.prototype.droppedFile_ = function(e)
{
    var mime, r, a, blob;

    e.target.snapEditorMIME = snapwebsites.OutputInstance.bufferToMIME(e.target.result);
    if(e.target.snapEditorAcceptImages && e.target.snapEditorMIME.substr(0, 6) == "image/")
    {
        // Dropped an Image managed as such

        // It is an image, now convert the data to URI encoding
        // (i.e. base64 encoding) before saving the result in the
        // target element

//console.log("image type: "+mime.substr(6)+", target = ["+e.target+"]");

        r = new FileReader;
        r.snapEditorElement = e.target.snapEditorElement;
        r.snapEditorFile = e.target.snapEditorFile;
        r.snapEditorAcceptImages = e.target.snapEditorAcceptImages;
        r.snapEditorAcceptFiles = e.target.snapEditorAcceptFiles;
        r.snapEditorMIME = e.target.snapEditorMIME;
        r.onload = this.droppedImageConvert_;
        a = [];
        a.push(e.target.snapEditorFile);
        blob = new Blob(a, { type: mime });
        r.readAsDataURL(blob);
    }
    else if(e.target.snapEditorAcceptFiles && e.target.snapEditorMIME)
    {
        // Dropped a file managed as an attachment
        this.droppedAttachment(e);
    }
    else
    {
        // generate an error
        //
        // TODO: we don't yet have code to dynamically generate errors
        //       (we can show messages when created by the server, and
        //       want the same thing with errors, but that's not yet
        //       available...)
        //
    }
};


/** \brief Save the resulting image in the target.
 *
 * This function receives the image as data that can readily be stick
 * in the 'src' attribute of an 'img' tag (i.e. data:image/fmt;base64=...).
 * It gets loaded in an Image object so we can verify the image width
 * and height before it gets presented to the end user.
 *
 * @param {ProgressEvent} e  The file information.
 * @private
 */
snapwebsites.EditorWidgetType.prototype.droppedImageConvert_ = function(e)
{
    var img, that = this;

    img = new Image();

    // The image parameters (width/height) are only available after the
    // onload() event kicks in
    img.onload = function()
        {
            // keep this function here because it is a full closure (it
            // uses 'img' 'that', and even 'e')

            var sizes, limit_width = 0, limit_height = 0, w, h, nw, nh,
                max_sizes, saved_active_element;

            // make sure we do it just once
            img.onload = null;

            w = img.width;
            h = img.height;

            if(e.target.snapEditorElement.attr("min-sizes"))
            {
                sizes = e.target.snapEditorElement.attr("min-sizes").split("x");
                if(w < sizes[0] || h < sizes[1])
                {
                    // image too small...
                    // TODO: fix alert with clean error popup
                    alert("This image is too small. Minimum required is "
                            + e.target.snapEditorElement.attr("min-sizes")
                            + ". Please try with a larger image.");
                    return;
                }
            }
            if(e.target.snapEditorElement.attr("max-sizes"))
            {
                sizes = e.target.snapEditorElement.attr("max-sizes").split("x");
                if(w > sizes[0] || h > sizes[1])
                {
                    // image too large...
                    // TODO: fix alert with clean error popup
                    alert("This image is too large. Maximum allowed is "
                            + e.target.snapEditorElement.attr("max-sizes")
                            + ". Please try with a smaller image.");
                    return;
                }
            }

            if(e.target.snapEditorElement.attr("resize-sizes"))
            {
                max_sizes = e.target.snapEditorElement.attr("resize-sizes").split("x");
                limit_width = max_sizes[0];
                limit_height = max_sizes[1];
            }

            if(limit_width > 0 && limit_height > 0)
            {
                if(w > limit_width || h > limit_height)
                {
                    // source image is too large
                    nw = Math.round(limit_height / h * w);
                    nh = Math.round(limit_width / w * h);
                    if(nw > limit_width && nh > limit_height)
                    {
                        // TBD can this happen?
                        alert("somehow we could not adjust the dimentions of the image properly!?");
                    }
                    if(nw > limit_width)
                    {
                        h = nh;
                        w = limit_width;
                    }
                    else
                    {
                        w = nw;
                        h = limit_height;
                    }
                }
            }
            jQuery(img)
                .attr("width", w)
                .attr("height", h)
                .attr("filename", e.target.snapEditorFile.name)
                .css({top: (limit_height - h) / 2, left: (limit_width - w) / 2, position: "relative"});

            that.droppedImage(e, img);
        };
    img.src = e.target.result;

    // TBD: still a valid test? img.readyState is expected to be a string!
    //
    // a fix for browsers that don't call onload() if the image is
    // already considered loaded by now
    if(img.complete || img.readyState == 4)
    {
        img.onload();
    }
};


/** \brief Handle an image that was just dropped.
 *
 * This function handles an image as it was just dropped.
 *
 * @param {ProgressEvent} e  The reader data.
 * @param {Image} img  The image te user dropped.
 */
snapwebsites.EditorWidgetType.prototype.droppedImage = function(e, img)
{
    throw Error("snapwebsites.EditorWidgetType.prototype.droppedImage() not overridden and thus it cannot handle the dropped image.");
};


/** \brief Handle an image that was just dropped.
 *
 * This function handles an image as it was just dropped.
 *
 * @param {ProgressEvent} e  The reader data.
 */
snapwebsites.EditorWidgetType.prototype.droppedAttachment = function(e)
{
    throw Error("snapwebsites.EditorWidgetType.prototype.droppedAttachment() not overridden and thus it cannot handle the dropped attachment.");
};



/** \brief Editor widget type for Text Edit widgets.
 *
 * This widget defines the full text edit in the editor forms. This is
 * an equivalent to the text area of a standard form.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeTextEdit = function()
{
    snapwebsites.EditorWidgetType.apply(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and it's super.
 */
snapwebsites.EditorWidgetTypeTextEdit.prototype = new snapwebsites.EditorWidgetType;


/** \brief Return "text-edit".
 *
 * Return the name of the text edit type.
 *
 * @return {string} The name of the text edit type.
 * @override
 */
snapwebsites.EditorWidgetTypeTextEdit.prototype.getType = function() // virtual
{
    return "text-edit";
};


/** \brief Initialize the widget.
 *
 * This function initializes the text-edit widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeTextEdit.prototype.initializeWidget = function(widget) // virtual
{
    snapwebsites.EditorWidgetType.prototype.initializeWidget.apply(this, [widget]);
};



/** \brief Editor widget type for Text Edit widgets.
 *
 * This widget defines the full text edit in the editor forms. This is
 * an equivalent to the text area of a standard form.
 *
 * @return {!snapwebsites.EditorWidgetTypeLineEdit}
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeTextEdit}
 * @struct
 */
snapwebsites.EditorWidgetTypeLineEdit = function()
{
    snapwebsites.EditorWidgetTypeTextEdit.apply(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and it's super.
 */
snapwebsites.EditorWidgetTypeLineEdit.prototype = new snapwebsites.EditorWidgetTypeTextEdit;


/** \brief The constructor of this widget.
 *
 * The constructor one can use to build this type of object.
 *
 * @type {function(): !snapwebsites.EditorWidgetTypeLineEdit}
 */
snapwebsites.EditorWidgetTypeLineEdit.prototype.constructor = snapwebsites.EditorWidgetTypeLineEdit;


/** \brief Return "line-edit".
 *
 * Return the name of the line edit type.
 *
 * @return {string} The name of the line edit type.
 * @override
 */
snapwebsites.EditorWidgetTypeLineEdit.prototype.getType = function()
{
    return "line-edit";
};


/** \brief Initialize the widget.
 *
 * This function initializes the line-edit widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeLineEdit.prototype.initializeWidget = function(widget) // virtual
{
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget);
    var w = editor_widget.getWidget();
    var c = w.children(".editor-content");

    snapwebsites.EditorWidgetTypeTextEdit.prototype.initializeWidget.apply(this, [widget]);

    c.keydown(function(e)
        {
            if(c.is(".read-only"))
            {
                e.preventDefault();
                e.stopPropagation();
            }
        });
};



/** \brief Editor widget type for Checkmark widgets.
 *
 * This widget defines a checkmark in the editor forms.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetType}
 * @struct
 */
snapwebsites.EditorWidgetTypeCheckmark = function()
{
    snapwebsites.EditorWidgetType.apply(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and it's super.
 */
snapwebsites.EditorWidgetTypeCheckmark.prototype = new snapwebsites.EditorWidgetType;


/** \brief Return "checkmark".
 *
 * Return the name of the checkmark type.
 *
 * @return {string} The name of the checkmark type.
 * @override
 */
snapwebsites.EditorWidgetTypeCheckmark.prototype.getType = function() // virtual
{
    return "checkmark";
};


/** \brief Initialize the widget.
 *
 * This function initializes the checkmark widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeCheckmark.prototype.initializeWidget = function(widget) // virtual
{
    var that = this;
    var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget);
    var w = editor_widget.getWidget();
    var c = editor_widget.getWidgetContent();
    var toggle = function()
    {
        // toggle the current value
        var checkmark = c.find(".checkmark-area");
        checkmark.toggleClass("checked");
        c.attr("value", checkmark.hasClass("checked") ? 1 : 0);

        // TBD: necessary to avoid setting the focus anew?
        //      if not then we should remove the if() statement
        if(editor_widget.getEditorBase().getActiveElement().get() != c)
        {
            c.focus();
        }

        // tell the editor that something may have changed
        // TODO: call the widget function which in turn tells the
        //       editor instead of re-testing all the widgets?!
        editor_widget.getEditorBase().checkModified();
    };

    snapwebsites.EditorWidgetType.prototype.initializeWidget.apply(this, [widget]);

    c.keydown(function(e)
        {
            if(e.which == 0x20) // spacebar
            {
                e.preventDefault();
                e.stopPropagation();

                toggle();
            }
        });

    w.click(function(e)
        {
            // TODO: add support for clicks on links part of the label

            // the default may do weird stuff, so avoid it!
            e.preventDefault();
            e.stopPropagation();

            toggle();
        });
};


// This is not necessary anymore, but I want to keep it for documentation
// purposes.
//
///* brief Change the result to just 0 or 1.
// *
// * This function changes the result of a checkmark as the value 0 or 1
// * instead of the HTML of the sub-objects. This value represents the
// * current selection (0 -- not checked, or 1 -- checked.)
// *
// * param {snapwebsites.EditorWidgetTypeBase.SaveData} data  The data object with the HTML and result parameters.
// */
//snapwebsites.EditorWidgetTypeCheckmark.prototype.saving = function(data) // virtual
//{
//    snapwebsites.EditorWidgetType.prototype.initializeWidget.apply(this, data);
//
//    data.result = edit_area.find(".checkmark-area").hasClass("checked") ? 1 : 0;
//}



/** \brief Editor widget type for Dropdown widgets.
 *
 * This widget defines a dropdown in the editor forms.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeLineEdit}
 * @struct
 */
snapwebsites.EditorWidgetTypeDropdown = function()
{
    snapwebsites.EditorWidgetTypeLineEdit.apply(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and it's super.
 */
snapwebsites.EditorWidgetTypeDropdown.prototype = new snapwebsites.EditorWidgetTypeLineEdit;


/** \brief Return "dropdown".
 *
 * Return the name of the dropdown type.
 *
 * @return {string} The name of the dropdown type.
 * @override
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.getType = function() // virtual
{
    return "dropdown";
};


/** \brief Initialize the widget.
 *
 * This function initializes the dropdown widget.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeDropdown.prototype.initializeWidget = function(widget) // virtual
{
    //var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget);
    //var w = editor_widget.getWidget();
    //var c = w.children(".editor-content");

    snapwebsites.EditorWidgetTypeLineEdit.prototype.initializeWidget.apply(this, [widget]);

    // TODO: we need to add support for the up/down arrow keys to change
    //       the selection
    //w.keydown(function(e)
    //    {
    //        // ...
    //    });
};



/** \brief Editor widget type for Image Box widgets.
 *
 * This widget defines an image box in the editor forms. The whole widget
 * is just and only an image. You cannot type in data and when an image is
 * dragged and dropped over that widget, it replaces the previous version
 * of the image.
 *
 * If required, the widget is smart enough to use a proportional resize
 * so the image fits the widget area.
 *
 * @constructor
 * @extends {snapwebsites.EditorWidgetTypeLineEdit}
 * @struct
 */
snapwebsites.EditorWidgetTypeImageBox = function()
{
    snapwebsites.EditorWidgetType.apply(this);

    return this;
};


/** \brief Chain up the extension.
 *
 * This is the chain between this class and it's super.
 */
snapwebsites.EditorWidgetTypeImageBox.prototype = new snapwebsites.EditorWidgetType;


/** \brief Return "image".
 *
 * Return the name of the image type.
 *
 * @return {string} The name of the image type.
 * @override
 */
snapwebsites.EditorWidgetTypeImageBox.prototype.getType = function() // virtual
{
    return "image";
};


/** \brief Initialize the widget.
 *
 * This function initializes the image box widget.
 *
 * \note
 * At this point the Image Box widget do not attach to any events since all
 * the drag and drop work is done at the EditorWidgetType level. However,
 * we have a droppedImage() function (see below) which finishes the work
 * of the drag and drop implementation.
 *
 * @param {!Object} widget  The widget being initialized.
 * @override
 */
snapwebsites.EditorWidgetTypeImageBox.prototype.initializeWidget = function(widget) // virtual
{
    //var editor_widget = /** @type {snapwebsites.EditorWidget} */ (widget);
    //var w = editor_widget.getWidget();

    snapwebsites.EditorWidgetTypeLineEdit.prototype.initializeWidget.apply(this, [widget]);
};


/** \brief Handle the dropped image.
 *
 * This function handles the dropped image by saving it in the target
 * element.
 *
 * @param {ProgressEvent} e  The element.
 * @param {Image} img  The image to insert in the destination widget.
 *
 * @override
 */
snapwebsites.EditorWidgetTypeImageBox.prototype.droppedImage = function(e, img)
{
    var saved_active_element, editor_base;

    e.target.snapEditorElement.empty();
    jQuery(img).appendTo(e.target.snapEditorElement);

    // now make sure the editor detects the change
    editor_base = snapwebsites.EditorInstance;
    saved_active_element = editor_base.getActiveElement();
    editor_base.setActiveElement(e.target.snapEditorElement);
    editor_base.checkModified();
    editor_base.setActiveElement(saved_active_element);
};


// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.EditorInstance = new snapwebsites.Editor();
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeTextEdit());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeLineEdit());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeCheckmark());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeDropdown());
        snapwebsites.EditorInstance.registerWidgetType(new snapwebsites.EditorWidgetTypeImageBox());
    }
);
// vim: ts=4 sw=4 et
