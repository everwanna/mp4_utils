#pragma once

#include "atom.h"

struct stts_table_t
{
	uint32_t sample_count_;
	uint32_t sample_duration_;
};

struct ctts_table_t
{
	uint32_t sample_count_;
	uint32_t sample_offset_;
};

struct stsc_table_t
{
	uint32_t chunk_;
	uint32_t samples_;
	uint32_t id_;
};

struct stbl_t
{
	unsigned char* start_;
	//stsd stsd_;               // sample description
	unsigned char* stts_;     // decoding time-to-sample
	unsigned char* stss_;     // sync sample
	unsigned char* stsc_;     // sample-to-chunk
	unsigned char* stsz_;     // sample size
	unsigned char* stco_;     // chunk offset
	unsigned char* ctts_;     // composition time-to-sample
};


struct minf_t
{
	unsigned char* start_;
	struct stbl_t stbl_;
};

struct mdia_t
{
	unsigned char* start_;
	unsigned char* mdhd_;
	struct minf_t minf_;
	//  hdlr hdlr_;
};

struct chunks_t
{
public:
	chunks_t()
		: sample_(0)
		, size_(0)
		, id_(0)
		, pos_(0)
		, start_time_(0)
		, data_size_(0)
	{}

	unsigned int sample_;   // number of the first sample in the chunk
	unsigned int size_;     // number of samples in the chunk
	int id_;                // for multiple codecs mode - not used
	unsigned int pos_;      // start byte position of chunk

	size_t		 start_time_;	// chunk start time, i.e. first sample start time
	size_t		 data_size_;		// chunk data size, i.e. the total data size of samples in chunk
};

struct samples_t
{
	unsigned int pts_;      // decoding/presentation time
	unsigned int size_;     // size in bytes
	unsigned int pos_;      // byte offset
	unsigned int cto_;      // composition time offset
};
