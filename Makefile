# Top-level Makefile.  This file coordinates the actions of the fragments and
# provides global values.

WARNINGS = -Wall -W -Wconversion -Wsign-compare \
	-Wdisabled-optimization -D_GLIBCPP_CONCEPT_CHECKS
OPTIMIZE_FLAGS = -O3 -march=pentium3 -pipe -ffast-math \
	-g -fomit-frame-pointer
CXXFLAGS = $(WARNINGS) $(OPTIMIZE_FLAGS)
LDFLAGS =
CXX = g++-3.3
SHELL = /bin/sh

CPPFLAGS = -Iinclude
.DEFAULT: all

# Variables that the fragments can add to.
bins = 
libs = 
clean_files = 
distclean_files =

include Makefile.core
include Makefile.gtk2
include Makefile.tests

# Default pattern rules.  These are overridden for each of the fragments right
# now.
%.lo: %.cpp
	$(CXX) -c -o $@ $(CPPFLAGS) -MMD -MF $*.d -fpic $(CXXFLAGS) $<

%.o: %.cpp
	$(CXX) -c -o $@ $(CPPFLAGS) -MMD -MF $*.d  $(CXXFLAGS) $<

.PHONY: all clean distclean
.SUFFIXES:

all: $(libs) $(bins)

clean:
	-rm -f $(clean_files)

distclean: clean
	-rm -f $(distclean_files)
