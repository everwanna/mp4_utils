#include "StdAfx.h"
#include "buffer_io.h"

buffer_io::buffer_io(void)
{
}

buffer_io::~buffer_io(void)
{
}

int buffer_io::read_char(unsigned char const* buffer)
{
	return buffer[0];
}

void buffer_io::write_char(unsigned char* outbuffer, int value)
{
	outbuffer[0] = (unsigned char)(value);
}

int buffer_io::read_int32(void const* buffer)
{
	unsigned char* p = (unsigned char*)buffer;
	return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
}

void buffer_io::write_int32(void* outbuffer, uint32_t value)
{
	unsigned char* p = (unsigned char*)outbuffer;
	p[0] = (unsigned char)((value >> 24) & 0xff);
	p[1] = (unsigned char)((value >> 16) & 0xff);
	p[2] = (unsigned char)((value >> 8) & 0xff);
	p[3] = (unsigned char)((value >> 0) & 0xff);
}
