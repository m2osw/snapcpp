/* TLD library -- TLD example
 * Copyright (C) 2011-2013  Made to Order Software Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/** \file
 * \brief Simple usage example of the C library.
 *
 * This file is an example of usage of the tld() function defined in the
 * libtld library.
 */

#include "libtld/tld.h"
#include <stdio.h>


/** \brief Simple example of a call to the tld() function.
 *
 * The following is a very simple code sample used to present the libtld
 * library usage.
 *
 * To test different URIs, edit the file and change the uri variable
 * defined below.
 *
 * \include example.c
 *
 * \param[in] argc  Number of command line arguments passed in.
 * \param[in] argv  The arguments passed in.
 *
 * \return The function returns 0 on success, 1 otherwise.
 */
int main(int argc, char *argv[])
{
	char *uri = "www.example.co.uk";
	struct tld_info info;
	enum tld_result r;

	r = tld(uri, &info);
	if(r == TLD_RESULT_SUCCESS) {
		const char *tld = info.f_tld;
		const char *s = uri + info.f_offset - 1;
		while(s > uri) {
			if(*s == '.') {
				++s;
				break;
			}
			--s;
		}
		// here uri points to your sub-domains, the length is "s - uri"
		// if uri == s then there are no sub-domains
		// s points to the domain name, the length is "info.f_tld - s"
		// and info.f_tld points to the TLD
		//
		// When TLD_RESULT_SUCCESS is returned the domain cannot be an
		// empty string; also the TLD cannot be empty, however, there
		// may be no sub-domains.
		printf("Sub-domain(s): \"%.*s\"\n", (int)(s - uri), uri);
		printf("Domain: \"%.*s\"\n", (int)(info.f_tld - s), s);
		printf("TLD: \"%s\"\n", info.f_tld);
		return 0;
	}

	return 1;
}

// vim: ts=4 sw=4
