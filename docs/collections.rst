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

A set of *Taxonomies*

   Collections contain a named set of taxonomies, each of which contains a set
   of categories.  The schema can associate fields with taxonomies.  Searches
   can then efficiently match all items in a given category, or in the children
   of a given category.

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

Collections are identified by name, which is assumed to be a UTF-8 encoded
value when referred to in URIs, and is not allowed to contain the following
characters:

 * "Control" characters: ie, characters in the range 0 to 31.
 * ``:``, ``/``, ``\``, ``.``, ``,``, ``[``, ``]``, ``{``, ``}``

---------
Documents
---------

As far as RestPose is concerned, a document is a object consisting of an
unordered set of fieldnames, each of which has a value, or an ordered list of
values, associated with it.  In JSON representation, a document will often
look something like::

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

Document IDs and type names are assumed to be UTF-8 encoded values when
referred to in URIs, and are not allowed to contain the following characters:

 * "Control" characters: ie, characters in the range 0 to 31.
 * ``:``, ``/``, ``\``, ``.``, ``,``, ``[``, ``]``, ``{``, ``}``

.. _types_and_schemas:

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
    A description of the configuration for each known `field_type`_.
*patterns*
    A list of `patterns`_ to apply, in order, to unknown fields.

The collection configuration contains a section which defines which fields are
used to hold identifiers for document ids and document types.

.. _field_type:

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

They have a `group` parameter used to distinguish the terms generated by these
fields from other fields.  The `group` parameter may not be empty.

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

Exact fields have a `group` parameter which is used to distinguish the terms
generated by these fields from other fields.  The `group` parameter may not be
empty.

If an integer is supplied (either when indexing or searching), it is converted
to a decimal representation of the integer (as long as the integer is positive,
and requires no more than 64 bits to represent it in binary form).

Exact fields have a `wdfinc` parameter, which specifies the frequency to store
for each occurrence of a term.  This must be an integer, and defaults to 0,
which means that terms will be stored with a frequency in documents of 0, no
matter how often they occur in a document.  This is an appropriate value for
fields which are only ever used for filtering, but if you wish to get a higher
weight for searches which contain multiple matching terms in them, you should
set it to at least 1.  Higher values will cause documents matching terms in
this field to get a higher weight.

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

Numeric (double) fields
-----------------------

(`type` = `double`)

Numeric fields expect a numeric value, which will be stored as a double
precision floating point value.  Precision loss may occur if the numeric values
supplied cannot be represented as a double precision floating point value (but
note that, for example, all 32 bit integer values can be accurately represented
as doubles).

They have one additional parameter: the "slot" parameter, which is the number
or name of the slot that the values will be stored in.  Each numeric field that
should be searchable should be given a distinct value for the "slot" parameter.
See the `slot_numbers`_ section for more details about slot numbers.

Category fields
---------------

(`type` = `cat`)

Category fields are somewhat similar to exact fields, but are attached to a
taxonomy (essentially, a hierarchy of field values).  Searches can then be used
to find all documents in which a value in a document is a descendant of the
search value.

They have one additional parameter: the "taxonomy" parameter, which is the name
of the taxonomy used by the field.  Multiple independent fields may make use of
the same taxononmy.

In order to work correctly, it is advisable to ensure that the term `group`
used for a category field is not shared with any other fields which use a
different taxonomy, or which are not category fields.

Each field value may be given one or more parents.  It is also possible for a
parent to have multiple child values.  It is an error to attempt to set up
loops in the inheritance graph, however.

See the `Taxonomies`_ section for more details.

Timestamp fields
----------------

(`type` = `timestamp`)

Timestamp fields expect an integer number of seconds since the Unix epoch
(1970).  They can only handle positive values.

They have one additional parameter: the "slot" parameter, which is the number
or name of the slot that the timestamps will be stored in.  Each timestamp that
should be searchable should be given a distinct value for the "slot" parameter.
See the `slot_numbers`_ section for more details about slot numbers.

Date fields
-----------

(`type` = `date`)

Date fields expect a date in the form "year-month-day", in which year, month
and day are integer values.  Negative years are allowed.

They have one additional parameter: the "slot" parameter, which is the number
or name of the slot that dates will be stored in.  Each date that should be
searchable should be given a distinct value for the "slot" parameter.  See the
`slot_numbers`_ section for more details about slot numbers.

LonLat fields (geospatial)
--------------------------

(`type` = `lonlat`)

LonLat fields expect to be given geographic coordinates (as longitude, latitude
pairs, expressed in degrees).  Multiple coordinates may be given in a field;
each coordinate may be expressed either as an array of numbers of length 2 (in
which case, the longitude must be given first), or as an object with "lon" and
"lat" properties, holding the longitude and latitude respectively, as numbers.

They have one additional parameter: the "slot" parameter, which is the number
or name of the slot that the coordinates will be stored in.  Each field
containing coordinates that should be separately searchable should be given a
distinct value for the "slot" parameter.  See the `slot_numbers`_ section for
more details about slot numbers.

