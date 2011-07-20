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
#include "jsonxapian/collconfig.h"
#include "jsonxapian/doctojson.h"
#include "jsonxapian/indexing.h"
#include "jsonxapian/schema.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

using namespace RestPose;
using namespace std;

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
    CollectionConfig config("test"); // dummy config, used for testing.
    Schema s("");
    s.set("id", new IDFieldConfig(""));
    s.set("url", new ExactFieldConfig("url", 120, ExactFieldConfig::TOOLONG_HASH, "url", 0));
    s.set("nostore", new ExactFieldConfig("ns", 120, ExactFieldConfig::TOOLONG_ERROR, "", 0));

    {
	Json::Value v(Json::objectValue);
	string idterm;
	IndexingErrors errors;

	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL(doc.get_data(), "");
	Json::Value tmp;
	CHECK_EQUAL("{}", json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{}");
    }

    {
	Json::Value v(Json::objectValue);
	v["id"] = "abcd";
	v["url"] = "http://example.com/";
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "\t\tabcd");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL(doc.get_data(), "\003url\027[\"http://example.com/\"]");
	Json::Value tmp;
	CHECK_EQUAL("{\"data\":{\"url\":[\"http://example.com/\"]},\"terms\":{\"\\\\t\\\\tabcd\":{},\"url\\\\thttp://example.com/\":{}}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"url\":[\"http://example.com/\"]}");
    }

    {
	Json::Value v(Json::objectValue);
	v["nostore"] = "abcd";
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL(doc.get_data(), "");
	Json::Value tmp;
	CHECK_EQUAL("{\"terms\":{\"ns\\\\tabcd\":{}}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{}");
    }
}

TEST(LongExactFields)
{
    CollectionConfig config("test"); // dummy config, used for testing.
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
	string idterm;
	IndexingErrors errors;
	s.process(v, config, idterm, errors);
	CHECK_EQUAL(1u, errors.errors.size());
	v["id"] = "0123456789012345678901234567890123456789012345678901234567890123";
	errors.errors.clear();
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "\t\t0123456789012345678901234567890123456789012345678901234567890123");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"terms\":{\"\\\\t\\\\t0123456789012345678901234567890123456789012345678901234567890123\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{}");
    }

    {
	Json::Value v(Json::objectValue);
	v["url"] = "0123456789012345678901234567890";
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"url\":[\"0123456789012345678901234567890\"]},\"terms\":{\"url\\\\t012345678901234567890123Y]^J/ \":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"url\":[\"0123456789012345678901234567890\"]}");
    }

    {
	Json::Value v(Json::objectValue);
	v["tags"] = "0123456789012345678901234567890";
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"tags\":[\"0123456789012345678901234567890\"]},\"terms\":{\"tags\\\\t012345678901234567890123456789\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"tags\":[\"0123456789012345678901234567890\"]}");
    }

    {
	Json::Value v(Json::objectValue);
	v["valuable"] = "0123456789012345678901234567890";
	string idterm;
	IndexingErrors errors;
	s.process(v, config, idterm, errors);
	CHECK_EQUAL(1u, errors.errors.size());
	errors.errors.clear();
	v["valuable"] = "012345678901234567890123456789";
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"valuable\":[\"012345678901234567890123456789\"]},\"terms\":{\"valuable\\\\t012345678901234567890123456789\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"valuable\":[\"012345678901234567890123456789\"]}");
    }
}

