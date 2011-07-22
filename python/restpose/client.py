# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

"""
The RestPose client mirrors the resources provided by the RestPose server as
Python objects.

"""

from .resource import RestPoseResource
from .query import Query, QueryAll, QueryNone, QueryField, QueryMeta, \
                   SearchResults
from .errors import RestPoseError, CheckPointExpiredError

class Server(object):
    """Representation of a RestPose server.

    Allows indexing, searching, status management, etc.

    """

    _resource_class = RestPoseResource

    def __init__(self, uri='http://127.0.0.1:7777',
                 resource_class=None,
                 resource_instance=None,
                 **client_opts):
        """
        :param uri: Full URI to the top path of the server.

        :param resource_class: If specified, defines a resource class to use
               instead of the default class.  This should usually be a subclass
               of :class:`RestPoseResource`.

        :param resource_instance: If specified, defines a resource instance to
               use instead of making one with the default class (or the class
               specified by `resource_class`.

        :param client_opts: Parameters to use to update the existing
               client_opts in the resource (if `resource_instance` is
               specified), or to use when creating the resource (if
               `resource_class` is specified).

        """
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

        Returns a dictionary.

        .. todo:: Document the contents of the response.

        """
        return self._resource.get('/status').json

    @property
    def collections(self):
        """Get a list of existing collections.

        Returns a list of collections

        """
        resp = self._resource.get('/coll')
        resp.expect_status(200)
        return resp.json.keys()

    def collection(self, coll_name):
        """Access to a collection.

        :param coll_name: The name of the collection to access.

        :returns: a Collection object which can be used to search and modify the
                  contents of the Collection.

        .. note:: No request is performed directly by this method; a Collection
                  object is simply created which will make requests when
                  needed.  For this reason, no error will be reported at this
                  stage even if the collection does not exist, or if a
                  collection name containing invalid characters is used.

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

    def search(self, search):
        """Perform a search.

        @param search is a search structure to be sent to the server, or a
        Search or Query object.

        """
        if hasattr(search, '_build_search'):
            body = search._build_search()
        else:
            body = search
        result = self._resource.post(self._basepath + "/search",
                                     payload=body).json
        return SearchResults(result)


class Document(object):
    def __init__(self, collection, doc_type, doc_id):
        if collection is None:
            # doc_type should be a DocumentType object.
            self._resource = doc_type._resource
            self._path = doc_type._basepath + '/id/' + doc_id
        else:
            # doc_type should be a string.
            self._resource = collection._resource
            self._path = collection._basepath + '/type/' + doc_type + '/id/' + doc_id
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
    def __init__(self, collection, doc_type):
        self._basepath = collection._basepath + '/type/' + doc_type
        self._resource = collection._resource

    def add_doc(self, doc, doc_id=None):
        """Add a document to the collection.

        """
        path = self._basepath
        use_put = True
        if doc_id is None:
            use_put = False
        else:
            path += '/id/%s' % doc_id
        if use_put:
            resp = self._resource.put(path, payload=doc).json
        else:
            resp = self._resource.post(path, payload=doc).json
        if resp.status_int != 202:
            raise RestPoseError("Unexpected return status from add_doc: %d" %
                                resp.status_int)

    def get_doc(self, doc_id):
        return Document(None, self, doc_id)


