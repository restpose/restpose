/* @file serialise.h
 * @brief functions to convert classes to strings and back
 *
 * Copyright (C) 2006,2007,2008,2009 Olly Betts
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

#ifndef XAPIANCOMMON_INCLUDED_SERIALISE_H
#define XAPIANCOMMON_INCLUDED_SERIALISE_H

#include <string>

// Forward class declarations:

/** Encode a length as a variable-length string.
 *
 *  The encoding specifies its own length.
 *
 *  @param len	The length to encode.
 *
 *  @return	The encoded length.
 */
template<class T>
std::string
encode_length(T len)
{
    std::string result;
    if (len < 255) {
	result += static_cast<unsigned char>(len);
    } else {
	result += '\xff';
	len -= 255;
	while (true) {
	    unsigned char byte = static_cast<unsigned char>(len & 0x7f);
	    len >>= 7;
	    if (!len) {
		result += (byte | static_cast<unsigned char>(0x80));
		break;
	    }
	    result += byte;
	}
    }
    return result;
}

/** Decode a length encoded by encode_length.
 *
 *  @param p	Pointer to a pointer to the string, which will be advanced past
 *		the encoded length.
 *  @param end	Pointer to the end of the string.
 *  @param check_remaining	Check the result against the amount of data
 *				remaining after the length has been decoded.
 *
 *  @return	The decoded length.
 */
size_t decode_length(const char ** p, const char *end, bool check_remaining);

#endif
