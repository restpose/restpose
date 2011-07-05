/** @file utils.h
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

#ifndef RESTPOSE_INCLUDED_UTILS_H
#define RESTPOSE_INCLUDED_UTILS_H

#include <string>

/** Get a string description of an errno value.
 *
 *  @param errno_value The error code, usually obtained from errno.
 */
std::string get_sys_error(int errno_value);

/// Quote a url string (ie, replace unsafe characters with %XX values)
std::string urlquote(const std::string & value);

/// Unquote a url string (ie, replace %XX values with characters)
std::string urlunquote(const std::string & value);

/// Return true iff str starts with prefix.
inline bool startswith(const std::string & str, const std::string & prefix) {
    return str.rfind(prefix, 0) == 0;
}

#endif /* RESTPOSE_INCLUDED_UTILS_H */
