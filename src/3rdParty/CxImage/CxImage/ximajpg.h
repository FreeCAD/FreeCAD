/*
 * File:	ximajpg.h
 * Purpose:	JPG Image Class Loader and Writer
 */
/* ==========================================================
 * CxImageJPG (c) 07/Aug/2001 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 *
 * Special thanks to Troels Knakkergaard for new features, enhancements and bugfixes
 *
 * Special thanks to Chris Shearer Cooper for CxFileJpg tips & code
 *
 * EXIF support based on jhead-1.8 by Matthias Wandel <mwandel(at)rim(dot)net>
 *
 * original CImageJPG  and CImageIterator implementation are:
 * Copyright:	(c) 1995, Alejandro Aguilar Sierra <asierra(at)servidor(dot)unam(dot)mx>
 *
 * This software is based in part on the work of the Independent JPEG Group.
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * ==========================================================
 */
#if !defined(__ximaJPEG_h)
#define __ximaJPEG_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_JPG

#define CXIMAGEJPG_SUPPORT_EXIF 1

extern "C" {
 #include "../jpeg/jpeglib.h"
 #include "../jpeg/jerror.h"
}

class DLL_EXP CxImageJPG: public CxImage
{
public:
	CxImageJPG();
	~CxImageJPG();

//	bool Load(const TCHAR * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_JPG);}
//	bool Save(const TCHAR * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_JPG);}
	bool Decode(CxFile * hFile);
	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile);
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
#endif // CXIMAGE_SUPPORT_ENCODE

/*
 * EXIF support based on jhead-1.8 by Matthias Wandel <mwandel(at)rim(dot)net>
 */

#if CXIMAGEJPG_SUPPORT_EXIF

#define MAX_COMMENT 1000
#define MAX_SECTIONS 20

typedef struct tag_ExifInfo {
	char  Version      [5];
    char  CameraMake   [32];
    char  CameraModel  [40];
    char  DateTime     [20];
    int   Height, Width;
    int   Orientation;
    int   IsColor;
    int   Process;
    int   FlashUsed;
    float FocalLength;
    float ExposureTime;
    float ApertureFNumber;
    float Distance;
    float CCDWidth;
    float ExposureBias;
    int   Whitebalance;
    int   MeteringMode;
    int   ExposureProgram;
    int   ISOequivalent;
    int   CompressionLevel;
	float FocalplaneXRes;
	float FocalplaneYRes;
	float FocalplaneUnits;
	float Xresolution;
	float Yresolution;
	float ResolutionUnit;
	float Brightness;
    char  Comments[MAX_COMMENT];

    unsigned char * ThumbnailPointer;  /* Pointer at the thumbnail */
    unsigned ThumbnailSize;     /* Size of thumbnail. */

	bool  IsExif;
} EXIFINFO;

//--------------------------------------------------------------------------
// JPEG markers consist of one or more 0xFF bytes, followed by a marker
// code byte (which is not an FF).  Here are the marker codes of interest
// in this program.  (See jdmarker.c for a more complete list.)
//--------------------------------------------------------------------------

#define M_SOF0  0xC0            // Start Of Frame N
#define M_SOF1  0xC1            // N indicates which compression process
#define M_SOF2  0xC2            // Only SOF0-SOF2 are now in common use
#define M_SOF3  0xC3
#define M_SOF5  0xC5            // NB: codes C4 and CC are NOT SOF markers
#define M_SOF6  0xC6
#define M_SOF7  0xC7
#define M_SOF9  0xC9
#define M_SOF10 0xCA
#define M_SOF11 0xCB
#define M_SOF13 0xCD
#define M_SOF14 0xCE
#define M_SOF15 0xCF
#define M_SOI   0xD8            // Start Of Image (beginning of datastream)
#define M_EOI   0xD9            // End Of Image (end of datastream)
#define M_SOS   0xDA            // Start Of Scan (begins compressed data)
#define M_JFIF  0xE0            // Jfif marker
#define M_EXIF  0xE1            // Exif marker
#define M_COM   0xFE            // COMment 

#define PSEUDO_IMAGE_MARKER 0x123; // Extra value.

#define EXIF_READ_EXIF  0x01
#define EXIF_READ_IMAGE 0x02
#define EXIF_READ_ALL   0x03

class DLL_EXP CxExifInfo
{

typedef struct tag_Section_t{
    BYTE*    Data;
    int      Type;
    unsigned Size;
} Section_t;

public:
	EXIFINFO* m_exifinfo;
	char m_szLastError[256];
	CxExifInfo(EXIFINFO* info = NULL);
	~CxExifInfo();
	bool DecodeExif(CxFile * hFile, int nReadMode = EXIF_READ_EXIF);
	bool EncodeExif(CxFile * hFile);
	void DiscardAllButExif();
protected:
	bool process_EXIF(unsigned char * CharBuf, unsigned int length);
	void process_COM (const BYTE * Data, int length);
	void process_SOFn (const BYTE * Data, int marker);
	int Get16u(void * Short);
	int Get16m(void * Short);
	long Get32s(void * Long);
	unsigned long Get32u(void * Long);
	double ConvertAnyFormat(void * ValuePtr, int Format);
	void* FindSection(int SectionType);
	bool ProcessExifDir(unsigned char * DirStart, unsigned char * OffsetBase, unsigned ExifLength,
                           EXIFINFO * const pInfo, unsigned char ** const LastExifRefdP, int NestingLevel=0);
	int ExifImageWidth;
	int MotorolaOrder;
	Section_t Sections[MAX_SECTIONS];
	int SectionsRead;
	bool freeinfo;
};

	CxExifInfo* m_exif;
	EXIFINFO m_exifinfo;
	bool DecodeExif(CxFile * hFile);
	bool DecodeExif(FILE * hFile) { CxIOFile file(hFile); return DecodeExif(&file); }

