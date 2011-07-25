bin_PROGRAMS += restpose

INCLUDES += -I$(top_srcdir)/src

restpose_SOURCES = \
 src/restpose.cc

restpose_LDADD = \
 libmongoimporter.a \
 libhttpserver.a \
 librest.a \
 libfeatures.a \
 libserver.a \
 libjsonxapian.a \
 libjsonmanip.a \
 libngramcat.a \
 libcjktokenizer.a \
 liblogger.a \
 libdbgroup.a \
 libutils.a \
 libmatchspies.a \
 libxapiancommon.a \
 libmongocdriver.a \
 libcli.a \
 libjsoncpp.a \
 libs/libmicrohttpd/src/daemon/libmicrohttpd.la \
 $(XAPIAN_LIBS)
