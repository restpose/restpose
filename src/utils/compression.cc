/*
 * Copyright 2007,2008,2009,2010 Olly Betts
 * Copyright 2008 Lemur Consulting Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <config.h>
#include <memory>
#include <cstdio>

#include "compression.h"
#include "str.h"
#include "rsperrors.h"

void
ZlibInflater::make_inflate_zstream()
{
    std::auto_ptr<z_stream> inflate_zstream(new z_stream);

    inflate_zstream->zalloc = reinterpret_cast<alloc_func>(0);
    inflate_zstream->zfree = reinterpret_cast<free_func>(0);

    inflate_zstream->next_in = Z_NULL;
    inflate_zstream->avail_in = 0;

    int err = inflateInit2(inflate_zstream.get(), 15);
    if (rare(err != Z_OK)) {
	if (err == Z_MEM_ERROR) {
	    throw std::bad_alloc();
	}
	std::string msg = "inflateInit2 failed (";
	if (inflate_zstream->msg) {
	    msg += inflate_zstream->msg;
	} else {
	    msg += str(err);
	}
	msg += ')';
	throw RestPose::ImporterError(msg);
    }
    stream = inflate_zstream.release();
}

std::string
ZlibInflater::inflate(const char * data, size_t data_len)
{
    if (stream) {
	delete stream;
	stream = NULL;
    }
    make_inflate_zstream();
    std::string uncompressed;
    stream->next_in = (Bytef*)const_cast<char *>(data);
    stream->avail_in = (uInt)data_len;
    Bytef buf[8192];
    int err = Z_OK;
    while (err != Z_STREAM_END) {
	stream->next_out = buf;
	stream->avail_out = (uInt)sizeof(buf);
	err = ::inflate(stream, Z_SYNC_FLUSH);

	if (err != Z_OK && err != Z_STREAM_END) {
	    if (err == Z_MEM_ERROR) throw std::bad_alloc();
	    std::string msg = "inflate failed";
	    if (stream->msg) {
		msg += " (";
		msg += stream->msg;
		msg += ')';
	    }
	    throw RestPose::ImporterError(msg);
	}
	uncompressed.append(reinterpret_cast<const char *>(buf),
			    stream->next_out - buf);
    }
    if (uncompressed.size() != stream->total_out) {
	std::string msg = "compressed tag didn't expand to the expected size: ";
	msg += str(uncompressed.size());
	msg += " != ";
	// OpenBSD's zlib.h uses off_t instead of uLong for total_out.
	msg += str((size_t)stream->total_out);
	throw RestPose::ImporterError(msg);
    }
    return uncompressed;
}
