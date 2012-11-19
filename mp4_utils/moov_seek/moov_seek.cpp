// moov_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <boost/cstdint.hpp>
using namespace boost;
//#include "inttypes.h"

#ifdef _DEBUG
#else
#define atome_print(x)
#endif // _DEBUG

static int read_char(unsigned char const* buffer)
{
	return buffer[0];
}

static int read_int32(void const* buffer)
{
	unsigned char* p = (unsigned char*)buffer;
	return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
}

static void write_int32(void* outbuffer, uint32_t value)
{
	unsigned char* p = (unsigned char*)outbuffer;
	p[0] = (unsigned char)((value >> 24) & 0xff);
	p[1] = (unsigned char)((value >> 16) & 0xff);
	p[2] = (unsigned char)((value >> 8) & 0xff);
	p[3] = (unsigned char)((value >> 0) & 0xff);
}


struct atom_t
{
	unsigned char type_[4];
	unsigned int size_;
	unsigned char* start_;
	unsigned char* end_;

	atom_t() : size_(0), start_(0), end_(0)
	{
		memset( type_, 0, sizeof(type_) );
	}
};

#define ATOM_PREAMBLE_SIZE 8

static unsigned int atom_header_size(unsigned char* atom_bytes)
{
	return (atom_bytes[0] << 24) +
		(atom_bytes[1] << 16) +
		(atom_bytes[2] << 8) +
		(atom_bytes[3]);
}

static unsigned char* atom_read_header(unsigned char* buffer, struct atom_t* atom)
{
	atom->start_ = buffer;
	memcpy(&atom->type_[0], &buffer[4], 4);
	atom->size_ = atom_header_size(buffer);
	atom->end_ = atom->start_ + atom->size_;

	return buffer + ATOM_PREAMBLE_SIZE;
}

static unsigned char* atom_skip(struct atom_t const* atom)
{
	return atom->end_;
}

static int atom_is(struct atom_t const* atom, const char* type)
{
	return (atom->type_[0] == type[0] &&
		atom->type_[1] == type[1] &&
		atom->type_[2] == type[2] &&
		atom->type_[3] == type[3])
		;
}

static void atom_print(struct atom_t const* atom)
{
	printf("Atom(%c%c%c%c,%d)\n", atom->type_[0], atom->type_[1],
		atom->type_[2], atom->type_[3], atom->size_);
}

#define MAX_TRACKS 8

unsigned int stts_get_entries(unsigned char const* stts)
{
	return read_int32(stts + 4);
}

void stts_get_sample_count_and_duration(unsigned char const* stts,
										unsigned int idx, unsigned int* sample_count, unsigned int* sample_duration)
{
	unsigned char const* table = stts + 8 + idx * 8;
	*sample_count = read_int32(table);
	*sample_duration = read_int32(table + 4);
}

struct stts_table_t
{
	uint32_t sample_count_;
	uint32_t sample_duration_;
};

unsigned int ctts_get_entries(unsigned char const* ctts)
{
	return read_int32(ctts + 4);
}

void ctts_get_sample_count_and_offset(unsigned char const* ctts,
									  unsigned int idx, unsigned int* sample_count, unsigned int* sample_offset)
{
	unsigned char const* table = ctts + 8 + idx * 8;
	*sample_count = read_int32(table);
	*sample_offset = read_int32(table + 4);
}

unsigned int ctts_get_samples(unsigned char const* ctts)
{
	long samples = 0;
	long entries = ctts_get_entries(ctts);
	int i;
	for(i = 0; i != entries; ++i)
	{
		unsigned int sample_count;
		unsigned int sample_offset;
		ctts_get_sample_count_and_offset(ctts, i, &sample_count, &sample_offset);
		samples += sample_count;
	}

	return samples;
}

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

unsigned int stsc_get_entries(unsigned char const* stsc)
{
	return read_int32(stsc + 4);
}

void stsc_get_table(unsigned char const* stsc, unsigned int i, struct stsc_table_t *stsc_table)
{
	struct stsc_table_t* table = (struct stsc_table_t*)(stsc + 8);
	stsc_table->chunk_ = read_int32(&table[i].chunk_) - 1;
	stsc_table->samples_ = read_int32(&table[i].samples_);
	stsc_table->id_ = read_int32(&table[i].id_);
}

unsigned int stsc_get_chunk(unsigned char* stsc, unsigned int sample)
{
	unsigned int entries = read_int32(stsc + 4);
	struct stsc_table_t* table = (struct stsc_table_t*)(stsc + 8);

	if(entries == 0)
	{
		return 0;
	}
	else
		//  if(entries == 1)
		//  {
		//    unsigned int table_samples = read_int32(&table[0].samples_);
		//    unsigned int chunk = (sample + 1) / table_samples;
		//    return chunk - 1;
		//  }
		//  else
	{
		unsigned int total = 0;
		unsigned int chunk1 = 1;
		unsigned int chunk1samples = 0;
		unsigned int chunk2entry = 0;
		unsigned int chunk, chunk_sample;

		do
		{
			unsigned int range_samples;
			unsigned int chunk2 = read_int32(&table[chunk2entry].chunk_);
			chunk = chunk2 - chunk1;
			range_samples = chunk * chunk1samples;

			if(sample < total + range_samples)
				break;

			chunk1samples = read_int32(&table[chunk2entry].samples_);
			chunk1 = chunk2;

			if(chunk2entry < entries)
			{
				chunk2entry++;
				total += range_samples;
			}
		} while(chunk2entry < entries);

		if(chunk1samples)
		{
			unsigned int sample_in_chunk = (sample - total) % chunk1samples;
			if(sample_in_chunk != 0)
			{
				printf("ERROR: sample must be chunk aligned: %d\n", sample_in_chunk);
			}
			chunk = (sample - total) / chunk1samples + chunk1;
		}
		else
			chunk = 1;

		chunk_sample = total + (chunk - chunk1) * chunk1samples;

		return chunk;
	}
}

