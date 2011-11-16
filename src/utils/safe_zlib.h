/** @file safe_zlib.h
 * @brief Import zlib, with some platform fixes.
 */
/* Copyright 2011 Richard Boulton
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

#ifndef RESTPOSE_INCLUDED_SAFE_ZLIB_H
#define RESTPOSE_INCLUDED_SAFE_ZLIB_H

#ifdef __WIN32__
   typedef __int64 int64_t;
#else
   #include <stdint.h>
#endif

/** zlib 1.2.4 and 1.2.5 cause compiler warnings in some environments (observed
 *  on OSX), due to constructs of the form "FOO-0", where FOO is not defined.
 *
 *  To avoid the warning, we ensure that the relevant identifiers are defined,
 *  while we import zlib.
 */

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 0
#define RESTPOSE_LARGEFILE64_SOURCE 1
#endif
#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE 0
#define RESTPOSE_LFS64_LARGEFILE  1
#endif
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#define RESTPOSE_FILE_OFFSET_BITS 1
#endif
#ifndef off64_t
#define off64_t int64_t
#define RESTPOSE_OFF64_T 1
#endif

#include <zlib.h>

/** Clean up again, in case some other library is using #if defined()
 */
#ifdef RESTPOSE_LARGEFILE64_SOURCE
#undef RESTPOSE_LARGEFILE64_SOURCE
#undef _LARGEFILE64_SOURCE
#endif

#ifdef RESTPOSE_LFS64_LARGEFILE
#undef RESTPOSE_LFS64_LARGEFILE
#undef _LFS64_LARGEFILE
#endif

#ifdef RESTPOSE_FILE_OFFSET_BITS
#undef RESTPOSE_FILE_OFFSET_BITS
#undef _FILE_OFFSET_BITS
#endif

#ifdef RESTPOSE_OFF64_T
#undef RESTPOSE_OFF64_T
#undef off64_t
#endif

#endif /* RESTPOSE_INCLUDED_SAFE_ZLIB_H */
