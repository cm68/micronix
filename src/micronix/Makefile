all:
	cd lib ; make
	cd cmd ; make
	cd sys ; make

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

