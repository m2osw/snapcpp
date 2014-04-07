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
// This version makes use of jQuery 1.10+ to access objects. It should be
// compatible with older versions of jQuery.
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
 * +-------------------+  Reference   +--+--+------------+    |
 * |                   |<-------------+                  |    |
 * |   EditorBase      |              |   EditorToolbar  |    |
 * |                   |<----+        |                  |<---+
 * |                   |     |        |                  |    |
 * +-------------------+     |        +------------------+    |
 *          ^ Inherit        |              ^                 |
 *          |                | Reference    | Reference       |
 *          |                |              |                 |
 * +--------+----------+     |        +-----+------------+    |
 * |                   |     +--------+                  |    |
 * |   Editor          |              |   EditorForm     |    |
 * |                   | Create (1,n) |                  |    |
 * |                   +------------->|                  |    |
 * +-----------------+-+              +------------------+    |
 *       ^           |                                        |
 *       |           |    Create (1,1)                        |
 *       |           +----------------------------------------+
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

//    _findFirstVisibleTextNode: function(p)
//    {
//        var textNodes, nonWhitespaceMatcher = /\S/;
//
//        function __findFirstVisibleTextNode(p)
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
//                result = __findFirstVisibleTextNode(child);
//                if(result)
//                {
//                    return result;
//                }
//            }
//        }
//
//        return __findFirstVisibleTextNode(p);
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
//console.log("active is ["+this._activeElement+"]");
//        var elem=this._findFirstVisibleTextNode(this._activeElement);//jQuery(this._activeElement).filter(":visible:text:first");
//console.log("first elem = ["+this._activeElement+"/"+jQuery(elem).length+"] asis:["+elem+"] text:["+elem.nodeValue+"] node:["+jQuery(elem).prop("nodeType")+"]");
////jQuery(this._activeElement).find("*").filter(":visible").each(function(i,e){
////console.log(" + child = ["+this+"] ["+jQuery(this).prop("tagName")+"] node:["+jQuery(this).prop("nodeType")+"]");
////});
//
//        range = document.createRange();//Create a range (a range is a like the selection but invisible)
//        range.selectNodeContents(elem);//this._activeElement);//Select the entire contents of the element with the range
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
//            range.setStart(this._activeElement, 0);
//            range.setEnd(this._activeElement, 0);
//        }
//    },

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
    return this;
};


/** \brief The prototype of the EditorBase.
 *
 * This object defines the fields and functions offered in the editor
 * base object.
 *
 * It cannot reference any of the other objects (not "legally" at least).
 *
 * @struct
 */
