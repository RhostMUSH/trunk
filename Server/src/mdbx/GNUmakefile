# This makefile is for GNU Make, and nowadays provided
# just for compatibility and preservation of traditions.
# Please use CMake in case of any difficulties or problems.
#
# Preprocessor macros (for MDBX_OPTIONS) of interest...
# Note that the defaults should already be correct for most platforms;
# you should not need to change any of these. Read their descriptions
# in README and source code if you do. There may be other macros of interest.
SHELL   := env bash

# install sandbox
SANDBOX ?=

# install prefixes (inside sandbox)
prefix  ?= /usr/local
mandir  ?= $(prefix)/man

# lib/bin suffix for multiarch/biarch, e.g. '.x86_64'
suffix  ?=

CC      ?= gcc
LD      ?= ld
MDBX_OPTIONS ?= -DNDEBUG=1
CFLAGS  ?= -O2 -g -Wall -Werror -Wextra -Wpedantic -ffunction-sections -fPIC -fvisibility=hidden -std=gnu11 -pthread -Wno-error=attributes
# -Wno-tautological-compare

# HINT: Try append '--no-as-needed,-lrt' for ability to built with modern glibc, but then run with the old.
LIBS    ?= $(shell uname | grep -qi SunOS && echo "-lkstat") $(shell uname | grep -qi -e Darwin -e OpenBSD || echo "-lrt")

LDFLAGS ?= $(shell $(LD) --help 2>/dev/null | grep -q -- --gc-sections && echo '-Wl,--gc-sections,-z,relro,-O1')$(shell $(LD) --help 2>/dev/null | grep -q -- -dead_strip && echo '-Wl,-dead_strip')
EXE_LDFLAGS ?= -pthread

################################################################################

UNAME      := $(shell uname -s 2>/dev/null || echo Unknown)
define uname2sosuffix
  case "$(UNAME)" in
    Darwin*|Mach*) echo dylib;;
    CYGWIN*|MINGW*|MSYS*|Windows*) echo dll;;
    *) echo so;;
  esac
endef
SO_SUFFIX  := $(shell $(uname2sosuffix))

HEADERS    := mdbx.h
LIBRARIES  := libmdbx.a libmdbx.$(SO_SUFFIX)

.PHONY: mdbx all install clean test dist check

all: $(LIBRARIES)

mdbx: libmdbx.a libmdbx.$(SO_SUFFIX)

strip: all
	strip libmdbx.$(SO_SUFFIX)

clean:
	rm -rf $(TOOLS) mdbx_test @* *.[ao] *.[ls]o *~ tmp.db/* \
		*.gcov *.log *.err src/*.o test/*.o mdbx_example dist \
		config.h src/elements/config.h src/elements/version.c *.tar*

libmdbx.a: mdbx-static.o
	$(AR) rs $@ $?

libmdbx.$(SO_SUFFIX): mdbx-dylib.o
	$(CC) $(CFLAGS) $^ -pthread -shared $(LDFLAGS) $(LIBS) -o $@


################################################################################
# Amalgamated source code, i.e. distributed after `make dists`

config.h: mdbx.c $(lastword $(MAKEFILE_LIST))
	(echo '#define MDBX_BUILD_TIMESTAMP "$(shell date +%Y-%m-%dT%H:%M:%S%z)"' \
	&& echo '#define MDBX_BUILD_FLAGS "$(CFLAGS) $(LDFLAGS) $(LIBS)"' \
	&& echo '#define MDBX_BUILD_COMPILER "$(shell set -o pipefail; $(CC) --version | head -1 || echo 'Please use GCC or CLANG compatible compiler')"' \
	&& echo '#define MDBX_BUILD_TARGET "$(shell set -o pipefail; LC_ALL=C $(CC) -v 2>&1 | grep -i '^Target:' | cut -d ' ' -f 2- || echo 'Please use GCC or CLANG compatible compiler')"' \
	) > $@

mdbx-dylib.o: config.h mdbx.c $(lastword $(MAKEFILE_LIST))
	$(CC) $(CFLAGS) $(MDBX_OPTIONS) '-DMDBX_CONFIG_H="config.h"' -DLIBMDBX_EXPORTS=1 -c mdbx.c -o $@

mdbx-static.o: config.h mdbx.c $(lastword $(MAKEFILE_LIST))
	$(CC) $(CFLAGS) $(MDBX_OPTIONS) '-DMDBX_CONFIG_H="config.h"' -ULIBMDBX_EXPORTS -c mdbx.c -o $@

mdbx_%:	mdbx_%.c libmdbx.a
	$(CC) $(CFLAGS) $(MDBX_OPTIONS) '-DMDBX_CONFIG_H="config.h"' $^ $(EXE_LDFLAGS) $(LIBS) -o $@

install: $(LIBRARIES) $(TOOLS) $(HEADERS)
	mkdir -p $(SANDBOX)$(prefix)/lib$(suffix) \
		&& cp -t $(SANDBOX)$(prefix)/lib$(suffix) $(LIBRARIES) && \
	mkdir -p $(SANDBOX)$(prefix)/include \
		&& cp -t $(SANDBOX)$(prefix)/include $(HEADERS) && \
