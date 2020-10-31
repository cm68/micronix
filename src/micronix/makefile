#
# makefile is tried first, before Makefile, by the new make
# the old make tries Makefile first
# so, if you have both Makefile and makefile, it depends on which
# make you are running.
# this will be used to bootstrap the new make on a system where
# the new make does not exist yet.
#
# this is the makefile used by the new make
#

all:
	cd lib ; make
	cd cmd ; make
	cd sys ; make

#
# the update rule is used when inside the source tree in the simulator
# the micronix shell cannot handle this syntax at all
#
update:
	find . -type f \! -path \*UNUSED\* | \
        grep -v \.o$$ | grep -v \.swp | while read i ; do \
		dest=../../filesystem/usr/src/$$i ; \
        if [ ! -d $$(dirname $$dest) ] ; then \
			echo new dir: $$(dirname $$dest) ; \
            mkdir -p $$(dirname $$dest) ; \
        fi ; \
        if [ ! -f $$dest ] ; then \
            echo new: $$i ; \
            cp $$i $$dest ; \
        fi ; \
        if ! cmp -s $$i $$dest ; then \
            echo different: $$i ; \
            cp $$i $$dest ; \
        fi ; \
    done

