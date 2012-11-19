#pragma once
#include "atom.h"
#include "trak.h"

class moov
{
public:
	moov(void)
		: start_(NULL)
		, tracks_(0)
		, mvhd_(NULL)
	{
	}
	~moov()
	{
	}

public:
	int parse( byte * buffer, size_t size );

public:
	unsigned char* start_;
	unsigned int tracks_;
	unsigned char* mvhd_;
	trak traks_[MAX_TRACKS];
};
