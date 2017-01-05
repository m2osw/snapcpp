/* json_to_string.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/json.h"
#include    "as2js/exceptions.h"
#include    "as2js/message.h"
#include    "as2js/as2js.h"
#include    "license.h"

#include    <advgetopt/advgetopt.h>

#include    <unistd.h>


class messages : public as2js::MessageCallback
{
public:
    messages()
    {
        as2js::Message::set_message_callback(this);
    }

    // implementation of the output
    virtual void output(as2js::message_level_t message_level, as2js::err_code_t error_code, as2js::Position const& pos, std::string const& message)
    {
        std::cerr << "error:" << static_cast<int>(message_level)
                       << ":" << static_cast<int>(error_code)
                       << ":" << pos
                       << ":" << message << std::endl;
    }
};



int main(int argc, char **argv)
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
            'o',
            0,
            "output",
            nullptr,
            "the output filename",
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
        opt.usage(advgetopt::getopt::status_t::no_error, "Usage: json_to_string [--opt] [test-name]");
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

    if(!opt.is_defined("filename"))
    {
        std::cerr << "error: no filename specified." << std::endl;
        exit(1);
    }

    if(!opt.is_defined("output"))
    {
        std::cerr << "error: no output specified." << std::endl;
        exit(1);
    }

    std::string output_filename(opt.get_string("output"));
    as2js::FileOutput::pointer_t out(new as2js::FileOutput());
    if(!out->open(output_filename))
    {
        std::cerr << "error: could not open output file \"" << output_filename << "\" for writing." << std::endl;
        exit(1);
    }

    messages msg;
    int err(0);
    int max_filenames(opt.size("filename"));
    for(int idx(0); idx < max_filenames; ++idx)
    {
        // first we use JSON to load the file, if we detect an
        // error return 1 instead of 0
        std::string filename(opt.get_string("filename", idx));
        as2js::JSON::pointer_t load_json(new as2js::JSON);
        as2js::JSON::JSONValue::pointer_t loaded_value(load_json->load(filename));
        if(loaded_value)
        {
            as2js::FileInput::pointer_t in(new as2js::FileInput());
            if(in->open(filename))
            {
                as2js::FileInput::char_t c(0);
                while(c >= 0)
                {
                    // read one line of JSON
                    as2js::String str;
                    as2js::String indent;
                    for(;;)
                    {
                        c = in->getc();
                        if(c < 0 || c == '\n')
                        {
                            break;
                        }
                        if((c == ' ' || c == '\t') && str.empty())
                        {
                            // left trim
                            indent += c;
                            continue;
                        }
                        if(str.empty() && c == '/')
                        {
                            c = in->getc();
                            if(c == '/')
                            {
                                // skip comments
                                str += "/";
                                do
                                {
                                    str += c;
                                    c = in->getc();
                                }
                                while(c > 0 && c != '\n');
                                // keep the comments, but not inside the JSON strings
                                out->write(indent);
                                out->write(str);
                                if(str[str.length() - 1] == '\\')
                                {
                                    // we add a $ when str ends with a '\'
                                    out->write("$");
                                }
                                out->write("\n");
                                indent.clear();
                                str.clear();
                                continue;
                            }
                        }
                        if(c == '"')
                        {
                            // add 1 '\' characters in front of the '"'
                            str += "\\\"";
                        }
                        else if(c == '\\')
                        {
                            // add 2 '\' character for each '\'
                            str += "\\\\";
                        }
                        else
                        {
                            str += c;
                        }
                    }
                    if(!str.empty())
                    {
                        // if string ends with "\" then we need to add a "\n"
                        if(str[str.length() - 1] == '\\')
                        {
                            str += "\\n";
                        }
                        // output this one line as a C++ string
                        out->write(indent);
                        out->write("\"");
                        out->write(str);
                        out->write("\"\n");
                    }
                }
            }
            else
            {
                as2js::Message err_msg(as2js::message_level_t::MESSAGE_LEVEL_FATAL, as2js::err_code_t::AS_ERR_CANNOT_COMPILE, loaded_value->get_position());
                err_msg << "could not re-open this JSON input file \"" << filename << "\".";
                err = 1;
            }
        }
        else
        {
            err = 1;
        }
    }

    if(err == 1)
    {
        // on error make sure to delete because otherwise cmake thinks
        // that the target is all good.
        unlink(opt.get_string("output").c_str());
    }

    return err;
}

// vim: ts=4 sw=4 et
