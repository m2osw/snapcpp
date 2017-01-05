/*    unittest_main.h
 *    Copyright (C) 2006-2017  Made to Order Software Corporation
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *    Authors
 *    Alexis Wilke   alexis@m2osw.com
 */
#ifndef UNIT_TEST_MAIN_H
#define UNIT_TEST_MAIN_H
#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <catch.hpp>
#pragma GCC diagnostic pop

#ifdef _MSC_VER
// The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: ... See online help for details.
// This is because of the putenv() and strdup() used in the class below
#pragma warning(disable: 4996)
#endif

namespace unittest
{

extern std::string   tmp_dir;

class obj_setenv
{
public:
	obj_setenv(const std::string& var)
		: f_copy(strdup(var.c_str()))
	{
		putenv(f_copy);
		std::string::size_type p(var.find_first_of('='));
		f_name = var.substr(0, p);
	}
	~obj_setenv()
	{
		putenv(strdup((f_name + "=").c_str()));
		free(f_copy);
	}

private:
	char *		f_copy;
	std::string	f_name;
};



}
#endif
