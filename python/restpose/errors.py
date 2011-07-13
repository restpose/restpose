# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

"""
Errors specific to RestPose.

"""

class RestPoseError(Exception):
    pass


class CheckPointExpiredError(RestPoseError):
    """An error raised when a checkpoint has expired.

    """
    pass
