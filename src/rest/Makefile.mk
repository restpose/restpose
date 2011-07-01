noinst_LIBRARIES += librest.a

noinst_HEADERS += \
 src/rest/handler.h \
 src/rest/handlers.h \
 src/rest/router.h \
 src/rest/routes.h

librest_a_SOURCES = \
 src/rest/handler.cc \
 src/rest/handlers.cc \
 src/rest/router.cc \
 src/rest/routes.cc
