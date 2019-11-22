/*
 * File:	ximajas.h
 * Purpose:	Jasper Image Class Loader and Writer
 */
/* ==========================================================
 * CxImageJAS (c) 12/Apr/2003 Davide Pizzolato - www.xdp.it
 * For conditions of distribution and use, see copyright notice in ximage.h
 *
 * based on JasPer Copyright (c) 2001-2003 Michael David Adams - All rights reserved.
 * ==========================================================
 */
#if !defined(__ximaJAS_h)
#define __ximaJAS_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_JASPER

#include "../jasper/include/jasper/jasper.h"

class CxImageJAS: public CxImage
{
public:
	CxImageJAS(): CxImage((DWORD)0) {}	// <vho> cast to DWORD

//	bool Load(const TCHAR * imageFileName){ return CxImage::Load(imageFileName,0);}
//	bool Save(const TCHAR * imageFileName){ return CxImage::Save(imageFileName,0);}
	bool Decode(CxFile * hFile, DWORD imagetype = 0);
	bool Decode(FILE *hFile, DWORD imagetype = 0) { CxIOFile file(hFile); return Decode(&file,imagetype); }

#if CXIMAGE_SUPPORT_ENCODE
	bool Encode(CxFile * hFile, DWORD imagetype = 0);
	bool Encode(FILE *hFile, DWORD imagetype = 0) { CxIOFile file(hFile); return Encode(&file,imagetype); }
#endif // CXIMAGE_SUPPORT_ENCODE
protected:

	class CxFileJas
	{
	public:
		CxFileJas(CxFile* pFile,jas_stream_t *stream)
		{
			if (stream->obj_) jas_free(stream->obj_);
			stream->obj_ = pFile;

			// <vho> - cannot set the stream->ops_->functions here,
			// because this overwrites a static structure in the Jasper library.
			// This structure is used by Jasper for internal operations too, e.g. tempfile.
			// However the ops_ pointer in the stream can be overwritten.

			//stream->ops_->close_ = JasClose;
			//stream->ops_->read_  = JasRead;
			//stream->ops_->seek_  = JasSeek;
			//stream->ops_->write_ = JasWrite;

			jas_stream_CxFile.close_ = JasClose;
			jas_stream_CxFile.read_  = JasRead;
			jas_stream_CxFile.seek_  = JasSeek;
			jas_stream_CxFile.write_ = JasWrite;

			stream->ops_ = &jas_stream_CxFile;

			// <vho> - end
		}
		static int JasRead(jas_stream_obj_t *obj, char *buf, int cnt)
		{		return ((CxFile*)obj)->Read(buf,1,cnt); }
		static int JasWrite(jas_stream_obj_t *obj, char *buf, int cnt)
		{		return ((CxFile*)obj)->Write(buf,1,cnt); }
		static long JasSeek(jas_stream_obj_t *obj, long offset, int origin)
		{		return ((CxFile*)obj)->Seek(offset,origin); }
		static int JasClose(jas_stream_obj_t * /*obj*/)
		{		return 1; }

	// <vho>
private:
		jas_stream_ops_t jas_stream_CxFile;
	// <vho> - end

	};

};

#endif

#endif