unsigned int stsc_get_samples(unsigned char* stsc)
{
	unsigned int entries = read_int32(stsc + 4);
	struct stsc_table_t* table = (struct stsc_table_t*)(stsc + 8);
	unsigned int samples = 0;
	unsigned int i;
	for(i = 0; i != entries; ++i)
	{
		samples += read_int32(&table[i].samples_);
	}
	return samples;
}

unsigned int stco_get_entries(unsigned char const* stco)
{
	return read_int32(stco + 4);
}

unsigned int stco_get_offset(unsigned char const* stco, int idx)
{
	uint32_t const* table = (uint32_t const*)(stco + 8);
	return read_int32(&table[idx]);
}

unsigned int stsz_get_sample_size(unsigned char const* stsz)
{
	return read_int32(stsz + 4);
}

unsigned int stsz_get_entries(unsigned char const* stsz)
{
	return read_int32(stsz + 8);
}

unsigned int stsz_get_size(unsigned char const* stsz, unsigned int idx)
{
	uint32_t const* table = (uint32_t const*)(stsz + 12);
	return read_int32(&table[idx]);
}

unsigned int stts_get_duration(unsigned char const* stts)
{
	long duration = 0;
	long entries = stts_get_entries(stts);
	int i;
	for(i = 0; i != entries; ++i)
	{
		unsigned int sample_count;
		unsigned int sample_duration;
		stts_get_sample_count_and_duration(stts, i,
			&sample_count, &sample_duration);
		duration += sample_duration * sample_count;
	}

	return duration;
}

unsigned int stts_get_samples(unsigned char const* stts)
{
	long samples = 0;
	long entries = stts_get_entries(stts);
	int i;
	for(i = 0; i != entries; ++i)
	{
		unsigned int sample_count;
		unsigned int sample_duration;
		stts_get_sample_count_and_duration(stts, i,
			&sample_count, &sample_duration);
		samples += sample_count;
	}

	return samples;
}

unsigned int stts_get_sample(unsigned char const* stts, unsigned int time)
{
	unsigned int stts_index = 0;
	unsigned int stts_count;

	unsigned int ret = 0;
	unsigned int time_count = 0;

	unsigned int entries = stts_get_entries(stts);
	for(; stts_index != entries; ++stts_index)
	{
		unsigned int sample_count;
		unsigned int sample_duration;
		stts_get_sample_count_and_duration(stts, stts_index,
			&sample_count, &sample_duration);
		if(time_count + sample_duration * sample_count >= time)
		{
			stts_count = (time - time_count) / sample_duration;
			time_count += stts_count * sample_duration;
			ret += stts_count;
			break;
		}
		else
		{
			time_count += sample_duration * sample_count;
			ret += sample_count;
			//      stts_index++;
		}
		//    if(stts_index >= table_.size())
		//      break;
	}
	//  *time = time_count;
	return ret;
}

unsigned int stts_get_time(unsigned char const* stts, unsigned int sample)
{
	unsigned int ret = 0;
	unsigned int stts_index = 0;
	unsigned int sample_count = 0;

	for(;;)
	{
		unsigned int table_sample_count;
		unsigned int table_sample_duration;
		stts_get_sample_count_and_duration(stts, stts_index,
			&table_sample_count, &table_sample_duration);

		if(sample_count + table_sample_count > sample)
		{
			unsigned int stts_count = (sample - sample_count);
			ret += stts_count * table_sample_duration;
			break;
		}
		else
		{
			sample_count += table_sample_count;
			ret += table_sample_count * table_sample_duration;
			stts_index++;
		}
	}
	return ret;
}


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

void stbl_parse(struct stbl_t* stbl, unsigned char* buffer, unsigned int size)
{
	struct atom_t leaf_atom;
	unsigned char* buffer_start = buffer;
	stbl->stss_ = 0;
	stbl->ctts_ = 0;

	stbl->start_ = buffer;

	while(buffer < buffer_start + size)
	{
		buffer = atom_read_header(buffer, &leaf_atom);

		atom_print(&leaf_atom);

		if(atom_is(&leaf_atom, "stts"))
		{
			stbl->stts_ = buffer;
		}
		else
			if(atom_is(&leaf_atom, "stss"))
			{
				stbl->stss_ = buffer;
			}
			else
				if(atom_is(&leaf_atom, "stsc"))
				{
					stbl->stsc_ = buffer;
				}
				else
					if(atom_is(&leaf_atom, "stsz"))
					{
						stbl->stsz_ = buffer;
					}
					else
						if(atom_is(&leaf_atom, "stco"))
						{
							stbl->stco_ = buffer;
						}
						else
							if(atom_is(&leaf_atom, "co64"))
							{
								perror("TODO: co64");
							}
							else
								if(atom_is(&leaf_atom, "ctts"))
								{
									stbl->ctts_ = buffer;
								}

								buffer = atom_skip(&leaf_atom);
	}
}

struct minf_t
{
	unsigned char* start_;
	struct stbl_t stbl_;
};

