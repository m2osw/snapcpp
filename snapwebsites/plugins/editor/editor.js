/*
 * Name: editor
 * Version: 0.0.2.74
 * Browsers: all
 * Copyright: Copyright 2013-2014 (c) Made to Order Software Corporation  All rights reverved.
 * License: GPL 2.0
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
};

snapwebsites.Editor.prototype = {
    constructor: snapwebsites.Editor,
    editorStyle: "#toolbar{border:1px solid black;-moz-border-radius:5px;-webkit-border-radius:5px;border-radius:5px;padding:5px;float:left;display:none;position:absolute;z-index:1;background:white;}"
                +"#toolbar div.group{float:left;width:4px;height:16px;margin:5px;background:url(/images/editor/buttons.png) no-repeat 0 0;}"
                +"#toolbar div.button{float:left;width:16px;height:16px;padding:5px;border:1px solid white;}"
                +"#toolbar div.button:hover{background-color:#e0f0ff;border:1px solid #a0d0ff;border-radius:5px;}"
                +"#toolbar div.button .image{display:block;width:16px;height:16px;}"
                +"#toolbar .horizontal-separator{clear:both;height:3px;margin:19px 0 0;float:none;width:100%}"
                +".button.bold .image{background:url(/images/editor/buttons.png) no-repeat -4px 0;}"
                +".button.italic .image{background:url(/images/editor/buttons.png) no-repeat -132px 0;}"
                +".button.underline .image{background:url(/images/editor/buttons.png) no-repeat -292px 0;}"
                +".button.strikeThrough .image{background:url(/images/editor/buttons.png) no-repeat -244px 0;}"
                +".button.removeFormat .image{background:url(/images/editor/buttons.png) no-repeat -228px 0;}"
                +".button.subscript .image{background:url(/images/editor/buttons.png) no-repeat -260px 0;}"
                +".button.superscript .image{background:url(/images/editor/buttons.png) no-repeat -276px 0;}"
                +".button.createLink .image{background:url(/images/editor/buttons.png) no-repeat -20px 0;}"
                +".button.unlink .image{background:url(/images/editor/buttons.png) no-repeat -308px 0;}"
                +".button.insertUnorderedList .image{background:url(/images/editor/buttons.png) no-repeat -116px 0;}"
                +".button.insertOrderedList .image{background:url(/images/editor/buttons.png) no-repeat -100px 0;}"
                +".button.outdent .image{background:url(/images/editor/buttons.png) no-repeat -212px 0;}"
                +".button.indent .image{background:url(/images/editor/buttons.png) no-repeat -52px 0;}"
                +".button.formatBlock .image{background:url(/images/editor/buttons.png) no-repeat -36px 0;}"
                +".button.insertHorizontalRule .image{background:url(/images/editor/buttons.png) no-repeat -84px 0;}"
                +".button.insertFieldset .image{background:url(/images/editor/buttons.png) no-repeat -68px 0;}"
                +".button.justifyLeft .image{background:url(/images/editor/buttons.png) no-repeat -180px 0;}"
                +".button.justifyCenter .image{background:url(/images/editor/buttons.png) no-repeat -148px 0;}"
                +".button.justifyRight .image{background:url(/images/editor/buttons.png) no-repeat -196px 0;}"
                +".button.justifyFull .image{background:url(/images/editor/buttons.png) no-repeat -164px 0;}"
                +".snap-editor:hover{box-shadow:inset 0 0 0 3px rgba(64, 192, 64, 0.5);}"
                +".editor-tooltip{display:none;padding:10px;position:absolute;z-index:1;border:1px solid black;border-radius:7px;background:#f0fff0;color:#0f000f;}"
                +".snap-editor [contenteditable=\"true\"] .filter-token{background-color:#e0e0e0;}"
                ,
    // TODO: support for translations
    //
    // WARNING: Some control keys cannot be used under different browsers
    //          (especially Internet Explorer which does not care much
    //          whether you try to capture those controls.)
    //
    //    . Ctrl-O -- open a new URL
    //
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
    inPopup: false,

    _unloadCalled: false,
    _unloadTimeoutID: -1,
    _toolbarTimeoutID: -1,
    _bottomToolbar: false,
    _toolbar: null,
    _toolbarVisible: false,
    _height: -1,
    _activeElement: null,
    _lastId: 0,
    _uniqueId: 0, // for images at this point
    _originalData: [],
    _modified: [],
    _linkDialogPopup: null,
    _savedTextRange: null,
    _savedRange: null,
    _msie: false,
    _keys: [],
    _openDropdown: null,

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
        //// remove dashes at the start & end
        title = title.replace(/^-+/, "");
        title = title.replace(/-+$/, "");

        return title;
    },

    _saveData: function(mode)
    {
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
        snapwebsites.Popup.darkenPage(150);

        var i, obj = {}, saved_data = {}, saved = [], edit_area, url, name, keep_darken_page = false;
        for(i = 1; i <= this._lastId; ++i)
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

                    if(edit_area.parent().hasClass("checkmark"))
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
            obj["editor_uri"] = this._titleToURI(jQuery("[field_name='title'] .editor-content").text());
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
                },
                success: function(data, result_status, jqxhr){
                    var i, modified, element_modified, results, doc;

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
                        alert("The server replied with an error while posting AJAX (status: " + result_status + " / error: " + error_msg + ")");
                    }
                    snapwebsites.Popup.darkenPage(-150);
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
            snapwebsites.Popup.darkenPage(-150);
        }
    },

    // when a user clicks on a save dialog button, you should call this
    // function to disable the dialog
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
                    + "</div></div>"
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

    _linkDialog: function(idx)
    {
        if(!this._linkDialogPopup)
        {
            var html = "<style>#snap_editor_link_dialog{display:none;position:absolute;z-index:2;float:left;background:#f0f0ae;padding:0;border:1px solid black;-moz-border-bottom-right-radius:10px;-webkit-border-bottom-right-radius:10px;border-bottom-right-radius:10px;-moz-border-bottom-left-radius:10px;-webkit-border-bottom-left-radius:10px;border-bottom-left-radius:10px;}#snap_editor_link_page{margin:0;padding:10px;}#snap_editor_link_dialog div.line{clear:both;padding:5px 3px;}#snap_editor_link_dialog label.limited{display:block;float:left;width:80px;}#snap_editor_link_dialog input{display:block;float:left;width:150px;}#snap_editor_link_dialog div.title{background:black;color:white;font-weight:bold;padding:5px;}</style>"
                    + "<div id='snap_editor_link_dialog'>"
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
            jQuery("#snap_editor_link_dialog #snap_editor_link_ok")
                .click(function(){
                    snapwebsites.EditorInstance._linkDialogPopup.fadeOut(150);
                    snapwebsites.Popup.darkenPage(-150);
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
        snapwebsites.Popup.darkenPage(150);
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

    _findFirstVisibleTextNode: function(p)
    {
        var textNodes, nonWhitespaceMatcher = /\S/;

        function __findFirstVisibleTextNode(p)
        {
            var i, max, result, child;

            // text node?
            if(p.nodeType == 3) // Node.TEXT_NODE == 3
            {
                return p;
            }

            max = p.childNodes.length;
            for(i = 0; i < max; ++i)
            {
                child = p.childNodes[i];

                // verify visibility first (avoid walking the tree of hidden nodes)
//console.log("node " + child + " has display = [" + jQuery(child).css("display") + "]");
//                if(jQuery(child).css("display") == "none")
//                {
//                    return result;
//                }

                result = __findFirstVisibleTextNode(child);
                if(result)
                {
                    return result;
                }
            }
        }

        return __findFirstVisibleTextNode(p);
    },

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
        var sel = window.getSelection();
        var range = sel.getRangeAt(0);

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
            this._height = jQuery(snapwebsites.EditorInstance._activeElement).parents('.snap-editor').height();
            var toolbarHeight = this._toolbar.outerHeight();
            var pos = jQuery(snapwebsites.EditorInstance._activeElement).parents('.snap-editor').position();
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
console.log("command "+idx+" "+this.toolbarButtons[idx][2]+"!!!");
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

        var html = "<style>" + this.editorStyle + "</style><div id=\"toolbar\">";
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
                        html += "<div class=\"group\" onclick='snapwebsites.EditorInstance._refocus();'></div>";
                    }
                }
                else
                {
                    // Buttons
                    html += "<div unselectable=\"on\" class=\"button " + originalName
                            + "\" onclick='javascript:snapwebsites.EditorInstance._refocus();snapwebsites.EditorInstance._command("
                            + idx + ");' title=\"" + this.toolbarButtons[idx][1] + "\">"
                            + "<span class=\"image\"></span></div>";
                }
            }
        }
        html += "</div>";
        jQuery(html).appendTo("body");
        this._toolbar = jQuery("#toolbar");

        this._toolbar.click(function(e){snapwebsites.EditorInstance._refocus();e.preventDefault();});
        this._toolbar.mousedown(function(e){snapwebsites.EditorInstance._cancel_toolbar_hide();e.preventDefault();});
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

    _checkModified: function()
    {
        var html = jQuery(this._activeElement).html();
        if(!this._modified[this._activeElement.objId])
        {
            this._modified[this._activeElement.objId] = this._originalData[this._activeElement.objId] != html;
            if(this._modified[this._activeElement.objId])
            {
                this._saveDialog();
            }
        }
        // replace nothingness by "background" value
        jQuery(this._activeElement).siblings(".snap-editor-background").toggle(this._isEmptyBlock(html));
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
        buf = Uint8Array(buffer);
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
            var sizes, limit_width = 0, limit_height = 0, nw, nh;

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
            snapwebsites.EditorInstance._checkModified();
            snapwebsites.EditorInstance._activeElement = saved_active_element;
        };
        img.src = e.target.result;

        // a fix for browsers that don't call onload() if the image is
        // already considered loaded by now
        if(img.complete || img.readyState === 4)
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
        var snap_editor, immediate, auto_focus;

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

        snap_editor
            .children(".editor-content")
            .each(function(){
                var that = jQuery(this);
                var html = jQuery(this).html();
                this.objId = ++snapwebsites.EditorInstance._lastId;
                that.attr("id", "editor-area-" + this.objId);
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
                        snapwebsites.EditorInstance._toggleToolbar(true);
                    }
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
                var used = false;
                if(e.ctrlKey)
                {
//console.log("ctrl "+(e.shiftKey?"+ shift ":"")+"= "+e.which+", idx = "+(snapwebsites.EditorInstance._keys[e.which + (e.shiftKey ? 0x10000 : 0)]));
                    if(snapwebsites.EditorInstance._command(snapwebsites.EditorInstance._keys[e.which + (e.shiftKey ? 0x10000 : 0)]))
                    {
                        e.preventDefault();
                        used = true;
                    }
                }
                else if(e.which == 0x20)
                {
                    if(jQuery(snapwebsites.EditorInstance._activeElement).is(".checkmark"))
                    {
                        jQuery(snapwebsites.EditorInstance._activeElement).find(".checkmark-area").toggleClass("checked");
                        snapwebsites.EditorInstance._checkModified();
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

        immediate.filter(".auto-focus").children(".editor-content").focus();

        // backgrounds are positioned as "absolute" so their width
        // is "broken" and we cannot center them in their parent
        // which is something we want to do for image-box objects
        jQuery(".snap-editor.image-box").each(function(){
            var that = jQuery(this);
            var background = that.children(".snap-editor-background");
            background.css("width", that.width())
                      .css("margin-top", (that.height() - background.height()) / 2);
        });

        jQuery(".editable.checkmark").click(function(e){
            var that = jQuery(this);

            // the default may do weird stuff, so avoid it!
            e.preventDefault();
            e.stopPropagation();

            that.find(".checkmark-area").toggleClass("checked");
            that.children(".editor-content").focus();

            snapwebsites.EditorInstance._checkModified();
        });

        jQuery(".editable.dropdown")
            .each(function(){
               jQuery(this).children(".dropdown-items").css("position", "absolute"); 
            })
            .click(function(e){
                var that = jQuery(this), visible;

                // avoid default browser behavior
                e.preventDefault();
                e.stopPropagation();

                visible = that.children(".dropdown-items").is(":visible");
                if(snapwebsites.EditorInstance._openDropdown)
                {
                    snapwebsites.EditorInstance._openDropdown.fadeOut(150);
                    snapwebsites.EditorInstance._openDropdown = null;
                }
                if(!visible)
                {
                    snapwebsites.EditorInstance._openDropdown = that.children(".dropdown-items");
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
                    content.attr("value", that.attr("value"));
                }
                else
                {
                    content.removeAttr("value");
                }

                snapwebsites.EditorInstance._checkModified();
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

    _unload: function()
    {
        jQuery(window).bind("beforeunload",function(){
            if(!snapwebsites.EditorInstance._unloadCalled)
            {
                var i;
                for(i = 1; i <= snapwebsites.EditorInstance._lastId; ++i)
                {
                    if(snapwebsites.EditorInstance._modified[i])
                    {
                        // verify one last time whether it was indeed modified
                        if(snapwebsites.EditorInstance._originalData[i] != jQuery("#editor-area-" + i).html())
                        {
                            snapwebsites.EditorInstance._unloadCalled = true;
                            snapwebsites.EditorInstance._unloadTimeoutID = setTimeout(function(){
                                snapwebsites.EditorInstance._unloadCalled = false;
                            },20);
                            return "You made changes to the page! Click Cancel to avoid closing the window and Save first.";
                        }
                    }
                }
            }
        });
    },

    init: function()
    {
        this._createToolbar();
        this._attach();
        this._unload();
    }
};

// auto-initialize
jQuery(document).ready(function(){
    snapwebsites.EditorInstance = new snapwebsites.Editor();
    snapwebsites.EditorInstance.init();
});
// vim: ts=4 sw=4 et
