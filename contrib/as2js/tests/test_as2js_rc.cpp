/* test_as2js_rc.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_rc.h"
#include    "test_as2js_main.h"

#include    "rc.h"
#include    "as2js/exceptions.h"
#include    "as2js/message.h"

#include    <unistd.h>
#include    <sys/stat.h>

#include    <cstring>
#include    <algorithm>
#include    <iomanip>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsRCUnitTests );

namespace
{


class test_callback : public as2js::MessageCallback
{
public:
    test_callback()
    {
        as2js::Message::set_message_callback(this);
        g_warning_count = as2js::Message::warning_count();
        g_error_count = as2js::Message::error_count();
    }

    ~test_callback()
    {
        // make sure the pointer gets reset!
        as2js::Message::set_message_callback(nullptr);
    }

    // implementation of the output
    virtual void output(as2js::message_level_t message_level, as2js::err_code_t error_code, as2js::Position const& pos, std::string const& message)
    {
        if(f_expected.empty())
        {
            std::cerr << "\nfilename = " << pos.get_filename() << "\n";
            std::cerr << "msg = " << message << "\n";
            std::cerr << "page = " << pos.get_page() << "\n";
            std::cerr << "line = " << pos.get_line() << "\n";
            std::cerr << "error_code = " << static_cast<int>(error_code) << "\n";
        }

        CPPUNIT_ASSERT(!f_expected.empty());

//std::cerr << "filename = " << pos.get_filename() << " / " << f_expected[0].f_pos.get_filename() << "\n";
//std::cerr << "msg = " << message << " / " << f_expected[0].f_message << "\n";
//std::cerr << "page = " << pos.get_page() << " / " << f_expected[0].f_pos.get_page() << "\n";
//std::cerr << "line = " << pos.get_line() << " / " << f_expected[0].f_pos.get_line() << "\n";
//std::cerr << "error_code = " << static_cast<int>(error_code) << " / " << static_cast<int>(f_expected[0].f_error_code) << "\n";

        CPPUNIT_ASSERT(f_expected[0].f_call);
        CPPUNIT_ASSERT(message_level == f_expected[0].f_message_level);
        CPPUNIT_ASSERT(error_code == f_expected[0].f_error_code);
        CPPUNIT_ASSERT(pos.get_filename() == f_expected[0].f_pos.get_filename());
        CPPUNIT_ASSERT(pos.get_function() == f_expected[0].f_pos.get_function());
        CPPUNIT_ASSERT(pos.get_page() == f_expected[0].f_pos.get_page());
        CPPUNIT_ASSERT(pos.get_page_line() == f_expected[0].f_pos.get_page_line());
        CPPUNIT_ASSERT(pos.get_paragraph() == f_expected[0].f_pos.get_paragraph());
        CPPUNIT_ASSERT(pos.get_line() == f_expected[0].f_pos.get_line());
        CPPUNIT_ASSERT(message == f_expected[0].f_message);

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_WARNING)
        {
            ++g_warning_count;
            CPPUNIT_ASSERT(g_warning_count == as2js::Message::warning_count());
        }

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_FATAL
        || message_level == as2js::message_level_t::MESSAGE_LEVEL_ERROR)
        {
            ++g_error_count;
//std::cerr << "error: " << g_error_count << " / " << as2js::Message::error_count() << "\n";
            CPPUNIT_ASSERT(g_error_count == as2js::Message::error_count());
        }

        f_expected.erase(f_expected.begin());
    }

    void got_called()
    {
        if(!f_expected.empty())
        {
            std::cerr << "\n*** STILL EXPECTED: ***\n";
            std::cerr << "filename = " << f_expected[0].f_pos.get_filename() << "\n";
            std::cerr << "msg = " << f_expected[0].f_message << "\n";
            std::cerr << "page = " << f_expected[0].f_pos.get_page() << "\n";
            std::cerr << "error_code = " << static_cast<int>(f_expected[0].f_error_code) << "\n";
        }
        CPPUNIT_ASSERT(f_expected.empty());
    }

    struct expected_t
    {
        bool                        f_call = true;
        as2js::message_level_t      f_message_level = as2js::message_level_t::MESSAGE_LEVEL_OFF;
        as2js::err_code_t           f_error_code = as2js::err_code_t::AS_ERR_NONE;
        as2js::Position             f_pos;
        std::string                 f_message; // UTF-8 string
    };

    std::vector<expected_t>     f_expected;

    static int32_t              g_warning_count;
    static int32_t              g_error_count;
};

int32_t   test_callback::g_warning_count = 0;
int32_t   test_callback::g_error_count = 0;

int32_t   g_empty_home_too_late = 0;

}
// no name namespace




void As2JsRCUnitTests::setUp()
{
    // verify that this user does not have existing rc files because
    // that can interfer with the tests! (and we do not want to delete
    // those under his/her feet)

    // AS2JS_RC variable
    CPPUNIT_ASSERT(getenv("AS2JS_RC") == nullptr);

    // local file
    struct stat st;
    CPPUNIT_ASSERT(stat("as2js/as2js.rc", &st) == -1);

    // user defined .config file
    as2js::String home;
    home.from_utf8(getenv("HOME"));
    as2js::String config(home);
    config += "/.config/as2js/as2js.rc";
    std::string cfg(config.to_utf8());
    CPPUNIT_ASSERT(stat(cfg.c_str(), &st) == -1);

    // system defined configuration file
    CPPUNIT_ASSERT(stat("/etc/as2js/as2js.rc", &st) == -1);
}


void As2JsRCUnitTests::test_basics()
{
    // this test is not going to work if the get_home() function was
    // already called with an empty HOME variable...
    if(g_empty_home_too_late == 2)
    {
        std::cout << " --- test_empty_home() not run, the other rc unit tests are not compatible with this test --- ";
        return;
    }

    g_empty_home_too_late = 1;

    {
        // test the get_home()
        as2js::String home;
        home.from_utf8(getenv("HOME"));
        as2js::String rc_home(as2js::rc_t::get_home());
        CPPUNIT_ASSERT(rc_home == home);

        // verify that changing the variable after the first change returns
        // the first value...
        char *new_home(reinterpret_cast<char *>(malloc(256UL)));
        CPPUNIT_ASSERT(new_home != nullptr);
        strcpy(new_home, "HOME=/got/changed/now");
        putenv(new_home);
        rc_home = as2js::rc_t::get_home();
        CPPUNIT_ASSERT(rc_home == home);

        // just in case, restore the variable
        char *restore_home(reinterpret_cast<char *>(malloc(home.to_utf8().length() + 10)));
        CPPUNIT_ASSERT(restore_home != nullptr);
        strcpy(restore_home, ("HOME=" + home.to_utf8()).c_str());
        putenv(restore_home);
    }

    {
        as2js::rc_t rc;
        CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
        CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        CPPUNIT_ASSERT(rc.get_temporary_variable_name() == "@temp");
    }

    {
        as2js::rc_t rc;

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_INSTALLATION;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "cannot find the as2js.rc file; the system default is usually put in /etc/as2js/as2js.rc";
        tc.f_expected.push_back(expected1);

        CPPUNIT_ASSERT_THROW(rc.init_rc(false), as2js::exception_exit);
        tc.got_called();

        rc.init_rc(true);

        CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
        CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
    }
}


void As2JsRCUnitTests::test_load_from_var()
{
    // this test is not going to work if the get_home() function was
    // already called with an empty HOME variable...
    if(g_empty_home_too_late == 2)
    {
        std::cout << " --- test_empty_home() not run, the other rc unit tests are not compatible with this test --- ";
        return;
    }

    g_empty_home_too_late = 1;

    // just in case it failed before...
    unlink("as2js.rc");

    {
        char rc_env[256UL];
        strcpy(rc_env, "AS2JS_RC=.");
        putenv(rc_env);

        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_INSTALLATION;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "cannot find the as2js.rc file; the system default is usually put in /etc/as2js/as2js.rc";
        tc.f_expected.push_back(expected1);

        as2js::rc_t rc;
        CPPUNIT_ASSERT_THROW(rc.init_rc(false), as2js::exception_exit);
        tc.got_called();

        {
            // create an .rc file
            {
                std::ofstream rc_file;
                rc_file.open("as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 'the/script',\n"
                        << "  'db': 'that/db',\n"
                        << "  'temporary_variable_name': '@temp$'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink("as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "the/script");
            CPPUNIT_ASSERT(rc.get_db() == "that/db");
            CPPUNIT_ASSERT(rc.get_temporary_variable_name() == "@temp$");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open("as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'db': 'that/db'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink("as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "that/db");
            CPPUNIT_ASSERT(rc.get_temporary_variable_name() == "@temp");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open("as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 'the/script'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink("as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "the/script");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
            CPPUNIT_ASSERT(rc.get_temporary_variable_name() == "@temp");
        }

        {
            // create an .rc file, with just the temporary variable name
            {
                std::ofstream rc_file;
                rc_file.open("as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  \"temporary_variable_name\": \"what about validity of the value? -- we on purpose use @ because it is not valid in identifiers\"\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink("as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
            CPPUNIT_ASSERT(rc.get_temporary_variable_name() == "what about validity of the value? -- we on purpose use @ because it is not valid in identifiers");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open("as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 123\n"
                        << "}\n";
            }

            test_callback::expected_t expected2;
            expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
            expected2.f_pos.set_filename("./as2js.rc");
            expected2.f_pos.set_function("unknown-func");
            expected2.f_pos.new_line();
            expected2.f_pos.new_line();
            expected2.f_message = "A resource file is expected to be an object of string elements.";
            tc.f_expected.push_back(expected2);

            CPPUNIT_ASSERT_THROW(rc.init_rc(true), as2js::exception_exit);
            tc.got_called();
            unlink("as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        {
            // create a null .rc file
            {
                std::ofstream rc_file;
                rc_file.open("as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "null\n";
            }

            rc.init_rc(false);
            unlink("as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        {
            // create an .rc file, without an object nor null
            {
                std::ofstream rc_file;
                rc_file.open("as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "['scripts', 123]\n";
            }

            test_callback::expected_t expected2;
            expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
            expected2.f_pos.set_filename("./as2js.rc");
            expected2.f_pos.set_function("unknown-func");
            expected2.f_pos.new_line();
            expected2.f_message = "A resource file (.rc) must be defined as a JSON object, or set to 'null'.";
            tc.f_expected.push_back(expected2);

            CPPUNIT_ASSERT_THROW(rc.init_rc(true), as2js::exception_exit);
            tc.got_called();
            unlink("as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        // test some other directory too
        strcpy(rc_env, "AS2JS_RC=/tmp");
        putenv(rc_env);

        {
            // create an .rc file
            {
                std::ofstream rc_file;
                rc_file.open("/tmp/as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 'the/script',\n"
                        << "  'db': 'that/db'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink("/tmp/as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "the/script");
            CPPUNIT_ASSERT(rc.get_db() == "that/db");
        }

        // make sure to delete that before exiting
        strcpy(rc_env, "AS2JS_RC");
        putenv(rc_env);
    }
}


void As2JsRCUnitTests::test_load_from_local()
{
    // this test is not going to work if the get_home() function was
    // already called with an empty HOME variable...
    if(g_empty_home_too_late == 2)
    {
        std::cout << " --- test_empty_home() not run, the other rc unit tests are not compatible with this test --- ";
        return;
    }

    g_empty_home_too_late = 1;

    // just in case it failed before...
    static_cast<void>(unlink("as2js/as2js.rc"));

    CPPUNIT_ASSERT(mkdir("as2js", 0700) == 0);

    {
        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_INSTALLATION;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "cannot find the as2js.rc file; the system default is usually put in /etc/as2js/as2js.rc";
        tc.f_expected.push_back(expected1);

        as2js::rc_t rc;
        CPPUNIT_ASSERT_THROW(rc.init_rc(false), as2js::exception_exit);
        tc.got_called();

        {
            // create an .rc file
            {
                std::ofstream rc_file;
                rc_file.open("as2js/as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 'the/script',\n"
                        << "  'db': 'that/db'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink("as2js/as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "the/script");
            CPPUNIT_ASSERT(rc.get_db() == "that/db");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open("as2js/as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'db': 'that/db'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink("as2js/as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "that/db");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open("as2js/as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 'the/script'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink("as2js/as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "the/script");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open("as2js/as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 123\n"
                        << "}\n";
            }

            test_callback::expected_t expected2;
            expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
            expected2.f_pos.set_filename("as2js/as2js.rc");
            expected2.f_pos.set_function("unknown-func");
            expected2.f_pos.new_line();
            expected2.f_pos.new_line();
            expected2.f_message = "A resource file is expected to be an object of string elements.";
            tc.f_expected.push_back(expected2);

            CPPUNIT_ASSERT_THROW(rc.init_rc(true), as2js::exception_exit);
            tc.got_called();
            unlink("as2js/as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        {
            // create a null .rc file
            {
                std::ofstream rc_file;
                rc_file.open("as2js/as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "null\n";
            }

            rc.init_rc(false);
            unlink("as2js/as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        {
            // create an .rc file, without an object nor null
            {
                std::ofstream rc_file;
                rc_file.open("as2js/as2js.rc");
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "['scripts', 123]\n";
            }

            test_callback::expected_t expected2;
            expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
            expected2.f_pos.set_filename("as2js/as2js.rc");
            expected2.f_pos.set_function("unknown-func");
            expected2.f_pos.new_line();
            expected2.f_message = "A resource file (.rc) must be defined as a JSON object, or set to 'null'.";
            tc.f_expected.push_back(expected2);

            CPPUNIT_ASSERT_THROW(rc.init_rc(true), as2js::exception_exit);
            tc.got_called();
            unlink("as2js/as2js.rc");

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }
    }

    // delete our temporary .rc file (should already have been deleted)
    unlink("as2js/as2js.rc");

    // if possible get rid of the directory (don't check for errors)
    rmdir("as2js");
}


void As2JsRCUnitTests::test_load_from_user_config()
{
    // this test is not going to work if the get_home() function was
    // already called with an empty HOME variable...
    if(g_empty_home_too_late == 2)
    {
        std::cout << " --- test_empty_home() not run, the other rc unit tests are not compatible with this test --- ";
        return;
    }

    g_empty_home_too_late = 1;

    as2js::String home;
    home.from_utf8(getenv("HOME"));

    // create the folders and make sure we clean up any existing .rc file
    // (although it was checked in the setUp() function and thus we should
    // not reach here if the .rc already existed!)
    as2js::String config(home);
    config += "/.config";
    std::cout << " --- config path \"" << config << "\" --- ";
    bool del_config(true);
    if(mkdir(config.to_utf8().c_str(), 0700) != 0) // usually this is 0755, but for security we cannot take that risk...
    {
        // if this mkdir() fails, it is because it exists
        CPPUNIT_ASSERT(errno == EEXIST);
        del_config = false;
    }
    as2js::String as2js_conf(config);
    as2js_conf += "/as2js";
    CPPUNIT_ASSERT(mkdir(as2js_conf.to_utf8().c_str(), 0700) == 0);
    as2js::String as2js_rc(as2js_conf);
    as2js_rc += "/as2js.rc";
    unlink(as2js_rc.to_utf8().c_str()); // delete that, just in case (the setup verifies that it does not exist)

    {
        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_INSTALLATION;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "cannot find the as2js.rc file; the system default is usually put in /etc/as2js/as2js.rc";
        tc.f_expected.push_back(expected1);

        as2js::rc_t rc;
        CPPUNIT_ASSERT_THROW(rc.init_rc(false), as2js::exception_exit);
        tc.got_called();

        {
            // create an .rc file
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 'the/script',\n"
                        << "  'db': 'that/db'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "the/script");
            CPPUNIT_ASSERT(rc.get_db() == "that/db");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'db': 'that/db'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "that/db");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 'the/script'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "the/script");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 123\n"
                        << "}\n";
            }

            test_callback::expected_t expected2;
            expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
            expected2.f_pos.set_filename(as2js_rc.to_utf8().c_str());
            expected2.f_pos.set_function("unknown-func");
            expected2.f_pos.new_line();
            expected2.f_pos.new_line();
            expected2.f_message = "A resource file is expected to be an object of string elements.";
            tc.f_expected.push_back(expected2);

            CPPUNIT_ASSERT_THROW(rc.init_rc(true), as2js::exception_exit);
            tc.got_called();
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        {
            // create a null .rc file
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "null\n";
            }

            rc.init_rc(false);
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        {
            // create an .rc file, without an object nor null
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "['scripts', 123]\n";
            }

            test_callback::expected_t expected2;
            expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
            expected2.f_pos.set_filename(as2js_rc.to_utf8().c_str());
            expected2.f_pos.set_function("unknown-func");
            expected2.f_pos.new_line();
            expected2.f_message = "A resource file (.rc) must be defined as a JSON object, or set to 'null'.";
            tc.f_expected.push_back(expected2);

            CPPUNIT_ASSERT_THROW(rc.init_rc(true), as2js::exception_exit);
            tc.got_called();
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }
    }

    // delete our temporary .rc file (should already have been deleted)
    unlink(as2js_rc.to_utf8().c_str());

    // if possible get rid of the directories (don't check for errors)
    rmdir(as2js_conf.to_utf8().c_str());
    if(del_config)
    {
        rmdir(config.to_utf8().c_str());
    }
}


//
// WARNING: this test requires root permissions, it can generally be
//          ignored though because it uses the same process as the
//          user local file in "as2js/as2js.rc"; it is here for
//          completeness in case you absolutely want to prove that
//          works as expected
//
void As2JsRCUnitTests::test_load_from_system_config()
{
    if(getuid() != 0)
    {
        std::cout << " --- test_load_from_system_config() requires root access to modify the /etc/as2js directory --- ";
        return;
    }

    // this test is not going to work if the get_home() function was
    // already called with an empty HOME variable...
    if(g_empty_home_too_late == 2)
    {
        std::cout << " --- test_empty_home() not run, the other rc unit tests are not compatible with this test --- ";
        return;
    }

    g_empty_home_too_late = 1;

    // create the folders and make sure we clean up any existing .rc file
    // (although it was checked in the setUp() function and thus we should
    // not reach here if the .rc already existed!)
    as2js::String as2js_conf("/etc/as2js");
    CPPUNIT_ASSERT(mkdir(as2js_conf.to_utf8().c_str(), 0700) == 0); // usually this is 0755, but for security we cannot take that risk...
    as2js::String as2js_rc(as2js_conf);
    as2js_rc += "/as2js.rc";
    unlink(as2js_rc.to_utf8().c_str()); // delete that, just in case (the setup verifies that it does not exist)

    {
        test_callback tc;

        test_callback::expected_t expected1;
        expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
        expected1.f_error_code = as2js::err_code_t::AS_ERR_INSTALLATION;
        expected1.f_pos.set_filename("unknown-file");
        expected1.f_pos.set_function("unknown-func");
        expected1.f_message = "cannot find the as2js.rc file; the system default is usually put in /etc/as2js/as2js.rc";
        tc.f_expected.push_back(expected1);

        as2js::rc_t rc;
        CPPUNIT_ASSERT_THROW(rc.init_rc(false), as2js::exception_exit);
        tc.got_called();

        {
            // create an .rc file
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 'the/script',\n"
                        << "  'db': 'that/db'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "the/script");
            CPPUNIT_ASSERT(rc.get_db() == "that/db");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'db': 'that/db'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "that/db");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 'the/script'\n"
                        << "}\n";
            }

            rc.init_rc(true);
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "the/script");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        {
            // create an .rc file, without scripts
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "{\n"
                        << "  'scripts': 123\n"
                        << "}\n";
            }

            test_callback::expected_t expected2;
            expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
            expected2.f_pos.set_filename(as2js_rc.to_utf8().c_str());
            expected2.f_pos.set_function("unknown-func");
            expected2.f_pos.new_line();
            expected2.f_pos.new_line();
            expected2.f_message = "A resource file is expected to be an object of string elements.";
            tc.f_expected.push_back(expected2);

            CPPUNIT_ASSERT_THROW(rc.init_rc(true), as2js::exception_exit);
            tc.got_called();
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        {
            // create a null .rc file
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "null\n";
            }

            rc.init_rc(false);
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }

        {
            // create an .rc file, without an object nor null
            {
                std::ofstream rc_file;
                rc_file.open(as2js_rc.to_utf8().c_str());
                CPPUNIT_ASSERT(rc_file.is_open());
                rc_file << "// rc file\n"
                        << "['scripts', 123]\n";
            }

            test_callback::expected_t expected2;
            expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
            expected2.f_pos.set_filename(as2js_rc.to_utf8().c_str());
            expected2.f_pos.set_function("unknown-func");
            expected2.f_pos.new_line();
            expected2.f_message = "A resource file (.rc) must be defined as a JSON object, or set to 'null'.";
            tc.f_expected.push_back(expected2);

            CPPUNIT_ASSERT_THROW(rc.init_rc(true), as2js::exception_exit);
            tc.got_called();
            unlink(as2js_rc.to_utf8().c_str());

            CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
            CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
        }
    }

    // delete our temporary .rc file (should already have been deleted)
    unlink(as2js_rc.to_utf8().c_str());

    // if possible get rid of the directories (don't check for errors)
    rmdir(as2js_conf.to_utf8().c_str());
}


void As2JsRCUnitTests::test_empty_home()
{
    // this test is not going to work if the get_home() function was
    // already called...
    if(g_empty_home_too_late == 1)
    {
        std::cout << " --- test_empty_home() not run, the other rc unit tests are not compatible with this test --- ";
        return;
    }

    g_empty_home_too_late = 2;

    // create an .rc file in the user's config directory
    as2js::String home;
    home.from_utf8(getenv("HOME"));

    as2js::String config(home);
    config += "/.config";
    std::cout << " --- config path \"" << config << "\" --- ";
    bool del_config(true);
    if(mkdir(config.to_utf8().c_str(), 0700) != 0) // usually this is 0755, but for security we cannot take that risk...
    {
        // if this mkdir() fails, it is because it exists
        CPPUNIT_ASSERT(errno == EEXIST);
        del_config = false;
    }

    as2js::String rc_path(config);
    rc_path += "/as2js";
    CPPUNIT_ASSERT(mkdir(rc_path.to_utf8().c_str(), 0700) == 0);

    as2js::String rc_filename(rc_path);
    rc_filename += "/as2js.rc";

    std::ofstream rc_file;
    rc_file.open(rc_filename.to_utf8().c_str());
    CPPUNIT_ASSERT(rc_file.is_open());
    rc_file << "// rc file\n"
            << "{\n"
            << "  'scripts': 'cannot read this one',\n"
            << "  'db': 'because it is not accessible'\n"
            << "}\n";

    // remove the variable from the environment
    char buf[256];
    strcpy(buf, "HOME");
    putenv(buf);

    {
        test_callback tc;

        // although we have an rc file under ~/.config/as2js/as2js.rc the
        // rc class cannot find it because the $HOME variable was just deleted
        as2js::rc_t rc;
        rc.init_rc(true);

        CPPUNIT_ASSERT(rc.get_scripts() == "as2js/scripts");
        CPPUNIT_ASSERT(rc.get_db() == "/tmp/as2js_packages.db");
    }

    unlink(rc_filename.to_utf8().c_str());
    rmdir(rc_path.to_utf8().c_str());
}


// vim: ts=4 sw=4 et
