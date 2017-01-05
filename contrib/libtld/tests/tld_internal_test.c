/* TLD library -- Test the TLD library by including the tld.c file.
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
 * \brief Test the tld.c, tld_data.c, and tld_domain_to_lowercase.c functions.
 *
 * This file implements various tests that can directly access the internal
 * functions of the tld.c, tld_data.c, and tld_domain_to_lowercase.c
 * files.
 *
 * For that purpose we directly include those files in this test. This
 * is why the test is not actually linked against the library, it
 * includes it within itself.
 */

#include "tld.c"
#include "tld_data.c"
#include "tld_domain_to_lowercase.c"

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
	char *s, *vd, *u;

	max = sizeof(d) / sizeof(d[0]);
	for(i = 0; i < max; ++i)
	{
		r = cmp(d[i].a, d[i].b, d[i].n);
		if(r != d[i].r) {
			fprintf(stderr, "error: cmp() failed with \"%s\" / \"%s\", expected %d and got %d\n",
					d[i].a, d[i].b, d[i].r, r);
			++err_count;
		}

		// create a version with uppercase and try again
		s = strdup(d[i].b);
		for(u = s; *u != '\0'; ++u)
		{
			if(*u >= 'a' && *u <= 'z')
			{
				*u &= 0x5F;
			}
		}
		vd = tld_domain_to_lowercase(s);
		r = cmp(d[i].a, d[i].b, d[i].n);
		if(r != d[i].r) {
			fprintf(stderr, "error: cmp() failed with \"%s\" / \"%s\", expected %d and got %d (with domain to lowercase)\n",
					d[i].a, d[i].b, d[i].r, r);
			++err_count;
		}
		free(vd);
		free(s);
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
		{ 7159, 8536, "uk", 2, 8430 },

		/* get each offset of the .uk 2nd level domain */
		{ 6960, 6985, "ac", 2,							6960 },
		{ 6960, 6985, "bl", 2,							6961 },
		{ 6960, 6985, "british-library", 15,			6962 },
		{ 6960, 6985, "co", 2,							6963 },
		{ 6960, 6985, "gov", 3,							6964 },
		{ 6960, 6985, "govt", 4,						6965 },
		{ 6960, 6985, "icnet", 5,						6966 },
		{ 6960, 6985, "jet", 3,							6967 },
		{ 6960, 6985, "lea", 3,							6968 },
		{ 6960, 6985, "ltd", 3,							6969 },
		{ 6960, 6985, "me", 2,							6970 },
		{ 6960, 6985, "mil", 3,							6971 },
		{ 6960, 6985, "mod", 3,							6972 },
		{ 6960, 6985, "national-library-scotland", 25,	6973 },
		{ 6960, 6985, "nel", 3,							6974 },
		{ 6960, 6985, "net", 3,							6975 },
		{ 6960, 6985, "nhs", 3,							6976 },
		{ 6960, 6985, "nic", 3,							6977 },
		{ 6960, 6985, "nls", 3,							6978 },
		{ 6960, 6985, "org", 3,							6979 },
		{ 6960, 6985, "orgn", 4,						6980 },
		{ 6960, 6985, "parliament", 10,					6981 },
		{ 6960, 6985, "plc", 3,							6982 },
		{ 6960, 6985, "police", 6,						6983 },
		{ 6960, 6985, "sch", 3,							6984 },

		/* test with a few invalid TLDs for .uk */
		{ 6960, 6985, "com", 3, -1 },
		{ 6960, 6985, "aca", 3, -1 },
		{ 6960, 6985, "aac", 3, -1 },
		{ 6960, 6985, "ca", 2, -1 },
		{ 6960, 6985, "cn", 2, -1 },
		{ 6960, 6985, "cp", 2, -1 },
		{ 6960, 6985, "cz", 2, -1 },

		/* get the .vu offset */
		{ 7159, 8536, "vu", 2, 8471 },

		/* get the 2nd level .vu offsets */
		{ 7099, 7104, "edu", 3, 7100 },
		{ 7099, 7104, "gov", 3, 7101 },
		{ 7099, 7104, "net", 3, 7102 },

		/* test with a few .vu 2nd level domains that do not exist */
		{ 7099, 7104, "nom", 3, -1 },
		{ 7099, 7104, "sch", 3, -1 },

		/* verify ordering of mari, mari-el, and marine (from .ru) */
		{ 6419, 6556, "mari",    4, 6482 },
		{ 6419, 6556, "mari-el", 7, 6483 },
		{ 6419, 6556, "marine",  6, 6484 },
	};

	size_t i;

	size_t const max = sizeof(d) / sizeof(d[0]);
	for(i = 0; i < max; ++i)
	{
		int const r = search(d[i].f_start, d[i].f_end, d[i].f_tld, d[i].f_length);
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
