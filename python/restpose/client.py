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
    >>> coll.status
    {}
    >>> coll.add_doc(doc, type="blurb")
    >>> coll.checkpoint()

"""

from .resource import RestPoseResource
import re

coll_name_re = re.compile('^[a-z0-9_-]+$')

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

class Collection(object):
    def __init__(self, server, coll_name):
        if coll_name_re.match(coll_name) is None:
            raise ValueError("Invalid character in collection name: names may "
                             "contain only lowercase letters, numbers, "
                             "underscore and hyphen.")
        self._basepath = '/coll/' + coll_name
        self._resource = server._resource

    @property
    def status(self):
        """Get the status of the collection.

        """
        return self._resource.get(self._basepath).json

    def add_doc(self, doc, type=None):
        """Add a document to the collection.

        """
        return self._resource.post(self._basepath + "/type/" + type, payload=doc).json

    def checkpoint(self, checkid=None):
        """Set a checkpoint to the collection.

        This creates a resource on the server which can be queried to detect
        whether indexing has reached the checkpoint yet.  When

        """
        return self._resource.post(self._basepath + "/checkpoint").json
