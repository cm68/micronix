#
# Makefile include file for cross build and update rules for micronix 
#
# GNUmakefile.inc
#
# Changed: <2023-07-04 19:55:33 curt>
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:

MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables
.SUFFIXES:

WARNS = -Wno-builtin-declaration-mismatch -Wformat-overflow=0 -Wno-pointer-sign \
        -fdiagnostics-color=always -fno-builtin

#
# calling makefiles should define
# DEPTH = levels up to this file in the form ../../..
# DIR = <directory name>
# CLASS = <prefix before DIR>
# DIRS = list of directories to recurse into
# TARGET = primary object file

# SRCS = list of s, c and h source files
# DOCSRCS = list of .mp and .ic files

ROOT=~/src/micronix/filesystem
SIM=~/src/micronix/sim
SRCDIR=/usr/src/$(CLASS)/$(DIR)
DEST=$(ROOT)/$(SRCDIR)
NOWARN = --warning=no-ignore-newer

CSRCS = $(wildcard *.c)
HSRCS = $(wildcard *.h)
ASSRCS = $(wildcard *.s)
DOCSRCS = $(wildcard *.mp *.ic)

SRCS = $(CSRCS) $(HSRCS) $(ASSRCS)

MKFILE = $(wildcard ?akefile)
DOCFILES = $(DOCSRCS:.mp=.0) $(DOCSRCS:.ic=.3)
OBJECTS = $(patsubst %.c,%.o,$(CSRCS))

MAKESH = $(DEST)/make.sh

ifdef CROSS

INCFLAG ?= -I$(DEPTH)/cmd/whitesmith/include
LIBS ?= -L../lib/libws -lWS

CCOPTS = -gdwarf -g3 -Og -m32 \
	-fno-eliminate-unused-debug-symbols -fno-eliminate-unused-debug-types
CC = gcc

ifdef TARGET
ifeq ($(suffix $(TARGET)),.a)
$(TARGET): $(OBJECTS)
	rm -f $(TARGET)
	ar -rcvs $(TARGET) $(OBJECTS)
else
$(TARGET): $(OBJECTS)
	$(CC) $(CCOPTS) -o $(TARGET) $(OBJECTS) $(LIBS)
endif
endif

%: $^
	$(CC) $(CCOPTS) -o $@ $^ -L../lib/libws -lWS

%.o : %.c
	$(CC) $(CCOPTS) $(WARNS) $(INCFLAG) -c $<

else

%.o : %.c $(MAKESH)
	-$(SIM) -d $(ROOT) /bin/sh /$(SRCDIR)/make.sh $@

%: $^ $(MAKESH)
	-$(SIM) -d $(ROOT) /bin/sh /$(SRCDIR)/make.sh $@

endif

all: $(MAKESH) $(TARGET) $(TARGETS) doc
	for dir in $(DIRS) ; do (cd $$dir ; make all) ; done
ifndef CROSS
	-$(SIM) -d $(ROOT) /bin/sh /$(SRCDIR)/make.sh
endif

clean: $(MAKESH)
	for dir in $(DIRS) ; do (cd $$dir ; make clean) ; done
ifndef CROSS
	-$(SIM) -d $(ROOT) /bin/sh /$(SRCDIR)/make.sh clean
endif
	rm -f *.o

clobber: $(MAKESH) clean
	for dir in $(DIRS) ; do (cd $$dir ; make clobber) ; done
ifndef CROSS
	-$(SIM) -d $(ROOT) /bin/sh /$(SRCDIR)/make.sh clobber
endif
	rm -f $(TARGET) make.sh $(DIR).tar $(GENERATED)

install: $(MAKESH)
	for dir in $(DIRS) ; do (cd $$dir ; make install) ; done
ifndef CROSS
	-$(SIM) -d $(ROOT) /bin/sh /$(SRCDIR)/make.sh install
endif

ifndef CROSS
$(DEST)/make.sh: GNUmakefile $(DEPTH)/GNUmakefile.inc $(SRCS) $(MKFILE)
	echo "echo" > make.sh
	echo "cd $(SRCDIR)" > make.sh
	echo "make \$$1" >> make.sh
	mkdir -p $(DEST)
	tar -f $(DIR).tar --ignore-failed-read -c $(MKFILE) $(SRCS) make.sh
	tar -f $(DIR).tar -C $(DEST) --keep-newer-files -x $(NOWARN)
endif

doc: $(DOCFILES)
	for dir in $(DIRS) ; do (cd $$dir ; make doc) ; done

%.0 : %.mp
	nroff -M.. $< -mws > $@

%.3 : %.ic
	nroff -M.. $< -mws > $@

disas:
	disas $(DEST)/$(TARGET) >$(TARGET).dis
