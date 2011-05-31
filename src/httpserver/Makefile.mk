noinst_LIBRARIES += libhttpserver.a

noinst_HEADERS += \
 src/httpserver/httpserver.h \
 src/httpserver/response.h

libhttpserver_a_SOURCES = \
 src/httpserver/httpserver.cc
