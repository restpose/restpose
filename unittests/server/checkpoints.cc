/** @file checkpoints.cc
 * @brief Tests for checkpoints
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
#include <json/json.h>
#include "server/checkpoints.h"
#include "UnitTest++.h"
#include "utils/jsonutils.h"

using namespace RestPose;
using namespace std;

TEST(CheckPoints)
{
    Json::Value tmp;
    CheckPoint cp;

    CHECK_EQUAL("{\"reached\":false}",
		json_serialise(cp.get_state(tmp)));

    cp.set_reached(NULL);
    CHECK_EQUAL("{\"errors\":[],\"reached\":true,\"total_errors\":0}",
		json_serialise(cp.get_state(tmp)));

    IndexingErrorLog * log = new IndexingErrorLog(10);
    cp.set_reached(log);
    CHECK_EQUAL("{\"errors\":[],\"reached\":true,\"total_errors\":0}",
		json_serialise(cp.get_state(tmp)));
}
