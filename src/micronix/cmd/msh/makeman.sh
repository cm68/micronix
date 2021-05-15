#!/bin/bash
for i in *.1 ; do
	nroff -man $i > $(basename $i .1).0
done
