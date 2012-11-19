#include "StdAfx.h"
#include "mp4_splitter.h"

#include "log.h"

mp4_splitter::mp4_splitter( const wstring & input )
: in_file_(input)
, interval_(static_cast<size_t>(-1))	// default no split

, in_filename_()
, in_ext_()

, in_(NULL)

, audio_track_index_(0)
, video_track_index_(1)

, mvhd_duration_(0)
, mvhd_time_scale_(0)

, clips_()
{
	parse_input_filename();
}
mp4_splitter::~mp4_splitter(void)
{
}

void mp4_splitter::split( size_t ms )
{
	interval_ = ms;

	LOGW( L"file: " << this->in_file_ << L"interval: " << interval_);

	parse_input_file();

	calc_clips_info();

	split_to_files();
}

void mp4_splitter::parse_input_file() throw( std::exception )
{
	FILE * in = open_file( in_file_, L"rb" );

	load_header( in );

	close_file( in );

	parse_header();

	if( moov_->tracks_ != 2 )
	{
		throw std::exception( "only support two trak mp4 files." );
	}

	if( moov_->traks_[0].mdia_.minf_.stbl_.stss_ != NULL &&
		moov_->traks_[1].mdia_.minf_.stbl_.stss_ == NULL )
	{
		video_track_index_ = 0;
		audio_track_index_ = 1;
	}
	else if( moov_->traks_[0].mdia_.minf_.stbl_.stss_ == NULL &&
		moov_->traks_[1].mdia_.minf_.stbl_.stss_ != NULL )
	{
		video_track_index_ = 1;
		audio_track_index_ = 0;
	}
	else
	{
		throw std::exception( "unsupported mp4 file." );
	}

	mvhd_duration_ = ::mvhd_get_duration( moov_->mvhd_ );
	mvhd_time_scale_ = ::mvhd_get_time_scale( moov_->mvhd_ );

	cout << "duration: " << mvhd_duration_ * 1000 / mvhd_time_scale_ 
		<< ", audio samples: " << moov_->traks_[audio_track_index_].samples_size_
		<< ", video samples: " << moov_->traks_[video_track_index_].samples_size_
		<< endl;
}

void mp4_splitter::calc_clips_info() throw( std::exception )
{
	size_t original_audio_sample_count = moov_->traks_[audio_track_index_].samples_size_;
	size_t original_video_sample_count = moov_->traks_[video_track_index_].samples_size_;

	const double duration = static_cast<double>( interval_ );

	size_t file_index = 1;
	double audio_start_time = 0.0;
	double video_start_time = 0.0;
	size_t audio_sample_start = 1;
	size_t video_sample_start = 1;

	bool done = false;
	while( ! done )
	{
		clip_ptr clip = calc_clip_info( file_index, audio_start_time, video_start_time, duration, audio_sample_start, video_sample_start );
		clips_.push_back( clip );

		cout << "clip[" << clip->file_index_ << "], start time=" << clip->start_time_ << ", duration=" << clip->duration_ << endl;
		cout << "\t audio time=" << clip->audio_track_->start_time_ << "/" << clip->audio_track_->end_time_ 
			<< ", sample=" << clip->audio_track_->sample_start_ << "/" << clip->audio_track_->sample_end_ 
			<< "\t video time=" << clip->video_track_->start_time_ << "/" << clip->video_track_->end_time_ 
			<< ", sample=" << clip->video_track_->sample_start_ << "/" << clip->video_track_->sample_end_ 
			<< endl << endl;

		// update status
		++file_index;
		audio_start_time = clip->audio_track_->end_time_;
		audio_sample_start = clip->audio_track_->sample_end_;

		video_start_time = clip->video_track_->end_time_;
		video_sample_start = clip->video_track_->sample_end_;
		
		if( clip->audio_track_->sample_end_ >= original_audio_sample_count &&
			clip->video_track_->sample_end_ >= original_video_sample_count )
		{
			done = true;
		}
	}
}

clip_ptr mp4_splitter::calc_clip_info( size_t file_index, double audio_start_time, double video_start_time, double duration, size_t audio_start, size_t video_start ) throw( std::exception )
{
	clip_ptr clip( new clip_info( file_index, audio_start_time, video_start_time, duration, audio_start, video_start ) );

	// seek video first
	seek_trak( moov_->traks_[video_track_index_], clip->video_track_ );
	cout << "clip[" << clip->file_index_ << "], video end time=" << clip->video_track_->end_time_ << ", end sample=" << clip->video_track_->sample_end_ << endl;

	// sync audio to video
	clip->audio_track_->end_time_ = clip->video_track_->end_time_;

	// seek audio
	seek_trak( moov_->traks_[audio_track_index_], clip->audio_track_ );
	cout << "clip[" << clip->file_index_ << "], audio end time=" << clip->audio_track_->end_time_ << ", end sample=" << clip->audio_track_->sample_end_ << endl;

	return clip;
}

