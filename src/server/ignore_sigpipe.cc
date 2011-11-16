/** @file ignore_sigpipe.cc
 * @brief Ignore the sigpipe signal.
 */
/* (C) 2011 Richard Boulton
 *
 * Parts of this file are copied verbatim from the libmicrohttpd documentation.
 * No separate license is specified for the libmicrohttpd documentation, so
 * they are included here under the terms of the LGPL.  Specifically:
 *
 * (C) 2007 Christian Grothoff (and other contributing authors)

 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.

 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.

 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>

#ifndef __WIN32__
#include "ignore_sigpipe.h"
#include "utils/rsperrors.h"

#include <cstdio>
#include <cerrno>
#include <signal.h>

// An empty signal handler.
static void
catcher(int /* sig */)
{
}

// Set a handler for SIGPIPE which ignores it.
void
ignore_sigpipe()
{
    struct sigaction oldsig;
    struct sigaction sig;

    sig.sa_handler = &catcher;
    sigemptyset (&sig.sa_mask);
#ifdef SA_INTERRUPT
    sig.sa_flags = SA_INTERRUPT;  /* SunOS */
#else
    sig.sa_flags = SA_RESTART;
#endif
    if (0 != sigaction (SIGPIPE, &sig, &oldsig)) {
	throw RestPose::SysError("Failed to install SIGPIPE handler", errno);
    }
}
#endif
