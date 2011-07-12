/** @file httpserver.h
 * @brief HTTP server for RestPose
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
#ifndef RESTPOSE_INCLUDED_HTTPSERVER_H
#define RESTPOSE_INCLUDED_HTTPSERVER_H

#include <string>
#include "server/result_handle.h"
#include "server/server.h"
#include <vector>

/* Forward declarations */
struct MHD_Daemon;
class Handler;
class Router;
class Response;

enum HTTPMethod {
    HTTP_UNKNOWN,
    HTTP_HEAD = 1,
    HTTP_GET = 2,
    HTTP_GETHEAD = 3, // Convenience; can be used to check for GET or HEAD.
    HTTP_POST = 4,
    HTTP_PUT = 8,
    HTTP_DELETE = 16,
    HTTP_METHODMASK_MAX = 31 // Set this to the bitwise or of all HTTPMethod values.
};

/** Per connection state.
 *
 *  FIXME - this could probably usefully be split into a Request object holding
 *  the details of the request, and a ConnectionInstance object, pointing to
 *  the Request object and the ResultHandle.
 */
class ConnectionInfo {
    /// The handle holding the response.
    RestPose::ResultHandle resulthandle;

    /** Respond to a request.
     *
     *  The resulthandle should have the response set in it, and be marked as
     *  ready, before this is called.
     */
    void respond();

  public:
    struct MHD_Connection * connection;
    HTTPMethod method;
    std::string host; // value from Host header
    const char * url; // url path supplied in request (doesn't include method or host)
    const char * version;
    const char * upload_data;
    size_t * upload_data_size;

    /// True the first time accept is called.
    bool first_call;

    /// True if a response has been sent.
    bool responded;

    /// The components of the url path, separated on /.
    std::vector<std::string> components;

    /// Decoded URI arguments.
    std::map<std::string, std::vector<std::string> > uri_args;

    /// The handler assigned to deal with this request.
    Handler * handler;

    ConnectionInfo(struct MHD_Connection * connection_,
		   const char * method_,
		   const char * url_,
		   const char * version_);

    ~ConnectionInfo();

    /** Get a vector of values of a URI argument.
     *
     *  Returns NULL if the argument doesn't exist.
     */
    const std::vector<std::string> *
	    get_uri_arg_vals(const std::string & key) const;

    /** Get a value stored as a URI argument.
     *
     *  If there are multiple values stored, this will return one of them.
     *
     *  Returns NULL if the argument doesn't exist.
     */
    const std::string * get_uri_arg_val(const std::string & key) const;

    /** Get a value stored as a URI argument, as a bool.
     *
     *  This gets the argument value, as for get_uri_arg_val(), and attempts
     *  to convert it to a bool, as follows:
     *
     *  1, true, yes, on map to true.
     *  0, false, no, off map to false.
     *
     *  If no argument is stored, or the value is none of the above, returns
     *  defval.
     */
    bool get_uri_arg_bool(const std::string & key,
			  bool defval) const;

    /** Parse the url components (separated by / ) into the components member.
     */
    void parse_url_components();

    /// Respond to a request using the response in a handle.
    void respond(const RestPose::ResultHandle & resulthandle_);

    /** Respond to a request.
     *
     *  The status_code, output buffer and content_type will be set by this
     *  call.  Any previous state (including extra headers) in the response
     *  object will be ignored.
     */
    void respond(int status_code,
		 const std::string & outbuf,
		 const std::string & content_type);

    /** Require the HTTP method used to be one of the allowed methods.
     *
     *  @param allowed_methods A bitmap of HTTP methods which are allowed.
     */
    bool require_method(int allowed_methods);

    /** Get a string name of the HTTP method used.
     */
    const char * method_str() const;
};

/** The HTTP server.
 *
 *  This class wraps an HTTP server using libmicrohttpd.  It uses the
 *  external-select mode of libmicrohttpd, so the caller must take care of
 *  calling it from a select loop.
 */
class HTTPServer : public SubServer {
    	int port;
	bool pedantic;
	Router * router;
	struct MHD_Daemon * daemon;

    public:
	/** Create a new HTTP server.
	 *
	 *  @param port_ The port to listen on.
	 *
	 *  @param pedantic_ Whether to be pedantic about the content of
	 *  incoming connections (use for testing clients).
	 *
	 *  @param router_ The router to send requests to.
	 */
	HTTPServer(int port_, bool pedantic_, Router * router_);

	/** Destroy the server.
	 *
	 *  Stops the server if it's still running.
	 */
	~HTTPServer();

	/** Start the server.
	 *
	 *  This should be called once, and will block until the server is
	 *  ready to accept connections.
	 */
	void start();

	/** Stop the server.
	 */
	void stop();

	/** Join the server.
	 */
	void join() { /* Nothing to join for the current async HTTP server */ }

	/** Get the active fdsets.
	 */
	void get_fdsets(fd_set * read_fd_set,
			fd_set * write_fd_set,
			fd_set * except_fd_set,
			int * max_fd,
			bool * have_timeout,
			uint64_t * timeout);

	/** Perform pening server requests.
	 *
	 *  Will be called from the mainloop each time select returns.
	 */
	void serve(fd_set * read_fd_set,
		   fd_set * write_fd_set,
		   fd_set * except_fd_set,
		   bool timed_out);

	/** Answer a connection.
	 *
	 *  Will be called multiple times for handling posts.
	 */
	void answer(ConnectionInfo & conn_info);
};

#endif /* RESTPOSE_INCLUDED_HTTPSERVER_H */
