/** @file multivalue_keymaker.cc
 * @brief KeyMaker for sorting by multivalued slots
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
#include "postingsources/multivalue_keymaker.h"

#include <cstring>
#include "jsonxapian/docvalues.h"
#include <memory>
#include "serialise.h"
#include <string>
#include <xapian.h>

using namespace RestPose;
using namespace std;

void
MultiValueKeyMaker::add_decoder(SlotDecoder * decoder, bool reverse)
{
    auto_ptr<SlotDecoder> decoder_ptr(decoder);
    decoders.push_back(std::pair<SlotDecoder *, bool>(NULL, reverse));
    decoders[decoders.size() - 1].first = decoder_ptr.release();
}

string
MultiValueKeyMaker::operator()(const Xapian::Document & doc) const
{
    const char * begin;
    string::size_type len;
    string result;
    for (std::vector<std::pair<SlotDecoder *, bool> >::const_iterator
	 i = decoders.begin(); i != decoders.end(); ++i) {
	i->first->newdoc(doc);
	if (i->first->next(&begin, &len)) {
	    if (i->second) {
		// Reverse sort order.
		// Subtract all bytes from \xff, except for zero bytes which
		// are converted to \xff\0.  Terminate with \xff\xff
		const char * end = begin + len;
		while (begin != end) {
		    unsigned char ch(*begin);
		    result += char(255 - ch);
		    if (ch == 0) {
			result += char(0);
		    }
		    ++begin;
		}
		result.append(2, '\xff');
	    } else {
		// Forward sort order.
		// Convert zero bytes to \0\xff.  Terminate with \0\0.
		while (len > 0) {
		    const char * zero =
			    static_cast<const char *>(memchr(begin, 0, len));
		    if (zero == NULL) {
			result.append(string(begin, len));
			break;
		    }
		    if (zero != begin) {
			result.append(string(begin, zero - begin));
			begin = zero;
			len -= (zero - begin);
		    }
		    result += char(0);
		}
		result.append(2, '\0');
	    }
	}
    }
    return result;
}
