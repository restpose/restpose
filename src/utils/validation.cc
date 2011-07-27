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

void
validate_collname(const std::string & value)
{
    for (std::string::size_type i = 0; i != value.size(); ++i) {
	unsigned char ch = value[i];
	switch (ch) {
	    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	    case 8: case 9: case 10: case 11: case 12: case 13: case 14:
	    case 15: case 16: case 17: case 18: case 19: case 20: case 21:
	    case 22: case 23: case 24: case 25: case 26: case 27: case 28:
	    case 29: case 30: case 31: case ':': case '/': case '\\':
	    case '.': case ',':
		throw InvalidValueError("Invalid character (" + hexesc(value.substr(i, 1)) + ") in collection name");
	}
    }
}

/** Check if a document type is valid.
 *
 *  Raises InvalidValueError if the type is not valid.
 */
void
validate_doc_type(const std::string & value)
{
    for (std::string::size_type i = 0; i != value.size(); ++i) {
	unsigned char ch = value[i];
	switch (ch) {
	    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	    case 8: case 9: case 10: case 11: case 12: case 13: case 14:
	    case 15: case 16: case 17: case 18: case 19: case 20: case 21:
	    case 22: case 23: case 24: case 25: case 26: case 27: case 28:
	    case 29: case 30: case 31: case ':': case '/': case '\\':
	    case '.': case ',':
		throw InvalidValueError("Invalid character (" + hexesc(value.substr(i, 1)) + ") in document type");
	}
    }
}

/** Check if a document ID is valid.
 *
 *  Raises InvalidValueError if the ID is not valid.
 */
void
validate_doc_id(const std::string & value)
{
    for (std::string::size_type i = 0; i != value.size(); ++i) {
	unsigned char ch = value[i];
	switch (ch) {
	    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
	    case 8: case 9: case 10: case 11: case 12: case 13: case 14:
	    case 15: case 16: case 17: case 18: case 19: case 20: case 21:
	    case 22: case 23: case 24: case 25: case 26: case 27: case 28:
	    case 29: case 30: case 31: case ':': case '/': case '\\':
	    case '.': case ',':
		throw InvalidValueError("Invalid character (" + hexesc(value.substr(i, 1)) + ") in document ID");
	}
    }
}

