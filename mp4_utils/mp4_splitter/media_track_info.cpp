#include "StdAfx.h"
#include "media_track_info.h"


void media_track_info::fix_sample_table( trak & track )
{
	if( _heapchk() != _HEAPOK )
	{
		cout << "heap corrupted." << endl;
		assert( false );
	}

	// fix stts, decoding time to sample
	fix_stts(track);

	// fix ctts, composition time to sample box
	fix_ctts(track);

	// fix stsc, sample to chunk
	fix_stsc(track);

	// fix stss, sync sample box
	fix_stss(track);

	fix_stsz(track);

	//// FIX STSC, STCO
	//throw std::exception( "to fix stsz, stco" );


	if( _heapchk() != _HEAPOK )
	{
		cout << "heap corrupted." << endl;
		assert( false );
	}
}

void media_track_info::fix_stss( trak & track )	// checked
{
	// process sync samples:
	if(track.mdia_.minf_.stbl_.stss_)
	{
		unsigned char* stss = track.mdia_.minf_.stbl_.stss_;
		unsigned int entries = buffer_io::read_int32(stss + 4);
		uint32_t* table = (uint32_t*)(stss + 8);
		unsigned int stss_start;
		unsigned int i;
		for(i = 0; i != entries; ++i)
		{
			if( static_cast<size_t>( buffer_io::read_int32(&table[i]) ) >= sample_start_ )
				break;
		}
		stss_start = i;
		for(; i != entries; ++i)
		{
			unsigned int sync_sample = buffer_io::read_int32(&table[i]);
			if(sync_sample >= sample_end_ )
				break;
			buffer_io::write_int32(&table[i - stss_start], sync_sample - sample_start_ + 1 );	// sample index start with 1

		}
		//    memmove(table, table + stss_start, (i - stss_start) * sizeof(uint32_t));
		buffer_io::write_int32(stss + 4, i - stss_start);
	}
}

