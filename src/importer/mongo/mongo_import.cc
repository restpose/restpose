/** @file mongo_import.cc
 * @brief Importer to import data from mongodb.
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
#include "mongo_import.h"

#include <cstdio>
#include <cstring>
#include "utils/compression.h"
#include "utils/threading.h"

#include "features/checkpoint_handlers.h"
#include "jsonxapian/collection.h"
#include "jsonxapian/schema.h"
#include "jsonmanip/jsonpath.h"
#include "json/reader.h"
#include "json/value.h"
#include "json/writer.h"
#include "server/task_manager.h"
#include "utils/jsonutils.h"
#include "loadfile.h"
extern "C" {
#include "mongo.h"
};
#include "safeerrno.h"
#include "str.h"
#include "realtime.h"

using namespace RestPose;
using namespace std;

struct MongoImporterConfig {
    /** The mongodb host IP address to connect to.
     */
    std::string host_ip;

    /** The mongodb port to connect to.
     */
    int port;

    /** The database name to import from.
     */
    std::string mongodb;

    /** The collection to import from.
     */
    std::string collection;

    /** The query to run to get documents to import, as serialised JSON.
     */
    Json::Value query;

    /** The database path to write to.
     */
    std::string out_db_path;

    /** The path to the schema for the database to write to.
     */
    std::string out_config_path;

    /** True if any current connection should be dropped and a new
     *  connection made.
     */
    bool changed;

    MongoImporterConfig()
	    : port(-1),
	      changed(true)
    {}

    /** Set the configuration from a JSON object.
     */
    void set_from_json(const Json::Value & newconfig);
};

void
MongoImporterConfig::set_from_json(const Json::Value & newconfig) {
    Json::Value member;
    json_check_object(newconfig, "MongoImporter config");

    host_ip = json_get_string_member(newconfig, "host_ip", std::string());

    member = newconfig.get("port", Json::Value::null);
    if (member.isNull()) {
	port = 27017;
    } else {
	port = member.asInt();
    }

    mongodb = json_get_string_member(newconfig, "mongodb", mongodb);
    collection = json_get_string_member(newconfig, "collection", collection);

    member = newconfig.get("query", Json::Value::null);
    if (member.isNull()) {
	query = Json::objectValue;
    } else {
	query = member;
    }

    out_db_path = json_get_string_member(newconfig, "out_db_path", std::string());
    out_config_path = json_get_string_member(newconfig, "out_config_path",
					     std::string());

    changed = true;
}

class MongoImporter::Internal : public Thread {
    /** The mutex to hold when updating the status.
     *
     *  Mutable so that it can be locked from const methods.
     */
    mutable Mutex status_mutex;

    /** The status value.
     */
    Json::Value status;

    /// The time at which the status was last updated.
    double last_display;

    /// The time at which the import run started.
    double starttime;

    /** The central RestPose server.
     */
    Server * server;

    /** The central task manager.
     */
    TaskManager * taskman;

    /** The configuration for the importer.
     *
     *  May be modified from another thread, so should only be accessed with
     *  the mutex locked.
     */
    MongoImporterConfig config;

    /** The connection to mongo used for reading documents to index.
     */
    mongo_connection read_conn;

    /** The connection to mongo used for updating document statuses.
     */
    mongo_connection write_conn;

    /** Current cursor obtained from the connection.
     */
    mongo_cursor * cursor;

    /** Current query in use.
     */
    bson query;

    /** Current namespace for connection to mongodb.
     */
    std::string ns;


    /** Connect to mongodb.
     *
     *  Note - mutex must be held by caller.
     */
    void connect();

    /** Build the query, from the config.
     */
    void build_query();

    /// Update the importer's status.
    void update_status(unsigned int count,
		       unsigned int batch_count);
  public:
    Internal(TaskManager * taskman_);

    /** Set the server in use.
     */
    void set_server(Server * server_);

    /** Set the configuration.
     */
    void set_config(const Json::Value & newconfig);

    /// Get the status.
    void get_status(Json::Value & result) const;

    /** Run the importer.
     */
    void run();

    /** Cleanup after running the importer.
     */
    void cleanup();
};

MongoImporter::MongoImporter(TaskManager * taskman)
	: BackgroundTask(),
	  internal(new MongoImporter::Internal(taskman))
{
}

MongoImporter::~MongoImporter()
{
    delete internal;
}

void
MongoImporter::set_config(const Json::Value & newconfig)
{
    internal->set_config(newconfig);
}

void
MongoImporter::get_status(Json::Value & result) const
{
    internal->get_status(result);
}

void
MongoImporter::start(Server * server_)
{
    internal->set_server(server_);
    internal->start();
}

