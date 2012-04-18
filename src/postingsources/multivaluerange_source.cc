/** @file multivaluerange_source.cc
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

#include <config.h>
#include "multivaluerange_source.h"
#include "serialise.h"
#include "str.h"
#include "utils/stringutils.h"

using namespace RestPose;
using namespace std;

MultiValueRangeSource::MultiValueRangeSource(Xapian::valueno slot_,
					     Xapian::weight wt_,
					     const string & start_val_,
					     const string & end_val_)
	: slot(slot_),
	  wt(wt_),
	  start_val(start_val_),
	  end_val(end_val_)
{
}

Xapian::docid
MultiValueRangeSource::get_docid() const
{
    return it.get_docid();
}

void
MultiValueRangeSource::next(Xapian::weight min_wt)
{
    if (!started) {
	it = db.valuestream_begin(slot);
	started = true;
    } else {
	++it;
    }
    if (min_wt > wt) {
	it = db.valuestream_end(slot);
	return;
    }
    while (it != db.valuestream_end(slot)) {
	if (check_range(*it)) {
	    return;
	}
	++it;
    }
}

void
MultiValueRangeSource::skip_to(Xapian::docid did, Xapian::weight min_wt)
{
    if (!started) {
	it = db.valuestream_begin(slot);
	started = true;
	if (it == db.valuestream_end(slot))
	    return;
    }
    if (min_wt > wt) {
	it = db.valuestream_end(slot);
	return;
    }
    it.skip_to(did);
    while (it != db.valuestream_end(slot)) {
	if (check_range(*it)) {
	    return;
	}
	++it;
    }
}

bool
MultiValueRangeSource::check(Xapian::docid did, Xapian::weight min_wt)
{
    if (!started) {
	it = db.valuestream_begin(slot);
	started = true;
	if (it == db.valuestream_end(slot))
	    return true;
    }

    if (min_wt > wt) {
	it = db.valuestream_end(slot);
	return true;
    }

    if (!it.check(did))
	return false;
    return check_range(*it);
}

bool
MultiValueRangeSource::at_end() const
{
    return started && it == db.valuestream_end(slot);
}

Xapian::PostingSource *
MultiValueRangeSource::clone() const
{
    return new MultiValueRangeSource(slot, wt, start_val, end_val);
}

string
MultiValueRangeSource::name() const
{
    return "MultiValueRangeSource";
}

string
MultiValueRangeSource::serialise() const
{
    return encode_length(slot) +
	    encode_length(start_val.size()) + start_val +
	    encode_length(end_val.size()) + end_val +
	    Xapian::sortable_serialise(wt);
}

Xapian::PostingSource *
MultiValueRangeSource::unserialise(const string &s) const
{
    const char * p = s.data();
    const char * end = p + s.size();

    Xapian::valueno new_slot = rsp_decode_length(&p, end, false);
    size_t start_len = rsp_decode_length(&p, end, true);
    string new_start_val(p, start_len);
    p += start_len;
    size_t end_len = rsp_decode_length(&p, end, true);
    string new_end_val(p, end_len);
    p += end_len;
    Xapian::weight new_wt = Xapian::sortable_unserialise(string(p, end - p));
    if (p != end) {
	throw Xapian::NetworkError("Bad serialised MultiValueRangeSource");
    }

    return new MultiValueRangeSource(new_slot, new_wt, new_start_val, new_end_val);
}

void
MultiValueRangeSource::init(const Xapian::Database & db_)
{
    db = db_;
    started = false;
    set_maxweight(wt);
    termfreq_max = db.get_value_freq(slot);
    termfreq_min = 0;

    // Note - could improve estimate based on how much of the range of slot
    // values is covered by the range.
    termfreq_est = termfreq_max / 2.0;
}

string
MultiValueRangeSource::get_description() const
{
    return string("MultiValueRangeSource(") +
	    str(slot) + ", " +
	    str(wt) + ", " +
	    hexesc(start_val) + ", " +
	    hexesc(end_val) + ")";
}

bool
MultiValueRangeSource::check_range(const std::string & value) const
{
    const char * pos = value.data();
    const char * endpos = pos + value.size();
    while (pos != endpos) {
	size_t len = rsp_decode_length(&pos, endpos, true);
	string val(pos, len);

	if (start_val <= val && val <= end_val)
	    return true;
	pos += len;
    }
    return false;
}
