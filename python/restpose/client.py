# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

"""
Client for RestPose.

Example:

    >>> from restpose import Server
    >>> server = Server()
    >>> status = server.status()
    >>> doc = { 'text': 'Hello world', 'tag': 'A tag' }
    >>> coll = server.collection("my_coll")
    >>> coll.add_doc(doc, type="blurb", docid="1")
    >>> #chkpt_id = coll.checkpoint()
    >>> #coll.waitfor(chkpt_id)
    >>> query = coll.type("blurb").field_is('tag', 'A tag')
    >>> results = query.search().do()
    >>> str(results)
    "SearchResults(offset=0, size=10, checkatleast=0, matches_lower_bound=1, matches_estimated=1, matches_upper_bound=1, items=[SearchResult(rank=0, fields={'text': ['Hello world'], 'tag': ['A tag'], 'id': ['1']})])"

    >>> results = query.search(info=[{'cooccur': {'prefix': 't'}}]).do()
    >>> str(results)
    "SearchResults(offset=0, size=10, checkatleast=4294967295, matches_lower_bound=1, matches_estimated=1, matches_upper_bound=1, items=[SearchResult(rank=0, fields={'text': ['Hello world'], 'tag': ['A tag'], 'id': ['1']})], info=[{'type': 'cooccur', 'counts': [['hello', 'world', 1]], 'terms_seen': 2, 'docs_seen': 1, 'prefix': 't'}])"

"""

from .resource import RestPoseResource
from .query import Query, QueryAll, QueryNone, QueryField, \
                   Search, SearchResults
import re

coll_name_re = re.compile('^[a-z0-9_-]+$')
doc_type_re = re.compile('^[a-z0-9_-]+$')

class Server(object):
    """Representation of a RestPose server.

    Allows indexing, searching, status management, etc.

    """

    _resource_class = RestPoseResource

    def __init__(self, uri='http://127.0.0.1:7777',
                 resource_class=None,
                 resource_instance=None,
                 **client_opts):
        self.uri = uri = uri.rstrip('/')

        if resource_class is not None:
            self._resource_class = resource_class

        if resource_instance:
            self._resource = resource_instance.clone()
            self._resource.initial['uri'] = uri
            self._resource.client_opts.update(client_opts)
        else:
            self._resource = self._resource_class(uri, **client_opts)

    def status(self):
        """Get server status.

        Returns a dictionary

        """
        return self._resource.get('/status').json

    def collection(self, coll_name):
        """Access to a collection.

        Returns a Collection object which can be used to search and modify the
        contents of the Collection.

        Collection names may contain only the following ascii characters:
        lowercase letters, numbers, underscore, hyphen.

        """
        return Collection(self, coll_name)

class QueryTarget(object):
    """An object which can be used to make and run queries.

    """

    def query_all(self):
        """Create a query which matches all documents."""
        return QueryAll(target=self._collection)

    def query_none(self):
        """Create a query which matches no documents."""
        return QueryNone(target=self)

    def query_field(self, fieldname, querytype, value):
        """Create a query based on the value in a field.

        See the documentation of the restpose query types for the possible
        values of querytype, and the corresponding structure which should be
        passed in value.

        Usually, rather than calling this method directly, it will be more
        convenient to call one of the field_*() methods to construct the
        appropriate query, which use this method internally.

        """
        return QueryField(fieldname, querytype, value, target=self)

    def field_is(self, fieldname, value):
        """Create a query for an exact value in a named field.

        This query type is available for "exact" and "id" field types.

        """
        if isinstance(value, (basestring, int)):
            value = (value,)
        return self.query_field(fieldname, 'is', value)

    def field_range(self, fieldname, begin, end):
        """Create a query for field values in a given range.

        This query type is currently only available for "date" field types.

        """
        return self.query_field(fieldname, 'range', (begin, end))

    def field_text(self, fieldname, text, op="phrase", window=None):
        """Create a query for a piece of text in a field.

         - @param "fieldname": the field to search within.
         - @param "text": The text to search for.  If empty, this query will
           match no results.
         - @param "op": The operator to use when searching.  One of "or",
           "and", "phrase" (ordered proximity), "near" (unordered proximity).
           Default="phrase".
         - @param "window": only relevant if op is "phrase" or "near". Window
           size in words within which the words in the text need to occur for a
           document to match; None=length of text. Integer or None.
           Default=None

        """
        value = dict(text=text)
        if op is not None:
            value['op'] = op
        if window is not None:
            value['window'] = window
        return self.query_field(fieldname, 'text', value)

    def field_parse(self, fieldname, text, op="and"):
        """Parse a structured query.

         - @param "fieldname": the field to search within.
         - @param "text": text to search for.  If empty, this query will match
           no results.
         - @param "op": The default operator to use when searching.  One of
           "or", "and".  Default="and"

        """
        value = dict(text=text)
        if op is not None:
            value['op'] = op
        return self.query_field(fieldname, 'parse', value)

    def search(self, query):
        """Perform a search.

        """
        if isinstance(query, Query):
            body = Search(target=self, query=query).body
        elif isinstance(query, Search):
            body = query.body
        else:
            body = query
        result = self._resource.post(self._basepath + "/search",
                                     payload=body).json
        # FIXME - wrap result in a more useful wrapper
        return SearchResults(result)


class DocumentType(QueryTarget):
    def __init__(self, collection, doc_type):
        if doc_type_re.match(doc_type) is None:
            raise ValueError("Invalid character in type name: names may "
                             "contain only lowercase letters, numbers, "
                             "underscore and hyphen.")
        self._basepath = collection._basepath + '/type/' + doc_type
        self._resource = collection._resource

    def add_doc(self, doc, docid=None):
        """Add a document to the collection.

        """
        path = self._basepath
        use_put = True
        if docid is None:
            use_put = False
        else:
            path += '/id/%s' % docid
        if use_put:
            self._resource.put(path, payload=doc).json
        else:
            self._resource.post(path, payload=doc).json


class Collection(QueryTarget):
    def __init__(self, server, coll_name):
        if coll_name_re.match(coll_name) is None:
            raise ValueError("Invalid character in collection name: names may "
                             "contain only lowercase letters, numbers, "
                             "underscore and hyphen.")
        self._basepath = '/coll/' + coll_name
        self._resource = server._resource

    def type(self, doc_type):
        return DocumentType(self, doc_type)

    @property
    def status(self):
        """Get the status of the collection.

        """
        return self._resource.get(self._basepath).json

    def add_doc(self, doc, type=None, docid=None):
        """Add a document to the collection.

        """
        path = self._basepath
        use_put = True

        if type is None:
            use_put = False
        else:
            path += '/type/%s' % type

        if docid is None:
            use_put = False
        else:
            path += '/id/%s' % docid

        if use_put:
            self._resource.put(path, payload=doc).json
        else:
            self._resource.post(path, payload=doc).json

    def checkpoint(self, checkid=None):
        """Set a checkpoint to the collection.

        This creates a resource on the server which can be queried to detect
        whether indexing has reached the checkpoint yet.  All updates sent
        before the checkpoint will be processed before indexing reaches the
        checkpoint, and no updates sent after the checkpoint will be processed
        before indexing reaches the checkpoint.

        """
        # FIXME - unimplemented here, or on server, so far.
        return CheckPoint(self._resource.post(self._basepath + "/checkpoint").json)
