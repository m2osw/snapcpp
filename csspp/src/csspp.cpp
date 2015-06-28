// CSS Preprocessor
// Copyright (C) 2015  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Implementation of the CSS Preprocessor command line tool.
 *
 */

#include "csspp/assembler.h"
#include "csspp/compiler.h"
#include "csspp/lexer.h"
#include "csspp/parser.h"

//#include "csspp/error.h"
//#include "csspp/exceptions.h"
//#include "csspp/unicode_range.h"

#include <advgetopt/advgetopt.h>

#include <fstream>
#include <iostream>

#include <unistd.h>

namespace
{

std::vector<std::string> const g_configuration_files; // Empty

advgetopt::getopt::option const g_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "Usage: %p [-<opt>] [file.css ...] [-o out.css]",
        advgetopt::getopt::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "where -<opt> is one or more of:",
        advgetopt::getopt::help_argument
    },
    {
        'd',
        0,
        "debug",
        nullptr,
        "show all messages, including @debug messages",
        advgetopt::getopt::no_argument
    },
    {
        'I',
        0,
        nullptr,
        nullptr,
        "specify a path to various user defined CSS files; \"-\" to clear the list",
        advgetopt::getopt::required_multiple_argument
    },
    {
        'o',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "output",
        nullptr,
        "save the results in the specified file",
        advgetopt::getopt::required_argument
    },
    {
        'p',
        0,
        "precision",
        nullptr,
        "define the number of digits to use after the decimal point, defaults to 3; note that for percent values, the precision is always 2.",
        advgetopt::getopt::no_argument
    },
    {
        'q',
        0,
        "quiet",
        nullptr,
        "suppress @info and @warning messages",
        advgetopt::getopt::no_argument
    },
    {
        's',
        0,
        "style",
        nullptr,
        "output style: compressed, tidy, compact, expanded",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        nullptr,
        "show the version of the snapdb executable",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "Werror",
        nullptr,
        "make warnings count as errors",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "[file.css ...]; use stdin if no filename specified",
        advgetopt::getopt::default_multiple_argument
    },
    {
        '\0',
        0,
        NULL,
        NULL,
        NULL,
        advgetopt::getopt::end_of_options
    }
};

class pp
{
public:
                                        pp(int argc, char * argv[]);

    int                                 compile();

private:
    std::shared_ptr<advgetopt::getopt>  f_opt;
    int                                 f_precision = 3;
};

pp::pp(int argc, char * argv[])
    : f_opt(new advgetopt::getopt(argc, argv, g_options, g_configuration_files, NULL))
{
    if(f_opt->is_defined("version"))
    {
        std::cerr << CSSPP_VERSION << std::endl;
        exit(1);
    }

    if(f_opt->is_defined("quiet"))
    {
        csspp::error::instance().set_hide_all(true);
    }

    if(f_opt->is_defined("debug"))
    {
        csspp::error::instance().set_show_debug(true);
    }

    if(f_opt->is_defined("Werror"))
    {
        csspp::error::instance().set_count_warnings_as_errors(true);
    }

    if(f_opt->is_defined("precision"))
    {
        f_precision = f_opt->get_long("precision");
    }
}

int pp::compile()
{
    csspp::lexer::pointer_t l;
    csspp::position::pointer_t pos;
    std::unique_ptr<std::stringstream> ss;

    csspp::safe_precision_t safe_precision(f_precision);

    if(f_opt->is_defined("--"))
    {
        // one or more filename specified
        int const arg_count(f_opt->size("--"));
        if(arg_count == 1
        && f_opt->get_string("--") == "-")
        {
            // user asked for stdin
            pos.reset(new csspp::position("-"));
            l.reset(new csspp::lexer(std::cin, *pos));
        }
        else
        {
            char * cwd(get_current_dir_name());
            ss.reset(new std::stringstream);
            pos.reset(new csspp::position("csspp.css"));
            for(int idx(0); idx < arg_count; ++idx)
            {
                // full paths so the -I have no effects on those files
                std::string filename(f_opt->get_string("--", idx));
                if(filename.empty())
                {
                    csspp::error::instance() << *pos
                            << "You cannot include a file with an empty name."
                            << csspp::error_mode_t::ERROR_WARNING;
                    return 1;
                }
                if(filename == "-")
                {
                    csspp::error::instance() << *pos
                            << "You cannot currently mix files and stdin. You may use @import \"filename\"; in your stdin data though."
                            << csspp::error_mode_t::ERROR_WARNING;
                    return 1;
                }
                if(filename[0] == '/')
                {
                    // already absolute
                    *ss << "@import \"" << filename << "\";\n";
                }
                else
                {
                    // make absolute so we do not need to have a "." path
                    *ss << "@import \"" << cwd << "/" << filename << "\";\n";
                }
            }
            l.reset(new csspp::lexer(*ss, *pos));
        }
    }
    else
    {
        // default to stdin
        pos.reset(new csspp::position("-"));
        l.reset(new csspp::lexer(std::cin, *pos));
    }

    // run the lexer and parser
    csspp::error_happened_t error_tracker;
    csspp::parser p(l);
    csspp::node::pointer_t root(p.stylesheet());
    if(error_tracker.error_happened())
    {
        exit(1);
    }

    // run the compiler
    csspp::compiler c;
    c.set_root(root);

    // add paths to the compiler (i.e. for the user and system @imports)
    if(f_opt->is_defined("I"))
    {
        int const count(f_opt->size("I"));
        for(int idx(0); idx < count; ++idx)
        {
            std::string const path(f_opt->get_string("I", idx));
            if(path == "-")
            {
                c.clear_paths();
            }
            else
            {
                c.add_path(path);
            }
        }
    }

    if(f_opt->is_defined(""))
    {
        c.set_empty_on_undefined_variable(true);
    }

    c.compile(false);
    if(error_tracker.error_happened())
    {
        return 1;
    }

//std::cerr << "Compiler result is: [" << *c.get_root() << "]\n";
    csspp::output_mode_t output_mode(csspp::output_mode_t::COMPRESSED);
    if(f_opt->is_defined("style"))
    {
        std::string const mode(f_opt->get_string("style"));
        if(mode == "compressed")
        {
            output_mode = csspp::output_mode_t::COMPRESSED;
        }
        else if(mode == "tidy")
        {
            output_mode = csspp::output_mode_t::TIDY;
        }
        else if(mode == "compact")
        {
            output_mode = csspp::output_mode_t::COMPACT;
        }
        else if(mode == "expanded")
        {
            output_mode = csspp::output_mode_t::EXPANDED;
        }
        else
        {
            csspp::error::instance() << root->get_position()
                    << "The output mode \""
                    << mode
                    << "\" is not supported. Try one of: compressed, tidy, compact, expanded instead."
                    << csspp::error_mode_t::ERROR_WARNING;
            return 1;
        }
    }

    std::ostream * out;
    if(f_opt->is_defined("output"))
    {
        out = new std::ofstream(f_opt->get_string("output"));
    }
    else
    {
        out = &std::cout;
    }
    csspp::assembler a(*out);
    a.output(c.get_root(), output_mode);
    if(f_opt->is_defined("output"))
    {
        delete out;
    }
    if(error_tracker.error_happened())
    {
        // this should be rare as the assembler generally does not generate
        // errors (it may throw though.)
        exit(1);
    }

    return 0;
}

} // no name namespace

int main(int argc, char *argv[])
{
    pp preprocessor(argc, argv);
    return preprocessor.compile();
}

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
