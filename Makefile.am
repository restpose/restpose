## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.5 subdir-objects
ACLOCAL_AMFLAGS = -I m4

LIBTOOL = @LIBTOOL@ $(QUIET:y=--quiet)

INCLUDES =
bin_PROGRAMS =
check_PROGRAMS =
TESTS =
EXTRA_DIST =

AM_LDFLAGS = $(NO_UNDEFINED)

AM_CXXFLAGS += $(XAPIAN_CXXFLAGS)

noinst_LIBRARIES =
noinst_HEADERS =

include libs/Makefile.mk
include src/Makefile.mk
include src/cli/Makefile.mk
include src/dbgroup/Makefile.mk
include src/httpserver/Makefile.mk
# include src/importer/filesystem/Makefile.mk
include src/importer/mongo/Makefile.mk
include src/jsonmanip/Makefile.mk
include src/jsonxapian/Makefile.mk
include src/logger/Makefile.mk
include src/matchspies/Makefile.mk
include src/ngramcat/Makefile.mk
include src/rest/Makefile.mk
include src/server/Makefile.mk
include src/utils/Makefile.mk
include unittests/Makefile.mk

SUBDIRS = libs/libmicrohttpd .

DISTCHECK_CONFIGURE_FLAGS = "XAPIAN_CONFIG=$(XAPIAN_CONFIG)"

EXTRA_DIST += \
 examples/lang_guess_model.json \
 examples/schema.json \
 scripts/get_wikipedia_sample_text \
 scripts/ma_index
