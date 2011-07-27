/** @file collection.cc
 * @brief Tests for Collections
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

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "jsonxapian/collection.h"
#include "jsonxapian/collection_pool.h"
#include "jsonxapian/doctojson.h"
#include "jsonxapian/indexing.h"
#include "jsonxapian/pipe.h"
#include "server/task_manager.h"
#include "utils.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"

using namespace RestPose;

#define DEFAULT_TYPE_SCHEMA \
"\"default_type\":{" \
  "\"patterns\":[" \
    "[\"*_text\",{\"group\":\"t*\",\"processor\":\"stem_en\",\"store_field\":\"*_text\",\"type\":\"text\"}]," \
    "[\"text\",{\"group\":\"t\",\"processor\":\"stem_en\",\"store_field\":\"text\",\"type\":\"text\"}]," \
    "[\"*_num\",{\"slot\":\"n*\",\"store_field\":\"*_num\",\"type\":\"double\"}]," \
    "[\"num\",{\"slot\":\"n\",\"store_field\":\"num\",\"type\":\"double\"}]," \
    "[\"*_time\",{\"slot\":\"d*\",\"store_field\":\"*_time\",\"type\":\"timestamp\"}]," \
    "[\"time\",{\"slot\":\"d\",\"store_field\":\"time\",\"type\":\"timestamp\"}]," \
    "[\"*_tag\",{\"group\":\"g*\",\"max_length\":100,\"store_field\":\"*_tag\",\"too_long_action\":\"hash\",\"type\":\"exact\"}]," \
    "[\"tag\",{\"group\":\"g\",\"max_length\":100,\"store_field\":\"tag\",\"too_long_action\":\"hash\",\"type\":\"exact\"}]," \
    "[\"*_url\",{\"group\":\"u*\",\"max_length\":100,\"store_field\":\"*_url\",\"too_long_action\":\"hash\",\"type\":\"exact\"}]," \
    "[\"url\",{\"group\":\"u\",\"max_length\":100,\"store_field\":\"url\",\"too_long_action\":\"hash\",\"type\":\"exact\"}]," \
    "[\"*_cat\",{\"group\":\"c*\",\"max_length\":32,\"store_field\":\"*_cat\",\"too_long_action\":\"hash\",\"type\":\"cat\"}]," \
    "[\"cat\",{\"group\":\"c\",\"max_length\":32,\"store_field\":\"cat\",\"too_long_action\":\"hash\",\"type\":\"cat\"}]," \
    "[\"id\",{\"store_field\":\"id\",\"type\":\"id\"}]," \
    "[\"type\",{\"group\":\"!\",\"store_field\":\"type\",\"type\":\"exact\"}]," \
    "[\"_meta\",{\"group\":\"#\",\"slot\":0,\"type\":\"meta\"}]," \
    "[\"*\",{\"group\":\"t\",\"store_field\":\"*\",\"type\":\"text\"}]" \
  "]" \
"}"

#define DEFAULT_SPECIAL_FIELDS "\"special_fields\":{\"id_field\":\"id\",\"meta_field\":\"_meta\",\"type_field\":\"type\"}"

class TempDir {
    std::string path;

    TempDir(const TempDir &);
    void operator=(const TempDir &);
  public:
    TempDir(std::string prefix) {
	char tmpl[prefix.size() + 7];
	memcpy(tmpl, prefix.c_str(), prefix.size());
	memset(tmpl + prefix.size(), 'X', 6);
	tmpl[prefix.size() + 6] = '\0';
	char * result = mkdtemp(tmpl);
	if (result == NULL) {
	    throw SysError("Can't make temporary directory", errno);
	}
	path = std::string(result, prefix.size() + 6);
    }

    ~TempDir() {
	// destroy the directory at path.
	if (!path.empty()) {
	    try {
		removedir(path);
	    } catch (const Xapian::Error & e) {
		printf("Error removing temporary directory: %s\n", e.get_description().c_str());
	    }
	}
    }

    std::string get() {
	return path;
    };
};

/// Test checking of the format in collection configs.
TEST(CollectionConfigFormatCheck)
{
    TempDir path("/tmp/jsonxapian");
    Collection c("test", path.get() + "/test");
    c.open_writable();

    CHECK_THROW(c.from_json(std::string("")), InvalidValueError);
    
    // Check error due to missing format
    CHECK_THROW(c.from_json(std::string("{}")), InvalidValueError);

    // Check encoding of an empty config.
    Json::Value tmp;
    c.from_json(json_unserialise(std::string("{\"format\": 3}"), tmp));
    tmp = Json::nullValue;
    CHECK_EQUAL("{" DEFAULT_TYPE_SCHEMA ","
		"\"format\":3,\"pipes\":{\"default\":{}},"
		DEFAULT_SPECIAL_FIELDS ","
		"\"types\":{}"
		"}",
		json_serialise(c.to_json(tmp)));
}

/// Test setting and getting collection schemas.
TEST(CollectionSchemas)
{
    TempDir path("/tmp/jsonxapian");
    Collection c("test", path.get() + "/test");
    c.open_writable();

    Schema s("");
    Json::Value tmp;

    // Set the schema for a type, and retrieve it.
    s.from_json(json_unserialise("{\"fields\":{\"store\":{\"store_field\":\"store\",\"type\":\"stored\"}}}", tmp));
    c.set_schema("default", s);
    {
	const Schema & s2 = c.get_schema("default");
	tmp = Json::nullValue;
	s2.to_json(tmp);
	CHECK_EQUAL("{\"fields\":{\"store\":{\"store_field\":\"store\",\"type\":\"stored\"}},\"patterns\":[]}", json_serialise(tmp));
    }

    // Check that trying to change the configuration for a field raises an
    // exception, and doesn't change it.
    s.from_json(json_unserialise("{\"fields\":{\"store\":{\"store_field\":\"store2\",\"type\":\"stored\"}}}", tmp));
    CHECK_THROW(c.set_schema("default", s), InvalidValueError);
    {
	const Schema & s2 = c.get_schema("default");
	tmp = Json::nullValue;
	s2.to_json(tmp);
	CHECK_EQUAL("{\"fields\":{\"store\":{\"store_field\":\"store\",\"type\":\"stored\"}},\"patterns\":[]}", json_serialise(tmp));
    }

    // Check that trying to set the configuration for a field to the same
    // value it already has doesn't raise an exception.
    s.from_json(json_unserialise("{\"fields\":{\"store\":{\"store_field\":\"store\",\"type\":\"stored\"}}}", tmp));
    c.set_schema("default", s);

    // Set the schema for a new field.
    s.from_json(json_unserialise("{\"fields\":{\"store2\":{\"store_field\":\"store2\",\"type\":\"stored\"}}}", tmp));
    c.set_schema("default", s);
    {
	const Schema & s2 = c.get_schema("default");
	tmp = Json::nullValue;
	s2.to_json(tmp);
	CHECK_EQUAL("{\"fields\":{\"store\":{\"store_field\":\"store\",\"type\":\"stored\"},\"store2\":{\"store_field\":\"store2\",\"type\":\"stored\"}},\"patterns\":[]}", json_serialise(tmp));
    }

    c.close();
    CHECK_THROW(c.get_schema("default"), InvalidStateError);

    c.open_readonly();
    {
	const Schema & s2 = c.get_schema("default");
	tmp = Json::nullValue;
	s2.to_json(tmp);
	CHECK_EQUAL("{\"fields\":{\"store\":{\"store_field\":\"store\",\"type\":\"stored\"},\"store2\":{\"store_field\":\"store2\",\"type\":\"stored\"}},\"patterns\":[]}", json_serialise(tmp));
    }
    CHECK_THROW(c.set_schema("default", s), InvalidStateError);
}

/// Test setting and getting collection pipes.
TEST(CollectionPipes)
{
    TempDir path("/tmp/jsonxapian");
    Collection c("test", path.get() + "/test");
    c.open_writable();

    Pipe p;
    Json::Value tmp;

    // Set a pipe, and retrieve it.
    p.from_json(json_unserialise("{\"mappings\":[{\"map\":[{\"from\":\"foo\",\"to\":\"bar\"}]}]}", tmp));
    c.set_pipe("default", p);
    {
	const Pipe & p2 = c.get_pipe("default");
	p2.to_json(tmp);
	CHECK_EQUAL("{\"mappings\":[{\"map\":[{\"from\":[\"foo\"],\"to\":\"bar\"}]}]}", json_serialise(tmp));
    }

    // Check that the configuration for a pipe can be changed.
    p.from_json(json_unserialise("{\"mappings\":[{\"map\":[{\"from\":\"foo\",\"to\":\"baz\"}]}]}", tmp));
    c.set_pipe("default", p);
    {
	const Pipe & p2 = c.get_pipe("default");
	p2.to_json(tmp);
	CHECK_EQUAL("{\"mappings\":[{\"map\":[{\"from\":[\"foo\"],\"to\":\"baz\"}]}]}", json_serialise(tmp));
    }


    c.close();
    CHECK_THROW(c.get_pipe("default"), InvalidStateError);

    c.open_readonly();
    {
	const Pipe & p2 = c.get_pipe("default");
	p2.to_json(tmp);
	CHECK_EQUAL("{\"mappings\":[{\"map\":[{\"from\":[\"foo\"],\"to\":\"baz\"}]}]}", json_serialise(tmp));
    }
    CHECK_THROW(c.set_pipe("default", p), InvalidStateError);

    CHECK_EQUAL("{"
		DEFAULT_TYPE_SCHEMA ","
		"\"format\":3,"
		"\"pipes\":{\"default\":{\"mappings\":[{\"map\":[{\"from\":[\"foo\"],\"to\":\"baz\"}]}]}},"
		DEFAULT_SPECIAL_FIELDS ","
		"\"types\":{}"
		"}", json_serialise(c.to_json(tmp)));
    CHECK_THROW(c.from_json(tmp), InvalidStateError);
}

// Test adding a document to a collection via a pipe
TEST(CollectionAddViaPipe)
{
    TempDir path("/tmp/jsonxapian");
    CollectionPool pool(path.get());
    TaskManager * taskman = new TaskManager(pool);
    taskman->start();

    Collection * c = pool.get_writable("default");

    Json::Value tmp;
    c->from_json(json_unserialise("{"
		"\"format\":3,"
	  "\"pipes\":{\"default\":{\"mappings\":[{\"map\":[{\"from\":[\"foo\"],\"to\":\"bar\"}]}]}},"
	  "\"types\":{\"default\":{\"fields\":{"
	    "\"id\":{\"max_length\":64,"
		    "\"store_field\":\"\","
		    "\"too_long_action\":\"error\",\"type\":\"id\"},"
	    "\"foo\":{\"store_field\":\"foo\",\"type\":\"stored\"},"
	    "\"bar\":{\"store_field\":\"bar\",\"type\":\"stored\"}"
	  "}}}"
	"}", tmp));
    c->commit();
    c->close();
    pool.release(c);
    c = pool.get_readonly("default");

    CHECK_EQUAL("{"
		DEFAULT_TYPE_SCHEMA ","
		"\"format\":3,"
		"\"pipes\":{\"default\":{\"mappings\":[{\"map\":[{\"from\":[\"foo\"],\"to\":\"bar\"}]}]}},"
		DEFAULT_SPECIAL_FIELDS ","
		"\"types\":{\"default\":{\"fields\":{"
		  "\"bar\":{\"store_field\":\"bar\",\"type\":\"stored\"},"
		  "\"foo\":{\"store_field\":\"foo\",\"type\":\"stored\"},"
		  "\"id\":{\"max_length\":64,"
			  "\"store_field\":\"\","
			  "\"too_long_action\":\"error\",\"type\":\"id\"}"
		  "},\"patterns\":[]}}"
		"}", json_serialise(c->to_json(tmp)));

    CHECK_EQUAL(0u, c->doc_count());

    c->send_to_pipe(taskman, "default",
		   json_unserialise("{"
				    "\"id\": \"1\","
				    "\"foo\": \"Hello world\""
				    "}",tmp));
    taskman->stop();
    taskman->join();
    delete taskman;
    CHECK_EQUAL(0u, c->doc_count());
    pool.release(c);
    c = pool.get_readonly("default");
    CHECK_EQUAL(1u, c->doc_count());

    c->close();
    CHECK_THROW(c->doc_count(), InvalidStateError);
    delete c;
}

/// Test using a categoriser in a collection.
TEST(CollectionCategoriser)
{
    TempDir path("/tmp/jsonxapian");
    CollectionPool pool(path.get());
    TaskManager * taskman = new TaskManager(pool);
    taskman->start();
    Json::Value tmp;

    Collection * c = pool.get_writable("default");

    c->from_json(json_unserialise("{"
		"\"format\": 3,"
		"\"types\":{\"default\":{\"fields\":{"
		  "\"text\":{\"store_field\":\"text\",\"type\":\"stored\"},"
		  "\"lang\":{\"store_field\":\"lang\",\"type\":\"stored\"},"
		  "\"id\":{\"max_length\":64,"
			  "\"store_field\":\"\","
			  "\"too_long_action\":\"error\",\"type\":\"id\"}"
		  "}}}"
	"}", tmp));
    CHECK_EQUAL("{"
		DEFAULT_TYPE_SCHEMA ","
		"\"format\":3,"
		"\"pipes\":{\"default\":{}},"
		DEFAULT_SPECIAL_FIELDS ","
		"\"types\":{\"default\":{\"fields\":{"
		  "\"id\":{\"max_length\":64,"
			  "\"store_field\":\"\","
			  "\"too_long_action\":\"error\",\"type\":\"id\"},"
		  "\"lang\":{\"store_field\":\"lang\",\"type\":\"stored\"},"
		  "\"text\":{\"store_field\":\"text\",\"type\":\"stored\"}"
		  "},\"patterns\":[]}}"
		"}", json_serialise(c->to_json(tmp)));

    Categoriser cat(1.03, 4, 10, 1);
    cat.add_target_profile("english", "hello world");
    cat.add_target_profile("russian", "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82 \xd0\x94\xd0\xbe\xd0\xb1\xd1\x80\xd0\xbe");
    c->set_categoriser("lang", cat);

    // Check some english text
    c->categorise("lang", "Hello", tmp);
    CHECK_EQUAL("[\"english\"]", json_serialise(tmp));

    // Check some russian text
    c->categorise("lang", "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82", tmp);
    CHECK_EQUAL("[\"russian\"]", json_serialise(tmp));

    // Check some ambiguous text
    c->categorise("lang", "caf\xc3\xa9", tmp);
    CHECK_EQUAL("[]", json_serialise(tmp));

    CHECK_EQUAL("{"
		"\"categorisers\":{"
		 "\"lang\":{"
		  "\"accuracy_threshold\":1.030,"
		  "\"max_candidates\":1,"
		  "\"max_ngram_length\":4,"
		  "\"max_ngrams\":10,"
		  "\"profiles\":{"
		   "\"english\":{\"max_ngrams\":10,\"ngrams\":[\"|\",\"l\",\"o\",\"d\",\"d|\",\"e\",\"el\",\"ell\",\"ello\",\"h\"]},"
		   "\"russian\":{\"max_ngrams\":10,\"ngrams\":[\"|\",\"\xd0\xbe\",\"\xd1\x80\",\"|\xd0\xb4\",\"|\xd0\xb4\xd0\xbe\",\"|\xd0\xb4\xd0\xbe\xd0\xb1\",\"|\xd0\xbf\",\"|\xd0\xbf\xd1\x80\",\"|\xd0\xbf\xd1\x80\xd0\xb8\",\"\xd0\xb1\"]}"
		  "},"
		  "\"type\":\"ngram_rank\""
		 "}"
		"},"
		DEFAULT_TYPE_SCHEMA ","
		"\"format\":3,"
		"\"pipes\":{\"default\":{}},"
		DEFAULT_SPECIAL_FIELDS ","
		"\"types\":{\"default\":{\"fields\":{"
		  "\"id\":{\"max_length\":64,"
			  "\"store_field\":\"\","
			  "\"too_long_action\":\"error\",\"type\":\"id\"},"
		  "\"lang\":{\"store_field\":\"lang\",\"type\":\"stored\"},"
		  "\"text\":{\"store_field\":\"text\",\"type\":\"stored\"}"
		  "},\"patterns\":[]}}"
		"}", json_serialise(c->to_json(tmp)));

    Pipe p;
    p.from_json(json_unserialise("{\"mappings\":["
	  "{\"map\":["
	    "{\"categoriser\":\"lang\",\"from\":[\"text\"],\"to\":\"lang\"},"
	    "{\"from\":[\"text\"],\"to\":\"text\"}"
	  "]}"
	"]}", tmp));
    CHECK_EQUAL("{\"mappings\":["
		  "{\"map\":["
		    "{\"categoriser\":\"lang\",\"from\":[\"text\"],\"to\":\"lang\"},"
		    "{\"from\":[\"text\"],\"to\":\"text\"}"
		  "]}"
		"]}", json_serialise(p.to_json(tmp)));
    c->set_pipe("default", p);

    c->commit();
    c->close();
    pool.release(c);
    c = pool.get_readonly("default");

    c->send_to_pipe(taskman, "default",
		   json_unserialise("{"
				    "\"id\": \"2\","
				    "\"text\": \"Hello world\""
				    "}",tmp));
    taskman->stop();
    taskman->join();
    delete taskman;
    CHECK_EQUAL(0u, c->doc_count());
    pool.release(c);
    c = pool.get_readonly("default");
    CHECK_EQUAL(1u, c->doc_count());
    c->get_document("default", "1", tmp);
    CHECK_EQUAL("null", json_serialise(tmp));

    tmp = Json::objectValue;
    tmp["foo"] = "regression"; // check that get_document clears its result

    c->get_document("default", "2", tmp);
    CHECK_EQUAL("{\"data\":{"
		   "\"lang\":[\"english\"],"
		   "\"text\":[\"Hello world\"]"
		 "},"
		 "\"terms\":{\"\\\\tdefault\\\\t2\":{}}"
		"}", json_serialise(tmp));
}

/// Test using a category hierarchy in a collection.
TEST(CollectionCategory)
{
    TempDir path("/tmp/jsonxapian");
    CollectionPool pool(path.get());
    Json::Value tmp;

    Collection * c = pool.get_writable("default");

    c->from_json(json_unserialise("{"
		"\"categories\":{\"foo\":{}},"
		"\"format\": 3,"
		"\"types\":{\"default\":{\"fields\":{"
		  "\"foo\":{\"store_field\":\"foo\",\"type\":\"cat\",\"group\":\"foo\"},"
		  "\"id\":{\"max_length\":64,"
			  "\"store_field\":\"\","
			  "\"too_long_action\":\"error\",\"type\":\"id\"}"
		  "}}}"
	"}", tmp));
    CHECK_EQUAL("{"
		"\"categories\":{\"foo\":{}},"
		DEFAULT_TYPE_SCHEMA ","
		"\"format\":3,"
		"\"pipes\":{\"default\":{}},"
		DEFAULT_SPECIAL_FIELDS ","
		"\"types\":{\"default\":{\"fields\":{"
		  "\"foo\":{"
			   "\"group\":\"foo\","
			   "\"max_length\":64,"
			   "\"store_field\":\"foo\","
			   "\"too_long_action\":\"error\","
			   "\"type\":\"cat\"},"
		  "\"id\":{\"max_length\":64,"
			  "\"store_field\":\"\","
			  "\"too_long_action\":\"error\",\"type\":\"id\"}"
		  "},\"patterns\":[]}}"
		"}", json_serialise(c->to_json(tmp)));

    CategoryHierarchy h = *(c->get_category_hierarchy("foo"));
    Categories modified;
    h.add_parent("child", "parent", modified);
    c->set_category_hierarchy("foo", h);
    CHECK_EQUAL("{"
		"\"categories\":{\"foo\":{\"child\":[\"parent\"],\"parent\":[]}},"
		DEFAULT_TYPE_SCHEMA ","
		"\"format\":3,"
		"\"pipes\":{\"default\":{}},"
		DEFAULT_SPECIAL_FIELDS ","
		"\"types\":{\"default\":{\"fields\":{"
		  "\"foo\":{"
			   "\"group\":\"foo\","
			   "\"max_length\":64,"
			   "\"store_field\":\"foo\","
			   "\"too_long_action\":\"error\","
			   "\"type\":\"cat\"},"
		  "\"id\":{\"max_length\":64,"
			  "\"store_field\":\"\","
			  "\"too_long_action\":\"error\",\"type\":\"id\"}"
		  "},\"patterns\":[]}}"
		"}", json_serialise(c->to_json(tmp)));

    {
	Json::Value doc(Json::objectValue);
	doc["foo"] = "hello";
	std::string idterm;
	Xapian::Document xdoc = c->process_doc(doc, "default", "0", idterm);
	CHECK_EQUAL("\tdefault\t0", idterm);
	CHECK_EQUAL("{\"data\":{\"foo\":[\"hello\"]},\"terms\":{\"\\\\tdefault\\\\t0\":{},\"foo\\\\tChello\":{}}}",
		    json_serialise(doc_to_json(xdoc, tmp)));

	doc = Json::objectValue;
	Json::Value & catval = doc["foo"] = Json::arrayValue;
	catval.append("world");
	catval.append("child");
	idterm.resize(0);
	xdoc = c->process_doc(doc, "default", "1", idterm);
	CHECK_EQUAL("\tdefault\t1", idterm);
	CHECK_EQUAL("{\"data\":{\"foo\":[\"world\",\"child\"]},\"terms\":{\"\\\\tdefault\\\\t1\":{},\"foo\\\\tAparent\":{},\"foo\\\\tCchild\":{},\"foo\\\\tCworld\":{}}}",
		    json_serialise(doc_to_json(xdoc, tmp)));
	c->raw_update_doc(xdoc, idterm);

	Json::Value tmp2;
	c->get_document("default", "1", tmp2);
	CHECK_EQUAL("{\"data\":{\"foo\":[\"world\",\"child\"]},\"terms\":{\"\\\\tdefault\\\\t1\":{},\"foo\\\\tAparent\":{},\"foo\\\\tCchild\":{},\"foo\\\\tCworld\":{}}}",
		    json_serialise(tmp2));

	c->category_add_parent("foo", "parent", "grand");
	c->get_document("default", "1", tmp2);
	CHECK_EQUAL("{\"data\":{\"foo\":[\"world\",\"child\"]},\"terms\":{\"\\\\tdefault\\\\t1\":{},\"foo\\\\tAgrand\":{},\"foo\\\\tAparent\":{},\"foo\\\\tCchild\":{},\"foo\\\\tCworld\":{}}}",
		    json_serialise(tmp2));

	c->category_remove("foo", "child");
	c->get_document("default", "1", tmp2);
	CHECK_EQUAL("{\"data\":{\"foo\":[\"world\",\"child\"]},\"terms\":{\"\\\\tdefault\\\\t1\":{},\"foo\\\\tCchild\":{},\"foo\\\\tCworld\":{}}}",
		    json_serialise(tmp2));

	c->category_add_parent("foo", "child", "parent");
	c->get_document("default", "1", tmp2);
	CHECK_EQUAL("{\"data\":{\"foo\":[\"world\",\"child\"]},\"terms\":{\"\\\\tdefault\\\\t1\":{},\"foo\\\\tAgrand\":{},\"foo\\\\tAparent\":{},\"foo\\\\tCchild\":{},\"foo\\\\tCworld\":{}}}",
		    json_serialise(tmp2));
    }
}

/// Test storing of meta info
TEST(MetaInfoSimple)
{
    CollectionConfig c("testcoll");
    Json::Value tmp;
    c.set_default();

    // A doc with a single valid field value.
    Json::Value doc(Json::objectValue);
    doc["foo"] = "hello";
    std::string idterm;
    IndexingErrors errors;
    Xapian::Document xdoc = c.process_doc(doc, "default", "0", idterm,
					  errors);
    CHECK_EQUAL("\tdefault\t0", idterm);
    CHECK_EQUAL(0u, errors.errors.size());
    CHECK_EQUAL("{\"data\":{\"foo\":[\"hello\"],"
		"\"id\":[\"0\"],"
		"\"type\":[\"default\"]},"
		"\"terms\":{"
			   "\"!\\\\tdefault\":{},"
			   "\"#\\\\tFfoo\":{},"
			   "\"#\\\\tN\":{},"
			   "\"#\\\\tNfoo\":{},"
			   "\"\\\\tdefault\\\\t0\":{},"
			   "\"t\\\\thello\":{\"positions\":[1],\"wdf\":1}"
		"}}",
		json_serialise(doc_to_json(xdoc, tmp)));
}

/// Test storing of meta info for missing fields
TEST(MetaInfoMissing)
{
    CollectionConfig c("testcoll");
    Json::Value tmp;
    c.set_default();

    // A doc with empty field values.
    {
	Json::Value doc(Json::objectValue);
	Json::Value emptyval(Json::arrayValue);
	emptyval.append(Json::nullValue);
	doc["foo_text"] = emptyval;
	doc["foo_time"] = emptyval;
	doc["foo_tag"] = emptyval;
	doc["foo_url"] = emptyval;
	doc["foo_cat"] = emptyval;
	std::string idterm;
	IndexingErrors errors;
	Xapian::Document xdoc = c.process_doc(doc, "default", "0", idterm,
					      errors);
	CHECK_EQUAL("\tdefault\t0", idterm);
	CHECK_EQUAL(0u, errors.errors.size());
	CHECK_EQUAL("{\"data\":{\"foo_cat\":[null],"
			       "\"foo_tag\":[null],"
			       "\"foo_text\":[null],"
			       "\"foo_time\":[null],"
			       "\"foo_url\":[null],"
			       "\"id\":[\"0\"],"
			       "\"type\":[\"default\"]},"
		     "\"terms\":{\"!\\\\tdefault\":{},"
				"\"#\\\\tFfoo_cat\":{},"
				"\"#\\\\tFfoo_tag\":{},"
				"\"#\\\\tFfoo_text\":{},"
				"\"#\\\\tFfoo_time\":{},"
				"\"#\\\\tFfoo_url\":{},"
				"\"#\\\\tM\":{},"
				"\"#\\\\tMfoo_cat\":{},"
				"\"#\\\\tMfoo_tag\":{},"
				"\"#\\\\tMfoo_text\":{},"
				"\"#\\\\tMfoo_time\":{},"
				"\"#\\\\tMfoo_url\":{},"
				"\"\\\\tdefault\\\\t0\":{}"
		    "}}",
		    json_serialise(doc_to_json(xdoc, tmp)));
    }
}