LonLat fields allow searches to be performed which:

 - return only documents within a given range.
 - return results in order of closest distance from a set of points.
 - return results in a combined order of distance and other scores from the query.

Stored fields
-------------

(`type` = `stored`)

Stored fields do nothing except store their input value for display.  They have
no additional parameters.

Ignore fields
-------------

(`type` = `ignore`)

Ignore fields are completely ignored.  They may be defined to prevent the
default action for unknown fields being performed on them.  Note that this
field type does not support the `store_field` parameter; the contents of an
ignored field will never be stored.

ID fields
---------

(`type` = `id`)

ID fields expect a single byte string as input.

There should only be one ID field in a schema.  This field is used to generate
a unique ID for the documents; if a new document is added with the same ID, the
old document with that ID will be replaced.

ID fields are very similar to Exact fields - they accept all the same
parameters, with the exception of the `group` parameter.

Meta fields
-----------

(`type` = `meta`)

Meta fields are a special field type.  A normal field shouldn't be assigned a
meta field type.  The meta field is used to store information about which
fields were present in a document, and which fields produced errors when
processing.  It can then be used to search for these values.


.. _slot_numbers:

Slot numbers
------------

Various fields (eg, timestamp and date fields) have a "slot" parameter in their
configuration.  This is related to a concept in Xapian called "value slots" -
each document can have values associated with it, to be used at search time for
filtering, sorting, etc.

Xapian has a limitation that the slots are addressed only by numbers rather
than by strings.  For convenience, restpose allows slots to be addressed by
strings, and hashes the strings to produce a number.  There is a small chance
of hash collision, but this is unlikely to be a problem unless you are using

It is still possible to use a number to reference a slot.  If you use a number
directly, it is advisable to use a number less than 0x0fffffff, since the
results of the hashing algorithm will always be in the range 0x10000000 to
0xffffffff.

Note that the string `"1"` is not the same as the number 1.  The string will be
hashed to produce a slot number, whereas the number will be used directly as
the slot number.

Groups
------

Many field types (eg, text and exact fields) have a "group" parameter in their
configuration.  These parameters hold a single string, which is the name of the
group to generate terms in for that field.

When indexing, these fields generate terms representing the content of the
fields.  In order that distinct fields do not generate the same terms, the
terms can be placed into separate groups.  This allows searches to specify
which field a piece of content must be found in.  However, sometimes you want
distinct fields to be merged when performing searches; in this case, you can
specify the same group for several fields.

In most cases, you should take care that all fields which share a group use a
compatible indexing strategy; ie, they should have the same field type, and
have the same values for configuration parameters other than those controlling
the frequencies stored (eg, the wdfinc).  If you really know what you are
doing, though, it is valid for entirely different configurations to share a
group - you just might not get the results you expected.

.. note:: With the current search engine backend, short values should be used
	  for the group parameters, since longer values will use quite a bit
	  more space.  A future backend database format is expected to make
	  this difference minimal, but for now, simply use short names if
	  you're concerned about disk usage (a few characters should be fine).

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
are used for special purposes.

*id_field*
     The field which is used for id lookups.  This should normally be a field
     of type id.  The terms generated from the id field will be used for
     replacing older versions of documents.
*type_field*
     The field which is used for type lookups.
*meta_field*
     The field which is used for storing meta information about which fields
     are present in documents, and which fields have errors.  This should be a
     field of type meta.  Incoming documents should not contain entries in the
     meta field - the entries will be automatically generated based on the
     result of processing the documents.

----------
Taxonomies
----------

A collection may contain a set of Taxonomies, each identified by a name.  These
taxonomies consist of a hierarchy of categories, and can be associated with
fields.  When associated with a category field, it becomes possible to search
efficiently for all documents in which a field value is a descendant of a given
category; normally, it would only be possible to do this by constructing a huge
query consisting of all the categories which are such descendants.

Taxonomies may be modifed at any time by adding or removing category-parent
relationships, or even hold categories.  The necessary index updates will be
performed automatically.

.. note:: If possible, it is better to put the hierarchy in place before adding
          documents, since this will require less work in total.

.. note:: Currently, the taxonomy feature is not designed to perform well with
	  large numbers of entries in the category hierarchy (ie, more than a
	  few hundred entries).  Performance improvements are planned, but if
	  you need to use the feature with deep hierarchies, contact the author
	  on IRC or email.

------------
Categorisers
------------

.. todo: document categorisers

---------
Orderings
---------

.. todo: implement and document orderings

-----
Pipes
-----

.. todo: document pipes, or replace them and document the replacement


.. _coll_config:

========================
Collection Configuration
========================

An example dump of the configuration for a collection can be found in
examples/schema.json.

The schema is a JSON file (with the extension that C-style comments are
permitted in it).  The current schema format is stored in a `schema_format`
property: this is to allow upgrades to the schema format to be performed in
future.  This document describes schemas for which `schema_format` is 3.

.. todo:: Describe the representation of the collection configuration fully.
