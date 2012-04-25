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

    /** Parse the options.
     *
     *  Returns 0 if parsed correctly.
     *  Returns -1 if parsed correctly, but execution should now terminate (eg,
     *  -h or -v was passed).
     *  Returns >0 if parse error (an appropriate message will have been
     *  displayed).
     */
    int parse(const char * progname, int argc, char * const* argv);

#ifdef __WIN32__
    /** Get the command to use when running as a service.
     */
    std::string service_command() const;

    enum service_action_type {
	SRVACT_NONE,
	SRVACT_INSTALL,
	SRVACT_REMOVE,
	SRVACT_REINSTALL,
	SRVACT_RUN_SERVICE
    };
    service_action_type service_action;
    std::string service_name;
    std::string service_user;
    std::string service_password;
#endif

    enum action_type {
	ACT_DEFAULT,
	ACT_SERVE,
	ACT_SEARCH,
	ACT_TRAIN
    };

    std::string datadir;
    action_type action;
    int port;
    bool pedantic;
    std::string dbname;
    std::vector<std::string> searchfiles;
    std::vector<std::string> languages;
    std::string mongo_import;
};

}

#endif /* RESTPOSE_INCLUDED_CLI_H */
