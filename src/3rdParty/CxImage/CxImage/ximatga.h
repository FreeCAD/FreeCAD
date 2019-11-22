/*
 * File:	ximatga.h
 * Purpose:	TARGA Image Class Loader and Writer
 */
/* ==========================================================
 * CxImageTGA (c) 05/Jan/2002 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 *
 * Parts of the code come from Paintlib : Copyright (c) 1996-1998 Ulrich von Zadow
 * ==========================================================
 */
#if !defined(__ximaTGA_h)
#define __ximaTGA_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_TGA

class CxImageTGA: public CxImage
{
#pragma pack(1)
typedef struct tagTgaHeader
{
    BYTE   IdLength;            // Image ID Field Length
    BYTE   CmapType;            // Color Map Type
    BYTE   ImageType;           // Image Type

    WORD   CmapIndex;           // First Entry Index
    WORD   CmapLength;          // Color Map Length
    BYTE   CmapEntrySize;       // Color Map Entry Size

    WORD   X_Origin;            // X-origin of Image
    WORD   Y_Origin;            // Y-origin of Image
    WORD   ImageWidth;          // Image Width
    WORD   ImageHeight;         // Image Height
    BYTE   PixelDepth;          // Pixel Depth
    BYTE   ImagDesc;            // Image Descriptor
} TGAHEADER;
#pragma pack()

public:
	CxImageTGA(): CxImage(CXIMAGE_FORMAT_TGA) {}

//	bool Load(const TCHAR * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_TGA);}
//	bool Save(const TCHAR * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_TGA);}
	bool Decode(CxFile * hFile);
	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile);
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
#endif // CXIMAGE_SUPPORT_ENCODE
protected:
	BYTE ExpandCompressedLine(BYTE* pDest,TGAHEADER* ptgaHead,CxFile *hFile,int width, int y, BYTE rleLeftover);
	void ExpandUncompressedLine(BYTE* pDest,TGAHEADER* ptgaHead,CxFile *hFile,int width, int y, int xoffset);
	void tga_toh(TGAHEADER* p);
};

#endif

#endif
