#pragma once
#include <stdexcept>

class io_exception :
	public std::exception
{
public:
	io_exception(void);

	io_exception( const char *const& msg )
		: std::exception(msg)
	{
	}
	~io_exception(void);
};
