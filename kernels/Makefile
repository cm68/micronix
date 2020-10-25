symbols:
	for k in micronix.* ; do \
		../src/tools/nm $$k | \
			tail -n +2 | tr -d : | \
			awk '{print "0x"$$3, $$2}' > $$k.sym ; \
	done