void
MongoImporter::stop()
{
    internal->stop();
}

void
MongoImporter::join()
{
    internal->join();
}


MongoImporter::Internal::Internal(TaskManager * taskman_)
	: status(Json::objectValue),
	  server(NULL),
	  taskman(taskman_),
	  cursor(NULL)
{
    bson_empty(&query);
}

void
MongoImporter::Internal::set_server(Server * server_)
{
    server = server_;
}

void
MongoImporter::Internal::set_config(const Json::Value & newconfig)
{
    ContextLocker lock(cond);
    config.set_from_json(newconfig);
    cond.broadcast();
}

void
MongoImporter::Internal::connect()
{
    if (mongo_connect(&read_conn, config.host_ip.c_str(), config.port)) {
	throw ImporterError("Couldn't connect to mongo server at " + config.host_ip + ":" + str(config.port));
    }
    if (mongo_connect(&write_conn, config.host_ip.c_str(), config.port)) {
	throw ImporterError("Couldn't connect to mongo server at " + config.host_ip + ":" + str(config.port));
    }
}

static void bson_object_to_json(const char * bsondata, Json::Value & result);
static void bson_array_to_json(const char * bsondata, Json::Value & result);

static void
bson_to_json(ZlibInflater & inflater,
	     bson_iterator & it,
	     bson_type t,
	     Json::Value & result)
{
    switch (t) {
	case bson_int:
	    result = bson_iterator_int(&it);
	    break;
	case bson_long:
	    result = static_cast<Json::Int64>(bson_iterator_long(&it));
	    break;
	case bson_double:
	    result = bson_iterator_double(&it);
	    break;
	case bson_string: {
	    const char * s = bson_iterator_string(&it);
	    result = Json::Value(s, s + bson_iterator_string_len(&it));
	    break;
	}
	case bson_bool:
	    result = bson_iterator_bool(&it);
	    break;
	case bson_date: {
	    // FIXME - should this be formatted as a ISO timestamp?
	    result = static_cast<Json::Int64>(bson_iterator_date(&it));
	    break;
	}

	case bson_undefined: // FALLTHROUGH
	case bson_null: {
	    result = Json::nullValue;
	    break;
	}

	case bson_timestamp: {
	    // Represent timestamps as an array of [seconds, increment]
	    bson_timestamp_t tval = bson_iterator_timestamp(&it);
	    Json::Value val(Json::arrayValue);
	    val.append(tval.t);
	    val.append(tval.i);
	    result = val;
	    break;
	}

	case bson_object: {
	    Json::Value obj;
	    bson_object_to_json(bson_iterator_value(&it), obj);
	    result = obj;
	    break;
	}
	case bson_array: {
	    Json::Value obj;
	    bson_array_to_json(bson_iterator_value(&it), obj);
	    result = obj;
	    break;
	}
	case bson_bindata: {
	    int bintype = bson_iterator_bin_type(&it);
	    const char * data;
	    int data_len;
	    std::string uncompressed;
	    switch (bintype) {
		case 0:
		case 2:
		    uncompressed = inflater.inflate(bson_iterator_bin_data(&it),
						    bson_iterator_bin_len(&it));
		    data = uncompressed.data();
		    data_len = uncompressed.size();
		    break;
		default:
		    throw InvalidValueError("Unsupported binary type (" +
					    str(bintype) +
					    ") in bson document");
	    }
	    result = Json::Value(data, data + data_len);
	    break;
	}
	case bson_oid: {
	    char hex_oid[25];
	    bson_oid_to_string(bson_iterator_oid(&it), hex_oid);
	    result = hex_oid;
	    break;
	}

	case bson_regex: {
	    throw InvalidValueError("Unsupported type (regex) in bson document");
	}
	case bson_dbref: {
	    throw InvalidValueError("Unsupported type (dbref) in bson document");
	}
	case bson_code: {
	    throw InvalidValueError("Unsupported type (code) in bson document");
	}
	case bson_symbol: {
	    throw InvalidValueError("Unsupported type (symbol) in bson document");
	}
	case bson_codewscope: {
	    throw InvalidValueError("Unsupported type (codewscope) in bson document");
	}
	default:
	    throw InvalidValueError("Unsupported type in bson document");
    }
}

static void
bson_array_to_json(const char * bsondata, Json::Value & result)
{
    ZlibInflater inflater;
    result = Json::arrayValue;
    bson_iterator it;
    bson_iterator_init(&it, bsondata);
    while (bson_iterator_next(&it)) {
	bson_type t = bson_iterator_type(&it);
	if (t == bson_eoo)
	    break;
	bson_to_json(inflater, it, t, result[result.size()]);
    }
}

