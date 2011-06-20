/** @file filesystem_import.cc
 * @brief Importer to import from filesystem
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

#include "filesystem_import.h"

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <xapian.h>
#include "json/reader.h"
#include "json/value.h"
#include "json/writer.h"

#include "diritor.h"
#include "loadfile.h"
#include "safeerrno.h"
#include "jsonxapian/schema.h"
#include "utils/rsperrors.h"

using namespace RestPose;

static Xapian::WritableDatabase
jxdb_create_or_open(const std::string & database_path,
		    const std::string & schema_path,
		    Schema & schema)
{
    Xapian::WritableDatabase db(database_path, Xapian::DB_CREATE_OR_OPEN);
    std::string schema_str = db.get_metadata("_schema");
    if (schema_str.empty()) {
	if (schema_path.empty()) {
	    throw InvalidValueError("Need to specify schema for a new database");
	}
    }
    if (!schema_path.empty()) {
	std::string new_schema_str;
	if (!load_file(schema_path, new_schema_str)) {
	    throw SysError("Error loading schema file \"" +
			   schema_path + "\"", errno);
	}
	if (new_schema_str.empty()) {
	    throw InvalidValueError("Schema file \"" + schema_path +
				    "\" was empty");
	}
	if (!schema_str.empty() && schema_str != new_schema_str) {
	    throw InvalidValueError("Cannot change schema on existing database");
	}
	schema.from_json(new_schema_str);
	db.set_metadata("_schema", new_schema_str);
    } else {
	schema.from_json(schema_str);
    }
    return db;
}

void
RestPose::index_file(Schema & schema,
		     Xapian::WritableDatabase & db,
		     const std::string & path)
{
    std::string contents;
    if (!load_file(path, contents, true)) {
	std::cerr << "Error loading file \"" << path << "\": "
		<< strerror(errno) << std::endl;
	return;
    }
    if (contents.empty()) {
	// Silently ignore empty files
	return;
    }
    Json::Reader reader;
    Json::Value value;
    bool ok = reader.parse(contents, value, false);
    if (!ok) {
	std::cerr << "Error in file \"" << path << "\": invalid JSON "
		<< reader.getFormatedErrorMessages() << std::endl;
	return;
    }
    std::string idterm;
    Xapian::Document doc(schema.process(value, idterm));
    if (idterm.empty()) {
	db.add_document(doc);
    } else {
	db.replace_document(idterm, doc);
    }

    if (db.get_doccount() % 100 == 0) {
	std::cout << "\rIndexed " << db.get_doccount() << " docs" << std::endl;
    }
}

void
RestPose::index_dir(Schema & schema,
		    Xapian::WritableDatabase & db,
		    const std::string & topdir)
{
    DirectoryIterator diter(true);
    try {
	diter.start(topdir);
	std::string basepath(topdir);
	if (basepath[basepath.size() - 1] != '/')
	    basepath += '/';
	std::string path;
	while (diter.next()) try {
	    path = basepath + diter.leafname();
	    switch (diter.get_type()) {
		case DirectoryIterator::DIRECTORY:
		    index_dir(schema, db, path);
		    break;
		case DirectoryIterator::REGULAR_FILE:
		    index_file(schema, db, path);
		    break;
		case DirectoryIterator::OTHER:
		    // Ignore other things silently
		    break;
	    }
	} catch (const std::string & error) {
	    std::cerr << error << ": skipping file \"" << path << "\""
		  << std::endl;
	}
    } catch (const std::string & error) {
	std::cerr << error << ": skipping directory \"" << topdir << "\""
		  << std::endl;
    }
}

int
RestPose::do_cmd(std::string datadir, std::string dbname,
		 std::string schema_path,
		 std::vector<std::string> indexdirs,
		 std::vector<std::string> searchfiles,
		 bool show_info)
{
    try {
	std::string database_path = datadir + "/" + dbname;

	{
	    Schema schema("FIXME");
	    Xapian::WritableDatabase db = 
		    jxdb_create_or_open(database_path, schema_path, schema);

	    if (show_info) {
		Json::Value schema_value;
		schema.to_json(schema_value);
		std::cout << schema_value.toStyledString() << std::endl;
	    }
	    for (std::vector<std::string>::const_iterator i = indexdirs.begin();
		 i != indexdirs.end(); ++i) {
		if (!(*i).empty())
		    index_dir(schema, db, *i);
	    }
	}

	for (std::vector<std::string>::const_iterator i = searchfiles.begin();
	     i != searchfiles.end(); ++i) {
	    std::string searchfile = *i;
	    if (!searchfile.empty()) {
		Xapian::Database db(database_path);
		Schema schema("FIXME");
		std::string schema_str = db.get_metadata("_schema");
		schema.from_json(schema_str);
		std::string search_str;
		if (searchfile == "-") {
		    std::cin >> std::noskipws;
		    std::istream_iterator<char> it(std::cin);
		    std::istream_iterator<char> end;
		    search_str = std::string(it, end);
		} else {
		    if (!load_file(searchfile, search_str)) {
			std::cerr << "Error loading search file \"" <<
				searchfile << "\": " << strerror(errno) << std::endl;
			return 1;
		    }
		}
		Json::Value search_results(Json::objectValue);
		schema.perform_search(db, search_str, search_results);
		Json::FastWriter writer;
		std::cout << writer.write(search_results) << std::endl;
	    }
	}
    } catch (const std::exception & e) {
	std::cerr << "Error: " << e.what() << std::endl;
    } catch (const Xapian::Error & e) {
	std::cerr << "Error: " << e.get_description() << std::endl;
    }
    return 0;
}