void mp4_splitter::seek_trak( trak & track, media_track_ptr track_info )
{
	struct stbl_t* stbl = &track.mdia_.minf_.stbl_;
	double end_time = track_info->end_time_;
	size_t track_time_scale = mdhd_get_time_scale( track.mdia_.mdhd_ );

	size_t sample_duration = get_sample_duration( &track );

	double track_time = get_samples_duration( track.samples_size_ /*- track_info->sample_start_ + 1*/, sample_duration, track_time_scale );

	size_t end_ticks = static_cast<size_t>( end_time * track_time_scale / 1000 );
	size_t end_sample = stts_get_sample(stbl->stts_, end_ticks);
	cout << "stts get end sample = " << end_sample << endl;

	end_sample = stbl_get_nearest_keyframe(stbl, end_sample);
	cout << "stss get end sample = " << end_sample << endl;

	end_time = get_samples_duration( end_sample - 1/* - track_info->sample_start_*/, sample_duration, track_time_scale );
	cout << "track time = " << track_time << ", end time = " << end_time;

	if( end_time + 1000 > track_time )
	{
		end_time = track_time;
		end_sample = track.samples_size_ + 1;
	}

	cout << ", actual end time = " << end_time << endl;

	track_info->end_time_ = end_time;
	track_info->sample_end_ = end_sample;
}


void mp4_splitter::split_to_files() throw( std::exception )
{
	in_ = open_file( in_file_, L"rb" );

	for( size_t i = 0; i < clips_.size(); ++i )
	{
		clip_ptr clip = clips_[i];

		clip->build_new_moov( moov_box_buffer_.get(), moov_buffer_size_ );

		write_clip_file( clip );
	}
	
	close_file( in_ );
}

void mp4_splitter::write_clip_file( clip_ptr clip )
{
	FILE * out = open_file( get_output_file( clip->file_index_ ), L"wb" );

	size_t data_size = write_chunks( out, clip );

	change_mdat_size( clip->mdat_header_buffer_.get(), clip->mdat_buffer_size_, data_size + clip->mdat_buffer_size_ );
	write_header( out, clip );

	close_file( out );
}

size_t mp4_splitter::write_chunks( FILE * out, clip_ptr clip )
{
	size_t data_start = ftyp_buffer_size_ + clip->new_moov_size_ + mdat_buffer_size_;
	::fseek( out, data_start, SEEK_SET );

	size_t chunk_offset = data_start;
	size_t data_size = 0;

	bool done = false;
	while( ! done ) {
		media_track_ptr track = select_track(clip);
		if( ! track )
		{
			done = true;
			break;
		}

		size_t length = write_chunk( out, track->status_, chunk_offset );

		chunk_offset += length;
		data_size += length;
	}

	return data_size;
}

void mp4_splitter::write_header( FILE * out, clip_ptr clip )
{
	clip->update_chunk_offset();

	::fseek( out, 0, SEEK_SET );

	write_file( out, ftyp_box_buffer_.get(), ftyp_buffer_size_ );
	write_file( out, clip->new_moov_buffer_.get(), clip->new_moov_size_ );
	write_file( out, clip->mdat_header_buffer_.get(), clip->mdat_buffer_size_ );
}

size_t mp4_splitter::write_file( FILE * out, byte * data, size_t count ) throw( std::exception )
{
	size_t length = ::fwrite( data, 1, count, out );
	if ( length != count )
	{
		throw io_exception( "write file failed." );
	}

	return length;
}
void mp4_splitter::close_file( FILE * & file )
{
	if ( file != NULL )
	{
		::fclose( file );
		file = NULL;
	}
}
size_t mp4_splitter::write_chunk( FILE * out, track_status &track, size_t chunk_offset)
{
	chunks_t &chunk = track.trak_->chunks_[track.chunk_index_];
	chunks_t &next_chunk = track.trak_->chunks_[track.chunk_index_ + 1];
	long trak_time_scale = mdhd_get_time_scale(track.trak_->mdia_.mdhd_);
	// read original data
//	cout << "trak[" << trak_time_scale << "]" << ", chunk: " << track.chunk_index_ << "/" << track.chunk_count_ << ", chunk start time: " << chunk.start_time_ << ", offset: " << chunk.pos_ << ", size: " << chunk.data_size_ << endl;
	scoped_array<byte> data(new byte[chunk.data_size_]);
	::fseek(in_, chunk.pos_, SEEK_SET);
	read_file(in_, data.get(), chunk.data_size_);
	// write data
	size_t length = write_file(out, data.get(), chunk.data_size_);
	chunk.pos_ = chunk_offset;
	// update offset
//	cout << "\t new offset: " << chunk.pos_ << endl;
	track.current_chunk_time_ = (double)next_chunk.start_time_ * 1000 / trak_time_scale;
	// update time
	++track.chunk_index_;
	// update index
	return length;
}

media_track_ptr mp4_splitter::select_track(clip_ptr clip) const
{
	if (clip->audio_track_->status_.chunks_written() && clip->video_track_->status_.chunks_written())
	{
		return media_track_ptr();
	}
	
	if (clip->audio_track_->status_.chunks_written() )// if track 0 is over
	{
		return clip->video_track_;
	}

	if (clip->video_track_->status_.chunks_written())
	{
		return clip->audio_track_;
	}

	if (clip->audio_track_->status_.current_chunk_time_ > clip->video_track_->status_.current_chunk_time_)
	{
		return clip->video_track_;
	}

	return clip->audio_track_;
}

double mp4_splitter::get_samples_duration( size_t count, size_t duration, size_t time_scale )
{
	return (double)count * duration * 1000 / time_scale;
}


