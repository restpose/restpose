/** @file response.h
 * @brief Response to an HTTP request.
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
#ifndef RESTPOSE_INCLUDED_RESPONSE_H
#define RESTPOSE_INCLUDED_RESPONSE_H

#include <string>
#include "json/value.h"
#include <vector>

/* Forward declarations */
struct MHD_Response;

class Response {
    struct MHD_Response * response;
    int status_code;

    std::string outbuf;

    Response(const Response &);
    void operator=(const Response &);
  public:
    Response();
    ~Response();

    /// Set the status code for the response.
    void set_status(int status_code_);

    /** Set the response body from a string.
     *
     *  This clears any headers which have been set already.
     */
    void set_data(const std::string & outbuf_);

    /** Set the content type for the response.
     *
     *  This is just a shortcut for calling add_header to set the content type.
     */
    void set_content_type(std::string content_type);

    /** Add a header to the response.
     *
     *  The response body must be set (for example, using set_data()) before
     *  this is called.
     */
    void add_header(std::string header, std::string value);

    /** Set a JSON response.
     *
     *  This sets the response body to be the serialised JSON value, sets the
     *  content type to application/json, and sets the status code to the value
     *  supplied.
     */
    void set(const Json::Value & body, int status_code_ = 200);

    struct MHD_Response * get_response();
    int get_status_code() const;
};

#endif /* RESTPOSE_INCLUDED_RESPONSE_H */
