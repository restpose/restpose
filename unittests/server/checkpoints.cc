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

TEST(CheckPoint)
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

    log = new IndexingErrorLog(2);
    log->append_error("Error parsing something", "", "");
    log->append_error("Error processing field", "type1", "doc1");
    log->append_error("Error processing field", "type1", "doc2");
    cp.set_reached(log);
    CHECK_EQUAL("{\"errors\":["
		"{\"msg\":\"Error parsing something\"},"
		"{\"doc_id\":\"doc1\",\"doc_type\":\"type1\",\"msg\":\"Error processing field\"}"
		"],\"reached\":true,\"total_errors\":3}",
		json_serialise(cp.get_state(tmp)));
}

TEST(CheckPoints)
{
    Json::Value tmp;
    CheckPoints cps;

    CHECK_EQUAL("[]", json_serialise(cps.ids_to_json(tmp)));
    CHECK_EQUAL("null", json_serialise(cps.get_state("unknown", tmp)));

    string checkid = "checkid";
    cps.publish_checkpoint(checkid);
    CHECK_EQUAL("[\"" + checkid + "\"]",
		json_serialise(cps.ids_to_json(tmp)));

    // Test getting the state of an unfinished checkpoint
    CHECK_EQUAL("{\"reached\":false}",
		json_serialise(cps.get_state(checkid, tmp)));
    CHECK_EQUAL("[\"" + checkid + "\"]",
		json_serialise(cps.ids_to_json(tmp)));
    CHECK_EQUAL("null", json_serialise(cps.get_state("unknown", tmp)));

    // Test a finished checkpoint
    cps.set_reached(checkid, NULL);
    CHECK_EQUAL("{\"errors\":[],\"reached\":true,\"total_errors\":0}",
		json_serialise(cps.get_state(checkid, tmp)));
    CHECK_EQUAL("[\"" + checkid + "\"]",
		json_serialise(cps.ids_to_json(tmp)));
    CHECK_EQUAL("null", json_serialise(cps.get_state("unknown", tmp)));

    // Test a finished checkpoint with errors.
    IndexingErrorLog * log = new IndexingErrorLog(2);
    log->append_error("Error parsing something", "", "");
    log->append_error("Error processing field", "type1", "doc1");
    log->append_error("Error processing field", "type1", "doc2");
    cps.set_reached(checkid, log);

    // Test expiry of checkpoints.
    // The test should never take as long as this, so this shouldn't expire anything.
    cps.expire(1000);
    CHECK_EQUAL("{\"errors\":["
		"{\"msg\":\"Error parsing something\"},"
		"{\"doc_id\":\"doc1\",\"doc_type\":\"type1\",\"msg\":\"Error processing field\"}"
		"],\"reached\":true,\"total_errors\":3}",
		json_serialise(cps.get_state(checkid, tmp)));
    CHECK_EQUAL("[\"" + checkid + "\"]",
		json_serialise(cps.ids_to_json(tmp)));
    CHECK_EQUAL("null", json_serialise(cps.get_state("unknown", tmp)));

    // This should expire everything.
    cps.expire(0);
    CHECK_EQUAL("null",
		json_serialise(cps.get_state(checkid, tmp)));
    CHECK_EQUAL("[]",
		json_serialise(cps.ids_to_json(tmp)));
    CHECK_EQUAL("null", json_serialise(cps.get_state("unknown", tmp)));
}

TEST(CheckPointManager)
{
    Json::Value tmp;
    CheckPointManager man(2, 10000);

    string checkid = man.alloc_checkpoint("mycoll");
    CHECK_EQUAL(checkid.size(), size_t(36));
    CHECK_EQUAL("[]",
		json_serialise(man.ids_to_json("mycoll", tmp)));
    CHECK_EQUAL("null",
		json_serialise(man.get_state("mycoll", checkid, tmp)));

    // Check that publishing makes a checkpoint visible, but only in the right
    // collection.
    man.publish_checkpoint("mycoll", checkid);
    CHECK_EQUAL("[\"" + checkid + "\"]",
		json_serialise(man.ids_to_json("mycoll", tmp)));
    CHECK_EQUAL("[]",
		json_serialise(man.ids_to_json("othercoll", tmp)));
    CHECK_EQUAL("{\"reached\":false}",
		json_serialise(man.get_state("mycoll", checkid, tmp)));
    CHECK_EQUAL("null",
		json_serialise(man.get_state("othercoll", checkid, tmp)));

    // Check that appending an error doesn't have any effect (before the
    // checkpoint is reached)
    man.append_error("mycoll", "Error processing field", "type1", "doc1");
    CHECK_EQUAL("[\"" + checkid + "\"]",
		json_serialise(man.ids_to_json("mycoll", tmp)));
    CHECK_EQUAL("[]",
		json_serialise(man.ids_to_json("othercoll", tmp)));
    CHECK_EQUAL("{\"reached\":false}",
		json_serialise(man.get_state("mycoll", checkid, tmp)));
    CHECK_EQUAL("null",
		json_serialise(man.get_state("othercoll", checkid, tmp)));

    // Check that the appropriate thing happens when a checkpoint is reached.
    man.set_reached("mycoll", checkid);
    CHECK_EQUAL("[\"" + checkid + "\"]",
		json_serialise(man.ids_to_json("mycoll", tmp)));
    CHECK_EQUAL("[]",
		json_serialise(man.ids_to_json("othercoll", tmp)));
    CHECK_EQUAL("{\"errors\":["
		"{\"doc_id\":\"doc1\",\"doc_type\":\"type1\",\"msg\":\"Error processing field\"}"
		"],\"reached\":true,\"total_errors\":1}",
		json_serialise(man.get_state("mycoll", checkid, tmp)));
    CHECK_EQUAL("null",
		json_serialise(man.get_state("othercoll", checkid, tmp)));



    // A manager which expires checkpoints immediately.
    CheckPointManager man2(2, 0.0);
    checkid = man2.alloc_checkpoint("mycoll");
    man2.publish_checkpoint("mycoll", checkid);
    CHECK_EQUAL("[]",
		json_serialise(man2.ids_to_json("mycoll", tmp)));
}
