// Snap Websites Server -- test against the versioned_filename class
// Copyright (C) 2014  Made to Order Software Corp.
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

//
// This test verifies that names, versions, and browsers are properly
// extracted and then that the resulting versioned_filename objects
// compare against each others as expected.
//

#include "versioned_filename.h"
#include "qstring_stream.h"
#include <iostream>

struct versions_t
{
    char const *        f_extension;
    char const *        f_left;
    char const *        f_left_canonicalized;
    char const *        f_right;
    char const *        f_right_canonicalized;
    bool                f_left_valid;
    bool                f_right_valid;
    int                 f_compare;
};


versions_t g_versions[] =
{
    {
        ".js",
        "name_1.2.3.js",
        "name_1.2.3.js",
        "name_2.5.7.js",
        "name_2.5.7.js",
        true,
        true,
        snap::versioned_filename::COMPARE_SMALLER
    },
    {
        ".js",
        "addr_2.5.7.js",
        "addr_2.5.7.js",
        "name_1.2.3.js",
        "name_1.2.3.js",
        true,
        true,
        snap::versioned_filename::COMPARE_SMALLER
    },
    {
        "css",
        "name_1.2.0.css",
        "name_1.2.css",
        "name_1.2.3.css",
        "name_1.2.3.css",
        true,
        true,
        snap::versioned_filename::COMPARE_SMALLER
    },
    {
        "css",
        "name_1.2.css",
        "name_1.2.css",
        "name_1.2.3.css",
        "name_1.2.3.css",
        true,
        true,
        snap::versioned_filename::COMPARE_SMALLER
    },
    {
        ".js",
        "poo-34_1.2.3.js",
        "poo-34_1.2.3.js",
        "poo-34_1.2.3_ie.js",
        "poo-34_1.2.3_ie.js",
        true,
        true,
        snap::versioned_filename::COMPARE_SMALLER
    },
    {
        ".js",
        "addr_1.2.3_ie.js",
        "addr_1.2.3_ie.js",
        "name_1.2.3.js",
        "name_1.2.3.js",
        true,
        true,
        snap::versioned_filename::COMPARE_SMALLER
    },
    {
        ".js",
        "name_1.2.3_ie.js",
        "name_1.2.3_ie.js",
        "name_1.2.3_mozilla.js",
        "name_1.2.3_mozilla.js",
        true,
        true,
        snap::versioned_filename::COMPARE_SMALLER
    },
    {
        "js",
        "q/name_01.02.03_mozilla.js",
        "name_1.2.3_mozilla.js",
        "name_1.2.3_mozilla.js",
        "name_1.2.3_mozilla.js",
        true,
        true,
        snap::versioned_filename::COMPARE_EQUAL
    },
    {
        "js",
        "name_1.2.3_moz-lla.js",
        "name_1.2.3_moz-lla.js",
        "just/a/path/name_01.02.03_moz-lla.js",
        "name_1.2.3_moz-lla.js",
        true,
        true,
        snap::versioned_filename::COMPARE_EQUAL
    },
    {
        "lla",
        "name_1.02.3.99999_mozi.lla",
        "name_1.2.3.99999_mozi.lla",
        "name_000001.2.03.99998_mozi.lla",
        "name_1.2.3.99998_mozi.lla",
        true,
        true,
        snap::versioned_filename::COMPARE_LARGER
    },
    {
        "lla",
        "zoob_1.02.3.99998_mozi.lla",
        "zoob_1.2.3.99998_mozi.lla",
        "name_000001.2.03.99999_mozi.lla",
        "name_1.2.3.99999_mozi.lla",
        true,
        true,
        snap::versioned_filename::COMPARE_LARGER
    },
    {
        ".js",
        "removed/name_2.5.7_ie.js",
        "name_2.5.7_ie.js",
        "name_1.2.3_ie.js",
        "name_1.2.3_ie.js",
        true,
        true,
        snap::versioned_filename::COMPARE_LARGER
    },
    {
        "jpg",
        "name_2.5.7a_ie.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "a_2.5.7_ie.jpg",
        "",
        "ignored/name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "path/name_3.5_ie.jpg",
        "name_3.5_ie.jpg",
        "super/long/path/name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        true,
        true,
        snap::versioned_filename::COMPARE_LARGER
    },
    {
        "jpg",
        "_2.5.7_ie.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_2.5.7_l.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_2.5.7_.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_2.5.7_LL.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_2.5.7_-p.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_2.5.7_p-.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq__ll.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_._ll.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_3._ll.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_.3_ll.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "q.q_4.3.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_3..jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_.3.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "6q_3.5.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "-q_3.5.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "q-_3.5.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "q--q_3.5.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    },
    {
        "jpg",
        "qq_3.5:.jpg",
        "",
        "name_1.2.3_ie.jpg",
        "name_1.2.3_ie.jpg",
        false,
        true,
        snap::versioned_filename::COMPARE_INVALID
    }
};

