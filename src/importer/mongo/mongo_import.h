/** @file mongo_import.h
 * @brief Importer to import data from mongodb.
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
#ifndef RESTPOSE_INCLUDED_MONGO_IMPORT_H
#define RESTPOSE_INCLUDED_MONGO_IMPORT_H

#include <string>
#include "json/value.h"
#include "server/server.h"

class TaskManager;

class MongoImporter : public BackgroundTask {
  public:
    class Internal;
  private:
    class Internal * internal;

    MongoImporter(const MongoImporter &);
    void operator=(const MongoImporter &);
  public:
    MongoImporter(TaskManager * taskman);
    ~MongoImporter();

    /** Set the configuration for the importer.
     */
    void set_config(const Json::Value & config);

    /** Get the status of the importer.
     *
     *  Called from the main server thread, so must return quickly.
     */
    void get_status(Json::Value & result) const;

    /** Start the importer.
     *
     *  Called after creation of the importer, and after an initial call to
     *  set_config.
     */
    void start(Server * server);

    /** Stop this importer.
     */
    void stop();

    /** Join the importer thread.
     */
    void join();
};

#endif /* RESTPOSE_INCLUDED_MONGO_IMPORT_H */
