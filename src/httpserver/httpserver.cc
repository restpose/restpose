/** @file httpserver.cc
 * @brief HTTP server, using libmicrohttpd
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
#include "httpserver.h"
#include "response.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>

#include "omassert.h"
#include "rest/handler.h"
#include "rest/router.h"
#include "server/ignore_sigpipe.h"
#include "utils/jsonutils.h"
#include "utils/rsperrors.h"
#include <xapian.h>

using namespace std;
using namespace RestPose;

Response::Response()
	: response(NULL),
	  status_code(MHD_HTTP_OK)
{}

Response::~Response()
{
    if (response) {
	MHD_destroy_response(response);
    }
}

void
Response::set_status(int status_code_)
{
    status_code = status_code_;
}

void
Response::set_data(const string & outbuf_)
{
    // Copy the output buffer into the response object, so it is available
    // until after the response has been sent.
    outbuf = outbuf_;

    if (response) {
	MHD_destroy_response(response);
	response = NULL;
    }
    response = MHD_create_response_from_buffer(outbuf.size(),
	const_cast<char *>(outbuf.data()),
	MHD_RESPMEM_PERSISTENT);
}

void
Response::set_content_type(string content_type)
{
    // Copy the content type into the response object, so it is available
    // until after the response has been sent.
    add_header("Content-Type", content_type);
}

void
Response::add_header(string header, string value)
{
    if (!response) {
	throw RestPose::HTTPServerError("Need to set response body before setting headers");
    }
    if (MHD_add_response_header(response,
				header.c_str(),
				value.c_str()) == MHD_NO) {
	throw RestPose::HTTPServerError("Can't set header '" + header + "' to '" + value + "'");
    }
}

void
Response::set(const Json::Value & body, int status_code_)
{
    set_data(json_serialise(body));
    set_content_type("application/json");
    set_status(status_code_);
}

struct MHD_Response *
Response::get_response()
{
    return response;
}

int
Response::get_status_code() const
{
    return status_code;
}

ConnectionInfo::ConnectionInfo(struct MHD_Connection *connection_,
			       const char * method_,
			       const char * url_,
			       const char * version_)
	: connection(connection_),
	  method(HTTP_UNKNOWN),
	  url(url_),
	  version(version_),

	  first_call(true),
	  responded(false),
	  handler(NULL)
{
    // Assume that the methods are usually one of HEAD, GET, DELETE, POST,
    // PUT and don't waste time checking more than we need to.
    switch (method_[0]) {
	case 'H': method = HTTP_HEAD; break;
	case 'G': method = HTTP_GET; break;
	case 'D': method = HTTP_DELETE; break;
	case 'P': switch(method_[1]) {
	    case 'O': method = HTTP_POST; break;
	    case 'U': method = HTTP_PUT; break;
	}; break;
    }
}

ConnectionInfo::~ConnectionInfo()
{
    delete handler;
}

void
ConnectionInfo::parse_url_components()
{
    components.clear();

    const char * pos = url + 1;
    while (*pos != '\0') {
	const char * next = strchr(pos + 1, '/');
	if (next == NULL) {
	    components.push_back(string(pos));
	    return;
	}
	components.push_back(string(pos, next - pos));
	pos = next + 1;
    }
    components.push_back(string());
}

void
ConnectionInfo::respond(int status_code,
			const string & outbuf,
			const string & content_type)
{
    Response & response(resulthandle.response());
    response.set_status(status_code);
    response.set_data(outbuf);
    response.set_content_type(content_type);
    respond();
}

void
ConnectionInfo::respond(const ResultHandle & resulthandle_)
{
    resulthandle = resulthandle_;
    respond();
}

void
ConnectionInfo::respond()
{
    Response & response(resulthandle.response());
    struct MHD_Response * response_ptr = response.get_response();
    if (!response_ptr) {
	throw RestPose::HTTPServerError("No response to send");
    }

    if (MHD_queue_response(connection, response.get_status_code(),
			   response_ptr) != MHD_YES) {
	throw RestPose::HTTPServerError("Couldn't queue response");
    }
    responded = true;
}

bool
ConnectionInfo::require_method(int allowed_methods)
{
    if (method & allowed_methods) {
	return true;
    }
    Response & response(resulthandle.response());
    response.set_status(MHD_HTTP_METHOD_NOT_ALLOWED);
    response.set_data("Invalid HTTP method");
    response.set_content_type("text/plain");

    string allowed;
    if (allowed_methods & HTTP_HEAD) allowed += "HEAD,";
    if (allowed_methods & HTTP_GET) allowed += "GET,";
    if (allowed_methods & HTTP_POST) allowed += "POST,";
    if (allowed_methods & HTTP_PUT) allowed += "PUT,";
    if (allowed_methods & HTTP_DELETE) allowed += "DELETE,";
    if (!allowed.empty()) {
	allowed[allowed.size() - 1] = '\0';
    }
    response.add_header("Allow", allowed);

    respond();
    return false;
}

const char *
ConnectionInfo::method_str() const
{
    switch (method) {
	case HTTP_HEAD: return "HEAD";
	case HTTP_GET: return "GET";
	case HTTP_POST: return "POST";
	case HTTP_PUT: return "PUT";
	case HTTP_DELETE: return "DELETE";
	default: return "UNKNOWN";
    }
}

static int
answer_connection_cb(void * cls,
		     struct MHD_Connection *connection,
		     const char *url,
		     const char *method,
		     const char *version,
		     const char *upload_data,
		     size_t *upload_data_size,
		     void **con_cls)
{
    try {
	HTTPServer * server = static_cast<HTTPServer *>(cls);
	ConnectionInfo * conn_info;
	if (*con_cls == NULL) {
	    conn_info = new ConnectionInfo(connection, method, url, version);
	    *con_cls = conn_info;
	} else {
	    conn_info = static_cast<ConnectionInfo *>(*con_cls);
	    conn_info->first_call = false;
	}

	// FIXME - are any of these always the same each time the callback is
	// called, so can be initialised only once?
	conn_info->upload_data = upload_data;
	conn_info->upload_data_size = upload_data_size;

	server->answer(*conn_info);
	return MHD_YES;
    } catch(RestPose::Error & e) {
	// FIXME log
	fprintf(stderr, "RestPose::Error(): %s\n", e.what());
	return MHD_NO;
    } catch(bad_alloc) {
	// FIXME log
	fprintf(stderr, "bad_alloc\n");
	return MHD_NO;
    }
}

static void
request_completed_cb(void * /* cls */,
		     struct MHD_Connection * /* connection */,
		     void **con_cls,
		     enum MHD_RequestTerminationCode /* toe */)
{
    if (*con_cls != NULL) {
	ConnectionInfo * conn_info = static_cast<ConnectionInfo *>(*con_cls);
	delete conn_info;
	*con_cls = NULL;
    }
}

