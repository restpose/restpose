noinst_LIBRARIES += libutils.a

noinst_HEADERS += \
 src/utils/compression.h \
 src/utils/io_wrappers.h \
 src/utils/jsonutils.h \
 src/utils/queueing.h \
 src/utils/rmdir.h \
 src/utils/rsperrors.h \
 src/utils/safe_inttypes.h \
 src/utils/safe_zlib.h \
 src/utils/stringutils.h \
 src/utils/threading.h \
 src/utils/threadsafequeue.h \
 src/utils/utils.h

libutils_a_SOURCES = \
 src/utils/compression.cc \
 src/utils/io_wrappers.cc \
 src/utils/jsonutils.cc \
 src/utils/rmdir.cc \
 src/utils/rsperrors.cc \
 src/utils/threading.cc \
 src/utils/utils.cc
