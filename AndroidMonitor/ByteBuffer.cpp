#include "StdAfx.h"
#include "ByteBuffer.h"
#include "Utils.h"

const char ByteBuffer::TAG[12] = "ByteBuffer";

ByteBuffer::ByteBuffer(int bytes)
{
	_pos      = 0;
	_size     = 0;
	_capacity = 0;

	_buf      = (BYTE*)GlobalAlloc(GPTR, bytes);
	if (!_buf) {
		Utils::err(TAG, "GlobalAlloc(%d) failed", bytes);
		throw exception("Out of memory in ByteBuffer()!");
	}

	_capacity = bytes;
}

ByteBuffer::~ByteBuffer(void)
{
	if (_buf) {
		GlobalFree(_buf);
		_buf = NULL;
	}
	_capacity = 0;
	_size     = 0;
	_pos      = 0;
}

char* ByteBuffer::buf(void)
{
	return (char*)_buf;
}

int ByteBuffer::capacity(void)
{
	return _capacity;
}

unsigned int ByteBuffer::getInt(void)
{
	unsigned int res = 0;
	int offset = 0;

	while (_pos < _size && offset <= 24) {
		res |= _buf[_pos++] << offset;
		offset += 8;
	}

	return res;
}

string ByteBuffer::getStr(int bytes)
{
	string res;

	// bytes == 0 is meanless
	if (bytes > 0) {
		if (_pos + bytes > _size) {
			bytes = _size - _pos;
		}
		res.assign((char*)(_buf + _pos), bytes);
		_pos += bytes;
	} else if (bytes == -1) {
		// TODO: optimize with string::asign()
		while (_buf[_pos] != 0 && _pos < _size) {
			res += _buf[_pos++];
		}
		// assume '\0' is copied into res.
		if (_pos < _size) {
			_pos++;
		}
	}

	return res;
}

string ByteBuffer::getLine(void)
{
	string res;
	while (_buf[_pos] != 0 && _pos < _size) {
		res += _buf[_pos];
		if (_buf[_pos++] == '\n') {
			break;
		}
	}
	return res;
}

int ByteBuffer::size(void)
{
	return _size;
}

int ByteBuffer::pos(void)
{
	return _pos;
}

void ByteBuffer::reset(void)
{
	_pos = 0;
}

void ByteBuffer::seek(int offset)
{
	_pos += offset;
}

void ByteBuffer::setSize(int bytes)
{
	_size = bytes;
}

/**
 * Append specified data at current position
 */
void ByteBuffer::append(const char* src, int bytes)
{
	bytes = Utils::min(bytes, _capacity - _pos);
	if (bytes > 0) {
		CopyMemory(_buf + _pos, src, bytes);
		_pos += bytes;
		_size += bytes;
	}
}

void ByteBuffer::save(char* fname)
{
#ifdef _DEBUG
	// TODO: append prefix with date time

	HANDLE hf = CreateFile(fname, GENERIC_READ | GENERIC_WRITE,
		(DWORD) 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);
	if (hf == INVALID_HANDLE_VALUE) {
		return;
	}

	DWORD dwTmp;
	WriteFile(hf, _buf, _size, (LPDWORD)&dwTmp, NULL);

	// Close the .BMP file.
	CloseHandle(hf);
#endif
}
