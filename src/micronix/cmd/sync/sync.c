/*
 * sync - flush the buffer cache
 *
 * cmd/sync/sync.c
 *
 * This was a shell builtin, and nothing about it wanted to be one.  A
 * builtin earns its place by needing the shell's own state - cd moves
 * the shell, exit ends it, alias and path are its tables - and sync
 * needs none of it: the call is the whole program.  Made a command,
 * it can be run by anything that runs commands, and the shell's table
 * gets shorter.
 *
 * vim: tabstop=4 shiftwidth=4 noexpandtab:
 */

main()
{
	sync();
	exit(0);
}
