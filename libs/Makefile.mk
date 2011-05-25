noinst_LIBRARIES += libjsoncpp.a
libjsoncpp_a_CXXFLAGS = $(AM_CXXFLAGS) -Wno-error
INCLUDES += \
 -I$(top_srcdir)/libs/jsoncpp/src/lib_json \
 -I$(top_srcdir)/libs/jsoncpp/include
noinst_HEADERS += \
 libs/jsoncpp/include/json/autolink.h \
 libs/jsoncpp/include/json/config.h \
 libs/jsoncpp/include/json/features.h \
 libs/jsoncpp/include/json/forwards.h \
 libs/jsoncpp/include/json/json.h \
 libs/jsoncpp/include/json/reader.h \
 libs/jsoncpp/include/json/value.h \
 libs/jsoncpp/include/json/writer.h \
 libs/jsoncpp/src/lib_json/json_batchallocator.h \
 libs/jsoncpp/src/lib_json/json_tool.h \
 libs/jsoncpp/src/lib_json/json_valueiterator.inl \
 libs/jsoncpp/src/test_lib_json/jsontest.h
libjsoncpp_a_SOURCES = \
 libs/jsoncpp/src/lib_json/json_reader.cpp \
 libs/jsoncpp/src/lib_json/json_value.cpp \
 libs/jsoncpp/src/lib_json/json_writer.cpp

# CJK tokenizer
noinst_LIBRARIES += libcjktokenizer.a
INCLUDES += \
 -I$(top_srcdir)/libs/cjk-tokenizer/cjk-tokenizer
noinst_HEADERS += \
 libs/cjk-tokenizer/cjk-tokenizer/cjk-tokenizer.h \
 libs/cjk-tokenizer/cjk-tokenizer/cjk-hanconvert.h
libcjktokenizer_a_SOURCES = \
 libs/cjk-tokenizer/cjk-tokenizer/cjk-tokenizer.cc \
 libs/cjk-tokenizer/cjk-tokenizer/cjk-hanconvert.cc

# Files from xapian-core/common.
noinst_LIBRARIES += libxapiancommon.a
INCLUDES += \
 -I$(top_srcdir)/libs/xapiancommon
noinst_HEADERS += \
 libs/xapiancommon/config.h \
 libs/xapiancommon/diritor.h \
 libs/xapiancommon/hashterm.h \
 libs/xapiancommon/loadfile.h \
 libs/xapiancommon/noreturn.h \
 libs/xapiancommon/omassert.h \
 libs/xapiancommon/realtime.h \
 libs/xapiancommon/safedirent.h \
 libs/xapiancommon/safeerrno.h \
 libs/xapiancommon/safefcntl.h \
 libs/xapiancommon/safesysstat.h \
 libs/xapiancommon/safeunistd.h \
 libs/xapiancommon/serialise.h \
 libs/xapiancommon/str.h \
 libs/xapiancommon/utils.h
libxapiancommon_a_SOURCES = \
 libs/xapiancommon/diritor.cc \
 libs/xapiancommon/hashterm.cc \
 libs/xapiancommon/loadfile.cc \
 libs/xapiancommon/serialise.cc \
 libs/xapiancommon/utils.cc

# UnitTest++
noinst_LIBRARIES += libunittestpp.a
INCLUDES += \
 -I$(top_srcdir)/libs/unittest-cpp/UnitTest++/src
