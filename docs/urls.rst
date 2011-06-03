
FIXME - this file is largely wrong, and mainly just me thinking "out loud".


Inserting documents
===================

GET a document of given ID and type:

 GET /coll/<collection name>/type/<type>/id/<id>


Insert a JSON document, calculating ID and type from contents.
 
 POST /coll/<collection name>

Insert a raw Xapian document, 

 POST /coll/<collection name/xapdoc

 POST /coll/<collection name>
 POST /coll/<collection name>/pipe/<pipe name>

Classifying the language of a piece of text.

 POST /coll/<collection name>/pipe/<pipe name>

Performing a search
===================

 GET or POST to /coll/<collection name>/search

 Search is sent in the body; see search_json.rst for details.

Getting the status
==================

 GET /status
