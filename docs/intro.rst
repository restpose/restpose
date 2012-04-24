Overview
========

RestPose is a search engine.  By this, we mean that it is a system which is
designed to take a set of documents, and then when given a query to return
ranked lists of documents which are a good match for that query.  RestPose
manages a set of internal indexes, and provides an interface (over HTTP, in a
fairly RESTful style, using JSON as the main transfer format) which allows
documents to be submitted and removed from indexes, and which allows searches
to be performed.

RestPose includes support for processing text in a variety of languages (the
most spoken European languages, and CJK text, have explicit support, but other
text can often be handled to some level).  It also has some basic, but
reasonably effective, support for automatically guessing the language of a
piece of text.

RestPose is developed under Linux and MacOSX, but should also be possible to
compile for windows with a small amount of work.


What is the point of a search engine?
-------------------------------------

With most database systems (certainly with relational databases, and also with
many of the newer "NoSQL" databases), the focus of queries performed on a
database is to retrieve a set of records matching the query.  In contrast, the
main focus of queries performed in a search engine is to retrieve a ranked list
of records; and often, only to return the most highly ranked.  Whilst a good
ranking is important, it is hard to define exactly what a good ranking is, so
there are many ways to customise how the ranking is performed.  It is generally
a binary yes/no decision to tell if a database is returning the correct answer
to a query; with a search engine, it is much less clear cut what the correct
answer to a query is, so the appropriate topic to discuss is often how high
quality the results of a query are - ie, how well the ranking matches some
theoretical ideal.

Many databases provide some support for "full text search" these days.  This is
often sufficient to implement a very simple search engine, but doesn't allow
the flexibility to tune ranking in the ways that a search engine allows. 

Similarly, it is possible to use a search engine as your primary datastore, at
the cost of losing some control over aspects such as fully transactional
updates, ability to perform complex joins, etc.

The difference of focus between a database and a search engine often means that
a good approach is to use both a standard database for some types of queries, and
a search engine for others.


Development
-----------

RestPose is still in early development - it is likely that the internal storage
formats will change from time to time (though this will be noted clearly in
release notes), and the APIs are likely to be modified as well.

Development is performed using Git, using github to hold the master repository.
The Python client is actively maintained by the same author as the core engine,
since it is used to run parts of the main testsuite.

Issues can be reported in the git repository at
http://github.com/rboulton/restpose
