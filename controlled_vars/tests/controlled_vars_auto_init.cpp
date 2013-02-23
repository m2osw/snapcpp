//
// File:	controlled_vars_auto_init.cpp
// Object:	Test that the controlled_vars_auto_init.h compiles
//              auto-initialize as expected.
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
#include "controlled_vars_auto_init.h"
#include <stdio.h>
#include <stdlib.h>


class Test
{
public:
	controlled_vars::zbool_t				f_false;
	controlled_vars::zschar_t				f_int8;
	controlled_vars::zint16_t				f_int16;
	controlled_vars::zint32_t				f_int32;
	controlled_vars::zint64_t				f_int64;
};


int main(int argc, char *argv[])
{
	int e(0);
	Test t;

	if(t.f_false != false) {
		fprintf(stderr, "error: expected t.f_false to be false.\n");
		e = 1;
	}

	if(t.f_int8 != 0) {
		fprintf(stderr, "error: expected t.f_int8 to be zero.\n");
		e = 1;
	}

	if(t.f_int16 != 0) {
		fprintf(stderr, "error: expected t.f_int16 to be zero.\n");
		e = 1;
	}

	if(t.f_int32 != 0) {
		fprintf(stderr, "error: expected t.f_int32 to be zero.\n");
		e = 1;
	}

	if(t.f_int64 != 0) {
		fprintf(stderr, "error: expected t.f_int64 to be zero.\n");
		e = 1;
	}

	exit(e);
}

