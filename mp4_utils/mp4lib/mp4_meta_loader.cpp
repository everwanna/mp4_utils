#include "StdAfx.h"
#include "mp4_meta_loader.h"

mp4_meta_loader::mp4_meta_loader(void)
: ftyp_box_buffer_()
, ftyp_buffer_size_(0)

, moov_box_buffer_()
, moov_buffer_size_(0)

, mdat_header_buffer_()
, mdat_buffer_size_(0)

, moov_()
{
}

mp4_meta_loader::~mp4_meta_loader(void)
{
}

void mp4_meta_loader::load_header( FILE * in ) throw( std::exception )
{
	load_ftyp( in );

	load_moov( in );

	load_mdat( in );
}

void mp4_meta_loader::load_ftyp( FILE * in ) throw( std::exception )
{
	box box_header = read_box_header( in );
	if ( box_header.get_type() != "ftyp" )
	{
		throw std::exception( "the first box is not ftyp. unsupported file." );
	}

	ftyp_buffer_size_ = box_header.size_;
	ftyp_box_buffer_.reset( new byte[ftyp_buffer_size_] );
//	assert( ftyp_buffer_size_ == 32 );

	::fseek( in, box_header.start_, SEEK_SET );
	read_file( in, ftyp_box_buffer_.get(), ftyp_buffer_size_ );
}

void mp4_meta_loader::load_moov( FILE * in ) throw( std::exception )
{
	box box_header = read_box_header( in );
	if ( box_header.get_type() != "moov" )
	{
		throw std::exception( "the second box is not moov. unsupported file." );
	}

	moov_buffer_size_ = box_header.size_;
	moov_box_buffer_.reset( new byte[moov_buffer_size_] );

	::fseek( in, box_header.start_, SEEK_SET );
	read_file( in, moov_box_buffer_.get(), moov_buffer_size_ );
}

void mp4_meta_loader::load_mdat( FILE * in ) throw( std::exception )
{
	box box_header = read_box_header( in );
	if ( box_header.get_type() != "mdat" )
	{
		throw std::exception( "the third box is not mdat. unsupported file." );
	}

	mdat_buffer_size_ = 8;
	mdat_header_buffer_.reset( new byte[mdat_buffer_size_] );
	::fseek( in, box_header.start_, SEEK_SET );
	read_file( in, mdat_header_buffer_.get(), mdat_buffer_size_ );
}


box mp4_meta_loader::read_box_header( FILE * in ) throw( std::exception )
{
	box box;
	
	byte buffer[8];
	box.start_ = ::ftell( in );

	read_file( in, buffer, sizeof(buffer) );
	
	::memcpy( &box.type_, buffer + 4, 4 );	// copy type
	box.size_ = buffer_io::read_int32( buffer ); // ::ntohl( *( reinterpret_cast<uint32_t*>(buffer) ) );
	box.end_ = box.start_ + box.size_;

	return box;
}

size_t mp4_meta_loader::read_file( FILE * in, byte * buffer, size_t count ) throw( std::exception )
{
	size_t length = ::fread( buffer, 1, count, in );
	if ( length != count )
	{
		throw std::exception( "read failed." );
	}

	return length;
}

void mp4_meta_loader::change_mdat_size( size_t size )
{
	buffer_io::write_int32( mdat_header_buffer_.get(), static_cast<int32_t>(size) );
}

void mp4_meta_loader::parse_header()
{
	moov_.reset( new moov() );

	moov_->parse( moov_box_buffer_.get() + atom::PREAMBLE_SIZE, moov_buffer_size_ - atom::PREAMBLE_SIZE );
}