/** @preserve
 * Name: change-email
 * Version: 0.0.0.2
 * Browsers: all
 * Depends: users_ui (>= 0.0.3.104), editor (>=0.0.3.922)
 * Copyright: Copyright 2013-2016 (c) Made to Order Software Corporation  All rights reverved.
 * License: GPL 2.0
 */

//
//
// Inline "command line" parameters for the Google Closure Compiler
// https://developers.google.com/closure/compiler/docs/js-for-compiler
//
// See output of:
//    java -jar .../google-js-compiler/compiler.jar --help
//
// ==ClosureCompiler==
// @compilation_level ADVANCED_OPTIMIZATIONS
// @externs $CLOSURE_COMPILER/contrib/externs/jquery-1.9.js
// @externs /home/snapwebsites/BUILD/dist/share/javascript/snapwebsites/externs/jquery-extensions.js
// @js /home/snapwebsites/BUILD/dist/share/javascript/snapwebsites/output/output.js
// @js /home/snapwebsites/BUILD/dist/share/javascript/snapwebsites/output/popup.js
// @js /home/snapwebsites/BUILD/dist/share/javascript/snapwebsites/server_access/server-access.js
// @js /home/snapwebsites/BUILD/dist/share/javascript/snapwebsites/listener/listener.js
// @js /home/snapwebsites/BUILD/dist/share/javascript/snapwebsites/editor/editor.js
// @js plugins/users_ui/users_ui.js
// ==/ClosureCompiler==
//
// This is not required and it may not exist at the time you run the
// JS compiler against this file (it gets generated)
// --js plugins/mimetype/mimetype-basics.js
//



/** \brief Handle the user settings.
 *
 * This function initializes a User object to handle the
 * user settings.
 *
 * @return {users_ui.User}  A reference to this object.
 *
 * @constructor
 * @struct
 */
users_ui.User = function()
{
    var save_button,
        editor = snapwebsites.EditorInstance,
        user_form = editor.getFormByName("change-email"),
        save_dialog = user_form.getSaveDialog();

    // replacement to the Save functionality
    save_button = jQuery("a.settings-save-button");
    save_dialog.setPopup(save_button);

    save_button
        .makeButton()
        .click(function(e)
            {
                // avoid the '#' from appearing in the URI
                e.preventDefault();

                // TODO: this should actually be "publish" and not "save"
                //       once we have the publish capability in place...
                user_form.saveData("save");
            });

    return this;
};


/** \brief Mark User as a base class.
 *
 * This class does not inherit from any other classes.
 */
snapwebsites.base(users_ui.User);



// auto-initialize
jQuery(document).ready(function()
    {
        users_ui.UserInstance = new users_ui.User();
    });

// vim: ts=4 sw=4 et
