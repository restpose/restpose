noinst_LIBRARIES += libjsonxapian.a

noinst_HEADERS += \
 src/jsonxapian/category_hierarchy.h \
 src/jsonxapian/collconfig.h \
 src/jsonxapian/collconfigs.h \
 src/jsonxapian/collection_pool.h \
 src/jsonxapian/collection.h \
 src/jsonxapian/docdata.h \
 src/jsonxapian/doctojson.h \
 src/jsonxapian/indexing.h \
 src/jsonxapian/infohandlers.h \
 src/jsonxapian/occurinfohandler.h \
 src/jsonxapian/pipe.h \
 src/jsonxapian/schema.h

libjsonxapian_a_SOURCES = \
 src/jsonxapian/category_hierarchy.cc \
 src/jsonxapian/collconfigs.cc \
 src/jsonxapian/collection_pool.cc \
 src/jsonxapian/collection.cc \
 src/jsonxapian/docdata.cc \
 src/jsonxapian/doctojson.cc \
 src/jsonxapian/indexing.cc \
 src/jsonxapian/infohandlers.cc \
 src/jsonxapian/occurinfohandler.cc \
 src/jsonxapian/pipe.cc \
 src/jsonxapian/schema.cc