TEST(IntegerExactFields)
{
    CollectionConfig config("test"); // dummy config, used for testing.
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
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"intid\":[5000000000]},\"terms\":{\"intid\\\\t5000000000\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"intid\":[5000000000]}");
    }

    {
	Json::Reader reader;
	Json::Value v;
	CHECK(reader.parse("{\"intid\": -1}", v, false));

	string idterm;
	IndexingErrors errors;
	s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(1u, errors.errors.size());

	CHECK(reader.parse("{\"intid\": 0}", v, false));
	errors.errors.clear();
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL("{\"data\":{\"intid\":[0]},\"terms\":{\"intid\\\\t0\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());

	CHECK(reader.parse("{\"intid\": 1}", v, false));
	errors.errors.clear();
	doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL("{\"data\":{\"intid\":[1]},\"terms\":{\"intid\\\\t1\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());

	CHECK(reader.parse("{\"intid\": 4294967295}", v, false)); // 2**32-1
	errors.errors.clear();
	doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL("{\"data\":{\"intid\":[4294967295]},\"terms\":{\"intid\\\\t4294967295\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());

	CHECK(reader.parse("{\"intid\": 9223372036854775807}", v, false)); // 2**63-1
	errors.errors.clear();
	doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL("{\"data\":{\"intid\":[9223372036854775807]},\"terms\":{\"intid\\\\t9223372036854775807\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());

	CHECK(reader.parse("{\"intid\": 9223372036854775808}", v, false)); // 2**63
	errors.errors.clear();
	doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL("{\"data\":{\"intid\":[9223372036854775808]},\"terms\":{\"intid\\\\t9223372036854775808\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());

	CHECK(reader.parse("{\"intid\": 18446744073709551615}", v, false)); // 2**64-1
	errors.errors.clear();
	doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL("{\"data\":{\"intid\":[18446744073709551615]},\"terms\":{\"intid\\\\t18446744073709551615\":{}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());

	CHECK(reader.parse("{\"intid\": 18446744073709551616}", v, false));
	errors.errors.clear();
	doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(1u, errors.errors.size()); // Too big
    }
}

TEST(DoubleFields)
{
    CollectionConfig config("test"); // dummy config, used for testing.
    Json::Value tmp, tmp2;
    Schema s2("");
    s2.set("num", new DoubleFieldConfig(7, "num"));
    CHECK_EQUAL("{\"fields\":{\"num\":{\"slot\":7,\"store_field\":\"num\",\"type\":\"double\"}},\"patterns\":[]}",
		json_serialise(s2.to_json(tmp2)));

    Schema s("");
    s.from_json(s2.to_json(tmp));
    tmp = tmp2 = Json::nullValue;
    CHECK_EQUAL(json_serialise(s.to_json(tmp)),
		json_serialise(s2.to_json(tmp2)));

    {
	Json::Value v(Json::objectValue);
	v["num"] = 0;
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"num\":[0]},\"values\":{\"7\":\"\\\\x80\"}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"num\":[0]}");
    }

    Xapian::Database db = Xapian::InMemory::open();
    Xapian::Query q = s.build_query(config, db, json_unserialise("{\"field\": [\"num\", \"range\", [-1, 1]]}", tmp));
    CHECK_EQUAL("Xapian::Query(VALUE_RANGE 7 ^ \xa0)", q.get_description());
    q = s.build_query(config, db, json_unserialise("{\"field\": [\"num\", \"range\", [0, 1]]}", tmp));
    CHECK_EQUAL("Xapian::Query(VALUE_RANGE 7 \x80 \xa0)", q.get_description());
}

TEST(TimestampFields)
{
    CollectionConfig config("test"); // dummy config, used for testing.
    Json::Value tmp, tmp2;
    Schema s2("");
    s2.set("timestamp", new TimestampFieldConfig(0, "timestamp"));
    CHECK_EQUAL("{\"fields\":{\"timestamp\":{\"slot\":0,\"store_field\":\"timestamp\",\"type\":\"timestamp\"}},\"patterns\":[]}",
		json_serialise(s2.to_json(tmp2)));

    Schema s("");
    s.from_json(s2.to_json(tmp));
    tmp = tmp2 = Json::nullValue;
    CHECK_EQUAL(json_serialise(s.to_json(tmp)),
		json_serialise(s2.to_json(tmp2)));

    {
	Json::Value v(Json::objectValue);
	v["timestamp"] = 1283400000;
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"timestamp\":[1283400000]},\"values\":{\"0\":\"\\\\xe0\\\\\\\\\\\\xc7\\\\xf2\\\\x14\"}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"timestamp\":[1283400000]}");
    }
}

TEST(DateFields)
{
    CollectionConfig config("test"); // dummy config, used for testing.
    Json::Value tmp, tmp2;
    Schema s2("");
    s2.set("date", new DateFieldConfig(0, "date2"));
    CHECK_EQUAL("{\"fields\":{\"date\":{\"slot\":0,\"store_field\":\"date2\",\"type\":\"date\"}},\"patterns\":[]}",
		json_serialise(s2.to_json(tmp2)));

    Schema s("");
    s.from_json(s2.to_json(tmp));
    tmp = tmp2 = Json::nullValue;
    CHECK_EQUAL(json_serialise(s.to_json(tmp)),
		json_serialise(s2.to_json(tmp2)));

    {
	Json::Value v(Json::objectValue);
	v["date"] = "2010-06-08";
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"date2\":[\"2010-06-08\"]},\"values\":{\"0\":\"\\\\xcf\\\\xda&(\"}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL("{\"date2\":[\"2010-06-08\"]}", s.display_doc_as_string(doc));
    }
}

TEST(CategoryFields)
{
    CollectionConfig config("test"); // dummy config, used for testing.
    Json::Value tmp, tmp2;
    Schema s2("");
    s2.set("cat", new CategoryFieldConfig("cat", 30, ExactFieldConfig::TOOLONG_ERROR, "category"));
    CHECK_EQUAL("{\"fields\":{\"cat\":{\"max_length\":30,\"prefix\":\"cat\",\"store_field\":\"category\",\"too_long_action\":\"error\",\"type\":\"cat\"}},\"patterns\":[]}",
		json_serialise(s2.to_json(tmp2)));

    Schema s("");
    s.from_json(s2.to_json(tmp));
    tmp = tmp2 = Json::nullValue;
    CHECK_EQUAL(json_serialise(s.to_json(tmp)),
		json_serialise(s2.to_json(tmp2)));

    {
	Json::Value v(Json::objectValue);
	v["cat"] = "foo";
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"category\":[\"foo\"]},\"terms\":{\"cat\\\\tCfoo\":{}}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL("{\"category\":[\"foo\"]}", s.display_doc_as_string(doc));
    }
}

TEST(StoreFields)
{
    CollectionConfig config("test"); // dummy config, used for testing.
    Json::Value tmp, tmp2;
    Schema s2("");
    s2.set("store", new StoredFieldConfig(string("store2")));
    CHECK_EQUAL("{\"fields\":{\"store\":{\"store_field\":\"store2\",\"type\":\"stored\"}},\"patterns\":[]}",
		json_serialise(s2.to_json(tmp2)));

    Schema s("");
    s.from_json(s2.to_json(tmp));
    tmp = tmp2 = Json::nullValue;
    CHECK_EQUAL(json_serialise(s.to_json(tmp)),
		json_serialise(s2.to_json(tmp2)));

    {
	Json::Value v(Json::objectValue);
	v["store"] = "Well, can you store me?";
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"store2\":[\"Well, can you store me?\"]}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL("{\"store2\":[\"Well, can you store me?\"]}",
		    s.display_doc_as_string(doc));
    }

    {
	Json::Value v(Json::objectValue);
	v["store"] = 1283400000;
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"store2\":[1283400000]}}",
		    json_serialise(doc_to_json(doc, tmp)));
	CHECK_EQUAL("{\"store2\":[1283400000]}", s.display_doc_as_string(doc));
    }
}

TEST(EnglishStemmedFields)
{
    CollectionConfig config("test"); // dummy config, used for testing.
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
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL("", idterm);
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"text\":[\"Some english words\"]},\"terms\":{"

		    "\"Zt\\\\tenglish\":{\"wdf\":1},"
		    "\"Zt\\\\tsome\":{\"wdf\":1},"
		    "\"Zt\\\\tword\":{\"wdf\":1},"
		    "\"t\\\\tenglish\":{\"positions\":[2],\"wdf\":1},"
		    "\"t\\\\tsome\":{\"positions\":[1],\"wdf\":1},"
		    "\"t\\\\twords\":{\"positions\":[3],\"wdf\":1}"
		    
		    "}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL("{\"text\":[\"Some english words\"]}", s.display_doc_as_string(doc));
    }
}

TEST(CJKFields)
{
    CollectionConfig config("test"); // dummy config, used for testing.
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
	string idterm;
	IndexingErrors errors;
	Xapian::Document doc = s.process(v, config, idterm, errors);
	CHECK_EQUAL(idterm, "");
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"text\":[\"Some english text\"]},\"terms\":{\"t\\\\tenglish\":{\"positions\":[2],\"wdf\":1},\"t\\\\tsome\":{\"positions\":[1],\"wdf\":1},\"t\\\\ttext\":{\"positions\":[3],\"wdf\":1}}}",
		    json_serialise(doc_to_json(doc, tmp2)));
	CHECK_EQUAL(s.display_doc_as_string(doc), "{\"text\":[\"Some english text\"]}");
    }
}

TEST(SlotNumbers)
{
    SlotName name;
    CHECK_EQUAL(Xapian::BAD_VALUENO, name.get());

    // Check a specific value (should help catch portability issues).
    name = string("1");
    CHECK_EQUAL(268435538u, name.get());
    name = string("alternate string");
    CHECK_EQUAL(2470924216u, name.get());
    name = string("string alternate");
    CHECK_EQUAL(3524491384u, name.get());

    name = 1;
    CHECK_EQUAL(1u, name.get());

    name = string("");
    CHECK(name.get() >= 0x10000000u && name.get() <= 0xffffffffu);
    CHECK(name.get() != 268435538u);  // check not all hashes are the same!

    name = string("Short string");
    CHECK(name.get() >= 0x10000000u && name.get() <= 0xffffffffu);

    name = string("Long sdjug siduh sidu ysidu ysiduy siduy string");
    CHECK(name.get() >= 0x10000000u && name.get() <= 0xffffffffu);
}
