#include "StdAfx.h"
#include "atom.h"
#include "buffer_io.h"

atom::~atom(void)
{
}

void atom::atom_write_header( unsigned char* outbuffer )
{
	int i;
	buffer_io::write_int32(outbuffer, size_);
	for(i = 0; i != 4; ++i)
	{
		buffer_io::write_char(outbuffer + 4 + i, type_[i]);
	}
}

unsigned char* atom::read_header( unsigned char* buffer )
{
	start_ = buffer;
	memcpy(type_, &buffer[4], 4);
	size_ = atom::header_size(buffer);
	end_ = start_ + size_;

	return buffer + PREAMBLE_SIZE;
}