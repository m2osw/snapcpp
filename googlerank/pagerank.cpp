//
// File:	pagerank.cpp
// Object:	Query the Google Page Rank from a URL specified on the
//              command line
//
// Copyright:	Copyright (c) 2012 Made to Order Software Corp.
//		All Rights Reserved.
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
#include <stdio.h>
#include <stdlib.h>
#include <googlepagerank.h>
#include <QCoreApplication>

/** \brief Print the usage / help for this tool.
 *
 * This function prints out the usage of the pagerank tool.
 *
 * The function does not return.
 */
void usage()
{
	fprintf(stderr, "Usage: googlerank [-opt] URI\n");
	fprintf(stderr, "  where -opt is one of:\n");
	fprintf(stderr, "    -h or --help     print out this help screen\n");
	fprintf(stderr, "    -r or --rank     print only the rank instead of URI and rank\n");
	fprintf(stderr, "    -t or --test     run against our test server\n");
	fprintf(stderr, "  invalid ranks are shown as negative numbers:\n");
	fprintf(stderr, "    -1  rank undefined\n");
	fprintf(stderr, "    -2  HTTP request not complete\n");
	fprintf(stderr, "    -3  the rank is not valid\n");
	fprintf(stderr, "    -4  HTTP request failed\n");
	exit(1);
}

/** \brief The tool main() function..
 *
 * The main function parse the command line arguments and request page rank
 * associated with each URL.
 *
 * The tool prints out the usage when the -h or --help argument is used.
 *
 * Otherwise, it prints out the rank of each URI passed on the command line.
 * The output is
 *
 * \code
 * URI rank
 * \endcode
 *
 * To remove the URI from the output, use the -r or --rank command line option.
 *
 * The rank is a number from 0 to 10. If the rank is not available, you may
 * get a negative number:
 *
 * \li -1 -- the rank is undefined
 * \li -2 -- the HTTP did not complete yet (this should not happen here)
 * \li -3 -- the rank is not valid
 * \li -4 -- the HTTP request failed
 *
 * \note
 * The Qt system identifies itself as "Mozilla/5.0" and uses HTTP/1.1 as the
 * protocol.
 *
 * \param[in] argc  The number of arguments in argv.
 * \param[in] argv  The argument in the form of a string.
 *
 * \return 0 on success, 1 otherwise.
 *
 * \sa usage()
 */
int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	bool rank_only(false);
	bool test(false);

	for(int i = 1; i < argc; ++i) {
		if(strcmp(argv[i], "-h") == 0
		|| strcmp(argv[i], "--help") == 0) {
			usage();
			/*NOTREACHED*/
		}
		if(strcmp(argv[i], "-r") == 0
		|| strcmp(argv[i], "--rank") == 0) {
			rank_only = true;
		}
		if(strcmp(argv[i], "-t") == 0
		|| strcmp(argv[i], "--test") == 0) {
			test = true;
		}
		else if(argv[i][0] == '-') {
			fprintf(stderr, "googlerank:error:unknown command line flag \"%s\".\n", argv[i]);
			exit(1);
		}
		else {
			googlerank::QGooglePageRank pr;
			googlerank::QGooglePageRank::RequestType req = pr.requestRank(QString(argv[i]), test);
			googlerank::QGooglePageRank::GooglePageRankStatus rank = pr.pageRank(req, true);
			if(rank_only) {
				printf("%d\n", rank);
			}
			else {
				printf("%s %d\n", argv[i], rank);
			}
		}
	}

	exit(0);
}