#endif //CXIMAGEJPG_SUPPORT_EXIF

////////////////////////////////////////////////////////////////////////////////////////
//////////////////////        C x F i l e J p g         ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

// thanks to Chris Shearer Cooper <cscooper(at)frii(dot)com>
class CxFileJpg : public jpeg_destination_mgr, public jpeg_source_mgr
	{
public:
	enum { eBufSize = 4096 };

	CxFileJpg(CxFile* pFile)
	{
        m_pFile = pFile;

		init_destination = InitDestination;
		empty_output_buffer = EmptyOutputBuffer;
		term_destination = TermDestination;

		init_source = InitSource;
		fill_input_buffer = FillInputBuffer;
		skip_input_data = SkipInputData;
		resync_to_restart = jpeg_resync_to_restart; // use default method
		term_source = TermSource;
		next_input_byte = NULL; //* => next byte to read from buffer 
		bytes_in_buffer = 0;	//* # of bytes remaining in buffer 

		m_pBuffer = new unsigned char[eBufSize];
	}
	~CxFileJpg()
	{
		delete [] m_pBuffer;
	}

	static void InitDestination(j_compress_ptr cinfo)
	{
		CxFileJpg* pDest = (CxFileJpg*)cinfo->dest;
		pDest->next_output_byte = pDest->m_pBuffer;
		pDest->free_in_buffer = eBufSize;
	}

	static boolean EmptyOutputBuffer(j_compress_ptr cinfo)
	{
		CxFileJpg* pDest = (CxFileJpg*)cinfo->dest;
		if (pDest->m_pFile->Write(pDest->m_pBuffer,1,eBufSize)!=(size_t)eBufSize)
			ERREXIT(cinfo, JERR_FILE_WRITE);
		pDest->next_output_byte = pDest->m_pBuffer;
		pDest->free_in_buffer = eBufSize;
		return TRUE;
	}

	static void TermDestination(j_compress_ptr cinfo)
	{
		CxFileJpg* pDest = (CxFileJpg*)cinfo->dest;
		size_t datacount = eBufSize - pDest->free_in_buffer;
		/* Write any data remaining in the buffer */
		if (datacount > 0) {
			if (!pDest->m_pFile->Write(pDest->m_pBuffer,1,datacount))
				ERREXIT(cinfo, JERR_FILE_WRITE);
		}
		pDest->m_pFile->Flush();
		/* Make sure we wrote the output file OK */
		if (pDest->m_pFile->Error()) ERREXIT(cinfo, JERR_FILE_WRITE);
		return;
	}

	static void InitSource(j_decompress_ptr cinfo)
	{
		CxFileJpg* pSource = (CxFileJpg*)cinfo->src;
		pSource->m_bStartOfFile = TRUE;
	}

	static boolean FillInputBuffer(j_decompress_ptr cinfo)
	{
		size_t nbytes;
		CxFileJpg* pSource = (CxFileJpg*)cinfo->src;
		nbytes = pSource->m_pFile->Read(pSource->m_pBuffer,1,eBufSize);
		if (nbytes <= 0){
			if (pSource->m_bStartOfFile)	//* Treat empty input file as fatal error 
				ERREXIT(cinfo, JERR_INPUT_EMPTY);
			WARNMS(cinfo, JWRN_JPEG_EOF);
			// Insert a fake EOI marker 
			pSource->m_pBuffer[0] = (JOCTET) 0xFF;
			pSource->m_pBuffer[1] = (JOCTET) JPEG_EOI;
			nbytes = 2;
		}
		pSource->next_input_byte = pSource->m_pBuffer;
		pSource->bytes_in_buffer = nbytes;
		pSource->m_bStartOfFile = FALSE;
		return TRUE;
	}

	static void SkipInputData(j_decompress_ptr cinfo, long num_bytes)
	{
		CxFileJpg* pSource = (CxFileJpg*)cinfo->src;
		if (num_bytes > 0){
			while (num_bytes > (long)pSource->bytes_in_buffer){
				num_bytes -= (long)pSource->bytes_in_buffer;
				FillInputBuffer(cinfo);
				// note we assume that fill_input_buffer will never return FALSE,
				// so suspension need not be handled.
			}
			pSource->next_input_byte += (size_t) num_bytes;
			pSource->bytes_in_buffer -= (size_t) num_bytes;
		}
	}

	static void TermSource(j_decompress_ptr /*cinfo*/)
	{
		return;
	}
protected:
    CxFile  *m_pFile;
	unsigned char *m_pBuffer;
	bool m_bStartOfFile;
};

public:
	enum CODEC_OPTION
	{
		ENCODE_BASELINE = 0x1,
		ENCODE_ARITHMETIC = 0x2,
		ENCODE_GRAYSCALE = 0x4,
		ENCODE_OPTIMIZE = 0x8,
		ENCODE_PROGRESSIVE = 0x10,
		ENCODE_LOSSLESS = 0x20,
		ENCODE_SMOOTHING = 0x40,
		DECODE_GRAYSCALE = 0x80,
		DECODE_QUANTIZE = 0x100,
		DECODE_DITHER = 0x200,
		DECODE_ONEPASS = 0x400,
		DECODE_NOSMOOTH = 0x800,
		ENCODE_SUBSAMPLE_422 = 0x1000,
		ENCODE_SUBSAMPLE_444 = 0x2000
	}; 

	int m_nPredictor;
	int m_nPointTransform;
	int m_nSmoothing;
	int m_nQuantize;
	J_DITHER_MODE m_nDither;

};

#endif

#endif
