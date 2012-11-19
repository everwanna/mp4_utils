#include "StdAfx.h"
#include "trak.h"
#include "parse.h"

trak::~trak()
{
}

trak::trak() : start_(NULL)
, tkhd_(NULL)
, mdia_()

, chunks_size_(0)
, chunks_(NULL)

, samples_size_(0)
, samples_(NULL)
{

}

void trak::parse( unsigned char* buffer, unsigned int size )
{
	atom leaf_atom;
	unsigned char* buffer_start = buffer;

	start_ = buffer;

	while(buffer < buffer_start + size)
	{
		buffer = leaf_atom.read_header(buffer);

		leaf_atom.print();

		if(leaf_atom.is_type("tkhd"))
		{
			tkhd_ = buffer;
		}
		else
			if(leaf_atom.is_type("mdia"))
			{
				mdia_parse(&mdia_, buffer, leaf_atom.size_ - atom::PREAMBLE_SIZE);
			}

			buffer = leaf_atom.skip();
	}
}

void trak::trak_build_index()
{
	void const* stco = mdia_.minf_.stbl_.stco_;

	chunks_size_ = stco_get_entries( (const unsigned char *)(stco) );
	chunks_.reset( new chunks_t[ chunks_size_ ] ); // = (chunks_t*)malloc(chunks_size_ * sizeof(struct chunks_t));

	calc_chunk_offset( stco);

	// process chunkmap:
	process_chunk_map();

	// calc pts of chunks:
	calc_chunk_pts();

	if( _heapchk() != _HEAPOK )
	{
		cout << "heap corrupted." << endl;
		assert( false );
	}

	// calc pts:
	calc_pts();

	// calc composition times:
	calc_composition_times();

	// calc sample offsets
	calc_sample_offset();
}

void trak::calc_chunk_offset( void const* stco )
	{
		unsigned int i;
		for(i = 0; i != chunks_size_; ++i)
		{
			chunks_[i].pos_ = stco_get_offset((const unsigned char *)stco, i);
		}
	}

void trak::process_chunk_map()
	{
		void const* stsc = mdia_.minf_.stbl_.stsc_;
		unsigned int last = chunks_size_;
		unsigned int i = stsc_get_entries((const unsigned char *)stsc);

		while(i > 0)
		{
			struct stsc_table_t stsc_table;
			unsigned int j;

			--i;

			stsc_get_table((const unsigned char *)stsc, i, &stsc_table);
			for(j = stsc_table.chunk_; j < last; j++)
			{
				chunks_[j].id_ = stsc_table.id_;
				chunks_[j].size_ = stsc_table.samples_;
			}
			last = stsc_table.chunk_;
		}
	}

void trak::calc_chunk_pts()
{
	void const* stsz = mdia_.minf_.stbl_.stsz_;
	unsigned int sample_size = stsz_get_sample_size((const unsigned char *)stsz);
	unsigned int s = 0;
	{
		unsigned int j;
		for(j = 0; j < chunks_size_; j++)
		{
			chunks_[j].sample_ = s;
			s += chunks_[j].size_;
		}
	}

	if(sample_size == 0)
	{
		samples_size_ = stsz_get_entries((const unsigned char *)stsz);
	}
	else
	{
		samples_size_ = s;
	}

	if( _heapchk() != _HEAPOK )
	{
		cout << "heap corrupted." << endl;
		assert( false );
	}

	// samples_ = (samples_t*)malloc(samples_size_ * sizeof(struct samples_t));
	samples_.reset( new samples_t[ samples_size_ ] );
	cout << "\tsample count: " << samples_size_ << endl;

	int total_sample_size = 0;
	if(sample_size == 0)
	{
		unsigned int i;
		for(i = 0; i != samples_size_ ; ++i) {
			samples_[i].size_ = stsz_get_size((const unsigned char *)stsz, i);
			total_sample_size += samples_[i].size_;
		}
	}
	else
	{
		unsigned int i;
		for(i = 0; i != samples_size_ ; ++i) {
			samples_[i].size_ = sample_size;
			total_sample_size += samples_[i].size_;
		}
	}
	cout << "\ttotal sample size: " << total_sample_size << endl;
}

void trak::calc_pts()
	{
		void const* stts = mdia_.minf_.stbl_.stts_;
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
				samples_[s].pts_ = pts;
				++s;
				pts += sample_duration;
			}
		}
		cout << "\ttotal samples in stts: " << s << endl;
	}

void trak::calc_composition_times()
{
		void const* ctts = mdia_.minf_.stbl_.ctts_;
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
					if ( s < samples_size_ )
					{
						samples_[s].cto_ = sample_offset;
					}
					else
					{
						cout << "entry index: " << j << ", sample index: " << s << endl;
					}
					++s;
				}
			}
			cout << "\ttotal samples in ctts: " << s << endl;
		}
}

void trak::calc_sample_offset()
{
		unsigned int s = 0;
		unsigned int j;

		for(j = 0; j < chunks_size_; j++)
		{

			size_t chunk_size = 0;

			unsigned int pos = chunks_[j].pos_;
			unsigned int i;
			for(i = 0; i < chunks_[j].size_; i++)
			{
				if( i == 0 )
				{
					chunks_[j].start_time_ = samples_[s].pts_;
				}

				samples_[s].pos_ = pos;
				pos += samples_[s].size_;

				chunk_size += samples_[s].size_;

				++s;
			}

			chunks_[j].data_size_ =chunk_size;
		}
}
void trak::copy_chunks_samples(trak const&t)
{
	chunks_size_ = t.chunks_size_;
	chunks_.reset(new chunks_t[chunks_size_]);
	samples_size_ = t.samples_size_;
	samples_.reset(new samples_t[samples_size_]);
	for (size_t i = 0; i < chunks_size_; ++i)
	{
			chunks_[i] = t.chunks_[i];
	}

	for (size_t i = 0; i < samples_size_; ++i)
	{
			samples_[i] = t.samples_[i];
	}
}
