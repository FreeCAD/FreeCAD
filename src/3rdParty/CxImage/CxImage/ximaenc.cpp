// xImaCodec.cpp : Encode Decode functions
/* 07/08/2001 v1.00 - Davide Pizzolato - www.xdp.it
 * CxImage version 6.0.0 02/Feb/2008
 */

#include "ximage.h"

#if CXIMAGE_SUPPORT_JPG
#include "ximajpg.h"
#endif

#if CXIMAGE_SUPPORT_GIF
#include "ximagif.h"
#endif

#if CXIMAGE_SUPPORT_PNG
#include "ximapng.h"
#endif

#if CXIMAGE_SUPPORT_MNG
#include "ximamng.h"
#endif

#if CXIMAGE_SUPPORT_BMP
#include "ximabmp.h"
#endif

#if CXIMAGE_SUPPORT_ICO
#include "ximaico.h"
#endif

#if CXIMAGE_SUPPORT_TIF
#include "ximatif.h"
#endif

#if CXIMAGE_SUPPORT_TGA
#include "ximatga.h"
#endif

#if CXIMAGE_SUPPORT_PCX
#include "ximapcx.h"
#endif

#if CXIMAGE_SUPPORT_WBMP
#include "ximawbmp.h"
#endif

#if CXIMAGE_SUPPORT_WMF
#include "ximawmf.h" // <vho> - WMF/EMF support
#endif

#if CXIMAGE_SUPPORT_JBG
#include "ximajbg.h"
#endif

#if CXIMAGE_SUPPORT_JASPER
#include "ximajas.h"
#endif

#if CXIMAGE_SUPPORT_SKA
#include "ximaska.h"
#endif

#if CXIMAGE_SUPPORT_RAW
#include "ximaraw.h"
#endif

