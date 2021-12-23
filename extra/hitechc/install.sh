#!/bin/bash
#
# copy the compiler and library to the linux machine's zxcc directory
dest=/usr/local/lib/cpm
bindest=$dest/bin80

cp -v C.COM $bindest/c.com
cp -v \$EXEC.COM $bindest/\$exec.com
cp -v LINK.COM $bindest/LINK.com

