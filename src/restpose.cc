/** @file restpose.cc
 * @brief Search server for xapian
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
#include <memory>
#include <iostream>
#include "loadfile.h"

#include "cli/cli.h"
#include "httpserver/httpserver.h"
// #include "importer/filesystem/filesystem_import.h"
#include "importer/mongo/mongo_import.h"
#include <pthread.h>
#include "rest/routes.h"
#include "rest/router.h"
#include "safeerrno.h"
#include "server/task_manager.h"
#include "server/server.h"
#include "utils/rsperrors.h"

// For training; this should probably be in a separate file
#include "ngramcat/categoriser.h"
#include "utils/io_wrappers.h"
#include "utils/jsonutils.h"

#define PROGNAME "restpose"

using namespace RestPose;

static int
main_do(int argc, char * const* argv)
{
    CliOptions opts;
    int ret = opts.parse(PROGNAME, argc, argv);
    if (ret) {
	return (ret > 0) ? ret : 0;
    }

    Server server;
    CollectionPool pool(opts.datadir);
    TaskManager * taskman = new TaskManager(pool);
    server.add("taskman", taskman);
    Router router(taskman, &server);
    setup_routes(router);
    g_log.start();

    try {

    if (opts.action == CliOptions::ACT_SERVE) {
	server.add("httpserver", new HTTPServer(opts.port, opts.pedantic, &router));

	if (!opts.mongo_import.empty()) {
	    std::auto_ptr<MongoImporter> importer(new MongoImporter(taskman));
	    Json::Value config;
	    importer->set_config(json_unserialise(opts.mongo_import, config));
	    server.add_bg_task("mongoimport", importer.release());
	}
	server.run();

    } else if (opts.action == CliOptions::ACT_SEARCH) {
	for (std::vector<std::string>::const_iterator i = opts.searchfiles.begin();
	     i != opts.searchfiles.end(); ++i) {
	    std::string searchfile = *i;
	    if (!searchfile.empty()) {
		Collection coll(opts.dbname, opts.datadir + "/" + opts.dbname);
		coll.open_readonly();
		std::string search_str;
		if (searchfile == "-") {
		    std::cin >> std::noskipws;
		    std::istream_iterator<char> it(std::cin);
		    std::istream_iterator<char> end;
		    search_str = std::string(it, end);
		} else {
		    if (!load_file(searchfile, search_str)) {
			throw SysError("Error loading search file \"" +
				       searchfile + "\"", errno);
		    }
		}
		Json::Value search;
		Json::Value search_results;
		coll.perform_search(json_unserialise(search_str, search),
				    "default", // FIXME - make the type configurable
				    search_results);

		std::cout << json_serialise(search_results) << std::endl;
	    }
	}

    } else if (opts.action == CliOptions::ACT_TRAIN) {
	Categoriser cat;
	for (std::vector<std::string>::const_iterator
	     i = opts.languages.begin(); i != opts.languages.end(); ++i) {
	    std::string path = opts.datadir + "/text_" + *i;
	    std::string text;
	    int fd = io_open_read(path.c_str());
	    if (fd == -1) {
		throw SysError("Unable to open data file at " + path, errno);
	    }
	    if (!io_read_exact(text, fd, 1024 * 1024)) {
		throw SysError("Unable to open read from file at " + path, errno);
	    }
	    cat.add_target_profile(*i, text);
	}
	Json::Value tmp;
	cat.to_json(tmp);
	printf("%s", json_serialise(tmp).c_str());
    }

    } catch(...) {
	g_log.stop();
	g_log.join();
	throw;
    }

    g_log.stop();
    g_log.join();

    return ret;
}

#ifdef PTW32_STATIC_LIB
extern "C" {
int ptw32_processInitialize();
};
#endif

int
main(int argc, char * const* argv)
{
#ifdef PTW32_STATIC_LIB
    ptw32_processInitialize();
    if (!pthread_win32_process_attach_np()) {
	fprintf(stderr, "Unable to initialise threading\n");
	return 1;
    }
#endif
    int result;
    try {
	result = main_do(argc, argv);
    } catch(const RestPose::Error & e) {
	fprintf(stderr, "Error: %s - server exiting\n", e.what());
	result = 1;
    } catch(const Xapian::Error & e) {
	fprintf(stderr, "Error: %s - server exiting\n", e.get_description().c_str());
	result = 1;
    } catch(const std::bad_alloc) {
	fprintf(stderr, "Error: out of memory\n");
	result = 1;
    }
#ifdef PTW32_STATIC_LIB
    pthread_win32_process_detach_np();
#endif
    return result;
}