void minf_parse(struct minf_t* minf, unsigned char* buffer, unsigned int size)
{
	struct atom_t leaf_atom;
	unsigned char* buffer_start = buffer;

	minf->start_ = buffer;

	while(buffer < buffer_start + size)
	{
		buffer = atom_read_header(buffer, &leaf_atom);

		atom_print(&leaf_atom);

		if(atom_is(&leaf_atom, "stbl"))
		{
			stbl_parse(&minf->stbl_, buffer, leaf_atom.size_ - ATOM_PREAMBLE_SIZE);
		}

		buffer = atom_skip(&leaf_atom);
	}
}

struct mdia_t
{
	unsigned char* start_;
	unsigned char* mdhd_;
	struct minf_t minf_;
	//  hdlr hdlr_;
};

void mdia_parse(struct mdia_t* mdia, unsigned char* buffer, unsigned int size)
{
	struct atom_t leaf_atom;
	unsigned char* buffer_start = buffer;

	mdia->start_ = buffer;

	while(buffer < buffer_start + size)
	{
		buffer = atom_read_header(buffer, &leaf_atom);

		atom_print(&leaf_atom);

		if(atom_is(&leaf_atom, "mdhd"))
		{
			mdia->mdhd_ = buffer;
		}
		else
			if(atom_is(&leaf_atom, "minf"))
			{
				minf_parse(&mdia->minf_, buffer, leaf_atom.size_ - ATOM_PREAMBLE_SIZE);
			}

			buffer = atom_skip(&leaf_atom);
	}
}

struct chunks_t
{
	unsigned int sample_;   // number of the first sample in the chunk
	unsigned int size_;     // number of samples in the chunk
	int id_;                // for multiple codecs mode - not used
	unsigned int pos_;      // start byte position of chunk
};

struct samples_t
{
	unsigned int pts_;      // decoding/presentation time
	unsigned int size_;     // size in bytes
	unsigned int pos_;      // byte offset
	unsigned int cto_;      // composition time offset
};

struct trak_t
{
	unsigned char* start_;
	unsigned char* tkhd_;
	struct mdia_t mdia_;

	/* temporary indices */
	unsigned int chunks_size_;
	struct chunks_t* chunks_;

	unsigned int samples_size_;
	struct samples_t* samples_;
};

void trak_init(struct trak_t* trak)
{
	trak->chunks_ = 0;
	trak->samples_ = 0;
}

void trak_exit(struct trak_t* trak)
{
	if(trak->chunks_)
		free(trak->chunks_);
	if(trak->samples_)
		free(trak->samples_);
}

void trak_parse(struct trak_t* trak, unsigned char* buffer, unsigned int size)
{
	struct atom_t leaf_atom;
	unsigned char* buffer_start = buffer;

	trak->start_ = buffer;

	while(buffer < buffer_start + size)
	{
		buffer = atom_read_header(buffer, &leaf_atom);

		atom_print(&leaf_atom);

		if(atom_is(&leaf_atom, "tkhd"))
		{
			trak->tkhd_ = buffer;
		}
		else
			if(atom_is(&leaf_atom, "mdia"))
			{
				mdia_parse(&trak->mdia_, buffer, leaf_atom.size_ - ATOM_PREAMBLE_SIZE);
			}

			buffer = atom_skip(&leaf_atom);
	}
}

struct moov_t
{
	unsigned char* start_;
	unsigned int tracks_;
	unsigned char* mvhd_;
	struct trak_t traks_[MAX_TRACKS];
};

void moov_init(struct moov_t* moov)
{
	moov->tracks_ = 0;
}

void moov_exit(struct moov_t* moov)
{
	unsigned int i;
	for(i = 0; i != moov->tracks_; ++i)
	{
		trak_exit(&moov->traks_[i]);
	}
}

void init_chunks( struct trak_t* trak, void const* stco )
{
	{
		unsigned int i;
		for(i = 0; i != trak->chunks_size_; ++i)
		{
			trak->chunks_[i].pos_ = stco_get_offset((const unsigned char *)stco, i);
		}
	}
}

void process_chunkmap( struct trak_t* trak )
{
	{
		void const* stsc = trak->mdia_.minf_.stbl_.stsc_;
		unsigned int last = trak->chunks_size_;
		unsigned int i = stsc_get_entries((const unsigned char *)stsc);
		while(i > 0)
		{
			struct stsc_table_t stsc_table;
			unsigned int j;

			--i;

			stsc_get_table((const unsigned char *)stsc, i, &stsc_table);
			for(j = stsc_table.chunk_; j < last; j++)
			{
				trak->chunks_[j].id_ = stsc_table.id_;
				trak->chunks_[j].size_ = stsc_table.samples_;
			}
			last = stsc_table.chunk_;
		}
	}
}

void calc_chunk_pts( struct trak_t* trak )
{
	{
		void const* stsz = trak->mdia_.minf_.stbl_.stsz_;
		unsigned int sample_size = stsz_get_sample_size((const unsigned char *)stsz);
		unsigned int s = 0;
		{
			unsigned int j;
			for(j = 0; j < trak->chunks_size_; j++)
			{
				trak->chunks_[j].sample_ = s;
				s += trak->chunks_[j].size_;
			}
		}

		if(sample_size == 0)
		{
			trak->samples_size_ = stsz_get_entries((const unsigned char *)stsz);
		}
		else
		{
			trak->samples_size_ = s;
		}

		trak->samples_ = (samples_t*)malloc(trak->samples_size_ * sizeof(struct samples_t));
		//		cout << "\tsample count: " << trak->samples_size_ << endl;

		int total_sample_size = 0;
		if(sample_size == 0)
		{
			unsigned int i;
			for(i = 0; i != trak->samples_size_ ; ++i) {
				trak->samples_[i].size_ = stsz_get_size((const unsigned char *)stsz, i);
				total_sample_size += trak->samples_[i].size_;
			}
		}
		else
		{
			unsigned int i;
			for(i = 0; i != trak->samples_size_ ; ++i) {
				trak->samples_[i].size_ = sample_size;
				total_sample_size += trak->samples_[i].size_;
			}
		}
		//		cout << "\ttotal sample size: " << total_sample_size << endl;
	}
}