////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
bool CxImage::EncodeSafeCheck(CxFile *hFile)
{
	if (hFile==NULL) {
		strcpy(info.szLastError,CXIMAGE_ERR_NOFILE);
		return true;
	}

	if (pDib==NULL){
		strcpy(info.szLastError,CXIMAGE_ERR_NOIMAGE);
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
//#ifdef WIN32
//bool CxImage::Save(LPCWSTR filename, DWORD imagetype)
//{
//	FILE* hFile;	//file handle to write the image
//	if ((hFile=_wfopen(filename,L"wb"))==NULL)  return false;
//	bool bOK = Encode(hFile,imagetype);
//	fclose(hFile);
//	return bOK;
//}
//#endif //WIN32
////////////////////////////////////////////////////////////////////////////////
// For UNICODE support: char -> TCHAR
/**
 * Saves to disk the image in a specific format.
 * \param filename: file name
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 * \return true if everything is ok
 */
bool CxImage::Save(const TCHAR * filename, DWORD imagetype)
{
	FILE* hFile;	//file handle to write the image

#ifdef WIN32
	if ((hFile=_tfopen(filename,_T("wb")))==NULL)  return false;	// For UNICODE support
#else
	if ((hFile=fopen(filename,"wb"))==NULL)  return false;
#endif

	bool bOK = Encode(hFile,imagetype);
	fclose(hFile);
	return bOK;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Saves to disk the image in a specific format.
 * \param hFile: file handle, open and enabled for writing.
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 * \return true if everything is ok
 */
bool CxImage::Encode(FILE *hFile, DWORD imagetype)
{
	CxIOFile file(hFile);
	return Encode(&file,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Saves to memory buffer the image in a specific format.
 * \param buffer: output memory buffer pointer. Must be NULL,
 * the function allocates and fill the memory,
 * the application must free the buffer, see also FreeMemory().
 * \param size: output memory buffer size.
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 * \return true if everything is ok
 */
bool CxImage::Encode(BYTE * &buffer, long &size, DWORD imagetype)
{
	if (buffer!=NULL){
		strcpy(info.szLastError,"the buffer must be empty");
		return false;
	}
	CxMemFile file;
	file.Open();
	if(Encode(&file,imagetype)){
		buffer=file.GetBuffer();
		size=file.Size();
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Saves to disk the image in a specific format.
 * \param hFile: file handle (CxMemFile or CxIOFile), with write access.
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 * \return true if everything is ok
 * \sa ENUM_CXIMAGE_FORMATS
 */
bool CxImage::Encode(CxFile *hFile, DWORD imagetype)
{

#if CXIMAGE_SUPPORT_BMP

	if (imagetype==CXIMAGE_FORMAT_BMP){
		CxImageBMP newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_ICO
	if (imagetype==CXIMAGE_FORMAT_ICO){
		CxImageICO newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_TIF
	if (imagetype==CXIMAGE_FORMAT_TIF){
		CxImageTIF newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JPG
	if (imagetype==CXIMAGE_FORMAT_JPG){
		CxImageJPG newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_GIF
	if (imagetype==CXIMAGE_FORMAT_GIF){
		CxImageGIF newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_PNG
	if (imagetype==CXIMAGE_FORMAT_PNG){
		CxImagePNG newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_MNG
	if (imagetype==CXIMAGE_FORMAT_MNG){
		CxImageMNG newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_TGA
	if (imagetype==CXIMAGE_FORMAT_TGA){
		CxImageTGA newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_PCX
	if (imagetype==CXIMAGE_FORMAT_PCX){
		CxImagePCX newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_WBMP
	if (imagetype==CXIMAGE_FORMAT_WBMP){
		CxImageWBMP newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_WMF && CXIMAGE_SUPPORT_WINDOWS // <vho> - WMF/EMF support
	if (imagetype==CXIMAGE_FORMAT_WMF){
		CxImageWMF newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JBG
	if (imagetype==CXIMAGE_FORMAT_JBG){
		CxImageJBG newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JASPER
	if (
 #if	CXIMAGE_SUPPORT_JP2
		imagetype==CXIMAGE_FORMAT_JP2 || 
 #endif
 #if	CXIMAGE_SUPPORT_JPC
		imagetype==CXIMAGE_FORMAT_JPC || 
 #endif
 #if	CXIMAGE_SUPPORT_PGX
		imagetype==CXIMAGE_FORMAT_PGX || 
 #endif
 #if	CXIMAGE_SUPPORT_PNM
		imagetype==CXIMAGE_FORMAT_PNM || 
 #endif
 #if	CXIMAGE_SUPPORT_RAS
		imagetype==CXIMAGE_FORMAT_RAS || 
 #endif
		 false ){
		CxImageJAS newima;
		newima.Ghost(this);
		if (newima.Encode(hFile,imagetype)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif

#if CXIMAGE_SUPPORT_SKA
	if (imagetype==CXIMAGE_FORMAT_SKA){
		CxImageSKA newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif

#if CXIMAGE_SUPPORT_RAW
	if (imagetype==CXIMAGE_FORMAT_RAW){
		CxImageRAW newima;
		newima.Ghost(this);
		if (newima.Encode(hFile)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif

	strcpy(info.szLastError,"Encode: Unknown format");
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Saves to disk or memory pagecount images, referenced by an array of CxImage pointers.
 * \param hFile: file handle.
 * \param pImages: array of CxImage pointers.
 * \param pagecount: number of images.
 * \param imagetype: can be CXIMAGE_FORMAT_TIF or CXIMAGE_FORMAT_GIF.
 * \return true if everything is ok
 */
bool CxImage::Encode(FILE * hFile, CxImage ** pImages, int pagecount, DWORD imagetype)
{
	CxIOFile file(hFile);
	return Encode(&file, pImages, pagecount,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Saves to disk or memory pagecount images, referenced by an array of CxImage pointers.
 * \param hFile: file handle (CxMemFile or CxIOFile), with write access.
 * \param pImages: array of CxImage pointers.
 * \param pagecount: number of images.
 * \param imagetype: can be CXIMAGE_FORMAT_TIF, CXIMAGE_FORMAT_GIF or CXIMAGE_FORMAT_ICO.
 * \return true if everything is ok
 */
bool CxImage::Encode(CxFile * hFile, CxImage ** pImages, int pagecount, DWORD imagetype)
{
#if CXIMAGE_SUPPORT_TIF
	if (imagetype==CXIMAGE_FORMAT_TIF){
		CxImageTIF newima;
		newima.Ghost(this);
		if (newima.Encode(hFile,pImages,pagecount)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_GIF
	if (imagetype==CXIMAGE_FORMAT_GIF){
		CxImageGIF newima;
		newima.Ghost(this);
		if (newima.Encode(hFile,pImages,pagecount)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_ICO
	if (imagetype==CXIMAGE_FORMAT_ICO){
		CxImageICO newima;
		newima.Ghost(this);
		if (newima.Encode(hFile,pImages,pagecount)){
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
	strcpy(info.szLastError,"Multipage Encode, Unsupported operation for this format");
	return false;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * exports the image into a RGBA buffer, Useful for OpenGL applications.
 * \param buffer: output memory buffer pointer. Must be NULL,
 * the function allocates and fill the memory,
 * the application must free the buffer, see also FreeMemory().
 * \param size: output memory buffer size.
 * \param bFlipY: direction of Y axis. default = false.
 * \return true if everything is ok
 */
bool CxImage::Encode2RGBA(BYTE * &buffer, long &size, bool bFlipY)
{
	if (buffer!=NULL){
		strcpy(info.szLastError,"the buffer must be empty");
		return false;
	}
	CxMemFile file;
	file.Open();
	if(Encode2RGBA(&file,bFlipY)){
		buffer=file.GetBuffer();
		size=file.Size();
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * exports the image into a RGBA buffer, Useful for OpenGL applications.
 * \param hFile: file handle (CxMemFile or CxIOFile), with write access.
 * \param bFlipY: direction of Y axis. default = false.
 * \return true if everything is ok
 */
bool CxImage::Encode2RGBA(CxFile *hFile, bool bFlipY)
{
	if (EncodeSafeCheck(hFile)) return false;

	for (long y1 = 0; y1 < head.biHeight; y1++) {
		long y = bFlipY ? head.biHeight - 1 - y1 : y1;
		for(long x = 0; x < head.biWidth; x++) {
			RGBQUAD color = BlindGetPixelColor(x,y);
			hFile->PutC(color.rgbRed);
			hFile->PutC(color.rgbGreen);
			hFile->PutC(color.rgbBlue);
			hFile->PutC(color.rgbReserved);
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
// For UNICODE support: char -> TCHAR
/**
 * Reads from disk the image in a specific format.
 * - If decoding fails using the specified image format,
 * the function will try the automatic file format recognition.
 *
 * \param filename: file name
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 * \return true if everything is ok
 */
bool CxImage::Load(const TCHAR * filename, DWORD imagetype)
//bool CxImage::Load(const char * filename, DWORD imagetype)
{
	/*FILE* hFile;	//file handle to read the image
	if ((hFile=fopen(filename,"rb"))==NULL)  return false;
	bool bOK = Decode(hFile,imagetype);
	fclose(hFile);*/

	/* automatic file type recognition */
	bool bOK = false;
	if ( GetTypeIndexFromId(imagetype) ){
		FILE* hFile;	//file handle to read the image

#ifdef WIN32
		if ((hFile=_tfopen(filename,_T("rb")))==NULL)  return false;	// For UNICODE support
#else
		if ((hFile=fopen(filename,"rb"))==NULL)  return false;
#endif

		bOK = Decode(hFile,imagetype);
		fclose(hFile);
		if (bOK) return bOK;
	}

	char szError[256];
	strcpy(szError,info.szLastError); //save the first error

	// if failed, try automatic recognition of the file...
	FILE* hFile;

#ifdef WIN32
	if ((hFile=_tfopen(filename,_T("rb")))==NULL)  return false;	// For UNICODE support
#else
	if ((hFile=fopen(filename,"rb"))==NULL)  return false;
#endif

	bOK = Decode(hFile,CXIMAGE_FORMAT_UNKNOWN);
	fclose(hFile);

	if (!bOK && imagetype > 0) strcpy(info.szLastError,szError); //restore the first error

	return bOK;
}
////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
//bool CxImage::Load(LPCWSTR filename, DWORD imagetype)
//{
//	/*FILE* hFile;	//file handle to read the image
//	if ((hFile=_wfopen(filename, L"rb"))==NULL)  return false;
//	bool bOK = Decode(hFile,imagetype);
//	fclose(hFile);*/
//
//	/* automatic file type recognition */
//	bool bOK = false;
//	if ( GetTypeIndexFromId(imagetype) ){
//		FILE* hFile;	//file handle to read the image
//		if ((hFile=_wfopen(filename,L"rb"))==NULL)  return false;
//		bOK = Decode(hFile,imagetype);
//		fclose(hFile);
//		if (bOK) return bOK;
//	}
//
//	char szError[256];
//	strcpy(szError,info.szLastError); //save the first error
//
//	// if failed, try automatic recognition of the file...
//	FILE* hFile;	//file handle to read the image
//	if ((hFile=_wfopen(filename,L"rb"))==NULL)  return false;
//	bOK = Decode(hFile,CXIMAGE_FORMAT_UNKNOWN);
//	fclose(hFile);
//
//	if (!bOK && imagetype > 0) strcpy(info.szLastError,szError); //restore the first error
//
//	return bOK;
//}
////////////////////////////////////////////////////////////////////////////////
/**
 * Loads an image from the application resources.
 * \param hRes: the resource handle returned by FindResource().
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS.
 * \param hModule: NULL for internal resource, or external application/DLL hinstance returned by LoadLibray.
 * \return true if everything is ok
 */
bool CxImage::LoadResource(HRSRC hRes, DWORD imagetype, HMODULE hModule)
{
	DWORD rsize=SizeofResource(hModule,hRes);
	HGLOBAL hMem=::LoadResource(hModule,hRes);
	if (hMem){
		char* lpVoid=(char*)LockResource(hMem);
		if (lpVoid){
			// FILE* fTmp=tmpfile(); doesn't work with network
			/*char tmpPath[MAX_PATH] = {0};
			char tmpFile[MAX_PATH] = {0};
			GetTempPath(MAX_PATH,tmpPath);
			GetTempFileName(tmpPath,"IMG",0,tmpFile);
			FILE* fTmp=fopen(tmpFile,"w+b");
			if (fTmp){
				fwrite(lpVoid,rsize,1,fTmp);
				fseek(fTmp,0,SEEK_SET);
				bool bOK = Decode(fTmp,imagetype);
				fclose(fTmp);
				DeleteFile(tmpFile);
				return bOK;
			}*/

			CxMemFile fTmp((BYTE*)lpVoid,rsize);
			return Decode(&fTmp,imagetype);
		}
	} else strcpy(info.szLastError,"Unable to load resource!");
	return false;
}
#endif //WIN32
////////////////////////////////////////////////////////////////////////////////
/**
 * Constructor from file name, see Load()
 * \param filename: file name
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 */
// 
// > filename: file name
// > imagetype: specify the image format (CXIMAGE_FORMAT_BMP,...)
// For UNICODE support: char -> TCHAR
CxImage::CxImage(const TCHAR * filename, DWORD imagetype)
//CxImage::CxImage(const char * filename, DWORD imagetype)
{
	Startup(imagetype);
	Load(filename,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Constructor from file handle, see Decode()
 * \param stream: file handle, with read access.
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 */
CxImage::CxImage(FILE * stream, DWORD imagetype)
{
	Startup(imagetype);
	Decode(stream,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Constructor from CxFile object, see Decode()
 * \param stream: file handle (CxMemFile or CxIOFile), with read access.
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 */
CxImage::CxImage(CxFile * stream, DWORD imagetype)
{
	Startup(imagetype);
	Decode(stream,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Constructor from memory buffer, see Decode()
 * \param buffer: memory buffer
 * \param size: size of buffer
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 */
CxImage::CxImage(BYTE * buffer, DWORD size, DWORD imagetype)
{
	Startup(imagetype);
	CxMemFile stream(buffer,size);
	Decode(&stream,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Loads an image from memory buffer
 * \param buffer: memory buffer
 * \param size: size of buffer
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 * \return true if everything is ok
 */
bool CxImage::Decode(BYTE * buffer, DWORD size, DWORD imagetype)
{
	CxMemFile file(buffer,size);
	return Decode(&file,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Loads an image from file handle.
 * \param hFile: file handle, with read access.
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 * \return true if everything is ok
 */
bool CxImage::Decode(FILE *hFile, DWORD imagetype)
{
	CxIOFile file(hFile);
	return Decode(&file,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Loads an image from CxFile object
 * \param hFile: file handle (CxMemFile or CxIOFile), with read access.
 * \param imagetype: file format, see ENUM_CXIMAGE_FORMATS
 * \return true if everything is ok
 * \sa ENUM_CXIMAGE_FORMATS
 */
bool CxImage::Decode(CxFile *hFile, DWORD imagetype)
{
	if (hFile == NULL){
		strcpy(info.szLastError,CXIMAGE_ERR_NOFILE);
		return false;
	}

	if (imagetype==CXIMAGE_FORMAT_UNKNOWN){
		DWORD pos = hFile->Tell();
#if CXIMAGE_SUPPORT_BMP
		{ CxImageBMP newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_JPG
		{ CxImageJPG newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_ICO
		{ CxImageICO newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_GIF
		{ CxImageGIF newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_PNG
		{ CxImagePNG newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_TIF
		{ CxImageTIF newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_MNG
		{ CxImageMNG newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_TGA
		{ CxImageTGA newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_PCX
		{ CxImagePCX newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_WBMP
		{ CxImageWBMP newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_WMF && CXIMAGE_SUPPORT_WINDOWS
		{ CxImageWMF newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_JBG
		{ CxImageJBG newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_JASPER
		{ CxImageJAS newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_SKA
		{ CxImageSKA newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
#if CXIMAGE_SUPPORT_RAW
		{ CxImageRAW newima; newima.CopyInfo(*this); if (newima.Decode(hFile)) { Transfer(newima); return true; } else hFile->Seek(pos,SEEK_SET); }
#endif
	}

#if CXIMAGE_SUPPORT_BMP
	if (imagetype==CXIMAGE_FORMAT_BMP){
		CxImageBMP newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JPG
	if (imagetype==CXIMAGE_FORMAT_JPG){
		CxImageJPG newima;
		newima.CopyInfo(*this); // <ignacio>
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_ICO
	if (imagetype==CXIMAGE_FORMAT_ICO){
		CxImageICO newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			info.nNumFrames = newima.info.nNumFrames;
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_GIF
	if (imagetype==CXIMAGE_FORMAT_GIF){
		CxImageGIF newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			info.nNumFrames = newima.info.nNumFrames;
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_PNG
	if (imagetype==CXIMAGE_FORMAT_PNG){
		CxImagePNG newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_TIF
	if (imagetype==CXIMAGE_FORMAT_TIF){
		CxImageTIF newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			info.nNumFrames = newima.info.nNumFrames;
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_MNG
	if (imagetype==CXIMAGE_FORMAT_MNG){
		CxImageMNG newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			info.nNumFrames = newima.info.nNumFrames;
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_TGA
	if (imagetype==CXIMAGE_FORMAT_TGA){
		CxImageTGA newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_PCX
	if (imagetype==CXIMAGE_FORMAT_PCX){
		CxImagePCX newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_WBMP
	if (imagetype==CXIMAGE_FORMAT_WBMP){
		CxImageWBMP newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_WMF && CXIMAGE_SUPPORT_WINDOWS // vho - WMF support
	if (imagetype == CXIMAGE_FORMAT_WMF){
		CxImageWMF newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JBG
	if (imagetype==CXIMAGE_FORMAT_JBG){
		CxImageJBG newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_JASPER
	if (
 #if	CXIMAGE_SUPPORT_JP2
		imagetype==CXIMAGE_FORMAT_JP2 || 
 #endif
 #if	CXIMAGE_SUPPORT_JPC
		imagetype==CXIMAGE_FORMAT_JPC || 
 #endif
 #if	CXIMAGE_SUPPORT_PGX
		imagetype==CXIMAGE_FORMAT_PGX || 
 #endif
 #if	CXIMAGE_SUPPORT_PNM
		imagetype==CXIMAGE_FORMAT_PNM || 
 #endif
 #if	CXIMAGE_SUPPORT_RAS
		imagetype==CXIMAGE_FORMAT_RAS || 
 #endif
		 false ){
		CxImageJAS newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile,imagetype)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif
#if CXIMAGE_SUPPORT_SKA
	if (imagetype==CXIMAGE_FORMAT_SKA){
		CxImageSKA newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif

#if CXIMAGE_SUPPORT_RAW
	if (imagetype==CXIMAGE_FORMAT_RAW){
		CxImageRAW newima;
		newima.CopyInfo(*this);
		if (newima.Decode(hFile)){
			Transfer(newima);
			return true;
		} else {
			strcpy(info.szLastError,newima.GetLastError());
			return false;
		}
	}
#endif

	strcpy(info.szLastError,"Decode: Unknown or wrong format");
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Loads an image from CxFile object
 * \param hFile: file handle (CxMemFile or CxIOFile), with read access.
 * \param imagetype: file format, default = 0 (CXIMAGE_FORMAT_UNKNOWN)
 * \return : if imagetype is not 0, the function returns true when imagetype
 *  matches the file image format. If imagetype is 0, the function returns true
 *  when the file image format is recognized as a supported format.
 *  If the returned value is true, use GetHeight(), GetWidth() or GetType()
 *  to retrieve the basic image information.
 * \sa ENUM_CXIMAGE_FORMATS
 */
bool CxImage::CheckFormat(CxFile * hFile, DWORD imagetype)
{
	SetType(CXIMAGE_FORMAT_UNKNOWN);
	SetEscape(-1);

	if (!Decode(hFile,imagetype))
		return false;

	if (GetType() == CXIMAGE_FORMAT_UNKNOWN || GetType() != imagetype)
		return false;

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::CheckFormat(BYTE * buffer, DWORD size, DWORD imagetype)
{
	if (buffer==NULL || size==NULL){
		strcpy(info.szLastError,"invalid or empty buffer");
		return false;
	}
	CxMemFile file(buffer,size);
	return CheckFormat(&file,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
