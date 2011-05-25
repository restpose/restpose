/** @file jsonutils.h
 * @brief Utilities for handling JSON values.
 */
/* Copyright (c) 2010 Richard Boulton
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

#ifndef RESTPOSE_INCLUDED_JSONUTILS_H
#define RESTPOSE_INCLUDED_JSONUTILS_H

#include "json/value.h"
#include <string>

namespace RestPose {
    /** Check that a JSON value is an object, raising an exception if not.
     */
    void json_check_object(const Json::Value & value,
			   const char * description);

    /** Check that a JSON value is an array, raising an exception if not.
     */
    void json_check_array(const Json::Value & value,
			  const char * description);

    /** Check that a JSON value is a string, raising an exception if not.
     */
    void json_check_string(const Json::Value & value,
			   const char * description);

    /** Check that a JSON value is a boolean, raising an exception if not.
     */
    void json_check_bool(const Json::Value & value,
			 const char * description);

    /** Get an unsigned 64 bit integer value from a JSON object, raising an
     *  exception if the member with the given key is not an integer, or is out
     *  of range.
     *
     *  @param value The value to look for the member in.
     *  @param key The key to look for the member for.
     *  @param max The maximum permissible value.
     *  @param def The default value, if the member is missing or null.
     */
    uint64_t json_get_uint64_member(const Json::Value & value,
				    const char * key,
				    uint64_t max,
				    uint64_t def);

    /** Get an unsigned 64 bit integer value from a JSON object, raising an
     *  exception if the member with the given key is not an integer, or is out
     *  of range, or is missing.
     *
     *  @param value The value to look for the member in.
     *  @param key The key to look for the member for.
     *  @param max The maximum permissible value.
     */
    uint64_t json_get_uint64_member(const Json::Value & value,
				    const char * key,
				    uint64_t max);

    /** Get a JSON value as a uint, raising appropriate exceptions if
     *  the value is invalid.
     */
    uint64_t json_get_uint64(const Json::Value & value);

    /** Get an string value from a JSON object, raising an exception if the
     *  member with the given key is not a string.
     *
     *  @param value The value to look for the member in.
     *  @param key The key to look for the member for.
     *  @param def The default value, if the member is missing or null.
     */
    std::string json_get_string_member(const Json::Value & value,
				       const char * key,
				       const std::string & def);

    /** Get a double value from a JSON object, raising an exception if the
     *  member with the given key is not numeric.
     *
     *  @param value The value to look for the member in.
     *  @param key The key to look for the member for.
     *  @param def The default value, if the member is missing or null.
     */
    double json_get_double_member(const Json::Value & value,
				  const char * key,
				  double def);

    /** Get a boolean value from a JSON object, raising an exception if the
     *  member with the given key is not boolean.
     *
     *  @param value The value to look for the member in.
     *  @param key The key to look for the member for.
     *  @param def The default value, if the member is missing or null.
     */
    bool json_get_bool(const Json::Value & value,
		       const char * key,
		       bool def);

    /** Serialise a JSON value as a string, in standard JSON format.
     */
    std::string json_serialise(const Json::Value & value);

    /** Parse a JSON value from a string.
     *
     *  Returns a reference to the value supplied, to allow easier use inline.
     */
    Json::Value & json_unserialise(const std::string & serialised, Json::Value & value);
};

#endif /* RESTPOSE_INCLUDED_JSONUTILS_H */
