/** @file facetmatchspy.h
 * @brief MatchSpy classes for faceting.
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

#ifndef RESTPOSE_INCLUDED_FACETMATCHSPY_H
#define RESTPOSE_INCLUDED_FACETMATCHSPY_H

#include <json/value.h>
#include "jsonxapian/docvalues.h"
#include <map>
#include <set>
#include <string>
#include <xapian.h>

namespace RestPose {

class BaseFacetMatchSpy : public Xapian::MatchSpy {
  protected:
    /** Slot to read facets from.
     */
    SlotDecoder * decoder;

    /** Name of field to read facets from.
     *
     *  If this was not supplied, it will be empty.
     */
    std::string fieldname;

    /** Number of documents seen by the matchspy.
     */
    Xapian::doccount docs_seen;

    /** Limit on the number of documents considered by matchspy.
     */
    Xapian::doccount doc_limit;

    /** Number of values seen by the matchspy (ie, sum of number of values in
     *  each document).
     */
    Xapian::termcount values_seen;

  public:
    BaseFacetMatchSpy(SlotDecoder * decoder_,
		      const std::string & fieldname_,
		      Xapian::doccount doc_limit_);

    virtual ~BaseFacetMatchSpy();

    virtual void operator()(const Xapian::Document &doc, Xapian::weight wt) = 0;

    virtual void get_result(Json::Value & result) const = 0;
};

class FacetCountMatchSpy : public BaseFacetMatchSpy {
  protected:
    /** Maximum number of resulting facet values to return.
     */
    Xapian::doccount result_limit;

    /** Count of number of times each facet value has been seen;
     */
    std::map<std::string, Xapian::doccount> counts;

    /** Append a value to the result count array.
     */
    virtual void append_value(Json::Value & rcounts,
			      const std::string & str,
			      Xapian::doccount freq) const;

  public:
    FacetCountMatchSpy(SlotDecoder * decoder_,
		       const std::string & fieldname_,
		       Xapian::doccount doc_limit_,
		       Xapian::doccount result_limit_)
	    : BaseFacetMatchSpy(decoder_, fieldname_, doc_limit_),
	      result_limit(result_limit_),
	      counts()
    {}

    void operator()(const Xapian::Document &doc, Xapian::weight wt);

    void get_result(Json::Value & result) const;
};

class DateFacetCountMatchSpy : public FacetCountMatchSpy {
    void append_value(Json::Value & rcounts,
		      const std::string & str,
		      Xapian::doccount freq) const;

  public:
    DateFacetCountMatchSpy(SlotDecoder * decoder_,
			   const std::string & fieldname_,
			   Xapian::doccount doc_limit_,
			   Xapian::doccount result_limit_)
	    : FacetCountMatchSpy(decoder_, fieldname_, doc_limit_,
				 result_limit_)
    {}

};

}

#endif /* RESTPOSE_INCLUDED_FACETMATCHSPY_H */
