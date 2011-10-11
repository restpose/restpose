noinst_LIBRARIES += libjsonxapian.a

noinst_HEADERS += \
 src/jsonxapian/taxonomy.h \
 src/jsonxapian/collconfig.h \
 src/jsonxapian/collconfigs.h \
 src/jsonxapian/collection_pool.h \
 src/jsonxapian/collection.h \
 src/jsonxapian/docdata.h \
 src/jsonxapian/docvalues.h \
 src/jsonxapian/doctojson.h \
 src/jsonxapian/facetinfohandler.h \
 src/jsonxapian/indexing.h \
 src/jsonxapian/infohandlers.h \
 src/jsonxapian/occurinfohandler.h \
 src/jsonxapian/pipe.h \
 src/jsonxapian/query_builder.h \
 src/jsonxapian/schema.h \
 src/jsonxapian/slotname.h

libjsonxapian_a_SOURCES = \
 src/jsonxapian/taxonomy.cc \
 src/jsonxapian/collconfig.cc \
 src/jsonxapian/collconfigs.cc \
 src/jsonxapian/collection_pool.cc \
 src/jsonxapian/collection.cc \
 src/jsonxapian/docdata.cc \
 src/jsonxapian/docvalues.cc \
 src/jsonxapian/doctojson.cc \
 src/jsonxapian/facetinfohandler.cc \
 src/jsonxapian/indexing.cc \
 src/jsonxapian/infohandlers.cc \
 src/jsonxapian/occurinfohandler.cc \
 src/jsonxapian/pipe.cc \
 src/jsonxapian/query_builder.cc \
 src/jsonxapian/schema.cc \
 src/jsonxapian/slotname.cc