static void
bson_object_to_json(const char * bsondata, Json::Value & result)
{
    ZlibInflater inflater;
    result = Json::objectValue;
    bson_iterator it;
    bson_iterator_init(&it, bsondata);
    while (bson_iterator_next(&it)) {
	bson_type t = bson_iterator_type(&it);
	if (t == bson_eoo)
	    break;
	const char * key = bson_iterator_key(&it);
	bson_to_json(inflater, it, t, result[key]);
    }
}

static void
json_to_bson(const Json::Value & input, bson * result)
{
    bson_destroy(result);
    if (input.empty()) {
	bson_empty(result);
    } else {
	unsigned int level = 0;
	bson_buffer buffer;
	bson_buffer_init(&buffer);
	try {
	    JSONWalker walker(input);
	    JSONWalker::Event event;
	    while (walker.next(event)) {
		std::string name;
		if (event.component.is_string()) {
		    name = event.component.key;
		} else {
		    name = str(event.component.index);
		}

		switch (event.type) {
		    case JSONWalker::Event::START:
		    {
			if (level > 0) {
			    if (event.value->isArray()) {
				bson_append_start_array(&buffer, name.c_str());
			    } else {
				bson_append_start_object(&buffer, name.c_str());
			    }
			}
			level++;
			break;
		    }
		    case JSONWalker::Event::LEAF:
		        if (event.value->isNull()) {
			    bson_append_null(&buffer, name.c_str());
			} else if (event.value->isIntegral()) {
			    bson_append_long(&buffer, name.c_str(), event.value->asInt64());
			} else if (event.value->isDouble()) {
			    bson_append_double(&buffer, name.c_str(), event.value->asDouble());
			} else if (event.value->isConvertibleTo(Json::stringValue)) {
			    bson_append_string(&buffer, name.c_str(), (*(event.value)).asString().c_str());
			} else {
			    throw InvalidValueError("Unsupported type in bson document");
			}
			break;
		    case JSONWalker::Event::END:
			level--;
			if (level > 0) {
			    bson_append_finish_object(&buffer);
			}
			break;
		}
	    }
	} catch(...) {
	    bson_buffer_destroy(&buffer);
	    throw;
	}
	bson_from_buffer(result, &buffer);
    }
}

void
MongoImporter::Internal::build_query()
{
    ns = config.mongodb;
    ns.append(".");
    ns.append(config.collection);

    json_to_bson(config.query, &query);
}

static void
set_update_status(mongo_connection * connptr,
		  const char * mongodb,
		  const char * ns,
		  std::string mongo_id,
		  int status,
		  bool reload_first)
{
    if (mongo_id.size() < 24) {
	throw ImporterError("Invalid mongo OID for object, when updating status in mongodb");
    }
    bson_oid_t oid;
    bson_oid_from_string(&oid, mongo_id.c_str());

    bson_date_t now = static_cast<bson_date_t>(RealTime::now() * 1000);

    bson cond;
    bson op;

    bson_buffer buffer;
    bson_buffer_init(&buffer);
    bson_append_oid(&buffer, "_id", &oid);
    bson_from_buffer(&cond, &buffer);

    bson_buffer_init(&buffer);
    bson_append_start_object(&buffer, "$set");
    bson_append_int(&buffer, "index_status", status);
    bson_append_date(&buffer, "index_status_updated_at", now);
    bson_append_date(&buffer, "updated_at", now);
    bson_append_finish_object(&buffer);
    bson_from_buffer(&op, &buffer);

    if (reload_first) {
	// Read the document to get it into memory, so that a write lock won't
	// be held open for long in the subsequent update command.
	(void)mongo_find_one(connptr, ns, &cond, NULL, NULL);
    }
    mongo_update(connptr, ns, &cond, &op, MONGO_UPDATE_MULTI);

    bson errout;
    bool err = mongo_cmd_get_last_error(connptr, mongodb, &errout);
    if (err) {
	throw ImporterError("Unable to update document's update status");
    }
}

void
MongoImporter::Internal::get_status(Json::Value & result) const
{
    ContextLocker lock(status_mutex);
    result = status;
}

void
MongoImporter::Internal::update_status(unsigned int count,
				       unsigned int batch_count)
{
    double now = RealTime::now();
    if (now - last_display > 1.0) {
	ContextLocker lock(status_mutex);
	status[Json::StaticString("processed")] = count;
	status[Json::StaticString("docs_per_second")] = count / (now - starttime);
	status[Json::StaticString("batches_finished")] = batch_count;
	last_display = now;
	printf("\rProcessed: %d (%.1f/s, finished %d batches)\n", count, count / (now - starttime), batch_count);
    }
    fflush(stdout);
}

