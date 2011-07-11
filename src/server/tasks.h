/** @file tasks.h
 * @brief Tasks to be placed on queues for performing later.
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
#ifndef RESTPOSE_INCLUDED_TASKS_H
#define RESTPOSE_INCLUDED_TASKS_H

#include "server/basetasks.h"
#include <string>

namespace Xapian {
    class Document;
};

namespace RestPose {
    class CollectionConfig;
};

class CollectionPool;

class StaticFileTask : public ReadonlyTask {
    std::string path;
  public:
    StaticFileTask(const RestPose::ResultHandle & resulthandle_,
		   const std::string & path_)
	    : ReadonlyTask(resulthandle_), path(path_)
    {}

    void perform(RestPose::Collection * collection);
};

class PerformSearchTask : public ReadonlyCollTask {
    Json::Value search;
    std::string doc_type;
  public:
    PerformSearchTask(const RestPose::ResultHandle & resulthandle_,
		      const std::string & coll_name_,
		      const Json::Value & search_,
		      const std::string & doc_type_)
	    : ReadonlyCollTask(resulthandle_, coll_name_),
	      search(search_),
	      doc_type(doc_type_)
    {}

    void perform(RestPose::Collection * collection);
};

class GetDocumentTask : public ReadonlyCollTask {
    std::string doc_type;
    std::string doc_id;
  public:
    GetDocumentTask(const RestPose::ResultHandle & resulthandle_,
		    const std::string & coll_name_,
		    const std::string & doc_type_,
		    const std::string & doc_id_)
	    : ReadonlyCollTask(resulthandle_, coll_name_),
	      doc_type(doc_type_),
	      doc_id(doc_id_)
    {}

    void perform(RestPose::Collection * collection);
};

class ServerStatusTask : public ReadonlyTask {
    const TaskManager * taskman;
  public:
    ServerStatusTask(const RestPose::ResultHandle & resulthandle_,
		     const TaskManager * taskman_)
	    : ReadonlyTask(resulthandle_),
	      taskman(taskman_)
    {}

    void perform(RestPose::Collection * collection);
};

/// Process a JSON document via a pipe.
class ProcessorPipeDocumentTask : public ProcessingTask {
    /// The pipe to send the document to.
    std::string target_pipe;

    /// The serialised document to send to the pipe.
    Json::Value doc;

  public:
    ProcessorPipeDocumentTask(const std::string & target_pipe_,
			      const Json::Value & doc_)
	    : target_pipe(target_pipe_), doc(doc_)
    {}

    /// Perform the processing task, given a collection (open for reading).
    void perform(const std::string & coll_name,
		 TaskManager * taskman);
};

/// Process a JSON document.
class ProcessorProcessDocumentTask : public ProcessingTask {
    /// The type of the document to process.
    std::string doc_type;

    /// The ID of the document to process.
    std::string doc_id;

    /// The serialised document to process.
    Json::Value doc;

  public:
    ProcessorProcessDocumentTask(const std::string & doc_type_,
				 const std::string & doc_id_,
				 const Json::Value & doc_)
	    : doc_type(doc_type_), doc_id(doc_id_), doc(doc_)
    {}

    /// Perform the processing task, given a collection (open for reading).
    void perform(const std::string & coll_name,
		 TaskManager * taskman);
};

class IndexerConfigChangedTask : public IndexingTask {
    Json::Value new_config;
  public:
    IndexerConfigChangedTask(const Json::Value & new_config_)
	    : new_config(new_config_)
    {}

    void perform_task(const std::string & coll_name,
		      RestPose::Collection * & collection,
		      TaskManager * taskman);
    void info(std::string & description,
	      std::string & doc_type,
	      std::string & doc_id) const;

    IndexingTask * clone() const;
};

/// Add or update a document.
class IndexerUpdateDocumentTask : public IndexingTask {
    /// The unique ID term for the document.
    std::string idterm;

    /// The document to add.
    Xapian::Document doc;

  public:
    IndexerUpdateDocumentTask(const std::string & idterm_,
			      const Xapian::Document & doc_)
	    : idterm(idterm_), doc(doc_)
    {}

    /// Perform the indexing task, given a collection (open for writing).
    void perform_task(const std::string & coll_name,
		      RestPose::Collection * & collection,
		      TaskManager * taskman);

    void info(std::string & description,
	      std::string & doc_type,
	      std::string & doc_id) const;

    /// Clone the task.
    IndexingTask * clone() const;
};

/// Delete a document.
class DeleteDocumentTask : public IndexingTask {
    /// The unique ID term for the document.
    std::string doc_type;
    std::string doc_id;

  public:
    DeleteDocumentTask(const std::string & doc_type_,
		       const std::string & doc_id_)
	    : doc_type(doc_type_),
	      doc_id(doc_id_)
    {}

    /// Perform the indexing task, given a collection (open for writing).
    void perform_task(const std::string & coll_name,
		      RestPose::Collection * & collection,
		      TaskManager * taskman);

    void info(std::string & description,
	      std::string & doc_type,
	      std::string & doc_id) const;

    /// Clone the task.
    IndexingTask * clone() const;
};


/// Delete a collection task for processing queue.
class DeleteCollectionProcessingTask : public ProcessingTask {
  public:
    DeleteCollectionProcessingTask() : ProcessingTask(false) {}
    void perform(const std::string & coll_name,
		 TaskManager * taskman);
};

/// Delete a collection.
class DeleteCollectionTask : public IndexingTask {
  public:
    /// Perform the indexing task, given a collection (open for writing).
    void perform_task(const std::string & coll_name,
		      RestPose::Collection * & collection,
		      TaskManager * taskman);

    void info(std::string & description,
	      std::string & doc_type,
	      std::string & doc_id) const;

    /// Clone the task.
    IndexingTask * clone() const;
};

#endif /* RESTPOSE_INCLUDED_TASKS_H */
