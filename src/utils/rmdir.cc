/** @file rmdir.cc
 * @brief Directory removal function.
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
#include "utils/rmdir.h"

#include "diritor.h"
#include "safeerrno.h"
#include "safeunistd.h"
#include "utils/rsperrors.h"

using namespace RestPose;
using namespace std;

void
rmdir_recursive(const string & dirname)
{
    DirectoryIterator iter(false);
    iter.start(dirname);
    while (iter.next()) {
	string path = dirname + "/" + iter.leafname();
	if (iter.get_type() == iter.DIRECTORY) {
	    rmdir_recursive(path);
	    if (rmdir(path.c_str())) {
		throw SysError("rmdir(\"" + path + "\") failed", errno);
	    }
	} else {
	    if (unlink(path.c_str())) {
		throw SysError("unlink(\"" + path + "\") failed", errno);
	    }
	}
    }
}
