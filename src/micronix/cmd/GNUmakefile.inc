#
# run the ws compiler under simulation to build $(DIR)
#
ROOT=~/src/micronix/filesystem
SIM=~/src/micronix/sim
SRCDIR=/usr/src/$(CLASS)/$(DIR)
DEST=$(ROOT)/$(SRCDIR)
NOWARN = --warning=no-ignore-newer

all: $(DEST)/make.sh
	-$(SIM) -d $(ROOT) /bin/sh /$(SRCDIR)/make.sh

clean: $(DEST)/make.sh
	-$(SIM) -d $(ROOT) /bin/sh /$(SRCDIR)/make.sh clean
	rm -f *.o

clobber: $(DEST)/make.sh clean
	-$(SIM) -d $(ROOT) /bin/sh /$(SRCDIR)/make.sh clobber
	rm -f $(TARGET) make.sh $(DIR).tar $(GENERATED)

$(DEST)/make.sh: GNUmakefile ../GNUmakefile.inc $(SRCS) makefile
	echo "echo" > make.sh
	echo "cd $(SRCDIR)" > make.sh
	echo "make \$$1" >> make.sh
	mkdir -p $(DEST)
	tar -f $(DIR).tar --ignore-failed-read -c makefile $(SRCS) make.sh
	tar -f $(DIR).tar -C $(DEST) --keep-newer-files -x $(NOWARN)
