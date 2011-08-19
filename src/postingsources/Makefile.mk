noinst_LIBRARIES += libpostingsources.a

noinst_HEADERS += \
 src/postingsources/multivalue_keymaker.h \
 src/postingsources/multivaluerange_source.h

libpostingsources_a_SOURCES = \
 src/postingsources/multivalue_keymaker.cc \
 src/postingsources/multivaluerange_source.cc
