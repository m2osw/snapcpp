//
// File:	controlled_vars_limited_need_init.cpp
// Object:	Test that the controlled_vars_limited_need_init.h compiles
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
#define CONTROLLED_VARS_LIMITED
#include "controlled_vars_limited_need_init.h"
#include <stdio.h>
#include <stdlib.h>


typedef controlled_vars::limited_need_init<int, 0, 100> mpercent_t;


class Test
{
public:
	Test() : f_percent(50)
	{
	}

	mpercent_t				f_percent;
};


int main(int /*argc*/, char * /*argv*/[])
{
	int e(0);
	Test t;

	if(t.f_percent != 50) {
		fprintf(stderr, "error: expected t.f_percent to be 50, got %d instead.\n", t.f_percent.value());
		e = 1;
	}

	exit(e);
}

