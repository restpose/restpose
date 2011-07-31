/** @file multivaluerange_source.h
 * @brief PostingSource for searching for ranges in multivalued slots
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

#ifndef RESTPOSE_INCLUDED_MULTIVALUERANGE_SOURCE_H
#define RESTPOSE_INCLUDED_MULTIVALUERANGE_SOURCE_H

#include <xapian.h>

namespace RestPose {

    class MultiValueRangeSource : public Xapian::PostingSource {
	Xapian::Database db;
	Xapian::valueno slot;
	Xapian::ValueIterator it;
	bool started;
	Xapian::doccount termfreq_min;
	Xapian::doccount termfreq_est;
	Xapian::doccount termfreq_max;
	Xapian::weight wt;
	std::string start_val;
	std::string end_val;
      public:
	MultiValueRangeSource(Xapian::valueno slot_,
			      Xapian::weight wt_,
			      const std::string & start_val_,
			      const std::string & end_val_);

	Xapian::doccount get_termfreq_min() const {
	    return termfreq_min;
	}
	Xapian::doccount get_termfreq_est() const {
	    return termfreq_est;
	}
	Xapian::doccount get_termfreq_max() const {
	    return termfreq_max;
	}
	Xapian::weight get_weight() const {
	    return wt;
	}
	Xapian::docid get_docid() const;
	void next(Xapian::weight min_wt);
	void skip_to(Xapian::docid did, Xapian::weight min_wt);
	bool check(Xapian::docid did, Xapian::weight min_wt);
	bool at_end() const;
	Xapian::PostingSource * clone() const;
	std::string name() const;
	std::string serialise() const;
	Xapian::PostingSource * unserialise(const std::string &s) const;
	void init(const Xapian::Database & db);
	std::string get_description() const;

	/** Check if a the value matches the range.
	 */
	bool check_range(const std::string & value) const;
    };

}

#endif /* RESTPOSE_INCLUDED_MULTIVALUERANGE_SOURCE_H */
