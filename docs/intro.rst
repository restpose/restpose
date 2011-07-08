Overview
========

Warning: this document is entirely out of date.

Compilation instructions on Fedora 8
------------------------------------

Install a recent Xapian first::

  sudo yum install -y gcc-c++ zlib-devel e2fsprogs-devel
  wget http://oligarchy.co.uk/xapian/1.2.3/xapian-core-1.2.3.tar.gz
  tar zxf xapian-core-1.2.3.tar.gz
  cd xapian-core-1.2.3
  ./configure
  make
  sudo make install
  sudo vi /etc/ld.so.conf
    - Add /usr/local/lib to end of file
  sudo ldconfig

Then, build libmicrohttpd::

  cd libs/libmicrohttpd
  ./configure
  make
  cd ../..

Then, build restpose by running "make".

This produces an executable "restpose", which is used for both indexing and
searching.

Indexing
--------

Before indexing, you need to design a schema.  This specifies what fields you
have in your data, what types they are, how to index them, and whether to store
them.  See `docs/field_config.rst` for details of the contents and format.

Once you have a schema, you can index some data using the "restpose" tool::

  ./restpose -s examples/schema -d /path/to/data /path/to/database

This creates a new database at "/path/to/database", and applies the schema at
"examples/schema" to it.  The schema is stored in the database.  Then, the "-d"
option tells jx to recurse through the directory hierarchy, starting at
"/path/to/data", indexing the contents of all the files found.  Status messages
will be displayed every few hundred documents to indicate progress.

On a machine with large memory, you should set the `XAPIAN_FLUSH_THRESHOLD`
environment variable to a high value (eg, 100000) to make indexing faster.  For example::

  XAPIAN_FLUSH_THRESHOLD=100000 ./jx -s examples/schema -d /path/to/data /path/to/database

Searching
---------

Searching is performed by supplying a query in JSON format to the "jx" tool.
This query can be read from a file, or read from stdin by using the special
filename "-".

The query format is a JSON object with a few possible properties::

  {
    "text": "A piece of text to search for",
    "filter": [], # A list of filters to apply
    "display": [], # A list of fieldnames 
    "offset": 0, # The offset of the first result to return
    "maxitems": 10, # The maximum number of items to return
  }

The "text" property simply contains a piece of text, which is treated as a
phrase and searched for in the text fields.  All documents containing the
phrase will be returned.

The "filter" property contains a list of filters to apply to the results.
Each filter is represented by a list containing 3 items: a fieldname, a filter
type, and a list of values for the filter to search for.  There are two types
of filter available:

 - date fields can be filtered using a "range" filter, to return only those
   dates in a given range.  Dates are specified as the number of seconds since
   1970.  For example, to search for a datetime in the range 1282031380 to
   1282031480, the filter would be::
   
     ["pub_date", "range", [1282031380, 1282031480]]

 - exact match fields (and ID fields) can be filtered using an "is" filter, to
   return only those documents which contain one of a list of values.  For
   example, to return documents with one of the ids "01", "02" or "03", the
   filter would be::

     ["id", "is", ["01", "02", "03"]]


At least one of the "text" and "filter" properties should be supplied (or no
results will be returned).  If no query is supplied, any document matching all
the filters will be returned.  If no filter is supplied, all documents matching
the query will be returned.

The "display" property contains a list of fields to return.  (This relies on
the schema being set such that field contents are stored for these fields.)

The "offset" and "maxitems" properties control the offset of the first result
to return (0 means the first result found), and the maximum number of results
to return.  Set maxitems to a large number to recieve all results.

Python
------

Searching can be performed by a simple python client, included in the python
directory (in python/jx.py).  This spawns a subprocess which runs "jx", but can
also be told to run that subprocess on a remote machine.  An example of using
this client is held in "python/jxtest.py".

To run a search on a remote machine, set up an ssh certificate to allow
passwordless login, pass the `ssh_host` parameter to jx.search(), and set the
`jx_path` parameter to be the full path to "jx" on the remote machine.
