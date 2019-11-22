/*
 * File:	ximamng.cpp
 * Purpose:	Platform Independent MNG Image Class Loader and Writer
 * Author:	07/Aug/2001 Davide Pizzolato - www.xdp.it
 * CxImage version 6.0.0 02/Feb/2008
 */

#include "ximamng.h"

#if CXIMAGE_SUPPORT_MNG

////////////////////////////////////////////////////////////////////////////////
// callbacks for the mng decoder:
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// memory allocation; data must be zeroed
static mng_ptr
mymngalloc( mng_uint32 size )
{
	return (mng_ptr)calloc(1, size);
}

////////////////////////////////////////////////////////////////////////////////
// memory deallocation
static void mymngfree(mng_ptr p, mng_uint32 size)
{
	free(p);
}

////////////////////////////////////////////////////////////////////////////////
// Stream open/close:
// since the user is responsible for opening and closing the file,
// we leave the default implementation open
static mng_bool mymngopenstream(mng_handle mng)      { return MNG_TRUE; }
static mng_bool mymngopenstreamwrite(mng_handle mng) { return MNG_TRUE; }
static mng_bool mymngclosestream(mng_handle mng)     { return MNG_TRUE; }

////////////////////////////////////////////////////////////////////////////////
// feed data to the decoder
static mng_bool mymngreadstream(mng_handle mng, mng_ptr buffer, mng_uint32 size, mng_uint32 *bytesread)
{
	mngstuff *mymng = (mngstuff *)mng_get_userdata(mng);
	// read the requested amount of data from the file
	*bytesread = mymng->file->Read( buffer, sizeof(BYTE), size);
	return MNG_TRUE;
}