void calc_pts( struct trak_t* trak )
{
	{
		void const* stts = trak->mdia_.minf_.stbl_.stts_;
		unsigned int s = 0;
		unsigned int entries = stts_get_entries((const unsigned char *)stts);
		unsigned int j;
		for(j = 0; j < entries; j++)
		{
			unsigned int i;
			unsigned int pts = 0;
			unsigned int sample_count;
			unsigned int sample_duration;
			stts_get_sample_count_and_duration((const unsigned char *)stts, j,
				&sample_count, &sample_duration);
			for(i = 0; i < sample_count; i++)
			{
				trak->samples_[s].pts_ = pts;
				++s;
				pts += sample_duration;
			}
		}
		//		cout << "\ttotal samples in stts: " << s << endl;
	}
}

void calc_composition_times( struct trak_t* trak )
{
	{
		void const* ctts = trak->mdia_.minf_.stbl_.ctts_;
		if(ctts)
		{
			unsigned int s = 0;
			unsigned int entries = ctts_get_entries((const unsigned char *)ctts);
			unsigned int j;
			for(j = 0; j < entries; j++)
			{
				unsigned int i;
				unsigned int sample_count;
				unsigned int sample_offset;
				ctts_get_sample_count_and_offset((const unsigned char *)ctts, j, &sample_count, &sample_offset);

				for(i = 0; i < sample_count; i++)
				{
					if ( s < trak->samples_size_ )
					{
						trak->samples_[s].cto_ = sample_offset;
					}
					else
					{
						//	cout << "entry index: " << j << ", sample index: " << s << endl;
					}
					++s;
				}
			}
			// cout << "\ttotal samples in ctts: " << s << endl;
		}
	}
}

void calc_sample_offsets( struct trak_t* trak )
{
	{
		unsigned int s = 0;
		unsigned int j;
		for(j = 0; j < trak->chunks_size_; j++)
		{
			unsigned int pos = trak->chunks_[j].pos_;
			unsigned int i;
			for(i = 0; i < trak->chunks_[j].size_; i++)
			{
				trak->samples_[s].pos_ = pos;
				pos += trak->samples_[s].size_;
				++s;
			}
		}
	}
}
void trak_build_index(struct trak_t* trak)
{
	void const* stco = trak->mdia_.minf_.stbl_.stco_;

	trak->chunks_size_ = stco_get_entries( (const unsigned char *)(stco) );
	trak->chunks_ = (chunks_t*)malloc(trak->chunks_size_ * sizeof(struct chunks_t));

	init_chunks(trak, stco);

	// process chunkmap:
	process_chunkmap(trak);

	// calc pts of chunks:
	calc_chunk_pts(trak);

	//  i = 0;
	//  for (j = 0; j < trak->durmap_size; j++)
	//    i += trak->durmap[j].num;
	//  if (i != s) {
	//    mp_msg(MSGT_DEMUX, MSGL_WARN,
	//           "MOV: durmap and chunkmap sample count differ (%i vs %i)\n", i, s);
	//    if (i > s) s = i;
	//  }

	// calc pts:
	calc_pts(trak);

	// calc composition times:
	calc_composition_times(trak);

	// calc sample offsets
	calc_sample_offsets(trak);
}

