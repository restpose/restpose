/** @file validation.h
 * @brief Validate names for various things
 */
/* Copyright 2011 Richard Boulton
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
#ifndef RESTPOSE_INCLUDED_VALIDATION_H
#define RESTPOSE_INCLUDED_VALIDATION_H

#include <string>

/** Check if a collection name is valid.
 *
 *  Returns an error message if the name is not valid.
 */
std::string validate_collname(const std::string & value);

/** Check if a document type is valid.
 *
 *  Returns an error message if the type is not valid.
 */
std::string validate_doc_type(const std::string & value);

/** Check if a document ID is valid.
 *
 *  Returns an error message if the ID is not valid.
 */
std::string validate_doc_id(const std::string & value);

#endif /* RESTPOSE_INCLUDED_VALIDATION_H */
