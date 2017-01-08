/* TLD library -- test the TLD version in all files
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
 * \brief Test the tld library versions.
 *
 * This file implements various test to verify that the
 * version is properly defined in all the files as expected.
 *
 * In the compiled data, the version gets copied using the
 * configure_file() from cmake. Other files, such as the changelog,
 * require manual labor and thus the version could end up
 * being wrong (humans are notorious to forget stuff like that).
 */

#include "libtld/tld.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

int err_count = 0;
int verbose = 0;



void check_version_compiled_with(const char *version)
{
    if(verbose)
    {
        printf("version library was compiled with: %s\n", LIBTLD_VERSION);
    }

    if(strcmp(LIBTLD_VERSION, version) != 0)
    {
        ++err_count;
        fprintf(stderr, "error: compiled with version %s, expected %s instead.\n", LIBTLD_VERSION, version);
    }
}



// Newer versions retrieve the version directly from the changelog file
//void check_main_cmakefiles_txt(const char *path, const char *version)
//{
//    const int cmakelists_txt_len = 15;
//    const int len = strlen(path);
//    char *filename = malloc(len + cmakelists_txt_len + 1);
//    FILE *f;
//    char buf[1024];
//    char *str;
//    int major = 0;
//    int minor = 0;
//    int patch = 0;
//
//    memcpy(filename, path, len);
//    memcpy(filename + len, "/CMakeLists.txt", cmakelists_txt_len + 1); // copy string and '\0'
//
//    f = fopen(filename, "r");
//    if(f == NULL)
//    {
//        ++err_count;
//        perror("error: fopen()");
//        fprintf(stderr, "error: could not open main CMakeLists.txt file (full path: \"%s\")\n:", filename);
//        free(filename);
//        return;
//    }
//
//    while(fgets(buf, sizeof(buf), f) != (char *) 0)
//    {
//        str = strstr(buf, "LIBTLD_VERSION_MAJOR");
//        if(str != (char *) 0 && str > buf && str[-1] != '{')
//        {
//            major = atol(str + 20);
//        }
//        str = strstr(buf, "LIBTLD_VERSION_MINOR");
//        if(str != (char *) 0 && str > buf && str[-1] != '{')
//        {
//            minor = atol(str + 20);
//        }
//        str = strstr(buf, "LIBTLD_VERSION_PATCH");
//        if(str != (char *) 0 && str > buf && str[-1] != '{')
//        {
//            patch = atol(str + 20);
//        }
//    }
//    fclose(f);
//
//    snprintf(buf, sizeof(buf), "%d.%d.%d", major, minor, patch);
//
//    if(verbose)
//    {
//        printf("main CMakeLists.txt version: %s\n", buf);
//    }
//
//    if(strcmp(buf, version) != 0)
//    {
//        ++err_count;
//        fprintf(stderr, "error: main CMakeLists.txt version (%s) is %s, expected %s instead.\n", filename, buf, version);
//    }
//
//    free(filename);
//}


void check_libtld_only_cmakefiles_txt(const char *path, const char *version)
{
    const int cmakelists_txt_len = 31;
    const int len = strlen(path);
    char *filename = malloc(len + cmakelists_txt_len + 1);
    FILE *f;
    char buf[1024];
    char *str;
    int major = 0;
    int minor = 0;
    int patch = 0;

    memcpy(filename, path, len);
    memcpy(filename + len, "/dev/libtld-only-CMakeLists.txt", cmakelists_txt_len + 1); // copy string and '\0'

    f = fopen(filename, "r");
    if(f == NULL)
    {
        ++err_count;
        perror("error: fopen()");
        fprintf(stderr, "error: could not open libtld only CMakeLists.txt file (full path: \"%s\")\n:", filename);
        free(filename);
        return;
    }

    while(fgets(buf, sizeof(buf), f) != (char *) 0)
    {
        str = strstr(buf, "LIBTLD_VERSION_MAJOR");
        if(str != (char *) 0 && str > buf && str[-1] != '{')
        {
            major = atol(str + 20);
        }
        str = strstr(buf, "LIBTLD_VERSION_MINOR");
        if(str != (char *) 0 && str > buf && str[-1] != '{')
        {
            minor = atol(str + 20);
        }
        str = strstr(buf, "LIBTLD_VERSION_PATCH");
        if(str != (char *) 0 && str > buf && str[-1] != '{')
        {
            patch = atol(str + 20);
        }
    }
    fclose(f);

    snprintf(buf, sizeof(buf), "%d.%d.%d", major, minor, patch);

    if(verbose)
    {
        printf("libtld only CMakeLists.txt version: %s\n", buf);
    }

    if(strcmp(buf, version) != 0)
    {
        ++err_count;
        fprintf(stderr, "error: libtld only CMakeLists.txt version (%s) is %s, expected %s instead.\n", filename, buf, version);
    }

    free(filename);
}