void media_track_info::fix_stts( trak & track ) // checked
{
	// stts = [entries * [sample_count, sample_duration]
	{
		unsigned char* stts = track.mdia_.minf_.stbl_.stts_;
		unsigned int entries = 0;
		struct stts_table_t* table = (struct stts_table_t*)(stts + 8);
		for(size_t s = sample_start_ - 1; s != sample_end_ - 1; ++s)
		{
			unsigned int sample_count = 1;
			unsigned int sample_duration = track.samples_[s + 1].pts_ - track.samples_[s].pts_;
			while(s != sample_end_ - 2)
			{
				if((track.samples_[s + 1].pts_ - track.samples_[s].pts_) != sample_duration)
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

		// check
		size_t sample_count = ::stts_get_samples(stts);
		if( sample_count != sample_end_ - sample_start_ )
		{
			printf("ERROR: stts_get_samples=%d, should be %d\n", sample_count, sample_end_ - sample_start_ );
		}
	}
}

void media_track_info::fix_stsc( trak & track )
{
	// process chunkmap:
	{
		unsigned char* stsc = track.mdia_.minf_.stbl_.stsc_;
		struct stsc_table_t* stsc_table = (struct stsc_table_t*)(stsc + 8);
		unsigned int i;
		unsigned int j = 0;

		for(i = 0; i != track.chunks_size_; ++i)
		{
			if( ( track.chunks_[i].sample_ + 1 ) + track.chunks_[i].size_ > sample_start_ )	// NOTE: chunks_[i].sample_ start at 0
				break;
		}

		for(j = i; j != track.chunks_size_; ++j)
		{
			if( ( track.chunks_[j].sample_ + 1 ) + track.chunks_[j].size_ > sample_end_ )
				break;
		}

		size_t chunk_start = i;	// include, start at 0
		size_t chunk_end = j;	// exclude, start at 0

		cout << "stsc: chunk count[" << track.chunks_size_ << "], chunk start[" << chunk_start << "], chunk end[" << chunk_end << "]\n";

		{
			unsigned int stsc_entries = 0;
			unsigned int samples =
				( track.chunks_[i].sample_ + 1 )  + track.chunks_[i].size_ - sample_start_;
			unsigned int id = track.chunks_[i].id_;

			if( samples > sample_end_ - sample_start_ )
			{
				samples = sample_end_ - sample_start_;
			}

			// write entry [chunk,samples,id], first chunk
			buffer_io::write_int32(&stsc_table[stsc_entries].chunk_, 1);
			buffer_io::write_int32(&stsc_table[stsc_entries].samples_, samples);
			buffer_io::write_int32(&stsc_table[stsc_entries].id_, id);
			++stsc_entries;

			cout << "stsc: chunk[" << 1 << "] samples[" << samples << "]" << endl;

			for(i += 1; i < chunk_end; ++i)
			{
				if(track.chunks_[i].size_ != samples)
				{
					samples = track.chunks_[i].size_;
					id = track.chunks_[i].id_;
					buffer_io::write_int32(&stsc_table[stsc_entries].chunk_, i - chunk_start + 1);
					buffer_io::write_int32(&stsc_table[stsc_entries].samples_, samples);
					buffer_io::write_int32(&stsc_table[stsc_entries].id_, id);
					cout << "stsc: chunk[" << i - chunk_start + 1 << "] samples[" << samples << "]" << endl;
					++stsc_entries;
				}
			}

			{
			// last chunk
				cout << "stsc: do last chunk, current chunk index = " << i << endl;
				if( i < track.chunks_size_ )
				{
					samples = sample_end_ - ( track.chunks_[i].sample_ + 1 ) ;
					id = track.chunks_[i].id_;
					buffer_io::write_int32(&stsc_table[stsc_entries].chunk_, i - chunk_start + 1);
					buffer_io::write_int32(&stsc_table[stsc_entries].samples_, samples);
					buffer_io::write_int32(&stsc_table[stsc_entries].id_, id);
					cout << "stsc: chunk[" << i - chunk_start + 1 << "] samples[" << samples << "]" << endl;
					++stsc_entries;
					++chunk_end;	// last sample in end chunk, should increase 1
				}
			}

			buffer_io::write_int32(stsc + 4, stsc_entries);
			cout << "stsc: entries[" << stsc_entries << "]" << endl;

			{
				unsigned char* stco = track.mdia_.minf_.stbl_.stco_;
				//        stco_erase(stco, chunk_start);
				unsigned int entries = buffer_io::read_int32(stco + 4);
				uint32_t* stco_table = (uint32_t*)(stco + 8);
				::memmove(stco_table, &stco_table[chunk_start],
					(chunk_end - chunk_start) * sizeof(uint32_t));
				buffer_io::write_int32(stco + 4, chunk_end - chunk_start);
				cout << "stco: chunk count[" << chunk_end - chunk_start << "]\n";

				// patch first chunk with correct sample offset
				//        uint32_t* stco_table = (uint32_t*)(stco + 8);
				buffer_io::write_int32(stco_table, track.samples_[sample_start_ - 1].pos_);
			}
		}
	}
}

void media_track_info::fix_ctts( trak & track ) // checked
{
	// ctts = [entries * [sample_count, sample_offset]
	{
		unsigned char* ctts = track.mdia_.minf_.stbl_.ctts_;
		if(ctts)
		{
			unsigned int entries = 0;
			struct ctts_table_t* table = (struct ctts_table_t*)(ctts + 8);
			unsigned int s;
			for(s = sample_start_ - 1; s != sample_end_ - 1; ++s)
			{
				unsigned int sample_count = 1;
				unsigned int sample_offset = track.samples_[s].cto_;
				while(s != sample_end_ - 2)
				{
					if(track.samples_[s + 1].cto_ != sample_offset)
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
			// check
			size_t sample_count = ::ctts_get_samples(ctts);
			if( sample_count != sample_end_ - sample_start_ )
			{
				printf("ERROR: ctts_get_samples=%d, should be %d\n", sample_count, sample_end_ - sample_start_ );
			}
		}
	}
}

void media_track_info::fix_stsz( trak & track )
{
  // process sample sizes
  {
    unsigned char* stsz = track.mdia_.minf_.stbl_.stsz_;
    //if(stsz_get_sample_size(stsz) == 0)
    //{
      uint32_t* table = (uint32_t*)(stsz + 12);
	  ::memmove(table, &table[sample_start_ - 1], (sample_end_ - sample_start_) * sizeof(uint32_t));
	  buffer_io::write_int32(stsz + 8, sample_end_ - sample_start_);
    //}
  }
}