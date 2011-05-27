noinst_LIBRARIES += libserver.a

noinst_HEADERS += \
 src/server/collection_pool.h \
 src/server/ignore_sigpipe.h \
 src/server/result_handle.h \
 src/server/server.h \
 src/server/signals.h \
 src/server/task_manager.h

libserver_a_SOURCES = \
 src/server/collection_pool.cc \
 src/server/ignore_sigpipe.cc \
 src/server/result_handle.cc \
 src/server/server.cc \
 src/server/signals.cc \
 src/server/task_manager.cc