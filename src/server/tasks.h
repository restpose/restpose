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

class CollListTask : public ReadonlyTask {
    CollectionPool & collections;
  public:
    CollListTask(const RestPose::ResultHandle & resulthandle_,
		 CollectionPool & collections_)
	    : ReadonlyTask(resulthandle_),
	      collections(collections_)
    {}

    void perform(RestPose::Collection * collection);
};

class CollInfoTask : public ReadonlyCollTask {
  public:
    CollInfoTask(const RestPose::ResultHandle & resulthandle_,
		 const std::string & coll_name_)
	    : ReadonlyCollTask(resulthandle_, coll_name_)
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
    std::string pipe;

    /// The serialised document to send to the pipe.
    Json::Value doc;

  public:
    ProcessorPipeDocumentTask(const std::string & pipe_,
			      const Json::Value & doc_)
	    : pipe(pipe_), doc(doc_)
    {}

    /// Perform the processing task, given a collection (open for reading).
    void perform(RestPose::Collection & collection,
		 TaskManager * taskman);
};

/// Process a JSON document.
class ProcessorProcessDocumentTask : public ProcessingTask {
    /// The type of the document to process.
    std::string doc_type;

    /// The serialised document to process.
    Json::Value doc;

  public:
    ProcessorProcessDocumentTask(const std::string & doc_type_,
				 const Json::Value & doc_)
	    : doc_type(doc_type_), doc(doc_)
    {}

    /// Perform the processing task, given a collection (open for reading).
    void perform(RestPose::Collection & collection,
		 TaskManager * taskman);
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
    void perform(RestPose::Collection & collection);

    /// Clone the task.
    IndexingTask * clone() const;
};

/// Delete a document.
class IndexerDeleteDocumentTask : public IndexingTask {
    /// The unique ID term for the document.
    std::string idterm;

  public:
    IndexerDeleteDocumentTask(const std::string & idterm_)
	    : idterm(idterm_)
    {}

    /// Perform the indexing task, given a collection (open for writing).
    void perform(RestPose::Collection & collection);

    /// Clone the task.
    IndexingTask * clone() const;
};

/// Commit changes.
class IndexerCommitTask : public IndexingTask {
  public:
    IndexerCommitTask()
    {}

    /// Perform the indexing task, given a collection (open for writing).
    void perform(RestPose::Collection & collection);

    /// Clone the task.
    IndexingTask * clone() const;
};

#endif /* RESTPOSE_INCLUDED_TASKS_H */
