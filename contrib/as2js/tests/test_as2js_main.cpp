/* test_as2js_main.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_main.h"
#include    "license.h"

#include    "as2js/as2js.h"

#include    <advgetopt/advgetopt.h>

#include    <cppunit/BriefTestProgressListener.h>
#include    <cppunit/CompilerOutputter.h>
#include    <cppunit/extensions/TestFactoryRegistry.h>
#include    <cppunit/TestRunner.h>
#include    <cppunit/TestResult.h>
#include    <cppunit/TestResultCollector.h>

//#include "time.h"
#include    <unistd.h>

#ifdef HAVE_QT4
#include    <qxcppunit/testrunner.h>
#include    <QApplication>
#endif


namespace as2js_test
{

std::string     g_tmp_dir;
std::string     g_as2js_compiler;
bool            g_gui = false;
bool            g_run_stdout_destructive = false;
bool            g_save_parser_tests = false;

}


// Recursive dumps the given Test heirarchy to cout
namespace
{
void dump(CPPUNIT_NS::Test *test, std::string indent)
{
    if(test)
    {
        std::cout << indent << test->getName() << std::endl;

        // recursive for children
        indent += "  ";
        int max(test->getChildTestCount());
        for(int i = 0; i < max; ++i)
        {
            dump(test->getChildTestAt(i), indent);
        }
    }
}

template<class R>
void add_tests(const advgetopt::getopt& opt, R& runner)
{
    CPPUNIT_NS::Test *root(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
    int max(opt.size("filename"));
    if(max == 0 || opt.is_defined("all"))
    {
        if(max != 0)
        {
            fprintf(stderr, "unittest: named tests on the command line will be ignored since --all was used.\n");
        }
        CPPUNIT_NS::Test *all_tests(root->findTest("All Tests"));
        if(all_tests == nullptr)
        {
            // this should not happen because cppunit throws if they do not find
            // the test you specify to the findTest() function
            std::cerr << "error: no tests were found." << std::endl;
            exit(1);
        }
        runner.addTest(all_tests);
    }
    else
    {
        for(int i = 0; i < max; ++i)
        {
            std::string test_name(opt.get_string("filename", i));
            CPPUNIT_NS::Test *test(root->findTest(test_name));
            if(test == nullptr)
            {
                // this should not happen because cppunit throws if they do not find
                // the test you specify to the findTest() function
                std::cerr << "error: test \"" << test_name << "\" was not found." << std::endl;
                exit(1);
            }
            runner.addTest(test);
        }
    }
}
}

int unittest_main(int argc, char *argv[])
{
    static const advgetopt::getopt::option options[] = {
        {
            '\0',
            0,
            nullptr,
            nullptr,
            "Usage: %p [--opt] [test-name]",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            nullptr,
            nullptr,
            "with --opt being one or more of the following:",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            'a',
            0,
            "all",
            nullptr,
            "run all the tests in the console (default)",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "destructive",
            nullptr,
            "also run the stdout destructive test (otherwise skip the test so we do not lose stdout)",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'g',
            0,
            "gui",
            nullptr,
#ifdef HAVE_QT4
            "start the GUI version if available",
#else
            "GUI version not available; this option will fail",
#endif
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'h',
            0,
            "help",
            nullptr,
            "print out this help screen",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "license",
            nullptr,
            "prints out the license of the tests",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "licence",
            nullptr,
            nullptr, // hide this one from the help screen
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'l',
            0,
            "list",
            nullptr,
            "list all the available tests",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'S',
            0,
            "seed",
            nullptr,
            "value to seed the randomizer",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            0,
            "save-parser-tests",
            nullptr,
            "save the JSON used to test the parser",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            't',
            0,
            "tmp",
            nullptr,
            "path to a temporary directory",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            0,
            "as2js",
            nullptr,
            "path to the as2js executable",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            'V',
            0,
            "version",
            nullptr,
            "print out the as2js project version these unit tests pertain to",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "filename",
            nullptr,
            nullptr, // hidden argument in --help screen
            advgetopt::getopt::argument_mode_t::default_multiple_argument
        },
        {
            '\0',
            0,
            nullptr,
            nullptr,
            nullptr,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };

    std::vector<std::string> configuration_files;
    advgetopt::getopt opt(argc, argv, options, configuration_files, "UNITTEST_OPTIONS");

    if(opt.is_defined("help"))
    {
        opt.usage(advgetopt::getopt::status_t::no_error, "Usage: test_as2js [--opt] [test-name]");
        /*NOTREACHED*/
    }

    if(opt.is_defined("version"))
    {
        std::cout << AS2JS_VERSION << std::endl;
        exit(1);
    }

    if(opt.is_defined("license") || opt.is_defined("licence"))
    {
        as2js_tools::license::license();
        exit(1);
    }

    if(opt.is_defined("list"))
    {
        CPPUNIT_NS::Test *all = CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest();
        dump(all, "");
        exit(1);
    }
    as2js_test::g_run_stdout_destructive = opt.is_defined("destructive");

    as2js_test::g_save_parser_tests = opt.is_defined("save-parser-tests");

    // by default we get a different seed each time; that really helps
    // in detecting errors! (I know, I wrote loads of tests before)
    unsigned int seed(static_cast<unsigned int>(time(nullptr)));
    if(opt.is_defined("seed"))
    {
        seed = static_cast<unsigned int>(opt.get_long("seed"));
    }
    srand(seed);
    std::cout << opt.get_program_name() << "[" << getpid() << "]" << ": version " << AS2JS_VERSION << ", seed is " << seed << std::endl;
    std::ofstream seed_file;
    seed_file.open("seed.txt");
    if(seed_file.is_open())
    {
        seed_file << seed << std::endl;
    }

    if(opt.is_defined("tmp"))
    {
        as2js_test::g_tmp_dir = opt.get_string("tmp");
    }
    if(opt.is_defined("as2js"))
    {
        as2js_test::g_as2js_compiler = opt.get_string("as2js");
    }

    if(opt.is_defined("gui"))
    {
#ifdef HAVE_QT4
        as2js_test::g_gui = true;
        QApplication app(argc, argv);
        QxCppUnit::TestRunner runner;
        add_tests(opt, runner);
        runner.run();
#else
        std::cerr << "error: no GUI compiled in this test, you cannot use the --gui option.\n";
        exit(1);
#endif
    }
    else
    {
        // Create the event manager and test controller
        CPPUNIT_NS::TestResult controller;

        // Add a listener that colllects test result
        CPPUNIT_NS::TestResultCollector result;
        controller.addListener(&result);        

        // Add a listener that print dots as test run.
        CPPUNIT_NS::BriefTestProgressListener progress;
        controller.addListener(&progress);      

        CPPUNIT_NS::TestRunner runner;

        add_tests(opt, runner);

        runner.run(controller);

        // Print test in a compiler compatible format.
        CPPUNIT_NS::CompilerOutputter outputter(&result, CPPUNIT_NS::stdCOut());
        outputter.write(); 

        if(result.testFailuresTotal())
        {
            return 1;
        }
    }

    return 0;
}


int main(int argc, char *argv[])
{
    return unittest_main(argc, argv);
}

// vim: ts=4 sw=4 et
