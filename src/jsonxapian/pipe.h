/** @file pipe.h
 * @brief Input pipelines
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

#ifndef RESTPOSE_INCLUDED_PIPE_H
#define RESTPOSE_INCLUDED_PIPE_H

#include <string>
#include "jsonmanip/mapping.h"

namespace RestPose {

/** An input pipeline.
 *
 *  Consists of a list of Mappings to apply, an optional target pipeline and
 *  some options.
 */
struct Pipe {
    /// The mappings in the pipe.
    std::vector<Mapping> mappings;

    /// Whether to apply all the mappings, or just the first that matches.
    bool apply_all;

    /// The target; an empty string for the target to be the collection.
    std::string target;


    Pipe() : mappings(), apply_all(false), target() {}

    /// Convert the pipe to a JSON object.
    Json::Value & to_json(Json::Value & value) const;

    /// Initialise the pipe from a JSON object.
    void from_json(const Json::Value & value);
};

}

#endif /* RESTPOSE_INCLUDED_PIPE_H */