bool g_verbose = false;



int check_version(versions_t *v)
{
    int errcnt(0);

    snap::versioned_filename l(v->f_extension);
    snap::versioned_filename r(v->f_extension);

    // parse left
    if(l.set_filename(v->f_left) != v->f_left_valid)
    {
        ++errcnt;
        std::cerr << "error: unexpected left validity for " << v->f_left << " / " << v->f_right << " with " << l.get_error() << std::endl;
    }
    else
    {
        if(g_verbose)
        {
            std::cout << "filename " << v->f_left << " became: name [" << l.get_name()
                      << "], version [" << l.get_version_string() << "/" << l.get_version().size()
                      << "], browser [" << l.get_browser() << "]" << std::endl;
            if(!v->f_left_valid)
            {
                std::cout << "   error: " << l.get_error() << std::endl;
            }
        }
        bool b(static_cast<bool>(l));
        if(b != v->f_left_valid)
        {
            ++errcnt;
            std::cerr << "error: unexpected left bool operator for " << v->f_left << " / " << v->f_right << std::endl;
        }
        b = !l;
        if(b == v->f_left_valid)
        {
            ++errcnt;
            std::cerr << "error: unexpected left ! operator for " << v->f_left << " / " << v->f_right << std::endl;
        }
    }
    if(l.get_filename(true) != v->f_left_canonicalized)
    {
        ++errcnt;
        std::cerr << "error: right canonicalization " << l.get_filename(true) << " expected " << v->f_left_canonicalized << " for " << v->f_left << " / " << v->f_right << std::endl;
    }
    else
    {
        QString name(v->f_left_canonicalized);
        int p(name.lastIndexOf('.'));
        name = name.left(p);
        if(l.get_filename() != name)
        {
            ++errcnt;
            std::cerr << "error: right canonicalization no extension " << l.get_filename() << " expected " << name << " for " << v->f_left << " / " << v->f_right << std::endl;
        }
    }

    // parse right
    if(r.set_filename(v->f_right) != v->f_right_valid)
    {
        ++errcnt;
        std::cerr << "error: unexpected right validity for " << v->f_left << " / " << v->f_right << " with " << r.get_error() << std::endl;
    }
    else
    {
        if(g_verbose)
        {
            std::cout << "filename " << v->f_right << " became: name [" << r.get_name()
                      << "], version [" << r.get_version_string() << "/" << r.get_version().size()
                      << "], browser [" << r.get_browser() << "]" << std::endl;
            if(!v->f_right_valid)
            {
                std::cout << "   error: " << r.get_error() << std::endl;
            }
        }
        bool b(static_cast<bool>(r));
        if(b != v->f_right_valid)
        {
            ++errcnt;
            std::cerr << "error: unexpected right bool operator for " << v->f_left << " / " << v->f_right << std::endl;
        }
        b = !r;
        if(b == v->f_right_valid)
        {
            ++errcnt;
            std::cerr << "error: unexpected right ! operator for " << v->f_left << " / " << v->f_right << std::endl;
        }
    }
    if(r.get_filename(true) != v->f_right_canonicalized)
    {
        ++errcnt;
        std::cerr << "error: right canonicalization " << r.get_filename(true) << " expected " << v->f_right_canonicalized << " for " << v->f_left << " / " << v->f_right << std::endl;
    }
    else
    {
        QString name(v->f_right_canonicalized);
        int p(name.lastIndexOf('.'));
        name = name.left(p);
        if(r.get_filename() != name)
        {
            ++errcnt;
            std::cerr << "error: right canonicalization no extension " << r.get_filename() << " expected " << name << " for " << v->f_left << " / " << v->f_right << std::endl;
        }
    }

    snap::versioned_filename::compare_t c(l.compare(r));
    if(c != v->f_compare)
    {
        ++errcnt;
        std::cerr << "error: unexpected compare() result: " << static_cast<int>(c) << ", for " << v->f_left << " / " << v->f_right << std::endl;
    }
    else
    {
        if(g_verbose)
        {
            std::cout << "   compare " << static_cast<int>(c) << std::endl;
        }
        switch(c)
        {
        case snap::versioned_filename::COMPARE_INVALID:
            if(l == r)
            {
                ++errcnt;
                std::cerr << "error: unexpected == result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(l != r)
            {
                ++errcnt;
                std::cerr << "error: unexpected != result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(l < r)
            {
                ++errcnt;
                std::cerr << "error: unexpected < result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(l <= r)
            {
                ++errcnt;
                std::cerr << "error: unexpected <= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(l > r)
            {
                ++errcnt;
                std::cerr << "error: unexpected > result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(l >= r)
            {
                ++errcnt;
                std::cerr << "error: unexpected >= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            break;

        case snap::versioned_filename::COMPARE_SMALLER:
            if((l == r))
            {
                ++errcnt;
                std::cerr << "error: unexpected == result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l != r))
            {
                ++errcnt;
                std::cerr << "error: unexpected != result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l < r))
            {
                ++errcnt;
                std::cerr << "error: unexpected < result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l <= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected <= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l > r))
            {
                ++errcnt;
                std::cerr << "error: unexpected > result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l >= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected >= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            break;

        case snap::versioned_filename::COMPARE_EQUAL:
            if(!(l == r))
            {
                ++errcnt;
                std::cerr << "error: unexpected == result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l != r))
            {
                ++errcnt;
                std::cerr << "error: unexpected != result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l < r))
            {
                ++errcnt;
                std::cerr << "error: unexpected < result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l <= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected <= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l > r))
            {
                ++errcnt;
                std::cerr << "error: unexpected > result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l >= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected >= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            break;

        case snap::versioned_filename::COMPARE_LARGER:
            if((l == r))
            {
                ++errcnt;
                std::cerr << "error: unexpected == result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l != r))
            {
                ++errcnt;
                std::cerr << "error: unexpected != result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l < r))
            {
                ++errcnt;
                std::cerr << "error: unexpected < result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if((l <= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected <= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l > r))
            {
                ++errcnt;
                std::cerr << "error: unexpected > result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            if(!(l >= r))
            {
                ++errcnt;
                std::cerr << "error: unexpected >= result for " << v->f_left << " / " << v->f_right << std::endl;
            }
            break;

        }
    }

    return errcnt;
}


int main(int argc, char *argv[])
{
    int errcnt(0);

    try
    {
        snap::versioned_filename l("");
        ++errcnt;
        std::cerr << "error: constructor accepted an empty extension." << std::endl;
    }
    catch(snap::versioned_filename_exception_invalid_extension const& msg)
    {
        // got the exception as expected
    }

    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "--verbose") == 0
        || strcmp(argv[i], "-v") == 0)
        {
            g_verbose = true;
        }
    }

    for(size_t i(0); i < sizeof(g_versions) / sizeof(g_versions[0]); ++i)
    {
        errcnt += check_version(g_versions + i);
    }

    if(errcnt != 0)
    {
        std::cerr << std::endl << "*** " << errcnt << " error" << (errcnt == 1 ? "" : "s") << " detected." << std::endl;
    }

    return errcnt;
}

// vim: ts=4 sw=4 et
