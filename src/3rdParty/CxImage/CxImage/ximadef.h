#if !defined(__ximadefs_h)
#define __ximadefs_h

#include "ximacfg.h"

#if defined(_AFXDLL)||defined(_USRDLL)
 #define DLL_EXP __declspec(dllexport)
#elif defined(_MSC_VER)&&(_MSC_VER<1200)
 #define DLL_EXP __declspec(dllimport)
#else
 #define DLL_EXP
#endif


#if CXIMAGE_SUPPORT_EXCEPTION_HANDLING
  #define cx_try try
  #define cx_throw(message) throw(message)
  #define cx_catch catch (const char *message)
#else
  #define cx_try bool cx_error=false;
  #define cx_throw(message) {cx_error=true; if(strcmp(message,"")) strncpy(info.szLastError,message,255); goto cx_error_catch;}
  #define cx_catch cx_error_catch: char message[]=""; if(cx_error)
#endif


#if CXIMAGE_SUPPORT_JP2 || CXIMAGE_SUPPORT_JPC || CXIMAGE_SUPPORT_PGX || CXIMAGE_SUPPORT_PNM || CXIMAGE_SUPPORT_RAS
 #define CXIMAGE_SUPPORT_JASPER 1
#else
 #define CXIMAGE_SUPPORT_JASPER 0
#endif

#if CXIMAGE_SUPPORT_DSP
#undef CXIMAGE_SUPPORT_TRANSFORMATION
 #define CXIMAGE_SUPPORT_TRANSFORMATION 1
#endif

#if CXIMAGE_SUPPORT_TRANSFORMATION || CXIMAGE_SUPPORT_TIF || CXIMAGE_SUPPORT_TGA || CXIMAGE_SUPPORT_BMP || CXIMAGE_SUPPORT_WINDOWS
 #define CXIMAGE_SUPPORT_BASICTRANSFORMATIONS 1
#endif

#if CXIMAGE_SUPPORT_DSP || CXIMAGE_SUPPORT_TRANSFORMATION
#undef CXIMAGE_SUPPORT_INTERPOLATION
 #define CXIMAGE_SUPPORT_INTERPOLATION 1
#endif

#if defined (_WIN32_WCE)
 #undef CXIMAGE_SUPPORT_WMF
 #define CXIMAGE_SUPPORT_WMF 0
#endif

#if !defined(WIN32) && !defined(_WIN32_WCE)
 #undef CXIMAGE_SUPPORT_WINDOWS
 #define CXIMAGE_SUPPORT_WINDOWS 0
#endif

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef PI
 #define PI 3.141592653589793f
#endif


#if defined(WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#include <tchar.h>
#endif

#include <stdio.h>
#include <math.h>

#ifdef __BORLANDC__

#ifndef _COMPLEX_DEFINED

typedef struct tagcomplex {
	double x,y;
} _complex;

#endif

#define _cabs(c) sqrt(c.x*c.x+c.y*c.y)

#endif


#if !defined(WIN32) && !defined(_WIN32_WCE)

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned int   UINT;

typedef DWORD          COLORREF;
typedef unsigned int   HANDLE;
typedef void*          HRGN;

#ifndef BOOL
#define	BOOL bool
#endif

#ifndef TRUE
#define	TRUE true
#endif

#ifndef FALSE
#define	FALSE false
#endif

#ifndef TCHAR
#define TCHAR char
#define _T
#endif

typedef struct tagRECT
{
	long    left;
	long    top;
	long    right;
	long    bottom;
} RECT;

typedef struct tagPOINT
{
	long  x;
	long  y;
} POINT;

typedef struct tagRGBQUAD {
	BYTE    rgbBlue;
	BYTE    rgbGreen;
	BYTE    rgbRed;
	BYTE    rgbReserved;
} RGBQUAD;

#pragma pack(1)

typedef struct tagBITMAPINFOHEADER{
	DWORD      biSize;
	long       biWidth;
	long       biHeight;
	WORD       biPlanes;
	WORD       biBitCount;
	DWORD      biCompression;
	DWORD      biSizeImage;
	long       biXPelsPerMeter;
	long       biYPelsPerMeter;
	DWORD      biClrUsed;
	DWORD      biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagBITMAPFILEHEADER {
	WORD    bfType;
	DWORD   bfSize;
	WORD    bfReserved1;
	WORD    bfReserved2;
	DWORD   bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPCOREHEADER {
	DWORD   bcSize;
	WORD    bcWidth;
	WORD    bcHeight;
	WORD    bcPlanes;
	WORD    bcBitCount;
} BITMAPCOREHEADER;

typedef struct tagRGBTRIPLE {
	BYTE    rgbtBlue;
	BYTE    rgbtGreen;
	BYTE    rgbtRed;
} RGBTRIPLE;

#pragma pack()

#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L

#define GetRValue(rgb)      ((BYTE)(rgb))
#define GetGValue(rgb)      ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)      ((BYTE)((rgb)>>16))
#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#ifndef _COMPLEX_DEFINED

typedef struct tagcomplex {
	double x,y;
} _complex;

#endif

#define _cabs(c) sqrt(c.x*c.x+c.y*c.y)

#endif

#endif //__ximadefs