noinst_HEADERS += \
 libs/unittest-cpp/UnitTest++/src/AssertException.h \
 libs/unittest-cpp/UnitTest++/src/CheckMacros.h \
 libs/unittest-cpp/UnitTest++/src/Checks.h \
 libs/unittest-cpp/UnitTest++/src/Config.h \
 libs/unittest-cpp/UnitTest++/src/CurrentTest.h \
 libs/unittest-cpp/UnitTest++/src/DeferredTestReporter.h \
 libs/unittest-cpp/UnitTest++/src/DeferredTestResult.h \
 libs/unittest-cpp/UnitTest++/src/ExecuteTest.h \
 libs/unittest-cpp/UnitTest++/src/MemoryOutStream.h \
 libs/unittest-cpp/UnitTest++/src/ReportAssert.h \
 libs/unittest-cpp/UnitTest++/src/TestDetails.h \
 libs/unittest-cpp/UnitTest++/src/Test.h \
 libs/unittest-cpp/UnitTest++/src/TestList.h \
 libs/unittest-cpp/UnitTest++/src/TestMacros.h \
 libs/unittest-cpp/UnitTest++/src/TestReporter.h \
 libs/unittest-cpp/UnitTest++/src/TestReporterStdout.h \
 libs/unittest-cpp/UnitTest++/src/TestResults.h \
 libs/unittest-cpp/UnitTest++/src/TestRunner.h \
 libs/unittest-cpp/UnitTest++/src/TestSuite.h \
 libs/unittest-cpp/UnitTest++/src/TimeConstraint.h \
 libs/unittest-cpp/UnitTest++/src/TimeHelpers.h \
 libs/unittest-cpp/UnitTest++/src/UnitTest++.h \
 libs/unittest-cpp/UnitTest++/src/XmlTestReporter.h \
 libs/unittest-cpp/UnitTest++/src/Posix/SignalTranslator.h \
 libs/unittest-cpp/UnitTest++/src/Posix/TimeHelpers.h
libunittestpp_a_SOURCES = \
 libs/unittest-cpp/UnitTest++/src/AssertException.cpp \
 libs/unittest-cpp/UnitTest++/src/Test.cpp \
 libs/unittest-cpp/UnitTest++/src/Checks.cpp \
 libs/unittest-cpp/UnitTest++/src/TestRunner.cpp \
 libs/unittest-cpp/UnitTest++/src/TestResults.cpp \
 libs/unittest-cpp/UnitTest++/src/TestReporter.cpp \
 libs/unittest-cpp/UnitTest++/src/TestReporterStdout.cpp \
 libs/unittest-cpp/UnitTest++/src/ReportAssert.cpp \
 libs/unittest-cpp/UnitTest++/src/TestList.cpp \
 libs/unittest-cpp/UnitTest++/src/TimeConstraint.cpp \
 libs/unittest-cpp/UnitTest++/src/TestDetails.cpp \
 libs/unittest-cpp/UnitTest++/src/MemoryOutStream.cpp \
 libs/unittest-cpp/UnitTest++/src/DeferredTestReporter.cpp \
 libs/unittest-cpp/UnitTest++/src/DeferredTestResult.cpp \
 libs/unittest-cpp/UnitTest++/src/XmlTestReporter.cpp \
 libs/unittest-cpp/UnitTest++/src/CurrentTest.cpp \
 libs/unittest-cpp/UnitTest++/src/Posix/SignalTranslator.cpp \
 libs/unittest-cpp/UnitTest++/src/Posix/TimeHelpers.cpp

# MicroHttpd
INCLUDES += \
 -I$(top_srcdir)/libs/libmicrohttpd/src/include

# Mongo-C-Driver
noinst_LIBRARIES += libmongocdriver.a
INCLUDES += \
 -I$(top_srcdir)/libs/mongo-c-driver/src
noinst_HEADERS += \
 libs/mongo-c-driver/src/bson.h \
 libs/mongo-c-driver/src/encoding.h \
 libs/mongo-c-driver/src/gridfs.h \
 libs/mongo-c-driver/src/md5.h \
 libs/mongo-c-driver/src/mongo_except.h \
 libs/mongo-c-driver/src/mongo.h \
 libs/mongo-c-driver/src/platform_hacks.h
libmongocdriver_a_SOURCES = \
 libs/mongo-c-driver/src/bson.c \
 libs/mongo-c-driver/src/encoding.c \
 libs/mongo-c-driver/src/gridfs.c \
 libs/mongo-c-driver/src/md5.c \
 libs/mongo-c-driver/src/mongo.c \
 libs/mongo-c-driver/src/numbers.c
