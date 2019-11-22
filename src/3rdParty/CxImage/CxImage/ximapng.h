/*
 * File:	ximapng.h
 * Purpose:	PNG Image Class Loader and Writer
 */
/* ==========================================================
 * CxImagePNG (c) 07/Aug/2001 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 *
 * Special thanks to Troels Knakkergaard for new features, enhancements and bugfixes
 *
 * original CImagePNG  and CImageIterator implementation are:
 * Copyright:	(c) 1995, Alejandro Aguilar Sierra <asierra(at)servidor(dot)unam(dot)mx>
 *
 * libpng Copyright (c) 1998-2003 Glenn Randers-Pehrson
 * ==========================================================
 */
#if !defined(__ximaPNG_h)
#define __ximaPNG_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_PNG

extern "C" {
#include "../png/png.h"
}

class CxImagePNG: public CxImage
{
public:
	CxImagePNG(): CxImage(CXIMAGE_FORMAT_PNG) {}

//	bool Load(const TCHAR * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_PNG);}
//	bool Save(const TCHAR * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_PNG);}
	bool Decode(CxFile * hFile);
	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile);
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
#endif // CXIMAGE_SUPPORT_ENCODE

protected:
	void ima_png_error(png_struct *png_ptr, char *message);
	void expand2to4bpp(BYTE* prow);

	static void PNGAPI user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
	{
		CxFile* hFile = (CxFile*)png_get_io_ptr(png_ptr);
		if (hFile == NULL || hFile->Read(data,1,length) != length) png_error(png_ptr, "Read Error");
	}

	static void PNGAPI user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
	{
		CxFile* hFile = (CxFile*)png_get_io_ptr(png_ptr);
		if (hFile == NULL || hFile->Write(data,1,length) != length) png_error(png_ptr, "Write Error");
	}

	static void PNGAPI user_flush_data(png_structp png_ptr)
	{
		CxFile* hFile = (CxFile*)png_get_io_ptr(png_ptr);
		if (hFile == NULL || !hFile->Flush()) png_error(png_ptr, "Flush Error");
	}

    static void PNGAPI user_error_fn(png_structp png_ptr,png_const_charp error_msg)
	{
		strncpy((char*)png_ptr->error_ptr,error_msg,255);
		longjmp(png_ptr->jmpbuf, 1);
	}
};

#endif

#endif
