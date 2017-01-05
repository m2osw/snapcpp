/*    unittest_advgetopt.cpp
 *    Copyright (C) 2006-2017  Made to Order Software Corporation
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *    Authors
 *    Alexis Wilke   alexis@m2osw.com
 */

#include "unittest_main.h"
#include "advgetopt.h"

#include <string.h>
#include <math.h>
#include <time.h>

#include <sstream>
#include <fstream>


class AdvGetOptUnitTests
{
public:
    AdvGetOptUnitTests();

    void invalid_parameters();
    void valid_config_files();
    void valid_config_files_extra();
};


AdvGetOptUnitTests::AdvGetOptUnitTests()
{
    //wpkg_filename::uri_filename config("~/.config/wpkg/wpkg.conf");
    //if(config.exists())
    //{
    //    fprintf(stderr, "\nerror:unittest_advgetopt: ~/.config/wpkg/wpkg.conf already exists, the advgetopt tests would not work as expected with such. Please delete or rename that file.\n");
    //    throw std::runtime_error("~/.config/wpkg/wpkg.conf already exists");
    //}
    const char *options(getenv("ADVGETOPT_TEST_OPTIONS"));
    if(options != NULL && *options != '\0')
    {
        std::cerr << std::endl << "error:unittest_advgetopt: ADVGETOPT_TEST_OPTIONS already exists, the advgetopt tests would not work as expected with such. Please unset that environment variable." << std::endl;
        throw std::runtime_error("ADVGETOPT_TEST_OPTIONS already exists");
    }
#ifndef ADVGETOPT_THROW_FOR_EXIT
    std::cerr << std::endl << "warning:unittest_advgetopt: the ADVGETOPT_THROW_FOR_EXIT flag is not defined, usage() calls will not be tested." << std::endl;
#endif
}


void AdvGetOptUnitTests::invalid_parameters()
{
    std::cout << std::endl << "Advanced GetOpt Output (expected until the test fails):" << std::endl;
    // default arguments
    const char *cargv[] =
    {
        "tests/unittests/AdvGetOptUnitTests::invalid_parameters",
        "--ignore-parameters",
        NULL
    };
    const int argc = sizeof(cargv) / sizeof(cargv[0]) - 1;
    char **argv = const_cast<char **>(cargv);
    std::vector<std::string> confs;

    // no options available
    const advgetopt::getopt::option options_empty_list[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (empty list)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, options_empty_list, confs, NULL); }, advgetopt::getopt_exception_invalid);
    }

    // option without a name and "wrong" type
    const advgetopt::getopt::option options_no_name_list[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (no name)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            NULL,
            "we can have a default though",
            NULL,
            advgetopt::getopt::argument_mode_t::required_long
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, options_no_name_list, confs, NULL); }, advgetopt::getopt_exception_invalid);
    }

    // long options must be 2+ characters
    const advgetopt::getopt::option options_2chars_minimum[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (2 chars minimum)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            "", // cannot be empty string (use NULL instead)
            NULL,
            "long option must be 2 characters long at least",
            advgetopt::getopt::argument_mode_t::default_multiple_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, options_2chars_minimum, confs, NULL); }, advgetopt::getopt_exception_invalid);
    }

    // long options must be 2+ characters
    const advgetopt::getopt::option options_2chars_minimum2[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (2 chars minimum 2nd)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            "f", // cannot be 1 character
            NULL,
            "long option must be 2 characters long at least",
            advgetopt::getopt::argument_mode_t::default_multiple_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, options_2chars_minimum2, confs, NULL); }, advgetopt::getopt_exception_invalid);
    }

    // same long option defined twice
    const advgetopt::getopt::option options_defined_twice[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (long defined twice)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            "filename",
            NULL,
            "options must be unique",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            0,
            "filename", // copy/paste problem maybe?
            NULL,
            "options must be unique",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, options_defined_twice, confs, NULL); }, advgetopt::getopt_exception_invalid);
    }

    // same short option defined twice
    const advgetopt::getopt::option options_short_defined_twice[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (short defined twice)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            'f',
            0,
            NULL,
            NULL,
            "options must be unique",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            'f',
            0,
            NULL,
            NULL,
            "options must be unique",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, options_short_defined_twice, confs, "ADVGETOPT_TEST_OPTIONS"); }, advgetopt::getopt_exception_invalid);
    }

    // 2 default_multiple_argument's in the same list is invalid
    const advgetopt::getopt::option options_two_default_multiple_arguments[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (two defaults, multiple args)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            "filename",
            NULL,
            "other parameters are viewed as filenames",
            advgetopt::getopt::argument_mode_t::default_multiple_argument
        },
        {
            '\0',
            0,
            "more",
            NULL,
            "yet other parameters are view as \"more\" data--here it breaks, one default max.",
            advgetopt::getopt::argument_mode_t::default_multiple_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, options_two_default_multiple_arguments, confs, NULL); }, advgetopt::getopt_exception_default);
    }

    // 2 default_argument's in the same list is invalid
    const advgetopt::getopt::option options_two_default_arguments[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (two default args)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            "filename",
            NULL,
            "one other parameter is viewed as a filename",
            advgetopt::getopt::argument_mode_t::default_argument
        },
        {
            '\0',
            0,
            "more",
            NULL,
            "yet other parameter viewed as \"more\" data--here it breaks, one default max.",
            advgetopt::getopt::argument_mode_t::default_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, options_two_default_arguments, confs, NULL); }, advgetopt::getopt_exception_default);
    }

    // mix of default arguments in the same list is invalid
    const advgetopt::getopt::option options_mix_of_default[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (mix of defaults)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            "filename",
            NULL,
            "other parameters are viewed as filenames",
            advgetopt::getopt::argument_mode_t::default_multiple_argument
        },
        {
            '\0',
            0,
            "more",
            NULL,
            "yet other parameter viewed as \"more\" data--here it breaks, one default max.",
            advgetopt::getopt::argument_mode_t::default_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, options_mix_of_default, confs, "ADVGETOPT_TEST_OPTIONS"); }, advgetopt::getopt_exception_default);
    }

#ifdef ADVGETOPT_THROW_FOR_EXIT
    // try the - and -- without a default in the arguments
    const advgetopt::getopt::option options_no_defaults[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (no defaults)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            "verbose",
            NULL,
            "just a flag to test.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        // a filename by itself is a problem when there is no default
        char const * sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::invalid_parameters",
            "--verbose",
            "this",
            "filename",
            NULL
        };
        const int sub_argc(sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1);
        char ** sub_argv(const_cast<char **>(sub_cargv));

std::cerr << "testing a 'special' options_no_defaults...\n";

        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(sub_argc, sub_argv, options_no_defaults, confs, NULL); }, advgetopt::getopt_exception_exiting);
std::cerr << "that returned?!\n";
    }
    {
        // a '-' by itself is a problem when there is no default because it
        // is expected to represent a filename (stdin)
        char const * sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::invalid_parameters",
            "--verbose",
            "-",
            NULL
        };
        const int sub_argc(sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1);
        char **sub_argv(const_cast<char **>(sub_cargv));

        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(sub_argc, sub_argv, options_no_defaults, confs, NULL); }, advgetopt::getopt_exception_exiting);
    }
    {
        // the -- by itself would be fine, but since it represents a
        // transition from arguments to only filenames (or whatever the
        // program expects as default options) it generates an error if
        // no default options are accepted
        const char *sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::invalid_parameters",
            "--verbose",
            "--", // already just by itself it causes problems
            NULL
        };
        const int sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
        char **sub_argv = const_cast<char **>(sub_cargv);

        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(sub_argc, sub_argv, options_no_defaults, confs, NULL); }, advgetopt::getopt_exception_exiting);
    }
    {
        const char *sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::invalid_parameters",
            "--verbose",
            "--",
            "66",
            "--filenames",
            "extra",
            "--file",
            "names",
            NULL
        };
        const int sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
        char **sub_argv = const_cast<char **>(sub_cargv);

        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(sub_argc, sub_argv, options_no_defaults, confs, NULL); }, advgetopt::getopt_exception_exiting);
    }
    {
        // check that -v, that does not exist, generates a usage error
        const char *sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::invalid_parameters",
            "-v",
            NULL
        };
        const int sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
        char **sub_argv = const_cast<char **>(sub_cargv);

        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(sub_argc, sub_argv, options_no_defaults, confs, NULL); }, advgetopt::getopt_exception_exiting);
    }
#endif

