noinst_LIBRARIES += libserver.a

noinst_HEADERS += \
 src/server/basetasks.h \
 src/server/collection_pool.h \
 src/server/ignore_sigpipe.h \
 src/server/result_handle.h \
 src/server/server.h \
 src/server/signals.h \
 src/server/task_manager.h \
 src/server/task_queue_group.h \
 src/server/task_threads.h \
 src/server/tasks.h \
 src/server/thread_pool.h

libserver_a_SOURCES = \
 src/server/collection_pool.cc \
 src/server/ignore_sigpipe.cc \
 src/server/result_handle.cc \
 src/server/server.cc \
 src/server/signals.cc \
 src/server/task_manager.cc \
 src/server/task_threads.cc \
 src/server/tasks.cc \
 src/server/thread_pool.cc
