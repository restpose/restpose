# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.
"""Python client for the RestPose search server.

"""

from .client import Server
from .version import dev_release, version_info, __version__

from restkit import ResourceNotFound, Unauthorized, RequestFailed, \
                    RedirectLimit, RequestError, InvalidUrl, \
                    ResponseError, ProxyError, ResourceError
