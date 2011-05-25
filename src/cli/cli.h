/** @file cli.h
 * @brief Command line interface.
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

#ifndef RESTPOSE_INCLUDED_CLI_H
#define RESTPOSE_INCLUDED_CLI_H

#include <string>
#include <vector>

namespace RestPose {

struct CliOptions {
    CliOptions();
    int parse(const char * progname, int argc, char * const* argv);

    enum action_type {
	ACT_DEFAULT,
	ACT_SERVE,
	ACT_CMD,
	ACT_SEARCH,
	ACT_TRAIN
    };

    std::string datadir;
    action_type action;
    int port;
    bool pedantic;
    std::string dbname;
    std::string schema_path;
    std::vector<std::string> indexdirs;
    std::vector<std::string> searchfiles;
    std::vector<std::string> languages;
    bool show_info;
    std::string mongo_import;
};

}

#endif /* RESTPOSE_INCLUDED_CLI_H */
