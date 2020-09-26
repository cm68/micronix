#!/bin/bash
for file in ../src/sgs/arc/*.o ; do 
	./dis $file > s/$(basename $file .o).s
done
