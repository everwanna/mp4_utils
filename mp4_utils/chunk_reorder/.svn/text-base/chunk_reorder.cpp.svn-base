// chunk_reorder.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mp4_chunk_reorder.h"

void show_usage(int argc, _TCHAR* argv[])
{
	if( argc > 0 )
	{
		wcout << argv[0] << L" [in file] [out file]" << endl;
	}
}

void reorder( const wstring & in_file, const wstring & out_file )
{
	mp4_chunk_reorder reorder(in_file, out_file);

	try
	{
		// reorder.test();
		reorder.reorder();
	}
	catch ( std::exception& e )
	{
		cout << "reorder failed. exception: " << endl
			<< e.what() << endl;
	}
}


int _tmain(int argc, _TCHAR* argv[])
{
	if ( argc < 3 )
	{
		show_usage( argc, argv );
		return 0;
	}

	wstring in_file = argv[1];
	wstring out_file = argv[2];

	reorder( in_file, out_file );

	cin.get();

	return 0;
}

