

version_info = (0, 6, 0) # FIXME: automatically set from restpose configure.ac
__version__ =  ".".join(map(str, version_info))

from .client import Server

from restkit import ResourceNotFound, Unauthorized, RequestFailed, \
                    RedirectLimit, RequestError, InvalidUrl, \
                    ResponseError, ProxyError, ResourceError
