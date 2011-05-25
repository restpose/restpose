noinst_LIBRARIES += libjsonmanip.a

noinst_HEADERS += \
 src/jsonmanip/conditionals.h \
 src/jsonmanip/jsonpath.h \
 src/jsonmanip/mapping.h

libjsonmanip_a_SOURCES = \
 src/jsonmanip/conditionals.cc \
 src/jsonmanip/jsonpath.cc \
 src/jsonmanip/mapping.cc
