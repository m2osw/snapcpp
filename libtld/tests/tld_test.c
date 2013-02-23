/* TLD library -- test the TLD interface
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

#include "libtld/tld.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

/* we get access to the table with all the TLDs so we can go through them all
 * the library does not give direct access by default... (although maybe we
 * could give users access to the data)
 */
#include "tld_data.h"
extern const struct tld_description tld_descriptions[];
extern unsigned short tld_start_offset;
extern unsigned short tld_end_offset;

int err_count = 0;

/*
 * This test calls the tld() function with all the TLDs and then
 * with wrong TLDs to make sure that the tld() functions works as
 * expected.
 *
 * extern enum tld_result tld(const char *uri, struct tld_info *info);
 */


/** \brief Build an extension from any offset.
 *
 * Create a domain name extensions from any entry in the TLD
 * descriptions.
 *
 * \param[in] offset  The offset in the tld_descriptions table
 * \param[in] uri  The URI buffer
 */
void cat_ext(int offset, char *uri)
{
	int	k, l;

	strcat(uri, tld_descriptions[offset].f_tld);
	l = offset;
	for(k = offset + 1; k < tld_end_offset; ++k)
	{
		if(l >= tld_descriptions[k].f_start_offset
		&& l < tld_descriptions[k].f_end_offset)
		{
			/* found a parent */
			strcat(uri, ".");
			strcat(uri, tld_descriptions[k].f_tld);
			l = k;
			k = tld_descriptions[k].f_end_offset;
		}
	}
}

/*
 * This test goes through all the domain names and extracts the domain,
 * sub-domains and TLDs. (Or at least verifies that we get the correct
 * information in order to do so.)
 *
 * It builds a URI with zero to many sub-domain names, adds a specific
 * domain name, then append a complete TLD. The result is then checked
 * with the tld() function from the library. The tld() is expected to
 * either return VALID or INVALID but nothing else (since all those
 * TLDs exist in our table.) Then we verify that the returned offset is
 * a perfect match.
 */
void test_all()
{
	const char *sub_domains[] = {
		"",
		"www.",
		"tld.",
		"george.snap.",
		"very.long.sub.domain.ext.en.sion.here."
	};
	struct tld_info	info;
	char			uri[256], extension_uri[256];
	int				i, j, k, l, p, max_subdomains;
	char			*s;
	enum tld_result	r;

	max_subdomains = sizeof(sub_domains) / sizeof(sub_domains[0]);

	for(i = 0; i < tld_end_offset; ++i)
	{
		for(j = 0; j < max_subdomains; ++j)
		{
			strcpy(uri, sub_domains[j]);
			strcat(uri, "domain-name.");
			cat_ext(i, uri);
			/* reset the structure so we can verify it gets initialized */
			memset(&info, 0xFE, sizeof(info));
			r = tld(uri, &info);
			/*
			for(l = 0; l < sizeof(info); ++l)
			{
				fprintf(stderr, "0x%02X ", ((unsigned char*)&info)[l]);
			}
			fprintf(stderr, "\nresult for [%s]: category[%d], status[%d], country[%s],"
								" tld[%s], offset[%d]\n",
					uri,
					(int)info.f_category, (int)info.f_status, info.f_country,
						info.f_tld, (int)info.f_offset);
			*/
			p = i;
			if(tld_descriptions[i].f_status == TLD_STATUS_EXCEPTION)
			{
				if(tld_descriptions[i].f_exception_apply_to == USHRT_MAX)
				{
					fprintf(stderr, "error: domain name for \"%s\" (%d) is said to be an exception but it has no apply-to parameter.\n",
							uri, i, r);
					++err_count;
				}
				else
				{
					p = tld_descriptions[i].f_exception_apply_to;
				}
			}
			if(tld_descriptions[i].f_status == TLD_STATUS_VALID)
			{
				if(r != TLD_RESULT_SUCCESS)
				{
					fprintf(stderr, "error: domain name for \"%s\" (%d) could not be extracted successfully (returned: %d)\n",
							uri, i, r);
					++err_count;
				}
				else
				{
					/* in this case we have to test the top domain name only */
					if(strncmp(uri + info.f_offset - 11, "domain-name", 11) != 0)
					{
						fprintf(stderr, "error: domain name for \"%s\" (%d) could not be extracted successfully (returned: %d)\n",
								uri, i, r);
						++err_count;
					}
					/*
					else
						fprintf(stderr, "valid: \"%s\" -> \"%s\"\n", uri, info.f_tld);
					*/
				}
			}
			else if(tld_descriptions[i].f_status == TLD_STATUS_EXCEPTION)
			{
				if(r != TLD_RESULT_SUCCESS)
				{
					fprintf(stderr, "error: domain name for \"%s\" (%d) could not be extracted successfully (returned: %d)\n",
							uri, i, r);
					++err_count;
				}
				else
				{
					strcpy(extension_uri, ".");
					cat_ext(p, extension_uri);
					if(strcmp(info.f_tld, extension_uri) != 0)
					//if(strncmp(uri + info.f_offset - 11, "domain-name", 11) != 0)
					{
						fprintf(stderr, "error: domain name for \"%s\" (%d) could not be extracted successfully (returned: %d)\n",
								uri, i, r);
						++err_count;
					}
					/*
					else
						fprintf(stderr, "valid: \"%s\" -> \"%s\"\n", uri, info.f_tld);
					*/
				}
			}
			else
			{
				if(r != TLD_RESULT_INVALID)
				{
					fprintf(stderr, "error: domain name for \"%s\" (%d) could not be extracted as expected (returned: %d)\n",
							uri, i, r);
					++err_count;
				}
				else if(p != i)
				{
					strcpy(extension_uri, ".");
					cat_ext(p, extension_uri);
					if(strcmp(info.f_tld, extension_uri) != 0)
					{
						fprintf(stderr, "error: domain name for \"%s\" (%d) could not be extracted successfully (returned: %d/%s)\n",
								uri, i, r, info.f_tld);
						++err_count;
					}
					/*
					else
						fprintf(stderr, "?? invalid: \"%s\" -> \"%s\"\n", uri, info.f_tld); 
					*/
				}
				else
				{
					if(strncmp(uri + info.f_offset - 11, "domain-name", 11) != 0)
					{
						fprintf(stderr, "error: domain name for \"%s\" (%d) could not be extracted successfully (returned: %d/%s)\n",
								uri, i, r, info.f_tld);
						++err_count;
					}
					/*
					else
						fprintf(stderr, "?? invalid: \"%s\" -> \"%s\"\n", uri, info.f_tld); 
					*/
				}
			}
		}
	}
}


