# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

"""
Resources for RestPose.

This module provides a convenient interface to the resources exposed via HTTP
by the RestPose server.

"""

from version import __version__
import restkit
try:
    import simplejson as json
except ImportError:
    import json

class RestPoseResponse(restkit.Response):
    """A response from the RestPose server.

    In addition to the properties exposed by :mod:`restkit:restkit.Response`, this
    exposes a `json` property, to decode JSON responses automatically.
    
    """

    @property
    def json(self):
        """Get the response body as JSON.

        :returns: The response body as a python object, decoded from JSON, if
                  the response Content-Type was application/json.  Otherwise,
                  return None.
        
        :raises: an exception if the Content-Type is application/json, but the
                 body is not valid JSON.

        """
        ctype = self.headers.get('Content-Type')
        if ctype == 'application/json':
            return json.loads(self.body_string())
        return None

    def expect_status(self, *expected):
        """Check that the status code is one of a set of expected codes.

        :param expected: The expected status codes.

        :raises: :exc:`RestPoseError` if the status code returned is not one of the supplied status codes.

        """
        if self.status_int not in expected:
            raise RestPoseError("Unexpected return status: %d" %
                                resp.status_int)


class RestPoseResource(restkit.Resource):
    """A resource providing access to a RestPose server.

    This may be subclassed and provided to :class:`restpose.Server`, to
    allow requests to be monitored or modified.  For example, a logging
    subclass could be used to record requests and their responses.

    """

    #: The user agent to send when making requests.
    user_agent = 'restpose_python/%s' % __version__

    def __init__(self, uri, **client_opts):
        """Initialise the resource.

        :param uri: The full URI for the resource.

        :param client_opts: Any options to be passed to :class:`restkit.Resource`.

        """
        client_opts['response_class'] = RestPoseResponse
        restkit.Resource.__init__(self, uri=uri, **client_opts)

    def request(self, method, path=None, payload=None, headers=None, **params):
        """Perform a request.

        :param method: the HTTP method to use, as a string

        .. todo:: document all params

        """

        headers = headers or {}
        headers.setdefault('Accept', 'application/json')
        headers.setdefault('User-Agent', self.user_agent)

        if payload is not None:
            if not hasattr(payload, 'read') and \
               not isinstance(payload, basestring):
                payload = json.dumps(payload).encode('utf-8')
                headers.setdefault('Content-Type', 'application/json')

        try:
            resp = restkit.Resource.request(self, method, path=path,
                                            payload=payload, headers=headers,
                                            **params)
        except restkit.ResourceError, e:
            # Unpack any errors which are in JSON format.
            msg = getattr(e, 'msg', '')
            msgobj = None
            if e.response and msg:
                ctype = e.response.headers.get('Content-Type')
                if ctype == 'application/json':
                    try:
                        msgobj = json.loads(msg)
                    except ValueError:
                        pass
            if msgobj is not None:
                e.msg = msgobj['err']
            e.msgobj = msgobj
            raise
            
        return resp