////////////////////////////////////////////////////////////////////////////////
static mng_bool mymngwritestream (mng_handle mng, mng_ptr pBuf, mng_uint32 iSize, mng_uint32 *iWritten)
{
	mngstuff *mymng = (mngstuff *)mng_get_userdata(mng);
	// write it
	*iWritten = mymng->file->Write (pBuf, 1, iSize);
	return MNG_TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// the header's been read. set up the display stuff
static mng_bool mymngprocessheader( mng_handle mng, mng_uint32 width, mng_uint32 height )
{
	// normally the image buffer is allocated here,
	// but in this module we don't know nothing about
	// the final environment.

	mngstuff *mymng = (mngstuff *)mng_get_userdata(mng);
	
	mymng->width  = width;
	mymng->height = height;
	mymng->bpp    = 24;
	mymng->effwdt = ((((width * mymng->bpp) + 31) >> 5) << 2);

	if (mng->bUseBKGD){
		mymng->nBkgndIndex = 0;
		mymng->nBkgndColor.rgbRed  = mng->iBGred >> 8;
		mymng->nBkgndColor.rgbGreen =mng->iBGgreen >> 8;
		mymng->nBkgndColor.rgbBlue = mng->iBGblue >> 8;
	}

	mymng->image = (BYTE*)malloc(height * mymng->effwdt);

	// tell the mng decoder about our bit-depth choice
#if CXIMAGE_SUPPORT_ALPHA
	mng_set_canvasstyle( mng, MNG_CANVAS_RGB8_A8 );
	mymng->alpha = (BYTE*)malloc(height * width);
#else
	mng_set_canvasstyle( mng, MNG_CANVAS_BGR8);
	mymng->alpha = NULL;
#endif
	return MNG_TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// return a row pointer for the decoder to fill
static mng_ptr mymnggetcanvasline( mng_handle mng, mng_uint32 line )
{
	mngstuff *mymng = (mngstuff *)mng_get_userdata(mng);
	return (mng_ptr)(mymng->image + (mymng->effwdt * (mymng->height - 1 - line)));
}
////////////////////////////////////////////////////////////////////////////////
// return a row pointer for the decoder to fill for alpha channel
static mng_ptr mymnggetalphaline( mng_handle mng, mng_uint32 line )
{
	mngstuff *mymng = (mngstuff *)mng_get_userdata(mng);
	return (mng_ptr)(mymng->alpha + (mymng->width * (mymng->height - 1 - line)));
}

////////////////////////////////////////////////////////////////////////////////
// timer
static mng_uint32 mymnggetticks(mng_handle mng)
{
#ifdef WIN32
	return (mng_uint32)GetTickCount();
#else
  return 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Refresh: actual frame need to be updated (Invalidate)
static mng_bool mymngrefresh(mng_handle mng, mng_uint32 x, mng_uint32 y, mng_uint32 w, mng_uint32 h)
{
//	mngstuff *mymng = (mngstuff *)mng_get_userdata(mng);
	return MNG_TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// interframe delay callback
static mng_bool mymngsettimer(mng_handle mng, mng_uint32 msecs)
{
	mngstuff *mymng = (mngstuff *)mng_get_userdata(mng);
	mymng->delay = msecs; 	// set the timer for when the decoder wants to be woken
	return MNG_TRUE;
}

////////////////////////////////////////////////////////////////////////////////
static mng_bool mymngerror(mng_handle mng, mng_int32 code, mng_int8 severity, mng_chunkid chunktype, mng_uint32 chunkseq, mng_int32 extra1, mng_int32 extra2, mng_pchar text)
{
	return mng_cleanup(&mng); //<Arkadiy Olovyannikov>
}

////////////////////////////////////////////////////////////////////////////////
// CxImage members
////////////////////////////////////////////////////////////////////////////////
CxImageMNG::CxImageMNG(): CxImage(CXIMAGE_FORMAT_MNG)
{
	hmng = NULL;
	memset(&mnginfo,0,sizeof(mngstuff));
	mnginfo.nBkgndIndex = -1;
	mnginfo.speed = 1.0f;
}
////////////////////////////////////////////////////////////////////////////////
CxImageMNG::~CxImageMNG()
{
	// cleanup and return
	if (mnginfo.thread){ //close the animation thread
		mnginfo.animation_enabled=0;
		ResumeThread(mnginfo.thread);
		WaitForSingleObject(mnginfo.thread,500);
		CloseHandle(mnginfo.thread);
	}
	// free objects
	if (mnginfo.image) free(mnginfo.image);
	if (mnginfo.alpha) free(mnginfo.alpha);
	if (hmng) mng_cleanup(&hmng); //be sure it's not needed any more. (active timers ?)
}
////////////////////////////////////////////////////////////////////////////////
void CxImageMNG::SetCallbacks(mng_handle mng)
{
	// set the callbacks
	mng_setcb_errorproc(mng, mymngerror);
	mng_setcb_openstream(mng, mymngopenstream);
	mng_setcb_closestream(mng, mymngclosestream);
	mng_setcb_readdata(mng, mymngreadstream);
	mng_setcb_processheader(mng, mymngprocessheader);
	mng_setcb_getcanvasline(mng, mymnggetcanvasline);
	mng_setcb_refresh(mng, mymngrefresh);
	mng_setcb_gettickcount(mng, mymnggetticks);
	mng_setcb_settimer(mng, mymngsettimer);
	mng_setcb_refresh(mng, mymngrefresh);
	mng_setcb_getalphaline(mng, mymnggetalphaline);
}
////////////////////////////////////////////////////////////////////////////////
// can't use the CxImage implementation because it looses mnginfo
bool CxImageMNG::Load(const TCHAR * imageFileName){
	FILE* hFile;	//file handle to read the image
#ifdef WIN32
	if ((hFile=_tfopen(imageFileName,_T("rb")))==NULL)  return false;	// For UNICODE support
#else
	if ((hFile=fopen(imageFileName,"rb"))==NULL)  return false;
#endif
	bool bOK = Decode(hFile);
	fclose(hFile);
	return bOK;
}
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageMNG::Decode(CxFile *hFile)
{
	if (hFile == NULL) return false;

	cx_try
	{
		// set up the mng decoder for our stream
		hmng = mng_initialize(&mnginfo, mymngalloc, mymngfree, MNG_NULL);
		if (hmng == NULL) cx_throw("could not initialize libmng");			

		// set the file we want to play
		mnginfo.file = hFile;

		// Set the colorprofile, lcms uses this:
		mng_set_srgb(hmng, MNG_TRUE );
		// Set white as background color:
		WORD Red,Green,Blue;
		Red = Green = Blue = (255 << 8) + 255;
		mng_set_bgcolor(hmng, Red, Green, Blue );
		// If PNG Background is available, use it:
		mng_set_usebkgd(hmng, MNG_TRUE );

		// No need to store chunks:
		mng_set_storechunks(hmng, MNG_FALSE);
		// No need to wait: straight reading
		mng_set_suspensionmode(hmng, MNG_FALSE);

		SetCallbacks(hmng);

		mng_datap pData = (mng_datap)hmng;

		// read in the image
		info.nNumFrames=0;
		int retval=MNG_NOERROR;

		retval = mng_readdisplay(hmng);

		if (retval != MNG_NOERROR && retval != MNG_NEEDTIMERWAIT){
			mng_store_error(hmng,retval,0,0);
			if (hmng->zErrortext){
				cx_throw(hmng->zErrortext);
			} else {
				cx_throw("Error in MNG file");
			}
		}

		if (info.nEscape == -1) {
			// Return output dimensions only
			head.biWidth = hmng->iWidth;
			head.biHeight = hmng->iHeight;
			info.dwType = CXIMAGE_FORMAT_MNG;
			return true;
		}

		// read all
		while(pData->bReading){
			retval = mng_display_resume(hmng);
			info.nNumFrames++;
		}

		// single frame check:
		if (retval != MNG_NEEDTIMERWAIT){
			info.nNumFrames--;
		} else {
			mnginfo.animation=1;
		}

		if (info.nNumFrames<=0) info.nNumFrames=1;

		if (mnginfo.animation_enabled==0){
			// select the frame
			if (info.nFrame>=0 && info.nFrame<info.nNumFrames){
				for (int n=0;n<info.nFrame;n++) mng_display_resume(hmng);
			} else cx_throw("Error: frame not present in MNG file");
		}

		if (mnginfo.nBkgndIndex >= 0){
			info.nBkgndIndex = mnginfo.nBkgndIndex;
			info.nBkgndColor.rgbRed = mnginfo.nBkgndColor.rgbRed;
			info.nBkgndColor.rgbGreen = mnginfo.nBkgndColor.rgbGreen;
			info.nBkgndColor.rgbBlue = mnginfo.nBkgndColor.rgbBlue;
		}

		//store the newly created image
		if (Create(mnginfo.width,mnginfo.height,mnginfo.bpp, CXIMAGE_FORMAT_MNG)){
			memcpy(GetBits(), mnginfo.image, info.dwEffWidth * head.biHeight);
#if CXIMAGE_SUPPORT_ALPHA
			SwapRGB2BGR();
			AlphaCreate();
			if(AlphaIsValid() && mnginfo.alpha){
				memcpy(AlphaGetPointer(),mnginfo.alpha,mnginfo.width * mnginfo.height);
			}
#endif
		} else cx_throw("CxImageMNG::Decode cannot create image");


	} cx_catch {
		if (strcmp(message,"")) strncpy(info.szLastError,message,255);
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageMNG::Encode(CxFile *hFile)
{
	if (EncodeSafeCheck(hFile)) return false;

	cx_try
	{
		if (head.biClrUsed != 0) cx_throw("MNG encoder can save only RGB images");
		// set the file we want to play
		mnginfo.file = hFile;
		mnginfo.bpp = head.biBitCount;
		mnginfo.effwdt = info.dwEffWidth;
		mnginfo.height = head.biHeight;
		mnginfo.width =  head.biWidth;

		mnginfo.image = (BYTE*)malloc(head.biSizeImage);
		if (mnginfo.image == NULL) cx_throw("could not allocate memory for MNG");
		memcpy(mnginfo.image,info.pImage, head.biSizeImage);

		// set up the mng decoder for our stream
		hmng = mng_initialize(&mnginfo, mymngalloc, mymngfree, MNG_NULL);
		if (hmng == NULL) cx_throw("could not initialize libmng");			

		mng_setcb_openstream(hmng, mymngopenstreamwrite );
		mng_setcb_closestream(hmng, mymngclosestream);
		mng_setcb_writedata(hmng, mymngwritestream);

		// Write File:
   		mng_create(hmng);
		// Just a single Frame (save a normal PNG):
		WritePNG(hmng, 0, 1 );
		// Now write file:
		mng_write(hmng);

	} cx_catch {
		if (strcmp(message,"")) strncpy(info.szLastError,message,255);
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
// Writes a single PNG datastream
void CxImageMNG::WritePNG( mng_handle hMNG, int Frame, int FrameCount )
{
	mngstuff *mymng = (mngstuff *)mng_get_userdata(hMNG);
	
	int OffsetX=0,OffsetY=0,OffsetW=mymng->width,OffsetH=mymng->height;

	BYTE *tmpbuffer = new BYTE[ (mymng->effwdt+1) * mymng->height];
	if( tmpbuffer == 0 ) return;

	// Write DEFI chunk.
	mng_putchunk_defi( hMNG, 0, 0, 0, MNG_TRUE, OffsetX, OffsetY, MNG_FALSE, 0, 0, 0, 0 );
 		 
	// Write Header:
	mng_putchunk_ihdr(
		hMNG, 
		OffsetW, OffsetH, 
		MNG_BITDEPTH_8, 
		MNG_COLORTYPE_RGB, 
		MNG_COMPRESSION_DEFLATE, 
		MNG_FILTER_ADAPTIVE, 
		MNG_INTERLACE_NONE 
	);

	// transfer data, add Filterbyte:
	for( int Row=0; Row<OffsetH; Row++ ){
		// First Byte in each Scanline is Filterbyte: Currently 0 -> No Filter.
		tmpbuffer[Row*(mymng->effwdt+1)]=0; 
		// Copy the scanline: (reverse order)
		memcpy(tmpbuffer+Row*(mymng->effwdt+1)+1, 
			mymng->image+((OffsetH-1-(OffsetY+Row))*(mymng->effwdt))+OffsetX,mymng->effwdt);
		// swap red and blue components
		RGBtoBGR(tmpbuffer+Row*(mymng->effwdt+1)+1,mymng->effwdt);
	} 

	// Compress data with ZLib (Deflate):
	BYTE *dstbuffer = new BYTE[(mymng->effwdt+1)*OffsetH];
	if( dstbuffer == 0 ) return;
	DWORD dstbufferSize=(mymng->effwdt+1)*OffsetH;

	// Compress data:
	if(Z_OK != compress2((Bytef *)dstbuffer,(ULONG *)&dstbufferSize,(const Bytef*)tmpbuffer,
						(ULONG) (mymng->effwdt+1)*OffsetH,9 )) return;

	// Write Data into MNG File:
	mng_putchunk_idat( hMNG, dstbufferSize, (mng_ptr*)dstbuffer);
	mng_putchunk_iend(hMNG);

	// Free the stuff:
	delete [] tmpbuffer;
	delete [] dstbuffer;
}
////////////////////////////////////////////////////////////////////////////////
long CxImageMNG::Resume()
{
	if (MNG_NEEDTIMERWAIT == mng_display_resume(hmng)){
		if (info.pImage==NULL){
			Create(mnginfo.width,mnginfo.height,mnginfo.bpp, CXIMAGE_FORMAT_MNG);
		}
		if (IsValid()){
			memcpy(GetBits(), mnginfo.image, info.dwEffWidth * head.biHeight);
#if CXIMAGE_SUPPORT_ALPHA
			SwapRGB2BGR();
			AlphaCreate();
			if(AlphaIsValid() && mnginfo.alpha){
				memcpy(AlphaGetPointer(),mnginfo.alpha,mnginfo.width * mnginfo.height);
			}
#endif
		}
	} else {
		mnginfo.animation_enabled = 0;
	}
	return mnginfo.animation_enabled;
}
////////////////////////////////////////////////////////////////////////////////
void CxImageMNG::SetSpeed(float speed)
{
	if (speed>10.0) mnginfo.speed = 10.0f;
	else if (speed<0.1) mnginfo.speed = 0.1f;
	else mnginfo.speed=speed;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_MNG
