//
// File:        src/iplock.h
// Object:      Allow users to easily add and remove IPs in an iptable
//              firewall; this is useful if you have a blacklist of IPs
//
// Copyright:   Copyright (c) 2007-2016 Made to Order Software Corp.
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
#ifndef IPLOCK_H
#define IPLOCK_H

#include <advgetopt/advgetopt.h>

class iplock
{
public:
    class command
    {
    public:
        typedef std::shared_ptr<command> pointer_t;

                            command(iplock * parent, char const * command_name, advgetopt::getopt::pointer_t opt);
        virtual             ~command();

        virtual void        run() = 0;

    protected:
        void                verify_ip(std::string const & ip);

        iplock *                        f_iplock = nullptr; // just in case, unused at this time...
        advgetopt::getopt::pointer_t    f_opt;
        advgetopt::getopt::pointer_t    f_iplock_opt;
        std::string                     f_chain = "unwanted";
        std::string                     f_interface = "eth0";
        bool const                      f_quiet;  // since it is const, you must specify it in the constructor
        bool const                      f_verbose;  // since it is const, you must specify it in the constructor
    };


    class scheme
        : public command
    {
    public:
        scheme( iplock * parent
              , char const * command_name
              , advgetopt::getopt::pointer_t opt
              );

        std::string get_cmdline( std::string const &name );

    protected:
        std::string                     f_scheme = "http";
        advgetopt::getopt::pointer_t    f_scheme_opt;
        std::vector<uint16_t>           f_ports;
    };

    class block_or_unblock
        : public scheme
    {
    public:
                            block_or_unblock(iplock * parent, char const * command_name, advgetopt::getopt::pointer_t opt);
        virtual             ~block_or_unblock() override;

        void                handle_ips(std::string const & cmdline, int run_on_result);
    };

    class block
        : public block_or_unblock
    {
    public:
                            block(iplock * parent, advgetopt::getopt::pointer_t opt);
        virtual             ~block() override;

        virtual void        run() override;

    private:
    };

    class unblock
        : public block_or_unblock
    {
    public:
                            unblock(iplock * parent, advgetopt::getopt::pointer_t opt);
        virtual             ~unblock() override;

        virtual void        run() override;

    private:
    };

    class count
        : public command
    {
    public:
                            count(iplock * parent, advgetopt::getopt::pointer_t opt);
        virtual             ~count() override;

        virtual void        run() override;

    private:
        bool const                      f_reset;  // since it is const, you must specify it in the constructor
        advgetopt::getopt::pointer_t    f_count_opt;
        std::vector<std::string>        f_targets;
    };

    class flush
        : public scheme
    {
    public:
                            flush(iplock * parent, advgetopt::getopt::pointer_t opt, char const* command_name = "iplock --flush");
        virtual             ~flush() override;

        virtual void        run() override;

    private:
    };

    class batch
        : public flush
    {
    public:
                            batch(iplock * parent, advgetopt::getopt::pointer_t opt);
        virtual             ~batch() override;

        virtual void        run() override;

    private:
        std::string         f_ip_addr_filename;
    };

                            iplock(int argc, char * argv[]);

    void                    run_command();

private:
    void                    set_command(command::pointer_t c);
    void                    make_root();

    command::pointer_t      f_command;
};





#endif
// vim: ts=4 sw=4 et
