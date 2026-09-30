# translate_slr.sed - zmac -> SLR Z80ASM.  Pure syntax: drop the zmac-only
# directives and convert zmac's trailing-Q octal to SLR's hex.  SLR's default
# E option uppercases, so lowercase db/equ/800h/.phase are accepted as-is.
/^[ 	]*\.z80[ 	]*$/d
/^[ 	]*aseg[ 	]*$/d
s/374Q/0FCH/g
s/370Q/0F8H/g
s/200Q/80H/g
s/120Q/50H/g
s/40Q/20H/g
s/10Q/8H/g
