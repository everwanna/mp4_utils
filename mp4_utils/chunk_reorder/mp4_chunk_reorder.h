#pragma once
#include "../mp4lib/mp4_meta_loader.h"

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

class mp4_chunk_reorder : public mp4_meta_loader
{
public:
	mp4_chunk_reorder( const wstring & in_file, const wstring & out_file );
	~mp4_chunk_reorder(void);

public:
	void reorder();

private:
	void load_header() throw( std::exception );

	void write_new_file();

	size_t write_file( FILE * out, byte * data, size_t count ) throw( std::exception );
	void close_file( FILE * & file );

	void write_mdat() throw( std::exception );
	void write_header() throw( std::exception );

	bool init_status();

	size_t write_chunk( track_status & track );
	size_t select_track();

private:
	// writing status
	FILE *						in_;
	FILE *						out_;
	size_t						chunk_offset_;
	size_t						last_track_index_;
	scoped_array<track_status>	tracks_;

private:
	wstring					in_file_;
	wstring					out_file_;

	const static size_t		BOX_HEADER_SIZE = 8;
	const static size_t		NO_TRACK_INDEX = static_cast<size_t>(-1);
};
