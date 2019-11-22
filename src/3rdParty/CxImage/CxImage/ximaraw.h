/*
 * File:	ximaraw.h
 * Purpose:	RAW Image Class Loader and Writer
 */
/* ==========================================================
 * CxImageRAW (c) May/2006 pdw63
 * For conditions of distribution and use, see copyright notice in ximage.h
 * Special thanks to David Coffin for dcraw without which this class would not exist
 *
 * libdcr (c) Dec/2007 Davide Pizzolato - www.xdp.it
 *
 * based on dcraw.c -- Dave Coffin's raw photo decoder
 * Copyright 1997-2007 by Dave Coffin, dcoffin a cybercom o net
 * ==========================================================
 */
#if !defined(__ximaRAW_h)
#define __ximaRAW_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_RAW

extern "C" {
 #include "../raw/libdcr.h"
}

class CxImageRAW: public CxImage
{

public:
	CxImageRAW(): CxImage(CXIMAGE_FORMAT_RAW) {}

//	bool Load(const char * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_ICO);}
//	bool Save(const char * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_ICO);}
	bool Decode(CxFile * hFile);
	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile);
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
#endif // CXIMAGE_SUPPORT_ENCODE

	enum CODEC_OPTION
	{
		DECODE_QUALITY_LIN = 0x00,
		DECODE_QUALITY_VNG = 0x01,
		DECODE_QUALITY_PPG = 0x02,
		DECODE_QUALITY_AHD = 0x03,
	}; 

protected:

	class CxFileRaw
	{
	public:
		CxFileRaw(CxFile* pFile,DCRAW *stream)
		{
			stream->obj_ = pFile;

			ras_stream_CxFile.read_ = raw_sfile_read;
			ras_stream_CxFile.write_ = raw_sfile_write;
			ras_stream_CxFile.seek_ = raw_sfile_seek;
			ras_stream_CxFile.close_ = raw_sfile_close;
			ras_stream_CxFile.gets_ = raw_sfile_gets;
			ras_stream_CxFile.eof_ = raw_sfile_eof;
			ras_stream_CxFile.tell_ = raw_sfile_tell;
			ras_stream_CxFile.getc_ = raw_sfile_getc;
			ras_stream_CxFile.scanf_ = raw_sfile_scanf;

			stream->ops_ = &ras_stream_CxFile;

		}

		static int raw_sfile_read(dcr_stream_obj *obj, void *buf, int size, int cnt)
		{	return ((CxFile*)obj)->Read(buf,size,cnt); }

		static int raw_sfile_write(dcr_stream_obj *obj, void *buf, int size, int cnt)
		{	return ((CxFile*)obj)->Write(buf,size,cnt); }

		static long raw_sfile_seek(dcr_stream_obj *obj, long offset, int origin)
		{	return ((CxFile*)obj)->Seek(offset,origin); }

		static int raw_sfile_close(dcr_stream_obj *obj)
		{	return 1; /*((CxFile*)obj)->Close();*/ }

		static char* raw_sfile_gets(dcr_stream_obj *obj, char *string, int n)
		{	return ((CxFile*)obj)->GetS(string,n); }

		static int   raw_sfile_eof(dcr_stream_obj *obj)
		{	return ((CxFile*)obj)->Eof(); }

		static long  raw_sfile_tell(dcr_stream_obj *obj)
		{	return ((CxFile*)obj)->Tell(); }

		static int   raw_sfile_getc(dcr_stream_obj *obj)
		{	return ((CxFile*)obj)->GetC(); }

		static int   raw_sfile_scanf(dcr_stream_obj *obj,const char *format, void* output)
		{	return ((CxFile*)obj)->Scanf(format, output); }

	private:
		dcr_stream_ops ras_stream_CxFile;
	};
};

#endif

#endif
