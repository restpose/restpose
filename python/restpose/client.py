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
    >>> coll.add_doc(doc, type="blurb", id="1")
    >>> checkpt = coll.checkpoint().wait()
    >>> checkpt.total_errors, checkpt.errors, checkpt.reached, checkpt.expired
    (0, [], True, False)
    >>> query = coll.type("blurb").field_is('tag', 'A tag')
    >>> results = query.search().do()
    >>> results.matches_estimated
    1
    >>> results.items[0].data['id']
    ['1']
    >>> results.items[0].data['type']
    ['blurb']

"""

from .resource import RestPoseResource
from .query import Query, QueryAll, QueryNone, QueryField, QueryMeta, \
                   Search, SearchResults
from .errors import CheckPointExpiredError

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
        return QueryAll(target=self)

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

    def query_meta(self, querytype, value):
        """Create a query based on the meta information about field presence.

        See the documentation of the restpose query types for the possible
        values of querytype, and the corresponding structure which should be
        passed in value.

        Usually, rather than calling this method directly, it will be more
        convenient to call one of the field_*() methods to construct the
        appropriate query, which use this method internally.

        """
        return QueryMeta(querytype, value, target=self)

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

    def field_exists(self, fieldname=None):
        """Search for documents in which a given field exists.

        If the fieldname supplied is None, searches for documents in which any
        field exists.

        """
        return self.query_meta('exists', (fieldname,))

    def field_nonempty(self, fieldname=None):
        """Search for documents in which a given field has a non-empty value.

        If the fieldname supplied is None, searches for documents in which any
        field has a non-empty value.

        """
        return self.query_meta('nonempty', (fieldname,))

    def field_empty(self, fieldname=None):
        """Search for documents in which a given field has an empty value.

        If the fieldname supplied is None, searches for documents in which any
        field has an empty value.

        """
        return self.query_meta('empty', (fieldname,))

    def field_has_error(self, fieldname=None):
        """Search for documents in which a given field produced errors when
        parsing.

        If the fieldname supplied is None, searches for documents in which any
        field produced errors when parsing.

        """
        return self.query_meta('error', (fieldname,))

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
        return SearchResults(result)


class Document(object):
    def __init__(self, collection, type, id):
        if collection is None:
            # type should be a DocumentType object.
            self._resource = type._resource
            self._path = type._basepath + '/id/' + id
        else:
            # type should be a string.
            self._resource = collection._resource
            self._path = collection._basepath + '/type/' + type + '/id/' + id
        self._data = None
        self._terms = None
        self._values = None
        self._raw = None

    def _fetch(self):
        self._raw = self._resource.get(self._path).json
        self._data = self._raw.get('data', {})
        self._terms = self._raw.get('terms', {})
        self._values = self._raw.get('values', {})

    @property
    def data(self):
        if self._raw is None:
            self._fetch()
        return self._data

    @property
    def terms(self):
        if self._raw is None:
            self._fetch()
        return self._terms

    @property
    def values(self):
        if self._raw is None:
            self._fetch()
        return self._values


class DocumentType(QueryTarget):
    def __init__(self, collection, type):
        self._basepath = collection._basepath + '/type/' + type
        self._resource = collection._resource

    def add_doc(self, doc, id=None):
        """Add a document to the collection.

        """
        path = self._basepath
        use_put = True
        if id is None:
            use_put = False
        else:
            path += '/id/%s' % id
        if use_put:
            result = self._resource.put(path, payload=doc).json
        else:
            result = self._resource.post(path, payload=doc).json

    def get_doc(self, id):
        return Document(None, self, id)


class Collection(QueryTarget):
    def __init__(self, server, coll_name):
        self._basepath = '/coll/' + coll_name
        self._resource = server._resource

    def type(self, type):
        return DocumentType(self, type)

    @property
    def status(self):
        """Get the status of the collection.

        """
        return self._resource.get(self._basepath).json

    def add_doc(self, doc, type=None, id=None):
        """Add a document to the collection.

        """
        path = self._basepath
        use_put = True

        if type is None:
            use_put = False
        else:
            path += '/type/%s' % type

        if id is None:
            use_put = False
        else:
            path += '/id/%s' % id

        if use_put:
            self._resource.put(path, payload=doc).json
        else:
            self._resource.post(path, payload=doc).json

    def delete_doc(self, type, id):
        """Delete a document from the collection.

        """
        path = '%s/type/%s/id/%s' % (self._basepath, type, id)
        return self._resource.delete(path).json

    def get_doc(self, type, id):
        """Get a document from the collection.

        """
        return Document(self, type, id)

    def checkpoint(self):
        """Set a checkpoint on the collection.

        This creates a resource on the server which can be queried to detect
        whether indexing has reached the checkpoint yet.  All updates sent
        before the checkpoint will be processed before indexing reaches the
        checkpoint, and no updates sent after the checkpoint will be processed
        before indexing reaches the checkpoint.

        """
        return CheckPoint(self,
            self._resource.post(self._basepath + "/checkpoint").json)

class CheckPoint(object):
    """A checkpoint, used to check the progress of indexing.

    """
    def __init__(self, collection, chkpt_response):
        self._check_id = chkpt_response.get('checkid')
        self._basepath = collection._basepath + '/checkpoint/' + self._check_id
        self._resource = collection._resource
        self._raw = None

    @property
    def check_id(self):
        """The ID of the checkpoint.

        This is used to identify the checkpoint on the server.

        """
        return self._check_id

    def _refresh(self):
        """Contact the server, and get the status of the checkpoint.

        """
        res = self._resource.get(self._basepath).json
        if res is None:
            self._raw = 'expired'
        elif res.get('reached', False):
            self._raw = res

    @property
    def expired(self):
        """Return true if a checkpoint has expired.

        Contacts the server to check the current state.

        """
        if self._raw != 'expired':
            self._refresh()
        return self._raw == 'expired'

    @property
    def reached(self):
        """Return true if the checkpoint has been reached.

        Contacts the server to check the current state.

        """
        if self._raw is None:
            self._refresh()
        return self._raw is not None

    @property
    def errors(self):
        """Return the list of errors associated with the CheckPoint.

        Note that if there are many errors, only the first few will be
        returned.

        Returns None if the checkpoint hasn't been reached yet.

        """
        if self._raw is None:
            return None
        return self._raw.get('errors', [])

    @property
    def total_errors(self):
        """Return the total count of errors associated with the CheckPoint.

        This may be larger than len(self.errors), if there were more errors
        than the CheckPoint is able to hold.

        Returns None if the checkpoint hasn't been reached yet.

        """
        if self._raw is None:
            return None
        return self._raw.get('total_errors', 0)

    def wait(self):
        """Wait for the checkpoint to be reached.

        This will contact the server, and wait until the checkpoint has been
        reached.

        If the checkpoint expires (before or during the call), a
        CheckPointExpiredError will be raised.  Otherwise, this will return the
        checkpoint, so that further methods can be chained on it.

        """
        while True:
            self._refresh()
            res = self._resource.get(self._basepath).json
            if res.get('reached', False):
                self._raw = res
                return self
            # FIXME - sleep a bit.  Currently the server doesn't long-poll for
            # the checkpoint, so we need to sleep to avoid using lots of CPU.
            import time
            time.sleep(1)
        return self
