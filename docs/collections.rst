===========
Collections
===========

Everything in RestPose is stored in a *Collection*.  There may be many
collections in a RestPose instance, each with entirely distinct contents.

The main objects stored in a collection are *Documents*.  These each have a
*type*, which governs how they are indexed, stored and searched, and an *ID*,
which is used to reference a specific document for updates and retrieval.

Collections also contain:

A *Schema* for each type of document.
   This controls how each field in the document is handled when indexing and
   searching.  In addition, the schema contains a set of patterns used to
   determine how to handle new fields - whether to raise an error for them,
   or add an appropriate field configuration for them.

A set of *Categorisers*
   These can can be applied to parts of a document for tasks such as
   identifying the language of a piece of text.

A set of *Category Hierarchies*
   Fields in documents can be attached to categories.  Searches can then 
   efficiently match all items in a given category, or in the children of a
   given category.

A set of *Orderings*
   Documents in the collection, or a subset of documents in the collection,
   may be placed in a specific order.  Search results may then be sorted by
   this ordering, and the position of a particular document in the ordering
   may be determined efficiently.

A set of *Pipes*
   These allow custom manipulation of documents to be performed, with the
   output sent to a subsequent pipe, or to the index.  Note - this subsystem
   doesn't feel elegant and clean, and may therefore be entirely replaced at
   some point in the near future by an alternative approach, such as Lua
   scripting.

---------
Documents
---------

As far as RestPose is concerned, a document is a object consisting of an
unordered set of fieldnames, each of which has a value, or an ordered list of
values, associated with it.  In JSON representation, a document will often
look something like:

    {
      "id": "1",
      "type": "default",
      "text": ["Lorem ipsum", "Dolor sit"],
      "tag": ["Simple", "Boring tag"]
    }

Documents will usually have two fields which are used for special purposes by
RestPose: an ID field, and a Type field.  The ID field is used for finding
documents with a specific ID, and the Type field is used for finding documents
with a specific type.

IDs are specific to a particular type of document - ie, two documents may exist
in the collection with the same ID if they have differing types.  In other
words, to uniquely reference a document in a collection, you need to use a
combination of the type and the document ID.

-----------------
Types and Schemas
-----------------

Each collection can contain a variety of types of document.  The indexing and
search configuration for each type is independent (with the exception that the
same fields must be used for storing ids and type ids in every type), allowing
entirely distinct indexing strategies to be used for each type.

Each type is described by a schema, which consists of several properties and
governs the way in which search and indexing is performed:

*fields*
    A description of the configuration for each known `field type`_.
*patterns*
    A list of `patterns`_ to apply, in order, to unknown fields.

The collection configuration contains a section which defines which fields are
used to hold identifiers for document ids and document types.

.. _field type:

Field types
===========

The list of known fields is stored in the "`fields`" property of a schema.
This maps from a fieldname to a set of properties for that field.  The
configuration for all fields must have a "`type`" property, which defines what
kind of processing to do to the field.  All field types except the "Ignore"
type also accept a "`store_field`" parameter, which specifies the fieldname to
store values seen in that field in (for display with search results).  This
will normally be either empty if the values shouldn't be stored, or the
fieldname for the field.

The other properties which are valid vary according to the type.  There are
several different types:

Text fields
-----------

(`type` = `text`)

Text fields expect a UTF-8 encoded string as input.

They have a `prefix` parameter used to stinguish the terms generated by these
fields from other fields.  The `prefix` parameter may not be empty.

They also have a `processor` parameter used to control how terms are generated
from words.  Currently supported values for this parameter are:

 - (empty): Standard Xapian term processing, with no stemming.  Essentially,
   this means: lowercase, split on whitespace and punctuation.
 - "stem_*": Standard Xapian term processing, using the stemming algorithm
   matched by "*".  Eg, "stem_en" to use the English stemming algorithm.
 - "cjk": Processing for CJK text, splitting text into ngrams.  Non CJK
   characters are split on whitespace.

Exact fields
------------

(`type` = `exact`)

Exact fields expect a single byte string, or a single integer, as input, and
store a representation of the exact value supplied as input.

