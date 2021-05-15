char buf[] = "hello, world\n";

main()
{
	write(1, buf, sizeof(buf));
}

