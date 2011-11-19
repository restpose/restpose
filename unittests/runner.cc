/** @file runner.cc
 * @brief Unittest runner.
 */
/* Copyright (c) 2010 Richard Boulton
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

#include <cstdio>
#include "logger/logger.h"
#include <pthread.h>
#include "TestReporterStdout.h"
#include "UnitTest++.h"

#ifdef WIN32
#include <winsock2.h>
#endif

#ifdef PTW32_STATIC_LIB
extern "C" {
int ptw32_processInitialize();
};
#endif

int main(int, char const **)
{
#ifdef PTW32_STATIC_LIB
    ptw32_processInitialize();
    if (!pthread_win32_process_attach_np()) {
	fprintf(stderr, "Unable to initialise threading\n");
	return 1;
    }
#endif
#ifdef WIN32
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
	fprintf(stderr, "Unable to initalise winsock: error code %d\n", err);
	return 1;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
	fprintf(stderr, "Could not find a usable version of winsock\n");
	WSACleanup();
	return 1;
    }
#endif
    RestPose::g_log.start();
    int result = UnitTest::RunAllTests();
    RestPose::g_log.stop();
    RestPose::g_log.join();
#ifdef WIN32
    WSACleanup();
#endif
#ifdef PTW32_STATIC_LIB
    pthread_win32_process_detach_np();
#endif
    return result;
}
