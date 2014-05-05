/* TLD library -- Test the TLD library by including the tld.c file.
 * Copyright (C) 2011-2014  Made to Order Software Corp.
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
		{ 6775, 7380, "uk", 2, 7333 },

		/* get each offset of the .uk 2nd level domain */
		{ 6587, 6612, "ac", 2,							6587 },
		{ 6587, 6612, "bl", 2,							6588 },
		{ 6587, 6612, "british-library", 15,			6589 },
		{ 6587, 6612, "co", 2,							6590 },
		{ 6587, 6612, "gov", 3,							6591 },
		{ 6587, 6612, "govt", 4,						6592 },
		{ 6587, 6612, "icnet", 5,						6593 },
		{ 6587, 6612, "jet", 3,							6594 },
		{ 6587, 6612, "lea", 3,							6595 },
		{ 6587, 6612, "ltd", 3,							6596 },
		{ 6587, 6612, "me", 2,							6597 },
		{ 6587, 6612, "mil", 3,							6598 },
		{ 6587, 6612, "mod", 3,							6599 },
		{ 6587, 6612, "national-library-scotland", 25,	6600 },
		{ 6587, 6612, "nel", 3,							6601 },
		{ 6587, 6612, "net", 3,							6602 },
		{ 6587, 6612, "nhs", 3,							6603 },
		{ 6587, 6612, "nic", 3,							6604 },
		{ 6587, 6612, "nls", 3,							6605 },
		{ 6587, 6612, "org", 3,							6606 },
		{ 6587, 6612, "orgn", 4,						6607 },
		{ 6587, 6612, "parliament", 10,					6608 },
		{ 6587, 6612, "plc", 3,							6609 },
		{ 6587, 6612, "police", 6,						6610 },
		{ 6587, 6612, "sch", 3,							6611 },

		/* test with a few invalid TLDs for .uk */
		{ 6530, 6555, "com", 3, -1 },
		{ 6530, 6555, "aca", 3, -1 },
		{ 6530, 6555, "aac", 3, -1 },
		{ 6530, 6555, "ca", 2, -1 },
		{ 6530, 6555, "cn", 2, -1 },
		{ 6530, 6555, "cp", 2, -1 },
		{ 6530, 6555, "cz", 2, -1 },

		/* get the .vu offset */
		{ 6775, 7380, "vu", 2, 7356 },

		/* get the .gov.vu offset */
		{ 6720, 6721, "gov", 3, 6720 },

		/* test with a few .vu 2nd level domains that do not exist */
		{ 6720, 6721, "edu", 3, -1 },
		{ 6720, 6721, "net", 3, -1 },

		/* verify ordering of mari, mari-el, and marine (from .ru) */
		{ 6086, 6222, "mari",    4, 6148 },
		{ 6086, 6222, "mari-el", 7, 6149 },
		{ 6086, 6222, "marine",  6, 6150 },
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
