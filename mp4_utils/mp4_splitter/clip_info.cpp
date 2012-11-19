#include "StdAfx.h"
#include "clip_info.h"


void clip_info::update_moov_durations()
{
	size_t time_scale = ::mvhd_get_time_scale( new_moov_->mvhd_ );

	double audio_duration = audio_track_->update_duration( new_moov_->traks_[ audio_track_index_ ], time_scale );
	double video_duration = video_track_->update_duration( new_moov_->traks_[ video_track_index_ ], time_scale );

	double duration = audio_duration;
	if( duration < video_duration )
	{
		duration = video_duration;
	}

	update_duration( duration );
}

void clip_info::init_moov( const byte * const data, size_t size )
{
	new_moov_size_ = size;
	new_moov_buffer_.reset( new byte[ new_moov_size_ ] );
	::memcpy( new_moov_buffer_.get(), data, size );

	new_moov_.reset( new moov() );
	new_moov_->parse( new_moov_buffer_.get() + atom::PREAMBLE_SIZE, new_moov_size_ - atom::PREAMBLE_SIZE );

	if( new_moov_->traks_[0].mdia_.minf_.stbl_.stss_ != NULL &&
		new_moov_->traks_[1].mdia_.minf_.stbl_.stss_ == NULL )
	{
		video_track_index_ = 0;
		audio_track_index_ = 1;
	}
	else if( new_moov_->traks_[0].mdia_.minf_.stbl_.stss_ == NULL &&
		new_moov_->traks_[1].mdia_.minf_.stbl_.stss_ != NULL )
	{
		video_track_index_ = 1;
		audio_track_index_ = 0;
	}
	else
	{
		throw std::exception( "unsupported mp4 file." );
	}
}

void clip_info::build_new_moov( const byte * const data, size_t size )
{
	init_moov( data, size );

	// fix clip duration and tracks duration.
	update_moov_durations();

	// fix audio track sample tables
	audio_track_->fix_sample_table( new_moov_->traks_[audio_track_index_] );

	// fix video track sample tables
	video_track_->fix_sample_table( new_moov_->traks_[video_track_index_] );

	shrink_header();

	audio_track_->update_track_status( new_moov_->traks_[audio_track_index_] );
	video_track_->update_track_status( new_moov_->traks_[video_track_index_] );
}

size_t clip_info::shrink_from_track( trak & track )
{
	byte * buffer = track.mdia_.minf_.stbl_.start_ - atom::PREAMBLE_SIZE;

	byte * buffer_start = buffer;

	atom leaf_atom;
	buffer = leaf_atom.read_header(buffer);
	size_t size = leaf_atom.size_;

	size_t stbl_shrink_size = 0;

	while(buffer < buffer_start + size)
	{
		buffer = leaf_atom.read_header(buffer);

		size_t org_box_size = leaf_atom.size_;
		size_t new_box_size = leaf_atom.size_;

		if( leaf_atom.is_type("stts") )	{
			size_t entry_count = ::stts_get_entries( buffer );
			new_box_size = 12 + 4 + entry_count * 8;
			assert( new_box_size == org_box_size );	// usually, only 1 entry, no need shrink

		}	else	if(leaf_atom.is_type("ctts"))	{

			size_t entry_count = ::ctts_get_entries( buffer );
			// full_box_size + entry_count_size + entries_size
			new_box_size = 12 + 4 + entry_count * 8;
			assert( new_box_size <= org_box_size );

		}	else	if(leaf_atom.is_type("stss"))	{

			size_t entry_count = ::stts_get_entries( buffer );
			new_box_size = 12 + 4 + entry_count * 4;
			assert( new_box_size <= org_box_size );

		}	else	if(leaf_atom.is_type("stsc"))	{

			size_t entry_count = ::stsc_get_entries( buffer );
			new_box_size = 12 + 4 + entry_count * 12;
			assert( new_box_size <= org_box_size );

		}	else	if(leaf_atom.is_type("stsz"))	{
			
			size_t sample_size = ::stsz_get_sample_size( buffer );
			size_t entry_count = ::stsz_get_entries( buffer );
			new_box_size = 12 + 4 + 4;
			if( sample_size == 0 )
			{
				new_box_size += entry_count * 4;
			}
			assert( new_box_size <= org_box_size );

		}	else	if(leaf_atom.is_type("stco"))	{

			size_t entry_count = ::stsc_get_entries( buffer );
			new_box_size = 12 + 4 + entry_count * 4;
			assert( new_box_size <= org_box_size );

		}	else	if(leaf_atom.is_type("co64"))	{
			throw std::exception( "64bit chunk offset not supported." );
		}

		size_t shrink_size = org_box_size - new_box_size;
		if( shrink_size > 0 )
		{
			decrease_box_size( leaf_atom.start_, atom::PREAMBLE_SIZE, shrink_size );

			// move memory
			byte * dest = leaf_atom.start_ + new_box_size;
			byte * src = leaf_atom.start_ + org_box_size;
			size_t remain_data_size = new_moov_buffer_.get() + new_moov_size_ - src;
			::memmove( dest, src, remain_data_size );

			new_moov_size_ -= shrink_size;
			stbl_shrink_size += shrink_size;
		}

		buffer = leaf_atom.read_header( leaf_atom.start_ );
		assert( leaf_atom.size_ == new_box_size );		
		buffer = leaf_atom.skip();
	}

	// fix boxes size
	decrease_box_size( track.mdia_.minf_.stbl_.start_ - atom::PREAMBLE_SIZE, atom::PREAMBLE_SIZE, stbl_shrink_size );
	decrease_box_size( track.mdia_.minf_.start_ - atom::PREAMBLE_SIZE, atom::PREAMBLE_SIZE, stbl_shrink_size );
	decrease_box_size( track.mdia_.start_ - atom::PREAMBLE_SIZE, atom::PREAMBLE_SIZE, stbl_shrink_size );
	decrease_box_size( track.start_ - atom::PREAMBLE_SIZE, atom::PREAMBLE_SIZE, stbl_shrink_size );

	return stbl_shrink_size;
}