void trak_write_index(struct trak_t* trak, unsigned int start, unsigned int end)
{
	// write samples [start,end>

	// stts = [entries * [sample_count, sample_duration]
	{
		unsigned char* stts = trak->mdia_.minf_.stbl_.stts_;
		unsigned int entries = 0;
		struct stts_table_t* table = (struct stts_table_t*)(stts + 8);
		unsigned int s;
		for(s = start; s != end; ++s)
		{
			unsigned int sample_count = 1;
			unsigned int sample_duration =
				trak->samples_[s + 1].pts_ - trak->samples_[s].pts_;
			while(s != end - 1)
			{
				if((trak->samples_[s + 1].pts_ - trak->samples_[s].pts_) != sample_duration)
					break;
				++sample_count;
				++s;
			}
			// write entry
			write_int32(&table[entries].sample_count_, sample_count);
			write_int32(&table[entries].sample_duration_, sample_duration);
			++entries;
		}
		write_int32(stts + 4, entries);
		if(stts_get_samples(stts) != end - start)
		{
			printf("ERROR: stts_get_samples=%d, should be %d\n",
				stts_get_samples(stts), end - start);
		}
	}

	// ctts = [entries * [sample_count, sample_offset]
	{
		unsigned char* ctts = trak->mdia_.minf_.stbl_.ctts_;
		if(ctts)
		{
			unsigned int entries = 0;
			struct ctts_table_t* table = (struct ctts_table_t*)(ctts + 8);
			unsigned int s;
			for(s = start; s != end; ++s)
			{
				unsigned int sample_count = 1;
				unsigned int sample_offset = trak->samples_[s].cto_;
				while(s != end - 1)
				{
					if(trak->samples_[s + 1].cto_ != sample_offset)
						break;
					++sample_count;
					++s;
				}
				// write entry
				write_int32(&table[entries].sample_count_, sample_count);
				write_int32(&table[entries].sample_offset_, sample_offset);
				++entries;
			}
			write_int32(ctts + 4, entries);
			if(ctts_get_samples(ctts) != end - start)
			{
				printf("ERROR: ctts_get_samples=%d, should be %d\n",
					ctts_get_samples(ctts), end - start);
			}
		}
	}

	// process chunkmap:
	{
		unsigned char* stsc = trak->mdia_.minf_.stbl_.stsc_;
		struct stsc_table_t* stsc_table = (struct stsc_table_t*)(stsc + 8);
		unsigned int i;
		for(i = 0; i != trak->chunks_size_; ++i)
		{
			if(trak->chunks_[i].sample_ + trak->chunks_[i].size_ > start)
				break;
		}

		{
			unsigned int stsc_entries = 0;
			unsigned int chunk_start = i;
			unsigned int samples =
				trak->chunks_[i].sample_ + trak->chunks_[i].size_ - start;
			unsigned int id = trak->chunks_[i].id_;

			// write entry [chunk,samples,id]
			write_int32(&stsc_table[stsc_entries].chunk_, 1);
			write_int32(&stsc_table[stsc_entries].samples_, samples);
			write_int32(&stsc_table[stsc_entries].id_, id);
			++stsc_entries;
			if(i != trak->chunks_size_)
			{
				for(i += 1; i != trak->chunks_size_; ++i)
				{
					if(trak->chunks_[i].size_ != samples)
					{
						samples = trak->chunks_[i].size_;
						id = trak->chunks_[i].id_;
						write_int32(&stsc_table[stsc_entries].chunk_, i - chunk_start + 1);
						write_int32(&stsc_table[stsc_entries].samples_, samples);
						write_int32(&stsc_table[stsc_entries].id_, id);
						++stsc_entries;
					}
				}
			}
			write_int32(stsc + 4, stsc_entries);
			{
				unsigned char* stco = trak->mdia_.minf_.stbl_.stco_;
				//        stco_erase(stco, chunk_start);
				unsigned int entries = read_int32(stco + 4);
				uint32_t* stco_table = (uint32_t*)(stco + 8);
				memmove(stco_table, &stco_table[chunk_start],
					(entries - chunk_start) * sizeof(uint32_t));
				write_int32(stco + 4, entries - chunk_start);

				// patch first chunk with correct sample offset
				//        uint32_t* stco_table = (uint32_t*)(stco + 8);
				write_int32(stco_table, trak->samples_[start].pos_);
			}
		}
	}

	// process sync samples:
	if(trak->mdia_.minf_.stbl_.stss_)
	{
		unsigned char* stss = trak->mdia_.minf_.stbl_.stss_;
		unsigned int entries = read_int32(stss + 4);
		uint32_t* table = (uint32_t*)(stss + 8);
		unsigned int stss_start;
		unsigned int i;
		for(i = 0; i != entries; ++i)
		{
			if(read_int32(&table[i]) >= start + 1)
				break;
		}
		stss_start = i;
		for(; i != entries; ++i)
		{
			unsigned int sync_sample = read_int32(&table[i]);
			if(sync_sample >= end + 1)
				break;
			write_int32(&table[i - stss_start], sync_sample - start);

		}
		//    memmove(table, table + stss_start, (i - stss_start) * sizeof(uint32_t));
		write_int32(stss + 4, i - stss_start);
	}

	// process sample sizes
	{
		unsigned char* stsz = trak->mdia_.minf_.stbl_.stsz_;
		if(stsz_get_sample_size(stsz) == 0)
		{
			uint32_t* table = (uint32_t*)(stsz + 12);
			memmove(table, &table[start], (end - start) * sizeof(uint32_t));
			write_int32(stsz + 8, end - start);
		}
	}
}

size_t calc_trak_size( struct trak_t * trak )
{
	size_t size = sizeof( struct trak_t );

	size += trak->chunks_size_ * sizeof( struct chunks_t );
	size += trak->samples_size_ * sizeof( struct samples_t );

	return size;
}

size_t calc_memory_size( struct moov_t* moov )
{
	size_t size = sizeof( struct moov_t );

	{
		unsigned int i;
		for(i = 0; i != moov->tracks_; ++i)
		{
			size += calc_trak_size(&moov->traks_[i]);
		}
	}

	return size;
}

int moov_parse(struct moov_t* moov, unsigned char* buffer, unsigned int size)
{
	struct atom_t leaf_atom;
	unsigned char* buffer_start = buffer;

	moov->start_ = buffer;

	while(buffer < buffer_start + size)
	{
		buffer = atom_read_header(buffer, &leaf_atom);

		atom_print(&leaf_atom);

		if(atom_is(&leaf_atom, "cmov"))
		{
			return 0;
		}
		else
			if(atom_is(&leaf_atom, "mvhd"))
			{
				moov->mvhd_ = buffer;
			}
			else
				if(atom_is(&leaf_atom, "trak"))
				{
					if(moov->tracks_ == MAX_TRACKS)
						return 0;
					else
					{
						struct trak_t* trak = &moov->traks_[moov->tracks_];
						trak_init(trak);
						trak_parse(trak, buffer, leaf_atom.size_ - ATOM_PREAMBLE_SIZE);
						++moov->tracks_;
					}
				}
				buffer = atom_skip(&leaf_atom);
	}

	// build the indexing tables
	{
		unsigned int i;
		for(i = 0; i != moov->tracks_; ++i)
		{
			trak_build_index(&moov->traks_[i]);
		}
	}

	//size_t memory_size = calc_memory_size( moov );
	//printf( "moov memory size: %d\n", memory_size );

	return 1;
}

