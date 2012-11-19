#pragma once

#include "../mp4lib/mp4_meta_loader.h"

#include "clip_info.h"
#include "io_exception.h"

class mp4_splitter : public mp4_meta_loader
{
public:
	mp4_splitter( const wstring & input );
	~mp4_splitter(void);

public:
	void split( size_t ms ) throw( std::exception );

private:
	void parse_input_file() throw( std::exception );

	void calc_clips_info() throw( std::exception );

	void split_to_files() throw( std::exception );

	clip_ptr calc_clip_info( size_t file_index, double audio_start_time, double video_start_time, double duration, size_t audio_start, size_t video_start ) throw( std::exception );

	void seek_trak( trak & track, media_track_ptr track_info );

	void write_clip_file( clip_ptr clip );

	double get_samples_duration( size_t count, size_t duration, size_t time_scale );

	size_t write_chunks( FILE * out, clip_ptr clip );

	size_t write_chunk( FILE * out, track_status &track, size_t chunk_offset);

	void write_header( FILE * out, clip_ptr clip );

	void change_mdat_size( byte * mdat_buffer, size_t buffer_size, size_t size ) {
		assert( buffer_size >= 8 );
		buffer_io::write_int32( mdat_buffer, static_cast<uint32_t>( size ) );
	}
	media_track_ptr select_track(clip_ptr clip) const;
	size_t write_chunk(track_status &track, size_t chunk_offset);

	void parse_input_filename() {
		wpath p( in_file_ );

		in_filename_ = p.stem();
		in_ext_ = p.extension();
	}

	wstring get_output_file( size_t index ) {
		wostringstream oss;
		oss << in_filename_ << "-" << index << in_ext_;
		return oss.str();
	}
/*
	void open_files( const wstring & out_file ) {
#pragma warning(disable:4996)
		in_ = ::_wfopen( in_file_.c_str(), L"rb" );
		FILE * out_ = ::_wfopen( out_file.c_str(), L"wb" );
#pragma warning(default:4996)
		if( in_ == NULL ) {
			throw std::exception( "cannot open input file. " );
		}
		if( out_ == NULL ) {
			throw std::exception( "cannot open output file. " );
		}
	}
*/

private:
	// file operations
	FILE * open_file( const wstring & filename, const wstring & op ) throw( io_exception )
	{
#pragma warning(disable:4996)
		FILE * f = ::_wfopen( filename.c_str(), op.c_str() );
#pragma warning(default:4996)

		if( f == NULL ) {
			throw io_exception( "cannot open the file. " );
		}

		return f;
	}

	size_t write_file( FILE * out, byte * data, size_t count ) throw( io_exception );

	void close_file( FILE * & file );

private:
	wstring				in_file_;
	size_t				interval_;

	wstring				in_filename_;	// file name without ext
	wstring				in_ext_;

private:
	FILE *				in_;

	size_t				audio_track_index_;
	size_t				video_track_index_;

	size_t				mvhd_duration_;
	size_t				mvhd_time_scale_;

private:
	// status
	vector<clip_ptr>	clips_;

	const static size_t	NO_TRACK_INDEX = static_cast<size_t>(-1);
};