#ifdef ADVGETOPT_THROW_FOR_EXIT
    // check -- when default does not allowed environment variables
    const advgetopt::getopt::option options_no_defaults_in_envvar[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (no defaults in envvar)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "verbose",
            NULL,
            "just a flag to test.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            "filenames",
            NULL,
            "default multiple filenames",
            advgetopt::getopt::argument_mode_t::default_multiple_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        unittest::obj_setenv env("ADVGETOPT_TEST_OPTIONS=--verbose - no default here");
        const char *sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::invalid_parameters",
            "--verbose",
            "-",
            "here",
            "it",
            "works",
            NULL
        };
        const int sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
        char **sub_argv = const_cast<char **>(sub_cargv);

        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(sub_argc, sub_argv, options_no_defaults_in_envvar, confs, "ADVGETOPT_TEST_OPTIONS"); }, advgetopt::getopt_exception_exiting);
    }
    {
        unittest::obj_setenv env("ADVGETOPT_TEST_OPTIONS=--verbose no default here");
        const char *sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::invalid_parameters",
            "--verbose",
            "-",
            "here",
            "it",
            "works",
            NULL
        };
        const int sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
        char **sub_argv = const_cast<char **>(sub_cargv);

        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(sub_argc, sub_argv, options_no_defaults_in_envvar, confs, "ADVGETOPT_TEST_OPTIONS"); }, advgetopt::getopt_exception_exiting);
    }
    {
        unittest::obj_setenv env("ADVGETOPT_TEST_OPTIONS=--verbose -- foo bar blah");
        const char *sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::invalid_parameters",
            "--verbose",
            "here",
            "it",
            "works",
            "--",
            "66",
            "--filenames",
            "extra",
            "--file",
            "names",
            NULL
        };
        const int sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
        char **sub_argv = const_cast<char **>(sub_cargv);

        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(sub_argc, sub_argv, options_no_defaults_in_envvar, confs, "ADVGETOPT_TEST_OPTIONS"); }, advgetopt::getopt_exception_exiting);
    }
#endif

    // unnknown long options
#ifdef ADVGETOPT_THROW_FOR_EXIT
    const advgetopt::getopt::option valid_options_unknown_command_line_option[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (unknown command line option)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            "--command",
            NULL,
            "there is a command, but the user tries --verbose!",
            advgetopt::getopt::argument_mode_t::default_multiple_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        const char *sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::invalid_parameters",
            "--verbose",
            NULL
        };
        const int sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
        char **sub_argv = const_cast<char **>(sub_cargv);

        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(sub_argc, sub_argv, valid_options_unknown_command_line_option, confs, NULL); }, advgetopt::getopt_exception_exiting);
    }
#endif

    // illegal short or long option in variable
#ifdef ADVGETOPT_THROW_FOR_EXIT
    const advgetopt::getopt::option options_illegal_in_variable[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (illegal in variable)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            'v',
            0,
            "verbose",
            NULL,
            "just a flag to test.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        // long
        unittest::obj_setenv env("ADVGETOPT_TEST_OPTIONS=--verbose");
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, options_illegal_in_variable, confs, "ADVGETOPT_TEST_OPTIONS"); }, advgetopt::getopt_exception_exiting);
    }
    {
        // short
        unittest::obj_setenv env("ADVGETOPT_TEST_OPTIONS=-v");
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, options_illegal_in_variable, confs, "ADVGETOPT_TEST_OPTIONS"); }, advgetopt::getopt_exception_exiting);
    }
#endif

    // configuration file options must have a long name
    const advgetopt::getopt::option configuration_long_name_missing[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (long name missing)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            'c',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
            NULL,
            NULL,
            "a valid option",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    {
        CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, configuration_long_name_missing, confs, "ADVGETOPT_TEST_OPTIONS"); }, advgetopt::getopt_exception_invalid);
    }

    // create invalid configuration files
#ifdef ADVGETOPT_THROW_FOR_EXIT
    const advgetopt::getopt::option valid_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: try this one and we get a throw (valid options!)",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
            "valid",
            NULL,
            "a valid option",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            0,
            "command",
            NULL,
            "a valid command, but not a valid configuration option",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            0,
            "filename",
            NULL,
            "other parameters are viewed as filenames",
            advgetopt::getopt::argument_mode_t::default_multiple_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };
    std::string tmpdir(unittest::tmp_dir);
    tmpdir += "/.config";
    std::stringstream ss;
    ss << "mkdir -p " << tmpdir;
    if(system(ss.str().c_str()) != 0)
    {
        std::cerr << "fatal error: creating sub-temporary directory \"" << tmpdir << "\" failed.\n";
        exit(1);
    }
    std::string const config_filename(tmpdir + "/advgetopt.config");
    {
        // = sign missing
        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                "valid param\n"
                "# no spaces acceptable in param names\n"
            ;
        }
        {
            std::vector<std::string> invalid_confs;
            invalid_confs.push_back(config_filename);
            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, valid_options, invalid_confs, NULL); }, advgetopt::getopt_exception_exiting);
        }
    }
    {
        // same effect with a few extra spaces
        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                " valid param \n"
                "# no spaces acceptable in param names\n"
            ;
        }
        {
            std::vector<std::string> invalid_confs;
            invalid_confs.push_back(config_filename);
            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, valid_options, invalid_confs, NULL); }, advgetopt::getopt_exception_exiting);
        }
    }
    {
        // param name missing
        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                " = valid param\n"
                "# no spaces acceptable in param names\n"
            ;
        }
        {
            std::vector<std::string> invalid_confs;
            invalid_confs.push_back(config_filename);
            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, valid_options, invalid_confs, NULL); }, advgetopt::getopt_exception_exiting);
        }
    }
    {
        // param name starts with a dash or more
        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                "--valid=param\n"
                "# no spaces acceptable in param names\n"
            ;
        }
        {
            std::vector<std::string> invalid_confs;
            invalid_confs.push_back(config_filename);
            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, valid_options, invalid_confs, NULL); }, advgetopt::getopt_exception_exiting);
        }
    }
    {
        // unknown param name
        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                "invalid=param\n"
                "# no spaces acceptable in param names\n"
            ;
        }
        {
            std::vector<std::string> invalid_confs;
            invalid_confs.push_back(config_filename);
            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, valid_options, invalid_confs, NULL); }, advgetopt::getopt_exception_exiting);
        }
    }
    {
        // known command, not valid in configuration file
        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                "command=value\n"
                "# no spaces acceptable in param names\n"
            ;
        }
        {
            std::vector<std::string> invalid_confs;
            invalid_confs.push_back(config_filename);
            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc, argv, valid_options, invalid_confs, NULL); }, advgetopt::getopt_exception_exiting);
        }
    }
