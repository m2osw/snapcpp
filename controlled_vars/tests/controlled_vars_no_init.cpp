//
// File:	controlled_vars_no_init.cpp
// Object:	Test that the controlled_vars_no_init.h compiles
//              requires initialization as expected.
//
// Copyright:	Copyright (c) 2011 Made to Order Software Corp.
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

// make sure we compile in debug mode otherwise we don't get the throws.
#define CONTROLLED_VARS_DEBUG
#include "controlled_vars_no_init.h"
#include "controlled_vars_no_enum_init.h"
#include <stdio.h>
#include <stdlib.h>


class Test
{
public:
	controlled_vars::rbool_t				f_false;
	controlled_vars::rschar_t				f_int8;
	controlled_vars::rint16_t				f_int16;
	controlled_vars::rint32_t				f_int32;
	controlled_vars::rint64_t				f_int64;
};


int main(int /*argc*/, char * /*argv*/[])
{
	int e(0);
	Test t;

	try {
		bool f = t.f_false;
		fprintf(stderr, "error: excpected a throw when reading uninitialized f_false variable: %d\n", f);
		e = 1;
	}
	catch(const controlled_vars::controlled_vars_error&) {
		// it worked
	}

	try {
		signed char c = t.f_int8;
		fprintf(stderr, "error: excpected a throw when reading uninitialized f_int8 variable: %d\n", c);
		e = 1;
	}
	catch(const controlled_vars::controlled_vars_error&) {
		// it worked
	}

	try {
		short s = t.f_int16;
		fprintf(stderr, "error: excpected a throw when reading uninitialized f_int16 variable: %d\n", s);
		e = 1;
	}
	catch(const controlled_vars::controlled_vars_error&) {
		// it worked
	}

	try {
		int i = t.f_int32;
		fprintf(stderr, "error: excpected a throw when reading uninitialized f_int32 variable: %d\n", i);
		e = 1;
	}
	catch(const controlled_vars::controlled_vars_error&) {
		// it worked
	}

	try {
		int64_t l = t.f_int64;
		fprintf(stderr, "error: excpected a throw when reading uninitialized f_int64 variable: %d\n", static_cast<int>(l));
		e = 1;
	}
	catch(const controlled_vars::controlled_vars_error&) {
		// it worked
	}

	exit(e);
}

