noinst_LIBRARIES += librest.a

noinst_HEADERS += \
 src/rest/handler.h \
 src/rest/handlers.h \
 src/rest/router.h

librest_a_SOURCES = \
 src/rest/router.cc
