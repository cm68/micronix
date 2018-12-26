.:=.data
_foo:
0150,0145,0154,0154,0157,054,040,0167
0157,0162,0154,0144,012,0
public _printf
public _strcpy
public _main
public _bar
public _foo
.:=.text
_main:
call c.ent
hl=&_foo
sp<=hl
hl=&_bar
sp<=hl
call _strcpy
af<=sp<=sp
hl=&_bar
sp<=hl
call _printf
af<=sp
jmp c.ret
