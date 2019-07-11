/*
 * extensible I/O framework
 * devices need to register I/O addresses and functions, which get
 * called whenever I/O instructions are issued
 */

typedef unsigned char portaddr;
typedef unsigned char portdata;

portdata (*input_handler[256])(portaddr port);
void (*output_handler[256])(portaddr port, portdata data);

void
register_input(
	portaddr portnum, 
	portdata (*handler)(portaddr port)) 
{
	input_handler[portnum] = handler;
}

void
register_output(
portaddr portnum, 
void (*handler)(portaddr port, portdata data)) 
{
	output_handler[portnum] = handler;
}

portdata
undef_in(portaddr p)
{
	return 0xff;
}

void
undef_out(portaddr p, portdata v)
{
}

void
ioinit()
{
	int i;
	for (i = 0; i < 256; i++) {
		input_handler[i] = undef_in;
		output_handler[i] = undef_out;
	}
}

void
output(portaddr p, portdata v)
{
	(*output_handler)(p,v);
}

portdata
input(portaddr p)
{
	return (*input_handler)(p);
}