class Collection(QueryTarget):
    def __init__(self, server, coll_name):
        self._basepath = '/coll/' + coll_name
        self._resource = server._resource

    def doc_type(self, doc_type):
        return DocumentType(self, doc_type)

    @property
    def status(self):
        """The status of the collection.

        """
        return self._resource.get(self._basepath).json

    @property
    def config(self):
        """The configuration of the collection.

        """
        return self._resource.get(self._basepath + '/config').json

    @config.setter
    def config(self, value):
        resp = self._resource.put(self._basepath + '/config', payload=value)
        if resp.status_int != 202:
            raise RestPoseError("Unexpected return status from config: %d" %
                                resp.status_int)

    def add_doc(self, doc, doc_type=None, doc_id=None):
        """Add a document to the collection.

        """
        path = self._basepath
        use_put = True

        if doc_type is None:
            use_put = False
        else:
            path += '/type/%s' % doc_type

        if doc_id is None:
            use_put = False
        else:
            path += '/id/%s' % doc_id

        if use_put:
            resp = self._resource.put(path, payload=doc)
        else:
            resp = self._resource.post(path, payload=doc)
        if resp.status_int != 202:
            raise RestPoseError("Unexpected return status from add_doc: %d" %
                                resp.status_int)


    def delete_doc(self, doc_type, doc_id):
        """Delete a document from the collection.

        """
        path = '%s/type/%s/id/%s' % (self._basepath, doc_type, doc_id)
        return self._resource.delete(path).json

    def get_doc(self, doc_type, doc_id):
        """Get a document from the collection.

        """
        return Document(self, doc_type, doc_id)

    def checkpoint(self, commit=True):
        """Set a checkpoint on the collection.

        This creates a resource on the server which can be queried to detect
        whether indexing has reached the checkpoint yet.  All updates sent
        before the checkpoint will be processed before indexing reaches the
        checkpoint, and no updates sent after the checkpoint will be processed
        before indexing reaches the checkpoint.

        """
        path = self._basepath + "/checkpoint"
        params_dict = {}
        if commit:
            params_dict['commit'] = '1'
        else:
            params_dict['commit'] = '0'
        return CheckPoint(self, self._resource.post(path,
                            params_dict=params_dict).json)

    def delete(self):
        """Delete the entire collection.

        """
        self._resource.delete(self._basepath)


class CheckPoint(object):
    """A checkpoint, used to check the progress of indexing.

    """
    def __init__(self, collection, chkpt_response):
        self._check_id = chkpt_response.get('checkid')
        self._basepath = collection._basepath + '/checkpoint/' + self._check_id
        self._resource = collection._resource

        # The raw representation of the checkpoint, as returned from the
        # request, or None if the checkpoint hasn't been reached or expired, or
        # 'expired' if the checkpoint has expired.
        self._raw = None

    @property
    def check_id(self):
        """The ID of the checkpoint.

        This is used to identify the checkpoint on the server.

        """
        return self._check_id

    def _refresh(self):
        """Contact the server, and get the status of the checkpoint.

        If the checkpoint has been reached or expired, this doesn't contact the
        server, since the checkpoint should no longer change at this point.

        """
        if self._raw is None:
            resp = self._resource.get(self._basepath).json
            if resp is None:
                self._raw = 'expired'
            elif resp.get('reached', False):
                self._raw = resp

        if self._raw == 'expired':
            raise CheckPointExpiredError("Checkpoint %s expired" %
                                         self.check_id)

    @property
    def reached(self):
        """Return true if the checkpoint has been reached.

        May contact the server to check the current state.

        Raises CheckPointExpiredError if the checkpoint expired before the
        state was checked.

        """
        self._refresh()
        return self._raw is not None and self._raw != 'expired'

    @property
    def errors(self):
        """Return the list of errors associated with the CheckPoint.

        Note that if there are many errors, only the first few will be
        returned.

        Returns None if the checkpoint hasn't been reached yet.

        Raises CheckPointExpiredError if the checkpoint expired before the
        state was checked.

        """
        self._refresh()
        if self._raw is None:
            return None
        return self._raw.get('errors', [])

    @property
    def total_errors(self):
        """Return the total count of errors associated with the CheckPoint.

        This may be larger than len(self.errors), if there were more errors
        than the CheckPoint is able to hold.

        Returns None if the checkpoint hasn't been reached yet.

        Raises CheckPointExpiredError if the checkpoint expired before the
        state was checked.

        """
        self._refresh()
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
            if self._raw is not None:
                return self
            # FIXME - sleep a bit.  Currently the server doesn't long-poll for
            # the checkpoint, so we need to sleep to avoid using lots of CPU.
            import time
            time.sleep(1)
