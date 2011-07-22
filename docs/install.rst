============
Installation
============

Please report any problems in this document with particular architectures.  So
far, RestPose has been built on Linux (Ubuntu 11.04 and Fedora 8) and Mac OSX
("Snow Leopard") without problems.

Binary packages
===============

Sorry, there are no binary packages available yet.


Compilation from source
=======================

The following are instructions for building from a source distribution tarball.

Installing prerequisites on Fedora 8
------------------------------------

Build and install a recent Xapian first::

  sudo yum install -y gcc-c++ zlib-devel e2fsprogs-devel
  wget http://oligarchy.co.uk/xapian/1.2.5/xapian-core-1.2.5.tar.gz
  tar zxf xapian-core-1.2.5.tar.gz
  cd xapian-core-1.2.5
  ./configure
  make
  sudo make install
  sudo vi /etc/ld.so.conf # Add /usr/local/lib to end of file
  sudo ldconfig

Installing prerequisites on Ubuntu
----------------------------------

Install the Xapian library and development headers, a compiler, and some
necessary dependency libraries, using::

  sudo apt-get install build-essential libxapian-dev python zlib1g-dev uuid-dev

Compiling RestPose
------------------

Then, build RestPose, simply by downloading and unpacking a distribution
tarball, and running::

  ./configure
  make

This produces an executable "restpose", which is used for both indexing and
searching.

Note that, if instead of a distribution tarball, you are working from a
checkout from a version control system, you will need to have GNU autotools
installed, and run::

  ./bootstrap

to build the build system and configure script.


Building documentation
----------------------

In order to build the documentation, you will need the Python "Sphinx" package
installed.  On Ubuntu, for example, the package can be installed using::

   sudo apt-get install python-sphinx

However, you often won't need to build your own copies of the documentation;
distribution tarballs contain a copy, and copies of the documentation are
available in various places; in particular, a copy of the latest revision is
available at http://readthedocs.org/docs/restpose/en/latest/

Running tests
-------------

Once you have built RestPose, you can build and run the testsuite simply by
running::

  make check

(In the source directory.)  Note that this will also build and run the
testsuite for libmicrohttpd, which may fail on some machines due to ports used
by the testsuite being in use, or due to firewalling rules.  Instead, you might
like to run::

  make check-am

which will only run the tests for RestPose.
