noinst_LIBRARIES += libfeatures.a

noinst_HEADERS += \
 src/features/category_handlers.h \
 src/features/category_tasks.h \
 src/features/checkpoint_handlers.h \
 src/features/checkpoint_tasks.h \
 src/features/coll_handlers.h \
 src/features/coll_tasks.h

libfeatures_a_SOURCES = \
 src/features/category_handlers.cc \
 src/features/category_tasks.cc \
 src/features/checkpoint_handlers.cc \
 src/features/checkpoint_tasks.cc \
 src/features/coll_handlers.cc \
 src/features/coll_tasks.cc
