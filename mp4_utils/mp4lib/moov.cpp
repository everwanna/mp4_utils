#include "StdAfx.h"
#include "moov.h"
#include "parse.h"

int moov::parse( byte * buffer, size_t size )
{
	atom leaf_atom;
	unsigned char* buffer_start = buffer;

	start_ = buffer;

	while(buffer < buffer_start + size)
	{
		buffer = leaf_atom.read_header(buffer);

		leaf_atom.print();

		if(leaf_atom.is_type("cmov"))
		{
			return 0;
		}
		else
			if(leaf_atom.is_type("mvhd"))
			{
				mvhd_ = buffer;
			}
			else
				if(leaf_atom.is_type("trak"))
				{
					if(tracks_ == MAX_TRACKS)
						return 0;
					else
					{
						trak* trak_atom = &traks_[tracks_];
						// trak_init(trak);
						trak_atom->parse( buffer, leaf_atom.size_ - atom::PREAMBLE_SIZE);
						++tracks_;
					}
				}
				buffer = leaf_atom.skip();
	}

	// build the indexing tables
	{
		unsigned int i;
		for(i = 0; i != tracks_; ++i)
		{
			traks_[i].trak_build_index();
		}
	}

	return 1;
}