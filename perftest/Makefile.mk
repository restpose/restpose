check_PROGRAMS += logperf

# TESTS += logperf$(EXEEXT)

# Source files holding tests.
logperf_SOURCES = \
 perftest/logperf.cc

logperf_LDADD = \
 liblogger.a \
 libutils.a \
 libxapiancommon.a \
 libs/libmicrohttpd/src/daemon/libmicrohttpd.la \
 $(XAPIAN_LIBS)

logperf_LDFLAGS = \
 -pthread
