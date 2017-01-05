//
// File:        src/main.cpp
// Object:      Allow users to easily add and remove IPs in an iptable
//              firewall; this is useful if you have a blacklist of IPs
//
// Copyright:   Copyright (c) 2007-2017 Made to Order Software Corp.
//              All Rights Reserved.
//
// http://snapwebsites.org/
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//


/** \mainpage
 *
 * \image html iplock-logo.jpg
 *
 * The iplock tool can be used to very easily add and remove IP addresses
 * you want blocking unwanted clients.
 *
 * Once installed properly, it will be capable to become root and
 * thus access the firewall as required. The rules used to add and
 * remove IPs are defined in the configuration file found under
 * /etc/network/iplock.conf (to avoid any security problems, the path
 * to the configuration file cannot be changed.)
 *
 * By default, the iplock tool expects a chain entry named bad_robots.
 * This can be changed in the configuration file.
 */

#include "iplock.h"

#include <iostream>

#include "poison.h"


int main(int argc, char * argv[])
{
    try
    {
        iplock l(argc, argv);

        l.run_command();

        exit(0);
    }
    catch(std::exception const & e)
    {
        std::cerr << "error:iplock: an exception occurred: " << e.what() << std::endl;
    }

    exit(1);
}


// vim: ts=4 sw=4 et
