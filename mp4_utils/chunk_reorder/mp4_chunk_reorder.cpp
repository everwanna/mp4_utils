#include "StdAfx.h"
#include "mp4_chunk_reorder.h"
#include "../mp4lib/atoms.h"

mp4_chunk_reorder::mp4_chunk_reorder( const wstring & in_file, const wstring & out_file )
: mp4_meta_loader()
, in_file_(in_file)
, out_file_(out_file)

, chunk_offset_(0)
, tracks_()

, in_(NULL)
, out_(NULL)
{
}

mp4_chunk_reorder::~mp4_chunk_reorder(void)
{
	close_file( in_ );
	close_file( out_ );
}

void mp4_chunk_reorder::reorder()
{

	load_header();

	parse_header();

	write_new_file();
}

void mp4_chunk_reorder::load_header() throw( std::exception )
{
	// ifstream in( in_file_.c_str() );
#pragma warning(disable:4996)
	FILE * in = ::_wfopen( in_file_.c_str(), L"rb" );
#pragma warning(default:4996)

	if( in == NULL ) {
		throw std::exception( "cannot open file. " );
	}

	mp4_meta_loader::load_header( in );

	::fclose( in );
}

void mp4_chunk_reorder::write_new_file()
{
	if( ! init_status() )
	{
		cout << "unsupported file." << endl;
		return ;
	}

#pragma warning(disable:4996)
	in_ = ::_wfopen( in_file_.c_str(), L"rb" );
	out_ = ::_wfopen( out_file_.c_str(), L"wb" );
#pragma warning(default:4996)

	if( in_ == NULL ) {
		throw std::exception( "cannot open input file. " );
	}
	if( out_ == NULL ) {
		throw std::exception( "cannot open output file. " );
	}

	write_mdat();

	write_header();

	close_file( in_ );
	close_file( out_ );
}

void mp4_chunk_reorder::write_mdat() throw( std::exception )
{
	size_t data_start = ftyp_buffer_size_ + moov_buffer_size_ + mdat_buffer_size_;
	::fseek( out_, data_start, SEEK_SET );

	chunk_offset_ = data_start;

	size_t data_size = 0;

	bool done = false;
	while( ! done ) {
		size_t track_index = select_track();
		if ( track_index == NO_TRACK_INDEX )
		{
			done = true;
			break;
		}
		size_t length = write_chunk( tracks_[ track_index ] );

		chunk_offset_ += length;
		data_size += length;
	}

	change_mdat_size( data_size + atom::PREAMBLE_SIZE );
}

void mp4_chunk_reorder::write_header() throw( std::exception )
{
	::fseek( out_, 0, SEEK_SET );

	for( size_t i = 0; i != moov_->tracks_; ++i)
	{
		trak* trak_atom = &moov_->traks_[i];

		::trak_write_index( trak_atom, 0, trak_atom->samples_size_ );
	}
	
	write_file( out_, ftyp_box_buffer_.get(), ftyp_buffer_size_ );
	write_file( out_, moov_box_buffer_.get(), moov_buffer_size_ );
	write_file( out_, mdat_header_buffer_.get(), mdat_buffer_size_ );
}

size_t mp4_chunk_reorder::write_file( FILE * out, byte * data, size_t count ) throw( std::exception )
{
	size_t length = ::fwrite( data, 1, count, out );
	if ( length != count )
	{
		throw std::exception( "write file failed." );
	}

	return length;
}

bool mp4_chunk_reorder::init_status()
{
	if ( moov_->tracks_ != 2 )
	{
		cout << "only support 2 tracks mp4 file." << endl;
		return false;
	}

	tracks_.reset( new track_status[2]() );

	tracks_[0].trak_ = &moov_->traks_[0];
	tracks_[0].chunk_count_ = moov_->traks_[0].chunks_size_;

	tracks_[1].trak_ = &moov_->traks_[1];
	tracks_[1].chunk_count_ = moov_->traks_[1].chunks_size_;

	last_track_index_ = 0;

	return true;
}


size_t mp4_chunk_reorder::select_track()
{
	if ( tracks_[0].chunks_written() &&	tracks_[1].chunks_written() )
	{
		return NO_TRACK_INDEX;
	}

	if ( tracks_[0].chunks_written() )	// if track 0 is over
	{
		return 1;
	}

	if ( tracks_[1].chunks_written() )
	{
		return 0;
	}

	if ( tracks_[0].current_chunk_time_ > tracks_[1].current_chunk_time_ )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

size_t mp4_chunk_reorder::write_chunk( track_status & track )
{
	chunks_t & chunk = track.trak_->chunks_[track.chunk_index_];
	chunks_t & next_chunk = track.trak_->chunks_[track.chunk_index_ + 1];

	long trak_time_scale = mdhd_get_time_scale(track.trak_->mdia_.mdhd_);
	// read original data
	cout << "trak[" << trak_time_scale << "]"
		<< ", chunk: " << track.chunk_index_ << "/" << track.chunk_count_
		<< ", chunk start time: " << chunk.start_time_ 
		<< ", offset: " << chunk.pos_
		<< ", size: " << chunk.data_size_ << endl;

	scoped_array<byte> data( new byte[ chunk.data_size_ ] );
	::fseek( in_, chunk.pos_, SEEK_SET );
	read_file( in_, data.get(), chunk.data_size_ );

	// write data
	size_t length = write_file( out_, data.get(), chunk.data_size_ );

	chunk.pos_ = chunk_offset_;						// update offset
	cout << "\t new offset: " << chunk.pos_ << endl;

	track.current_chunk_time_ = (double)next_chunk.start_time_ * 1000 / trak_time_scale;		// update time

	++ track.chunk_index_;		// update index

	return length;
}

void mp4_chunk_reorder::close_file( FILE * & file )
{
	if ( file != NULL )
	{
		::fclose( file );
		file = NULL;
	}
}
