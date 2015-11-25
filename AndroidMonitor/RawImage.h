#pragma once

#include <Windows.h>
#include "ByteBuffer.h"

class RawImage
{
public:
	RawImage(void);
	~RawImage(void);

	BOOL       readHeader(int ver, ByteBuffer& buf);
	HBITMAP    getBitmap(HDC hdc);
	static int getHeaderSize(int version);
	DWORD      size(void);
	DWORD      width(void);
	DWORD      height(void);
	char*      data(void);

	DWORD validSize;

private:
	static const TCHAR TAG[12];
	DWORD              _version;
	DWORD              _bpp;
	DWORD              _size;
	DWORD              _width;
	DWORD              _height;
	DWORD              _redMask;
	DWORD              _greenMask;
	DWORD              _blueMask;
	char*              _data;
};