#endif

    // one of the options has an invalid mode; explicit option
    {
        const advgetopt::getopt::option options[] =
        {
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                NULL,
                NULL,
                "Usage: one of the options has an invalid mode",
                advgetopt::getopt::argument_mode_t::help_argument
            },
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                "mode",
                NULL,
                "an argument with an invalid mode to see that we get an exception",
                static_cast<advgetopt::getopt::argument_mode_t>(static_cast<int>(advgetopt::getopt::argument_mode_t::end_of_options) + 1)
            },
            {
                '\0',
                0,
                NULL,
                NULL,
                NULL,
                advgetopt::getopt::argument_mode_t::end_of_options
            }
        };
        {
            const char *cargv2[] =
            {
                "tests/unittests/unittest_advgetopt",
                "--mode",
                "test",
                NULL
            };
            const int argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
            char **argv2 = const_cast<char **>(cargv2);

            // here we hit the one in add_options() (plural)
            // the one in add_option() is not reachable because it is called only
            // when a default option is defined and that means the mode is
            // correct
            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc2, argv2, options, confs, NULL); }, advgetopt::getopt_exception_invalid );
        }
        {
            const char *cargv2[] =
            {
                "tests/unittests/unittest_advgetopt",
                NULL
            };
            const int argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
            char **argv2 = const_cast<char **>(cargv2);

            // this one checks that the --mode flag does indeed generate a
            // throw when not used on the command line but then gets shown
            // in the usage() function
            advgetopt::getopt opt(argc2, argv2, options, confs, NULL);
            for(int i(static_cast<int>(advgetopt::getopt::status_t::no_error)); i <= static_cast<int>(advgetopt::getopt::status_t::fatal); ++i)
            {
                CATCH_REQUIRE_THROWS_AS( opt.usage(static_cast<advgetopt::getopt::status_t>(i), "test no error, warnings, errors..."), advgetopt::getopt_exception_invalid);
            }
        }
    }

    // a valid initialization, but not so valid calls afterward
    {
        advgetopt::getopt::option const options[] =
        {
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                NULL,
                NULL,
                "Usage: try this one and we get a throw (valid options, but not the calls after)",
                advgetopt::getopt::argument_mode_t::help_argument
            },
            {
                '\0',
                0,
                "validate",
                NULL,
                "this is used to validate different things.",
                advgetopt::getopt::argument_mode_t::no_argument
            },
            {
                '\0',
                0,
                "long",
                NULL,
                "used to validate that invalid numbers generate an error.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                'o',
                0,
                "out-of-bounds",
                NULL,
                "valid values from 1 to 9.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                '\0',
                0,
                "not-specified-and-no-default",
                NULL,
                "test long without having used the option and no default.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                '\0',
                0,
                "not-specified-with-invalid-default",
                "123abc",
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                '\0',
                0,
                "not-specified-string-without-default",
                NULL,
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                '\0',
                0,
                "string",
                NULL,
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                '\0',
                0,
                "filename",
                NULL,
                "other parameters are viewed as filenames",
                advgetopt::getopt::argument_mode_t::default_multiple_argument
            },
            {
                '\0',
                0,
                NULL,
                NULL,
                NULL,
                advgetopt::getopt::argument_mode_t::end_of_options
            }
        };
        char const * cargv2[] =
        {
            "tests/unittests/unittest_advgetopt",
            "--validate",
            "--long",
            "123abc",
            "--out-of-bounds",
            "123",
            "--string",
            "string value",
            NULL
        };
        int const argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
        char ** argv2 = const_cast<char **>(cargv2);

        advgetopt::getopt opt(argc2, argv2, options, confs, "ADVGETOPT_TEST_OPTIONS");

        // cannot get the default without a valid name!
        CATCH_REQUIRE_THROWS_AS( opt.get_default(""), advgetopt::getopt_exception_undefined);

        // cannot get a long named "blah"
        CATCH_REQUIRE_THROWS_AS( opt.get_long("blah"), advgetopt::getopt_exception_undefined);
        // existing "long", but only 1 entry
        CATCH_REQUIRE_THROWS_AS( opt.get_long("long", 100), advgetopt::getopt_exception_undefined);
        long l(-1);
        CATCH_REQUIRE_THROWS_AS( l = opt.get_long("not-specified-and-no-default", 0), advgetopt::getopt_exception_undefined);
        CATCH_REQUIRE(l == -1);
        CATCH_REQUIRE_THROWS_AS( l = opt.get_long("not-specified-with-invalid-default", 0), advgetopt::getopt_exception_invalid);
        CATCH_REQUIRE(l == -1);
#ifdef ADVGETOPT_THROW_FOR_EXIT
        CATCH_REQUIRE_THROWS_AS( l = opt.get_long("long"), advgetopt::getopt_exception_exiting);
        CATCH_REQUIRE(l == -1);
        CATCH_REQUIRE_THROWS_AS( l = opt.get_long("out-of-bounds", 0, 1, 9), advgetopt::getopt_exception_exiting);
        CATCH_REQUIRE(l == -1);
#endif
        std::string s;
        CATCH_REQUIRE_THROWS_AS( s = opt.get_string("not-specified-string-without-default", 0), advgetopt::getopt_exception_undefined);
        CATCH_REQUIRE(s.empty());
        CATCH_REQUIRE_THROWS_AS( s = opt.get_string("string", 100), advgetopt::getopt_exception_undefined);
        CATCH_REQUIRE(s.empty());

        // reuse all those invalid options with the reset() function
        // and expect the same result
        // (the constructor is expected to call reset() the exact same way)
        CATCH_REQUIRE_THROWS_AS( opt.reset(argc, argv, options_empty_list, confs, NULL), advgetopt::getopt_exception_invalid);
        CATCH_REQUIRE_THROWS_AS( opt.reset(argc, argv, options_no_name_list, confs, NULL), advgetopt::getopt_exception_invalid);
        CATCH_REQUIRE_THROWS_AS( opt.reset(argc, argv, options_2chars_minimum, confs, NULL), advgetopt::getopt_exception_invalid);
        CATCH_REQUIRE_THROWS_AS( opt.reset(argc, argv, options_2chars_minimum2, confs, NULL), advgetopt::getopt_exception_invalid);
        CATCH_REQUIRE_THROWS_AS( opt.reset(argc, argv, options_defined_twice, confs, "ADVGETOPT_TEST_OPTIONS"), advgetopt::getopt_exception_invalid);
        CATCH_REQUIRE_THROWS_AS( opt.reset(argc, argv, options_short_defined_twice, confs, NULL), advgetopt::getopt_exception_invalid);
        CATCH_REQUIRE_THROWS_AS( opt.reset(argc, argv, options_two_default_multiple_arguments, confs, NULL), advgetopt::getopt_exception_default);
        CATCH_REQUIRE_THROWS_AS( opt.reset(argc, argv, options_two_default_arguments, confs, "ADVGETOPT_TEST_OPTIONS"), advgetopt::getopt_exception_default);
        CATCH_REQUIRE_THROWS_AS( opt.reset(argc, argv, options_mix_of_default, confs, NULL), advgetopt::getopt_exception_default);
    }

    // valid initialization + usage calls
    {
        const advgetopt::getopt::option options[] =
        {
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                NULL,
                NULL,
                "Usage: try this one and we get a throw (valid options + usage calls)",
                advgetopt::getopt::argument_mode_t::help_argument
            },
            {
                '\0',
                0,
                "validate",
                NULL,
                "this is used to validate different things.",
                advgetopt::getopt::argument_mode_t::no_argument
            },
            {
                '\0',
                0,
                "long",
                NULL,
                "used to validate that invalid numbers generate an error.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                'o',
                0,
                "out-of-bounds",
                NULL,
                "valid values from 1 to 9.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                '\0',
                0,
                "not-specified-and-no-default",
                NULL,
                "test long without having used the option and no default.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                '\0',
                0,
                "not-specified-with-invalid-default",
                "123abc",
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::required_multiple_argument
            },
            {
                '\0',
                0,
                "not-specified-string-without-default",
                NULL,
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                '\0',
                0,
                "string",
                NULL,
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                'u',
                0,
                NULL,
                NULL,
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::optional_argument
            },
            {
                'q',
                0,
                NULL,
                NULL,
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::optional_multiple_argument
            },
            {
                '\0',
                0,
                "filename",
                NULL,
                "other parameters are viewed as filenames",
                advgetopt::getopt::argument_mode_t::default_multiple_argument
            },
            {
                '\0',
                0,
                NULL,
                NULL,
                NULL,
                advgetopt::getopt::argument_mode_t::end_of_options
            }
        };
        char const * cargv2[] =
        {
            "tests/unittests/unittest_advgetopt",
            "--validate",
            "--long",
            "123abc",
            "--out-of-bounds",
            "123",
            "--string",
            "string value",
            NULL
        };
        int const argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
        char ** argv2 = const_cast<char **>(cargv2);

        // this initialization works as expected
        advgetopt::getopt opt(argc2, argv2, options, confs, "ADVGETOPT_TEST_OPTIONS");

        // all of the following have the exiting exception
#ifdef ADVGETOPT_THROW_FOR_EXIT
        for(int i(static_cast<int>(advgetopt::getopt::status_t::no_error)); i <= static_cast<int>(advgetopt::getopt::status_t::fatal); ++i)
        {
            CATCH_REQUIRE_THROWS_AS( opt.usage(static_cast<advgetopt::getopt::status_t>(i), "test no error, warnings, errors..."), advgetopt::getopt_exception_exiting);
        }
