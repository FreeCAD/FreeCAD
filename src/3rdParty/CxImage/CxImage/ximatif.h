/*
 * File:	ximatif.h
 * Purpose:	TIFF Image Class Loader and Writer
 */
/* ==========================================================
 * CxImageTIF (c) 07/Aug/2001 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 *
 * Special thanks to Troels Knakkergaard for new features, enhancements and bugfixes
 *
 * Special thanks to Abe <God(dot)bless(at)marihuana(dot)com> for MultiPageTIFF code.
 *
 * LibTIFF is:
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 * ==========================================================
 */

#if !defined(__ximatif_h)
#define __ximatif_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_TIF

#include "../tiff/tiffio.h"

class DLL_EXP CxImageTIF: public CxImage
{
public:
	CxImageTIF(): CxImage(CXIMAGE_FORMAT_TIF) {m_tif2=NULL; m_multipage=false; m_pages=0;}
	~CxImageTIF();

	TIFF* TIFFOpenEx(CxFile * hFile);
	void  TIFFCloseEx(TIFF* tif);

//	bool Load(const TCHAR * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_TIF);}
//	bool Save(const TCHAR * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_TIF);}
	bool Decode(CxFile * hFile);
	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile, bool bAppend=false);
	bool Encode(CxFile * hFile, CxImage ** pImages, int pagecount);
	bool Encode(FILE *hFile, bool bAppend=false) { CxIOFile file(hFile); return Encode(&file,bAppend); }
	bool Encode(FILE *hFile, CxImage ** pImages, int pagecount)
				{ CxIOFile file(hFile); return Encode(&file, pImages, pagecount); }
#endif // CXIMAGE_SUPPORT_ENCODE

protected:
	void TileToStrip(uint8* out, uint8* in,	uint32 rows, uint32 cols, int outskew, int inskew);
	bool EncodeBody(TIFF *m_tif, bool multipage=false, int page=0, int pagecount=0);
	TIFF *m_tif2;
	bool m_multipage;
	int  m_pages;
	void MoveBits( BYTE* dest, BYTE* from, int count, int bpp );
	void MoveBitsPal( BYTE* dest, BYTE*from, int count, int bpp, RGBQUAD* pal );
};

#endif

#endif