void check_changelog(const char *path, const char *version)
{
    const int changelog_len = 17;
    const int len = strlen(path);
    char *filename = malloc(len + changelog_len + 1);
    FILE *f;
    char buf[1024];
    char *str;
    char *debian_version;
    int i;

    memcpy(filename, path, len);
    memcpy(filename + len, "/debian/changelog", changelog_len + 1); // copy string and '\0'

    f = fopen(filename, "r");
    if(f == NULL)
    {
        ++err_count;
        perror("error: fopen()");
        fprintf(stderr, "error: could not open debian/changelog file (full path: \"%s\")\n:", filename);
        free(filename);
        return;
    }

    if(fgets(buf, sizeof(buf), f) == (char *) 0)
    {
        ++err_count;
        perror("error: fgets()");
        fprintf(stderr, "error: could not read the first line of debian/changelog (full path: \"%s\")\n:", filename);
        free(filename);
        fclose(f);
        return;
    }

    fclose(f);

    if(buf[0] != 'l'
    || buf[1] != 'i'
    || buf[2] != 'b'
    || buf[3] != 't'
    || buf[4] != 'l'
    || buf[5] != 'd'
    || buf[6] != ' '
    || buf[7] != '(')
    {
        ++err_count;
        fprintf(stderr, "error: debian/changelog does not start with \"libtld (\" as expected (full path: \"%s\")\n:", filename);
        free(filename);
        return;
    }

    debian_version = buf + 8;

    for(i = 0; debian_version[i] != '\0' && debian_version[i] != ')'; ++i);

    if(debian_version[i] != ')')
    {
        ++err_count;
        fprintf(stderr, "error: debian/changelog does not seem to include a valid version (full path: \"%s\")\n:", filename);
        free(filename);
        return;
    }
    debian_version[i] = '\0';  // this modifies buf which is fine

    // the debian version is likely to include a build number and platform name
    i = 0;
    for(str = debian_version; str != '\0'; ++str)
    {
        if(*str == '~')
        {
            // stop immediately
            break;
        }
        else if(*str == '.')
        {
            ++i;
            if(i >= 3)
            {
                // stop at the 3rd '.' (i.e. build number)
                break;
            }
        }
    }
    *str = '\0'; // remove extra stuff

    if(verbose)
    {
        printf("changelog version: %s\n", debian_version);
    }

    if(strcmp(debian_version, version) != 0)
    {
        ++err_count;
        fprintf(stderr, "error: changelog version (%s) is %s, expected %s instead.\n", filename, debian_version, version);
    }

    free(filename);
}



int main(int argc, char *argv[])
{
    const char *version = tld_version();
    char *path = (char *) 0;
    int i;

    for(i = 1; i < argc; ++i)
    {
        if(strcmp(argv[i], "-v") == 0)
        {
            verbose = 1;
        }
        else
        {
            path = argv[i];
        }
    }

    if(path == (char *) 0)
    {
        ++err_count;
        fprintf(stderr, "error: mandatory source path not specified on command line.\n");
    }
    else
    {
        if(verbose)
        {
            printf("runtime libtld version: %s\n", version);
        }
        else
        {
            printf("%s\n", version);
        }

        check_version_compiled_with(version);
        //check_main_cmakefiles_txt(path, version);
        check_libtld_only_cmakefiles_txt(path, version);
        check_changelog(path, version);
    }

    exit(err_count ? 1 : 0);
}

/* vim: ts=4 sw=4 et
 */

