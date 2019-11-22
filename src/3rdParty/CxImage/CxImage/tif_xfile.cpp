/*
 * TIFF file IO, using CxFile.
 */

#ifdef WIN32
 #include <windows.h>
#endif
#include <stdio.h>

#include "ximage.h"

#if CXIMAGE_SUPPORT_TIF

#include "../tiff/tiffiop.h"

#include "xfile.h"

static tsize_t 
_tiffReadProcEx(thandle_t fd, tdata_t buf, tsize_t size)
{
	return (tsize_t)((CxFile*)fd)->Read(buf, 1, size);
}

static tsize_t
_tiffWriteProcEx(thandle_t fd, tdata_t buf, tsize_t size)
{
	return (tsize_t)((CxFile*)fd)->Write(buf, 1, size);
}

static toff_t
_tiffSeekProcEx(thandle_t fd, toff_t off, int whence)
{
	if ( off == 0xFFFFFFFF ) 
		return 0xFFFFFFFF;
	if (!((CxFile*)fd)->Seek(off, whence))
		return 0xFFFFFFFF;
	if (whence == SEEK_SET)
		return off;

	return (toff_t)((CxFile*)fd)->Tell();
}

// Return nonzero if error
static int
_tiffCloseProcEx(thandle_t /*fd*/)
{
//	return !((CxFile*)fd)->Close(); // "//" needed for memory files <DP>
	return 0;
}

#include <sys/stat.h>

static toff_t
_tiffSizeProcEx(thandle_t fd)
{
	return ((CxFile*)fd)->Size();
}

static int
_tiffMapProcEx(thandle_t /*fd*/, tdata_t* /*pbase*/, toff_t* /*psize*/)
{
	return (0);
}

static void
_tiffUnmapProcEx(thandle_t /*fd*/, tdata_t /*base*/, toff_t /*size*/)
{
}

// Open a TIFF file descriptor for read/writing.
/*
TIFF*
TIFFOpen(const char* name, const char* mode)
{
	static const char module[] = "TIFFOpen";
   FILE* stream = fopen(name, mode);
	if (stream == NULL) 
   {
		TIFFError(module, "%s: Cannot open", name);
		return NULL;
	}
	return (TIFFFdOpen((int)stream, name, mode));
}
*/

TIFF*
_TIFFFdOpen(void* fd, const char* name, const char* mode)
{
	TIFF* tif;

	tif = TIFFClientOpen(name, mode,
	    (thandle_t) fd,
	    _tiffReadProcEx, _tiffWriteProcEx, _tiffSeekProcEx, _tiffCloseProcEx,
	    _tiffSizeProcEx, _tiffMapProcEx, _tiffUnmapProcEx);
	if (tif)
		tif->tif_fd = fd;
	return (tif);
}

extern "C" TIFF* _TIFFOpenEx(CxFile* stream, const char* mode)
{
	return (_TIFFFdOpen(stream, "TIFF IMAGE", mode));
}

#ifdef __GNUC__
extern	char* malloc();
extern	char* realloc();
#else
#include <malloc.h>
#endif

tdata_t
_TIFFmalloc(tsize_t s)
{
	return (malloc((size_t) s));
}

void
_TIFFfree(tdata_t p)
{
	free(p);
}

tdata_t
_TIFFrealloc(tdata_t p, tsize_t s)
{
	return (realloc(p, (size_t) s));
}

void
_TIFFmemset(tdata_t p, int v, tsize_t c)
{
	memset(p, v, (size_t) c);
}

void
_TIFFmemcpy(tdata_t d, const tdata_t s, tsize_t c)
{
	memcpy(d, s, (size_t) c);
}

int
_TIFFmemcmp(const tdata_t p1, const tdata_t p2, tsize_t c)
{
	return (memcmp(p1, p2, (size_t) c));
}

#ifndef UNICODE
#define DbgPrint wvsprintf
#define DbgPrint2 wsprintf
#define DbgMsgBox MessageBox
#else
#define DbgPrint wvsprintfA
#define DbgPrint2 wsprintfA
#define DbgMsgBox MessageBoxA
#endif

static void
Win32WarningHandler(const char* module, const char* fmt, va_list ap)
{
#ifdef _DEBUG
#if (!defined(_CONSOLE) && !defined(_WIN32_WCE) && defined(WIN32))
	LPSTR szTitle;
	LPSTR szTmp;
	LPCSTR szTitleText = "%s Warning";
	LPCSTR szDefaultModule = "TIFFLIB";
	szTmp = (module == NULL) ? (LPSTR)szDefaultModule : (LPSTR)module;
	if ((szTitle = (LPSTR)LocalAlloc(LMEM_FIXED, (strlen(szTmp) +
			strlen(szTitleText) + strlen(fmt) + 128))) == NULL)
		return;
	DbgPrint2(szTitle, szTitleText, szTmp);
	szTmp = szTitle + (strlen(szTitle)+2);
	DbgPrint(szTmp, fmt, ap);
	DbgMsgBox(GetFocus(), szTmp, szTitle, MB_OK | MB_ICONINFORMATION);
	LocalFree(szTitle);
	return;
#else
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	fprintf(stderr, "Warning, ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
#endif
#endif
}
TIFFErrorHandler _TIFFwarningHandler = Win32WarningHandler;

static void
Win32ErrorHandler(const char* module, const char* fmt, va_list ap)
{
#ifdef _DEBUG
#if (!defined(_CONSOLE) && !defined(_WIN32_WCE) && defined(WIN32))
	LPSTR szTitle;
	LPSTR szTmp;
	LPCSTR szTitleText = "%s Error";
	LPCSTR szDefaultModule = "TIFFLIB";
	szTmp = (module == NULL) ? (LPSTR)szDefaultModule : (LPSTR)module;
	if ((szTitle = (LPSTR)LocalAlloc(LMEM_FIXED, (strlen(szTmp) +
			strlen(szTitleText) + strlen(fmt) + 128))) == NULL)
		return;
	DbgPrint2(szTitle, szTitleText, szTmp);
	szTmp = szTitle + (strlen(szTitle)+2);
	DbgPrint(szTmp, fmt, ap);
	DbgMsgBox(GetFocus(), szTmp, szTitle, MB_OK | MB_ICONEXCLAMATION);
	LocalFree(szTitle);
	return;
#else
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
#endif
#endif
}
TIFFErrorHandler _TIFFerrorHandler = Win32ErrorHandler;

#endif

