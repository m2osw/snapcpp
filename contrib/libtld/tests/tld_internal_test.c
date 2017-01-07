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
			fprintf(stderr, "error: cmp() failed with \"%s\" / \"%s\", expected %d and got %d [1]\n",
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
			fprintf(stderr, "error: cmp() failed with \"%s\" / \"%s\", expected %d and got %d (with domain to lowercase) [2]\n",
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
		{ 7566, 9165, "uk", 2, 9047 },

		/* get each offset of the .uk 2nd level domain */
		{ 7357, 7382, "ac", 2,							7357 },
		{ 7357, 7382, "bl", 2,							7358 },
		{ 7357, 7382, "british-library", 15,			7359 },
		{ 7357, 7382, "co", 2,							7360 },
		{ 7357, 7382, "gov", 3,							7361 },
		{ 7357, 7382, "govt", 4,						7362 },
		{ 7357, 7382, "icnet", 5,						7363 },
		{ 7357, 7382, "jet", 3,							7364 },
		{ 7357, 7382, "lea", 3,							7365 },
		{ 7357, 7382, "ltd", 3,							7366 },
		{ 7357, 7382, "me", 2,							7367 },
		{ 7357, 7382, "mil", 3,							7368 },
		{ 7357, 7382, "mod", 3,							7369 },
		{ 7357, 7382, "national-library-scotland", 25,	7370 },
		{ 7357, 7382, "nel", 3,							7371 },
		{ 7357, 7382, "net", 3,							7372 },
		{ 7357, 7382, "nhs", 3,							7373 },
		{ 7357, 7382, "nic", 3,							7374 },
		{ 7357, 7382, "nls", 3,							7375 },
		{ 7357, 7382, "org", 3,							7376 },
		{ 7357, 7382, "orgn", 4,						7377 },
		{ 7357, 7382, "parliament", 10,					7378 },
		{ 7357, 7382, "plc", 3,							7379 },
		{ 7357, 7382, "police", 6,						7380 },
		{ 7357, 7382, "sch", 3,							7381 },

		/* test with a few invalid TLDs for .uk */
		{ 7357, 7382, "com", 3, -1 },
		{ 7357, 7382, "aca", 3, -1 },
		{ 7357, 7382, "aac", 3, -1 },
		{ 7357, 7382, "ca", 2, -1 },
		{ 7357, 7382, "cn", 2, -1 },
		{ 7357, 7382, "cp", 2, -1 },
		{ 7357, 7382, "cz", 2, -1 },

		/* get the .vu offset */
		{ 7566, 9165, "vu", 2, 9094 },

		/* get the 2nd level .vu offsets */
		{ 7501, 7506, "com", 3, 7501 },
		{ 7501, 7506, "edu", 3, 7502 },
		{ 7501, 7506, "gov", 3, 7503 },
		{ 7501, 7506, "net", 3, 7504 },
		{ 7501, 7506, "org", 3, 7505 },

		/* test with a few .vu 2nd level domains that do not exist */
		{ 7501, 7506, "nom", 3, -1 },
		{ 7501, 7506, "sch", 3, -1 },

		/* verify ordering of mari, mari-el, and marine (from .ru) */
		{ 6806, 6943, "mari",    4, 6869 },
		{ 6806, 6943, "mari-el", 7, 6870 },
		{ 6806, 6943, "marine",  6, 6871 },
	};

	size_t i;

	size_t const max = sizeof(d) / sizeof(d[0]);
	for(i = 0; i < max; ++i)
	{
		int const r = search(d[i].f_start, d[i].f_end, d[i].f_tld, d[i].f_length);
		if(r != d[i].f_result)
		{
			fprintf(stderr, "error: test_search() failed with \"%s\", expected %d and got %d [3]\n",
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
			fprintf(stderr, "error: test_search_array() failed with \"%s\", expected %d and got %d [4]\n",
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
