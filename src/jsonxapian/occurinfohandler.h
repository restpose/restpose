/** @file occurinfohandler.h
 * @brief Information handler for counting occurrences of terms.
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

#ifndef RESTPOSE_INCLUDED_OCCURINFOHANDLER_H
#define RESTPOSE_INCLUDED_OCCURINFOHANDLER_H

#include <xapian.h>
#include <json/value.h>
#include "jsonxapian/infohandlers.h"
#include <string>

namespace RestPose {

class BaseTermOccurMatchSpy;

class BaseOccurInfoHandler : public InfoHandler {
  protected:
    const Xapian::Database * db;
    BaseTermOccurMatchSpy * spy;

  public:
    BaseOccurInfoHandler(const Xapian::Database * db_)
	    : db(db_), spy(NULL)
    {}
    virtual ~BaseOccurInfoHandler();

    void write_results(Json::Value & results,
		       const Xapian::MSet & mset) const;
};

class OccurInfoHandler : public BaseOccurInfoHandler {
  public:
    OccurInfoHandler(const Json::Value & params,
		     Xapian::Enquire & enq,
		     const Xapian::Database * db_,
		     const Schema * schema,
		     Xapian::doccount & checkatleast);

};

class CoOccurInfoHandler : public BaseOccurInfoHandler {
  public:
    CoOccurInfoHandler(const Json::Value & params,
		       Xapian::Enquire & enq,
		       const Xapian::Database * db_,
		       const Schema * schema,
		       Xapian::doccount & checkatleast);
};

}

#endif /* RESTPOSE_INCLUDED_OCCURINFOHANDLER_H */
