/** @file docdata.cc
 * @brief Abstraction for storing document data in fields.
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

#include "docdata.h"
#include "serialise.h"
#include "utils/jsonutils.h"

using namespace RestPose;
using namespace std;

std::string
DocumentData::serialise() const
{
    std::map<std::string, std::string>::const_iterator i;

    // Get an estimate of the required length.  The encoded form is a series of
    // strings, each preceded by its length as a variable encoding integer,
    // which probably takes 1 or 2 bytes.  We want to err on the side of
    // avoiding a reallocation, so we guess 4 bytes for each length.
    int expected_len = 0;
    for (i = fields.begin(); i != fields.end(); ++i) {
	expected_len += i->first.size() + i->second.size() + 8;
    }
    std::string result;
    result.reserve(expected_len);
    for (i = fields.begin(); i != fields.end(); ++i) {
	result += encode_length(i->first.size());
	result += i->first;
	result += encode_length(i->second.size());
	result += i->second;
    }

    return result;
};

void
DocumentData::unserialise(const std::string &s)
{
    fields.clear();
    const char * ptr = s.data();
    const char * endptr = ptr + s.size();
    while (ptr != endptr) {
	size_t len = rsp_decode_length(&ptr, endptr, true);
	std::string field(ptr, len);
	ptr += len;
	len = rsp_decode_length(&ptr, endptr, true);
	std::string value(ptr, len);
	ptr += len;
	fields[field] = value;
    }
}

Json::Value &
DocumentData::to_display(const Json::Value & fieldlist,
			 Json::Value & result) const
{
    result = Json::objectValue;
    if (fieldlist.isNull()) {
	// Return all fields.
	for (std::map<std::string, std::string>::const_iterator
	     i = fields.begin(); i != fields.end(); ++i) {
	    if (!i->second.empty()) {
		Json::Value tmp;
		result[i->first] = json_unserialise(i->second, tmp);
	    }
	}
    } else {
	// Return fields in fieldlist
	for (Json::Value::const_iterator fiter = fieldlist.begin();
	     fiter != fieldlist.end();
	     ++fiter) {
	    string fieldname((*fiter).asString());
	    std::map<std::string, std::string>::const_iterator
		    i = fields.find(fieldname);
	    if (i != fields.end()) {
		if (!i->second.empty()) {
		    Json::Value tmp;
		    result[fieldname] = json_unserialise(i->second, tmp);
		}
	    }
	}
    }
    return result;
}
