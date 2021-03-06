// mp4_splitter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "mp4_splitter.h"

void show_usage( int argc, _TCHAR * argv[] )
{
	if( argc > 0 )
	{
		wcout << argv[0] << L" [input] [interval:second]" << endl;
	}
}

void split( const wstring & filename, size_t interval )
{
	try
	{
		mp4_splitter splitter( filename );
		splitter.split( interval );
	}
	catch (std::exception & e)
	{
		cout << "split failed. message: " << e.what() << endl;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	if ( argc < 3 )
	{
		show_usage( argc, argv );
		return 0;
	}

	wstring	filename = argv[1];
	size_t	interval = static_cast<size_t>(-1);

	try
	{
		interval = boost::lexical_cast<size_t>( wstring( argv[2] ) ) * 1000;
	}
	catch ( bad_cast & e )
	{
		cout << "split interval invalid. message: " << e.what() << endl;
	}

	split( filename, interval );

//	cin.get();

	return 0;
}

