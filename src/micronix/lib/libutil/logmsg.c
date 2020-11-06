logmsg(s)
char *s;
{
	int fd;

    if ((fd = open("logfile", 1)) < 0) {
        fd = creat("logfile", 0777);
    }
    if (fd >= 0) {
        seek(fd, 0, 2);
        write(fd, s, strlen(s));
        write(fd, "\n", 1);
        close(fd);
    }
}
