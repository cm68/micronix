# the native build: mount /usr/src on drive 1 and run make, which
# rebuilds include, lib, the toolchain, libexec and every command in
# turn.  Written as /.login after the install passes are done.  The
# marker at the end names a build that returned, not one that succeeded
# - check the log for the last "Build successful:" and any error lines.
mount /dev/hd1a /usr/src
cd /usr/src
make
sync
echo === COMPILE-DONE