#endif
    }

    // valid initialization + usage calls with a few different options
    {
        const advgetopt::getopt::option options[] =
        {
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                NULL,
                NULL,
                "Usage: try this one and we get a throw (valid options + usage calls bis)",
                advgetopt::getopt::argument_mode_t::help_argument
            },
            {
                '\0',
                0,
                "validate",
                NULL,
                "this is used to validate different things.",
                advgetopt::getopt::argument_mode_t::no_argument
            },
            {
                '\0',
                0,
                "long",
                NULL,
                "used to validate that invalid numbers generate an error.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                'o',
                0,
                "out-of-bounds",
                NULL,
                "valid values from 1 to 9.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                '\0',
                0,
                "not-specified-and-no-default",
                NULL,
                "test long without having used the option and no default.",
                advgetopt::getopt::argument_mode_t::required_long
            },
            {
                '\0',
                0,
                "not-specified-with-invalid-default",
                "123abc",
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::required_multiple_long
            },
            {
                '\0',
                0,
                "not-specified-string-without-default",
                NULL,
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                '\0',
                0,
                "string",
                NULL,
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::required_argument
            },
            {
                'u',
                0,
                NULL,
                NULL,
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::optional_argument
            },
            {
                'q',
                0,
                NULL,
                NULL,
                "test long with an invalid default value.",
                advgetopt::getopt::argument_mode_t::optional_multiple_long
            },
            {
                'l',
                0,
                NULL,
                NULL,
                "long with just a letter.",
                advgetopt::getopt::argument_mode_t::required_long
            },
            {
                '\0',
                0,
                "filename",
                NULL,
                "other parameters are viewed as filenames; and we need at least one option with a very long help to check that it wraps perfectly (we'd really need to get the output of the command and check that against what is expected because at this time the test is rather blind in that respect!)",
                advgetopt::getopt::argument_mode_t::default_argument
            },
            {
                '\0',
                0,
                NULL,
                NULL,
                NULL,
                advgetopt::getopt::argument_mode_t::end_of_options
            }
        };
#ifdef ADVGETOPT_THROW_FOR_EXIT
        {
            // make sure that --long (required_long) fails if the
            // long value is not specified
            char const * cargv2[] =
            {
                "tests/unittests/unittest_advgetopt",
                "--validate",
                "--long",
                "--out-of-bounds",
                "123",
                "--string",
                "string value",
                NULL
            };
            int const argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
            char ** argv2 = const_cast<char **>(cargv2);

            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc2, argv2, options, confs, NULL); }, advgetopt::getopt_exception_exiting);
        }
        {
            // again with the lone -l (no long name)
            char const * cargv2[] =
            {
                "tests/unittests/unittest_advgetopt",
                "--validate",
                "-l",
                "--out-of-bounds",
                "123",
                "--string",
                "string value",
                NULL
            };
            int const argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
            char ** argv2 = const_cast<char **>(cargv2);

            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc2, argv2, options, confs, NULL); }, advgetopt::getopt_exception_exiting);
        }
#endif
        {
            char const * cargv2[] =
            {
                "tests/unittests/unittest_advgetopt",
                "--validate",
                "--long",
                "123abc",
                "--out-of-bounds",
                "123",
                "--string",
                "string value",
                NULL
            };
            int const argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
            char ** argv2 = const_cast<char **>(cargv2);

            // this initialization works as expected
            advgetopt::getopt opt(argc2, argv2, options, confs, "ADVGETOPT_TEST_OPTIONS");

            // all of the following have the exiting exception
#ifdef ADVGETOPT_THROW_FOR_EXIT
            for(int i(static_cast<int>(advgetopt::getopt::status_t::no_error)); i <= static_cast<int>(advgetopt::getopt::status_t::fatal); ++i)
            {
                CATCH_REQUIRE_THROWS_AS( opt.usage(static_cast<advgetopt::getopt::status_t>(i), "test no error, warnings, errors..."), advgetopt::getopt_exception_exiting);
            }
#endif
        }
    }

    // strange entry without a name
    {
        advgetopt::getopt::option const options[] =
        {
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                NULL,
                NULL,
                "Usage: try this one and we get a throw (strange empty entry!)",
                advgetopt::getopt::argument_mode_t::help_argument
            },
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                NULL,
                NULL,
                "this entry has help, but no valid name...",
                advgetopt::getopt::argument_mode_t::no_argument
            },
            {
                'v',
                0,
                "verbose",
                NULL,
                "show more stuff when found on the command line.",
                advgetopt::getopt::argument_mode_t::no_argument
            },
            {
                '\0',
                0,
                NULL,
                NULL,
                NULL,
                advgetopt::getopt::argument_mode_t::end_of_options
            }
        };
        {
            char const * cargv2[] =
            {
                "tests/unittests/unittest_advgetopt/AdvGetOptUnitTests::invalid_parameters/test-with-an-empty-entry",
                NULL
            };
            int const argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
            char ** argv2 = const_cast<char **>(cargv2);

            // this initialization works as expected
            advgetopt::getopt opt(argc2, argv2, options, confs, "ADVGETOPT_TEST_OPTIONS");

            // all of the following have the exiting exception
            for(int i(static_cast<int>(advgetopt::getopt::status_t::no_error)); i <= static_cast<int>(advgetopt::getopt::status_t::fatal); ++i)
            {
                CATCH_REQUIRE_THROWS_AS( opt.usage(static_cast<advgetopt::getopt::status_t>(i), "test no error, warnings, errors..."), advgetopt::getopt_exception_invalid);
            }
        }
    }

    // required multiple without arguments
#ifdef ADVGETOPT_THROW_FOR_EXIT
    {
        advgetopt::getopt::option const options[] =
        {
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                NULL,
                NULL,
                "Usage: try this one and we get a throw (required multiple without args)",
                advgetopt::getopt::argument_mode_t::help_argument
            },
            {
                'f',
                0,
                "filenames",
                NULL,
                "test a required multiple without any arguments and fail.",
                advgetopt::getopt::argument_mode_t::required_multiple_argument
            },
            {
                '\0',
                0,
                NULL,
                NULL,
                NULL,
                advgetopt::getopt::argument_mode_t::end_of_options
            }
        };
        {
            // first with -f
            char const * cargv2[] =
            {
                "tests/unittests/unittest_advgetopt/AdvGetOptUnitTests::invalid_parameters/test-with-an-empty-entry",
                "-f",
                NULL
            };
            int const argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
            char ** argv2 = const_cast<char **>(cargv2);

            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc2, argv2, options, confs, NULL); }, advgetopt::getopt_exception_exiting);
        }
        {
            // second with --filenames
            const char *cargv2[] =
            {
                "tests/unittests/unittest_advgetopt/AdvGetOptUnitTests::invalid_parameters/test-with-an-empty-entry",
                "--filenames",
                NULL
            };
            int const argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
            char ** argv2 = const_cast<char **>(cargv2);

            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc2, argv2, options, confs, NULL); }, advgetopt::getopt_exception_exiting);
        }
    }
#endif

    // required multiple without arguments, short name only
#ifdef ADVGETOPT_THROW_FOR_EXIT
    {
        advgetopt::getopt::option const options[] =
        {
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                NULL,
                NULL,
                "Usage: try this one and we get a throw (required multiple without args + short name)",
                advgetopt::getopt::argument_mode_t::help_argument
            },
            {
                'f',
                0,
                NULL,
                NULL,
                "test a required multiple without any arguments and fail.",
                advgetopt::getopt::argument_mode_t::required_multiple_argument
            },
            {
                '\0',
                0,
                NULL,
                NULL,
                NULL,
                advgetopt::getopt::argument_mode_t::end_of_options
            }
        };
        {
            // -f only in this case
            char const * cargv2[] =
            {
                "tests/unittests/unittest_advgetopt/AdvGetOptUnitTests::invalid_parameters/test-with-an-empty-entry",
                "-f",
                NULL
            };
            int const argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
            char ** argv2 = const_cast<char **>(cargv2);

            CATCH_REQUIRE_THROWS_AS( { advgetopt::getopt opt(argc2, argv2, options, confs, NULL); }, advgetopt::getopt_exception_exiting);
        }
    }
#endif
}


