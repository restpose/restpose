/** @file io_wrappers.h
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

#ifndef XAPSRV_INCLUDED_IO_WRAPPERS_H
#define XAPSRV_INCLUDED_IO_WRAPPERS_H

#include <string>
#include <unistd.h>

/** Open a file for appending, creating it if not present.
 *
 *  @param filename The filename to open.
 *  @param truncate true iff any existing file should be truncated.
 *
 *  @returns the file descriptor, or -1 if unable to open.  Errno will be set
 *  if -1 is returned.
 */
int io_open_append_create(const char * filename, bool truncate);

/** Open a file for reading.
 *
 *  @param filename The filename to open.
 *
 *  @returns the file descriptor, or -1 if unable to open.  Errno will be set
 *  if -1 is returned.
 */
int io_open_read(const char * filename);

/** Write to a file descriptor.
 *
 *  Guarantees to write all bytes, unless an error occurs.  Handles interrupts
 *  due to signals.
 *
 *  @param fd The file descriptor to write to.
 *  @param data The data to write.
 *  @param len The length (in bytes) of the data to write.
 *
 *  @returns true if written successfully, false otherwise.  Errno will be set
 *  if false is returned.
 */
bool io_write(int fd, const char * data, ssize_t len);

/** Write a byte to a file descriptor.
 *
 *  Guarantees to write the byte, unless an error occurs.  Handles interrupts
 *  due to signals.
 *
 *  @param fd The file descriptor to write to.
 *  @param byte The byte to write.
 *
 *  @returns true if written successfully, false otherwise.  Errno will be set
 *  if false is returned.
 */
bool io_write_byte(int fd, char byte);

/** Write to a file descriptor.
 *
 *  @param fd The file descriptor to write to.
 *  @param data The data to write.
 *
 *  @returns true if written successfully, false otherwise.  Errno will be set
 *  if false is returned.
 */
inline bool io_write(int fd, const std::string & data)
{
    return io_write(fd, data.data(), data.size());
}

/** Write some bytes to a file descriptor.
 *
 *  @param fd The file descriptor to write to.
 *  @param data The data to write.
 *  @param len The length (in bytes) of the data to write.
 *
 *  @returns the number of bytes written, or -1 on error.
 */
int io_write_some(int fd, const char * data, ssize_t len);

/** Write some bytes to a file descriptor.
 *
 *  @param fd The file descriptor to write to.
 *  @param data The data to write.
 *  @param len The length (in bytes) of the data to write.
 *
 *  @returns the number of bytes written, or -1 on error.
 */
inline int io_write_some(int fd, const std::string & data)
{
    return io_write_some(fd, data.data(), data.size());
}


/** Write a byte to a socket.
 *
 *  Guarantees to write the byte, unless an error occurs.  Handles interrupts
 *  due to signals.
 *
 *  @param fd The file descriptor to write to.
 *  @param byte The byte to write.
 *
 *  @returns true if written successfully, false otherwise.  Errno will be set
 *  if false is returned.
 */
bool io_send_byte(int fd, char byte);


/** Close a file descriptor.
 *
 *  @returns true if closed successfully, false otherwise.  Errno will be set
 *  if false is returned.
 */
bool io_close(int fd);

/** Close a socket.
 *
 *  @returns true if closed successfully, false otherwise.  Errno will be set
 *  if false is returned.
 */
bool io_close_socket(int fd);


/** Read an exact number of bytes, blocking until all the bytes are read.
 *
 *  @param result A string which will be set to contain the bytes which have
 *  been read.  If this is shorter than the number of bytes requested, EOF
 *  was encountered.
 *  @param fd The file descriptor to read from.
 *  @param to_read The number of bytes to read.
 *
 *  @returns true if read without error (though possibly having been unable
 *  to read all bytes requested due to EOF), false otherwise.  Errno will be
 *  set if false is returned.
 */
bool io_read_exact(std::string & result, int fd, size_t to_read);

/** Read some bytes, and append to a string.
 *
 *  @param result A string to which the bytes which have been read will be
 *  appended.
 *  @param fd The file descriptor to read from.
 *  @param max_to_read The maxium number of bytes to read.
 *
 *  @return the number of bytes read if read without error (possibly 0 if
 *  having been unable to read all bytes requested due to EOF), or -1 on error.
 *  Errno will be set if -1 is returned.
 */
int io_read_append(std::string & result, int fd, size_t max_to_read);

/** Read some bytes, and append to a string.
 *
 *  This will read a chunk of data from the file descriptor, but will not
 *  exhaustively read until EOF.
 *
 *  @param result A string to which the bytes which have been read will be
 *  appended.
 *  @param fd The file descriptor to read from.
 *
 *  @returns true if read without error (though possibly having reached EOF),
 *  false otherwise.  Errno will be set if false is returned.
 */
bool io_read_append(std::string & result, int fd);


/** Read some bytes from a socket, and append to a string.
 *
 *  This will read a chunk of data from the socket, but will not exhaustively
 *  read until EOF.
 *
 *  @param result A string to which the bytes which have been read will be
 *  appended.
 *  @param fd The file descriptor to read from.
 *
 *  @returns true if read without error (though possibly having reached EOF),
 *  false otherwise.  Errno will be set if false is returned.
 */
bool io_recv_append(std::string & result, int fd);

#endif /* XAPSRV_INCLUDED_IO_WRAPPERS_H */
