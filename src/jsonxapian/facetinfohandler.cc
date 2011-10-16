/** @file facetinfohandler.cc
 * @brief Information handler for performing faceting calculations.
 */
/* Copyright (c) 2011 Richard Boulton
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
#include "jsonxapian/facetinfohandler.h"

#include "jsonxapian/query_builder.h"
#include "jsonxapian/slotname.h"
#include <limits.h>
#include "logger/logger.h"
#include "matchspies/facetmatchspy.h"
#include <memory>
#include "utils/jsonutils.h"

using namespace RestPose;
using namespace std;

BaseFacetInfoHandler::~BaseFacetInfoHandler()
{
    delete spy;
}

void
BaseFacetInfoHandler::write_results(Json::Value & results,
				    const Xapian::MSet &) const
{
    if (!results.isMember("info")) {
	results["info"] = Json::arrayValue;
    }
    Json::Value & info = results["info"].append(Json::objectValue);
    spy->get_result(info);
}


FacetCountInfoHandler::FacetCountInfoHandler(const Json::Value & params,
					     const QueryBuilder & builder,
					     Xapian::Enquire & enq,
					     const Xapian::Database * db,
					     Xapian::doccount & check_at_least)
	: BaseFacetInfoHandler()
{
    Xapian::doccount doc_limit = json_get_uint64_member(params,
	"doc_limit", UINT_MAX, db->get_doccount());
    Xapian::doccount result_limit = json_get_uint64_member(params,
	"result_limit", UINT_MAX, UINT_MAX);

    string fieldname = json_get_string_member(params, "field", string());
    if (fieldname.empty()) {
	// Make a spy with no decoder, and don't add it to "enq", to get a
	// suitable structure added to the results.
	spy = new FacetCountMatchSpy(NULL, fieldname, doc_limit, result_limit);
	return;
    }

    // Set the slot from the fieldname.
    // Raises an exception if there is no single slot used for the field by
    // all types in the builder.
    auto_ptr<SlotDecoder> decoder(builder.get_slot_decoder(fieldname));
    if (decoder.get() == NULL) {
	// Make a spy with no decoder, and don't add it to "enq", to get a
	// suitable structure added to the results.
	spy = new FacetCountMatchSpy(NULL, fieldname, doc_limit, result_limit);
	return;
    }

    spy = new FacetCountMatchSpy(decoder.release(), fieldname, doc_limit,
				 result_limit);

    if (check_at_least < doc_limit) {
	check_at_least = doc_limit;
    }
    enq.add_matchspy(spy);
}
