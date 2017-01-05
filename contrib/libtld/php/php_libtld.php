<?php
/* TLD library -- PHP declarations
 * Copyright (C) 2013-2017  Made to Order Software Corp.
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

/*
 * I'm not too sure, at this time, how to add those declarations in the code
 * so it's writte in a .php that you can simply include.
 */

define('TLD_CATEGORY_INTERNATIONAL', 0);
define('TLD_CATEGORY_PROFESSIONALS', 1);
define('TLD_CATEGORY_LANGUAGE', 2);
define('TLD_CATEGORY_GROUPS', 3);
define('TLD_CATEGORY_REGION', 4);
define('TLD_CATEGORY_TECHNICAL', 5);
define('TLD_CATEGORY_COUNTRY', 6);
define('TLD_CATEGORY_ENTREPRENEURIAL', 7);
define('TLD_CATEGORY_BRAND', 8);
define('TLD_CATEGORY_UNDEFINED', 9);

define('TLD_STATUS_VALID', 0);
define('TLD_STATUS_PROPOSED', 1);
define('TLD_STATUS_DEPRECATED', 2);
define('TLD_STATUS_UNUSED', 3);
define('TLD_STATUS_RESERVED', 4);
define('TLD_STATUS_INFRASTRUCTURE', 5);
define('TLD_STATUS_UNDEFINED', 6);
define('TLD_STATUS_EXCEPTION', 100);

define('TLD_RESULT_SUCCESS', 0);
define('TLD_RESULT_INVALID', 1);
define('TLD_RESULT_NULL', 2);
define('TLD_RESULT_NO_TLD', 3);
define('TLD_RESULT_BAD_URI', 4);
define('TLD_RESULT_NOT_FOUND', 5);

define('TLD_VALID_URI_ASCII_ONLY', 0x0001);
define('TLD_VALID_URI_NO_SPACES', 0x0002);

