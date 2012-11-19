#pragma once

#include "box.h"
#include "atoms.h"

class mp4_meta_loader
{
public:
	mp4_meta_loader(void);
	virtual ~mp4_meta_loader(void);

protected:
	void load_header( FILE * in ) throw( std::exception );
	void parse_header();

	void load_ftyp( FILE * in ) throw( std::exception );
	void load_moov( FILE * in ) throw( std::exception );
	void load_mdat( FILE * in )  throw( std::exception );

	size_t read_file( FILE * in, byte * buffer, size_t count ) throw( std::exception );

	box read_box_header( FILE * in ) throw( std::exception );

	void change_mdat_size( size_t size );

protected:
	scoped_array<byte>		ftyp_box_buffer_;
	size_t					ftyp_buffer_size_;

	scoped_array<byte>		moov_box_buffer_;
	size_t					moov_buffer_size_;

	scoped_array<byte>		mdat_header_buffer_;
	size_t					mdat_buffer_size_;

	scoped_ptr<moov>		moov_;

};
