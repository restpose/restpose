/** @file default_routes.cc
 * @brief Setup the routes to be used.
 */
/* Copyright (c) 2011 Richard Boulton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <config.h>
#include "rest/default_routes.h"

#include "httpserver/httpserver.h"
#include "rest/handlers.h"
#include "rest/router.h"

using namespace RestPose;

void
setup_routes(Router & router)
{
    router.add("/", HTTP_GETHEAD, new RootHandlerFactory);
    router.add("/static/*", HTTP_GETHEAD, new FileHandlerFactory);
    router.add("/status", HTTP_GETHEAD, new ServerStatusHandlerFactory);
    router.add("/coll", HTTP_GETHEAD, new CollListHandlerFactory);
    router.add("/coll/?", HTTP_GETHEAD, new CollInfoHandlerFactory);
    router.add("/coll/?", HTTP_PUT, new CollCreateHandlerFactory);
    router.add("/coll/?/type/?/id/?", HTTP_PUT, new IndexDocumentHandlerFactory);
    router.add("/coll/?/type/?/id/?", HTTP_GET, new GetDocumentHandlerFactory);
    router.add("/coll/?/type/?/search", HTTP_GETHEAD | HTTP_POST, new SearchHandlerFactory);
    router.set_default(new NotFoundHandlerFactory);
}
