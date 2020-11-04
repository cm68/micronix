int jmp_buf[3] = 0;

foo()
{
	longjmp(jmp_buf, 1);
}

main()
{
	if (setjmp(jmp_buf) == 1) {
		write(1, "got here\n", 9);
		exit(1);
	}
	foo();
}
