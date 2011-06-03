/** @file search.cc
 * @brief Tests for searches
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
#include "UnitTest++.h"
#include <json/json.h>
#include "jsonxapian/doctojson.h"
#include "jsonxapian/schema.h"
#include "utils/rsperrors.h"
#include "utils/jsonutils.h"

using namespace RestPose;

TEST(SearchIntegerExactFields)
{
    Schema s("");
    s.set("id", new IDFieldConfig(""));
    s.set("intid", new ExactFieldConfig("intid", 30, ExactFieldConfig::TOOLONG_ERROR, "intid", 0));

    Xapian::WritableDatabase db(Xapian::InMemory::open());

    // Add a couple of documents.
    {
	Json::Reader reader;
	Json::Value value;
	CHECK(reader.parse("{\"id\": 32, \"intid\": 18446744073709551615}", value, false)); // 2**64-1
	std::string idterm;
	Xapian::Document doc(s.process(value, idterm));
	CHECK_EQUAL(doc_to_json_string(doc), "{\"data\":{\"intid\":[18446744073709551615]},\"terms\":{\"\\t\\t32\":{},\"intid\\t18446744073709551615\":{}}}");
	CHECK_EQUAL(idterm, "\t\t32");
	db.add_document(doc);
    }
    {
	Json::Reader reader;
	Json::Value value;
	CHECK(reader.parse("{\"id\": 18446744073709551615, \"intid\": 31}", value, false)); // 2**64-1
	std::string idterm;
	Xapian::Document doc(s.process(value, idterm));
	CHECK_EQUAL(doc_to_json_string(doc), "{\"data\":{\"intid\":[31]},\"terms\":{\"\\t\\t18446744073709551615\":{},\"intid\\t31\":{}}}");
	CHECK_EQUAL(idterm, "\t\t18446744073709551615");
	db.add_document(doc);
    }

    // First, check a search which should return nothing
    {
	Json::Value search_results(Json::objectValue);
	std::string search_str = "{\"query\":{\"field\":[\"id\",\"is\",[\"31\"]]}}";
	s.perform_search(db, search_str, search_results);
	CHECK_EQUAL("{\"checkatleast\":0,\"from\":0,\"items\":[],\"matches_estimated\":0,\"matches_lower_bound\":0,\"matches_upper_bound\":0,\"size\":10}",
		    json_serialise(search_results));
    }

    // Check a search matching on a low-range id as a string
    {
	Json::Value search_results(Json::objectValue);
	std::string search_str = "{\"query\":{\"field\":[\"id\",\"is\",[\"32\"]]}}";
	s.perform_search(db, search_str, search_results);
	CHECK_EQUAL("{\"checkatleast\":0,\"from\":0,\"items\":[{\"intid\":[18446744073709551615]}],\"matches_estimated\":1,\"matches_lower_bound\":1,\"matches_upper_bound\":1,\"size\":10}",
		    json_serialise(search_results));
    }

    // Check a search matching on a low-range id as a number
    {
	Json::Value search_results(Json::objectValue);
	std::string search_str = "{\"query\":{\"field\":[\"id\",\"is\",[32]]}}";
	s.perform_search(db, search_str, search_results);
	CHECK_EQUAL("{\"checkatleast\":0,\"from\":0,\"items\":[{\"intid\":[18446744073709551615]}],\"matches_estimated\":1,\"matches_lower_bound\":1,\"matches_upper_bound\":1,\"size\":10}",
		    json_serialise(search_results));
    }

    // Check a search matching on a high-range id as a number
    {
	Json::Value search_results(Json::objectValue);
	std::string search_str = "{\"query\":{\"field\":[\"id\",\"is\",[18446744073709551615]]}}";
	s.perform_search(db, search_str, search_results);
	CHECK_EQUAL("{\"checkatleast\":0,\"from\":0,\"items\":[{\"intid\":[31]}],\"matches_estimated\":1,\"matches_lower_bound\":1,\"matches_upper_bound\":1,\"size\":10}",
		    json_serialise(search_results));
    }

    // Check a search matching on a massive intid
    {
	Json::Value search_results(Json::objectValue);
	std::string search_str = "{\"query\":{\"field\":[\"intid\",\"is\",[\"18446744073709551615\"]]}}";
	s.perform_search(db, search_str, search_results);
	CHECK_EQUAL("{\"checkatleast\":0,\"from\":0,\"items\":[{\"intid\":[18446744073709551615]}],\"matches_estimated\":1,\"matches_lower_bound\":1,\"matches_upper_bound\":1,\"size\":10}",
		    json_serialise(search_results));
    }

    // Check a failing match on a massive intid
    {
	Json::Value search_results(Json::objectValue);
	std::string search_str = "{\"query\":{\"field\":[\"intid\",\"is\",[\"18446744073709551614\"]]}}";
	s.perform_search(db, search_str, search_results);
	CHECK_EQUAL("{\"checkatleast\":0,\"from\":0,\"items\":[],\"matches_estimated\":0,\"matches_lower_bound\":0,\"matches_upper_bound\":0,\"size\":10}",
		    json_serialise(search_results));
    }
}
