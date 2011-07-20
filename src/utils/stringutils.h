/** @file stringutils.h
 * @brief Some simple string utilities.
 */
/* Copyright 2011 Richard Boulton
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
#ifndef RESTPOSE_INCLUDED_STRINGUTILS_H
#define RESTPOSE_INCLUDED_STRINGUTILS_H

#include <string>

/** Check if a string starts with the given ending.
 */
inline bool
string_startswith(const std::string & text, const std::string & start)
{
    if (text.size() < start.size()) {
	return false;
    }
    return (0 == text.compare(0, start.size(), start));
}

/** Check if a string ends with the given ending.
 */
inline bool
string_endswith(const std::string & text, const std::string & ending)
{
    if (text.size() < ending.size()) {
	return false;
    }
    return (0 == text.compare(text.size() - ending.size(), ending.size(), ending));
}

template<class Iterator> std::string
string_join(const std::string & separator, Iterator begin, Iterator end)
{
    bool need_sep(false);
    std::string result;
    while (begin != end) {
	if (need_sep) {
	    result += separator;
	}
	result += *begin;
	need_sep = true;
	++begin;
    }
    return result;
}

/** Escape non-ascii characters in the string.
 */
inline std::string
hexesc(const std::string & input)
{
    std::string result;
    result.reserve(input.size() * 2);
    for (std::string::size_type i = 0; i != input.size(); ++i) {
	unsigned char ch = input[i];
	if (ch == '\\') {
	    result += '\\';
	    continue;
	} else if (ch >= 32 && ch <= 127) {
	    result += ch;
	    continue;
	}
	result += "\\x";
	result += "0123456789abcdef"[(ch & 0xf0) >> 4];
	result += "0123456789abcdef"[(ch & 0x0f)];
    }

    return result;
}

#endif /* RESTPOSE_INCLUDED_STRINGUTILS_H */
