/*
 * File:	ximamng.h
 * Purpose:	Declaration of the MNG Image Class
 * Author:	Davide Pizzolato - www.xdp.it
 * Created:	2001
 */
/* ==========================================================
 * CxImageMNG (c) 07/Aug/2001 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 *
 * Special thanks to Frank Haug <f.haug(at)jdm(dot)de> for suggestions and code.
 *
 * original mng.cpp code created by Nikolaus Brennig, November 14th, 2000. <virtualnik(at)nol(dot)at>
 *
 * LIBMNG Copyright (c) 2000,2001 Gerard Juyn (gerard@libmng.com)
 * ==========================================================
 */

#if !defined(__ximaMNG_h)
#define __ximaMNG_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_MNG

//#define MNG_NO_CMS
#define MNG_SUPPORT_DISPLAY
#define MNG_SUPPORT_READ
#define	MNG_SUPPORT_WRITE
#define MNG_ACCESS_CHUNKS
#define MNG_STORE_CHUNKS

extern "C" {
#include "../mng/libmng.h"
#include "../mng/libmng_data.h"
#include "../mng/libmng_error.h"
}

//unsigned long _stdcall RunMNGThread(void *lpParam);

typedef struct tagmngstuff 
{
	CxFile		*file;
	BYTE		*image;
	BYTE		*alpha;
	HANDLE		thread;
	mng_uint32	delay;
	mng_uint32  width;
	mng_uint32  height;
	mng_uint32  effwdt;
	mng_int16	bpp;
	mng_bool	animation;
	mng_bool	animation_enabled;
	float		speed;
	long		nBkgndIndex;
	RGBQUAD		nBkgndColor;
} mngstuff;

class CxImageMNG: public CxImage
{
public:
	CxImageMNG();
	~CxImageMNG();

	bool Load(const TCHAR * imageFileName);

	bool Decode(CxFile * hFile);
	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile);
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
	bool Save(const TCHAR * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_MNG);}
#endif // CXIMAGE_SUPPORT_ENCODE

	long Resume();
	void SetSpeed(float speed);
	
	mng_handle hmng;
	mngstuff mnginfo;
protected:
	void WritePNG(mng_handle hMNG, int Frame, int FrameCount );
	void SetCallbacks(mng_handle mng);
};

#endif

#endif
