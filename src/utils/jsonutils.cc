/** @file jsonutils.cc
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

#include <config.h>

#include "jsonutils.h"

#include "json/reader.h"
#include "json/value.h"
#include "json/writer.h"
#include <string>

#include "utils/rsperrors.h"

namespace RestPose {

void
json_check_object(const Json::Value & value, const char * description)
{
    if (!value.isObject()) {
	throw InvalidValueError(std::string("JSON value for ") +
				description + " was not an object");
    }
}

void
json_check_array(const Json::Value & value, const char * description)
{
    if (!value.isArray()) {
	throw InvalidValueError(std::string("JSON value for ") +
				description + " was not an array");
    }
}

void
json_check_string(const Json::Value & value, const char * description)
{
    if (!value.isString()) {
	throw InvalidValueError(std::string("JSON value for ") +
				description + " was not a string");
    }
}

void
json_check_bool(const Json::Value & value, const char * description)
{
    if (!value.isBool()) {
	throw InvalidValueError(std::string("JSON value for ") +
				description + " was not a boolean");
    }
}

uint64_t
json_get_uint64_member(const Json::Value & value, const char * key,
		       uint64_t max, uint64_t def)
{
    Json::Value member = value.get(key, Json::Value::null);
    if (member.isNull()) {
	return def;
    }
    if (!member.isIntegral()) {
	throw InvalidValueError(std::string("JSON value for ") + key +
				" was not an integer");
    }
    if (member < Json::Value::Int(0)) {
	throw InvalidValueError(std::string("JSON value for ") + key +
				" was negative");
    }
    if (member > Json::Value::UInt64(max)) {
	throw InvalidValueError(std::string("JSON value for ") + key +
				" was larger than maximum allowed (" +
				Json::valueToString(Json::Value::UInt64(max)) + ")");
    }
    return member.asUInt64();
}

uint64_t
json_get_uint64_member(const Json::Value & value, const char * key,
		       uint64_t max)
{
    Json::Value member = value.get(key, Json::Value::null);
    if (member.isNull()) {
	throw InvalidValueError(std::string("Member ") + key + " was missing");
    }
    if (!member.isIntegral()) {
	throw InvalidValueError(std::string("JSON value for ") + key +
				" was not an integer");
    }
    if (member < Json::Value::Int(0)) {
	throw InvalidValueError(std::string("JSON value for ") + key +
				" was negative");
    }
    if (member > Json::Value::UInt64(max)) {
	throw InvalidValueError(std::string("JSON value for ") + key +
				" was larger than maximum allowed (" +
				Json::valueToString(Json::Value::UInt64(max)) + ")");
    }
    return member.asUInt64();
}

uint64_t
json_get_uint64(const Json::Value & value)
{
    if (!value.isConvertibleTo(Json::uintValue)) {
	throw InvalidValueError("Value is not convertible to an integer");
    }
    if (value < Json::Value::Int(0)) {
	throw InvalidValueError("JSON value for was negative - wanted unsigned int");
    }
    if (value > Json::Value::maxUInt64) {
	throw InvalidValueError("JSON value " + value.toStyledString() +
				" was larger than maximum allowed (" +
				Json::valueToString(Json::Value::maxUInt64) +
				")");
    }
    return value.asUInt64();
}

std::string
json_get_string_member(const Json::Value & value, const char * key,
		       const std::string & def)
{
    Json::Value member = value.get(key, Json::Value::null);
    if (member.isNull()) {
	return def;
    }
    if (!member.isString()) {
	throw InvalidValueError(std::string("JSON value for ") + key +
				" was not a string");
    }
    return member.asString();
}

double
json_get_double_member(const Json::Value & value, const char * key,
		       double def)
{
    Json::Value member = value.get(key, Json::Value::null);
    if (member.isNull()) {
	return def;
    }
    if (!member.isConvertibleTo(Json::realValue)) {
	throw InvalidValueError(std::string("JSON value for ") + key +
				" was not convertible to a double");
    }
    return member.asDouble();
}

bool
json_get_bool(const Json::Value & value, const char * key, bool def)
{
    Json::Value member = value.get(key, Json::Value::null);
    if (member.isNull()) {
	return def;
    }
    if (!member.isConvertibleTo(Json::booleanValue)) {
	throw InvalidValueError(std::string("JSON value for ") + key +
				" was not convertible to a boolean");
    }
    return member.asBool();
}

std::string
json_get_idstyle_value(const Json::Value & value, std::string & errors)
{
    if (value.isString()) {
	return value.asString();
    }

    if (!value.isConvertibleTo(Json::uintValue)) {
	errors = "Expected value in field to be an integer or a string";
	return std::string();
    }
    if (value < Json::Value::Int64(0)) {
	errors = "JSON value for field was negative - wanted unsigned int";
	return std::string();
    }
    if (value > Json::Value::maxUInt64) {
	errors = "JSON value " + value.toStyledString() +
		" was larger than maximum allowed (" +
		Json::valueToString(Json::Value::maxUInt64) + ")";
	return std::string();
    }
    return Json::valueToString(value.asUInt64());
}

std::string
json_serialise(const Json::Value & value)
{
    Json::FastWriter writer;
    return writer.write(value);
}

Json::Value &
json_unserialise(const std::string & serialised, Json::Value & value)
{
    Json::Reader reader;
    bool ok = reader.parse(serialised, value, false);
    if (!ok) {
	throw InvalidValueError("Invalid JSON: " +
				reader.getFormatedErrorMessages());
    }
    return value;
}

};
