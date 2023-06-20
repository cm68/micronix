unsigned char memory[65536];

void
put_word(unsigned short addr, unsigned short value)
{
    memory[addr] = value & 0xff;
    memory[addr + 1] = (value >> 8) & 0xff;
}

void
put_byte(unsigned short addr, unsigned char value)
{
    memory[addr] = value;
}

unsigned short
get_word(unsigned short addr)
{
    return memory[addr] + (memory[addr + 1] << 8);
}

unsigned char
get_byte(unsigned short addr)
{
    addr &= 0xffff;
    return memory[addr];
}

unsigned char
input(unsigned short p)
{
	return 0;
}

void
output(unsigned short p, unsigned char v)
{
}

unsigned char
int_ack()
{
	return 0;
}

