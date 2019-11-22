/*
 * File:	ximagif.h
 * Purpose:	GIF Image Class Loader and Writer
 */
/* ==========================================================
 * CxImageGIF (c) 07/Aug/2001 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 *
 * Special thanks to Troels Knakkergaard for new features, enhancements and bugfixes
 *
 * original CImageGIF  and CImageIterator implementation are:
 * Copyright:	(c) 1995, Alejandro Aguilar Sierra <asierra(at)servidor(dot)unam(dot)mx>
 *
 * 6/15/97 Randy Spann: Added GIF87a writing support
 *         R.Spann@ConnRiver.net
 *
 * DECODE.C - An LZW decoder for GIF
 * Copyright (C) 1987, by Steven A. Bennett
 * Copyright (C) 1994, C++ version by Alejandro Aguilar Sierra
 *
 * In accordance with the above, I want to credit Steve Wilhite who wrote
 * the code which this is heavily inspired by...
 *
 * GIF and 'Graphics Interchange Format' are trademarks (tm) of
 * Compuserve, Incorporated, an H&R Block Company.
 *
 * Release Notes: This file contains a decoder routine for GIF images
 * which is similar, structurally, to the original routine by Steve Wilhite.
 * It is, however, somewhat noticably faster in most cases.
 *
 * ==========================================================
 */

#if !defined(__ximaGIF_h)
#define __ximaGIF_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_GIF

typedef short int       code_int;   

/* Various error codes used by decoder */
#define OUT_OF_MEMORY -10
#define BAD_CODE_SIZE -20
#define READ_ERROR -1
#define WRITE_ERROR -2
#define OPEN_ERROR -3
#define CREATE_ERROR -4
#define MAX_CODES   4095
#define GIFBUFTAM 16383
#define TRANSPARENCY_CODE 0xF9

//LZW GIF Image compression
#define MAXBITSCODES    12
#define HSIZE  5003     /* 80% occupancy */
#define MAXCODE(n_bits) (((code_int) 1 << (n_bits)) - 1)
#define HashTabOf(i)    htab[i]
#define CodeTabOf(i)    codetab[i]


class CImageIterator;
class DLL_EXP CxImageGIF: public CxImage
{
#pragma pack(1)

typedef struct tag_gifgce{
  BYTE flags; /*res:3|dispmeth:3|userinputflag:1|transpcolflag:1*/
  WORD delaytime;
  BYTE transpcolindex;
} struct_gifgce;

typedef struct tag_dscgif{		/* Logic Screen Descriptor  */
  char header[6];				/* Firma and version */
  WORD scrwidth;
  WORD scrheight;
  char pflds;
  char bcindx;
  char pxasrat;
} struct_dscgif;

typedef struct tag_image{      /* Image Descriptor */
  WORD l;
  WORD t;
  WORD w;
  WORD h;
  BYTE   pf;
} struct_image;

typedef struct tag_TabCol{		/* Tabla de colores */
  short colres;					/* color resolution */
  short sogct;					/* size of global color table */
  rgb_color paleta[256];		/* paleta */
} struct_TabCol;

typedef struct tag_RLE{
	int rl_pixel;
	int rl_basecode;
	int rl_count;
	int rl_table_pixel;
	int rl_table_max;
	int just_cleared;
	int out_bits;
	int out_bits_init;
	int out_count;
	int out_bump;
	int out_bump_init;
	int out_clear;
	int out_clear_init;
	int max_ocodes;
	int code_clear;
	int code_eof;
	unsigned int obuf;
	int obits;
	unsigned char oblock[256];
	int oblen;
} struct_RLE;
#pragma pack()

public:
	CxImageGIF(): CxImage(CXIMAGE_FORMAT_GIF) {m_loops=0; info.dispmeth=0; m_comment[0]='\0';}

//	bool Load(const TCHAR * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_GIF);}
//	bool Save(const TCHAR * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_GIF);}
	
	bool Decode(CxFile * fp);
	bool Decode(FILE *fp) { CxIOFile file(fp); return Decode(&file); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * fp);
	bool Encode(CxFile * fp, CxImage ** pImages, int pagecount, bool bLocalColorMap = false, bool bLocalDispMeth = false);
	bool Encode(FILE *fp) { CxIOFile file(fp); return Encode(&file); }
	bool Encode(FILE *fp, CxImage ** pImages, int pagecount, bool bLocalColorMap = false)
				{ CxIOFile file(fp); return Encode(&file, pImages, pagecount, bLocalColorMap); }
#endif // CXIMAGE_SUPPORT_ENCODE

