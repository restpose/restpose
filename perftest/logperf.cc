/** @file logperf.cc
 * @brief Performance test for logging.
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
#include "logger/logger.h"

#include "realtime.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace RestPose;

int main(int argc, const char ** argv) {
    (void) argc;
    (void) argv;
    int fds[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fds);
    if (ret == -1) {
	fprintf(stderr, "Couldn't create internal socketpair: %d", errno);
    }

    //Logger logger(fds[0]);
    Logger logger(1);

    double start(RealTime::now());
    for (int i = 0; i != 1000; ++i) {
	logger.info("test");
    }
    logger.start();
    for (int i = 0; i != 10; ++i) {
	logger.info("test");
    }
    logger.stop();
    logger.join();
    double end(RealTime::now());
    printf("Processed in %f seconds\n", end - start);

    return 0;
}
