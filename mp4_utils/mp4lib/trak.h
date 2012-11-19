#pragma once

#include "atom_structs.h"

#define MAX_TRACKS 8

class trak
{
public:
	trak();
	trak( const trak & t )
		: start_( t.start_ )
		, tkhd_( t.tkhd_ )
		, mdia_( t.mdia_ )
	{
		copy_chunks_samples(t);
	}
	~trak();

public:
	void parse( unsigned char* buffer, unsigned int size);

	void trak_build_index();

	trak & operator=( const trak & t )
	{
		start_ = t.start_;
		tkhd_ = t.tkhd_;
		mdia_ = t.mdia_;

		copy_chunks_samples(t);
	}

private:
	void copy_chunks_samples(trak const&t);

	void calc_chunk_offset(  void const* stco );

	void process_chunk_map();

	void calc_chunk_pts();

	void calc_pts();

	void calc_composition_times();

	void calc_sample_offset();

public:
	unsigned char* start_;
	unsigned char* tkhd_;
	struct mdia_t mdia_;

	/* temporary indices */
	unsigned int chunks_size_;
	scoped_array<chunks_t> chunks_;

	unsigned int samples_size_;
	scoped_array<samples_t> samples_;
};
