#include "StdAfx.h"
#include "RawImage.h"
#include "Utils.h"

const TCHAR RawImage::TAG[12] = "RawImage";

RawImage::RawImage(void)
{
	_size      = 0;
	validSize  = 0;
	_width     = 0;
	_height    = 0;
	_version   = 0;
	_bpp       = 0;
	_redMask   = 0;
	_greenMask = 0;
	_blueMask  = 0;
	_data      = NULL;
}

RawImage::~RawImage(void)
{
	if (_data) {
		GlobalFree(_data);
	}
	_size      = 0;
	validSize  = 0;
	_width     = 0;
	_height    = 0;
	_version   = 0;
	_bpp       = 0;
	_redMask   = 0;
	_greenMask = 0;
	_blueMask  = 0;
}

/**
 * Returns the size in bytes of the header for a specific version of the
 * framebuffer adb protocol.
 * @param version the version of the protocol
 * @return the bytes of integers that makes up the header.
 */
int RawImage::getHeaderSize(int version)
{
	switch (version) {
	case 16: // compatibility mode
	case 24:
	case 32:
		return 12; // size, _width, _height
	case 1:
		return 48; // bpp, size, _width, _height, 4*(length, offset)
	}

	return 0;
}

/**
 * Reads the header of a RawImage from a {@link ByteBuffer}.
 * The way the _data is sent over adb is defined in system/core/adb/framebuffer_service.c
 * @param ver the version of the protocol.
 * @param buf the buffer to read from.
 * @return true if success
 */
BOOL RawImage::readHeader(int ver, ByteBuffer& buf)
{
	_version = ver;
	switch (ver) {
	case 1: {
			_bpp = buf.getInt();
			_size = buf.getInt();
			_width = buf.getInt();
			_height = buf.getInt();
			DWORD redOffset = buf.getInt();
			DWORD redLength = buf.getInt();
			DWORD blueOffset = buf.getInt();
			DWORD blueLength = buf.getInt();
			DWORD greenOffset = buf.getInt();
			DWORD greenLength = buf.getInt();
			DWORD alphaOffset = buf.getInt();
			DWORD alphaLength = buf.getInt();

			_redMask = ((1 << redLength) - 1) << redOffset;
			_greenMask = ((1 << greenLength) - 1) << greenOffset;
			_blueMask = ((1 << blueLength) - 1) << blueOffset;
		}
		break;

	case 16:
		// compatibility mode with original protocol
		_bpp = ver;
		_size = buf.getInt();
		_width = buf.getInt();
		_height = buf.getInt();
		// create color mask for default format, i.e 565
		_redMask = 0x001F;
		_greenMask = 0x07E0;
		_blueMask = 0xF800;
		break;

	case 24:
	case 32:
		// compatibility mode with original protocol
		_bpp = ver;
		_size = buf.getInt();
		_width = buf.getInt();
		_height = buf.getInt();
		// create color mask for default format, i.e 888
		_redMask = 0x0000FF;
		_greenMask = 0x00FF00;
		_blueMask = 0xFF0000;
		break;

	default:
		break;
	}

	LOG(TAG, "readHeader() ver=%d", ver);
	LOG(TAG, "readHeader() bpp=%d", _bpp);
	LOG(TAG, "readHeader() size=%d", _size);
	LOG(TAG, "readHeader() width=%d, height=%d", _width, _height);
	LOG(TAG, "readHeader() mask red=0x%08X, green=0x%08X, blue=0x%08X", _redMask, _greenMask, _blueMask);

	if (_size > 0) {
		// the version is supported.
		_data = (char*)GlobalAlloc(GPTR, _size);
		if (!_data) {
			Utils::err(TAG, "Failed to alloc _data buffer!");
			throw exception("Out of memory in RawImage::readHeader()!");
		}
	}

	LOG(TAG, "readHeader() data=0x%08X", _data);

	return _size > 0;
}

HBITMAP RawImage::getBitmap(HDC hdc)
{
	HBITMAP bitmap = NULL;

	if (!_data) {
		return bitmap;
	}

	switch (_bpp) {
	case 16:
	case 24:
	case 32: {
			LPBITMAPINFO bmpInfo = (LPBITMAPINFO)GlobalAlloc(GPTR, sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 2);
			if (!bmpInfo) break;

			bmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmpInfo->bmiHeader.biWidth = _width;
			bmpInfo->bmiHeader.biHeight = -((LONG)_height);
			bmpInfo->bmiHeader.biPlanes = 1;
			bmpInfo->bmiHeader.biBitCount = _bpp;
			bmpInfo->bmiHeader.biCompression = BI_BITFIELDS;
			bmpInfo->bmiHeader.biSizeImage = _size;
			*((DWORD*)&(bmpInfo->bmiColors[0])) = _redMask;
			*((DWORD*)&(bmpInfo->bmiColors[1])) = _greenMask;
			*((DWORD*)&(bmpInfo->bmiColors[2])) = _blueMask;

			bitmap = CreateDIBitmap(hdc, &(bmpInfo->bmiHeader), CBM_INIT, _data, bmpInfo, DIB_RGB_COLORS);

//			Utils::saveBitmap(bmpInfo, _data, _size);

			GlobalFree(bmpInfo);
		}
		break;
	}

	return bitmap;
}

DWORD RawImage::size(void)
{
	return _size;
}

DWORD RawImage::width(void)
{
	return _width;
}

DWORD RawImage::height(void)
{
	return _height;
}

char* RawImage::data(void)
{
	return _data;
}
