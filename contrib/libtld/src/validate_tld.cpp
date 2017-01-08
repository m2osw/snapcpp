/* TLD library -- TLD validation command line tools
 * Copyright (C) 2011-2017  Made to Order Software Corp.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/** \file
 * \brief Command line tool to validate TLDs.
 *
 * This tool is used to verify URIs and emails on the command line and
 * in scripts.
 */

#include "libtld/tld.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

/// Number of errors so we know whether to exit with 0 or 1
int err_count = 0;

/// Whether the user asked for verbosity, false by default.
int verbose = 0;

/** \brief List of schemes that we more or less support (some schemes have extensions using the semi-colon that we do not support yet.)
 *
 * This list of schemes comes from http://en.wikipedia.org/wiki/URI_scheme and is
 * likely not 100% valid, but it should cover a pretty large number of schemes
 * expected to work with our system.
 *
 * \note
 * The documentation does not show you all the schemes. Check out the
 * source to see all the schemes currently included.
 */
const char *schemes = "afp,adiumxtra,aw,beshare,bolo,cap,coap,crid,dns,feed,file,"
                      "finger,fish,ftp,ftps,git,gopher,http,https,icap,imap,"
                      "ipp,irc,irc6,ircs,mumble,mupdate,mysql,nfs,nntp,"
                      "opaquelocktoken,pop,psql,psyc,rmi,rsync,rtmp,rtsp,rtspu,"
                      "sftp,shttp,sieve,smb,snmp,soap.beep,soap.beeps,soldat,"
                      "ssh,teamspeak,telnet,tftp,tip,udp,unreal,ut2004,vemmi,"
                      "ventrilo,wais,webcal,wyciwyg,z39.50r,z39.50s";

/// Hold a list of schemes as defined by the end user.
const char *user_schemes = NULL;

/** \brief Check the parameter as a URI.
 *
 * This function verifies that the URI is valid.
 *
 * \param[in] uri  The URI to be checked.
 */
void check_uri(const char *uri)
{
    tld_result result;
    if(strncasecmp(uri, "mailto:", 7) == 0)
    {
        tld_email_list mail;
        result = mail.parse(uri + 7, 0);
    }
    else
    {
        struct tld_info info;
        const char *s(user_schemes == NULL ? schemes : user_schemes);
        result = tld_check_uri(uri, &info, s, 0);
    }
    if(result != TLD_RESULT_SUCCESS)
    {
        fprintf(stderr, "error: URI \"%s\" is not considered valid.\n", uri);
        ++err_count;
    }
}

/** \brief List the default schemes accepted.
 *
 * This function lists all the schemes defined in the \p schemes variable.
 */
void list()
{
    for(const char *s(schemes); *s != '\0'; ++s)
    {
        if(*s == ',')
        {
            printf("\n");
        }
        else
        {
            printf("%c", *s);
        }
    }
    printf("\n");
    exit(1);
}

/** \brief Print out the help of the tld tool.
 *
 * This function prints out the help information about the validate_tld tool.
 * The function does not return.
 */
void usage()
{
    printf("Usage: validate_tld [-<opts>] <uri> | <email>\n");
    printf("Where <uri> or <email> are URIs starting with a valid scheme.\n");
    printf("The <email> scheme is mailto:.\n");
    printf("Where -<opts> are:\n");
    printf("  -h | --help               print out this help screen\n");
    printf("  -l | --list               print the default list of schemes\n");
    printf("  -s | --schemes <list>     set the list of schemes with user's defined schemes\n");
    printf("                            the list is a comma separate set of scheme names\n");
    printf("  -v | --verbose            request some verbosity of the tool's work\n");
    exit(1);
}

/** \brief The validate tools.
 *
 * The parameters can include any number of URIs and emails. The system
 * must be told what's what using a protocol. For emails, use the name
 * "mail".
 *
 * \param[in] argc  Number of command line arguments passed in.
 * \param[in] argv  The arguments passed in.
 *
 * \return The tool returns 0 on success meaning that all the URIs and emails are valid, 1 otherwise.
 */
int main(int argc, char *argv[])
{
    try
    {
        bool uri(false);

        for(int i(1); i < argc; ++i)
        {
            if(argv[i][0] == '-')
            {
                if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
                {
                    usage();
                    /*NOTREACHED*/
                }
                else if(strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--list") == 0)
                {
                    list();
                    /*NOTREACHED*/
                }
                else if(strcmp(argv[i], "--version") == 0)
                {
                    printf("%s\n", LIBTLD_VERSION);
                    if(verbose)
                    {
                        printf("libtld v%s\n", tld_version());
                    }
                    exit(1);
                }
                else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--schemes") == 0)
                {
                    ++i;
                    if(i >= argc)
                    {
                        fprintf(stderr, "error: the --schemes option requires a list of comma separated schemes.\n");
                    }
                    user_schemes = argv[i];
                }
                else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
                {
                    verbose = 1;
                }
            }
            else
            {
                uri = true;
                check_uri(argv[i]);
            }
        }

        if(!uri)
        {
            fprintf(stderr, "error: no URI were specified on the command line.\n");
            ++err_count;
        }

        return err_count > 0 ? 1 : 0;
    }
    catch(std::exception const& e)
    {
        // an exception occured, print out the message and exit with an error
        std::cerr << "exception: " << e.what() << std::endl;
        exit(1);
    }
}

// vim: ts=4 sw=4 et