void stco_shift_offsets(unsigned char* stco, int offset)
{
	unsigned int entries = read_int32(stco + 4);
	unsigned int* table = (unsigned int*)(stco + 8);
	unsigned int i;
	for(i = 0; i != entries; ++i)
		write_int32(&table[i], (read_int32(&table[i]) + offset));
}

void trak_shift_offsets(struct trak_t* trak, int offset)
{
	unsigned char* stco = trak->mdia_.minf_.stbl_.stco_;
	stco_shift_offsets(stco, offset);
}

void moov_shift_offsets(struct moov_t* moov, int offset)
{
	unsigned int i;
	for(i = 0; i != moov->tracks_; ++i)
	{
		trak_shift_offsets(&moov->traks_[i], offset);
	}
}

long mvhd_get_time_scale(unsigned char* mvhd)
{
	int version = read_char(mvhd);
	unsigned char* p = mvhd + (version == 0 ? 12 : 20);
	return read_int32(p);
}

void mvhd_set_duration(unsigned char* mvhd, long duration)
{
	int version = read_char(mvhd);
	if(version == 0)
	{
		write_int32(mvhd + 16, duration);
	}
	else
	{
		perror("mvhd_set_duration");
		//    write_int64(mvhd + 24, duration);
	}
}

long mdhd_get_time_scale(unsigned char* mdhd)
{
	return read_int32(mdhd + 12);
}

void mdhd_set_duration(unsigned char* mdhd, unsigned int duration)
{
	write_int32(mdhd + 16, duration);
}

void tkhd_set_duration(unsigned char* tkhd, unsigned int duration)
{
	int version = read_char(tkhd);
	if(version == 0)
	{
		write_int32(tkhd + 20, duration);
	}
	else
	{
		perror("tkhd_set_duration");
		//    write_int64(tkhd + 28, duration);
	}
}

unsigned int stss_get_entries(unsigned char const* stss)
{
	return read_int32(stss + 4);
}

unsigned int stss_get_sample(unsigned char const* stss, unsigned int idx)
{
	unsigned char const* p = stss + 8 + idx * 4;
	return read_int32(p);
}

unsigned int stss_get_nearest_keyframe(unsigned char const* stss, unsigned int sample)
{
	// scan the sync samples to find the key frame that precedes the sample number
	unsigned int i;
	unsigned int entries = stss_get_entries(stss);
	unsigned int table_sample = 0;
	for(i = 0; i != entries; ++i)
	{
		table_sample = stss_get_sample(stss, i);
		if(table_sample >= sample)
			break;
	}
	if(table_sample == sample)
		return table_sample;
	else
		return stss_get_sample(stss, i - 1);
}

unsigned int stbl_get_nearest_keyframe(struct stbl_t const* stbl, unsigned int sample)
{
	// If the sync atom is not present, all samples are implicit sync samples.
	if(!stbl->stss_)
		return sample;

	return stss_get_nearest_keyframe(stbl->stss_, sample);
}

