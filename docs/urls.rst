======================
RestPose URL structure
======================

This section of the RestPose documentation details the URLs at which RestPose
makes resources available.

General notes
=============

Parameter types
---------------

There are three ways to pass parameters in HTTP requests:

 * As part of the request path (referred to in the following documentation as
   `Parameters`).

 * As part of the request query string (referred to in the following
   documentation as `Query Parameters`).

 * As part of the request body.  Where this is applicable, the details of the
   body to supply is described below.

Boolean parameters
------------------

When a parameter is documented as accepting a boolean value, "true" can be
represented as `1`, `true`, `yes` or `on`, and "false" can be represented as
`0`, `false`, `no` or `off` (with all strings comparisons here being case
insensitive).

C-style escapes
---------------

Where non-unicode strings need to be returned, they will be escaped using
C-style escaping.  More precisely:

 * \\ will be represented as \\\\

 * Tab (hex code 0x09) will be represented as \\t

 * Line feed (hex code 0x0a) will be represented as \\n

 * Carriage return (hex code 0x0d) will be represented as \\r

 * Characters from 0x32 to 0x7f (inclusive) will be represented as
   themselves.

 * Any other characters will be represented as \\xXX, where XX is the hex
   representation of the character code.

Error returns
-------------

If a 400 or 500 series HTTP error code is returned from the following methods,
except where noted otherwise, the response body will be of type
:mimetype:`application/json`, and will contain a JSON object with, at least, an
``err`` property, containing a string describing the error.

Collections
===========

Everything to do with collections is available under the `/coll` URL.

Listing collections
-------------------

.. http:get:: /coll

   Get details of the collections held by the server.

   Takes no parameters.

   :statuscode 200: Normal response: returns a JSON object keyed by collection
	       name, in which the values are empty objects.  (In future, some
	       information about the collections may be available in the
	       values.)


Information about an individual collection
------------------------------------------

.. http:get:: /coll/(collection_name)

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.

   On success, the return value is a JSON object with the following members:

    * ``doc_count``: The number of documents in the collection.

   :statuscode 200: If the collection exists, and no errors occur.
   :statuscode 404: If the collection does not exist.  Returns a standard error object.


Deleting a collection
---------------------

.. http:delete:: /coll/(collection_name)

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.

   The collection of the given name is deleted, if it exists.  If it doesn't
   exist, returns successfully, but doesn't change anything.

   :statuscode 200: Normal response: returns an empty JSON object.


Collection configuration
------------------------

The collection configuration is represented as a JSON object; for details of its contents, see :ref:`Collection Configuration <coll_config>`.

.. http:get:: /coll/(collection_name)/config

   Get the collection configuration.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.

   :statuscode 200: Normal response: returns a JSON object representing the
	       full configuration for the collection.  See :ref:`coll_config`
	       for details.

   :statuscode 404: If the collection does not exist.  Returns a standard error object.

.. http:put:: /coll/(collection_name)/config

   Set the collection configuration.  Actually, adds a task to set the
   collection configuration to the processing queue.  This may be monitored,
   waited for, and committed using checkpoints in just the same way as for the
   document addition APIs.

   Creates the collection if it didn't exist before the call.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.

   :statuscode 202: Normal response: returns a JSON object representing the
	       full configuration for the collection.  See :ref:`coll_config`
	       for details.


Checkpoints
-----------

Checkpoints are used to control committing of changes, sequence order of
modification operations, and also to allow a client to wait until changes have
been applied.

Note that checkpoints will be removed automatically after a timeout (though by
default this timeout is around 1 day, so this will rarely be an issue in
practice).

Checkpoints also do not persist across server restarts.

.. http:get:: /coll/(collection_name)/checkpoint

   Get details of the checkpoints which exist for a collection.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.

   :statuscode 200: Normal response: returns a JSON array of strings, each
	       string is the ID of a checkpoint on the collection.  If the
	       collection doesn't exist, returns an empty array.

.. http:post:: /coll/(collection_name)/checkpoint

   Create a checkpoint.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.

   :queryparam commit: (boolean). True if the checkpoint should cause a commit,
               False if the checkpoint should not cause a commit.

   :statuscode 201: Normal response: returns a JSON object containing a single
	       item, with a key of ``checkid`` and a value being a string used
	       to identify the newly created checkpoint.  The ``Location`` HTTP
	       header will be set to a URL at which the status of the
	       checkpoint can be accessed.

.. http:get:: /coll/(collection_name)/checkpoint/(checkpoint_id)

   Get the status of a checkpoint.  If the checkpoint doesn't exist (or has
   expired), or the collection doesn't exist, returns a null JSON value.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.
   :param checkpoint_id: The id of the checkpoint.

   :statuscode 200: If the checkpoint or collection doesn't exist, returns a
	       null JSON value.  Otherwise, returns a JSON object with three
	       members:

	       * `reached`: A boolean, true if the checkpoint has been reached,
		 false otherwise.  If false, no other members will exist in the
		 JSON object.
	       * `total_errors`: The number of errors which has occurred since
		 the last error.  Each error is a JSON object with the
		 following members:

		 * `msg`: A string holding the error message.

		 * `doc_type`: The type of the document that was being
		   processed when the error occurred, or an empty string if no
		   document type is relevant.

		 * `doc_id`: The ID of the document that was being processed
		   when the error occurred, or an empty string if no document
		   ID is relevant.

	       * `errors`: An array of errors.  If very many errors have
		 occurred, only the top few will be returned.

