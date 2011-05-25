noinst_LIBRARIES += libmongoimporter.a

noinst_HEADERS += \
 src/importer/mongo/mongo_import.h

libmongoimporter_a_SOURCES = \
 src/importer/mongo/mongo_import.cc