void
MongoImporter::Internal::run()
{
    unsigned int count = 0;
    unsigned int batch_count = 0;
    unsigned int batch_size = 100000;
    unsigned int batch_end = count + batch_size;
    starttime = RealTime::now();

    std::string coll_name("default");

    Collection coll(coll_name, config.out_db_path);
    coll.open_writable();
    {
	std::string config_str;
	if (!load_file(config.out_config_path, config_str)) {
	    throw SysError("Error loading schema file \"" +
			   config.out_config_path + "\"", errno);
	}
	Json::Value config_json;
	coll.from_json(json_unserialise(config_str, config_json));
    }
    coll.close();

    last_display = RealTime::now();

    while (true) {
	{
	    ContextLocker lock(cond);
	    if (stop_requested) return;
	    if (config.changed) {
		connect();
		build_query();
		config.changed = false;
	    }
	}

	// In the following call, 10 is the number of items to return in the
	// first batch of results.  0 doesn't seem to work with all servers (I
	// had a mongo 2.8 server returning no entries for 0, but a 2.4 server
	// worked fine), so we just pick 10 as an arbitrary number.
	//
	// 64 is the flag for "exhaust" - stream down all the data as fast as
	// possible.  The client isn't allowed to stop reading the data other
	// than by closing the connection, which is fine for us since we don't
	// want to do anything else.
	cursor = mongo_find(&read_conn, ns.c_str(), &query,
			    NULL, 10, 0, 64);

	std::vector<std::string> unflushed_ids;
	while (mongo_cursor_next(cursor)) {
	    Json::Value item;
	    bson_object_to_json(cursor->current.data, item);
	    if (item.isObject()) {
		Json::Value err = item.get("$err", Json::nullValue);
		if (!err.isNull()) {
		    throw ImporterError("Received error from mongo server: " +
					json_serialise(err));
		}
	    }

	    std::string mongo_id = json_get_string_member(item, "_id", "");
	    if (mongo_id.empty()) {
		throw ImporterError("Recieved doc from mongo server with no "
				    "id: " + json_serialise(item));
	    }
	    set_update_status(&write_conn, config.mongodb.c_str(),
			      ns.c_str(), mongo_id, 20, false);
	    unflushed_ids.push_back(mongo_id);

	    Queue::QueueState state;
	    while (true) {
		state = taskman->queue_pipe_document(coll_name, "default",
						     item, true,
						     RealTime::now() + 1.0);
		if (state == Queue::CLOSED) return;
		if (state != Queue::FULL) break;

		{
		    ContextLocker lock(cond);
		    if (stop_requested) return;
		}
		continue;
	    }

	    ++count;
	    if (count % 23) {
		update_status(count, batch_count);
	    }

	    if (count > batch_end) {
		// FIXME - queue a commit to happen after processing all items
		// in the queue, and wait for it to complete.
		string checkid;
		CollCreateCheckpointHandler::create_checkpoint(taskman,
		    coll_name, checkid, true, false);
		for (std::vector<std::string>::const_iterator
		     i = unflushed_ids.begin();
		     i != unflushed_ids.end();
		     ++i) {
		    set_update_status(&write_conn, config.mongodb.c_str(), ns.c_str(),
				      *i, 100, true);
		}
		unflushed_ids.clear();

		batch_end = count + batch_size;
		batch_count += 1;
	    }

	    {
		ContextLocker lock(cond);
		if (stop_requested) return;
	    }
	}

	{
	    // FIXME - queue a commit to happen after processing all items
	    // in the queue, and wait for it to complete.
	    string checkid;
	    CollCreateCheckpointHandler::create_checkpoint(taskman, coll_name,
		checkid, true, false);
	    for (std::vector<std::string>::const_iterator
		 i = unflushed_ids.begin();
		 i != unflushed_ids.end();
		 ++i) {
		set_update_status(&write_conn, config.mongodb.c_str(), ns.c_str(),
				  *i, 100, true);
	    }
	    unflushed_ids.clear();

	    batch_end = count + batch_size;
	    batch_count += 1;
	}

	update_status(count, batch_count);

	server->shutdown();

	{
	    ContextLocker lock(cond);
	    if (stop_requested) return;
	    // FIXME - timed wait.
	    cond.wait();
	}
    }
}

void
MongoImporter::Internal::cleanup()
{
    printf("cleanup importer thread\n");
    server->shutdown();
    bson_destroy(&query);
    mongo_cursor_destroy(cursor);
    mongo_destroy(&read_conn);
    mongo_destroy(&write_conn);
}
