/** @file pipe.cc
 * @brief Tests for Pipes
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
#include "UnitTest++.h"
#include "jsonxapian/pipe.h"
#include "utils/rsperrors.h"
#include "utils/jsonutils.h"

using namespace RestPose;

TEST(Pipes)
{
    Pipe p;
    Json::Value tmp;

    // Check initial configuration of a pipe.
    p.to_json(tmp);
    CHECK_EQUAL("{}", json_serialise(tmp));
    CHECK_EQUAL(0u, p.mappings.size());
    CHECK_EQUAL(false, p.apply_all);
    CHECK_EQUAL("", p.target);

    // Check that setting configuration to a string throws an error.
    CHECK_THROW(p.from_json(""), InvalidValueError);
    json_unserialise("\"\"", tmp);
    CHECK_THROW(p.from_json(tmp), InvalidValueError);

    // Check setting a full, complex, configuration.
    json_unserialise("{"
      "  \"mappings\": ["
      "    {"
      "      /* Apply this filter for documents with content */"
      "      \"when\": {"
      "        \"exists\": [\"document_ids\", \"content\"]"
      "      },"
      "      /* Pick out the appropriate matching fields */"
      "      \"map\": ["
      "        {\"from\": [\"document_ids\", \"content\"], \"to\": \"id\"},"
      "        {\"from\": [\"extracted_text\", \"content\"], \"to\": \"text\"},"
      "        {\"from\": [\"raw_text\", \"content\"], \"to\": \"raw_text\"}"
      "      ]"
      "    },"
      "    {"
      "      /* Apply this filter for documents with summary */"
      "      \"when\": {"
      "        \"exists\": [\"document_ids\", \"summary\"]"
      "      },"
      "      /* Pick out the appropriate part of the dict fields. */"
      "      \"map\": ["
      "        {\"from\": [\"document_ids\", \"summary\"], \"to\": \"id\"},"
      "        {\"from\": [\"extracted_text\", \"summary\"], \"to\": \"text\"},"
      "        {\"from\": [\"raw_text\", \"summary\"], \"to\": \"raw_text\"}"
      "      ]"
      "    }"
      "  ],"
      "  \"apply_all\": true,"
      "  \"target\": \"next\""
      "}", tmp);
    p.from_json(tmp);
    CHECK_EQUAL(2u, p.mappings.size());
    CHECK_EQUAL(true, p.apply_all);
    CHECK_EQUAL("next", p.target);
    p.to_json(tmp);
    CHECK_EQUAL("{"
      "\"apply_all\":true,"
      "\"mappings\":["
      "{"
      "\"map\":["
      "{\"from\":[\"document_ids\",\"content\"],\"to\":\"id\"},"
      "{\"from\":[\"extracted_text\",\"content\"],\"to\":\"text\"},"
      "{\"from\":[\"raw_text\",\"content\"],\"to\":\"raw_text\"}"
      "],"
      "\"when\":{"
      "\"exists\":[\"document_ids\",\"content\"]"
      "}"
      "},"
      "{"
      "\"map\":["
      "{\"from\":[\"document_ids\",\"summary\"],\"to\":\"id\"},"
      "{\"from\":[\"extracted_text\",\"summary\"],\"to\":\"text\"},"
      "{\"from\":[\"raw_text\",\"summary\"],\"to\":\"raw_text\"}"
      "],"
      "\"when\":{"
      "\"exists\":[\"document_ids\",\"summary\"]"
      "}"
      "}"
      "],"
      "\"target\":\"next\""
      "}", json_serialise(tmp));

    // Check setting the pipe to an empty configuration.
    json_unserialise("{}", tmp);
    p.from_json(tmp);
    p.to_json(tmp);
    CHECK_EQUAL("{}", json_serialise(tmp));
    CHECK_EQUAL(0u, p.mappings.size());
    CHECK_EQUAL(false, p.apply_all);
    CHECK_EQUAL("", p.target);
}