	void SetLoops(int loops);
	long GetLoops();
	void SetComment(const char* sz_comment_in);
	void GetComment(char* sz_comment_out);

protected:
	bool DecodeExtension(CxFile *fp);
	void EncodeHeader(CxFile *fp);
	void EncodeLoopExtension(CxFile *fp);
	void EncodeExtension(CxFile *fp);
	void EncodeBody(CxFile *fp, bool bLocalColorMap = false);
	void EncodeComment(CxFile *fp);
	bool EncodeRGB(CxFile *fp);
	void GifMix(CxImage & imgsrc2, struct_image & imgdesc);
	
	struct_gifgce gifgce;

	int             curx, cury;
	long             CountDown;
	unsigned long    cur_accum;
	int              cur_bits;
	int interlaced, iypos, istep, iheight, ipass;
	int ibf;
	int ibfmax;
	BYTE buf[GIFBUFTAM + 1];
// Implementation
	int GifNextPixel ();
	void Putword (int w, CxFile* fp );
	void compressNONE (int init_bits, CxFile* outfile);
	void compressLZW (int init_bits, CxFile* outfile);
	void output (code_int code );
	void cl_hash (long hsize);
	void char_out (int c);
	void flush_char ();
	short init_exp(short size);
	short get_next_code(CxFile*);
	short decoder(CxFile*, CImageIterator* iter, short linewidth, int &bad_code_count);
	int get_byte(CxFile*);
	int out_line(CImageIterator* iter, unsigned char *pixels, int linelen);
	int get_num_frames(CxFile *f,struct_TabCol* TabColSrc,struct_dscgif* dscgif);
	long seek_next_image(CxFile* fp, long position);

	short curr_size;                     /* The current code size */
	short clear;                         /* Value for a clear code */
	short ending;                        /* Value for a ending code */
	short newcodes;                      /* First available code */
	short top_slot;                      /* Highest code for current size */
	short slot;                          /* Last read code */

	/* The following static variables are used
	* for seperating out codes */
	short navail_bytes;              /* # bytes left in block */
	short nbits_left;                /* # bits left in current BYTE */
	BYTE b1;                           /* Current BYTE */
	BYTE byte_buff[257];               /* Current block */
	BYTE *pbytes;                      /* Pointer to next BYTE in block */
	/* The reason we have these seperated like this instead of using
	* a structure like the original Wilhite code did, is because this
	* stuff generally produces significantly faster code when compiled...
	* This code is full of similar speedups...  (For a good book on writing
	* C for speed or for space optomisation, see Efficient C by Tom Plum,
	* published by Plum-Hall Associates...)
	*/
	BYTE stack[MAX_CODES + 1];            /* Stack for storing pixels */
	BYTE suffix[MAX_CODES + 1];           /* Suffix table */
	WORD prefix[MAX_CODES + 1];           /* Prefix linked list */

//LZW GIF Image compression routines
	long htab [HSIZE];
	unsigned short codetab [HSIZE];
	int n_bits;				/* number of bits/code */
	code_int maxcode;		/* maximum code, given n_bits */
	code_int free_ent;		/* first unused entry */
	int clear_flg;
	int g_init_bits;
	CxFile* g_outfile;
	int ClearCode;
	int EOFCode;

	int a_count;
	char accum[256];

	char m_comment[256];
	int m_loops;

//RLE compression routines
	void compressRLE( int init_bits, CxFile* outfile);
	void rle_clear(struct_RLE* rle);
	void rle_flush(struct_RLE* rle);
	void rle_flush_withtable(int count, struct_RLE* rle);
	void rle_flush_clearorrep(int count, struct_RLE* rle);
	void rle_flush_fromclear(int count,struct_RLE* rle);
	void rle_output_plain(int c,struct_RLE* rle);
	void rle_reset_out_clear(struct_RLE* rle);
	unsigned int rle_compute_triangle_count(unsigned int count, unsigned int nrepcodes);
	unsigned int rle_isqrt(unsigned int x);
	void rle_write_block(struct_RLE* rle);
	void rle_block_out(unsigned char c, struct_RLE* rle);
	void rle_block_flush(struct_RLE* rle);
	void rle_output(int val, struct_RLE* rle);
	void rle_output_flush(struct_RLE* rle);
};

#endif

#endif
