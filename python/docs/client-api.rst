Client
======

The central class involved is the :class:`restpose.Server <restpose.client.Server>` class.

.. py:currentmodule:: restpose.client

Servers provide access to a set of collection resources.  A collection can be
accessed using the :meth:`Server.collection()` method.

    >>> from restpose import Server
    >>> server = Server()
    >>> doc = { 'text': 'Hello world', 'tag': 'A tag' }
    >>> coll = server.collection("my_coll")
    >>> coll.add_doc(doc, doc_type="blurb", doc_id="1")

After making changes, we use a checkpoint to cause the changes to be applied
immediately, and to wait until they have been applied.  The changes will be
applied after a short period of inactivity (by default, 5 seconds) even if we
don't use a checkpoint.

    >>> checkpt = coll.checkpoint().wait()
    >>> checkpt.total_errors, checkpt.errors, checkpt.reached
    (0, [], True)

Once the changes have been applied, we can perform a search by building a
query.

    >>> query = coll.doc_type("blurb").field_is('tag', 'A tag')
    >>> query.matches_estimated
    1
    >>> query[0].data['id']
    ['1']
    >>> query[0].data['type']
    ['blurb']


