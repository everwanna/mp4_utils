#include "StdAfx.h"
#include "parse.h"

#include "buffer_io.h"

unsigned int stts_get_entries(unsigned char const* stts)
{
	return buffer_io::read_int32(stts + 4);
}

void stts_get_sample_count_and_duration(unsigned char const* stts,
										unsigned int idx, unsigned int* sample_count, unsigned int* sample_duration)
{
	unsigned char const* table = stts + 8 + idx * 8;
	*sample_count = buffer_io::read_int32(table);
	*sample_duration = buffer_io::read_int32(table + 4);
}

uint32_t get_sample_duration( trak * trak_atom )
{
	unsigned int count = 0;
	unsigned int duration = 0;

	stts_get_sample_count_and_duration( trak_atom->mdia_.minf_.stbl_.stts_, 0, &count, &duration );

	return duration;
}

unsigned int ctts_get_entries(unsigned char const* ctts)
{
	return buffer_io::read_int32(ctts + 4);
}

void ctts_get_sample_count_and_offset(unsigned char const* ctts,
									  unsigned int idx, unsigned int* sample_count, unsigned int* sample_offset)
{
	unsigned char const* table = ctts + 8 + idx * 8;
	*sample_count = buffer_io::read_int32(table);
	*sample_offset = buffer_io::read_int32(table + 4);
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

unsigned int stsc_get_entries(unsigned char const* stsc)
{
	return buffer_io::read_int32(stsc + 4);
}

void stsc_get_table(unsigned char const* stsc, unsigned int i, struct stsc_table_t *stsc_table)
{
	struct stsc_table_t* table = (struct stsc_table_t*)(stsc + 8);
	stsc_table->chunk_ = buffer_io::read_int32(&table[i].chunk_) - 1;
	stsc_table->samples_ = buffer_io::read_int32(&table[i].samples_);
	stsc_table->id_ = buffer_io::read_int32(&table[i].id_);
}

unsigned int stsc_get_chunk(unsigned char* stsc, unsigned int sample)
{
	unsigned int entries = buffer_io::read_int32(stsc + 4);
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
			unsigned int chunk2 = buffer_io::read_int32(&table[chunk2entry].chunk_);
			chunk = chunk2 - chunk1;
			range_samples = chunk * chunk1samples;

			if(sample < total + range_samples)
				break;

			chunk1samples = buffer_io::read_int32(&table[chunk2entry].samples_);
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
	unsigned int entries = buffer_io::read_int32(stsc + 4);
	struct stsc_table_t* table = (struct stsc_table_t*)(stsc + 8);
	unsigned int samples = 0;
	unsigned int i;
	for(i = 0; i != entries; ++i)
	{
		samples += buffer_io::read_int32(&table[i].samples_);
	}
	return samples;
}

unsigned int stco_get_entries(unsigned char const* stco)
{
	return buffer_io::read_int32(stco + 4);
}

unsigned int stco_get_offset(unsigned char const* stco, int idx)
{
	uint32_t const* table = (uint32_t const*)(stco + 8);
	return buffer_io::read_int32(&table[idx]);
}

unsigned int stsz_get_sample_size(unsigned char const* stsz)
{
	return buffer_io::read_int32(stsz + 4);
}

unsigned int stsz_get_entries(unsigned char const* stsz)
{
	return buffer_io::read_int32(stsz + 8);
}

unsigned int stsz_get_size(unsigned char const* stsz, unsigned int idx)
{
	uint32_t const* table = (uint32_t const*)(stsz + 12);
	return buffer_io::read_int32(&table[idx]);
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
			return ret;
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
	if( time_count >= time )
	{
		return ret;
	}

	return -1;
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


long mdhd_get_time_scale(unsigned char* mdhd)
{
	return buffer_io::read_int32(mdhd + 12);
}


void stbl_parse(struct stbl_t* stbl, unsigned char* buffer, unsigned int size)
{
	atom leaf_atom;
	unsigned char* buffer_start = buffer;
	stbl->stss_ = 0;
	stbl->ctts_ = 0;

	stbl->start_ = buffer;

	while(buffer < buffer_start + size)
	{
		buffer = leaf_atom.read_header(buffer);

		leaf_atom.print();

		if( leaf_atom.is_type("stts") )
		{
			stbl->stts_ = buffer;
		}
		else
			if(leaf_atom.is_type("stss"))
			{
				stbl->stss_ = buffer;
			}
			else
				if(leaf_atom.is_type("stsc"))
				{
					stbl->stsc_ = buffer;
				}
				else
					if(leaf_atom.is_type("stsz"))
					{
						stbl->stsz_ = buffer;
					}
					else
						if(leaf_atom.is_type("stco"))
						{
							stbl->stco_ = buffer;
						}
						else
							if(leaf_atom.is_type("co64"))
							{
								perror("TODO: co64");
							}
							else
								if(leaf_atom.is_type("ctts"))
								{
									stbl->ctts_ = buffer;
								}

								buffer = leaf_atom.skip();
	}
}

void minf_parse(struct minf_t* minf, unsigned char* buffer, unsigned int size)
{
	atom leaf_atom;
	unsigned char* buffer_start = buffer;

	minf->start_ = buffer;

	while(buffer < buffer_start + size)
	{
		buffer = leaf_atom.read_header(buffer);

		leaf_atom.print();

		if(leaf_atom.is_type("stbl"))
		{
			stbl_parse(&minf->stbl_, buffer, leaf_atom.size_ - atom::PREAMBLE_SIZE);
		}

		buffer = leaf_atom.skip();
	}
}


void mdia_parse(struct mdia_t* mdia, unsigned char* buffer, unsigned int size)
{
	atom leaf_atom;
	unsigned char* buffer_start = buffer;

	mdia->start_ = buffer;

	while(buffer < buffer_start + size)
	{
		buffer = leaf_atom.read_header(buffer);

		leaf_atom.print();

		if(leaf_atom.is_type("mdhd"))
		{
			mdia->mdhd_ = buffer;
		}
		else
			if(leaf_atom.is_type("minf"))
			{
				minf_parse(&mdia->minf_, buffer, leaf_atom.size_ - atom::PREAMBLE_SIZE);
			}

			buffer = leaf_atom.skip();
	}
}


void trak_write_index(trak* trak_atom, unsigned int start, unsigned int end)
{
	// write samples [start,end>

	// stts = [entries * [sample_count, sample_duration]
	{
		unsigned char* stts = trak_atom->mdia_.minf_.stbl_.stts_;
		unsigned int entries = 0;
		struct stts_table_t* table = (struct stts_table_t*)(stts + 8);
		unsigned int s;
		for(s = start; s != end; ++s)
		{
			unsigned int sample_count = 1;
			unsigned int sample_duration =
				trak_atom->samples_[s + 1].pts_ - trak_atom->samples_[s].pts_;
			while(s != end - 1)
			{
				if((trak_atom->samples_[s + 1].pts_ - trak_atom->samples_[s].pts_) != sample_duration)
					break;
				++sample_count;
				++s;
			}
			// write entry
			buffer_io::write_int32(&table[entries].sample_count_, sample_count);
			buffer_io::write_int32(&table[entries].sample_duration_, sample_duration);
			++entries;
		}
		buffer_io::write_int32(stts + 4, entries);
		if(stts_get_samples(stts) != end - start)
		{
			printf("ERROR: stts_get_samples=%d, should be %d\n",
				stts_get_samples(stts), end - start);
		}
	}

	// ctts = [entries * [sample_count, sample_offset]
	{
		unsigned char* ctts = trak_atom->mdia_.minf_.stbl_.ctts_;
		if(ctts)
		{
			unsigned int entries = 0;
			struct ctts_table_t* table = (struct ctts_table_t*)(ctts + 8);
			unsigned int s;
			for(s = start; s != end; ++s)
			{
				unsigned int sample_count = 1;
				unsigned int sample_offset = trak_atom->samples_[s].cto_;
				while(s != end - 1)
				{
					if(trak_atom->samples_[s + 1].cto_ != sample_offset)
						break;
					++sample_count;
					++s;
				}
				// write entry
				buffer_io::write_int32(&table[entries].sample_count_, sample_count);
				buffer_io::write_int32(&table[entries].sample_offset_, sample_offset);
				++entries;
			}
			buffer_io::write_int32(ctts + 4, entries);
			if(ctts_get_samples(ctts) != end - start)
			{
				printf("ERROR: ctts_get_samples=%d, should be %d\n",
					ctts_get_samples(ctts), end - start);
			}
		}
	}

	// process chunkmap:
	{
		unsigned char* stsc = trak_atom->mdia_.minf_.stbl_.stsc_;
		struct stsc_table_t* stsc_table = (struct stsc_table_t*)(stsc + 8);
		unsigned int i;
		for(i = 0; i != trak_atom->chunks_size_; ++i)
		{
			if(trak_atom->chunks_[i].sample_ + trak_atom->chunks_[i].size_ > start)
				break;
		}

		{
			unsigned int stsc_entries = 0;
			unsigned int chunk_start = i;
			unsigned int samples =
				trak_atom->chunks_[i].sample_ + trak_atom->chunks_[i].size_ - start;
			unsigned int id = trak_atom->chunks_[i].id_;

			// write entry [chunk,samples,id]
			buffer_io::write_int32(&stsc_table[stsc_entries].chunk_, 1);
			buffer_io::write_int32(&stsc_table[stsc_entries].samples_, samples);
			buffer_io::write_int32(&stsc_table[stsc_entries].id_, id);
			++stsc_entries;
			if(i != trak_atom->chunks_size_)
			{
				for(i += 1; i != trak_atom->chunks_size_; ++i)
				{
					if(trak_atom->chunks_[i].size_ != samples)
					{
						samples = trak_atom->chunks_[i].size_;
						id = trak_atom->chunks_[i].id_;
						buffer_io::write_int32(&stsc_table[stsc_entries].chunk_, i - chunk_start + 1);
						buffer_io::write_int32(&stsc_table[stsc_entries].samples_, samples);
						buffer_io::write_int32(&stsc_table[stsc_entries].id_, id);
						++stsc_entries;
					}
				}
			}
			buffer_io::write_int32(stsc + 4, stsc_entries);

			{
				unsigned char* stco = trak_atom->mdia_.minf_.stbl_.stco_;
				//        stco_erase(stco, chunk_start);
				unsigned int entries = buffer_io::read_int32(stco + 4);
				uint32_t* stco_table = (uint32_t*)(stco + 8);
				memmove(stco_table, &stco_table[chunk_start],
					(entries - chunk_start) * sizeof(uint32_t));
				buffer_io::write_int32(stco + 4, entries - chunk_start);

				for ( size_t chunk_index = chunk_start; chunk_index < entries; ++ chunk_index )
				{
					buffer_io::write_int32( stco_table + (chunk_index - chunk_start), trak_atom->chunks_[chunk_index].pos_ );
				}

				// patch first chunk with correct sample offset
				//        uint32_t* stco_table = (uint32_t*)(stco + 8);
				// write_int32(stco_table, trak_atom->samples_[start].pos_);
			}
		}
	}

	// process sync samples:
	if(trak_atom->mdia_.minf_.stbl_.stss_)
	{
		unsigned char* stss = trak_atom->mdia_.minf_.stbl_.stss_;
		unsigned int entries = buffer_io::read_int32(stss + 4);
		uint32_t* table = (uint32_t*)(stss + 8);
		unsigned int stss_start;
		unsigned int i;
		for(i = 0; i != entries; ++i)
		{
			if( static_cast<uint32_t>( buffer_io::read_int32(&table[i]) ) >= start + 1)
				break;
		}
		stss_start = i;
		for(; i != entries; ++i)
		{
			unsigned int sync_sample = buffer_io::read_int32(&table[i]);
			if(sync_sample >= end + 1)
				break;
			buffer_io::write_int32(&table[i - stss_start], sync_sample - start);

		}
		//    memmove(table, table + stss_start, (i - stss_start) * sizeof(uint32_t));
		buffer_io::write_int32(stss + 4, i - stss_start);
	}

	// process sample sizes
	{
		unsigned char* stsz = trak_atom->mdia_.minf_.stbl_.stsz_;
		if(stsz_get_sample_size(stsz) == 0)
		{
			uint32_t* table = (uint32_t*)(stsz + 12);
			memmove(table, &table[start], (end - start) * sizeof(uint32_t));
			buffer_io::write_int32(stsz + 8, end - start);
		}
	}
}

long mvhd_get_time_scale(unsigned char* mvhd)
{
	int version = buffer_io::read_char(mvhd);
	unsigned char* p = mvhd + (version == 0 ? 12 : 20);
	return buffer_io::read_int32(p);
}

void mvhd_set_duration(unsigned char* mvhd, long duration)
{
	int version = buffer_io::read_char(mvhd);
	if(version == 0)
	{
		buffer_io::write_int32(mvhd + 16, duration);
	}
	else
	{
		perror("mvhd_set_duration");
		//    write_int64(mvhd + 24, duration);
	}
}

uint32_t mvhd_get_duration(unsigned char* mvhd)
{
	int version = buffer_io::read_char(mvhd);
	if(version == 0)
	{
		return static_cast<uint32_t>( buffer_io::read_int32(mvhd + 16) );
	}
	else
	{
		throw std::exception( "do not support 64bit operations. can not get mvhd duration." );
		//    write_int64(mvhd + 24, duration);
	}
}

void mdhd_set_duration(unsigned char* mdhd, unsigned int duration)
{
	buffer_io::write_int32(mdhd + 16, duration);
}

void tkhd_set_duration(unsigned char* tkhd, unsigned int duration)
{
	int version = buffer_io::read_char(tkhd);
	if(version == 0)
	{
		buffer_io::write_int32(tkhd + 20, duration);
	}
	else
	{
		perror("tkhd_set_duration");
		//    write_int64(tkhd + 28, duration);
	}
}

unsigned int stss_get_entries(unsigned char const* stss)
{
	return buffer_io::read_int32(stss + 4);
}

unsigned int stss_get_sample(unsigned char const* stss, unsigned int idx)
{
	unsigned char const* p = stss + 8 + idx * 4;
	return buffer_io::read_int32(p);
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
		{
			return table_sample;
		}
	}

	return static_cast<unsigned int>(-1);
	//if(table_sample == sample)
	//	return table_sample;
	//else
	//	return stss_get_sample(stss, i - 1);
}

unsigned int stbl_get_nearest_keyframe(struct stbl_t const* stbl, unsigned int sample)
{
	// If the sync atom is not present, all samples are implicit sync samples.
	if(!stbl->stss_)
		return sample;

	return stss_get_nearest_keyframe(stbl->stss_, sample);
}
//
int file_atom_read_header(FILE* infile, atom* atom)
{
	unsigned char atom_bytes[atom::PREAMBLE_SIZE];

	atom->start_ = (unsigned char *)ftell(infile);

	fread(atom_bytes, atom::PREAMBLE_SIZE, 1, infile);
	memcpy(&atom->type_[0], &atom_bytes[4], 4);
	atom->size_ = atom::header_size(atom_bytes);
	atom->end_ = atom->start_ + atom->size_;

	return 1;
}


void file_atom_skip(FILE* infile, atom const* atom)
{
	fseek(infile, (long)atom->end_, SEEK_SET);
}

//
//void stco_shift_offsets(unsigned char* stco, int offset)
//{
//	unsigned int entries = buffer_io::read_int32(stco + 4);
//	unsigned int* table = (unsigned int*)(stco + 8);
//	unsigned int i;
//	for(i = 0; i != entries; ++i)
//		buffer_io::write_int32(&table[i], (buffer_io::read_int32(&table[i]) + offset));
//}
//
//void trak_shift_offsets(trak* trak_atom, int offset)
//{
//	unsigned char* stco = trak_atom->mdia_.minf_.stbl_.stco_;
//	stco_shift_offsets(stco, offset);
//}
//
//void moov_shift_offsets(moov* moov, int offset)
//{
//	unsigned int i;
//	for(i = 0; i != moov->tracks_; ++i)
//	{
//		trak_shift_offsets(&moov->traks_[i], offset);
//	}
//}

//unsigned int moov_seek(unsigned char* moov_data, unsigned int size,
//					   float start_time,
//					   unsigned int* mdat_start,
//					   unsigned int* mdat_size,
//					   unsigned int offset)
//{
//	long moov_time_scale = 0;
//	unsigned int sync_start_time = 0;
//	/// ** by liwei
//	unsigned int sync_start = static_cast<unsigned int>(-1);	// sync 
//	/// ** end
//
//	moov* moov_atom = new moov(); // (moov_t*)malloc(sizeof(moov_t));
////	moov_init(moov);
//	if(!moov_atom->parse( moov_data, size))
//	{
//		delete moov_atom;
//		return 0;
//	}
//
//
//	{
//		moov_time_scale = mvhd_get_time_scale(moov_atom->mvhd_);
//		printf( "time scale: %d\n", moov_time_scale );
//		unsigned int start = (unsigned int)(start_time * moov_time_scale);
//		//  unsigned int end = (unsigned int)(end_time * moov_time_scale);
//		unsigned int bytes_to_skip = UINT_MAX;
//		unsigned int i;
//
//		// for every trak, convert seconds to sample (time-to-sample).
//		// adjust sample to keyframe
//		unsigned int trak_sample_start[MAX_TRACKS];
//		//  unsigned int trak_sample_end[MAX_TRACKS];
//
//		unsigned int moov_duration = 0;
//
//		// clayton.mp4 has a third track with one sample that lasts the whole clip.
//		// Assuming the first two tracks are the audio and video track, we patch
//		// the remaining tracks to 'free' atoms.
//		if(moov_atom->tracks_ > 2)
//		{
//			for(i = 2; i != moov_atom->tracks_; ++i)
//			{
//				// patch 'trak' to 'free'
//				unsigned char* p = moov_atom->traks_[i].start_ - 4;
//				p[0] = 'f';
//				p[1] = 'r';
//				p[2] = 'e';
//				p[3] = 'e';
//			}
//			moov_atom->tracks_ = 2;
//		}
//
//		/// ** by liwei
//		// av out of sync because: 
//		// audio track without stss, seek to the exact time. 
//		// however, video track with stss, seek to the nearest key frame time.
//
//		printf( "\n\n== seeking....\n" );
//		printf( "original start: %d\n", start );
//
//		// sync start time to sync track key frame time.
//		for(i = 0; i != moov_atom->tracks_; ++i)
//		{
//			trak* trak_atom = &moov_atom->traks_[i];
//			struct stbl_t* stbl = &trak_atom->mdia_.minf_.stbl_;
//
//			if( stbl->stss_ )	// there is 'stss', random access points, sync sample table.
//			{
//				long trak_time_scale = mdhd_get_time_scale(trak_atom->mdia_.mdhd_);
//				float moov_to_trak_time = (float)trak_time_scale / (float)moov_time_scale;
//				float trak_to_moov_time = (float)moov_time_scale / (float)trak_time_scale;
//
//				start = stts_get_sample(stbl->stts_, (unsigned int)(start * moov_to_trak_time));
//				start = stss_get_nearest_keyframe(stbl->stss_, start + 1) - 1;
//
//				trak_sample_start[i] = start;
//				start = (unsigned int)(stts_get_time(stbl->stts_, start) * trak_to_moov_time);
//
//				sync_start = start;
//				sync_start_time = start;
//				printf( "track [%d] start time: %d\n", i, start );
//			}
//		}
//		printf( "sync start: %d\n", sync_start );
//		/// ** end
//
//		for(i = 0; i != moov_atom->tracks_; ++i)
//		{
//			trak* trak_atom = &moov_atom->traks_[i];
//			struct stbl_t* stbl = &trak_atom->mdia_.minf_.stbl_;
//
//			/// ** by liwei
//			if( stbl->stss_ )		// already seeked
//			{
//				continue;
//			}
//
//			if ( sync_start != -1 )
//			{
//				start = sync_start;	// sync to sync track's time
//			}
//			/// ** end
//
//			long trak_time_scale = mdhd_get_time_scale(trak_atom->mdia_.mdhd_);
//			float moov_to_trak_time = (float)trak_time_scale / (float)moov_time_scale;
//			float trak_to_moov_time = (float)moov_time_scale / (float)trak_time_scale;
//
//			start = stts_get_sample(stbl->stts_, (unsigned int)(start * moov_to_trak_time));
//			start = stbl_get_nearest_keyframe(stbl, start + 1) - 1;
//			trak_sample_start[i] = start;	// go to sample
//			start = (unsigned int)(stts_get_time(stbl->stts_, start) * trak_to_moov_time);	// sample time
//			printf( "track [%d] start time: %d\n", i, start );
//			sync_start_time = start;
//		}
//
//		printf("start=%u\n", start);
//
//		for(i = 0; i != moov_atom->tracks_; ++i)
//		{
//			trak* trak_atom = &moov_atom->traks_[i];
//			struct stbl_t* stbl = &trak_atom->mdia_.minf_.stbl_;
//
//			unsigned int start_sample = trak_sample_start[i];
//			unsigned int end_sample = trak_atom->samples_size_;
//
//			trak_write_index(trak_atom, start_sample, end_sample);
//
//			{
//				unsigned skip =
//					trak_atom->samples_[start_sample].pos_ - trak_atom->samples_[0].pos_;
//				if(skip < bytes_to_skip)
//					bytes_to_skip = skip;
//				printf("Trak[%d] can skip %u bytes\n", i, skip);
//
//				long trak_time_scale = mdhd_get_time_scale(trak_atom->mdia_.mdhd_);
//				float trak_to_moov_time = (float)moov_time_scale / (float)trak_time_scale;
//
//				unsigned int prev = start_sample - 1;
//				unsigned int next = start_sample + 1;
//
//				unsigned int prev_pos = trak_atom->samples_[prev].pos_;
//				unsigned int current_pos = trak_atom->samples_[start_sample].pos_;
//				unsigned int next_pos = trak_atom->samples_[next].pos_;
//
//				unsigned int prev_time = (unsigned int)( stts_get_time( stbl->stts_, prev) * trak_to_moov_time);
//				unsigned int current_time = (unsigned int)( stts_get_time( stbl->stts_, start_sample) * trak_to_moov_time);
//				unsigned int next_time = (unsigned int)( stts_get_time( stbl->stts_, next) * trak_to_moov_time);
//				printf( "trak[%d] positions: prev %d-%d, current %d-%d, next %d-%d\n", i,
//					prev_pos, prev_time, current_pos, current_time, next_pos, next_time );
//			}
//
//			{
//				// fixup trak (duration)
//				unsigned int trak_duration = stts_get_duration(stbl->stts_);
//				long trak_time_scale = mdhd_get_time_scale(trak_atom->mdia_.mdhd_);
//				float trak_to_moov_time = (float)moov_time_scale / (float)trak_time_scale;
//				unsigned int duration = (long)((float)trak_duration * trak_to_moov_time);
//				mdhd_set_duration(trak_atom->mdia_.mdhd_, trak_duration);
//				tkhd_set_duration(trak_atom->tkhd_, duration);
//				printf("trak[%d]: new_duration=%d\n", i, duration);
//
//				if(duration > moov_duration)
//					moov_duration = duration;
//			}
//
//			printf("stco.size=%d, ", buffer_io::read_int32(stbl->stco_ + 4));
//			printf("stts.size=%d samples=%d\n", buffer_io::read_int32(stbl->stts_ + 4), stts_get_samples(stbl->stts_));
//			printf("stsz.size=%d\n", buffer_io::read_int32(stbl->stsz_ + 8));
//			printf("stsc.samples=%d\n", stsc_get_samples(stbl->stsc_));
//		}
//		mvhd_set_duration(moov_atom->mvhd_, moov_duration);
//
//		offset -= bytes_to_skip;
//
//		printf("shifting offsets by %d\n", offset);
//		moov_shift_offsets(moov_atom, offset);
//
//		*mdat_start += bytes_to_skip;
//		*mdat_size -= bytes_to_skip;
//	}
//	if( _heapchk() != _HEAPOK ) {
//		cout << "heap corrupted." << endl;
//	}
//
//	delete moov_atom;
//
//	if( _heapchk() != _HEAPOK ) {
//		cout << "heap corrupted." << endl;
//	}
//
//	return sync_start_time * 1000 / moov_time_scale;
//}
