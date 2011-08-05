/** @file validation.cc
 * @brief Validate names for various things
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

#include <config.h>
#include "utils/validation.h"

#include "utils/rsperrors.h"
#include "utils/stringutils.h"

using namespace RestPose;
using namespace std;

string
validate_collname(const string & value)
{
    if (value.empty()) {
	return "Invalid empty collection name";
    }
    for (string::size_type i = 0; i != value.size(); ++i) {
	unsigned char ch = value[i];
	switch (ch) {
	    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	    case 8: case 9: case 10: case 11: case 12: case 13: case 14:
	    case 15: case 16: case 17: case 18: case 19: case 20: case 21:
	    case 22: case 23: case 24: case 25: case 26: case 27: case 28:
	    case 29: case 30: case 31: case ':': case '/': case '\\':
	    case '.': case ',': case '[': case ']': case '{': case '}':
		return "Invalid character (" + hexesc(value.substr(i, 1)) + ") in collection name";
	}
    }
    return string();
}

void
validate_collname_throw(const string & value)
{
    string error = validate_collname(value);
    if (!error.empty()) {
	throw InvalidValueError(error);
    }
}

string
validate_doc_type(const string & value)
{
    if (value.empty()) {
	return "Invalid empty document type name";
    }
    for (string::size_type i = 0; i != value.size(); ++i) {
	unsigned char ch = value[i];
	switch (ch) {
	    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	    case 8: case 9: case 10: case 11: case 12: case 13: case 14:
	    case 15: case 16: case 17: case 18: case 19: case 20: case 21:
	    case 22: case 23: case 24: case 25: case 26: case 27: case 28:
	    case 29: case 30: case 31: case ':': case '/': case '\\':
	    case '.': case ',': case '[': case ']': case '{': case '}':
		return "Invalid character (" + hexesc(value.substr(i, 1)) + ") in document type";
	}
    }
    return string();
}

string
validate_doc_id(const string & value)
{
    if (value.empty()) {
	return "Invalid empty document ID";
    }
    for (string::size_type i = 0; i != value.size(); ++i) {
	unsigned char ch = value[i];
	switch (ch) {
	    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	    case 8: case 9: case 10: case 11: case 12: case 13: case 14:
	    case 15: case 16: case 17: case 18: case 19: case 20: case 21:
	    case 22: case 23: case 24: case 25: case 26: case 27: case 28:
	    case 29: case 30: case 31: case ':': case '/': case '\\':
	    case '.': case ',': case '[': case ']': case '{': case '}':
		return "Invalid character (" + hexesc(value.substr(i, 1)) + ") in document ID";
	}
    }
    return string();
}

string
validate_catid(const string & value)
{
    if (value.empty()) {
	return "Invalid empty category identifier";
    }
    for (string::size_type i = 0; i != value.size(); ++i) {
	unsigned char ch = value[i];
	switch (ch) {
	    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	    case 8: case 9: case 10: case 11: case 12: case 13: case 14:
	    case 15: case 16: case 17: case 18: case 19: case 20: case 21:
	    case 22: case 23: case 24: case 25: case 26: case 27: case 28:
	    case 29: case 30: case 31: case ':': case '/': case '\\':
	    case '.': case ',': case '[': case ']': case '{': case '}':
		return "Invalid character (" + hexesc(value.substr(i, 1)) + ") in category identifier";
	}
    }
    return string();
}

void
validate_catid_throw(const string & value)
{
    string error = validate_catid(value);
    if (!error.empty()) {
	throw InvalidValueError(error);
    }
}