void AdvGetOptUnitTests::valid_config_files()
{
    // default arguments
    const char *cargv[] =
    {
        "tests/unittests/AdvGetOptUnitTests::valid_config_files",
        "--valid-parameter",
        NULL
    };
    const int argc = sizeof(cargv) / sizeof(cargv[0]) - 1;
    char **argv = const_cast<char **>(cargv);

    std::vector<std::string> empty_confs;

    std::string tmpdir(unittest::tmp_dir);
    tmpdir += "/.config";
    std::stringstream ss;
    ss << "mkdir -p " << tmpdir;
    if(system(ss.str().c_str()) != 0)
    {
        std::cerr << "fatal error: creating sub-temporary directory \"" << tmpdir << "\" failed.\n";
        exit(1);
    }
    std::string const config_filename(tmpdir + "/advgetopt.config");

    std::vector<std::string> confs;
    confs.push_back(config_filename);

    // some command line options to test against
    const advgetopt::getopt::option valid_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: test valid options",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            "valid-parameter",
            NULL,
            "a valid option",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            'v',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "verbose",
            NULL,
            "a verbose like option, select it or not",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "not-specified",
            NULL,
            "a verbose like option, but never specified anywhere",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "number",
            "111",
            "expect a valid number",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "string",
            "the default string",
            "expect a valid string",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "filenames",
            "a.out",
            "expect multiple strings",
            advgetopt::getopt::argument_mode_t::required_multiple_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };

    // test that a configuration files gets loaded as expected
    {
        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                "number = 5\n"
                "string=     strange\n"
                "verbose\n"
                "filenames\t= foo bar blah\n"
            ;
        }

        advgetopt::getopt opt(argc, argv, valid_options, confs, "ADVGETOPT_TEST_OPTIONS");

        // check that the result is valid

        // an invalid parameter, MUST NOT EXIST
        CATCH_REQUIRE(!opt.is_defined("invalid-parameter"));

        // the valid parameter
        CATCH_REQUIRE(opt.is_defined("valid-parameter"));
        CATCH_REQUIRE(opt.get_default("valid-parameter") == NULL);
        CATCH_REQUIRE(opt.size("valid-parameter") == 1);

        // a valid number
        CATCH_REQUIRE(opt.is_defined("number"));
        CATCH_REQUIRE(opt.get_long("number") == 5);
        CATCH_REQUIRE(strcmp(opt.get_default("number"), "111") == 0);
        CATCH_REQUIRE(opt.size("number") == 1);

        // a valid string
        CATCH_REQUIRE(opt.is_defined("string"));
        CATCH_REQUIRE(opt.get_string("string") == "strange");
        CATCH_REQUIRE(strcmp(opt.get_default("string"), "the default string") == 0);
        CATCH_REQUIRE(opt.size("string") == 1);

        // verbosity
        CATCH_REQUIRE(opt.is_defined("verbose"));
        CATCH_REQUIRE(opt.get_string("verbose") == "");
        CATCH_REQUIRE(opt.get_default("verbose") == NULL);
        CATCH_REQUIRE(opt.size("verbose") == 1);

        // filenames
        CATCH_REQUIRE(opt.is_defined("filenames"));
        CATCH_REQUIRE(opt.get_string("filenames") == "foo"); // same as index = 0
        CATCH_REQUIRE(opt.get_string("filenames", 0) == "foo");
        CATCH_REQUIRE(opt.get_string("filenames", 1) == "bar");
        CATCH_REQUIRE(opt.get_string("filenames", 2) == "blah");
        CATCH_REQUIRE(strcmp(opt.get_default("filenames"), "a.out") == 0);
        CATCH_REQUIRE(opt.size("filenames") == 3);

        // as we're at it, make sure that indices out of bounds generate an exception
        for(int i(-100); i <= 100; ++i)
        {
            if(i != 0 && i != 1 && i != 2)
            {
                CATCH_REQUIRE_THROWS_AS( opt.get_string("filenames", i), advgetopt::getopt_exception_undefined);
            }
        }

        // other parameters
        CATCH_REQUIRE(opt.get_program_name() == "AdvGetOptUnitTests::valid_config_files");
        CATCH_REQUIRE(opt.get_program_fullname() == "tests/unittests/AdvGetOptUnitTests::valid_config_files");
    }

    // make sure that command line options have priority or are cumulative
    {
        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                "number = 5\n"
                "string=     strange\n"
                "verbose\n"
                "filenames\t= foo bar blah\n"
            ;
        }

        char const * sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::valid_config_files",
            "--valid-parameter",
            "--number",
            "66",
            "--filenames",
            "extra",
            "file",
            "names",
            NULL
        };
        int const sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
        char ** sub_argv = const_cast<char **>(sub_cargv);

        advgetopt::getopt opt(sub_argc, sub_argv, valid_options, confs, "ADVGETOPT_TEST_OPTIONS");

        // check that the result is valid

        // an invalid parameter, MUST NOT EXIST
        CATCH_REQUIRE(!opt.is_defined("invalid-parameter"));

        // the valid parameter
        CATCH_REQUIRE(opt.is_defined("valid-parameter"));
        CATCH_REQUIRE(opt.get_default("valid-parameter") == NULL);
        CATCH_REQUIRE(opt.size("valid-parameter") == 1);

        // a valid number
        CATCH_REQUIRE(opt.is_defined("number"));
        CATCH_REQUIRE(opt.get_long("number") == 66);
        CATCH_REQUIRE(strcmp(opt.get_default("number"), "111") == 0);
        CATCH_REQUIRE(opt.size("number") == 1);

        // a valid string
        CATCH_REQUIRE(opt.is_defined("string"));
        CATCH_REQUIRE(opt.get_string("string") == "strange");
        CATCH_REQUIRE(strcmp(opt.get_default("string"), "the default string") == 0);
        CATCH_REQUIRE(opt.size("string") == 1);

        // verbosity
        CATCH_REQUIRE(opt.is_defined("verbose"));
        CATCH_REQUIRE(opt.get_string("verbose") == "");
        CATCH_REQUIRE(opt.get_default("verbose") == NULL);
        CATCH_REQUIRE(opt.size("verbose") == 1);

        // filenames
        CATCH_REQUIRE(opt.is_defined("filenames"));
        CATCH_REQUIRE(opt.get_string("filenames") == "foo"); // same as index = 0
        CATCH_REQUIRE(opt.get_string("filenames", 0) == "foo");
        CATCH_REQUIRE(opt.get_string("filenames", 1) == "bar");
        CATCH_REQUIRE(opt.get_string("filenames", 2) == "blah");
        CATCH_REQUIRE(opt.get_string("filenames", 3) == "extra");
        CATCH_REQUIRE(opt.get_string("filenames", 4) == "file");
        CATCH_REQUIRE(opt.get_string("filenames", 5) == "names");
        CATCH_REQUIRE(strcmp(opt.get_default("filenames"), "a.out") == 0);
        CATCH_REQUIRE(opt.size("filenames") == 6);

        // other parameters
        CATCH_REQUIRE(opt.get_program_name() == "AdvGetOptUnitTests::valid_config_files");
        CATCH_REQUIRE(opt.get_program_fullname() == "tests/unittests/AdvGetOptUnitTests::valid_config_files");
    }

    // repeat with ADVGETOPT_TEST_OPTIONS instead of a configuration file
    {
        // here we have verbose twice which should hit the no_argument case
        // in the add_option() function
        unittest::obj_setenv env("ADVGETOPT_TEST_OPTIONS= --verbose --number\t15\t--filenames foo bar blah --string weird -v");
        advgetopt::getopt opt(argc, argv, valid_options, empty_confs, "ADVGETOPT_TEST_OPTIONS");

        // check that the result is valid

        // an invalid parameter, MUST NOT EXIST
        CATCH_REQUIRE(!opt.is_defined("invalid-parameter"));

        // the valid parameter
        CATCH_REQUIRE(opt.is_defined("valid-parameter"));
        CATCH_REQUIRE(opt.get_default("valid-parameter") == NULL);
        CATCH_REQUIRE(opt.size("valid-parameter") == 1);

        // a valid number
        CATCH_REQUIRE(opt.is_defined("number"));
        CATCH_REQUIRE(opt.get_long("number") == 15);
        CATCH_REQUIRE(strcmp(opt.get_default("number"), "111") == 0);
        CATCH_REQUIRE(opt.size("number") == 1);

        // a valid string
        CATCH_REQUIRE(opt.is_defined("string"));
        CATCH_REQUIRE(opt.get_string("string") == "weird");
        CATCH_REQUIRE(strcmp(opt.get_default("string"), "the default string") == 0);
        CATCH_REQUIRE(opt.size("string") == 1);

        // verbosity
        CATCH_REQUIRE(opt.is_defined("verbose"));
        CATCH_REQUIRE(opt.get_string("verbose") == "");
        CATCH_REQUIRE(opt.get_default("verbose") == NULL);
        CATCH_REQUIRE(opt.size("verbose") == 1);

        // filenames
        CATCH_REQUIRE(opt.is_defined("filenames"));
        CATCH_REQUIRE(opt.get_string("filenames") == "foo"); // same as index = 0
        CATCH_REQUIRE(opt.get_string("filenames", 0) == "foo");
        CATCH_REQUIRE(opt.get_string("filenames", 1) == "bar");
        CATCH_REQUIRE(opt.get_string("filenames", 2) == "blah");
        CATCH_REQUIRE(strcmp(opt.get_default("filenames"), "a.out") == 0);
        CATCH_REQUIRE(opt.size("filenames") == 3);

        // other parameters
        CATCH_REQUIRE(opt.get_program_name() == "AdvGetOptUnitTests::valid_config_files");
        CATCH_REQUIRE(opt.get_program_fullname() == "tests/unittests/AdvGetOptUnitTests::valid_config_files");
    }

    // test that the environment variable has priority over a configuration file
    {
        unittest::obj_setenv env(const_cast<char *>("ADVGETOPT_TEST_OPTIONS=--number 501 --filenames more files"));

        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                "number=99\n"
                "string      =     strange\n"
                "verbose\n"
                "filenames =\tfoo\tbar \t blah\n"
            ;
        }
        advgetopt::getopt opt(argc, argv, valid_options, confs, "ADVGETOPT_TEST_OPTIONS");

        // check that the result is valid

        // an invalid parameter, MUST NOT EXIST
        CATCH_REQUIRE(!opt.is_defined("invalid-parameter"));

        // the valid parameter
        CATCH_REQUIRE(opt.is_defined("valid-parameter"));
        CATCH_REQUIRE(opt.get_default("valid-parameter") == NULL);
        CATCH_REQUIRE(opt.size("valid-parameter") == 1);

        // a valid number
        CATCH_REQUIRE(opt.is_defined("number"));
        CATCH_REQUIRE(opt.get_long("number") == 501);
        CATCH_REQUIRE(strcmp(opt.get_default("number"), "111") == 0);
        CATCH_REQUIRE(opt.size("number") == 1);

        // a valid string
        CATCH_REQUIRE(opt.is_defined("string"));
        CATCH_REQUIRE(opt.get_string("string") == "strange");
        CATCH_REQUIRE(strcmp(opt.get_default("string"), "the default string") == 0);
        CATCH_REQUIRE(opt.size("string") == 1);

        // verbosity
        CATCH_REQUIRE(opt.is_defined("verbose"));
        CATCH_REQUIRE(opt.get_string("verbose") == "");
        CATCH_REQUIRE(opt.get_default("verbose") == NULL);
        CATCH_REQUIRE(opt.size("verbose") == 1);

        // filenames
        CATCH_REQUIRE(opt.is_defined("filenames"));
        CATCH_REQUIRE(opt.get_string("filenames") == "foo"); // same as index = 0
        CATCH_REQUIRE(opt.get_string("filenames", 0) == "foo");
        CATCH_REQUIRE(opt.get_string("filenames", 1) == "bar");
        CATCH_REQUIRE(opt.get_string("filenames", 2) == "blah");
        CATCH_REQUIRE(opt.get_string("filenames", 3) == "more");
        CATCH_REQUIRE(opt.get_string("filenames", 4) == "files");
        CATCH_REQUIRE(strcmp(opt.get_default("filenames"), "a.out") == 0);
        CATCH_REQUIRE(opt.size("filenames") == 5);

        // other parameters
        CATCH_REQUIRE(opt.get_program_name() == "AdvGetOptUnitTests::valid_config_files");
        CATCH_REQUIRE(opt.get_program_fullname() == "tests/unittests/AdvGetOptUnitTests::valid_config_files");
    }

    // test order: conf files, environment var, command line
    {
        unittest::obj_setenv env(const_cast<char *>("ADVGETOPT_TEST_OPTIONS=--number 501 --filenames more files"));
        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                "number=99\n"
                "string      =     strange\n"
                "verbose\n"
                "filenames =\tfoo\tbar \t blah\n"
            ;
        }

        const char *sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::valid_config_files",
            "--valid-parameter",
            "--string",
            "hard work",
            "--filenames",
            "extra",
            "file",
            "names",
            NULL
        };
        const int sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
        char **sub_argv = const_cast<char **>(sub_cargv);

        advgetopt::getopt opt(sub_argc, sub_argv, valid_options, confs, "ADVGETOPT_TEST_OPTIONS");

        // check that the result is valid

        // an invalid parameter, MUST NOT EXIST
        CATCH_REQUIRE(!opt.is_defined("invalid-parameter"));

        // the valid parameter
        CATCH_REQUIRE(opt.is_defined("valid-parameter"));
        CATCH_REQUIRE(opt.get_default("valid-parameter") == NULL);
        CATCH_REQUIRE(opt.size("valid-parameter") == 1);

        // a valid number
        CATCH_REQUIRE(opt.is_defined("number"));
        CATCH_REQUIRE(opt.get_long("number") == 501);
        CATCH_REQUIRE(strcmp(opt.get_default("number"), "111") == 0);
        CATCH_REQUIRE(opt.size("number") == 1);

        // a valid string
        CATCH_REQUIRE(opt.is_defined("string"));
        CATCH_REQUIRE(opt.get_string("string") == "hard work");
        CATCH_REQUIRE(strcmp(opt.get_default("string"), "the default string") == 0);
        CATCH_REQUIRE(opt.size("string") == 1);

        // verbosity
        CATCH_REQUIRE(opt.is_defined("verbose"));
        CATCH_REQUIRE(opt.get_string("verbose") == "");
        CATCH_REQUIRE(opt.get_default("verbose") == NULL);
        CATCH_REQUIRE(opt.size("verbose") == 1);

        // filenames
        CATCH_REQUIRE(opt.is_defined("filenames"));
        CATCH_REQUIRE(opt.get_string("filenames") == "foo"); // same as index = 0
        CATCH_REQUIRE(opt.get_string("filenames", 0) == "foo");
        CATCH_REQUIRE(opt.get_string("filenames", 1) == "bar");
        CATCH_REQUIRE(opt.get_string("filenames", 2) == "blah");
        CATCH_REQUIRE(opt.get_string("filenames", 3) == "more");
        CATCH_REQUIRE(opt.get_string("filenames", 4) == "files");
        CATCH_REQUIRE(opt.get_string("filenames", 5) == "extra");
        CATCH_REQUIRE(opt.get_string("filenames", 6) == "file");
        CATCH_REQUIRE(opt.get_string("filenames", 7) == "names");
        CATCH_REQUIRE(strcmp(opt.get_default("filenames"), "a.out") == 0);
        CATCH_REQUIRE(opt.size("filenames") == 8);

        // other parameters
        CATCH_REQUIRE(opt.get_program_name() == "AdvGetOptUnitTests::valid_config_files");
        CATCH_REQUIRE(opt.get_program_fullname() == "tests/unittests/AdvGetOptUnitTests::valid_config_files");
    }

    // test again, just in case: conf files, environment var, command line
    {
        unittest::obj_setenv env(const_cast<char *>("ADVGETOPT_TEST_OPTIONS=--number 709 --filenames more files --string \"hard work in env\""));
        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                "number=99\n"
                "string      =     strange\n"
                "verbose\n"
                "filenames =\tfoo\tbar \t blah\n"
            ;
        }

        const char *sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::valid_config_files",
            "--valid-parameter",
            "--filenames",
            "extra",
            "file",
            "names",
            NULL
        };
        const int sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
        char **sub_argv = const_cast<char **>(sub_cargv);

        advgetopt::getopt opt(sub_argc, sub_argv, valid_options, confs, "ADVGETOPT_TEST_OPTIONS");

        // check that the result is valid

        // an invalid parameter, MUST NOT EXIST
        CATCH_REQUIRE(!opt.is_defined("invalid-parameter"));

        // the valid parameter
        CATCH_REQUIRE(opt.is_defined("valid-parameter"));
        CATCH_REQUIRE(opt.get_default("valid-parameter") == NULL);
        CATCH_REQUIRE(opt.size("valid-parameter") == 1);

        // a valid number
        CATCH_REQUIRE(opt.is_defined("number"));
        CATCH_REQUIRE(opt.get_long("number") == 709);
        CATCH_REQUIRE(strcmp(opt.get_default("number"), "111") == 0);
        CATCH_REQUIRE(opt.size("number") == 1);

        // a valid string
        CATCH_REQUIRE(opt.is_defined("string"));
        CATCH_REQUIRE(opt.get_string("string") == "hard work in env");
        CATCH_REQUIRE(strcmp(opt.get_default("string"), "the default string") == 0);
        CATCH_REQUIRE(opt.size("string") == 1);

        // verbosity
        CATCH_REQUIRE(opt.is_defined("verbose"));
        CATCH_REQUIRE(opt.get_string("verbose") == "");
        CATCH_REQUIRE(opt.get_default("verbose") == NULL);
        CATCH_REQUIRE(opt.size("verbose") == 1);

        // filenames
        CATCH_REQUIRE(opt.is_defined("filenames"));
        CATCH_REQUIRE(opt.get_string("filenames") == "foo"); // same as index = 0
        CATCH_REQUIRE(opt.get_string("filenames", 0) == "foo");
        CATCH_REQUIRE(opt.get_string("filenames", 1) == "bar");
        CATCH_REQUIRE(opt.get_string("filenames", 2) == "blah");
        CATCH_REQUIRE(opt.get_string("filenames", 3) == "more");
        CATCH_REQUIRE(opt.get_string("filenames", 4) == "files");
        CATCH_REQUIRE(opt.get_string("filenames", 5) == "extra");
        CATCH_REQUIRE(opt.get_string("filenames", 6) == "file");
        CATCH_REQUIRE(opt.get_string("filenames", 7) == "names");
        CATCH_REQUIRE(strcmp(opt.get_default("filenames"), "a.out") == 0);
        CATCH_REQUIRE(opt.size("filenames") == 8);

        // other parameters
        CATCH_REQUIRE(opt.get_program_name() == "AdvGetOptUnitTests::valid_config_files");
        CATCH_REQUIRE(opt.get_program_fullname() == "tests/unittests/AdvGetOptUnitTests::valid_config_files");
    }
}