Categories
----------

.. http:get:: /coll/(collection_name)/category

   Get a list of all category hierarchies available in the collection.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.

   :statuscode 200: Returns a JSON array of strings, holding the names of the
               category hierarchies in the collection.

   :statuscode 404: If the collection does not exist.

.. http:get:: /coll/(collection_name)/category/(hierarchy_name)

   Get details of the named hierarchy in the collection.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.
   :param hierarchy_name: The name of the hierarchy.  May not contain ``:/\.,``
          or tab characters.

   :statuscode 200: Returns a JSON object representing the contents of the
               hierarchy, mapping from category ID to an array of parent IDs.

   :statuscode 404: If the collection or hierarchy do not exist.

.. http:get:: /coll/(collection_name)/category/(hierarchy_name)/id/(cat_id)

   Get details of a category in a named hierarchy in the collection.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.
   :param hierarchy_name: The name of the hierarchy.  May not contain ``:/\.,``
          or tab characters.
   :param cat_id: The ID of the category.  May not contain ``:/\.,`` or tab
          characters.

   :statuscode 200: Returns a JSON object representing the contents of the
               hierarchy, mapping from category ID to an array of parent IDs.

   :statuscode 404: If the collection, hierarchy or category do not exist.

.. http:get:: /coll/(collection_name)/category/(hierarchy_name)/id/(cat_id)/parent/(parent_id)

   Check if a category has a given parent, in the named hierarchy in the
   collection.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.
   :param hierarchy_name: The name of the hierarchy.  May not contain ``:/\.,``
          or tab characters.
   :param cat_id: The ID of the category.  May not contain ``:/\.,`` or tab
          characters.
   :param parent_id: The ID of the parent category.  May not contain ``:/\.,``
          or tab characters.

   :statuscode 200: Returns an empty JSON object if the parent supplied is a
               parent of the category supplier.

   :statuscode 404: If the collection, hierarchy or category or parent do not
               exist, or the parent is not a parent of the category.


Documents
---------

.. http:get:: /coll/(collection_name)/type/(type)/id/(id)

   Get the stored information about the document of given ID and type.

   Note that the information returned is not exactly the same as that supplied
   when the document was indexed: the returned information depends on the
   stored fields, but also includes the indexed information about the document.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.
   :param type: The type of the document.
   :param id: The ID of the document.

   :statuscode 200: Normal response: returns a JSON object representing the
	       document.  This object will have some or all of the following
	       properties (properties for which the value would be empty are
	       omitted).

	       * data: A JSON object holding the stored fields, keyed by field
		 name. Each value is an array of the values supplied for that
		 field.  Each item in the array of values may be any JSON
		 value, depending on what was supplied when indexing the field.

	       * terms: A JSON object holding the terms in the document.  Each
		 key is the string representation of a term (escaped using
		 C-style escapes, since terms may be arbitrary binary values),
		 in which the value is another JSON object with information
		 about the occurrence of the term:
		 
		 * If the within-document-frequency of the term is non-zero,
		   the `wdf` key will contain the within-document-frequency, as
		   an integer.

		 * If there are positions stored for the term, the `positions`
		   key will contain an array of integer positions at which the
		   term occurs.

	       * values: A JSON object holding the values in the document.  The
	         keys in this object will be the slot numbers used, and the
	         values will be a string holding a C-style escaped version of
	         the data stored in the value slot.

   :statuscode 404: If the collection, type or document ID doesn't exist:
               returns a standard error object.

.. http:put:: /coll/(collection_name)/type/(type)/id/(id)

   Create, or update, a document with the given `collection_name`, `type` and
   `id`.

   Creates the collection with default settings if it didn't exist before the
   call.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.
   :param type: The type of the document.
   :param id: The ID of the document.

   :statuscode 202: Normal response: returns a JSON object.  This will usually
               be empty, but may contain the following:

	       * ``high_load``: contains an integer value of 1 if the
		 processing queue is busy.  Clients should reduce the rate at
		 which they're sending documents is ``high_load`` messages
		 persist.

.. http:post:: /coll/(collection_name)/type/(type)

   Create, or update, a document with the given `collection_name` and `type`.
   The id of the document will be read from the document body, from the field
   configured in the collection configuration for storing IDs (by default,
   this is `id`).

   Creates the collection with default settings if it didn't exist before the
   call.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.
   :param type: The type of the document.

   :statuscode 202: Normal response: returns a JSON object.  This will usually
               be empty, but may contain the following:

	       * ``high_load``: contains an integer value of 1 if the
		 processing queue is busy.  Clients should reduce the rate at
		 which they're sending documents is ``high_load`` messages
		 persist.

