#pragma once

#include "media_track_info.h"

class clip_info;
typedef shared_ptr<clip_info> clip_ptr;

class clip_info
{
public:
	clip_info( size_t file_index, double audio_start_time, double video_start_time, double duration, size_t audio_start, size_t video_start )
		: file_index_( file_index )

		, start_time_( audio_start_time )
		, duration_( duration )

		, audio_track_( new media_track_info( audio_start_time, duration, audio_start ) )
		, video_track_( new media_track_info( video_start_time, duration, video_start ) )
		, audio_track_index_( static_cast<size_t>(-1) )
		, video_track_index_( static_cast<size_t>(-1) )

		, new_moov_size_(0)
		, new_moov_buffer_()

		, new_moov_()

		, mdat_buffer_size_(8)
		, mdat_header_buffer_( new byte[mdat_buffer_size_] )
	{
		if ( video_start_time < start_time_ )
		{
			start_time_ = video_start_time;
		}

		mdat_header_buffer_[4] = 'm';
		mdat_header_buffer_[5] = 'd';
		mdat_header_buffer_[6] = 'a';
		mdat_header_buffer_[7] = 't';
	}

	void build_new_moov( const byte * const data, size_t size );

	void update_chunk_offset()
	{
		audio_track_->update_chunk_offset( new_moov_->traks_[audio_track_index_] );
		video_track_->update_chunk_offset( new_moov_->traks_[video_track_index_] );
	}

private:

	void init_moov( const byte * const data, size_t size );

	void shrink_header()
	{
		for( size_t i = 0; i < new_moov_->tracks_; ++ i )
		{
			size_t shrink_size = shrink_from_track( new_moov_->traks_[ i ] );

			decrease_box_size( new_moov_buffer_.get(), new_moov_size_, shrink_size );

			new_moov_.reset( new moov() );
			new_moov_->parse( new_moov_buffer_.get() + atom::PREAMBLE_SIZE, new_moov_size_ - atom::PREAMBLE_SIZE );
		}
	}

	size_t decrease_box_size( byte * const data, size_t data_size, size_t decreased_size )
	{
		assert( data_size >= atom::PREAMBLE_SIZE );

		size_t original_size = static_cast<size_t>( buffer_io::read_int32( data ) );
		size_t new_size = original_size - decreased_size;

		assert( new_size <= original_size );

		buffer_io::write_int32( data, new_size );

		return new_size;
	}

	size_t shrink_from_track( trak & track );

	void update_moov_durations();

	double update_duration( double duration )
	{
		size_t time_scale = ::mvhd_get_time_scale( new_moov_->mvhd_ );

		size_t ticks = static_cast<size_t>( duration * time_scale / 1000 );
		::mvhd_set_duration( new_moov_->mvhd_, ticks );

		return duration;
	}

	public:
		size_t		file_index_;

		double		start_time_;
		double		duration_;

		media_track_ptr	audio_track_;
		media_track_ptr	video_track_;

		size_t				audio_track_index_;
		size_t				video_track_index_;

		size_t				new_moov_size_;
		shared_array<byte>	new_moov_buffer_;

		shared_ptr<moov>	new_moov_;

		size_t				mdat_buffer_size_;
		shared_array<byte>	mdat_header_buffer_;
	};
