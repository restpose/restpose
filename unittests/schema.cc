/** @file schema.cc
 * @brief Tests for Schema
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
#include "UnitTest++.h"
#include <json/json.h>
#include "jsonxapian/doctojson.h"
#include "jsonxapian/schema.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

using namespace RestPose;

TEST(SchemaParams)
{
    Schema s("");
    Json::Value tmp;
    CHECK_THROW(s.from_json(json_unserialise("", tmp)), InvalidValueError);

    // Check encoding of an empty schema.
    s.from_json(json_unserialise("{}", tmp));
    CHECK_EQUAL("{\"patterns\":[]}",
		json_serialise(s.to_json(tmp)));

    CHECK(s.get("id") == NULL);
    CHECK(s.get("url") == NULL);
    CHECK(s.get("missing") == NULL);

    s.set("id", new IDFieldConfig(""));
    CHECK_EQUAL("{\"fields\":{"
		"\"id\":{\"max_length\":64,"
		        "\"store_field\":\"\","
		        "\"too_long_action\":\"error\",\"type\":\"id\"}"
		"},\"patterns\":[]}",
		json_serialise(s.to_json(tmp)));

    s.set("url", new ExactFieldConfig("url", 120, ExactFieldConfig::TOOLONG_HASH, "url", 0));
    CHECK_EQUAL("{\"fields\":{"
		"\"id\":{\"max_length\":64,"
		        "\"store_field\":\"\","
		        "\"too_long_action\":\"error\",\"type\":\"id\"},"
		"\"url\":{\"max_length\":120,"
		         "\"prefix\":\"url\","
		         "\"store_field\":\"url\","
		         "\"too_long_action\":\"hash\","
			 "\"type\":\"exact\","
		         "\"wdfinc\":0}"
		"},\"patterns\":[]}",
		json_serialise(s.to_json(tmp)));

    CHECK(s.get("id") != NULL);
    CHECK(s.get("url") != NULL);
    CHECK(s.get("missing") == NULL);

    Schema s2("");
    Json::Value tmp2;
    CHECK(json_serialise(s2.to_json(tmp2)) != json_serialise(s.to_json(tmp)));
    s2.from_json(json_unserialise(json_serialise(s.to_json(tmp)), tmp2));
    CHECK_EQUAL(json_serialise(s2.to_json(tmp2)), json_serialise(s.to_json(tmp)));
}

TEST(SchemaToDoc)
{
    Schema s("");
    s.set("id", new IDFieldConfig(""));
    s.set("url", new ExactFieldConfig("url", 120, ExactFieldConfig::TOOLONG_HASH, "url", 0));
    s.set("nostore", new ExactFieldConfig("ns", 120, ExactFieldConfig::TOOLONG_ERROR, "", 0));

    {
	Json::Value v(Json::objectValue);
	std::string idterm;
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(doc.get_data(), "");
	Json::Value tmp;
	CHECK_EQUAL("{}", json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{}");
    }

    {
	Json::Value v(Json::objectValue);
	v["id"] = "abcd";
	v["url"] = "http://example.com/";
	std::string idterm;
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "\t\tabcd");
	CHECK_EQUAL(doc.get_data(), "\003url\027[\"http://example.com/\"]");
	Json::Value tmp;
	CHECK_EQUAL("{\"data\":{\"url\":[\"http://example.com/\"]},\"terms\":{\"\\t\\tabcd\":{},\"url\\thttp://example.com/\":{}}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"url\":[\"http://example.com/\"]}");
    }

    {
	Json::Value v(Json::objectValue);
	v["nostore"] = "abcd";
	std::string idterm;
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(doc.get_data(), "");
	Json::Value tmp;
	CHECK_EQUAL("{\"terms\":{\"ns\\tabcd\":{}}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{}");
    }
}

TEST(LongExactFields)
{
    Schema s2("");
    s2.set("id", new IDFieldConfig("")); // ID terms have a default max length of 64
    s2.set("url", new ExactFieldConfig("url", 30, ExactFieldConfig::TOOLONG_HASH, "url", 0));
    s2.set("tags", new ExactFieldConfig("tags", 30, ExactFieldConfig::TOOLONG_TRUNCATE, "tags", 0));
    s2.set("valuable", new ExactFieldConfig("valuable", 30, ExactFieldConfig::TOOLONG_ERROR, "valuable", 0));

    Schema s("");
    Json::Value tmp;
    Json::Value tmp2;
    s.from_json(s2.to_json(tmp));

    {
	Json::Value v(Json::objectValue);
	v["id"] = "01234567890123456789012345678901234567890123456789012345678901234";
	std::string idterm;
	CHECK_THROW(s.process(v, idterm), InvalidValueError);
	v["id"] = "0123456789012345678901234567890123456789012345678901234567890123";
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "\t\t0123456789012345678901234567890123456789012345678901234567890123");
	CHECK_EQUAL("{\"terms\":{\"\\t\\t0123456789012345678901234567890123456789012345678901234567890123\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{}");
    }

    {
	Json::Value v(Json::objectValue);
	v["url"] = "0123456789012345678901234567890";
	std::string idterm;
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL("{\"data\":{\"url\":[\"0123456789012345678901234567890\"]},\"terms\":{\"url\\t012345678901234567890123Y]^J/ \":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"url\":[\"0123456789012345678901234567890\"]}");
    }

    {
	Json::Value v(Json::objectValue);
	v["tags"] = "0123456789012345678901234567890";
	std::string idterm;
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL("{\"data\":{\"tags\":[\"0123456789012345678901234567890\"]},\"terms\":{\"tags\\t012345678901234567890123456789\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"tags\":[\"0123456789012345678901234567890\"]}");
    }

    {
	Json::Value v(Json::objectValue);
	v["valuable"] = "0123456789012345678901234567890";
	std::string idterm;
	CHECK_THROW(s.process(v, idterm), InvalidValueError);
	v["valuable"] = "012345678901234567890123456789";
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL("{\"data\":{\"valuable\":[\"012345678901234567890123456789\"]},\"terms\":{\"valuable\\t012345678901234567890123456789\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"valuable\":[\"012345678901234567890123456789\"]}");
    }
}

TEST(IntegerExactFields)
{
    Json::Value tmp, tmp2;
    Schema s2("");
    s2.set("intid", new ExactFieldConfig("intid", 30, ExactFieldConfig::TOOLONG_ERROR, "intid", 0));

    CHECK_EQUAL("{\"fields\":{\"intid\":{\"max_length\":30,\"prefix\":\"intid\",\"store_field\":\"intid\",\"too_long_action\":\"error\",\"type\":\"exact\",\"wdfinc\":0}},\"patterns\":[]}",
		json_serialise(s2.to_json(tmp2)));

    Schema s("");
    s.from_json(s2.to_json(tmp));
    tmp = tmp2 = Json::nullValue;
    CHECK_EQUAL(json_serialise(s.to_json(tmp)),
		json_serialise(s2.to_json(tmp2)));

    {
	Json::Value v(Json::objectValue);
	v["intid"] = Json::UInt64(5000000000LL);
	std::string idterm;
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL("{\"data\":{\"intid\":[5000000000]},\"terms\":{\"intid\\t5000000000\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"intid\":[5000000000]}");
    }

    {
	Json::Reader reader;
	Json::Value v;
	CHECK(reader.parse("{\"intid\": -1}", v, false));

	std::string idterm;
	CHECK_THROW(s.process(v, idterm), InvalidValueError);
	CHECK_EQUAL(idterm, "");

	CHECK(reader.parse("{\"intid\": 0}", v, false));
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL("{\"data\":{\"intid\":[0]},\"terms\":{\"intid\\t0\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");

	CHECK(reader.parse("{\"intid\": 1}", v, false));
	doc = s.process(v, idterm);
	CHECK_EQUAL("{\"data\":{\"intid\":[1]},\"terms\":{\"intid\\t1\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");

	CHECK(reader.parse("{\"intid\": 4294967295}", v, false)); // 2**32-1
	doc = s.process(v, idterm);
	CHECK_EQUAL("{\"data\":{\"intid\":[4294967295]},\"terms\":{\"intid\\t4294967295\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");

	CHECK(reader.parse("{\"intid\": 9223372036854775807}", v, false)); // 2**63-1
	doc = s.process(v, idterm);
	CHECK_EQUAL("{\"data\":{\"intid\":[9223372036854775807]},\"terms\":{\"intid\\t9223372036854775807\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");

	CHECK(reader.parse("{\"intid\": 9223372036854775808}", v, false)); // 2**63
	doc = s.process(v, idterm);
	CHECK_EQUAL("{\"data\":{\"intid\":[9223372036854775808]},\"terms\":{\"intid\\t9223372036854775808\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");

	CHECK(reader.parse("{\"intid\": 18446744073709551615}", v, false)); // 2**64-1
	doc = s.process(v, idterm);
	CHECK_EQUAL("{\"data\":{\"intid\":[18446744073709551615]},\"terms\":{\"intid\\t18446744073709551615\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");

	CHECK(reader.parse("{\"intid\": 18446744073709551616}", v, false));
	CHECK_THROW(s.process(v, idterm), InvalidValueError); // Too big

    }
}

TEST(DateFields)
{
    Json::Value tmp, tmp2;
    Schema s2("");
    s2.set("date", new DateFieldConfig(0, "date"));
    CHECK_EQUAL("{\"fields\":{\"date\":{\"slot\":0,\"store_field\":\"date\",\"type\":\"date\"}},\"patterns\":[]}",
		json_serialise(s2.to_json(tmp2)));

    Schema s("");
    s.from_json(s2.to_json(tmp));
    tmp = tmp2 = Json::nullValue;
    CHECK_EQUAL(json_serialise(s.to_json(tmp)),
		json_serialise(s2.to_json(tmp2)));

    {
	Json::Value v(Json::objectValue);
	v["date"] = 1283400000;
	std::string idterm;
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL("{\"data\":{\"date\":[1283400000]},\"values\":{\"0\":\"\340\\\\\307\362\\u0014\"}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"date\":[1283400000]}");
    }
}

TEST(StoreFields)
{
    Json::Value tmp, tmp2;
    Schema s2("");
    s2.set("store", new StoredFieldConfig(std::string("store")));
    CHECK_EQUAL("{\"fields\":{\"store\":{\"store_field\":\"store\",\"type\":\"stored\"}},\"patterns\":[]}",
		json_serialise(s2.to_json(tmp2)));

    Schema s("");
    s.from_json(s2.to_json(tmp));
    tmp = tmp2 = Json::nullValue;
    CHECK_EQUAL(json_serialise(s.to_json(tmp)),
		json_serialise(s2.to_json(tmp2)));

    {
	Json::Value v(Json::objectValue);
	v["store"] = "Well, can you store me?";
	std::string idterm;
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL("{\"data\":{\"store\":[\"Well, can you store me?\"]}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"store\":[\"Well, can you store me?\"]}");
    }

    {
	Json::Value v(Json::objectValue);
	v["store"] = 1283400000;
	std::string idterm;
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL("{\"data\":{\"store\":[1283400000]}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"store\":[1283400000]}");
    }
}

TEST(EnglishStemmedFields)
{
    Json::Value tmp, tmp2;
    Schema s2("");
    s2.set("text", new TextFieldConfig("t", "text", "stem_en"));
    CHECK_EQUAL("{\"fields\":{\"text\":{\"prefix\":\"t\",\"processor\":\"stem_en\",\"store_field\":\"text\",\"type\":\"text\"}},\"patterns\":[]}",
		json_serialise(s2.to_json(tmp2)));

    Schema s("");
    s.from_json(s2.to_json(tmp));
    tmp = tmp2 = Json::nullValue;
    CHECK_EQUAL(json_serialise(s.to_json(tmp)),
		json_serialise(s2.to_json(tmp2)));

    {
	Json::Value v(Json::objectValue);
	v["text"] = "Some english words";
	std::string idterm;
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL("", idterm);
	CHECK_EQUAL("{\"data\":{\"text\":[\"Some english words\"]},\"terms\":{"

		    "\"Zt\\tenglish\":{\"wdf\":1},"
		    "\"Zt\\tsome\":{\"wdf\":1},"
		    "\"Zt\\tword\":{\"wdf\":1},"
		    "\"t\\tenglish\":{\"positions\":[2],\"wdf\":1},"
		    "\"t\\tsome\":{\"positions\":[1],\"wdf\":1},"
		    "\"t\\twords\":{\"positions\":[3],\"wdf\":1}"
		    
		    "}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL("{\"text\":[\"Some english words\"]}", s.display_doc_as_string(doc));
    }
}

TEST(CJKFields)
{
    Json::Value tmp, tmp2;
    Schema s2("");
    s2.set("text", new TextFieldConfig("t", "text", "cjk"));
    CHECK_EQUAL("{\"fields\":{\"text\":{\"prefix\":\"t\",\"processor\":\"cjk\",\"store_field\":\"text\",\"type\":\"text\"}},\"patterns\":[]}",
		json_serialise(s2.to_json(tmp2)));

    Schema s("");
    s.from_json(s2.to_json(tmp));
    tmp = tmp2 = Json::nullValue;
    CHECK_EQUAL(json_serialise(s.to_json(tmp)),
		json_serialise(s2.to_json(tmp2)));

    {
	Json::Value v(Json::objectValue);
	v["text"] = "Some english text";
	std::string idterm;
	Xapian::Document doc = s.process(v, idterm);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL("{\"data\":{\"text\":[\"Some english text\"]},\"terms\":{\"t\\tenglish\":{\"positions\":[2],\"wdf\":1},\"t\\tsome\":{\"positions\":[1],\"wdf\":1},\"t\\ttext\":{\"positions\":[3],\"wdf\":1}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"text\":[\"Some english text\"]}");
    }
}
