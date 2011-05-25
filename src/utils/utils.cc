/** @file utils.cc
 * @brief General utility functions.
 */
/* Copyright (c) 2010 Richard Boulton
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <config.h>
#include "utils.h"

#include <ctype.h>
#include "str.h"
#include <string>
#include <string.h>

std::string
get_sys_error(int errno_value)
{
#ifdef HAVE_STRERROR_R
#define ERRBUFSIZE 256
    char buf[ERRBUFSIZE];
    buf[0] = '\0';
#ifdef STRERROR_R_CHAR_P
    return std::string(strerror_r(errno_value, buf, ERRBUFSIZE));
#else
    int ret = strerror_r(errno_value, buf, ERRBUFSIZE);
    if (ret == 0) {
	buf[ERRBUFSIZE - 1] = '\0';
	return std::string(buf);
    } else {
	return std::string("code " + str(errno_value));
    }
#endif
#undef ERRBUFSIZE
#else
    return std::string(strerror(errno_value));
#endif
}

std::string
urlquote(const std::string & value)
{
    std::string result;
    for (std::string::size_type i = 0;
	 i != value.size(); ++i) {
	char ch = value[i];
	if (isalnum(ch) || ch == '_' || ch == '-' || ch == '.') {
	    result += ch;
	} else {
	    result += '%';
	    result += "0123456789abcdef" [ch / 16];
	    result += "0123456789abcdef" [ch % 16];
	}
    }
    return result;
}

static int
hex_to_int(char ch)
{
    if (ch >= '0' && ch <= '9')
	return ch - '0';
    if (ch >= 'A' && ch <= 'F')
	return ch - 'A';
    if (ch >= 'a' && ch <= 'f')
	return ch - 'a';
    return 0;
}

std::string
urlunquote(const std::string & value)
{
    std::string result;
    for (std::string::size_type i = 0;
	 i != value.size(); ++i) {
	char ch = value[i];
	if (ch == '%') {
	    if (i + 2 >= value.size()) {
		result += value.substr(i, value.size() - i);
		break;
	    }
	    char ch1 = value[i + 1];
	    char ch2 = value[i + 1];
	    i += 2;
	    
	    result += char(hex_to_int(ch1) * 16 + hex_to_int(ch2));
	} else {
	    result += ch;
	}
    }
    return result;
}
