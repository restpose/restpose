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
          ``:/\.`` or tab characters.

   On success, the return value is a JSON object with the following members:

    * ``doc_count``: The number of documents in the collection.

   :statuscode 200: If the collection exists, and no errors occur.
   :statuscode 404: If the collection does not exist.  Returns a standard error object.


Deleting a collection
---------------------

.. http:delete:: /coll/(collection_name)

   :param collection_name: The name of the collection.  May not contain
          ``:/\.`` or tab characters.

   The collection of the given name is deleted, if it exists.  If it doesn't
   exist, returns successfully, but doesn't change anything.

   :statuscode 200: Normal response: returns an empty JSON object.


Collection configuration
------------------------

.. http:get:: /coll/(collection_name)/config

   Get the collection configuration.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.`` or tab characters.

   :statuscode 200: Normal response: returns a JSON object representing the
	       full configuration for the collection.  See :ref:`coll_config`
	       for  details.

   :statuscode 404: If the collection does not exist.  Returns a standard error object.

.. http:put:: /coll/(collection_name)/config

   Set the collection configuration.  Actually, adds a task to set the
   collection configuration to the processing queue.  This may be monitored,
   waited for, and committed using checkpoints in just the same way as for the
   document addition APIs.

   Creates the collection it it didn't exist before the call.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.`` or tab characters.

   :statuscode 202: Normal response: returns a JSON object representing the
	       full configuration for the collection.  See :ref:`coll_config`
	       for  details.


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
          ``:/\.`` or tab characters.

   :statuscode 200: Normal response: returns a JSON array of strings, each
	       string is the ID of a checkpoint on the collection.  If the
	       collection doesn't exist, returns an empty array.

.. http:post:: /coll/(collection_name)/checkpoint

   Create a checkpoint.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.`` or tab characters.

   :queryparam commit: (boolean). True if the checkpoint should cause a commit,
               False if the checkpoint should not cause a commit.

   :statuscode 201: Normal response: returns a JSON object containing a single
	       item, with a key of `checkid` and a value being a string used to
	       identify the newly created checkpoint.

.. http:get:: /coll/(collection_name)/checkpoint/(checkpoint_id)

   Get the status of a checkpoint.  If the checkpoint doesn't exist (or has
   expired), or the collection doesn't exist, returns a null JSON value.

   :param collection_name: The name of the collection.  May not contain
          ``:/\.`` or tab characters.
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

Documents
---------

FIXME - after this point, details of calls need to be added.

.. http:get:: /coll/(collection_name)/type/(type)/id/(id)

   Get a document of given ID and type.

.. http:put:: /coll/(collection_name)/type/(type)/id/(id)

   Creates the collection with default settings it it didn't exist before the
   call.

.. http:delete:: /coll/(collection_name)/type/(type)/id/(id)

To be added in future:

Insert a JSON document, calculating ID and type from contents.
 
 POST /coll/<collection name>

Insert a raw Xapian document, 

 POST /coll/<collection name/xapdoc

 POST /coll/<collection name>
 POST /coll/<collection name>/pipe/<pipe name>

Classifying the language of a piece of text.

 POST /coll/<collection name>/pipe/<pipe name>

Performing a search
-------------------

.. http:get:: /coll/(collection_name)/type/(type)/search
.. http:post:: /coll/(collection_name)/type/(type)/search

   Search is sent in the body; see search_json.rst for details.

Getting the status of the server
================================

.. http:get:: /status

Root and static files
=====================

.. http:get:: /

.. http:get:: /static/(static_path)
