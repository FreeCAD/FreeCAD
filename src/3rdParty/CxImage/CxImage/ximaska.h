/*
 * File:	ximaska.h
 * Purpose:	SKA Image Class Loader and Writer
 */
/* ==========================================================
 * CxImageSKA (c) 25/Sep/2007 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 * ==========================================================
 */
#if !defined(__ximaSKA_h)
#define __ximaSKA_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_SKA

class CxImageSKA: public CxImage
{
#pragma pack(1)
	typedef struct tagSkaHeader {
    unsigned short  Width;
    unsigned short  Height;
    BYTE  BppExp;
    DWORD dwUnknown;
} SKAHEADER;
#pragma pack()

public:
	CxImageSKA(): CxImage(CXIMAGE_FORMAT_SKA) {}

//	bool Load(const char * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_ICO);}
//	bool Save(const char * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_ICO);}
	bool Decode(CxFile * hFile);
	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile);
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
#endif // CXIMAGE_SUPPORT_ENCODE
};

#endif

#endif