unsigned int moov_seek(unsigned char* moov_data, unsigned int size,
					   float start_time,
					   unsigned int* mdat_start,
					   unsigned int* mdat_size,
					   unsigned int offset)
{
	long moov_time_scale = 0;
	unsigned int sync_start_time = 0;
	/// ** by liwei
	unsigned int sync_start = -1;	// sync 
	/// ** end

	struct moov_t* moov = (moov_t*)malloc(sizeof(struct moov_t));
	moov_init(moov);
	if(!moov_parse(moov, moov_data, size))
	{
		moov_exit(moov);
		free(moov);
		return 0;
	}


	{
		moov_time_scale = mvhd_get_time_scale(moov->mvhd_);
		printf( "time scale: %d\n", moov_time_scale );
		unsigned int start = (unsigned int)(start_time * moov_time_scale);
		//  unsigned int end = (unsigned int)(end_time * moov_time_scale);
		unsigned int bytes_to_skip = UINT_MAX;
		unsigned int i;

		// for every trak, convert seconds to sample (time-to-sample).
		// adjust sample to keyframe
		unsigned int trak_sample_start[MAX_TRACKS];
		//  unsigned int trak_sample_end[MAX_TRACKS];

		unsigned int moov_duration = 0;

		// clayton.mp4 has a third track with one sample that lasts the whole clip.
		// Assuming the first two tracks are the audio and video track, we patch
		// the remaining tracks to 'free' atoms.
		if(moov->tracks_ > 2)
		{
			for(i = 2; i != moov->tracks_; ++i)
			{
				// patch 'trak' to 'free'
				unsigned char* p = moov->traks_[i].start_ - 4;
				p[0] = 'f';
				p[1] = 'r';
				p[2] = 'e';
				p[3] = 'e';
			}
			moov->tracks_ = 2;
		}

		/// ** by liwei
		// av out of sync because: 
		// audio track without stss, seek to the exact time. 
		// however, video track with stss, seek to the nearest key frame time.

		printf( "\n\n== seeking....\n" );
		printf( "original start: %d\n", start );

		// sync start time to sync track key frame time.
		for(i = 0; i != moov->tracks_; ++i)
		{
			struct trak_t* trak = &moov->traks_[i];
			struct stbl_t* stbl = &trak->mdia_.minf_.stbl_;

			if( stbl->stss_ )	// there is 'stss', random access points, sync sample table.
			{
				long trak_time_scale = mdhd_get_time_scale(trak->mdia_.mdhd_);
				float moov_to_trak_time = (float)trak_time_scale / (float)moov_time_scale;
				float trak_to_moov_time = (float)moov_time_scale / (float)trak_time_scale;

				start = stts_get_sample(stbl->stts_, (unsigned int)(start * moov_to_trak_time));
				start = stss_get_nearest_keyframe(stbl->stss_, start + 1) - 1;

				trak_sample_start[i] = start;
				start = (unsigned int)(stts_get_time(stbl->stts_, start) * trak_to_moov_time);

				sync_start = start;
				sync_start_time = start;
				printf( "track [%d] start time: %d\n", i, start );
			}
		}
		printf( "sync start: %d\n", sync_start );
		/// ** end

		for(i = 0; i != moov->tracks_; ++i)
		{
			struct trak_t* trak = &moov->traks_[i];
			struct stbl_t* stbl = &trak->mdia_.minf_.stbl_;

			/// ** by liwei
			if( stbl->stss_ )		// already seeked
			{
				continue;
			}

			if ( sync_start != -1 )
			{
				start = sync_start;	// sync to sync track's time
			}
			/// ** end

			long trak_time_scale = mdhd_get_time_scale(trak->mdia_.mdhd_);
			float moov_to_trak_time = (float)trak_time_scale / (float)moov_time_scale;
			float trak_to_moov_time = (float)moov_time_scale / (float)trak_time_scale;

			start = stts_get_sample(stbl->stts_, (unsigned int)(start * moov_to_trak_time));
			start = stbl_get_nearest_keyframe(stbl, start + 1) - 1;
			trak_sample_start[i] = start;	// go to sample
			start = (unsigned int)(stts_get_time(stbl->stts_, start) * trak_to_moov_time);	// sample time
			printf( "track [%d] start time: %d\n", i, start );
			sync_start_time = start;
		}

		printf("start=%u\n", start);

		for(i = 0; i != moov->tracks_; ++i)
		{
			struct trak_t* trak = &moov->traks_[i];
			struct stbl_t* stbl = &trak->mdia_.minf_.stbl_;

			unsigned int start_sample = trak_sample_start[i];
			unsigned int end_sample = trak->samples_size_;

			trak_write_index(trak, start_sample, end_sample);

			{
				unsigned skip =
					trak->samples_[start_sample].pos_ - trak->samples_[0].pos_;
				if(skip < bytes_to_skip)
					bytes_to_skip = skip;
				printf("Trak[%d] can skip %u bytes\n", i, skip);

				long trak_time_scale = mdhd_get_time_scale(trak->mdia_.mdhd_);
				float moov_to_trak_time = (float)trak_time_scale / (float)moov_time_scale;
				float trak_to_moov_time = (float)moov_time_scale / (float)trak_time_scale;

				//unsigned int prev = start_sample - 1;
				//unsigned int next = start_sample + 1;

				//unsigned int prev_pos = trak->samples_[prev].pos_;
				unsigned int current_pos = trak->samples_[start_sample].pos_;
				//unsigned int next_pos = trak->samples_[next].pos_;

				//unsigned int prev_time = (unsigned int)( stts_get_time( stbl->stts_, prev) * trak_to_moov_time);
				unsigned int current_time = (unsigned int)( stts_get_time( stbl->stts_, start_sample) * trak_to_moov_time);
				//unsigned int next_time = (unsigned int)( stts_get_time( stbl->stts_, next) * trak_to_moov_time);
				//printf( "trak[%d] positions: prev %d-%d, current %d-%d, next %d-%d\n", i,
				//	prev_pos, prev_time, current_pos, current_time, next_pos, next_time );
			}

			{
				// fixup trak (duration)
				unsigned int trak_duration = stts_get_duration(stbl->stts_);
				long trak_time_scale = mdhd_get_time_scale(trak->mdia_.mdhd_);
				float trak_to_moov_time = (float)moov_time_scale / (float)trak_time_scale;
				unsigned int duration = (long)((float)trak_duration * trak_to_moov_time);
				mdhd_set_duration(trak->mdia_.mdhd_, trak_duration);
				tkhd_set_duration(trak->tkhd_, duration);
				printf("trak[%d]: new_duration=%d\n", i, duration);

				if(duration > moov_duration)
					moov_duration = duration;
			}

			printf("stco.size=%d, ", read_int32(stbl->stco_ + 4));
			printf("stts.size=%d samples=%d\n", read_int32(stbl->stts_ + 4), stts_get_samples(stbl->stts_));
			printf("stsz.size=%d\n", read_int32(stbl->stsz_ + 8));
			printf("stsc.samples=%d\n", stsc_get_samples(stbl->stsc_));
		}
		mvhd_set_duration(moov->mvhd_, moov_duration);

		offset -= bytes_to_skip;

		printf("shifting offsets by %d\n", offset);
		moov_shift_offsets(moov, offset);

		*mdat_start += bytes_to_skip;
		*mdat_size -= bytes_to_skip;
	}

	moov_exit(moov);
	free(moov);

	return sync_start_time * 1000 / moov_time_scale;
}

void write_char(unsigned char* outbuffer, int value)
{
	outbuffer[0] = (unsigned char)(value);
}

#define ATOM_PREAMBLE_SIZE 8


int file_atom_read_header(FILE* infile, struct atom_t* atom)
{
	unsigned char atom_bytes[ATOM_PREAMBLE_SIZE];

	atom->start_ = (unsigned char *)ftell(infile);

	fread(atom_bytes, ATOM_PREAMBLE_SIZE, 1, infile);
	memcpy(&atom->type_[0], &atom_bytes[4], 4);
	atom->size_ = atom_header_size(atom_bytes);
	atom->end_ = atom->start_ + atom->size_;

	return 1;
}

