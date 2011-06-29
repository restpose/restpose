/** @file collconfig.h
 * @brief Collection configuration.
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

#ifndef RESTPOSE_INCLUDED_COLLCONFIG_H
#define RESTPOSE_INCLUDED_COLLCONFIG_H

#include "category_hierarchy.h"
#include "json/value.h"
#include <map>
#include <string>
#include <xapian.h>

class TaskManager;

namespace RestPose {

class Schema;
class Pipe;
class Categoriser;

/** All the configuration of the collection.
 *
 *  This contains all the information needed to process documents.
 */
class CollectionConfig {
    /// Collection name.
    std::string coll_name;

    /// Default schema to use for new types.
    Json::Value default_type_config;

    /// The field used to hold ids.
    std::string id_field;

    /// The field used to hold type ids.
    std::string type_field;

    /// Schemas, by type name.
    std::map<std::string, Schema *> types;

    /// Pipes, by given name.
    std::map<std::string, Pipe *> pipes;

    /// Categorisers, by given name.
    std::map<std::string, Categoriser *> categorisers;

    /// Named category hierarchies.
    std::map<std::string, CategoryHierarchy *> categories;

    /// Flag to track whether the collection configuration has been changed.
    bool changed;

    CollectionConfig(const CollectionConfig &);
    void operator=(const CollectionConfig &);

    /** Clear all configuration.
     */
    void clear();

    /** Set the default schema configuration.
     *
     *  This sets default_type_config, id_field and type_field to default
     *  values.
     */
    void set_default_schema();

    /** Write the schemas configuration to a JSON value.
     */
    void schemas_config_to_json(Json::Value & value) const;

    /** Set the schemas configuration from a JSON value.
     */
    void schemas_config_from_json(const Json::Value & value);

    /** Write the pipes configuration to a JSON value.
     */
    void pipes_config_to_json(Json::Value & value) const;

    /** Set the pipes configuration from a JSON value.
     */
    void pipes_config_from_json(const Json::Value & value);

    /** Write the categorisers configuration to a JSON value.
     */
    void categorisers_config_to_json(Json::Value & value) const;

    /** Set the categorisers configuration from a JSON value.
     */
    void categorisers_config_from_json(const Json::Value & value);

    /** Write the categories configuration to a JSON value.
     */
    void categories_config_to_json(Json::Value & value) const;

    /** Set the categories configuration from a JSON value.
     */
    void categories_config_from_json(const Json::Value & value);

  public:
    CollectionConfig(const std::string & coll_name_);
    ~CollectionConfig();

    const std::string & get_name() const {
	return coll_name;
    }

    bool is_changed() const {
	return changed;
    }

    void clear_changed() {
	changed = false;
    }

    /** Set the default configuration.
     *
     *  This is used for newly created collections, when an explicit
     *  configuration is not set.
     */
    void set_default();

    /** Clone this configuration.
     *
     *  Copies the contents of this configuration into an entirely independent,
     *  newly allocated configuration object.
     */
    CollectionConfig * clone() const;

    /** Convert the collection configuration to JSON.
     *
     *  Returns a reference to the value supplied, to allow easier use inline.
     */
    Json::Value & to_json(Json::Value & value) const;

    /** Set the configuration from a JSON representation.
     *
     *  Wipes out any previous configuration.
     */
    void from_json(const Json::Value & value);

    /** Get the schema for a given type.
     *
     *  Returns NULL if the type is not known.
     *
     *  The returned pointer is invalid after modifications have been made
     *  to the collection's schema.
     */
    Schema * get_schema(const std::string & type);

    /** Get the schema for a given type.
     *
     *  Returns NULL if the type is not known.
     *
     *  The returned pointer is invalid after modifications have been made
     *  to the collection's schema.
     */
    const Schema * get_schema(const std::string & type) const;

    /** Set the schema for a given type.
     *
     *  Takes a copy of the supplied schema, and merges it with the existing
     *  schema for the type.
     *
     *  Returns a pointer to the resulting schema for the type.
     */
    Schema * set_schema(const std::string & type,
			const Schema & schema);

    /** Get an input pipe.
     *
     *  Raises an exception if the pipe is not known.
     *
     *  The returned reference is invalid after modifications have been made
     *  to the collection's pipe configuration.
     */
    const Pipe & get_pipe(const std::string & pipe_name) const;

    /** Set an input pipe.
     *
     *  Takes a copy of the supplied pipe.
     */
    void set_pipe(const std::string & pipe_name,
		  const Pipe & pipe);

    /** Get a categoriser.
     *
     *  Raises an exception if the categoriser is not known.
     *
     *  The returned reference is invalid after modifications have been made
     *  to the collection's categoriser configuration.
     */
    const Categoriser & get_categoriser(
	const std::string & categoriser_name) const;

    /** Set a categoriser.
     *
     *  Takes a copy of the supplied categoriser.
     */
    void set_categoriser(const std::string & categoriser_name,
			 const Categoriser & categoriser);

    /** Get a CategoryHierarchy.
     *
     *  Returns NULL if the hierarchy is not known.
     *
     *  The returned pointer is invalid after modifications have been made
     *  to the collection's categoriser configuration.
     */
    const CategoryHierarchy *
	    get_category(const std::string & category_name) const;

    /** Set a category hierarchy.
     *
     *  Takes a copy of the supplied category hierarchy.
     */
    void set_category(const std::string & category_name,
		      const CategoryHierarchy & category);

    /** Categorise a piece of text.
     *
     *  @param categoriser_name The categoriser to use.
     *  @param text The text to categorise.
     *  @param result A JSON object to store the result in.
     *
     *  The result is returned as a JSON array of strings, with the best
     *  matching category first.
     *
     *  Returns a reference to `result`, for easier use in subsequent
     *  operations.
     */
    Json::Value & categorise(const std::string & categoriser_name,
			     const std::string & text,
			     Json::Value & result) const;

    /** Send a document, as a JSON object, to an input pipe.
     */
    void send_to_pipe(TaskManager * taskman,
		      const std::string & pipe_name,
		      Json::Value & obj);

    /** Process a JSON document into a Xapian document.
     */
    Xapian::Document process_doc(Json::Value & doc_obj,
				 const std::string & doc_type,
				 const std::string & doc_id,
				 std::string & idterm);
};

}

#endif /* RESTPOSE_INCLUDED_COLLCONFIG_H */