void AdvGetOptUnitTests::valid_config_files_extra()
{
    std::vector<std::string> empty_confs;

    std::string tmpdir(unittest::tmp_dir);
    tmpdir += "/.config";
    std::stringstream ss;
    ss << "mkdir -p " << tmpdir;
    if(system(ss.str().c_str()) != 0)
    {
        std::cerr << "fatal error: creating sub-temporary directory \"" << tmpdir << "\" failed.\n";
        exit(1);
    }
    std::string const config_filename(tmpdir + "/advgetopt.config");

    std::vector<std::string> confs;
    confs.push_back(config_filename);

    // new set of options to test the special "--" option
    const advgetopt::getopt::option valid_options_with_multiple[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: test valid options",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            0,
            "valid-parameter",
            NULL,
            "a valid option",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            'v',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "verbose",
            NULL,
            "a verbose like option, select it or not",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "not-specified",
            NULL,
            "a verbose like option, but never specified anywhere",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "number",
            "111",
            "expect a valid number",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "string",
            "the default string",
            "expect a valid string",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "filenames",
            "a.out",
            "expect multiple strings, may be used after the -- and - is added to it too",
            advgetopt::getopt::argument_mode_t::default_multiple_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };

    // yet again, just in case: conf files, environment var, command line
    {
        unittest::obj_setenv env(const_cast<char *>("ADVGETOPT_TEST_OPTIONS=- --verbose -- more files --string \"hard work in env\""));
        {
            std::ofstream config_file;
            config_file.open(config_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            CATCH_REQUIRE(config_file.good());
            config_file <<
                "# Auto-generated\n"
                "number      =\t\t\t\t1111\t\t\t\t\n"
                "string      =     strange    \n"
                " filenames =\tfoo\tbar \t blah \n"
            ;
        }

        char const * sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::valid_config_files_extra",
            "--valid-parameter",
            "--",
            "extra",
            "-file",
            "names",
            "-", // copied as is since we're after --
            NULL
        };
        int const sub_argc(sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1);
        char ** sub_argv = const_cast<char **>(sub_cargv);

        advgetopt::getopt opt(sub_argc, sub_argv, valid_options_with_multiple, confs, "ADVGETOPT_TEST_OPTIONS");

        // check that the result is valid

        // an invalid parameter, MUST NOT EXIST
        CATCH_REQUIRE(!opt.is_defined("invalid-parameter"));

        // the valid parameter
        CATCH_REQUIRE(opt.is_defined("valid-parameter"));
        CATCH_REQUIRE(opt.get_default("valid-parameter") == NULL);
        CATCH_REQUIRE(opt.size("valid-parameter") == 1);

        // a valid number
        CATCH_REQUIRE(opt.is_defined("number"));
        CATCH_REQUIRE(opt.get_long("number") == 1111);
        CATCH_REQUIRE(strcmp(opt.get_default("number"), "111") == 0);
        CATCH_REQUIRE(opt.size("number") == 1);

        // a valid string
        CATCH_REQUIRE(opt.is_defined("string"));
        CATCH_REQUIRE(opt.get_string("string") == "strange");
        CATCH_REQUIRE(strcmp(opt.get_default("string"), "the default string") == 0);
        CATCH_REQUIRE(opt.size("string") == 1);

        // verbosity
        CATCH_REQUIRE(opt.is_defined("verbose"));
        CATCH_REQUIRE(opt.get_string("verbose") == "");
        CATCH_REQUIRE(opt.get_default("verbose") == NULL);
        CATCH_REQUIRE(opt.size("verbose") == 1);

        // filenames
        CATCH_REQUIRE(opt.is_defined("filenames"));
        CATCH_REQUIRE(opt.get_string("filenames") == "foo"); // same as index = 0
        CATCH_REQUIRE(opt.get_string("filenames",  0) == "foo");
        CATCH_REQUIRE(opt.get_string("filenames",  1) == "bar");
        CATCH_REQUIRE(opt.get_string("filenames",  2) == "blah");
        CATCH_REQUIRE(opt.get_string("filenames",  3) == "-");
        CATCH_REQUIRE(opt.get_string("filenames",  4) == "more");
        CATCH_REQUIRE(opt.get_string("filenames",  5) == "files");
        CATCH_REQUIRE(opt.get_string("filenames",  6) == "--string");
        CATCH_REQUIRE(opt.get_string("filenames",  7) == "hard work in env");
        CATCH_REQUIRE(opt.get_string("filenames",  8) == "extra");
        CATCH_REQUIRE(opt.get_string("filenames",  9) == "-file");
        CATCH_REQUIRE(opt.get_string("filenames", 10) == "names");
        CATCH_REQUIRE(opt.get_string("filenames", 11) == "-");
        CATCH_REQUIRE(strcmp(opt.get_default("filenames"), "a.out") == 0);
        CATCH_REQUIRE(opt.size("filenames") == 12);

        // other parameters
        CATCH_REQUIRE(opt.get_program_name() == "AdvGetOptUnitTests::valid_config_files_extra");
        CATCH_REQUIRE(opt.get_program_fullname() == "tests/unittests/AdvGetOptUnitTests::valid_config_files_extra");
    }

    // check that multiple flags can be used one after another
    const advgetopt::getopt::option valid_short_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            NULL,
            NULL,
            "Usage: test valid options",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            'a',
            0,
            NULL,
            NULL,
            "letter option",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            'c',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            0,
            NULL,
            "letter option",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'd',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            0,
            NULL,
            "letter option",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'f',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            NULL,
            NULL,
            "another letter",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'r',
            0,
            NULL,
            NULL,
            "another letter",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };

    // test that we can use -cafard as expected
    {
        char const * sub_cargv[] =
        {
            "tests/unittests/AdvGetOptUnitTests::valid_config_files_extra",
            "-cafard",
            "alpha",
            "-",
            "recurse",
            NULL
        };
        int const sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
        char **sub_argv = const_cast<char **>(sub_cargv);

        advgetopt::getopt opt(sub_argc, sub_argv, valid_short_options, empty_confs, "ADVGETOPT_TEST_OPTIONS");

        // check that the result is valid

        // an invalid parameter, MUST NOT EXIST
        CATCH_REQUIRE(!opt.is_defined("invalid-parameter"));

        // 2x 'a' in cafard, but we only keep the last entry
        CATCH_REQUIRE(opt.is_defined("a"));
        CATCH_REQUIRE(opt.get_string("a") == "-");
        CATCH_REQUIRE(opt.get_string("a", 0) == "-");
        CATCH_REQUIRE(opt.get_default("a") == NULL);
        CATCH_REQUIRE(opt.size("a") == 1);

        // c
        CATCH_REQUIRE(opt.is_defined("c"));
        CATCH_REQUIRE(opt.get_string("c") == "");
        CATCH_REQUIRE(opt.get_default("c") == NULL);
        CATCH_REQUIRE(opt.size("c") == 1);

        // d
        CATCH_REQUIRE(opt.is_defined("d"));
        CATCH_REQUIRE(opt.get_string("d") == "");
        CATCH_REQUIRE(opt.get_default("d") == NULL);
        CATCH_REQUIRE(opt.size("d") == 1);

        // f
        CATCH_REQUIRE(opt.is_defined("f"));
        CATCH_REQUIRE(opt.get_string("f") == "");
        CATCH_REQUIRE(opt.get_default("f") == NULL);
        CATCH_REQUIRE(opt.size("f") == 1);

        // r
        CATCH_REQUIRE(opt.is_defined("r"));
        CATCH_REQUIRE(opt.get_string("r") == "recurse");
        CATCH_REQUIRE(opt.get_string("r", 0) == "recurse");
        CATCH_REQUIRE(opt.get_default("r") == NULL);
        CATCH_REQUIRE(opt.size("r") == 1);

        // other parameters
        CATCH_REQUIRE(opt.get_program_name() == "AdvGetOptUnitTests::valid_config_files_extra");
        CATCH_REQUIRE(opt.get_program_fullname() == "tests/unittests/AdvGetOptUnitTests::valid_config_files_extra");
    }

    // check that an optional option gets its default value if no arguments
    // were specified on the command line
    {
        // we need options with a --filenames that is optional
        const advgetopt::getopt::option valid_options_with_optional_filenames[] =
        {
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                NULL,
                NULL,
                "Usage: test valid options",
                advgetopt::getopt::argument_mode_t::help_argument
            },
            {
                '\0',
                0,
                "valid-parameter",
                NULL,
                "a valid option",
                advgetopt::getopt::argument_mode_t::optional_argument
            },
            {
                'v',
                advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
                "verbose",
                NULL,
                "a verbose like option, select it or not",
                advgetopt::getopt::argument_mode_t::no_argument
            },
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
                "filenames",
                "a.out",
                "expect multiple strings",
                advgetopt::getopt::argument_mode_t::optional_multiple_argument
            },
            {
                '\0',
                0,
                NULL,
                NULL,
                NULL,
                advgetopt::getopt::argument_mode_t::end_of_options
            }
        };

        {
            // first try with that option by itself
            const char *sub_cargv[] =
            {
                "tests/unittests/AdvGetOptUnitTests::valid_config_files_extra",
                "--valid-parameter",
                "optional argument",
                "--filenames",
                NULL
            };
            const int sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
            char **sub_argv = const_cast<char **>(sub_cargv);

            advgetopt::getopt opt(sub_argc, sub_argv, valid_options_with_optional_filenames, empty_confs, "ADVGETOPT_TEST_OPTIONS");

            // check that the result is valid

            // an invalid parameter, MUST NOT EXIST
            CATCH_REQUIRE(!opt.is_defined("invalid-parameter"));

            // valid parameter
            CATCH_REQUIRE(opt.is_defined("valid-parameter"));
            CATCH_REQUIRE(opt.get_string("valid-parameter") == "optional argument"); // same as index = 0
            CATCH_REQUIRE(opt.get_string("valid-parameter", 0) == "optional argument");
            CATCH_REQUIRE(opt.get_default("valid-parameter") == NULL);
            CATCH_REQUIRE(opt.size("valid-parameter") == 1);

            // filenames
            CATCH_REQUIRE(opt.is_defined("filenames"));
            CATCH_REQUIRE(opt.get_string("filenames") == "a.out"); // same as index = 0
            CATCH_REQUIRE(opt.get_string("filenames", 0) == "a.out");
            CATCH_REQUIRE(strcmp(opt.get_default("filenames"), "a.out") == 0);
            CATCH_REQUIRE(opt.size("filenames") == 1);

            // other parameters
            CATCH_REQUIRE(opt.get_program_name() == "AdvGetOptUnitTests::valid_config_files_extra");
            CATCH_REQUIRE(opt.get_program_fullname() == "tests/unittests/AdvGetOptUnitTests::valid_config_files_extra");
        }
        {
            // try again with a -v after the --filenames without filenames
            const char *sub_cargv[] =
            {
                "tests/unittests/AdvGetOptUnitTests::valid_config_files_extra",
                "--filenames",
                "-v",
                NULL
            };
            const int sub_argc = sizeof(sub_cargv) / sizeof(sub_cargv[0]) - 1;
            char **sub_argv = const_cast<char **>(sub_cargv);

            advgetopt::getopt opt(sub_argc, sub_argv, valid_options_with_optional_filenames, empty_confs, "ADVGETOPT_TEST_OPTIONS");

            // check that the result is valid

            // an invalid parameter, MUST NOT EXIST
            CATCH_REQUIRE(!opt.is_defined("invalid-parameter"));

            // filenames
            CATCH_REQUIRE(opt.is_defined("filenames"));
            CATCH_REQUIRE(opt.get_string("filenames") == "a.out"); // same as index = 0
            CATCH_REQUIRE(opt.get_string("filenames", 0) == "a.out");
            CATCH_REQUIRE(strcmp(opt.get_default("filenames"), "a.out") == 0);
            CATCH_REQUIRE(opt.size("filenames") == 1);

            // other parameters
            CATCH_REQUIRE(opt.get_program_name() == "AdvGetOptUnitTests::valid_config_files_extra");
            CATCH_REQUIRE(opt.get_program_fullname() == "tests/unittests/AdvGetOptUnitTests::valid_config_files_extra");
        }
    }

    // strange entry without a name
    {
        const advgetopt::getopt::option options[] =
        {
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                NULL,
                NULL,
                "Usage: try this one and we get a throw (strange entry without a name)",
                advgetopt::getopt::argument_mode_t::help_argument
            },
            {
                '\0',
                advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
                NULL, // no name
                "README",
                NULL, // no help
                advgetopt::getopt::argument_mode_t::default_multiple_argument
            },
            {
                'v',
                0,
                "verbose",
                NULL,
                "show more stuff when found on the command line.",
                advgetopt::getopt::argument_mode_t::no_argument
            },
            {
                '\0',
                0,
                NULL,
                NULL,
                NULL,
                advgetopt::getopt::argument_mode_t::end_of_options
            }
        };
        {
            const char *cargv2[] =
            {
                "tests/unittests/unittest_advgetopt/AdvGetOptUnitTests::invalid_parameters/no-name-arg-defaults-to-dash-dash",
                "-v",
                "wpkg.cpp",
                NULL
            };
            const int argc2 = sizeof(cargv2) / sizeof(cargv2[0]) - 1;
            char **argv2 = const_cast<char **>(cargv2);

            // this initialization works as expected
            advgetopt::getopt opt(argc2, argv2, options, empty_confs, "ADVGETOPT_TEST_OPTIONS");

            // check that the result is valid

            // an invalid parameter, MUST NOT EXIST
            CATCH_REQUIRE(!opt.is_defined("invalid-parameter"));

            // verbose
            CATCH_REQUIRE(opt.is_defined("verbose"));
            CATCH_REQUIRE(opt.get_string("verbose") == ""); // same as index = 0
            CATCH_REQUIRE(opt.get_string("verbose", 0) == "");
            CATCH_REQUIRE(opt.get_default("verbose") == NULL);
            CATCH_REQUIRE(opt.size("verbose") == 1);

            // the no name parameter!?
            CATCH_REQUIRE(opt.is_defined("--"));
            CATCH_REQUIRE(opt.get_string("--") == "wpkg.cpp"); // same as index = 0
            CATCH_REQUIRE(opt.get_string("--", 0) == "wpkg.cpp");
            CATCH_REQUIRE(strcmp(opt.get_default("--"), "README") == 0);
            CATCH_REQUIRE(opt.size("--") == 1);

            // other parameters
            CATCH_REQUIRE(opt.get_program_name() == "no-name-arg-defaults-to-dash-dash");
            CATCH_REQUIRE(opt.get_program_fullname() == "tests/unittests/unittest_advgetopt/AdvGetOptUnitTests::invalid_parameters/no-name-arg-defaults-to-dash-dash");
        }
    }
}


CATCH_TEST_CASE( "AdvGetOptUnitTests::invalid_parameters", "AdvGetOptUnitTests" )
{
    AdvGetOptUnitTests advgetopt;
    advgetopt.invalid_parameters();
}


CATCH_TEST_CASE( "AdvGetOptUnitTests::valid_config_files", "AdvGetOptUnitTests" )
{
    AdvGetOptUnitTests advgetopt;
    advgetopt.valid_config_files();
}


CATCH_TEST_CASE( "AdvGetOptUnitTests::valid_config_files_extra", "AdvGetOptUnitTests" )
{
    AdvGetOptUnitTests advgetopt;
    advgetopt.valid_config_files_extra();
}


// vim: ts=4 sw=4 et
