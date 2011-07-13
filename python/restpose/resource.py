# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

"""
Resources for RestPose.

FIXME - document more.

"""

from . import __version__
import restkit
try:
    import simplejson as json
except ImportError:
    import json

USER_AGENT = 'restpose_python/%s' % __version__

class RestPoseResponse(restkit.Response):
    @property
    def json(self):
        """Get the response body as JSON.
        
        If the response Content-Type was application/json, convert the
        response body to JSON and return it.  Otherwise, return None.

        """
        ctype = self.headers.get('Content-Type')
        if ctype == 'application/json':
            return json.loads(self.body_string())
        return None


class RestPoseResource(restkit.Resource):
    def __init__(self, uri, **client_opts):
        client_opts['response_class'] = RestPoseResponse
        restkit.Resource.__init__(self, uri=uri, **client_opts)

    def request(self, method, path=None, payload=None, headers=None, **params):

        headers = headers or {}
        headers.setdefault('Accept', 'application/json')
        headers.setdefault('User-Agent', USER_AGENT)

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
