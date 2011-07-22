# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.
"""Version information.

"""

#: The development release suffix.  Will be an empty string for releases.
dev_release = "dev"

#: The version of this restpose client, as a tuple of numbers.
#: This does not include information about whether this is a development
#: release.
version_info = (0, 7, 0)

#: The version of this restpose client, as a string.
#: This will have a 
__version__ =  ".".join(map(str, version_info)) + dev_release
