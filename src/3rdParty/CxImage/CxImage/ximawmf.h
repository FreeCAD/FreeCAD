/*
*********************************************************************
 * File:	ximawmf.h
 * Purpose:	Windows Metafile Class Loader and Writer
 * Author:	Volker Horch - vhorch@gmx.de
 * created:	13-Jun-2002
*********************************************************************
 */

/*
*********************************************************************
	Notes by Author:
*********************************************************************

	Limitations:
	============

	a) Transparency:

	A Metafile is vector graphics, which has transparency by design.
	This class always converts into	a Bitmap format. Transparency is
	supported, but there is no good way to find out, which parts
	of the Metafile are transparent. There are two ways how we can
	handle this:

	- Clear the Background of the Bitmap with the background color
	  you like (i have used COLOR_WINDOW) and don't support transparency.

	  below #define XMF_SUPPORT_TRANSPARENCY 0
			#define XMF_COLOR_BACK RGB(Background color you like)

	- Clear the Background of the Bitmap with a very unusual color
	  (which one ?) and use this color as the transparent color

	  below #define XMF_SUPPORT_TRANSPARENCY 1
			#define	XMF_COLOR_TRANSPARENT_R ...
			#define	XMF_COLOR_TRANSPARENT_G	...
			#define	XMF_COLOR_TRANSPARENT_B	...

	b) Resolution

	Once we have converted the Metafile into a Bitmap and we zoom in
	or out, the image may not look very good. If we still had the
	original Metafile, zooming would produce good results always.

	c) Size

	Although the filesize of a Metafile may be very small, it might
	produce a Bitmap with a bombastic size. Assume you have a Metafile
	with an image size of 6000*4000, which contains just one Metafile
	record ((e.g. a line from (0,0) to (6000, 4000)). The filesize
	of this Metafile would be let's say 100kB. If we convert it to
	a 6000*4000 Bitmap with 24 Bits/Pixes, the Bitmap would consume
	about 68MB of memory.

	I have choosen, to limit the size of the Bitmap to max.
	screensize, to avoid memory problems.

	If you want something else,
	modify #define XMF_MAXSIZE_CX / XMF_MAXSIZE_CY below

*********************************************************************
*/

#ifndef _XIMAWMF_H
#define _XIMAWMF_H

#include "ximage.h"

#if CXIMAGE_SUPPORT_WMF && CXIMAGE_SUPPORT_WINDOWS

class CxImageWMF: public CxImage
{

#pragma pack(1)

typedef struct tagRECT16
{
	short int	left;
	short int	top;
	short int	right;
	short int	bottom;
} RECT16;

// taken from Windos 3.11 SDK Documentation (Programmer's Reference Volume 4: Resources)
typedef struct tagMETAFILEHEADER
{
	DWORD	key;		// always 0x9ac6cdd7
	WORD	reserved1;	// reserved = 0
	RECT16	bbox;		// bounding rectangle in metafile units as defined in "inch"
	WORD	inch;		// number of metafile units per inch (should be < 1440)
	DWORD	reserved2;	// reserved = 0
	WORD	checksum;	// sum of the first 10 WORDS (using XOR operator)
} METAFILEHEADER;

#pragma pack()

public:
	CxImageWMF(): CxImage(CXIMAGE_FORMAT_WMF) { }

	bool Decode(CxFile * hFile, long nForceWidth=0, long nForceHeight=0);
	bool Decode(FILE *hFile, long nForceWidth=0, long nForceHeight=0)
			{ CxIOFile file(hFile); return Decode(&file,nForceWidth,nForceHeight); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile);
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
#endif // CXIMAGE_SUPPORT_ENCODE

protected:
	void ShrinkMetafile(int &cx, int &cy);
	BOOL CheckMetafileHeader(METAFILEHEADER *pmetafileheader);
	HENHMETAFILE ConvertWmfFiletoEmf(CxFile *pFile, METAFILEHEADER *pmetafileheader);
	HENHMETAFILE ConvertEmfFiletoEmf(CxFile *pFile, ENHMETAHEADER *pemfh);

};

#define	METAFILEKEY	0x9ac6cdd7L

// Background color definition (if no transparency). see Notes above
#define	XMF_COLOR_BACK	GetSysColor(COLOR_WINDOW)
// alternatives
//#define	XMF_COLOR_BACK	RGB(192, 192, 192)	// lite gray
//#define	XMF_COLOR_BACK	RGB(  0,   0,   0)	// black
//#define	XMF_COLOR_BACK	RGB(255, 255, 255)	// white


// transparency support. see Notes above
#define	XMF_SUPPORT_TRANSPARENCY	0
#define	XMF_COLOR_TRANSPARENT_R		211
#define	XMF_COLOR_TRANSPARENT_G		121
#define	XMF_COLOR_TRANSPARENT_B		112
// don't change
#define	XMF_COLOR_TRANSPARENT		RGB (XMF_COLOR_TRANSPARENT_R, \
										 XMF_COLOR_TRANSPARENT_G, \
										 XMF_COLOR_TRANSPARENT_B)
// don't change
#define	XMF_RGBQUAD_TRANSPARENT		XMF_COLOR_TRANSPARENT_B, \
									XMF_COLOR_TRANSPARENT_G, \
									XMF_COLOR_TRANSPARENT_R, \
									0
// max. size. see Notes above
// alternatives
//#define	XMF_MAXSIZE_CX	(GetSystemMetrics(SM_CXSCREEN)-10)
//#define	XMF_MAXSIZE_CY	(GetSystemMetrics(SM_CYSCREEN)-50)
//#define	XMF_MAXSIZE_CX	(2*GetSystemMetrics(SM_CXSCREEN)/3)
//#define	XMF_MAXSIZE_CY	(2*GetSystemMetrics(SM_CYSCREEN)/3)
#define	XMF_MAXSIZE_CX	4000
#define	XMF_MAXSIZE_CY	4000


#endif

#endif
