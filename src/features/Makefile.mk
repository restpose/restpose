noinst_LIBRARIES += libfeatures.a

noinst_HEADERS += \
 src/features/checkpoint_handlers.h \
 src/features/checkpoint_tasks.h

libfeatures_a_SOURCES = \
 src/features/checkpoint_handlers.cc \
 src/features/checkpoint_tasks.cc
