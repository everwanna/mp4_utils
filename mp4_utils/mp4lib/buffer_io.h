#pragma once

class buffer_io
{
public:
	buffer_io(void);
	~buffer_io(void);

public:
	static void write_char(unsigned char* outbuffer, int value);
	static int read_char(unsigned char const* buffer);

	static int read_int32(void const* buffer);

	static void write_int32(void* outbuffer, uint32_t value);

};
