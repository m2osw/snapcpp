/*
 *
 */

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

// Code verification with Google Closure Compiler:
// Source: http://code.google.com/p/closure-compiler/
// Source: https://developers.google.com/closure/compiler/docs/error-ref
// Source: http://www.jslint.com/
//
// Command line one can use to verify the editor:
//
//   java -jar .../google-js-closure-compiler/compiler.jar \
//        --warning_level VERBOSE \
//        tests/jquery-for-closure.js \
//        plugins/content/content_0.0.1.js \
//        plugins/editor/editor_0.0.1.js \
//        --js_output_file /de/vnull \
//        2>&1 | less -S
//
// WARNING: output of that command is garbage as far as we're concerned
//          only the warnings are of interest.
//

var snapwebsites = {};

/** \brief Snap Editor constructor.
 *
 * \note
 * The Snap! Editor is a singleton and should never be created by you. It
 * gets initialized automatically when this editor.js file gets included.
 *
 * @constructor
 */
snapwebsites.Editor = function()
{
    this._init();
};

snapwebsites.Editor.prototype = {
    constructor: snapwebsites.Editor,
    modified: false,
    toolbarStyle: "#toolbar{border:1px solid black;-moz-border-radius:5px;-webkit-border-radius:5px;border-radius:5px;padding:5px;float:left;display:none;position:absolute;z-index:1;background:white;}#toolbar div.button{float:left;}#toolbar div.button img{display:block;padding:0 5px;}#toolbar .horizontal-separator{clear:both;height:3px;margin:19px 0 0;float:none;width:100%}",
    // TODO: support for translations
    toolbarButtons: [
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
        ["*", null, 0x54, "toggleToolbar"]         // Ctrl-T
    ],
    toolbarAutoVisible: true,

    _toolbarTimeoutID: -1,
    _bottomToolbar: false,
    _toolbar: null,
    _toolbarVisible: false,
    _height: -1,
    _activeElement: null,
    _saveData: null,
    _linkDialogPopup: null,
    _savedTextRange: null,
    _savedRange: null,
    _msie: false,
    _keys: [],

    _linkDialog: function(idx)
    {
        if(!this._linkDialogPopup)
        {
            var html = "<style>#linkDialog{display:none;position:absolute;z-index:2;float:left;background:#f0f0ae;padding:0;border:1px solid black;-moz-border-bottom-right-radius:10px;-webkit-border-bottom-right-radius:10px;border-bottom-right-radius:10px;-moz-border-bottom-left-radius:10px;-webkit-border-bottom-left-radius:10px;border-bottom-left-radius:10px;}#snap_editor_link_page{margin:0;padding:10px;}#linkDialog div.line{clear:both;padding:5px 3px;}#linkDialog label.limited{display:block;float:left;width:80px;}#linkDialog input{display:block;float:left;width:150px;}#darkenPage{position:fixed;z-index:1;left:0;top:0;width:100%;height:100%;background-color:black;opacity:0.2;filter:alpha(opacity=20);}#linkDialog div.title{background:black;color:white;font-weight:bold;padding:5px;}</style>"
                    + "<div id='darkenPage'></div>"
                    + "<div id='linkDialog'>"
                    + "<div class='title'>Link Administration</div>"
                    + "<div id='snap_editor_link_page'>"
                    + "<div class='line'><label class='limited' for='snap_editor_link_text'>Text:</label> <input id='snap_editor_link_text' name='text' title='Enter the text representing the link. If none, the link will appear as itself.'/></div>"
                    + "<div class='line'><label class='limited' for='snap_editor_link_url'>Link:</label> <input id='snap_editor_link_url' name='url' title='Enter a URL.'/></div>"
                    + "<div class='line'><label class='limited' for='snap_editor_link_title'>Tooltip:</label> <input id='snap_editor_link_title' name='title' title='The tooltip which appears when a user hovers the mouse cursor over the link.'/></div>"
                    + "<div class='line'><label class='limited'>&nbsp;</label><input id='snap_editor_link_new_window' type='checkbox' value='' title='Click to save your changes.'/> <label for='snap_editor_link_new_window'>New Window</label></div>"
                    + "<div class='line'><label class='limited'>&nbsp;</label><input id='snap_editor_link_ok' type='button' value='OK' title='Click to save your changes.'/></div>"
                    + "<div style='clear:both;padding:0;'></div></div></div>";
            jQuery(html).appendTo("body");
            this._linkDialogPopup = jQuery("#linkDialog");
            jQuery("#linkDialog #snap_editor_link_ok")
                .click(function(){
                    snapwebsites.EditorInstance._linkDialogPopup.fadeOut(150);
                    jQuery("#darkenPage").fadeOut(150);
                    snapwebsites.EditorInstance._refocus();
                    snapwebsites.EditorInstance._restoreSelection();
                    var url = jQuery("#snap_editor_link_url");
                    document.execCommand("createLink", false, url.val());
                    var links = snapwebsites.EditorInstance.getLinksInSelection();
                    if(links.length > 0)
                    {
                        var jtag = jQuery(links[0]);
                        var text = jQuery("#snap_editor_link_text");
                        if(text.length > 0)
                        {
                            jtag.text(text.val());
                        }
                        // do NOT erase the existing text if the user OKed
                        // without any text

                        var title = jQuery("#snap_editor_link_title");
                        if(title.length > 0)
                        {
                            jtag.attr("title", title.val());
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
                });
        }
        var jtag = jQuery(this.getSelectionBoundaryElement(true));
        var selectionText = this.getSelectionText();
        var links = snapwebsites.EditorInstance.getLinksInSelection();
        var new_window = true;
        if(links.length > 0)
        {
            jtag = jQuery(links[0]);
            // it is already the anchor, we can use the text here
            // in this case we also have a URL and possibly a title
            jQuery("#snap_editor_link_url").val(jtag.attr("href"));
            jQuery("#snap_editor_link_title").val(jtag.attr("title"));
            new_window = jtag.attr("target") == "_blank";
        }
        else
        {
            // this is not yet the anchor, we need to retrieve the selection
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
        jQuery("#darkenPage").fadeIn(150);
    },

    _saveSelection: function()
    {
        if(document.selection)
        {
            this._savedTextRange = document.selection.createRange();
        }
        else
        {
            var sel = window.getSelection();
            if(sel.getRangeAt && sel.rangeCount > 0)
            {
                this._savedRange = sel.getRangeAt(0);
            }
            else
            {
                this._savedRange = null;
            }
        }
    },

    _restoreSelection: function()
    {
        if(this._savedTextRange)
        {
            this._savedTextRange.select();
        }
        else
        {
            var sel = window.getSelection();
            sel.removeAllRanges();
            sel.addRange(this._savedRange);
        }
    },

    getSelectionText: function()
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
    },

    _trimSelectionText: function()
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

    getSelectionBoundaryElement: function(isStart)
    {
        var range;
        if(document.selection)
        {
            // Note that IE offers a RangeText here, not a Range
            range = document.selection.createRange();
            range.collapse(isStart);
            return range.parentElement();
        }
        else
        {
            var sel = window.getSelection();
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
                var container = range[isStart ? "startContainer" : "endContainer"];

               // Check if the container is a text node and return its parent if so
               return container.nodeType === 3 ? container.parentNode : container;
            }
        }
    },

    setSelectionBoundaryElement: function(tag)
    {
        // This works since IE9
        var range = document.createRange();
        range.setStartBefore(tag);
        range.setEndAfter(tag);
        var sel = window.getSelection();
        sel.removeAllRanges();
        sel.addRange(range);
    },

    getLinksInSelection: function() 
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

    _toggleToolbar: function(force)
    {
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
            this._toolbar.fadeIn(300);
            this._height = jQuery(snapwebsites.EditorInstance._activeElement).height();
            var toolbarHeight = this._toolbar.outerHeight();
            var pos = jQuery(snapwebsites.EditorInstance._activeElement).position();
            this._bottomToolbar = pos.top < toolbarHeight + 5;
            if(this._bottomToolbar)
            {
                // too low, put the toolbar at the bottom
                this._toolbar.css("top", (pos.top + snapwebsites.EditorInstance._height + 3) + "px");
            }
            else
            {
                this._toolbar.css("top", (pos.top - toolbarHeight - 3) + "px");
            }
            this._toolbar.css("left", (pos.left + 5) + "px");
        }
        else
        {
            this._toolbar.fadeOut(150);
        }
    },

    _command: function(idx)
    {
        if(!this.toolbarButtons[idx])
        {
            return false;
        }

        // require better selection for a link? (i.e. full link)
        if(this.toolbarButtons[idx][2] & 0x20000)
        {
            this._trimSelectionText();
            var tag = this.getSelectionBoundaryElement(true);
            var tagName = jQuery(tag).prop("tagName");
            if(tagName == "A")
            {
                // if we get here the whole tag was not selected,
                // select it now
                this.setSelectionBoundaryElement(tag);
            }
            else
            {
                // if this is for a new link, trim the start and end
                // spaces in the selection
            }
        }

        if(this.toolbarButtons[idx][2] & 0x40000)
        {
            this._saveSelection();
            eval("snapwebsites.EditorInstance." + this.toolbarButtons[idx][4] + "(" + idx + ")");
            // the dialog OK button will do the rest of the work as
            // required by this specific entry
            return true;
        }

        if(this.toolbarButtons[idx][0] == "*")
        {
            // "internal command"
            switch(this.toolbarButtons[idx][3])
            {
            case "toggleToolbar":
                this._toggleToolbar("");
                break;

            }
        }
        else
        {
            // if there is a toolbar parameter, make sure to pass it along
            if(this.toolbarButtons[idx][3])
            {
                // TODO need to define the toolbar parameter
                document.execCommand(this.toolbarButtons[idx][0], false, this.toolbarButtons[idx][3]);
            }
            else
            {
                document.execCommand(this.toolbarButtons[idx][0], false, null);
            }
        }
        this._checkModified();

        return true;
    },

    _refocus: function()
    {
        if(this._activeElement)
        {
            this._activeElement.focus();
        }
    },

    _createToolbar: function()
    {
        // already initialized?
        if(this._toolbar)
        {
            return;
        }

        // IE?
        this._msie = /msie/.exec(navigator.userAgent.toLowerCase());

        var html = "<style>" + this.toolbarStyle + "</style><div id=\"toolbar\">";
        var originalName, isGroup;
        var idx, max = this.toolbarButtons.length;
        for(idx = 0; idx < max; ++idx)
        {
            // the name of the image always uses the original name
            originalName = this.toolbarButtons[idx][0];
            if(this._msie)
            {
                if(this.toolbarButtons[idx][0] == "hiliteColor")
                {
                    this.toolbarButtons[idx][0] = "backColor";
                }
            }
            else
            {
                if(this.toolbarButtons[idx][0] == "insertFieldset")
                {
                    this.toolbarButtons[idx][0] = "insertHTML";
                    this.toolbarButtons[idx][3] = "<fieldset><legend>Fieldset</legend><p>&nbsp;</p></fieldset>";
                }
            }
            isGroup = this.toolbarButtons[idx][0] == "|";
            if(!isGroup)
            {
                this._keys[this.toolbarButtons[idx][2] & 0x1FFFF] = idx;
            }
            if(this.toolbarButtons[idx][1] != null)
            {
                if(isGroup)
                {
                    if(this.toolbarButtons[idx][1] == "-")
                    {
                        // horizontal separator, create a new line
                        html += "<div class=\"horizontal-separator\" onclick='snapwebsites.EditorInstance._refocus();'></div>";
                    }
                    else
                    {
                        // vertical separator, show a small vertical bar
                        html += "<div class=\"button\" onclick='snapwebsites.EditorInstance._refocus();'><img alt=\"Vertical Separator\" src=\"buttons/group.png\"/></div>";
                    }
                }
                else
                {
                    // Buttons look like this:
                    //<div class="button"><img onclick='snapwebsites.EditorInstance._command("bold")' alt="Bold (Ctrl-B)" title="Bold (Ctrl-B)" src="buttons/bold.png"/></div>
                    html += "<div class=\"button\"><img onclick='javascript:snapwebsites.EditorInstance._refocus();snapwebsites.EditorInstance._command(\""
                            + idx + "\");' alt=\""
                            + this.toolbarButtons[idx][1] + "\" title=\""
                            + this.toolbarButtons[idx][1] + "\" src=\"buttons/"
                            + originalName + ".png\"/></div>";
                }
            }
        }
        html += "</div>";
        jQuery(html).appendTo("body");
        this._toolbar = jQuery("#toolbar");

        // TODO: a click on the toolbar in a location that is not a button
        //       loses the active element selection
        this._toolbar.click(function(e){snapwebsites.EditorInstance._refocus();e.preventDefault();});
    },

    _checkModified: function()
    {
        if(!this.modified)
        {
            var newData = jQuery(this).html();
            if(this._saveData != newData)
            {
                this._saveData = newData;
                this.modified = true;
console.log("just modified!!!");
            }
        }
    },

    _attach: function()
    {
        jQuery(".snap-editor")
            .focus(function(){
                snapwebsites.EditorInstance._activeElement = this;
                if(snapwebsites.EditorInstance._toolbarTimeoutID != -1)
                {
                    // prevent hiding of the toolbar
                    clearTimeout(snapwebsites.EditorInstance._toolbarTimeoutID);
                }
                snapwebsites.EditorInstance._saveData = jQuery(this).html();
                if(snapwebsites.EditorInstance.toolbarAutoVisible)
                {
                    snapwebsites.EditorInstance._toggleToolbar(true);
                }
            })
            .blur(function(){
                // don't blur the toolbar immediately because if the user just
                // clicked on it, it would break it
                snapwebsites.EditorInstance._toolbarTimeoutID = setTimeout(function(){
                    snapwebsites.EditorInstance._toggleToolbar(false);
                }, 200);
            })
            .keydown(function(e){
                if(e.ctrlKey)
                {
//console.log("ctrl "+(e.shiftKey?"+ shift ":"")+"= "+e.which+", idx = "+(snapwebsites.EditorInstance._keys[e.which + (e.shiftKey ? 0x10000 : 0)]));
                    if(snapwebsites.EditorInstance._command(snapwebsites.EditorInstance._keys[e.which + (e.shiftKey ? 0x10000 : 0)]))
                    {
                        e.preventDefault();
                    }
                }
            })
            .bind("keyup bind cut copy paste",function(){
                if(snapwebsites.EditorInstance._bottomToolbar)
                {
                    var newHeight = jQuery(this).outerHeight();
                    if(newHeight != snapwebsites.EditorInstance._height)
                    {
                        snapwebsites.EditorInstance._height = newHeight;
                        var pos = jQuery(this).position();
                        snapwebsites.EditorInstance._toolbar.animate({top: pos.top + jQuery(this).height() + 3}, 200);
                    }
                }
                snapwebsites.EditorInstance._checkModified();
            })
        ;
    },

    _unload: function()
    {
        jQuery(window)
            .bind('beforeunload', function(){
                if(snapwebsites.EditorInstance.modified)
                {
                    return "You made changes to the page! Click Cancel to avoid closing the window and Save first.";
                }
            })
        ;
    },

    _init: function()
    {
        this._createToolbar();
        this._attach();
        this._unload();
    }
};

// auto-initialize
jQuery(document).ready(function(){snapwebsites.EditorInstance = new snapwebsites.Editor();});
// vim: ts=4 sw=4 et
