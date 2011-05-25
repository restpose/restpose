bin_PROGRAMS += restpose

INCLUDES += -I$(top_srcdir)/src

restpose_SOURCES = \
 src/restpose.cc

restpose_LDADD = \
 libfilesystemimporter.a \
 libmongoimporter.a \
 libjsonxapian.a \
 libjsonmanip.a \
 libhttpserver.a \
 librest.a \
 libserver.a \
 libngramcat.a \
 libcjktokenizer.a \
 libjsoncpp.a \
 libdbgroup.a \
 libutils.a \
 libmatchspies.a \
 libxapiancommon.a \
 libmongocdriver.a \
 libcli.a \
 libs/libmicrohttpd/src/daemon/libmicrohttpd.la \
 $(XAPIAN_LIBS)