Exact fields have a `prefix` parameter which is used to distinguish the terms
generated by these fields from other fields.  The `prefix` parameter may not be
empty.

If an integer is supplied (either when indexing or searching), it is converted
to a decimal representation of the integer (as long as the integer is positive,
and requires no more than 64 bits to represent it in binary form).

Exact fields have a `max_length` parameter, which specifies the maximum length
for a field value to be stored.  This defaults to 64.  With current Xapian
backends there is a limit on term length - to avoid any possible problems, this
limit shouldn't be raised above 120 (though you can get away with larger values
in many cases).  It's probably unwise to have very long terms anyway.

Exact fields also have a `too_long_action` parameter, which specifies an action
to take if the field value exceeds the length specified by `max_length`.  This
can be one of:

 - `error`: Log an error if the field value exceeds the maximum length.  This
   is the default.

 - `hash`: Replace the end of the field value with a hash of the end of the
   field value, to bring the length into compliance with `max_length`.  Note
   that if `max_length` is very low (a few bytes), the hashed length might
   still exceed it.

 - `truncate`: Truncate the field value to the `max_length` value.

Date fields
-----------

(`type` = `date`)

Date fields expect an integer number of seconds since the Unix epoch (1970).

They have one additional parameter: the "slot" parameter, which is an unsigned
integer value.  Each distinct date that should be searchable should be given a
distinct value for the "slot" parameter.

Geo fields
----------

(`type` = `geo`)

.. todo: Design and implement support for geo fields.

ID fields
---------

(`type` = `id`)

ID fields expect a single byte string as input.

There should only be one ID field in a schema.  This field is used to generate
a unique ID for the documents; if a new document is added with the same ID, the
old document with that ID will be replaced.

ID fields are very similar to Exact fields - they accept all the same
parameters, with the exception of the `prefix` parameter.

Stored fields
-------------

(`type` = `date`)

Stored fields do nothing except store their input value for display.  They have
no additional parameters.

Ignore fields
-------------

(`type` = `ignore`)

Ignore fields are completely ignored.  They may be defined to prevent the
default action for unknown fields being performed on them.

.. todo: check that the behaviour for an ignore field which has a store_field parameter is sensible, and document it.


.. _patterns:

Patterns
========

The "`patterns`" property of a schema contains a list of patterns which are
used to define new field types automatically the first time a new field is
seen.

Each pattern is a list containing two items: the pattern to match, and the type
definition to use when the pattern matches.

When indexing, for any field which is not already in the list of known fields
in the schema the patterns are checked, in order, against the name of the new
field.  The first matching value is then used to create a new field type.

Because document processing happens in parallel, it is important that the order
of processing documents is not significant in controlling what the new field's
configuration should be.  Therefore, only the name of the new field is taken
into account; the contents of the field in the document being processed are not
significant.

Currently, the only syntax supported for patterns is a literal fieldname with
an optional leading "*".  If present, the "*" will match any number of
characters (including 0) at the start of the fieldname.

A "*" character may be used in the values of type definitions to use when a
pattern matches.  This will be replaced by whatever the "*" matched in the
pattern.

Special fields
==============

The "`special_fields`" property of a collection defines the field names which
are used for two special purposes.

*idfield*
     The field which is used for id lookups.  This should normally be a field
     of type id.  The terms generated from the id field will be used for
     replacing older versions of documents.
*typefield*
     The field which is used for type lookups.

------------
Categorisers
------------

.. todo: document categorisers

--------------------
Category Hierarchies
--------------------

.. todo: implement and document category hierarchies

---------
Orderings
---------

.. todo: implement and document orderings

-----
Pipes
-----

.. todo: document pipes, or replace them and document the replacement


========================
Collection Configuration
========================

An example dump of the configuration for a collection can be found in
examples/schema.json.

The schema is a JSON file (with the extension that C-style comments are
permitted in it).  The current schema format is stored in a `schema_format`
property: this is to allow upgrades to the schema format to be performed in
future.  This document describes schemas for which `schema_format` is 3.