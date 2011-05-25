/** @file io_wrappers.cc
 * @brief Convenient wrappers around unix IO system calls.
 */
/* Copyright 2010 Richard Boulton
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
#include "io_wrappers.h"

#include <errno.h>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int
io_open_append_create(const char * filename, bool truncate)
{
    int flags = O_WRONLY | O_CREAT | O_APPEND;
    if (truncate) {
	flags |= O_TRUNC;
    }
    int fd;
    do {
	fd = open(filename, flags, 0666);
	// Repeat open if we got interrupted by a signal
	// (probably only possible if filename is a FIFO).
    } while (fd == -1 && errno == EINTR);
    return fd;
}

int
io_open_read(const char * filename)
{
    int fd;
    do {
	fd = open(filename, O_RDONLY);
	// Repeat open if we got interrupted by a signal
	// (probably only possible if filename is a FIFO).
    } while (fd == -1 && errno == EINTR);
    return fd;
}

bool
io_write(int fd, const char * data, ssize_t len)
{
    while (len) {
	ssize_t c = write(fd, data, len);
	if (c < 0) {
	    if (errno == EINTR) continue;
	    return false;
	}
	data += c;
	len -= c;
    }
    return true;
}

bool
io_write_byte(int fd, char byte)
{
    char buf[1];
    buf[0] = byte;
    return io_write(fd, buf, 1);
}

int
io_write_some(int fd, const char * data, ssize_t len)
{
    if (len == 0) return 0;
    while (true) {
	ssize_t c = write(fd, data, len);
	if (c < 0) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	return c;
    }
}

bool
io_close(int fd)
{
    int ret;
    do {
	ret = close(fd);
    } while(ret == -1 && errno == EINTR);
    return (ret != -1);
}

#define CHUNKSIZE 4096

bool
io_read_exact(std::string & result, int fd, size_t to_read)
{
    size_t remaining_to_read = to_read;
    result.clear();
    while (remaining_to_read > 0) {
	char buf[CHUNKSIZE];
	size_t count = std::min(remaining_to_read, sizeof(buf));
	ssize_t bytes_read = read(fd, buf, count);

	if (bytes_read == 0) {
	    // EOF - return what we've got.
	    return true;
	}

	if (bytes_read > 0) {
	    result.append(buf, bytes_read);
	    if (result.size() >= to_read) return true;
	    remaining_to_read -= bytes_read;
	    continue;
	}

	if (errno != EINTR) return false;
    }
    return true;
}

int
io_read_append(std::string & result, int fd, size_t max_to_read)
{
    while (true) {
	char buf[max_to_read];
	ssize_t bytes_read = read(fd, buf, max_to_read);

	if (bytes_read == 0) {
	    return bytes_read;
	} else if (bytes_read > 0) {
	    result.append(buf, bytes_read);
	    return bytes_read;
	} else {
	    if (errno != EINTR) return -1;
	}
    }
}

bool
io_read_append(std::string & result, int fd)
{
    return io_read_append(result, fd, CHUNKSIZE);
}