HTTPServer::HTTPServer(int port_,
		       bool pedantic_,
		       Router * router_)
	: port(port_),
	  pedantic(pedantic_),
	  router(router_),
	  daemon(NULL)
{
}

HTTPServer::~HTTPServer()
{
    stop();
    join();
}

void
HTTPServer::start()
{
    if (daemon) return;
    //printf("Starting HTTPServer\n");
    int flags = MHD_NO_FLAG;
    if (pedantic) {
	flags |= MHD_USE_PEDANTIC_CHECKS;
    }
    daemon = MHD_start_daemon(flags,
			      port,

			      /* Checks before accepting connection. */
			      NULL,
			      NULL,

			      /* Handle a connection. */
			      &answer_connection_cb,
			      this,

			      /* Cleanup after handling a connection. */
			      MHD_OPTION_NOTIFY_COMPLETED,
			      request_completed_cb,
			      NULL,

			      MHD_OPTION_END);

    if (!daemon) {
	throw RestPose::HTTPServerError("Unable to start HTTP daemon");
    }
}

void
HTTPServer::stop()
{
    if (!daemon) return;
    //printf("Stopping HTTPServer\n");
    MHD_stop_daemon(daemon);
    daemon = NULL;
}

void
HTTPServer::get_fdsets(fd_set * read_fd_set,
		       fd_set * write_fd_set,
		       fd_set * except_fd_set,
		       int * max_fd,
		       bool * have_timeout,
		       uint64_t * timeout)
{
    if (MHD_get_fdset(daemon, read_fd_set, write_fd_set, except_fd_set,
		      max_fd) != MHD_YES) {
	throw RestPose::HTTPServerError("Unable to get fdset");
    }
    unsigned MHD_LONG_LONG mhd_timeout;
    if (MHD_get_timeout(daemon, &mhd_timeout) == MHD_YES) {
	if (*have_timeout) {
	    if (*timeout < mhd_timeout)
		*timeout = mhd_timeout;
	} else {
	    *have_timeout = true;
	    *timeout = mhd_timeout;
	}
    }
}

void
HTTPServer::serve(fd_set *, fd_set *, fd_set *, bool)
{
    //printf("HTTPServer::serve()\n");
    if (MHD_run(daemon) != MHD_YES) {
	throw RestPose::HTTPServerError("Can't poll server (MHD_run failed)");
    }
}

void
HTTPServer::answer(ConnectionInfo & conn)
{
    try {
	if (conn.handler == NULL) {
	    conn.handler = router->route(conn);
	    if (conn.handler == NULL) {
		// This happens if the request can't be routed; a response will
		// already have been sent.
		return;
	    }
	}
	conn.handler->handle(conn);

    } catch(RestPose::Error & e) {
	fprintf(stderr, "RestPose::Error(): %s\n", e.what());
	Json::Value response(Json::objectValue);
	response["err"] = string("RestPose::Error(): ") + e.what();
	conn.respond(500, json_serialise(response), "application/json");
    } catch(Xapian::Error & e) {
	fprintf(stderr, "Xapian::Error(): %s\n", e.get_description().c_str());
	Json::Value response(Json::objectValue);
	response["err"] = string("Xapian::Error(): ") + e.get_description();
	conn.respond(500, json_serialise(response), "application/json");
    }
}
