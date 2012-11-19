#pragma once

class box
{
public:
	box() : size_(0), start_(0), end_(0)
	{
		memset( type_, 0, sizeof(type_) );
	}

public:
	string get_type() {
		return string( reinterpret_cast<char*>(type_), 4 );
	}

public:
	byte		type_[4];
	uint32_t	size_;
	uint32_t	start_;
	uint32_t	end_;
};