.. http:post:: /coll/(collection_name)/id/(id)

   Create, or update, a document with the given `collection_name` and `id`.
   The type of the document will be read from the document body, from the
   field configured in the collection configuration for storing types (by
   default, this is `type`).

   Creates the collection with default settings if it didn't exist before the
   call.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.
   :param id: The ID of the document.

   :statuscode 202: Normal response: returns a JSON object.  This will usually
               be empty, but may contain the following:

	       * ``high_load``: contains an integer value of 1 if the
		 processing queue is busy.  Clients should reduce the rate at
		 which they're sending documents is ``high_load`` messages
		 persist.

.. http:post:: /coll/(collection_name)

   Create, or update, a document in the collection `collection_name`.  The
   type and ID of the document will be read from the document body, from the
   fields configured in the collection configuration for storing types and
   IDs (by default, these are `type` and `id`).

   Creates the collection with default settings if it didn't exist before the
   call.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.

   :statuscode 202: Normal response: returns a JSON object.  This will usually
               be empty, but may contain the following:

	       * ``high_load``: contains an integer value of 1 if the
		 processing queue is busy.  Clients should reduce the rate at
		 which they're sending documents is ``high_load`` messages
		 persist.

.. http:delete:: /coll/(collection_name)/type/(type)/id/(id)

   Delete a document from a collection.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.
   :param type: The type of the document.
   :param id: The ID of the document.

   :statuscode 202: Normal response: returns a JSON object.  This will usually
               be empty, but may contain the following:

	       * ``high_load``: contains an integer value of 1 if the
		 processing queue is busy.  Clients should reduce the rate at
		 which they're sending documents is ``high_load`` messages
		 persist.

Performing a search
-------------------

Searches are performed by sending a JSON search structure in the request body.
This may be done using a :http:method:`GET` request, but will usually be done
with a :http:method:`POST` request, since not all software supports sending a
body as part of a `GET` request.


.. http:get:: /coll/(collection_name)/search
.. http:post:: /coll/(collection_name)/search

   Search for documents in a collection, across all document types.

   The search is sent as a JSON structure in the request body: see the
   :ref:`searches` section for details on the search structure.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.

   :statuscode 200: Returns the result of running the search, as a JSON
	       structure.  See the :ref:`search_results` section for details on
	       the search result structure.

   :statuscode 404: If the collection is not found.


.. http:get:: /coll/(collection_name)/type/(type)/search
.. http:post:: /coll/(collection_name)/type/(type)/search

   Search for documents in a collection, and with a given document type.

   The search is sent as a JSON structure in the request body: see the
   :ref:`searches` section for details on the search structure.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.,`` or tab characters.
   :param type: The type of the documents to search for.

   :statuscode 200: Returns the result of running the search, as a JSON
	       structure.  See the :ref:`search_results` section for details on
	       the search result structure.

   :statuscode 404: If the collection is not found.


Getting the status of the server
================================

.. http:get:: /status

   Gets details of the status of the server.

   :statuscode 200: Returns a JSON object, with the following items:

    * ``tasks``: Details of the task processing queues and pools in progress.
      This is an object, with an entry for each named group of task queues in
      the system (eg, for "indexing", "processing" and "search").  Each entry
      is an object with the following members:
      
      * ``queues``: Details of the status of the queues in the task queue
	group.  This has an entry for each queue (for the "indexing" and
	"processing" groups, the name of each task queue is the name of the
	corresponding collection.  For the "search" group, the name of each
	task queue corresponds to the name of the task being performed; eg, the
	task which produces this status output is in the "status" group, so
	there will always be a ``status`` entry in the ``search`` group).  Each
	entry is an object with the following members:
      
	* ``active``: (bool) True if the group is actively being processed.
	  False if the group has been deactivated (this is usually done
	  temporarily to avoid overloading target queues - eg, processing
	  queues will be deactivated temporarily if their corresponding
	  indexing queue exceeds a certain size.  They will reactivate
	  automatically when the indexing queue becomes less full).

	* ``assigned``: (bool) True if a worker is assigned exclusively to this
	  group.  This is generally true for indexing tasks, but false for
	  processing and search tasks.

	* ``in_progress``: (int) The number of tasks in the group currently
	  being processed.

	* ``size``: (int) The number of tasks on the queue, waiting to be
	  processed.  This does not include the number of tasks actively being
	  processed (ie, those counted by ``in_progress``).

      * ``threads``: Details of the threads in the thread pool for the queue.
        This has the following members:

	* ``running``: (int) The number of running threads in the thread pool
	  (including threads waiting for tasks).

	* ``size``: (int) The number of threads owned by the pool (including
	  threads shutting down).

	* ``waiting_for_join``: (int) The number of threads in the pool waiting
	  for cleanup after shutting down.

Root and static files
=====================

.. http:get:: /
.. http:get:: /static/(static_path)

   Static files are served from the ``static`` directory.  This is intended for
   hosting pretty web interfaces for server administration and management.

   :statuscode 200: the contents of the file.  Note that the mimetype is
	       guessed from the file extension, and only a very limited set of
	       common extensions are known about currently.

   :statuscode 404: the file was not found.
