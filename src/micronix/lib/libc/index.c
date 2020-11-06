/*
 * el bizarro semantics: if we pass in a '\0', then hit at end.
 */
char *
index(s, c)
char *s;
char c;
{
	do {
		if (*s == c) return s;
	} while (*s++);
	return 0;	
}
