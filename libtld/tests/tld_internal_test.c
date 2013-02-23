/* TLD library -- Test the TLD library by including the tld.c file.
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

/* We want to directly test the library functions, including all the private
 * functions so we include the source directly.
 */
#include "tld.c"
#include "tld_data.c"
#include <stdlib.h>
#include <string.h>

int err_count = 0;

void test_compare()
{
	struct data
	{
		const char *a;
		const char *b;
		int n;
		int r;
	};
	struct data d[] = {
		{ "uj", "uk", 2, -1 },
		{ "uk", "uk", 2,  0 },
		{ "ul", "uk", 2,  1 },

		{ "uj", "ukmore",  2, -1 },
		{ "uk", "ukstuff", 2,  0 },
		{ "ul", "ukhere",  2,  1 },

		{ "uk1", "ukmore",  2, 1 },
		{ "uk2", "ukstuff", 2, 1 },
		{ "uk3", "ukhere",  2, 1 },

		{ "uk1", "uk.", 3, 1 },
		{ "uk2", "uk.", 3, 1 },
		{ "uk3", "uk.", 3, 1 },

		{ "uk1", ".uk", 3, 1 },
		{ "uk2", ".uk", 3, 1 },
		{ "uk3", ".uk", 3, 1 },

		{ "uk", "uk1",   3, -1 },
		{ "uk", "uk22",  4, -1 },
		{ "uk", "uk333", 5, -1 },

		{ "uk1",   "uk", 2, 1 },
		{ "uk22",  "uk", 2, 1 },
		{ "uk333", "uk", 2, 1 },
	};
	int i, r, max;

	max = sizeof(d) / sizeof(d[0]);
	for(i = 0; i < max; ++i)
	{
		r = cmp(d[i].a, d[i].b, d[i].n);
		if(r != d[i].r) {
			fprintf(stderr, "error: cmp() failed with \"%s\" / \"%s\", expected %d and got %d\n",
					d[i].a, d[i].b, d[i].r, r);
			++err_count;
		}
	}
}

void test_search()
{
	struct search_info
	{
		int				f_start;
		int				f_end;
		const char *	f_tld;
		int				f_length;
		int				f_result;
	};
	struct search_info d[] = {
		/*
		 * This table is very annoying since each time the data changes
		 * it gets out of sync. On the other hand that's the best way
		 * to make sure our tests work like in the real world.
		 */

		/* get the .uk offset */
		{ 6718, 7076, "uk", 2, 7052 },

		/* get each offset of the .uk 2nd level domain */
		{ 6530, 6555, "ac", 2,							6530 },
		{ 6530, 6555, "bl", 2,							6531 },
		{ 6530, 6555, "british-library", 15,			6532 },
		{ 6530, 6555, "co", 2,							6533 },
		{ 6530, 6555, "gov", 3,							6534 },
		{ 6530, 6555, "govt", 4,						6535 },
		{ 6530, 6555, "icnet", 5,						6536 },
		{ 6530, 6555, "jet", 3,							6537 },
		{ 6530, 6555, "lea", 3,							6538 },
		{ 6530, 6555, "ltd", 3,							6539 },
		{ 6530, 6555, "me", 2,							6540 },
		{ 6530, 6555, "mil", 3,							6541 },
		{ 6530, 6555, "mod", 3,							6542 },
		{ 6530, 6555, "national-library-scotland", 25,	6543 },
		{ 6530, 6555, "nel", 3,							6544 },
		{ 6530, 6555, "net", 3,							6545 },
		{ 6530, 6555, "nhs", 3,							6546 },
		{ 6530, 6555, "nic", 3,							6547 },
		{ 6530, 6555, "nls", 3,							6548 },
		{ 6530, 6555, "org", 3,							6549 },
		{ 6530, 6555, "orgn", 4,						6550 },
		{ 6530, 6555, "parliament", 10,					6551 },
		{ 6530, 6555, "plc", 3,							6552 },
		{ 6530, 6555, "police", 6,						6553 },
		{ 6530, 6555, "sch", 3,							6554 },

		/* test with a few invalid TLDs for .uk */
		{ 6530, 6555, "com", 3, -1 },
		{ 6530, 6555, "aca", 3, -1 },
		{ 6530, 6555, "aac", 3, -1 },
		{ 6530, 6555, "ca", 2, -1 },
		{ 6530, 6555, "cn", 2, -1 },
		{ 6530, 6555, "cp", 2, -1 },
		{ 6530, 6555, "cz", 2, -1 },

		/* get the .vu offset */
		{ 6718, 7076, "vu", 2, 7062 },

		/* get the .gov.vu offset */
		{ 6663, 6664, "gov", 3, 6663 },

		/* test with a few .vu 2nd level domains that do not exist */
		{ 6663, 6664, "edu", 3, -1 },
		{ 6663, 6664, "net", 3, -1 },

		/* verify ordering of mari, mari-el, and marine */
		{ 6029, 6164, "mari",    4, 6091 },
		{ 6029, 6164, "mari-el", 7, 6092 },
		{ 6029, 6164, "marine",  6, 6093 },
	};
	int i, r, max;

	max = sizeof(d) / sizeof(d[0]);
	for(i = 0; i < max; ++i)
	{
		r = search(d[i].f_start, d[i].f_end, d[i].f_tld, d[i].f_length);
		if(r != d[i].f_result)
		{
			fprintf(stderr, "error: test_search() failed with \"%s\", expected %d and got %d\n",
					d[i].f_tld, d[i].f_result, r);
			++err_count;
		}
	}
}


void test_search_array(int start, int end)
{
	int		i, r;

	/* now test all from the arrays */
	for(i = start; i < end; ++i)
	{
/*printf("{%d..%d} i = %d, [%s]\n", start, end, i, tld_descriptions[i].f_tld);*/
		r = search(start, end, tld_descriptions[i].f_tld, strlen(tld_descriptions[i].f_tld));
		if(r != i)
		{
			fprintf(stderr, "error: test_search_array() failed with \"%s\", expected %d and got %d\n",
					tld_descriptions[i].f_tld, i, r);
			++err_count;
		}
		if(tld_descriptions[i].f_start_offset != -1)
		{
			test_search_array(tld_descriptions[i].f_start_offset,
							  tld_descriptions[i].f_end_offset);
		}
	}
}

void test_search_all()
{
	test_search_array(tld_start_offset, tld_end_offset);
}


int main(int argc, char *argv[])
{
	fprintf(stderr, "testing tld version %s\n", tld_version());

	/* call all the tests, one by one
	 * failures are "recorded" in the err_count global variable
	 * and the process stops with an error message and exit(1)
	 * if err_count is not zero.
	 */
	test_compare();
	test_search();
	test_search_all();

	if(err_count)
	{
		fprintf(stderr, "%d error%s occured.\n",
					err_count, err_count != 1 ? "s" : "");
	}
	exit(err_count ? 1 : 0);
}

/* vim: ts=4 sw=4
 */
