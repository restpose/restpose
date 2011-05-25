noinst_LIBRARIES += libngramcat.a

noinst_HEADERS += \
 src/ngramcat/categoriser.h \
 src/ngramcat/profile.h

libngramcat_a_SOURCES = \
 src/ngramcat/categoriser.cc \
 src/ngramcat/profile.cc
