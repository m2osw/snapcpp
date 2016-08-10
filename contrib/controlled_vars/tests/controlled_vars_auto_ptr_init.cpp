//
// File:	controlled_vars_auto_ptr_init.cpp
// Object:	Test that the controlled_vars_auto_ptr_init.h compiles
//              requires initialization as expected.
//
// Copyright:	Copyright (c) 2012 Made to Order Software Corp.
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

#include "controlled_vars_ptr_auto_init.h"
#include <stdio.h>

int main()
{
	//typedef controlled_vars::auto_ptr_init<char> zpchar_t;

	controlled_vars::zpchar_t c;
	c += 10;
	c -= 8;
	c++;
	controlled_vars::zpchar_t q(c);
	c--;
	char buf[256];
	controlled_vars::zpchar_t z(*buf);
	--c;
	++c;
	printf("c = %p   q = %p  z = %p\n", c.get(), q.get(), z.get());
	if(c) {
		printf("c is \"true\"\n");
	}
	c.swap(z);
	printf("c = %p   z = %p\n", c.get(), z.get());
	z.swap(c);
	//char *g(z);
	char *ptr(new char[123]);
	controlled_vars::zpchar_t nptr(ptr);
	// these work, but the z[5] = ... below then crashes
	//z.reset(*g);
	//z.reset(&c);

	controlled_vars::zpchar_t f;
	if(f) {
		printf("error: f is \"true\"\n");
	}
	else {
		printf("f is \"false\"\n");
	}
	if(!f) {
		printf("f is really \"false\"\n");
	}
	z[5] = 0x54;
	printf("buf[5] = %c  and z[5] = %c\n", buf[5], z[5]);
	//printf("f value = %c", *f); // properly throws with a null pointer dereference error
}

