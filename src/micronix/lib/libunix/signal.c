/*
 * this is a hack to make the signal handler save registers in and out
 */

short stab[15];
/*
 * The trampolines are not a fixed stride: the assembler shortens a
 * jump when it can reach, and the addresses have to be read rather
 * than computed.
 */
extern short jtaddr[];
extern short _signal(int s, short h);

short
signal(short sig, short handler)
{
	short ret;

	if (sig < 1 || sig > 15) {
		return -1;
	}
	if (handler == 0 || handler == 1) {
		ret = _signal(sig, handler);
	} else {
		stab[sig - 1] = handler;
		ret = _signal(sig, jtaddr[sig - 1]);
	}
	if (!(ret == 1 || ret == 0 || ret == -1)) {
		ret = stab[sig - 1];
	} 
	return ret;
}

/* vim: set tabstop=4 shiftwidth=4 noexpandtab: */
