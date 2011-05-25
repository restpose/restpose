/** @file mapping.cc
 * @brief Tests for JSON mappings
 */
/* Copyright (C) 2011 Richard Boulton
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
#include "UnitTest++.h"

#include "jsonmanip/mapping.h"
#include "jsonxapian/collection.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include <json/json.h>

#include <string>
#include <vector>

#include <cstdio>

using namespace RestPose;

/** Apply a mapping test to several docs, returning a string */
static std::string
map_docs(const std::string & docs, const Mapping & mapping)
{
    std::vector<Json::Value>::const_iterator i;
    std::string result;
    Json::Value input, output;
    Collection collection("foo", "foo");

    size_t pos = 0;
    while (true) {
	size_t lastpos = pos;
	pos = docs.find('\n', pos + 1);
	if (pos == docs.npos) break;
	json_unserialise(docs.substr(lastpos, pos), input);
	bool pass = mapping.apply(collection, input, output);
	result += pass ? "T" : "F";
	result += json_serialise(output);
	result += '\n';
    }
    return result;
}

static const char * docs =
"{}\n"
"{\"name\": \"arthur\"}\n"
"{\"name\": [\"arthur\", \"dent\"]}\n"
"{\"name\": {\"first\": \"arthur\", \"second\": \"dent\"}}\n"
;

/// Test a mapping with just a conditional
TEST(MappingConditional)
{
    Mapping m;
    Json::Value tmp;
    json_unserialise("{\"when\": {\"exists\": [\"name\"]}}", tmp);
    m.from_json(tmp);
    {
	Json::Value tmp2;
	m.to_json(tmp2);
	CHECK_EQUAL("{\"when\":{\"exists\":[\"name\"]}}",
		    json_serialise(tmp2));
    }
    CHECK_EQUAL("Fnull\n"
		"T{\"name\":[\"arthur\"]}\n"
		"T{\"name\":[\"arthur\",\"dent\"]}\n"
		"T{\"name\":[{\"first\":\"arthur\",\"second\":\"dent\"}]}\n",
		map_docs(docs, m));
}

/// Test a simple mapping
TEST(MappingSimple)
{
    Mapping m;
    Json::Value tmp;
    json_unserialise("{\"when\": {\"exists\": [\"name\"]},"
		     "\"map\": [{\"from\": [\"name\"], \"to\": \"nom\"}]}",
		     tmp);
    m.from_json(tmp);
    {
	Json::Value tmp2;
	m.to_json(tmp2);
	CHECK_EQUAL("{\"map\":[{\"from\":[\"name\"],\"to\":\"nom\"}],"
		    "\"when\":{\"exists\":[\"name\"]}}",
		    json_serialise(tmp2));
    }
    CHECK_EQUAL("Fnull\n"
		"T{\"nom\":[\"arthur\"]}\n"
		"T{\"nom\":[\"arthur\",\"dent\"]}\n"
		"T{\"nom\":[{\"first\":\"arthur\",\"second\":\"dent\"}]}\n",
		map_docs(docs, m));

}

/// Test a mapping with multiple levels
TEST(MappingMultiLevel)
{
    Mapping m;
    Json::Value tmp;
    json_unserialise("{\"map\": ["
		     "{\"from\": \"name\", \"to\": \"name1\"},"
		     "{\"from\": [\"name\"], \"to\": \"name2\"},"
		     "{\"from\": [\"name\", \"first\"], \"to\": \"name3\"}"
		     "]}",
		     tmp);
    m.from_json(tmp);
    {
	Json::Value tmp2;
	m.to_json(tmp2);
	CHECK_EQUAL("{\"map\":["
		     "{\"from\":[\"name\"],\"to\":\"name1\"},"
		     "{\"from\":[\"name\"],\"to\":\"name2\"},"
		     "{\"from\":[\"name\",\"first\"],\"to\":\"name3\"}"
		    "]}",
		    json_serialise(tmp2));
    }
    CHECK_EQUAL("T{}\n"
		"T{\"name1\":[\"arthur\"],\"name2\":[\"arthur\"]}\n"
		"T{\"name1\":[\"arthur\",\"dent\"],\"name2\":[\"arthur\",\"dent\"]}\n"
		"T{\"name1\":[{\"first\":\"arthur\",\"second\":\"dent\"}],"
		  "\"name2\":[{\"first\":\"arthur\",\"second\":\"dent\"}],"
		  "\"name3\":[\"arthur\"]"
		"}\n",
		map_docs(docs, m));

}

/// Test a mapping with indexes
TEST(MappingIndexes)
{
    Mapping m;
    Json::Value tmp;
    json_unserialise("{\"map\": ["
		     "{\"from\": [\"name\", 0], \"to\": \"name1\"}"
		     "]}",
		     tmp);
    m.from_json(tmp);
    {
	Json::Value tmp2;
	m.to_json(tmp2);
	CHECK_EQUAL("{\"map\":["
		     "{\"from\":[\"name\",0],\"to\":\"name1\"}"
		    "]}",
		    json_serialise(tmp2));
    }
    CHECK_EQUAL("T{}\n"
		"T{\"name\":[\"arthur\"]}\n"
		"T{\"name1\":[\"arthur\"]}\n"
		"T{\"name\":[{\"first\":\"arthur\",\"second\":\"dent\"}]}\n",
		map_docs(docs, m));

}
