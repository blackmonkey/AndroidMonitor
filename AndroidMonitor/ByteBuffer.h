#pragma once

#include <Windows.h>
#include <string>

using namespace std;

class ByteBuffer
{
public:
	ByteBuffer(int bytes);
	~ByteBuffer(void);
	char*        buf(void);
	int          capacity(void);   // capacity in bytes of buffer
	int          size(void);
	int          pos(void);
	void         reset(void);      // reset current pos
	void         seek(int offset); // seek current pos by offset
	void         setSize(int bytes);
	unsigned int getInt(void);
	string       getStr(int bytes);
	string       getLine(void);
	void         append(const char* src, int bytes);
	void         save(char* fname);

	// TODO: remove this constant
	static const int BUF_BLOCK = 307200;

private:
	static const char TAG[12];
	int               _capacity;
	int               _pos;
	int               _size; // size in bytes of valid data in buffer
	BYTE*             _buf;
};
