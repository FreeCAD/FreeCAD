/*
 * File:	ximapcx.h
 * Purpose:	PCX Image Class Loader and Writer
 */
/* ==========================================================
 * CxImagePCX (c) 05/Jan/2002 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 *
 * Parts of the code come from Paintlib: Copyright (c) 1996-1998 Ulrich von Zadow
 * ==========================================================
 */
#if !defined(__ximaPCX_h)
#define __ximaPCX_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_PCX

class CxImagePCX: public CxImage
{
// PCX Image File
#pragma pack(1)
typedef struct tagPCXHEADER
{
  char Manufacturer;	// always 0X0A
  char Version;			// version number
  char Encoding;		// always 1
  char BitsPerPixel;	// color bits
  WORD Xmin, Ymin;		// image origin
  WORD Xmax, Ymax;		// image dimensions
  WORD Hres, Vres;		// resolution values
  BYTE ColorMap[16][3];	// color palette
  char Reserved;
  char ColorPlanes;		// color planes
  WORD BytesPerLine;	// line buffer size
  WORD PaletteType;		// grey or color palette
  char Filter[58];
} PCXHEADER;
#pragma pack()

public:
	CxImagePCX(): CxImage(CXIMAGE_FORMAT_PCX) {}

//	bool Load(const TCHAR * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_PCX);}
//	bool Save(const TCHAR * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_PCX);}
	bool Decode(CxFile * hFile);
	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile);
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
#endif // CXIMAGE_SUPPORT_ENCODE
protected:
	bool PCX_PlanesToPixels(BYTE * pixels, BYTE * bitplanes, short bytesperline, short planes, short bitsperpixel);
	bool PCX_UnpackPixels(BYTE * pixels, BYTE * bitplanes, short bytesperline, short planes, short bitsperpixel);
	void PCX_PackPixels(const long p,BYTE &c, BYTE &n, CxFile &f);
	void PCX_PackPlanes(BYTE* buff, const long size, CxFile &f);
	void PCX_PixelsToPlanes(BYTE* raw, long width, BYTE* buf, long plane);
	void PCX_toh(PCXHEADER* p);
};

#endif

#endif