void atom_write_header(unsigned char* outbuffer, struct atom_t* atom)
{
	int i;
	write_int32(outbuffer, atom->size_);
	for(i = 0; i != 4; ++i)
		write_char(outbuffer + 4 + i, atom->type_[i]);
}

void file_atom_skip(FILE* infile, struct atom_t const* atom)
{
	fseek(infile, (long)atom->end_, SEEK_SET);
}

int main( int argc, char ** argv ) {
	if( argc < 4 ) {
		cout << argv[0] << " [filename] [time] [out filename]" << endl;
		return 0;
	}

	char * filename = argv[1];
	double start = strtod( argv[2], NULL );
	char * out_file = argv[3];

	FILE* infile;
	struct atom_t ftyp_atom;
	struct atom_t moov_atom;
	struct atom_t mdat_atom;
	unsigned char* moov_data = 0;
	unsigned char* ftyp_data = 0;
	struct stat file_stat;

	int err = stat( filename, &file_stat );
	if( 0 != err ) {
		printf( "read file stat failed.\n" );
		return 0;
	}

	infile = fopen(filename, "rb");
	if(!infile) {
		return -1;
	}

	{
		unsigned int filesize = file_stat.st_size;

		struct atom_t leaf_atom;
		while(ftell(infile) < filesize)
		{
			if(!file_atom_read_header(infile, &leaf_atom))
				break;

			atom_print(&leaf_atom);

			if(atom_is(&leaf_atom, "ftyp"))
			{
				ftyp_atom = leaf_atom;
				ftyp_data = (unsigned char *)malloc(ftyp_atom.size_);
				if ( ftyp_data == NULL )
				{
					// cout << "malloc ftyp_data failed.\n" << endl;
					return 1;
				}
				fseek(infile, (long)ftyp_atom.start_, SEEK_SET);
				fread(ftyp_data, ftyp_atom.size_, 1, infile);
			}
			else
				if(atom_is(&leaf_atom, "moov"))
				{
					moov_atom = leaf_atom;
					moov_data = (unsigned char *)malloc(moov_atom.size_);
					if(moov_data == NULL) {
						// cout << "malloc moov_data failed\n" << endl;
						return 1;
					}
					fseek(infile, (long)moov_atom.start_, SEEK_SET);
					fread(moov_data, moov_atom.size_, 1, infile);
				}
				else
					if(atom_is(&leaf_atom, "mdat"))
					{
						mdat_atom = leaf_atom;
					}
					file_atom_skip(infile, &leaf_atom);
					//atom_skip( &leaf_atom );
		}
	}
	fclose(infile);

	if(!moov_data) {
		printf( "no moov data found.\n" );
		return -1;
	}

//	test_seek( moov_data, moov_atom, mdat_atom );

	{

		unsigned int mdat_start = (ftyp_data ? ftyp_atom.size_ : 0) + moov_atom.size_;
		unsigned int sync_start_time = moov_seek(moov_data + ATOM_PREAMBLE_SIZE,
			moov_atom.size_ - ATOM_PREAMBLE_SIZE,
			start,
			(unsigned int *)&mdat_atom.start_, &mdat_atom.size_,
			mdat_start - (unsigned int)(mdat_atom.start_) );
		if(!sync_start_time) {
			printf( "seek failed.\n" );
			// return HANDLER_GO_ON;
			return -1;
		}

		printf( "**== report **\n" );
		FILE * outFile = fopen( out_file, "wb" );
		if(ftyp_data)
		{
			// buffer_append_memory(b, ftyp_data, ftyp_atom.size_);
			fwrite( ftyp_data, 1, ftyp_atom.size_, outFile );
			printf( "ftyp atom size: %d\n", ftyp_atom.size_ );
			free(ftyp_data);
		}

		// buffer_append_memory(b, moov_data, moov_atom.size_);
		fwrite( moov_data, moov_atom.size_, 1, outFile );
		printf( "moov atom size: %d\n", moov_atom.size_ );
		free(moov_data);

		{
			unsigned char mdat_bytes[ATOM_PREAMBLE_SIZE];
			//			mdat_atom.size_ -= bytes_to_skip;
			atom_write_header(mdat_bytes, &mdat_atom);
			// buffer_append_memory(b, mdat_bytes, ATOM_PREAMBLE_SIZE);
			fwrite( mdat_bytes, 1, ATOM_PREAMBLE_SIZE, outFile );
			//b->used++; // add virtual \0
		}

		printf( "mdat start: %d, size: %d, start time: %d\n", (unsigned int)mdat_atom.start_ + ATOM_PREAMBLE_SIZE, mdat_atom.size_ - ATOM_PREAMBLE_SIZE, sync_start_time );
		//http_chunk_append_file(srv, con, con->physical.path, mdat_atom.start_ + ATOM_PREAMBLE_SIZE,
		//	mdat_atom.size_ - ATOM_PREAMBLE_SIZE);
		size_t bytes_write = 0;
		infile = fopen(filename, "rb");
		fseek( infile, (unsigned long)(mdat_atom.start_ + ATOM_PREAMBLE_SIZE), SEEK_SET);
		char buf[1024];
		while( true ) {
			int bytes = fread( buf, 1, 1024, infile );
			if( bytes <= 0 ) {
				break;
			}
			fwrite( buf, 1, bytes, outFile );
		}
		fclose(infile);
		fclose(outFile);
	}

}

// End Of File



