noinst_LIBRARIES += libmatchspies.a

noinst_HEADERS += \
 src/matchspies/facetmatchspy.h \
 src/matchspies/termoccurmatchspy.h

libmatchspies_a_SOURCES = \
 src/matchspies/facetmatchspy.cc \
 src/matchspies/termoccurmatchspy.cc
