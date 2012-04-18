/** @file serialise.cc
 * @brief functions to convert Xapian objects to strings and back
 */
/* Copyright (C) 2006,2007,2008,2009,2010 Olly Betts
 * Copyright (C) 2010 Richard Boulton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "serialise.h"
#include "utils/rsperrors.h"

using namespace std;

size_t
rsp_decode_length(const char ** p, const char *end, bool check_remaining)
{
    if (*p == end) {
	throw RestPose::UnserialisationError("Bad encoded length: no data");
    }

    size_t len = static_cast<unsigned char>(*(*p)++);
    if (len == 0xff) {
	len = 0;
	unsigned char ch;
	int shift = 0;
	do {
	    if (*p == end || shift > 28)
		throw RestPose::UnserialisationError("Bad encoded length: insufficient data");
	    ch = *(*p)++;
	    len |= size_t(ch & 0x7f) << shift;
	    shift += 7;
	} while ((ch & 0x80) == 0);
	len += 255;
    }
    if (check_remaining && len > size_t(end - *p)) {
	throw RestPose::UnserialisationError("Bad encoded length: length greater than data");
    }
    return len;
}
