#pragma once
#include "../mp4lib/atoms.h"
#include "track_status.h"

class media_track_info;
typedef shared_ptr<media_track_info> media_track_ptr;

class media_track_info
{
public:
	media_track_info( double start_time, double duration, size_t sample_start )
		: start_time_( start_time )
		, end_time_( duration + start_time_ )
		, sample_start_( sample_start )
		, sample_end_(0)
	{}

	double update_duration( trak & track, size_t moov_time_scale )
	{
		double duration = end_time_ - start_time_;

		size_t time_scale = ::mdhd_get_time_scale( track.mdia_.mdhd_ );
		size_t ticks = static_cast<size_t>( duration * time_scale / 1000 );
		::mdhd_set_duration( track.mdia_.mdhd_, ticks );

		size_t moov_ticks = static_cast<size_t>( duration * moov_time_scale / 1000 );
		::tkhd_set_duration(track.tkhd_, moov_ticks );

		return duration;
	}

	void update_chunk_offset( trak & track )
	{
		unsigned char* stco = track.mdia_.minf_.stbl_.stco_;
		unsigned int entries = buffer_io::read_int32(stco + 4);
		uint32_t* stco_table = (uint32_t*)(stco + 8);

		for ( size_t chunk_index = 0; chunk_index < entries; ++ chunk_index )
		{
			buffer_io::write_int32( stco_table + chunk_index, track.chunks_[chunk_index].pos_ );
		}
	}

	void update_track_status( trak & track )
	{	
		status_.trak_ = &track;
		status_.chunk_index_ = 0;
		status_.current_chunk_time_ = 0;
		status_.chunk_count_ = track.chunks_size_;
	}

	void fix_sample_table( trak & track );

private:

	void fix_ctts( trak & track );
	void fix_stsc( trak & track );
	void fix_stts( trak & track );
	void fix_stss( trak & track );

	void fix_stsz( trak & track );
public:
	double		start_time_;
	double		end_time_;

	size_t		sample_start_;	// start at 1
	size_t		sample_end_;

	track_status	status_;
};
