//
// File:	controlled_vars_test.cpp
// Object:	Test that the controlled_vars.h compiles and throws as expected.
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
#ifndef CONTROLLED_VARS_DEBUG
#define CONTROLLED_VARS_DEBUG
#endif
#ifndef CONTROLLED_VARS_LIMITED
#define CONTROLLED_VARS_LIMITED
#endif
#include "controlled_vars.h"
#include <stdio.h>
#include <stdlib.h>

// Yes we need to have a slightly better test that checks all the
// possibilities rather than just two or three!

class Test
{
public:
	Test() : f_mandatory_int32(123)
	{
	}

	controlled_vars::zfloat_t				f_zero_float;
	controlled_vars::mint32_t				f_mandatory_int32;
	controlled_vars::rbool_t				f_random_bool;
	controlled_vars::limited_auto_init<int32_t, 1, 100, 50>	f_auto_percent;
};


#pragma GCC push
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter" )
int main(int argc, char * /*argv*/[])
#pragma GCC pop
{
	int e(0);
	Test t;

	//t.f_zero_float %= 33.5f;  // does not compile (good)
	//t.f_auto_percent %= 33.5f;  // does not compile (good)

	// decrementing beyond the limit
	t.f_auto_percent = 2;
	--t.f_auto_percent;
	try {
		--t.f_auto_percent;
		fprintf(stderr, "error: expected exception on -- did not occur.\n");
		e = 1;
	}
	catch(const controlled_vars::controlled_vars_error&) {
		// it worked
	}

	// incrementing beyond the limit
	t.f_auto_percent = 99;
	++t.f_auto_percent;
	try {
		++t.f_auto_percent;
		fprintf(stderr, "error: expected exception on ++ did not occur.\n");
		e = 1;
	}
	catch(const controlled_vars::controlled_vars_error&) {
		// it worked
	}

	// incrementing beyond the limit
	t.f_auto_percent = 90;
	t.f_auto_percent += 5;
	try {
		t.f_auto_percent += 7;
		fprintf(stderr, "error: expected exception on += did not occur (current value: %d).\n", t.f_auto_percent.value());
		e = 1;
	}
	catch(const controlled_vars::controlled_vars_error&) {
		// it worked
	}

	// reading the random value in debug mode throws
	try {
		argc = t.f_random_bool;
		fprintf(stderr, "error: expected exception on read did not occur.\n");
		e = 1;
	}
	catch(const controlled_vars::controlled_vars_error&) {
		// it worked
	}

	exit(e);
}