snapwebsites.EditorBase.prototype =
{
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
    _activeElement: null,

    /** \brief The constructor of this object.
     *
     * Make sure to declare the constructor for proper inheritance
     * support.
     *
     * @type {function()}
     */
    constructor: snapwebsites.EditorBase,

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
        this._activeElement = element;
    },

    /** \brief Retrieve the currently active element.
     *
     * This function returns a reference to the currently focused element.
     *
     * @return {jQuery} The DOM element with the focus.
     */
    getActiveElement: function()
    {
        //jQuery(snapwebsites.EditorInstance._activeElement)
        return this._activeElement;
    },

    /** \brief Refocus the active element.
     *
     * This function is used by the toolbar to refocus the currently
     * active element because when one clicks on the toolbar, the focus
     * is lost from the active element.
     */
    refocus: function()
    {
        if(this._activeElement)
        {
            this._activeElement.focus();
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
    get_link_dialog: function()
    {
        throw new Error("get_link_dialog() cannot directly be called on the EditorBase class.");
    }
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
    this._editor_base = editor_base;

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
    this._linkDialogPopup = jQuery("#snap_editor_link_dialog");

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
    /** \brief The constructor of this object.
     *
     * Make sure to declare the constructor for proper inheritance
     * support.
     *
     * @type {function(snapwebsites.EditorBase)}
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
    _editor_base: null,

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
    _linkDialogPopup: null,

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
    _selection_range: null,

    /** \brief Open the link dialog.
     *
     * This function opens (i.e. shows and positions) the link dialog.
     *
     * A strong side effect of this function is to darken anything
     * else in the background.
     */
    open: function()
    {
        this._selection_range = snapwebsites.EditorSelection.saveSelection();

        var jtag;
        var selectionText = snapwebsites.EditorSelection.getSelectionText();
        var links = snapwebsites.EditorSelection.getLinksInSelection();
        var new_window = true;

        if(links.length > 0)
        {
            jtag = jQuery(links[0]);
            // it is already the anchor, we can use the text here
            // in this case we also have a URL and possibly a title
            jQuery("#snap_editor_link_url").val( /** @type {string} */ (jtag.attr("href")));
            jQuery("#snap_editor_link_title").val( /** @type {string} */ (jtag.attr("title")));
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
        this._linkDialogPopup.css("top", pos.top + height);
        var left = pos.left - 5;
        if(left < 10)
        {
            left = 10;
        }
        this._linkDialogPopup.css("left", left);
        this._linkDialogPopup.fadeIn(300,function(){jQuery(focusItem).focus();});
        snapwebsites.PopupInstance.darkenPage(150);
    },

    /** \brief Close the editor link dialog.
     *
     * This function copies the link to the widget being edited and
     * closes the popup.
     */
    close: function()
    {
        snapwebsites.EditorInstance._linkDialogPopup.fadeOut(150);
        snapwebsites.PopupInstance.darkenPage(-150);

        this._editor_base.refocus();
        snapwebsites.EditorSelection.restoreSelection(this._selection_range);
        var url = jQuery("#snap_editor_link_url");
        document.execCommand("createLink", false, url.val());
        var links = snapwebsites.EditorSelection.getLinksInSelection();
        if(links.length > 0)
        {
            var jtag = jQuery(links[0]);
            var text = jQuery("#snap_editor_link_text");
            if(text.length > 0)
            {
                jtag.text( /** @type {string} */ (text.val()));
            }
            // do NOT erase the existing text if the user OKed
            // without any text

            var title = jQuery("#snap_editor_link_title");
            if(title.length > 0)
            {
                jtag.attr("title", /** @type {string} */ (title.val()));
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
    // save the reference to the editor base
    this._editor_base = editor_base;

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
    /** \brief The constructor of this object.
     *
     * Make sure to declare the constructor for proper inheritance
     * support.
     *
     * @type {function(snapwebsites.EditorBase)}
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
    _toolbarButtons: [
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
        ["createLink", "Manage Link (Ctrl-L)", 0x6004C, "http://snapwebsites.org/", "_linkDialog"],
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
        ["*", null, 0x54, snapwebsites.EditorToolbar.prototype.toggleToolbar]            // Ctrl-T
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
    _editor_base: null,

    /** \brief The list of keys understood by the toolbar.
     *
     * Each toolbar button (and non-buttons) can have a key assigned to it.
     * The _keys variable is a map of those keys to very quickly find the
     * command that corresponds to a key. This table is generated at the
     * time the toolbar is initialized.
     *
     * \sa _init()
     *
     * @type {Array.<number>}
     * @private
     */
    _keys: [],

    /** \brief The jQuery toolbar
     *
     * This is the jQuery object representing the toolbar (a \<div\>
     * tag in the DOM.)
     *
     * @type {jQuery}
     * @private
     */
    _toolbar: null,

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
    _toolbarVisible: false,

    /** \brief Whether the toolbar is shown at the bottom of the widget.
     *
     * When there isn't enough room to put the toolbar at the top, this
     * flag is set to true. The default is rather meaningful.
     *
     * @type {boolean}
     * @private
     */
    _bottomToolbar: false,

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
    _height: -1,

    /** \brief Call this function whenever the toolbar is about to be accessed.
     *
     * This function is called whenever the EditorToolbar object is about to
     * access the toolbar DOM. This allows the system to create the toolbar
     * only once required. This is whenever an editor key (i.e. Ctrl-B) is
     * hit or if the toolbar is to be shown.
     *
     * @private
     */
    _createToolbar: function()
    {
        var that = this, msie, html, originalName, isGroup, idx, max;

        if(!this._toolbar)
        {
            msie = /msie/.exec(navigator.userAgent.toLowerCase()); // IE?
            html = "<div id=\"toolbar\">";
            max = this._toolbarButtons.length;

            for(idx = 0; idx < max; ++idx)
            {
                // the name of the image always uses the original name
                originalName = this._toolbarButtons[idx][0];
                if(msie)
                {
                    if(this._toolbarButtons[idx][0] == "hiliteColor")
                    {
                        this._toolbarButtons[idx][0] = "backColor";
                    }
                }
                else
                {
                    if(this._toolbarButtons[idx][0] == "insertFieldset")
                    {
                        this._toolbarButtons[idx][0] = "insertHTML";
                        this._toolbarButtons[idx][3] = "<fieldset><legend>Fieldset</legend><p>&nbsp;</p></fieldset>";
                    }
                }
                isGroup = this._toolbarButtons[idx][0] == "|";
                if(!isGroup)
                {
                    this._keys[this._toolbarButtons[idx][2] & 0x1FFFF] = idx;
                }
                if(this._toolbarButtons[idx][1] != null)
                {
                    if(isGroup)
                    {
                        if(this._toolbarButtons[idx][1] == "-")
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
                                + this._toolbarButtons[idx][1] + "\">"
                                + "<span class=\"image\"></span></div>";
                    }
                }
            }
            html += "</div>";
            jQuery(html).appendTo("body");
            this._toolbar = jQuery("#toolbar");

            this._toolbar
                .click(function(e){
                    that._editor_base.refocus();
                    e.preventDefault();
                })
                .mousedown(function(e){
                    // XXX: this needs to be handled through a form of callback
                    snapwebsites.EditorInstance._cancel_toolbar_hide();
                    e.preventDefault();
                })
                .find(":any(.horizontal-separator .group)")
                    .click(function(){
                        that._editor_base.refocus();
                    });
            this._toolbar
                .find(".button")
                    .click(function(){
                        var idx = this.attr("button-id");
                        that._editor_base.refocus();
                        that.command(idx);
                    });
        }
    },

    /** \brief Execute a toolbar command.
     *
     * This function expects the index of the command to execute. The
     * command index is the index of the command in the _toolbarButtons
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
        var tag, internal_callback;

        // command is defined?
        if(!this._toolbarButtons[idx])
        {
            return false;
        }

console.log("run command "+idx+" "+this._toolbarButtons[idx][2]+"!!!");

        // require better selection for a link? (i.e. full link)
        if(this._toolbarButtons[idx][2] & 0x20000)
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

        if(this._toolbarButtons[idx][2] & 0x40000)
        {
            internal_callback = this._toolbarButtons[idx][4];
//#ifdef DEBUG
            if(typeof internal_callback != "function")
            {
                throw Error("command() internal callback function \"" + internal_callback + "\" is not defined as a function");
            }
//#endif
            internal_callback.apply(this, this._toolbarButtons[idx]);
            // the dialog OK button will do the rest of the work as
            // required by this specific command
            return true;
        }

        if(this._toolbarButtons[idx][0] == "*")
        {
            // "internal command"
            internal_callback = this._toolbarButtons[idx][4];
//#ifdef DEBUG
            if(typeof internal_callback != "function")
            {
                throw Error("command() internal callback function \"" + internal_callback + "\" is not defined as a function");
            }
//#endif
            internal_callback.apply(this, this._toolbarButtons[idx]);

            //switch(this._toolbarButtons[idx][3])
            //{
            //case "toggleToolbar":
            //    this.toggleToolbar(""); // Note: using "" because google compiler "forces" us to
            //    break;
            //default:
            //    throw new Error("command() unknown internal command " + this._toolbarButtons[idx][3] + " (index: " + idx + ")");
            //}
        }
        else
        {
            // if there is a toolbar parameter, make sure to pass it along
            if(this._toolbarButtons[idx][3])
            {
                // TODO: need to define the toolbar parameter
                //       (i.e. a color, font name, size, etc.)
                document.execCommand( /** @type string */ (this._toolbarButtons[idx][0]), false, this._toolbarButtons[idx][3]);
            }
            else
            {
                document.execCommand( /** @type string */ (this._toolbarButtons[idx][0]), false, null);
            }
        }
        this._editor_base.checkModified();

        return true;
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
     * \param[in] force  Whether to show (true), hide (false),
     *                   or toggle (undefined) the visibility.
     */
    toggleToolbar: function(force)
    {
        var toolbarHeight, pos, widget;

        if(force === true || force === false)
        {
            this._toolbarVisible = force;
        }
        else
        {
            this._toolbarVisible = !this._toolbarVisible;
        }

        if(this._toolbarVisible)
        {
            // in this case we definitvely need to have a toolbar, create it
            this._createToolbar();

            // start showing the bar, then place it (when display is none,
            // the size is incorrect, calling fadeIn() first fixes that
            // problem.)
            this._toolbar.fadeIn(300);
            widget = this._editor_base.getActiveElement();
            this._height = /** @type {number} */ (widget.parents('.snap-editor').height());
            toolbarHeight = this._toolbar.outerHeight();
            pos = widget.parents('.snap-editor').position();
            this._bottomToolbar = pos.top < toolbarHeight + 5;
            if(this._bottomToolbar)
            {
                // too low, put the toolbar at the bottom
                this._toolbar.css("top", (pos.top + this._height + 3) + "px");
            }
            else
            {
                this._toolbar.css("top", (pos.top - toolbarHeight - 3) + "px");
            }
            this._toolbar.css("left", (pos.left + 5) + "px");
        }
        else if(this._toolbar) // if no toolbar anyway, exit
        {
            // make sure it is gone
            this._toolbar.fadeOut(150);
        }
    },

    /** \brief ToggleToobar callback to support the Ctrl-T key.
     *
     * This function is called
     */
    _toggleToolbar: function(a, b, c, d)
    {
        this.toggleToolbar('');
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

        if(snapwebsites.EditorInstance._bottomToolbar)
        {
            newHeight = jQuery(this).outerHeight();
            if(newHeight != snapwebsites.EditorInstance._height)
            {
                snapwebsites.EditorInstance._height = newHeight;
                pos = jQuery(this).position();
                snapwebsites.EditorInstance._toolbar.animate({top: pos.top + jQuery(this).height() + 3}, 200);
            }
        }
    },

    /** \brief Callback to define a link.
     *
     * This function is the callback that opens a popup to edit a link in
     * the editor. It makes use of the EditorLinkDialog.
     *
     * \param[in] cmd  The command that generated this call.
     */
    _linkDialog: function(cmd)
    {
        // for now we can ignore cmd

        this._editor_base.get_link_dialog().open();
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
 *
 * @return {snapwebsites.EditorForm}  The newly created object.
 *
 * @constructor
 * @struct
 */
snapwebsites.EditorForm = function(editor_base)
{
    this._editor_base = editor_base;
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
snapwebsites.EditorForm.prototype =
{
    /** \brief The constructor of this object.
     *
     * Make sure to declare the constructor for proper inheritance
     * support.
     *
     * @type {function(snapwebsites.EditorBase)}
     */
    constructor: snapwebsites.EditorForm,

    /** \brief A reference to the base editor object.
     *
     * This value is a reference to the base editor object so the
     * EditorForm objects can access it.
     *
     * @type {snapwebsites.EditorBase}
     * @private
     */
    _editor_base: null,

    /** \brief The popup dialog.
     *
     * This member is the save dialog widget. It is a jQuery object of
     * the dialog DOM object. It is often changed by different systems
     * to make use of different sets of buttons. To change this
     * parameter, make sure to make use of the functions.
     *
     * @type {jQuery}
     * @private
     */
    _saveDialogPopup: null,

    toolbarAutoVisible: true,
    inPopup: false,

    _unloadCalled: false,
    _toolbarTimeoutID: -1,
    _height: -1,
    _lastFormId: 0,
    _lastItemId: 0,
    _uniqueId: 0, // for images at this point
    _originalData: [],
    _modified: [],
    _linkDialogPopup: null,
    _savedTextRange: null,
    _savedRange: null,
    _openDropdown: null,

    /** \brief Massage the title to make it a URI.
     *
     * This function transforms the characters in \p title so it can be
     * used as a segment of the URI of this page. This is quite important
     * since we use the URI to save the page.
     *
     * @param {string} title  The title to tweak.
     *
     * @return {string}  The tweaked title. It may be an empty string.
     */
    _titleToURI: function(title)
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
    },

    _saveData: function(mode)
    {
        // TODO: this looks wrong, (this) is the editor, not a DOM object
        //       also there is no code setting that class, remove the
        //       class once done, or CSS using it either.
        //       [see the _saveDialogStatus() function]
        if(jQuery(this).hasClass("editor-disabled"))
        {
            // TODO: translation support
            alert("You already clicked one of these buttons. Please wait until the save is over.");
            return;
        }

        // TODO: add a condition coming from the DOM (i.e. we don't want
        //       to gray out the screen if the user is expected to be
        //       able to continue editing while saving)
        //       the class is nearly there (see header trying to assign body
        //       attributes), we will then need to test it here
        snapwebsites.PopupInstance.darkenPage(150);

        var i, obj = {}, saved_data = {}, saved = [], edit_area, url, name,
            keep_darken_page = false, value;
        for(i = 1; i <= this._lastItemId; ++i)
        {
            if(this._modified[i])
            {
                // verify one last time whether it was indeed modified
                edit_area = jQuery("#editor-area-" + i);
                if(this._originalData[i] != edit_area.html())
                {
                    saved.push(i);
                    name = edit_area.parent().attr("field_name");
                    saved_data[name] = edit_area.html();

                    value = edit_area.attr("value");
console.log(name + ": " + value);
                    if(typeof value !== "undefined")
                    {
                        obj[name] = value;
                    }
                    else if(edit_area.parent().hasClass("checkmark"))
                    {
                        // a checkmark pushes "on" or "off"
                        obj[name] = edit_area.find(".checkmark-area").hasClass("checked") ? "on" : "off";
                    }
                    else
                    {
                        // clean up the string so that way it's a bit smaller
                        obj[name] = saved_data[name].replace(/^(<br *\/?>| |\t|\n|\r|\v|\f|&nbsp;|&#160;|&#xA0;)+/, "")
                                                    .replace(/(<br *\/?>| |\t|\n|\r|\v|\f|&nbsp;|&#160;|&#xA0;)+$/, "");
                    }
                }
            }
        }
        // this test is not 100% correct for the Publish or Create Branch
        // buttons...
        if(name) // if name is defined we have at least one field defined
        {
            this._saveDialogStatus(false); // disable buttons

            obj["editor_save_mode"] = mode;
            obj["editor_session"] = jQuery("meta[name='editor_session']").attr("content");
            obj["editor_uri"] = this._titleToURI( /** @type {string} */ (jQuery("[field_name='title'] .editor-content").text()));
            url = jQuery("link[rel='canonical']").attr("href");
            url = url ? url : "/";
            keep_darken_page = true;
            jQuery.ajax(url, {
                type: "POST",
                processData: true,
                data: obj,
                error: function(jqxhr, result_status, error_msg){
                    // TODO get the messages in the HTML and display those
                    //      in a popup; later we should have one error per
                    //      widget whenever a widget is specified
                    alert("An error occured while posting AJAX (status: " + result_status + " / error: " + error_msg + ")");
                    snapwebsites.PopupInstance.darkenPage(-150);
                },
                success: function(data, result_status, jqxhr){
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
                                snapwebsites.EditorInstance._originalData[i] = saved_data[name];
                                element_modified = saved_data[name] != edit_area.html();
                                snapwebsites.EditorInstance._modified[i] = element_modified;

                                modified = modified || element_modified;
                            }
                            // if not modified while processing the POST, hide
                            // those buttons
                            if(!modified)
                            {
                                snapwebsites.EditorInstance._saveDialogPopup.fadeOut(300);
                            }

                            redirect = jqxhr.responseXML.getElementsByTagName("redirect");
                            if(redirect.length == 1)
                            {
                                // redirect the user after a successful save
                                doc = document;
                                if(snapwebsites.EditorInstance.inPopup)
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
                    snapwebsites.EditorInstance._saveDialogStatus(true); // enable buttons
                },
                dataType: "xml"
            });
        }

        if(!keep_darken_page)
        {
            snapwebsites.PopupInstance.darkenPage(-150);
        }
    },

    /** \brief Setup the save dialog status.
     *
     * when a user clicks on a save dialog button, you should call this
     * function to disable the dialog
     *
     * @param {boolean} new_status  Whether the widget is enabled (true)
     *                              or disabled (false).
     *
     * @private
     */
    _saveDialogStatus: function(new_status)
    {
        // dialog even exists?
        if(!this._saveDialogPopup)
        {
            return;
        }

        if(new_status)
        {
            jQuery(this._saveDialogPopup).parent().children("a").removeClass("disabled");
        }
        else
        {
            jQuery(this._saveDialogPopup).parent().children("a").addClass("disabled");
        }
    },

    // if anything was modified, show this save dialog
    // this includes some branching capabilities:
    //
    //    * Publish -- save a new revision in current branch
    //    * Save -- create a new branch
    //    * Save Draft -- make sure a draft is saved (1 draft per user per page)
    //
    // However, the branch the user decided to edit (i.e. with the query string
    // ...?a=edit&revision=1.2) needs to be taken in account as well.
    //
    _saveDialog: function()
    {
        if(!this._saveDialogPopup)
        {
            var html = "<style>"
                        + "#snap_editor_save_dialog{border:1px solid black;border-radius:7px;background-color:#fff8f0;padding:10px;position:fixed;top:10px;z-index:1;width:150px;}"
                        + "#snap_editor_save_dialog .title{text-align:center;}"
                        + "#snap_editor_save_dialog .button{border:1px solid black;border-radius:5px;background-color:#b0e8a0;display:block;padding:10px;margin:10px;font-weight:bold;text-align:center;font-variant:small-caps;text-decoration:none;}"
                        + "#snap_editor_save_dialog .button.disabled{background-color:#aaaaaa;}"
                        + "#snap_editor_save_dialog .description{font-size:80%;font-style:italic;}"
                    + "</style>"
                    + "<div id='snap_editor_save_dialog'>"
                    + "<h3 class='title'>Editor</h3>"
                    + "<div id='snap_editor_save_dialog_page'>"
                    + "<p class='description'>You made changes to your page. Make sure to save your modifications.</p>"
                    // this is wrong at this point because the current branch
                    // management is more complicated...
                    // (i.e. if you are editing a new branch that is not
                    //       public then Publish would make that branch
                    //       public and the Save would make that !)
                    + "<p class='snap_editor_publish_p'><a class='button' id='snap_editor_publish' href='#'>Publish</a></p>"
                    + "<p class='snap_editor_save_p'><a class='button' id='snap_editor_save' href='#'>Save</a></p>"
                    + "<p class='snap_editor_save_new_branch_p'><a class='button' id='snap_editor_save_new_branch' href='#'>Save New Branch</a></p>"
                    + "<p class='snap_editor_save_draft_p'><a class='button' id='snap_editor_save_draft' href='#'>Save Draft</a></p>"
                    + "</div></div>";
            jQuery(html).appendTo("body");
            this._saveDialogPopup = jQuery("#snap_editor_save_dialog");
            this._saveDialogPopup.css("left", jQuery(window).outerWidth(true) - 190);
            jQuery("#snap_editor_publish")
                .click(function(){
                    snapwebsites.EditorInstance._saveData("publish");
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
                    snapwebsites.EditorInstance._saveData("draft");
                });
            if(jQuery("meta[name='path']").attr("content") == "admin/drafts")
            {
                jQuery(".snap_editor_save_p").hide();
                jQuery(".snap_editor_save_new_branch_p").hide();
            }
        }
        this._saveDialogStatus(true); // enable buttons
        this._saveDialogPopup.fadeIn(300).css("display", "block");
    },

    _isEmptyBlock: function(html)
    {
        // This test is too slow for large buffers so we do not
        // do it for them; plus large buffers are not likely all empty!
        if(html.length < 256)
        {
            // replace nothingness by "background" value
            html = html.replace(/<br *\/?>| |\t|\n|\r|&nbsp;/g, "");
            if(html.length == 0)
            {
                return true;
            }
        }
        return false;
    },

    _cancel_toolbar_hide: function()
    {
        if(snapwebsites.EditorInstance._toolbarTimeoutID != -1)
        {
            // prevent hiding of the toolbar
            clearTimeout(snapwebsites.EditorInstance._toolbarTimeoutID);
        }
    },

    // The buffer is expected to be an ArrayBuffer() as read with a FileReader
    _buffer2mime: function(buffer)
    {
        var buf;

        buf = new Uint8Array(buffer);
        if(buf[0] == 0xFF
        && buf[1] == 0xD8
        && buf[2] == 0xFF
        && buf[3] == 0xE0
        && buf[4] == 0x00
        && buf[5] == 0x10
        && buf[6] == 0x4A  // J
        && buf[7] == 0x46  // F
        && buf[8] == 0x49  // I
        && buf[9] == 0x46) // F
        {
            return "image/jpeg";
        }
        if(buf[0] == 0x89
        && buf[1] == 0x50  // P
        && buf[2] == 0x4E  // N
        && buf[3] == 0x47  // G
        && buf[4] == 0x0D  // \r
        && buf[5] == 0x0A) // \n
        {
            return "image/png";
        }
        if(buf[0] == 0x47  // G
        && buf[1] == 0x49  // I
        && buf[2] == 0x46  // F
        && buf[3] == 0x38  // 8
        && buf[4] == 0x39  // 9
        && buf[5] == 0x61) // a
        {
            return "image/gif";
        }

        // unknown
        return "";
    },

    _droppedImageAssign: function(e){
        var img, id;

        img = new Image();
        img.onload = function(){
            var sizes, limit_width = 0, limit_height = 0, w, h, nw, nh,
                max_sizes, saved_active_element;

            // make sure we do it once
            img.onload = null;

            w = img.width;
            h = img.height;

            if(jQuery(e.target.snapEditorElement).attr("min-sizes"))
            {
                sizes = jQuery(e.target.snapEditorElement).attr("min-sizes").split("x");
                if(w < sizes[0] || h < sizes[1])
                {
                    // image too small...
                    // TODO: fix alert with clean error popup
                    alert("This image is too small. Minimum required is "
                            + jQuery(e.target.snapEditorElement).attr("min-sizes")
                            + ". Please try with a larger image.");
                    return;
                }
            }
            if(jQuery(e.target.snapEditorElement).attr("max-sizes"))
            {
                sizes = jQuery(e.target.snapEditorElement).attr("max-sizes").split("x");
                if(w > sizes[0] || h > sizes[1])
                {
                    // image too large...
                    // TODO: fix alert with clean error popup
                    alert("This image is too large. Maximum allowed is "
                            + jQuery(e.target.snapEditorElement).attr("max-sizes")
                            + ". Please try with a smaller image.");
                    return;
                }
            }

            if(jQuery(e.target.snapEditorElement).attr("resize-sizes"))
            {
                max_sizes = jQuery(e.target.snapEditorElement).attr("resize-sizes").split("x");
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
            jQuery(e.target.snapEditorElement).empty();
            jQuery(img)
                .attr("width", w)
                .attr("height", h)
                .attr("filename", e.target.snapEditorFile.name)
                .css({top: (limit_height - h) / 2, left: (limit_width - w) / 2, position: "relative"})
                .appendTo(e.target.snapEditorElement);

            // now make sure the editor detects the change
            saved_active_element = snapwebsites.EditorInstance._activeElement;
            snapwebsites.EditorInstance._activeElement = e.target.snapEditorElement;
            snapwebsites.EditorInstance.checkModified();
            snapwebsites.EditorInstance._activeElement = saved_active_element;
        };
        img.src = e.target.result;

        // a fix for browsers that don't call onload() if the image is
        // already considered loaded by now
        if(img.complete || img.readyState == 4)
        {
            img.onload();
        }
    },

    _droppedImage: function(e){
        var mime, r, a, blob;

        mime = snapwebsites.EditorInstance._buffer2mime(e.target.result);
        if(mime.substr(0, 6) == "image/")
        {
//console.log("image type: "+mime.substr(6)+", target = ["+e.target+"]");
            // XXX: at some point we could try to reuse the reader in target
            //      but it is certainly safer to use a new one here...
            r = new FileReader;
            r.snapEditorElement = e.target.snapEditorElement;
            r.snapEditorFile = e.target.snapEditorFile;
            r.onload = snapwebsites.EditorInstance._droppedImageAssign;
            a = [];
            a.push(e.target.snapEditorFile);
            blob = new Blob(a, {type: mime});
            r.readAsDataURL(blob);
        }
    },

    _attach: function()
    {
        var snap_editor, immediate, auto_focus, that = this;

        if(jQuery("body").hasClass("snap-editor-initialized"))
        {
            return;
        }
        jQuery("body").addClass("snap-editor-initialized");

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

        jQuery(".editor-form")
            .each(function(){
                ++snapwebsites.EditorInstance._lastFormId;
                jQuery(this)
                    .attr("id", "editor-form-" + snapwebsites.EditorInstance._lastFormId)
                    .find(".editor-content")
                    .each(function(){
                        var that = jQuery(this);
                        var html = jQuery(this).html();
                        this.objId = ++snapwebsites.EditorInstance._lastItemId;
                        that.attr("id", "editor-area-" + this.objId);
                        that.attr("refformid", snapwebsites.EditorInstance._lastFormId);
                        snapwebsites.EditorInstance._originalData[this.objId] = html;
                        snapwebsites.EditorInstance._modified[this.objId] = false;
                        // replace nothingness by "background" values
                        that.siblings(".snap-editor-background").toggle(snapwebsites.EditorInstance._isEmptyBlock(html));
                    })
                    .focus(function(){
                        snapwebsites.EditorInstance._activeElement = this;
                        if(!jQuery(this).is(".no-toolbar"))
                        {
                            snapwebsites.EditorInstance._cancel_toolbar_hide();
                            if(snapwebsites.EditorInstance.toolbarAutoVisible)
                            {
                                snapwebsites.EditorInstance.toggleToolbar(true);
                            }
                        }
                    })
                    .blur(function(){
                        // don't blur the toolbar immediately because if the user just
                        // clicked on it, it would break it
                        snapwebsites.EditorInstance._toolbarTimeoutID = setTimeout(function(){
                            snapwebsites.EditorInstance.toggleToolbar(false);
                        }, 200);
                    })
                    .keydown(function(e){
                        var used = false;
                        if(e.ctrlKey)
                        {
//console.log("ctrl "+(e.shiftKey?"+ shift ":"")+"= "+e.which+", idx = "+(snapwebsites.EditorInstance._keys[e.which + (e.shiftKey ? 0x10000 : 0)]));
                            if(snapwebsites.EditorInstance.command(snapwebsites.EditorInstance._keys[e.which + (e.shiftKey ? 0x10000 : 0)]))
                            {
                                e.preventDefault();
                                used = true;
                            }
                        }
                        else if(e.which == 0x20)
                        {
                            if(jQuery(snapwebsites.EditorInstance._activeElement).parent().is(".checkmark"))
                            {
                                jQuery(snapwebsites.EditorInstance._activeElement).find(".checkmark-area").toggleClass("checked");
                                this._editor_base.checkModified();
                                used = true;
                            }
                        }
                        if(!used)
                        {
                            if(jQuery(snapwebsites.EditorInstance._activeElement).is(".select-only"))
                            {
                                e.preventDefault();
                                e.stopPropagation();
                                used = true;
                            }
                        }
                    })
                    .on("dragenter",function(e){
                        e.preventDefault();
                        e.stopPropagation();
                        jQuery(this).parent().addClass("dragging-over");
                    })
                    .on("dragover",function(e){
                        // this is said to make things work better in some browsers...
                        e.preventDefault();
                        e.stopPropagation();
                    })
                    .on("dragleave",function(e){
                        e.preventDefault();
                        e.stopPropagation();
                        jQuery(this).parent().removeClass("dragging-over");
                    })
                    .on("drop",function(e){
                        var i, r, accept_images, accept_files;

                        // TODO:
                        // At this point this code breaks the normal behavior that
                        // properly places the image where the user wants it; I'm
                        // not too sure how we can follow up on the "go ahead and
                        // do a normal instead" without propagating the event, but
                        // I'll just ask on StackOverflow for now...

                        // remove the dragging-over class on a drop because we
                        // do not get the dragleave event otherwise
                        jQuery(this).parent().removeClass("dragging-over");

                        // always prevent the default dropping mechanism
                        // we handle the file manually all the way
                        e.preventDefault();
                        e.stopPropagation();

                        // anything transferred on widget that accepts files?
                        if(e.originalEvent.dataTransfer
                        && e.originalEvent.dataTransfer.files.length)
                        {
                            accept_images = jQuery(this).hasClass("image");
                            accept_files = jQuery(this).hasClass("attachment");
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
                                    r.snapEditorElement = this;
                                    r.snapEditorFile = e.originalEvent.dataTransfer.files[i];
                                    r.onload = accept_files ? snapwebsites.EditorInstance._droppedImage  // TODO: handle attachments (instead of just images)
                                                            : snapwebsites.EditorInstance._droppedImage;
                                    // TBD: right now we only check the first few bytes
                                    //      but we may want to increase that size later
                                    //      to allow for JPEG that have the width and
                                    //      height defined (much) further in the stream
                                    r.readAsArrayBuffer(r.snapEditorFile.slice(0, 64));
                                }
                            }
                        }

                        return false;
                    })
                    .on("keyup bind cut copy paste",function(){
                        this._editor_base.checkModified();
                    });
            });

        immediate.filter(".auto-focus").children(".editor-content").focus();

        // backgrounds are positioned as "absolute" so their width
        // is "broken" and we cannot center them in their parent
        // which is something we want to do for image-box objects
        jQuery(".snap-editor.image-box").each(function(){
            var that = jQuery(this);
            var background = that.children(".snap-editor-background");
            background.css("width", /** @type {number} */ (that.width()))
                      .css("margin-top", (that.height() - background.height()) / 2);
        });

        jQuery(".editable.checkmark").click(function(e){
            var that = jQuery(this);

            // the default may do weird stuff, so avoid it!
            e.preventDefault();
            e.stopPropagation();

            that.find(".checkmark-area").toggleClass("checked");
            that.children(".editor-content").focus();

            this._editor_base.checkModified();
        });

        jQuery(".editable.dropdown")
            .each(function(){
               jQuery(this).children(".dropdown-items").css("position", "absolute");
            })
            .click(function(e){
                var that = jQuery(this), visible, z;

                // avoid default browser behavior
                e.preventDefault();
                e.stopPropagation();

                visible = that.is(".disabled") || that.children(".dropdown-items").is(":visible");
                if(snapwebsites.EditorInstance._openDropdown)
                {
                    snapwebsites.EditorInstance._openDropdown.fadeOut(150);
                    snapwebsites.EditorInstance._openDropdown = null;
                }
                if(!visible)
                {
                    snapwebsites.EditorInstance._openDropdown = that.children(".dropdown-items");

                    // setup z-index
                    // (reset itself first so we don't just +1 each time)
                    snapwebsites.EditorInstance._openDropdown.css("z-index", 0);
                    z = jQuery("div.zordered").maxZIndex() + 1;
                    snapwebsites.EditorInstance._openDropdown.css("z-index", z);

                    snapwebsites.EditorInstance._openDropdown.fadeIn(150);
                }
            })
            .find(".dropdown-items .dropdown-item").click(function(e){
                var that = jQuery(this), content, items;

                // avoid default browser behavior
                e.preventDefault();
                e.stopPropagation();

                items = that.parents(".dropdown-items");
                items.toggle();
                items.find(".dropdown-item").removeClass("selected");
                that.addClass("selected");
                content = items.siblings(".editor-content");
                content.empty();
                content.append(that.html());
                if(that.attr("value") != "")
                {
                    content.attr("value", /** @type {string} */ (that.attr("value")));
                }
                else
                {
                    content.removeAttr("value");
                }

                this._editor_base.checkModified();
            });
        jQuery(".editable.dropdown .editor-content")
            .blur(function(e){
                if(snapwebsites.EditorInstance._openDropdown)
                {
                    snapwebsites.EditorInstance._openDropdown.fadeOut(150);
                    snapwebsites.EditorInstance._openDropdown = null;
                }
            });

        // Make labels focus the corresponding editable box
        jQuery("label[for!='']").click(function(e){
            // the default may recapture the focus, so avoid it!
            e.preventDefault();
            e.stopPropagation();
            jQuery("div[name='"+jQuery(this).attr("for")+"']").focus();
        });
    },

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
    isModified: function(widget)
    {
        // check whether this active element was modified
        if(!this._modified[widget.objId])
        {
            this._modified[widget.objId] = this._originalData[widget.objId] != jQuery(widget).html();
            return this._modified[widget.objId];
        }
        return false;
    },

    /** \brief This function checks whether the form was modified.
     *
     * This function checks whether the form was modified. First it
     * checks whether the form is to be saved. If not then the function
     * just returns false.
     *
     * @return {boolean} true if the form was modified.
     */
    wasModified: function(w)
    {
        return true;
    }
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
 * @struct
 */
snapwebsites.Editor = function()
{
    

    this._createToolbar();
    this._attachToForms();
    this._unload();

    return this;
};


snapwebsites.Editor.prototype = new snapwebsites.EditorBase();


/** \brief The constructor of this object.
 *
 * Make sure to declare the constructor for proper inheritance
 * support.
 *
 * @type {function()}
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
snapwebsites.Editor.prototype._toolbar = null;

/** \brief List of EditorForm objects.
 *
 * This variable member holds the array of EditorForm objects created
 * on initialization or dynamically later.
 *
 * @type {Array.<snapwebsites.EditorForm>}
 * @private
 */
snapwebsites.Editor.prototype._editor_forms = [];

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
snapwebsites.Editor.prototype._unloadCalled = false;

/** \brief The link dialog.
 *
 * The get_link_dialog() function creates this link dialog the first
 * time it is called.
 *
 * @type {snapwebsites.EditorLinkDialog}
 * @private
 */
snapwebsites.Editor.prototype._link_dialog = null;

/** \brief Create an editor toolbar.
 *
 * This function creates and initialize the editor toolbar.
 */
snapwebsites.Editor.prototype._createToolbar = function()
{
    this._toolbar = new snapwebsites.EditorToolbar(this);
};

/** \brief Attach to the EditorForms defined in the DOM.
 *
 * This function attaches the editor to the existing editor forms
 * as defined in the DOM. Editor forms are detected by the fact
 * that a \<div\> tag has class ".editor-form".
 *
 * The function is expected to be called only once.
 */
snapwebsites.Editor.prototype._attachToForms = function()
{
    var that = this;

    jQuery(".editor-form")
        .each(function(){
            that._editor_forms.push(new snapwebsites.EditorForm(that));
        });
};

/** \brief Capture the unload event.
 *
 * This function adds the necessary code to handle the unload event.
 * This is used to make sure that users will save their data before
 * they actually close their browser window or tab.
 *
 * The function loops through all the editor forms and if one or
 * more was modified, then it displays a message saying so.
 *
 * \return A message saying that something wasn't saved if it applies.
 */
snapwebsites.Editor.prototype._unload = function()
{
    var that = this;

    jQuery(window).bind("beforeunload", function()
    {
        var i, max;

        if(!that._unloadCalled)
        {
            max = that._editor_forms.length;
            for(i = 0; i < max; ++i)
            {
                if(that._editor_forms[i].wasModified(true))
                {
                    // add this flag and timeout to avoid a doulbe
                    // "are you sure?" under Firefox browsers
                    that._unloadCalled = true;
                    setTimeout(function(){
                            that._unloadCalled = false;
                        }, 20);

                    // TODO: translation
                    return "You made changes to this page! Click Cancel to avoid closing the window and Save your changes first.";
                }
            }
        }
    });
};

/** \brief Retrieve the toolbar object.
 *
 * This function returns a reference to the toolbar object.
 *
 * @return {snapwebsites.EditorToolbar} The toolbar object.
 */
snapwebsites.Editor.prototype.getToolbar = function() // virtual
{
    return this._toolbar;
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
 */
snapwebsites.Editor.prototype.checkModified = function() // virtual
{
    var html, form_element, form_idx;

    // checkMofified only applies to the active element so make sure
    // there is one when called
    if(this._activeElement)
    {
        // allow the toolbar to adjust itself (move to a new location)
        this._toolbar.checkPosition();

        // get the form in which this element is defined
        // and call the isModified() function on it
        if(this._activeElement.editorForm.isModified(this._activeElement))
        {
            this._saveDialog();
        }

        // replace nothingness by "background" value
        jQuery(this._activeElement).siblings(".snap-editor-background").toggle(this._isEmptyBlock(jQuery(this._activeElement).html()));
    }
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
snapwebsites.Editor.prototype.get_link_dialog = function()
{
    if(!this._link_dialog)
    {
        this._link_dialog = new snapwebsites.EditorLinkDialog(this);
    }
    return this._link_dialog;
};



// auto-initialize
jQuery(document).ready(
    function()
    {
        snapwebsites.EditorInstance = new snapwebsites.Editor();
    }
);
// vim: ts=4 sw=4 et
