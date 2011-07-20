/** @file doctojson.cc
 * @brief Routines for converting documents to JSON objects.
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

#include "doctojson.h"

#include "docdata.h"
#include <json/writer.h>
#include "utils/jsonutils.h"
#include "utils/stringutils.h"

using namespace RestPose;

Json::Value &
RestPose::doc_to_json(const Xapian::Document & doc, Json::Value & result)
{
    json_check_object(result, "target for doc_to_json");
    result = Json::objectValue;

    {
	DocumentData docdata;
	docdata.unserialise(doc.get_data());
	Json::Value & dataval(result["data"]);
	dataval = Json::objectValue;
	for (DocumentData::const_iterator i = docdata.begin();
	     i != docdata.end(); ++i) {
	    json_unserialise(i->second, dataval[i->first]);
	}
	if (dataval.empty()) {
	    result.removeMember("data");
	}
    }

    {
	Json::Value & termsval(result["terms"]);
	termsval = Json::objectValue;
	for (Xapian::TermIterator i = doc.termlist_begin();
	     i != doc.termlist_end(); ++i) {
	    Json::Value termval(Json::objectValue);
	    if (i.get_wdf() != 0)
		termval["wdf"] = i.get_wdf();
	    if (i.positionlist_count() != 0) {
		Json::Value posval(Json::arrayValue);
		for (Xapian::PositionIterator j = i.positionlist_begin();
		     j != i.positionlist_end();
		     ++j) {
		    posval.append(*j);
		}
		termval["positions"] = posval;
	    }
	    termsval[hexesc(*i)] = termval;
	}
	if (termsval.empty()) {
	    result.removeMember("terms");
	}
    }

    {
	Json::Value & valuesval(result["values"]);
	valuesval = Json::objectValue;
	for (Xapian::ValueIterator i = doc.values_begin();
	     i != doc.values_end(); ++i) {
	    valuesval[Json::valueToString(Json::UInt64(i.get_valueno()))] = hexesc(*i);
	}
	if (valuesval.empty()) {
	    result.removeMember("values");
	}
    }
    return result;
}
