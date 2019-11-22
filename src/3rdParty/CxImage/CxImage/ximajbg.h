/*
 * File:	ximajbg.h
 * Purpose:	JBG Image Class Loader and Writer
 */
/* ==========================================================
 * CxImageJBG (c) 18/Aug/2002 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 *
 * based on LIBJBG Copyright (c) 2002, Markus Kuhn - All rights reserved.
 * ==========================================================
 */
#if !defined(__ximaJBG_h)
#define __ximaJBG_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_JBG

extern "C" {
#include "../jbig/jbig.h"
};

class CxImageJBG: public CxImage
{
public:
	CxImageJBG(): CxImage(CXIMAGE_FORMAT_JBG) {}

//	bool Load(const TCHAR * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_JBG);}
//	bool Save(const TCHAR * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_JBG);}
	bool Decode(CxFile * hFile);
	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile);
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
#endif // CXIMAGE_SUPPORT_ENCODE
protected:
	static void jbig_data_out(BYTE *buffer, unsigned int len, void *file)
							{((CxFile*)file)->Write(buffer,len,1);}
};

#endif

#endif
