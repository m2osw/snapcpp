/* TLD library -- Test the TLD library by including the tld.c file.
 * Copyright (C) 2011-2015  Made to Order Software Corp.
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

/* We want to directly test the library functions, including all the private
 * functions so we include the source directly.
 */
#include "tld.c"
#include "tld_data.c"
#include <stdlib.h>
#include <string.h>

int err_count = 0;
int verbose = 0;

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
		{ 6914, 7748, "uk", 2, 7700 },

		/* get each offset of the .uk 2nd level domain */
		{ 6717, 6742, "ac", 2,							6717 },
		{ 6717, 6742, "bl", 2,							6718 },
		{ 6717, 6742, "british-library", 15,			6719 },
		{ 6717, 6742, "co", 2,							6720 },
		{ 6717, 6742, "gov", 3,							6721 },
		{ 6717, 6742, "govt", 4,						6722 },
		{ 6717, 6742, "icnet", 5,						6723 },
		{ 6717, 6742, "jet", 3,							6724 },
		{ 6717, 6742, "lea", 3,							6725 },
		{ 6717, 6742, "ltd", 3,							6726 },
		{ 6717, 6742, "me", 2,							6727 },
		{ 6717, 6742, "mil", 3,							6728 },
		{ 6717, 6742, "mod", 3,							6729 },
		{ 6717, 6742, "national-library-scotland", 25,	6730 },
		{ 6717, 6742, "nel", 3,							6731 },
		{ 6717, 6742, "net", 3,							6732 },
		{ 6717, 6742, "nhs", 3,							6733 },
		{ 6717, 6742, "nic", 3,							6734 },
		{ 6717, 6742, "nls", 3,							6735 },
		{ 6717, 6742, "org", 3,							6736 },
		{ 6717, 6742, "orgn", 4,						6737 },
		{ 6717, 6742, "parliament", 10,					6738 },
		{ 6717, 6742, "plc", 3,							6739 },
		{ 6717, 6742, "police", 6,						6740 },
		{ 6717, 6742, "sch", 3,							6741 },

		/* test with a few invalid TLDs for .uk */
		{ 6717, 6742, "com", 3, -1 },
		{ 6717, 6742, "aca", 3, -1 },
		{ 6717, 6742, "aac", 3, -1 },
		{ 6717, 6742, "ca", 2, -1 },
		{ 6717, 6742, "cn", 2, -1 },
		{ 6717, 6742, "cp", 2, -1 },
		{ 6717, 6742, "cz", 2, -1 },

		/* get the .vu offset */
		{ 6914, 7748, "vu", 2, 7729 },

		/* get the .gov.vu offset */
		{ 6855, 6860, "edu", 3, 6856 },
		{ 6855, 6860, "gov", 3, 6857 },
		{ 6855, 6860, "net", 3, 6858 },

		/* test with a few .vu 2nd level domains that do not exist */
		{ 6855, 6860, "nom", 3, -1 },
		{ 6855, 6860, "sch", 3, -1 },

		/* verify ordering of mari, mari-el, and marine (from .ru) */
		{ 6213, 6349, "mari",    4, 6275 },
		{ 6213, 6349, "mari-el", 7, 6276 },
		{ 6213, 6349, "marine",  6, 6277 },
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
		if(verbose)
		{
			printf("{%d..%d} i = %d, [%s]\n", start, end, i, tld_descriptions[i].f_tld);
		}
		r = search(start, end, tld_descriptions[i].f_tld, strlen(tld_descriptions[i].f_tld));
		if(r != i)
		{
			fprintf(stderr, "error: test_search_array() failed with \"%s\", expected %d and got %d\n",
					tld_descriptions[i].f_tld, i, r);
			++err_count;
		}
		if(tld_descriptions[i].f_start_offset != USHRT_MAX)
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
	fprintf(stderr, "testing internal tld version %s\n", tld_version());

	if(argc > 1)
	{
		if(strcmp(argv[1], "-v") == 0)
		{
			verbose = 1;
		}
	}

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