/*
 * This test checks out URIs that end with an invalid TLD. This is
 * expected to return an error every single time.
 */
void test_unknown()
{
	struct bad_data
	{
		const char *		f_uri;
	};
	struct bad_data	d[] = {
		"this.is.wrong",
		"missing.tld",
		".net.absolutely.com.no.info.on.this"
	};
	struct tld_info	info;
	int i, max;
	enum tld_result r;

	max = sizeof(d) / sizeof(d[0]);
	for(i = 0; i < max; ++i)
	{
		memset(&info, 0xFE, sizeof(info));
		r = tld(d[i].f_uri, &info);
		if(r != TLD_RESULT_NOT_FOUND)
		{
			fprintf(stderr, "error: the invalid URI \"%s\" was found by tld()!\n", d[i].f_uri);
			++err_count;
		}
	}
}




void test_invalid()
{
	struct tld_info	undefined_info;
	struct tld_info	clear_info;
	struct tld_info	info;
	enum tld_result r;

	/*
	 * We reset the undefined_info the same way we reset the info
	 * structure because the alignment on 64bits may add another
	 * 4 bytes at the end of the structure that are not otherwise
	 * accessible.
	 */
	memset(&undefined_info, 0xFE, sizeof(undefined_info));
	undefined_info.f_category = TLD_CATEGORY_UNDEFINED;
	undefined_info.f_status   = TLD_STATUS_UNDEFINED;
	undefined_info.f_country  = (const char *) 0;
	undefined_info.f_tld      = (const char *) 0;
	undefined_info.f_offset   = -1;

	memset(&clear_info, 0xFE, sizeof(clear_info));

	/* test: NULL */
	info = clear_info;
	r = tld(NULL, &info);
	if(r != TLD_RESULT_NULL)
	{
		fprintf(stderr, "error: the NULL URI did not return the TLD_RESULT_NULL result.\n");
		++err_count;
	}
	if(memcmp(&info, &undefined_info, sizeof(info)) != 0)
	{
		fprintf(stderr, "error: the NULL URI did not return a reset info structure.\n");
		++err_count;
	}

	/* test: "" */
	info = clear_info;
	r = tld("", &info);
	if(r != TLD_RESULT_NULL)
	{
		fprintf(stderr, "error: the \"\" URI did not return the TLD_RESULT_NULL result.\n");
		++err_count;
	}
	if(memcmp(&info, &undefined_info, sizeof(info)) != 0)
	{
		fprintf(stderr, "error: the \"\" URI did not return a reset info structure.\n");
		++err_count;
	}

	/* test: ".." (two periods one after another) */
	info = clear_info;
	r = tld("test..com", &info);
	if(r != TLD_RESULT_BAD_URI)
	{
		fprintf(stderr, "error: the \"test..com\" URI did not return the TLD_RESULT_BAD_URI result.\n");
		++err_count;
	}
	if(memcmp(&info, &undefined_info, sizeof(info)) != 0)
	{
		fprintf(stderr, "error: the \"test..com\" URI did not return a reset info structure.\n");
		++err_count;
	}

	/* test: ".." (two periods one after another) */
	info = clear_info;
	r = tld("more..test.com", &info);
	if(r != TLD_RESULT_BAD_URI)
	{
		fprintf(stderr, "error: the \"more..test.com\" URI did not return the TLD_RESULT_BAD_URI result.\n");
		++err_count;
	}
	if(memcmp(&info, &undefined_info, sizeof(info)) != 0)
	{
		fprintf(stderr, "error: the \"more..test.com\" URI did not return a reset info structure.\n");
		++err_count;
	}

	/* test: "noperiodanywhere" (no periods anywhere) */
	info = clear_info;
	r = tld("noperiodanywhere", &info);
	if(r != TLD_RESULT_NO_TLD)
	{
		fprintf(stderr, "error: the \"noperiodanywhere\" URI did not return the TLD_RESULT_NO_TLD result.\n");
		++err_count;
	}
	if(memcmp(&info, &undefined_info, sizeof(info)) != 0)
	{
		fprintf(stderr, "error: the \"noperiodanywhere\" URI did not return a reset info structure.\n");
		++err_count;
	}
}




int main(int argc, char *argv[])
{
	fprintf(stderr, "testing tld version %s\n", tld_version());

	/* call all the tests, one by one
	 * failures are "recorded" in the err_count global variable
	 * and the process stops with an error message and exit(1)
	 * if err_count is not zero.
	 */
	test_all();
	test_unknown();
	test_invalid();

	if(err_count)
	{
		fprintf(stderr, "%d error%s occured.\n",
					err_count, err_count != 1 ? "s" : "");
	}
	exit(err_count ? 1 : 0);
}

/* vim: ts=4 sw=4
 */
