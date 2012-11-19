#pragma once

#include "../mp4lib/atoms.h"

class track_status {
public:
	track_status()
		: trak_(NULL)
		, chunk_index_(0)
		, chunk_count_(0)
		, current_chunk_time_(0.0)
	{
	}

	track_status( trak * t, size_t count )
		: trak_(t)
		, chunk_index_(0)
		, chunk_count_(count)
		, current_chunk_time_(0.0)
	{
	}

	bool chunks_written() {
		return chunk_index_ == chunk_count_;
	}

public:
	trak *		trak_;
	size_t		chunk_index_;
	size_t		chunk_count_;
	double		current_chunk_time_;
};
