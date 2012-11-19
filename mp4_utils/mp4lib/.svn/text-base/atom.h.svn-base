#pragma once

class atom
{
public:
	atom() : size_(0), start_(0), end_(0)
	{
		memset( type_, 0, sizeof(type_) );
	}

	virtual ~atom(void);

public:
	string get_type() {
		return string( reinterpret_cast<char*>(type_), 4 );
	}

	bool is_type( const string type ) {
		return get_type() == type;
	}

	void print()
	{
		cout << "atom(" << get_type() << "," << size_ << ")" << endl;
	}
	static unsigned int header_size(unsigned char* atom_bytes)
	{
		return (atom_bytes[0] << 24) +
			(atom_bytes[1] << 16) +
			(atom_bytes[2] << 8) +
			(atom_bytes[3]);
	}

	unsigned char* skip() const
	{
		return end_;
	}

	
	unsigned char* read_header(unsigned char* buffer);

	void atom_write_header(unsigned char* outbuffer);

public:
	unsigned char type_[4];
	unsigned int size_;
	unsigned char* start_;
	unsigned char* end_;

	const static size_t PREAMBLE_SIZE = 8;
};
