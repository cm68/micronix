KERNELS = micronix.14 micronix.161 micronix.167 micronix.new

symbols:
	for k in $(KERNELS) ; do \
		../src/tools/nm $$k | \
			tail -n +2 | tr -d : | \
			awk '{print "0x"$$3, $$2}' > $$k.sym ; \
	done
