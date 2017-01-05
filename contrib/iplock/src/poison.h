//
// File:        src/iplock.h
// Object:      Allow users to easily add and remove IPs in an iptable
//              firewall; this is useful if you have a blacklist of IPs
//
// Copyright:   Copyright (c) 2007-2017 Made to Order Software Corp.
//              All Rights Reserved.
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
#ifndef IPLOCK_POISON_H
#define IPLOCK_POISON_H

#pragma GCC poison strcat strncat wcscat wcsncat strcpy
#pragma GCC poison printf   fprintf   sprintf   snprintf \
                   vprintf  vfprintf  vsprintf  vsnprintf \
                   wprintf  fwprintf  swprintf  snwprintf \
                   vwprintf vfwprintf vswprintf vswnprintf

#endif
// vim: ts=4 sw=4 et
