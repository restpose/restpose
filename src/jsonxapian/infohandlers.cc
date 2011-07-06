/** @file infohandlers.cc
 * @brief Information handlers, for gathering information about a search.
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
#include "jsonxapian/infohandlers.h"

#include "jsonxapian/occurinfohandler.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

using namespace std;
using namespace RestPose;

InfoHandler::~InfoHandler()
{}

InfoHandlers::InfoHandlers()
{}

InfoHandlers::~InfoHandlers()
{
    for (vector<InfoHandler *>::iterator i = handlers.begin();
	 i != handlers.end(); ++i) {
	delete *i;
    }
}

void
InfoHandlers::write_results(Json::Value & results,
			    const Xapian::MSet & mset) const
{
    for (vector<InfoHandler *>::const_iterator i = handlers.begin();
	 i != handlers.end(); ++i) {
	if (*i != NULL) {
	    (*i)->write_results(results, mset);
	}
    }
}

void
InfoHandlers::add_handler(const Json::Value & handler,
			  Xapian::Enquire & enq,
			  const Xapian::Database * db,
			  const Schema * schema,
			  Xapian::doccount & check_at_least)
{
    json_check_object(handler, "search info item to gather");
    if (handler.size() != 1) {
	throw InvalidValueError("info item must have exactly one member");
    }
    if (handlers.empty() || handlers.back() != NULL) {
	handlers.push_back(NULL);
    }
    if (handler.isMember("occur")) {
	InfoHandler * tmp = new OccurInfoHandler(handler["occur"], enq, db, schema, check_at_least);
	handlers.back() = tmp;
    }
    if (handler.isMember("cooccur")) {
	handlers.back() = new CoOccurInfoHandler(handler["cooccur"], enq, db, schema, check_at_least);
    }
#if 0
    if (handler.isMember("facet")) {
	handlers.back() = new FacetInfoHandler(handler["facet"], enq, db, schema)
    }
#endif
}
