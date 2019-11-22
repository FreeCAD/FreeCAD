// xImaTran.cpp : Transformation functions
/* 07/08/2001 v1.00 - Davide Pizzolato - www.xdp.it
 * CxImage version 6.0.0 02/Feb/2008
 */

#include "ximage.h"
#include "ximath.h"

#if CXIMAGE_SUPPORT_BASICTRANSFORMATIONS
////////////////////////////////////////////////////////////////////////////////
bool CxImage::GrayScale()
{
	if (!pDib) return false;
	if (head.biBitCount<=8){
		RGBQUAD* ppal=GetPalette();
		int gray;
		//converts the colors to gray, use the blue channel only
		for(DWORD i=0;i<head.biClrUsed;i++){
			gray=(int)RGB2GRAY(ppal[i].rgbRed,ppal[i].rgbGreen,ppal[i].rgbBlue);
			ppal[i].rgbBlue = (BYTE)gray;
		}
		// preserve transparency
		if (info.nBkgndIndex >= 0) info.nBkgndIndex = ppal[info.nBkgndIndex].rgbBlue;
		//create a "real" 8 bit gray scale image
		if (head.biBitCount==8){
			BYTE *img=info.pImage;
			for(DWORD i=0;i<head.biSizeImage;i++) img[i]=ppal[img[i]].rgbBlue;
			SetGrayPalette();
		}
		//transform to 8 bit gray scale
		if (head.biBitCount==4 || head.biBitCount==1){
			CxImage ima;
			ima.CopyInfo(*this);
			if (!ima.Create(head.biWidth,head.biHeight,8,info.dwType)) return false;
			ima.SetGrayPalette();
#if CXIMAGE_SUPPORT_SELECTION
			ima.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION
#if CXIMAGE_SUPPORT_ALPHA
			ima.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA
			for (long y=0;y<head.biHeight;y++){
				BYTE *iDst = ima.GetBits(y);
				BYTE *iSrc = GetBits(y);
				for (long x=0;x<head.biWidth; x++){
					//iDst[x]=ppal[BlindGetPixelIndex(x,y)].rgbBlue;
					if (head.biBitCount==4){
						BYTE pos = (BYTE)(4*(1-x%2));
						iDst[x]= ppal[(BYTE)((iSrc[x >> 1]&((BYTE)0x0F<<pos)) >> pos)].rgbBlue;
					} else {
						BYTE pos = (BYTE)(7-x%8);
						iDst[x]= ppal[(BYTE)((iSrc[x >> 3]&((BYTE)0x01<<pos)) >> pos)].rgbBlue;
					}
				}
			}
			Transfer(ima);
		}
	} else { //from RGB to 8 bit gray scale
		BYTE *iSrc=info.pImage;
		CxImage ima;
		ima.CopyInfo(*this);
		if (!ima.Create(head.biWidth,head.biHeight,8,info.dwType)) return false;
		ima.SetGrayPalette();
#if CXIMAGE_SUPPORT_SELECTION
		ima.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION
#if CXIMAGE_SUPPORT_ALPHA
		ima.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA
		BYTE *img=ima.GetBits();
		long l8=ima.GetEffWidth();
		long l=head.biWidth * 3;
		for(long y=0; y < head.biHeight; y++) {
			for(long x=0,x8=0; x < l; x+=3,x8++) {
				img[x8+y*l8]=(BYTE)RGB2GRAY(*(iSrc+x+2),*(iSrc+x+1),*(iSrc+x+0));
			}
			iSrc+=info.dwEffWidth;
		}
		Transfer(ima);
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \sa Mirror
 * \author [qhbo]
 */
bool CxImage::Flip(bool bFlipSelection, bool bFlipAlpha)
{
	if (!pDib) return false;

	BYTE *buff = (BYTE*)malloc(info.dwEffWidth);
	if (!buff) return false;

	BYTE *iSrc,*iDst;
	iSrc = GetBits(head.biHeight-1);
	iDst = GetBits(0);
	for (long i=0; i<(head.biHeight/2); ++i)
	{
		memcpy(buff, iSrc, info.dwEffWidth);
		memcpy(iSrc, iDst, info.dwEffWidth);
		memcpy(iDst, buff, info.dwEffWidth);
		iSrc-=info.dwEffWidth;
		iDst+=info.dwEffWidth;
	}

	free(buff);

	if (bFlipSelection){
#if CXIMAGE_SUPPORT_SELECTION
		SelectionFlip();
#endif //CXIMAGE_SUPPORT_SELECTION
	}

	if (bFlipAlpha){
#if CXIMAGE_SUPPORT_ALPHA
		AlphaFlip();
#endif //CXIMAGE_SUPPORT_ALPHA
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \sa Flip
 */
bool CxImage::Mirror(bool bMirrorSelection, bool bMirrorAlpha)
{
	if (!pDib) return false;

	CxImage* imatmp = new CxImage(*this,false,true,true);
	if (!imatmp) return false;
	if (!imatmp->IsValid()){
		delete imatmp;
		return false;
	}

	BYTE *iSrc,*iDst;
	long wdt=(head.biWidth-1) * (head.biBitCount==24 ? 3:1);
	iSrc=info.pImage + wdt;
	iDst=imatmp->info.pImage;
	long x,y;
	switch (head.biBitCount){
	case 24:
		for(y=0; y < head.biHeight; y++){
			for(x=0; x <= wdt; x+=3){
				*(iDst+x)=*(iSrc-x);
				*(iDst+x+1)=*(iSrc-x+1);
				*(iDst+x+2)=*(iSrc-x+2);
			}
			iSrc+=info.dwEffWidth;
			iDst+=info.dwEffWidth;
		}
		break;
	case 8:
		for(y=0; y < head.biHeight; y++){
			for(x=0; x <= wdt; x++)
				*(iDst+x)=*(iSrc-x);
			iSrc+=info.dwEffWidth;
			iDst+=info.dwEffWidth;
		}
		break;
	default:
		for(y=0; y < head.biHeight; y++){
			for(x=0; x <= wdt; x++)
				imatmp->SetPixelIndex(x,y,GetPixelIndex(wdt-x,y));
		}
	}

	if (bMirrorSelection){
#if CXIMAGE_SUPPORT_SELECTION
		imatmp->SelectionMirror();
#endif //CXIMAGE_SUPPORT_SELECTION
	}

	if (bMirrorAlpha){
#if CXIMAGE_SUPPORT_ALPHA
		imatmp->AlphaMirror();
#endif //CXIMAGE_SUPPORT_ALPHA
	}

	Transfer(*imatmp);
	delete imatmp;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
#define RBLOCK 64

////////////////////////////////////////////////////////////////////////////////
bool CxImage::RotateLeft(CxImage* iDst)
{
	if (!pDib) return false;

	long newWidth = GetHeight();
	long newHeight = GetWidth();

	CxImage imgDest;
	imgDest.CopyInfo(*this);
	imgDest.Create(newWidth,newHeight,GetBpp(),GetType());
	imgDest.SetPalette(GetPalette());

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()) imgDest.AlphaCreate();
#endif

#if CXIMAGE_SUPPORT_SELECTION
	if (SelectionIsValid()) imgDest.SelectionCreate();
#endif

	long x,x2,y,dlineup;
	
	// Speedy rotate for BW images <Robert Abram>
	if (head.biBitCount == 1) {
	
		BYTE *sbits, *dbits, *dbitsmax, bitpos, *nrow,*srcdisp;
		ldiv_t div_r;

		BYTE *bsrc = GetBits(), *bdest = imgDest.GetBits();
		dbitsmax = bdest + imgDest.head.biSizeImage - 1;
		dlineup = 8 * imgDest.info.dwEffWidth - imgDest.head.biWidth;

		imgDest.Clear(0);
		for (y = 0; y < head.biHeight; y++) {
			// Figure out the Column we are going to be copying to
			div_r = ldiv(y + dlineup, (long)8);
			// set bit pos of src column byte				
			bitpos = (BYTE)(1 << div_r.rem);
			srcdisp = bsrc + y * info.dwEffWidth;
			for (x = 0; x < (long)info.dwEffWidth; x++) {
				// Get Source Bits
				sbits = srcdisp + x;
				// Get destination column
				nrow = bdest + (x * 8) * imgDest.info.dwEffWidth + imgDest.info.dwEffWidth - 1 - div_r.quot;
				for (long z = 0; z < 8; z++) {
				   // Get Destination Byte
					dbits = nrow + z * imgDest.info.dwEffWidth;
					if ((dbits < bdest) || (dbits > dbitsmax)) break;
					if (*sbits & (128 >> z)) *dbits |= bitpos;
				}
			}
		}//for y

#if CXIMAGE_SUPPORT_ALPHA
		if (AlphaIsValid()) {
			for (x = 0; x < newWidth; x++){
				x2=newWidth-x-1;
				for (y = 0; y < newHeight; y++){
					imgDest.AlphaSet(x,y,BlindAlphaGet(y, x2));
				}//for y
			}//for x
		}
#endif //CXIMAGE_SUPPORT_ALPHA

#if CXIMAGE_SUPPORT_SELECTION
		if (SelectionIsValid()) {
			imgDest.info.rSelectionBox.left = newWidth-info.rSelectionBox.top;
			imgDest.info.rSelectionBox.right = newWidth-info.rSelectionBox.bottom;
			imgDest.info.rSelectionBox.bottom = info.rSelectionBox.left;
			imgDest.info.rSelectionBox.top = info.rSelectionBox.right;
			for (x = 0; x < newWidth; x++){
				x2=newWidth-x-1;
				for (y = 0; y < newHeight; y++){
					imgDest.SelectionSet(x,y,BlindSelectionGet(y, x2));
				}//for y
			}//for x
		}
#endif //CXIMAGE_SUPPORT_SELECTION

	} else {
	//anything other than BW:
	//bd, 10. 2004: This optimized version of rotation rotates image by smaller blocks. It is quite
	//a bit faster than obvious algorithm, because it produces much less CPU cache misses.
	//This optimization can be tuned by changing block size (RBLOCK). 96 is good value for current
	//CPUs (tested on Athlon XP and Celeron D). Larger value (if CPU has enough cache) will increase
	//speed somehow, but once you drop out of CPU's cache, things will slow down drastically.
	//For older CPUs with less cache, lower value would yield better results.
		
		BYTE *srcPtr, *dstPtr;                        //source and destionation for 24-bit version
		int xs, ys;                                   //x-segment and y-segment
		for (xs = 0; xs < newWidth; xs+=RBLOCK) {       //for all image blocks of RBLOCK*RBLOCK pixels
			for (ys = 0; ys < newHeight; ys+=RBLOCK) {
				if (head.biBitCount==24) {
					//RGB24 optimized pixel access:
					for (x = xs; x < min(newWidth, xs+RBLOCK); x++){    //do rotation
						info.nProgress = (long)(100*x/newWidth);
						x2=newWidth-x-1;
						dstPtr = (BYTE*) imgDest.BlindGetPixelPointer(x,ys);
						srcPtr = (BYTE*) BlindGetPixelPointer(ys, x2);
						for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
							//imgDest.SetPixelColor(x, y, GetPixelColor(y, x2));
							*(dstPtr) = *(srcPtr);
							*(dstPtr+1) = *(srcPtr+1);
							*(dstPtr+2) = *(srcPtr+2);
							srcPtr += 3;
							dstPtr += imgDest.info.dwEffWidth;
						}//for y
					}//for x
				} else {
					//anything else than 24bpp (and 1bpp): palette
					for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
						info.nProgress = (long)(100*x/newWidth); //<Anatoly Ivasyuk>
						x2=newWidth-x-1;
						for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
							imgDest.SetPixelIndex(x, y, BlindGetPixelIndex(y, x2));
						}//for y
					}//for x
				}//if (version selection)
#if CXIMAGE_SUPPORT_ALPHA
				if (AlphaIsValid()) {
					for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
						x2=newWidth-x-1;
						for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
							imgDest.AlphaSet(x,y,BlindAlphaGet(y, x2));
						}//for y
					}//for x
				}//if (alpha channel)
#endif //CXIMAGE_SUPPORT_ALPHA

#if CXIMAGE_SUPPORT_SELECTION
				if (SelectionIsValid()) {
					imgDest.info.rSelectionBox.left = newWidth-info.rSelectionBox.top;
					imgDest.info.rSelectionBox.right = newWidth-info.rSelectionBox.bottom;
					imgDest.info.rSelectionBox.bottom = info.rSelectionBox.left;
					imgDest.info.rSelectionBox.top = info.rSelectionBox.right;
					for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
						x2=newWidth-x-1;
						for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
							imgDest.SelectionSet(x,y,BlindSelectionGet(y, x2));
						}//for y
					}//for x
				}//if (selection)
#endif //CXIMAGE_SUPPORT_SELECTION
			}//for ys
		}//for xs
	}//if

	//select the destination
	if (iDst) iDst->Transfer(imgDest);
	else Transfer(imgDest);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool CxImage::RotateRight(CxImage* iDst)
{
	if (!pDib) return false;

	long newWidth = GetHeight();
	long newHeight = GetWidth();

	CxImage imgDest;
	imgDest.CopyInfo(*this);
	imgDest.Create(newWidth,newHeight,GetBpp(),GetType());
	imgDest.SetPalette(GetPalette());

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()) imgDest.AlphaCreate();
#endif

#if CXIMAGE_SUPPORT_SELECTION
	if (SelectionIsValid()) imgDest.SelectionCreate();
#endif

	long x,y,y2;
	// Speedy rotate for BW images <Robert Abram>
	if (head.biBitCount == 1) {
	
		BYTE *sbits, *dbits, *dbitsmax, bitpos, *nrow,*srcdisp;
		ldiv_t div_r;

		BYTE *bsrc = GetBits(), *bdest = imgDest.GetBits();
		dbitsmax = bdest + imgDest.head.biSizeImage - 1;

		imgDest.Clear(0);
		for (y = 0; y < head.biHeight; y++) {
			// Figure out the Column we are going to be copying to
			div_r = ldiv(y, (long)8);
			// set bit pos of src column byte				
			bitpos = (BYTE)(128 >> div_r.rem);
			srcdisp = bsrc + y * info.dwEffWidth;
			for (x = 0; x < (long)info.dwEffWidth; x++) {
				// Get Source Bits
				sbits = srcdisp + x;
				// Get destination column
				nrow = bdest + (imgDest.head.biHeight-1-(x*8)) * imgDest.info.dwEffWidth + div_r.quot;
				for (long z = 0; z < 8; z++) {
				   // Get Destination Byte
					dbits = nrow - z * imgDest.info.dwEffWidth;
					if ((dbits < bdest) || (dbits > dbitsmax)) break;
					if (*sbits & (128 >> z)) *dbits |= bitpos;
				}
			}
		}

#if CXIMAGE_SUPPORT_ALPHA
		if (AlphaIsValid()){
			for (y = 0; y < newHeight; y++){
				y2=newHeight-y-1;
				for (x = 0; x < newWidth; x++){
					imgDest.AlphaSet(x,y,BlindAlphaGet(y2, x));
				}
			}
		}
#endif //CXIMAGE_SUPPORT_ALPHA

#if CXIMAGE_SUPPORT_SELECTION
		if (SelectionIsValid()){
			imgDest.info.rSelectionBox.left = info.rSelectionBox.bottom;
			imgDest.info.rSelectionBox.right = info.rSelectionBox.top;
			imgDest.info.rSelectionBox.bottom = newHeight-info.rSelectionBox.right;
			imgDest.info.rSelectionBox.top = newHeight-info.rSelectionBox.left;
			for (y = 0; y < newHeight; y++){
				y2=newHeight-y-1;
				for (x = 0; x < newWidth; x++){
					imgDest.SelectionSet(x,y,BlindSelectionGet(y2, x));
				}
			}
		}
#endif //CXIMAGE_SUPPORT_SELECTION

	} else {
		//anything else but BW
		BYTE *srcPtr, *dstPtr;                        //source and destionation for 24-bit version
		int xs, ys;                                   //x-segment and y-segment
		for (xs = 0; xs < newWidth; xs+=RBLOCK) {
			for (ys = 0; ys < newHeight; ys+=RBLOCK) {
				if (head.biBitCount==24) {
					//RGB24 optimized pixel access:
					for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
						info.nProgress = (long)(100*y/newHeight); //<Anatoly Ivasyuk>
						y2=newHeight-y-1;
						dstPtr = (BYTE*) imgDest.BlindGetPixelPointer(xs,y);
						srcPtr = (BYTE*) BlindGetPixelPointer(y2, xs);
						for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
							//imgDest.SetPixelColor(x, y, GetPixelColor(y2, x));
							*(dstPtr) = *(srcPtr);
							*(dstPtr+1) = *(srcPtr+1);
							*(dstPtr+2) = *(srcPtr+2);
							dstPtr += 3;
							srcPtr += info.dwEffWidth;
						}//for x
					}//for y
				} else {
					//anything else than BW & RGB24: palette
					for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
						info.nProgress = (long)(100*y/newHeight); //<Anatoly Ivasyuk>
						y2=newHeight-y-1;
						for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
							imgDest.SetPixelIndex(x, y, BlindGetPixelIndex(y2, x));
						}//for x
					}//for y
				}//if
#if CXIMAGE_SUPPORT_ALPHA
				if (AlphaIsValid()){
					for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
						y2=newHeight-y-1;
						for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
							imgDest.AlphaSet(x,y,BlindAlphaGet(y2, x));
						}//for x
					}//for y
				}//if (has alpha)
#endif //CXIMAGE_SUPPORT_ALPHA

#if CXIMAGE_SUPPORT_SELECTION
				if (SelectionIsValid()){
					imgDest.info.rSelectionBox.left = info.rSelectionBox.bottom;
					imgDest.info.rSelectionBox.right = info.rSelectionBox.top;
					imgDest.info.rSelectionBox.bottom = newHeight-info.rSelectionBox.right;
					imgDest.info.rSelectionBox.top = newHeight-info.rSelectionBox.left;
					for (y = ys; y < min(newHeight, ys+RBLOCK); y++){
						y2=newHeight-y-1;
						for (x = xs; x < min(newWidth, xs+RBLOCK); x++){
							imgDest.SelectionSet(x,y,BlindSelectionGet(y2, x));
						}//for x
					}//for y
				}//if (has alpha)
#endif //CXIMAGE_SUPPORT_SELECTION
			}//for ys
		}//for xs
	}//if

	//select the destination
	if (iDst) iDst->Transfer(imgDest);
	else Transfer(imgDest);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool CxImage::Negative()
{
	if (!pDib) return false;

	if (head.biBitCount<=8){
		if (IsGrayScale()){ //GRAYSCALE, selection
			if (pSelection){
				for(long y=info.rSelectionBox.bottom; y<info.rSelectionBox.top; y++){
					for(long x=info.rSelectionBox.left; x<info.rSelectionBox.right; x++){
#if CXIMAGE_SUPPORT_SELECTION
						if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
						{
							BlindSetPixelIndex(x,y,(BYTE)(255-BlindGetPixelIndex(x,y)));
						}
					}
				}
			} else {
				BYTE *iSrc=info.pImage;
				for(unsigned long i=0; i < head.biSizeImage; i++){
					*iSrc=(BYTE)~(*(iSrc));
					iSrc++;
				}
			}
		} else { //PALETTE, full image
			RGBQUAD* ppal=GetPalette();
			for(DWORD i=0;i<head.biClrUsed;i++){
				ppal[i].rgbBlue =(BYTE)(255-ppal[i].rgbBlue);
				ppal[i].rgbGreen =(BYTE)(255-ppal[i].rgbGreen);
				ppal[i].rgbRed =(BYTE)(255-ppal[i].rgbRed);
			}
		}
	} else {
		if (pSelection==NULL){ //RGB, full image
			BYTE *iSrc=info.pImage;
			for(unsigned long i=0; i < head.biSizeImage; i++){
				*iSrc=(BYTE)~(*(iSrc));
				iSrc++;
			}
		} else { // RGB with selection
			RGBQUAD color;
			for(long y=info.rSelectionBox.bottom; y<info.rSelectionBox.top; y++){
				for(long x=info.rSelectionBox.left; x<info.rSelectionBox.right; x++){
#if CXIMAGE_SUPPORT_SELECTION
					if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
					{
						color = BlindGetPixelColor(x,y);
						color.rgbRed = (BYTE)(255-color.rgbRed);
						color.rgbGreen = (BYTE)(255-color.rgbGreen);
						color.rgbBlue = (BYTE)(255-color.rgbBlue);
						BlindSetPixelColor(x,y,color);
					}
				}
			}
		}
		//<DP> invert transparent color too
		info.nBkgndColor.rgbBlue = (BYTE)(255-info.nBkgndColor.rgbBlue);
		info.nBkgndColor.rgbGreen = (BYTE)(255-info.nBkgndColor.rgbGreen);
		info.nBkgndColor.rgbRed = (BYTE)(255-info.nBkgndColor.rgbRed);
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_BASICTRANSFORMATIONS
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_TRANSFORMATION
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
bool CxImage::Rotate(float angle, CxImage* iDst)
{
	if (!pDib) return false;

	//  Copyright (c) 1996-1998 Ulrich von Zadow

	// Negative the angle, because the y-axis is negative.
	double ang = -angle*acos((float)0)/90;
	int newWidth, newHeight;
	int nWidth = GetWidth();
	int nHeight= GetHeight();
	double cos_angle = cos(ang);
	double sin_angle = sin(ang);

	// Calculate the size of the new bitmap
	POINT p1={0,0};
	POINT p2={nWidth,0};
	POINT p3={0,nHeight};
	POINT p4={nWidth,nHeight};
	CxPoint2 newP1,newP2,newP3,newP4, leftTop, rightTop, leftBottom, rightBottom;

	newP1.x = (float)p1.x;
	newP1.y = (float)p1.y;
	newP2.x = (float)(p2.x*cos_angle - p2.y*sin_angle);
	newP2.y = (float)(p2.x*sin_angle + p2.y*cos_angle);
	newP3.x = (float)(p3.x*cos_angle - p3.y*sin_angle);
	newP3.y = (float)(p3.x*sin_angle + p3.y*cos_angle);
	newP4.x = (float)(p4.x*cos_angle - p4.y*sin_angle);
	newP4.y = (float)(p4.x*sin_angle + p4.y*cos_angle);

	leftTop.x = min(min(newP1.x,newP2.x),min(newP3.x,newP4.x));
	leftTop.y = min(min(newP1.y,newP2.y),min(newP3.y,newP4.y));
	rightBottom.x = max(max(newP1.x,newP2.x),max(newP3.x,newP4.x));
	rightBottom.y = max(max(newP1.y,newP2.y),max(newP3.y,newP4.y));
	leftBottom.x = leftTop.x;
	leftBottom.y = rightBottom.y;
	rightTop.x = rightBottom.x;
	rightTop.y = leftTop.y;

	newWidth = (int) floor(0.5f + rightTop.x - leftTop.x);
	newHeight= (int) floor(0.5f + leftBottom.y - leftTop.y);
	CxImage imgDest;
	imgDest.CopyInfo(*this);
	imgDest.Create(newWidth,newHeight,GetBpp(),GetType());
	imgDest.SetPalette(GetPalette());

#if CXIMAGE_SUPPORT_ALPHA
	if(AlphaIsValid())	//MTA: Fix for rotation problem when the image has an alpha channel
	{
		imgDest.AlphaCreate();
		imgDest.AlphaClear();
	}
#endif //CXIMAGE_SUPPORT_ALPHA

	int x,y,newX,newY,oldX,oldY;

	if (head.biClrUsed==0){ //RGB
		for (y = (int)leftTop.y, newY = 0; y<=(int)leftBottom.y; y++,newY++){
			info.nProgress = (long)(100*newY/newHeight);
			if (info.nEscape) break;
			for (x = (int)leftTop.x, newX = 0; x<=(int)rightTop.x; x++,newX++){
				oldX = (long)(x*cos_angle + y*sin_angle + 0.5);
				oldY = (long)(y*cos_angle - x*sin_angle + 0.5);
				imgDest.SetPixelColor(newX,newY,GetPixelColor(oldX,oldY));
#if CXIMAGE_SUPPORT_ALPHA
				imgDest.AlphaSet(newX,newY,AlphaGet(oldX,oldY));				//MTA: copy the alpha value
#endif //CXIMAGE_SUPPORT_ALPHA
			}
		}
	} else { //PALETTE
		for (y = (int)leftTop.y, newY = 0; y<=(int)leftBottom.y; y++,newY++){
			info.nProgress = (long)(100*newY/newHeight);
			if (info.nEscape) break;
			for (x = (int)leftTop.x, newX = 0; x<=(int)rightTop.x; x++,newX++){
				oldX = (long)(x*cos_angle + y*sin_angle + 0.5);
				oldY = (long)(y*cos_angle - x*sin_angle + 0.5);
				imgDest.SetPixelIndex(newX,newY,GetPixelIndex(oldX,oldY));
#if CXIMAGE_SUPPORT_ALPHA
				imgDest.AlphaSet(newX,newY,AlphaGet(oldX,oldY));				//MTA: copy the alpha value
#endif //CXIMAGE_SUPPORT_ALPHA
			}
		}
	}
	//select the destination
	if (iDst) iDst->Transfer(imgDest);
	else Transfer(imgDest);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Rotates image around it's center.
 * Method can use interpolation with paletted images, but does not change pallete, so results vary.
 * (If you have only four colours in a palette, there's not much room for interpolation.)
 * 
 * \param  angle - angle in degrees (positive values rotate clockwise)
 * \param  *iDst - destination image (if null, this image is changed)
 * \param  inMethod - interpolation method used
 *              (IM_NEAREST_NEIGHBOUR produces aliasing (fast), IM_BILINEAR softens picture a bit (slower)
 *               IM_SHARPBICUBIC is slower and produces some halos...)
 * \param  ofMethod - overflow method (how to choose colour of pixels that have no source)
 * \param  replColor - replacement colour to use (OM_COLOR, OM_BACKGROUND with no background colour...)
 * \param  optimizeRightAngles - call faster methods for 90, 180, and 270 degree rotations. Faster methods
 *                         are called for angles, where error (in location of corner pixels) is less
 *                         than 0.25 pixels.
 * \param  bKeepOriginalSize - rotates the image without resizing.
 *
 * \author ***bd*** 2.2004
 */
bool CxImage::Rotate2(float angle, 
                       CxImage *iDst, 
                       InterpolationMethod inMethod, 
                       OverflowMethod ofMethod, 
                       RGBQUAD *replColor,
                       bool const optimizeRightAngles,
					   bool const bKeepOriginalSize)
{
	if (!pDib) return false;					//no dib no go
	
	double ang = -angle*acos(0.0f)/90.0f;		//convert angle to radians and invert (positive angle performs clockwise rotation)
	float cos_angle = (float) cos(ang);			//these two are needed later (to rotate)
	float sin_angle = (float) sin(ang);
	
	//Calculate the size of the new bitmap (rotate corners of image)
	CxPoint2 p[4];								//original corners of the image
	p[0]=CxPoint2(-0.5f,-0.5f);
	p[1]=CxPoint2(GetWidth()-0.5f,-0.5f);
	p[2]=CxPoint2(-0.5f,GetHeight()-0.5f);
	p[3]=CxPoint2(GetWidth()-0.5f,GetHeight()-0.5f);
	CxPoint2 newp[4];								//rotated positions of corners
	//(rotate corners)
	if (bKeepOriginalSize){
		for (int i=0; i<4; i++) {
			newp[i].x = p[i].x;
			newp[i].y = p[i].y;
		}//for
	} else {
		for (int i=0; i<4; i++) {
			newp[i].x = (p[i].x*cos_angle - p[i].y*sin_angle);
			newp[i].y = (p[i].x*sin_angle + p[i].y*cos_angle);
		}//for i
		
		if (optimizeRightAngles) { 
			//For rotations of 90, -90 or 180 or 0 degrees, call faster routines
			if (newp[3].Distance(CxPoint2(GetHeight()-0.5f, 0.5f-GetWidth())) < 0.25) 
				//rotation right for circa 90 degrees (diagonal pixels less than 0.25 pixel away from 90 degree rotation destination)
				return RotateRight(iDst);
			if (newp[3].Distance(CxPoint2(0.5f-GetHeight(), -0.5f+GetWidth())) < 0.25) 
				//rotation left for ~90 degrees
				return RotateLeft(iDst);
			if (newp[3].Distance(CxPoint2(0.5f-GetWidth(), 0.5f-GetHeight())) < 0.25) 
				//rotation left for ~180 degrees
				return Rotate180(iDst);
			if (newp[3].Distance(p[3]) < 0.25) {
				//rotation not significant
				if (iDst) iDst->Copy(*this);		//copy image to iDst, if required
				return true;						//and we're done
			}//if
		}//if
	}//if

	//(read new dimensions from location of corners)
	float minx = (float) min(min(newp[0].x,newp[1].x),min(newp[2].x,newp[3].x));
	float miny = (float) min(min(newp[0].y,newp[1].y),min(newp[2].y,newp[3].y));
	float maxx = (float) max(max(newp[0].x,newp[1].x),max(newp[2].x,newp[3].x));
	float maxy = (float) max(max(newp[0].y,newp[1].y),max(newp[2].y,newp[3].y));
	int newWidth = (int) floor(maxx-minx+0.5f);
	int newHeight= (int) floor(maxy-miny+0.5f);
	float ssx=((maxx+minx)- ((float) newWidth-1))/2.0f;   //start for x
	float ssy=((maxy+miny)- ((float) newHeight-1))/2.0f;  //start for y

	float newxcenteroffset = 0.5f * newWidth;
	float newycenteroffset = 0.5f * newHeight;
	if (bKeepOriginalSize){
		ssx -= 0.5f * GetWidth();
		ssy -= 0.5f * GetHeight();
	}

	//create destination image
	CxImage imgDest;
	imgDest.CopyInfo(*this);
	imgDest.Create(newWidth,newHeight,GetBpp(),GetType());
	imgDest.SetPalette(GetPalette());
#if CXIMAGE_SUPPORT_ALPHA
	if(AlphaIsValid()) imgDest.AlphaCreate(); //MTA: Fix for rotation problem when the image has an alpha channel
#endif //CXIMAGE_SUPPORT_ALPHA
	
	RGBQUAD rgb;			//pixel colour
	RGBQUAD rc;
	if (replColor!=0) 
		rc=*replColor; 
	else {
		rc.rgbRed=255; rc.rgbGreen=255; rc.rgbBlue=255; rc.rgbReserved=0;
	}//if
	float x,y;              //destination location (float, with proper offset)
	float origx, origy;     //origin location
	int destx, desty;       //destination location
	
	y=ssy;                  //initialize y
	if (!IsIndexed()){ //RGB24
		//optimized RGB24 implementation (direct write to destination):
		BYTE *pxptr;
#if CXIMAGE_SUPPORT_ALPHA
		BYTE *pxptra=0;
#endif //CXIMAGE_SUPPORT_ALPHA
		for (desty=0; desty<newHeight; desty++) {
			info.nProgress = (long)(100*desty/newHeight);
			if (info.nEscape) break;
			//initialize x
			x=ssx;
			//calculate pointer to first byte in row
			pxptr=(BYTE *)imgDest.BlindGetPixelPointer(0, desty);
#if CXIMAGE_SUPPORT_ALPHA
			//calculate pointer to first byte in row
			if (AlphaIsValid()) pxptra=imgDest.AlphaGetPointer(0, desty);
#endif //CXIMAGE_SUPPORT_ALPHA
			for (destx=0; destx<newWidth; destx++) {
				//get source pixel coordinate for current destination point
				//origx = (cos_angle*(x-head.biWidth/2)+sin_angle*(y-head.biHeight/2))+newWidth/2;
				//origy = (cos_angle*(y-head.biHeight/2)-sin_angle*(x-head.biWidth/2))+newHeight/2;
				origx = cos_angle*x+sin_angle*y;
				origy = cos_angle*y-sin_angle*x;
				if (bKeepOriginalSize){
					origx += newxcenteroffset;
					origy += newycenteroffset;
				}
				rgb = GetPixelColorInterpolated(origx, origy, inMethod, ofMethod, &rc);   //get interpolated colour value
				//copy alpha and colour value to destination
#if CXIMAGE_SUPPORT_ALPHA
				if (pxptra) *pxptra++ = rgb.rgbReserved;
#endif //CXIMAGE_SUPPORT_ALPHA
				*pxptr++ = rgb.rgbBlue;
				*pxptr++ = rgb.rgbGreen;
				*pxptr++ = rgb.rgbRed;
				x++;
			}//for destx
			y++;
		}//for desty
	} else { 
		//non-optimized implementation for paletted images
		for (desty=0; desty<newHeight; desty++) {
			info.nProgress = (long)(100*desty/newHeight);
			if (info.nEscape) break;
			x=ssx;
			for (destx=0; destx<newWidth; destx++) {
				//get source pixel coordinate for current destination point
				origx=(cos_angle*x+sin_angle*y);
				origy=(cos_angle*y-sin_angle*x);
				if (bKeepOriginalSize){
					origx += newxcenteroffset;
					origy += newycenteroffset;
				}
				rgb = GetPixelColorInterpolated(origx, origy, inMethod, ofMethod, &rc);
				//***!*** SetPixelColor is slow for palleted images
#if CXIMAGE_SUPPORT_ALPHA
				if (AlphaIsValid()) 
					imgDest.SetPixelColor(destx,desty,rgb,true);
				else 
#endif //CXIMAGE_SUPPORT_ALPHA     
					imgDest.SetPixelColor(destx,desty,rgb,false);
				x++;
			}//for destx
			y++;
		}//for desty
	}
	//select the destination
	
	if (iDst) iDst->Transfer(imgDest);
	else Transfer(imgDest);
	
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Rotate180(CxImage* iDst)
{
	if (!pDib) return false;

	long wid = GetWidth();
	long ht = GetHeight();

	CxImage imgDest;
	imgDest.CopyInfo(*this);
	imgDest.Create(wid,ht,GetBpp(),GetType());
	imgDest.SetPalette(GetPalette());

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid())	imgDest.AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA

	long x,y,y2;
	for (y = 0; y < ht; y++){
		info.nProgress = (long)(100*y/ht); //<Anatoly Ivasyuk>
		y2=ht-y-1;
		for (x = 0; x < wid; x++){
			if(head.biClrUsed==0)//RGB
				imgDest.SetPixelColor(wid-x-1, y2, BlindGetPixelColor(x, y));
			else  //PALETTE
				imgDest.SetPixelIndex(wid-x-1, y2, BlindGetPixelIndex(x, y));

#if CXIMAGE_SUPPORT_ALPHA
			if (AlphaIsValid())	imgDest.AlphaSet(wid-x-1, y2,BlindAlphaGet(x, y));
#endif //CXIMAGE_SUPPORT_ALPHA

		}
	}

	//select the destination
	if (iDst) iDst->Transfer(imgDest);
	else Transfer(imgDest);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * Resizes the image. mode can be 0 for slow (bilinear) method ,
 * 1 for fast (nearest pixel) method, or 2 for accurate (bicubic spline interpolation) method.
 * The function is faster with 24 and 1 bpp images, slow for 4 bpp images and slowest for 8 bpp images.
 */
bool CxImage::Resample(long newx, long newy, int mode, CxImage* iDst)
{
	if (newx==0 || newy==0) return false;

	if (head.biWidth==newx && head.biHeight==newy){
		if (iDst) iDst->Copy(*this);
		return true;
	}

	float xScale, yScale, fX, fY;
	xScale = (float)head.biWidth  / (float)newx;
	yScale = (float)head.biHeight / (float)newy;

	CxImage newImage;
	newImage.CopyInfo(*this);
	newImage.Create(newx,newy,head.biBitCount,GetType());
	newImage.SetPalette(GetPalette());
	if (!newImage.IsValid()){
		strcpy(info.szLastError,newImage.GetLastError());
		return false;
	}

	switch (mode) {
	case 1: // nearest pixel
	{ 
		for(long y=0; y<newy; y++){
			info.nProgress = (long)(100*y/newy);
			if (info.nEscape) break;
			fY = y * yScale;
			for(long x=0; x<newx; x++){
				fX = x * xScale;
				newImage.SetPixelColor(x,y,GetPixelColor((long)fX,(long)fY));
			}
		}
		break;
	}
	case 2: // bicubic interpolation by Blake L. Carlson <blake-carlson(at)uiowa(dot)edu
	{
		float f_x, f_y, a, b, rr, gg, bb, r1, r2;
		int   i_x, i_y, xx, yy;
		RGBQUAD rgb;
		BYTE* iDst;
		for(long y=0; y<newy; y++){
			info.nProgress = (long)(100*y/newy);
			if (info.nEscape) break;
			f_y = (float) y * yScale - 0.5f;
			i_y = (int) floor(f_y);
			a   = f_y - (float)floor(f_y);
			for(long x=0; x<newx; x++){
				f_x = (float) x * xScale - 0.5f;
				i_x = (int) floor(f_x);
				b   = f_x - (float)floor(f_x);

				rr = gg = bb = 0.0f;
				for(int m=-1; m<3; m++) {
					r1 = KernelBSpline((float) m - a);
					yy = i_y+m;
					if (yy<0) yy=0;
					if (yy>=head.biHeight) yy = head.biHeight-1;
					for(int n=-1; n<3; n++) {
						r2 = r1 * KernelBSpline(b - (float)n);
						xx = i_x+n;
						if (xx<0) xx=0;
						if (xx>=head.biWidth) xx=head.biWidth-1;

						if (head.biClrUsed){
							rgb = GetPixelColor(xx,yy);
						} else {
							iDst  = info.pImage + yy*info.dwEffWidth + xx*3;
							rgb.rgbBlue = *iDst++;
							rgb.rgbGreen= *iDst++;
							rgb.rgbRed  = *iDst;
						}

						rr += rgb.rgbRed * r2;
						gg += rgb.rgbGreen * r2;
						bb += rgb.rgbBlue * r2;
					}
				}

				if (head.biClrUsed)
					newImage.SetPixelColor(x,y,RGB(rr,gg,bb));
				else {
					iDst = newImage.info.pImage + y*newImage.info.dwEffWidth + x*3;
					*iDst++ = (BYTE)bb;
					*iDst++ = (BYTE)gg;
					*iDst   = (BYTE)rr;
				}

			}
		}
		break;
	}
	default: // bilinear interpolation
		if (!(head.biWidth>newx && head.biHeight>newy && head.biBitCount==24)) {
			// (c) 1999 Steve McMahon (steve@dogma.demon.co.uk)
			long ifX, ifY, ifX1, ifY1, xmax, ymax;
			float ir1, ir2, ig1, ig2, ib1, ib2, dx, dy;
			BYTE r,g,b;
			RGBQUAD rgb1, rgb2, rgb3, rgb4;
			xmax = head.biWidth-1;
			ymax = head.biHeight-1;
			for(long y=0; y<newy; y++){
				info.nProgress = (long)(100*y/newy);
				if (info.nEscape) break;
				fY = y * yScale;
				ifY = (int)fY;
				ifY1 = min(ymax, ifY+1);
				dy = fY - ifY;
				for(long x=0; x<newx; x++){
					fX = x * xScale;
					ifX = (int)fX;
					ifX1 = min(xmax, ifX+1);
					dx = fX - ifX;
					// Interpolate using the four nearest pixels in the source
					if (head.biClrUsed){
						rgb1=GetPaletteColor(GetPixelIndex(ifX,ifY));
						rgb2=GetPaletteColor(GetPixelIndex(ifX1,ifY));
						rgb3=GetPaletteColor(GetPixelIndex(ifX,ifY1));
						rgb4=GetPaletteColor(GetPixelIndex(ifX1,ifY1));
					}
					else {
						BYTE* iDst;
						iDst = info.pImage + ifY*info.dwEffWidth + ifX*3;
						rgb1.rgbBlue = *iDst++;	rgb1.rgbGreen= *iDst++;	rgb1.rgbRed =*iDst;
						iDst = info.pImage + ifY*info.dwEffWidth + ifX1*3;
						rgb2.rgbBlue = *iDst++;	rgb2.rgbGreen= *iDst++;	rgb2.rgbRed =*iDst;
						iDst = info.pImage + ifY1*info.dwEffWidth + ifX*3;
						rgb3.rgbBlue = *iDst++;	rgb3.rgbGreen= *iDst++;	rgb3.rgbRed =*iDst;
						iDst = info.pImage + ifY1*info.dwEffWidth + ifX1*3;
						rgb4.rgbBlue = *iDst++;	rgb4.rgbGreen= *iDst++;	rgb4.rgbRed =*iDst;
					}
					// Interplate in x direction:
					ir1 = rgb1.rgbRed   + (rgb3.rgbRed   - rgb1.rgbRed)   * dy;
					ig1 = rgb1.rgbGreen + (rgb3.rgbGreen - rgb1.rgbGreen) * dy;
					ib1 = rgb1.rgbBlue  + (rgb3.rgbBlue  - rgb1.rgbBlue)  * dy;
					ir2 = rgb2.rgbRed   + (rgb4.rgbRed   - rgb2.rgbRed)   * dy;
					ig2 = rgb2.rgbGreen + (rgb4.rgbGreen - rgb2.rgbGreen) * dy;
					ib2 = rgb2.rgbBlue  + (rgb4.rgbBlue  - rgb2.rgbBlue)  * dy;
					// Interpolate in y:
					r = (BYTE)(ir1 + (ir2-ir1) * dx);
					g = (BYTE)(ig1 + (ig2-ig1) * dx);
					b = (BYTE)(ib1 + (ib2-ib1) * dx);
					// Set output
					newImage.SetPixelColor(x,y,RGB(r,g,b));
				}
			} 
		} else {
			//high resolution shrink, thanks to Henrik Stellmann <henrik.stellmann@volleynet.de>
			const long ACCURACY = 1000;
			long i,j; // index for faValue
			long x,y; // coordinates in  source image
			BYTE* pSource;
			BYTE* pDest = newImage.info.pImage;
			long* naAccu  = new long[3 * newx + 3];
			long* naCarry = new long[3 * newx + 3];
			long* naTemp;
			long  nWeightX,nWeightY;
			float fEndX;
			long nScale = (long)(ACCURACY * xScale * yScale);

			memset(naAccu,  0, sizeof(long) * 3 * newx);
			memset(naCarry, 0, sizeof(long) * 3 * newx);

			int u, v = 0; // coordinates in dest image
			float fEndY = yScale - 1.0f;
			for (y = 0; y < head.biHeight; y++){
				info.nProgress = (long)(100*y/head.biHeight); //<Anatoly Ivasyuk>
				if (info.nEscape) break;
				pSource = info.pImage + y * info.dwEffWidth;
				u = i = 0;
				fEndX = xScale - 1.0f;
				if ((float)y < fEndY) {       // complete source row goes into dest row
					for (x = 0; x < head.biWidth; x++){
						if ((float)x < fEndX){       // complete source pixel goes into dest pixel
							for (j = 0; j < 3; j++)	naAccu[i + j] += (*pSource++) * ACCURACY;
						} else {       // source pixel is splitted for 2 dest pixels
							nWeightX = (long)(((float)x - fEndX) * ACCURACY);
							for (j = 0; j < 3; j++){
								naAccu[i] += (ACCURACY - nWeightX) * (*pSource);
								naAccu[3 + i++] += nWeightX * (*pSource++);
							}
							fEndX += xScale;
							u++;
						}
					}
				} else {       // source row is splitted for 2 dest rows       
					nWeightY = (long)(((float)y - fEndY) * ACCURACY);
					for (x = 0; x < head.biWidth; x++){
						if ((float)x < fEndX){       // complete source pixel goes into 2 pixel
							for (j = 0; j < 3; j++){
								naAccu[i + j] += ((ACCURACY - nWeightY) * (*pSource));
								naCarry[i + j] += nWeightY * (*pSource++);
							}
						} else {       // source pixel is splitted for 4 dest pixels
							nWeightX = (int)(((float)x - fEndX) * ACCURACY);
							for (j = 0; j < 3; j++) {
								naAccu[i] += ((ACCURACY - nWeightY) * (ACCURACY - nWeightX)) * (*pSource) / ACCURACY;
								*pDest++ = (BYTE)(naAccu[i] / nScale);
								naCarry[i] += (nWeightY * (ACCURACY - nWeightX) * (*pSource)) / ACCURACY;
								naAccu[i + 3] += ((ACCURACY - nWeightY) * nWeightX * (*pSource)) / ACCURACY;
								naCarry[i + 3] = (nWeightY * nWeightX * (*pSource)) / ACCURACY;
								i++;
								pSource++;
							}
							fEndX += xScale;
							u++;
						}
					}
					if (u < newx){ // possibly not completed due to rounding errors
						for (j = 0; j < 3; j++) *pDest++ = (BYTE)(naAccu[i++] / nScale);
					}
					naTemp = naCarry;
					naCarry = naAccu;
					naAccu = naTemp;
					memset(naCarry, 0, sizeof(int) * 3);    // need only to set first pixel zero
					pDest = newImage.info.pImage + (++v * newImage.info.dwEffWidth);
					fEndY += yScale;
				}
			}
			if (v < newy){	// possibly not completed due to rounding errors
				for (i = 0; i < 3 * newx; i++) *pDest++ = (BYTE)(naAccu[i] / nScale);
			}
			delete [] naAccu;
			delete [] naCarry;
		}
	}

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()){
		newImage.AlphaCreate();
		for(long y=0; y<newy; y++){
			fY = y * yScale;
			for(long x=0; x<newx; x++){
				fX = x * xScale;
				newImage.AlphaSet(x,y,AlphaGet((long)fX,(long)fY));
			}
		}
	}
#endif //CXIMAGE_SUPPORT_ALPHA

	//select the destination
	if (iDst) iDst->Transfer(newImage);
	else Transfer(newImage);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * New simpler resample. Adds new interpolation methods and simplifies code (using GetPixelColorInterpolated
 * and GetAreaColorInterpolated). It also (unlike old method) interpolates alpha layer. 
 *
 * \param  newx, newy - size of resampled image
 * \param  inMethod - interpolation method to use (see comments at GetPixelColorInterpolated)
 *              If image size is being reduced, averaging is used instead (or simultaneously with) inMethod.
 * \param  ofMethod - what to replace outside pixels by (only significant for bordering pixels of enlarged image)
 * \param  iDst - pointer to destination CxImage or NULL.
 * \param  disableAveraging - force no averaging when shrinking images (Produces aliasing.
 *                      You probably just want to leave this off...)
 *
 * \author ***bd*** 2.2004
 */
bool CxImage::Resample2(
  long newx, long newy, 
  InterpolationMethod const inMethod, 
  OverflowMethod const ofMethod, 
  CxImage* const iDst,
  bool const disableAveraging)
{
	if (newx<=0 || newy<=0 || !pDib) return false;
	
	if (head.biWidth==newx && head.biHeight==newy) {
		//image already correct size (just copy and return)
		if (iDst) iDst->Copy(*this);
		return true;
	}//if
	
	//calculate scale of new image (less than 1 for enlarge)
	float xScale, yScale;
	xScale = (float)head.biWidth  / (float)newx;    
	yScale = (float)head.biHeight / (float)newy;
	
	//create temporary destination image
	CxImage newImage;
	newImage.CopyInfo(*this);
	newImage.Create(newx,newy,head.biBitCount,GetType());
	newImage.SetPalette(GetPalette());
	if (!newImage.IsValid()){
		strcpy(info.szLastError,newImage.GetLastError());
		return false;
	}
	
	//and alpha channel if required
#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()) newImage.AlphaCreate();
	BYTE *pxptra = 0;	// destination alpha data
#endif
	
	float sX, sY;         //source location
	long dX,dY;           //destination pixel (int value)
	if ((xScale<=1 && yScale<=1) || disableAveraging) {
		//image is being enlarged (or interpolation on demand)
		if (!IsIndexed()) {
			//RGB24 image (optimized version with direct writes)
			RGBQUAD q;              //pixel colour
			BYTE *pxptr;            //pointer to destination pixel
			for(dY=0; dY<newy; dY++){
				info.nProgress = (long)(100*dY/newy);
				if (info.nEscape) break;
				sY = (dY + 0.5f) * yScale - 0.5f;
				pxptr=(BYTE*)(newImage.BlindGetPixelPointer(0,dY));
#if CXIMAGE_SUPPORT_ALPHA
				pxptra=newImage.AlphaGetPointer(0,dY);
#endif
				for(dX=0; dX<newx; dX++){
					sX = (dX + 0.5f) * xScale - 0.5f;
					q=GetPixelColorInterpolated(sX,sY,inMethod,ofMethod,0);
					*pxptr++=q.rgbBlue;
					*pxptr++=q.rgbGreen;
					*pxptr++=q.rgbRed;
#if CXIMAGE_SUPPORT_ALPHA
					if (pxptra) *pxptra++=q.rgbReserved;
#endif
				}//for dX
			}//for dY
		} else {
			//enlarge paletted image. Slower method.
			for(dY=0; dY<newy; dY++){
				info.nProgress = (long)(100*dY/newy);
				if (info.nEscape) break;
				sY = (dY + 0.5f) * yScale - 0.5f;
				for(dX=0; dX<newx; dX++){
					sX = (dX + 0.5f) * xScale - 0.5f;
					newImage.SetPixelColor(dX,dY,GetPixelColorInterpolated(sX,sY,inMethod,ofMethod,0),true);
				}//for x
			}//for y
		}//if
	} else {
		//image size is being reduced (averaging enabled)
		for(dY=0; dY<newy; dY++){
			info.nProgress = (long)(100*dY/newy); if (info.nEscape) break;
			sY = (dY+0.5f) * yScale - 0.5f;
			for(dX=0; dX<newx; dX++){
				sX = (dX+0.5f) * xScale - 0.5f;
				newImage.SetPixelColor(dX,dY,GetAreaColorInterpolated(sX, sY, xScale, yScale, inMethod, ofMethod,0),true);
			}//for x
		}//for y
	}//if

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid() && pxptra == 0){
		for(long y=0; y<newy; y++){
			dY = (long)(y * yScale);
			for(long x=0; x<newx; x++){
				dX = (long)(x * xScale);
				newImage.AlphaSet(x,y,AlphaGet(dX,dY));
			}
		}
	}
#endif //CXIMAGE_SUPPORT_ALPHA

	//copy new image to the destination
	if (iDst) 
		iDst->Transfer(newImage);
	else 
		Transfer(newImage);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Reduces the number of bits per pixel to nbit (1, 4 or 8).
 * ppal points to a valid palette for the final image; if not supplied the function will use a standard palette.
 * ppal is not necessary for reduction to 1 bpp.
 */
bool CxImage::DecreaseBpp(DWORD nbit, bool errordiffusion, RGBQUAD* ppal, DWORD clrimportant)
{
	if (!pDib) return false;
	if (head.biBitCount <  nbit){
		strcpy(info.szLastError,"DecreaseBpp: target BPP greater than source BPP");
		return false;
	}
	if (head.biBitCount == nbit){
		if (clrimportant==0) return true;
		if (head.biClrImportant && (head.biClrImportant<clrimportant)) return true;
	}

	long er,eg,eb;
	RGBQUAD c,ce;

	CxImage tmp;
	tmp.CopyInfo(*this);
	tmp.Create(head.biWidth,head.biHeight,(WORD)nbit,info.dwType);
	if (clrimportant) tmp.SetClrImportant(clrimportant);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

#if CXIMAGE_SUPPORT_SELECTION
	tmp.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
	tmp.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA

	if (ppal) {
		if (clrimportant) {
			tmp.SetPalette(ppal,clrimportant);
		} else {
			tmp.SetPalette(ppal,1<<tmp.head.biBitCount);
		}
	} else {
		tmp.SetStdPalette();
	}

	for (long y=0;y<head.biHeight;y++){
		if (info.nEscape) break;
		info.nProgress = (long)(100*y/head.biHeight);
		for (long x=0;x<head.biWidth;x++){
			if (!errordiffusion){
				tmp.BlindSetPixelColor(x,y,BlindGetPixelColor(x,y));
			} else {
				c = BlindGetPixelColor(x,y);
				tmp.BlindSetPixelColor(x,y,c);

				ce = tmp.BlindGetPixelColor(x,y);
				er=(long)c.rgbRed - (long)ce.rgbRed;
				eg=(long)c.rgbGreen - (long)ce.rgbGreen;
				eb=(long)c.rgbBlue - (long)ce.rgbBlue;

				c = GetPixelColor(x+1,y);
				c.rgbRed = (BYTE)min(255L,max(0L,(long)c.rgbRed + ((er*7)/16)));
				c.rgbGreen = (BYTE)min(255L,max(0L,(long)c.rgbGreen + ((eg*7)/16)));
				c.rgbBlue = (BYTE)min(255L,max(0L,(long)c.rgbBlue + ((eb*7)/16)));
				SetPixelColor(x+1,y,c);
				int coeff=1;
				for(int i=-1; i<2; i++){
					switch(i){
					case -1:
						coeff=2; break;
					case 0:
						coeff=4; break;
					case 1:
						coeff=1; break;
					}
					c = GetPixelColor(x+i,y+1);
					c.rgbRed = (BYTE)min(255L,max(0L,(long)c.rgbRed + ((er * coeff)/16)));
					c.rgbGreen = (BYTE)min(255L,max(0L,(long)c.rgbGreen + ((eg * coeff)/16)));
					c.rgbBlue = (BYTE)min(255L,max(0L,(long)c.rgbBlue + ((eb * coeff)/16)));
					SetPixelColor(x+i,y+1,c);
				}
			}
		}
	}

	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Increases the number of bits per pixel of the image.
 * \param nbit: 4, 8, 24
 */
bool CxImage::IncreaseBpp(DWORD nbit)
{
	if (!pDib) return false;
	switch (nbit){
	case 4:
		{
			if (head.biBitCount==4) return true;
			if (head.biBitCount>4) return false;

			CxImage tmp;
			tmp.CopyInfo(*this);
			tmp.Create(head.biWidth,head.biHeight,4,info.dwType);
			tmp.SetPalette(GetPalette(),GetNumColors());
			if (!tmp.IsValid()){
				strcpy(info.szLastError,tmp.GetLastError());
				return false;
			}


#if CXIMAGE_SUPPORT_SELECTION
			tmp.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
			tmp.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA

			for (long y=0;y<head.biHeight;y++){
				if (info.nEscape) break;
				for (long x=0;x<head.biWidth;x++){
					tmp.BlindSetPixelIndex(x,y,BlindGetPixelIndex(x,y));
				}
			}
			Transfer(tmp);
			return true;
		}
	case 8:
		{
			if (head.biBitCount==8) return true;
			if (head.biBitCount>8) return false;

			CxImage tmp;
			tmp.CopyInfo(*this);
			tmp.Create(head.biWidth,head.biHeight,8,info.dwType);
			tmp.SetPalette(GetPalette(),GetNumColors());
			if (!tmp.IsValid()){
				strcpy(info.szLastError,tmp.GetLastError());
				return false;
			}

#if CXIMAGE_SUPPORT_SELECTION
			tmp.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
			tmp.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA

			for (long y=0;y<head.biHeight;y++){
				if (info.nEscape) break;
				for (long x=0;x<head.biWidth;x++){
					tmp.BlindSetPixelIndex(x,y,BlindGetPixelIndex(x,y));
				}
			}
			Transfer(tmp);
			return true;
		}
	case 24:
		{
			if (head.biBitCount==24) return true;
			if (head.biBitCount>24) return false;

			CxImage tmp;
			tmp.CopyInfo(*this);
			tmp.Create(head.biWidth,head.biHeight,24,info.dwType);
			if (!tmp.IsValid()){
				strcpy(info.szLastError,tmp.GetLastError());
				return false;
			}

			if (info.nBkgndIndex>=0) //translate transparency
				tmp.info.nBkgndColor=GetPaletteColor((BYTE)info.nBkgndIndex);

#if CXIMAGE_SUPPORT_SELECTION
			tmp.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
			tmp.AlphaCopy(*this);
			if (AlphaPaletteIsValid() && !AlphaIsValid()) tmp.AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA

			for (long y=0;y<head.biHeight;y++){
				if (info.nEscape) break;
				for (long x=0;x<head.biWidth;x++){
					tmp.BlindSetPixelColor(x,y,BlindGetPixelColor(x,y),true);
				}
			}
			Transfer(tmp);
			return true;
		}
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Converts the image to B&W using the desired method :
 * - 0 = Floyd-Steinberg
 * - 1 = Ordered-Dithering (4x4) 
 * - 2 = Burkes
 * - 3 = Stucki
 * - 4 = Jarvis-Judice-Ninke
 * - 5 = Sierra
 * - 6 = Stevenson-Arce
 * - 7 = Bayer (4x4 ordered dithering) 
 */
bool CxImage::Dither(long method)
{
	if (!pDib) return false;
	if (head.biBitCount == 1) return true;
	
	GrayScale();

	CxImage tmp;
	tmp.CopyInfo(*this);
	tmp.Create(head.biWidth, head.biHeight, 1, info.dwType);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

#if CXIMAGE_SUPPORT_SELECTION
	tmp.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
	tmp.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA

	switch (method){
	case 1:
	{
		// Multi-Level Ordered-Dithering by Kenny Hoff (Oct. 12, 1995)
		#define dth_NumRows 4
		#define dth_NumCols 4
		#define dth_NumIntensityLevels 2
		#define dth_NumRowsLessOne (dth_NumRows-1)
		#define dth_NumColsLessOne (dth_NumCols-1)
		#define dth_RowsXCols (dth_NumRows*dth_NumCols)
		#define dth_MaxIntensityVal 255
		#define dth_MaxDitherIntensityVal (dth_NumRows*dth_NumCols*(dth_NumIntensityLevels-1))

		int DitherMatrix[dth_NumRows][dth_NumCols] = {{0,8,2,10}, {12,4,14,6}, {3,11,1,9}, {15,7,13,5} };
		
		unsigned char Intensity[dth_NumIntensityLevels] = { 0,1 };                       // 2 LEVELS B/W
		//unsigned char Intensity[NumIntensityLevels] = { 0,255 };                       // 2 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,127,255 };                   // 3 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,85,170,255 };                // 4 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,63,127,191,255 };            // 5 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,51,102,153,204,255 };        // 6 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,42,85,127,170,213,255 };     // 7 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,36,73,109,145,182,219,255 }; // 8 LEVELS
		int DitherIntensity, DitherMatrixIntensity, Offset, DeviceIntensity;
		unsigned char DitherValue;
  
		for (long y=0;y<head.biHeight;y++){
			info.nProgress = (long)(100*y/head.biHeight);
			if (info.nEscape) break;
			for (long x=0;x<head.biWidth;x++){

				DeviceIntensity = BlindGetPixelIndex(x,y);
				DitherIntensity = DeviceIntensity*dth_MaxDitherIntensityVal/dth_MaxIntensityVal;
				DitherMatrixIntensity = DitherIntensity % dth_RowsXCols;
				Offset = DitherIntensity / dth_RowsXCols;
				if (DitherMatrix[y&dth_NumRowsLessOne][x&dth_NumColsLessOne] < DitherMatrixIntensity)
					DitherValue = Intensity[1+Offset];
				else
					DitherValue = Intensity[0+Offset];

				tmp.BlindSetPixelIndex(x,y,DitherValue);
			}
		}
		break;
	}
	case 2:
	{
		//Burkes error diffusion (Thanks to Franco Gerevini)
		int TotalCoeffSum = 32;
		long error, nlevel, coeff=1;
		BYTE level;

		for (long y = 0; y < head.biHeight; y++) {
			info.nProgress = (long)(100 * y / head.biHeight);
			if (info.nEscape) 
				break;
			for (long x = 0; x < head.biWidth; x++) {
				level = BlindGetPixelIndex(x, y);
				if (level > 128) {
					tmp.SetPixelIndex(x, y, 1);
					error = level - 255;
				} else {
					tmp.SetPixelIndex(x, y, 0);
					error = level;
				}

				nlevel = GetPixelIndex(x + 1, y) + (error * 8) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(x + 1, y, level);
				nlevel = GetPixelIndex(x + 2, y) + (error * 4) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(x + 2, y, level);
				int i;
				for (i = -2; i < 3; i++) {
					switch (i) {
					case -2:
						coeff = 2;
						break;
					case -1:
						coeff = 4;
						break;
					case 0:
						coeff = 8; 
						break;
					case 1:
						coeff = 4; 
						break;
					case 2:
						coeff = 2; 
						break;
					}
					nlevel = GetPixelIndex(x + i, y + 1) + (error * coeff) / TotalCoeffSum;
					level = (BYTE)min(255, max(0, (int)nlevel));
					SetPixelIndex(x + i, y + 1, level);
				}
			}
		}
		break;
	}
	case 3:
	{
		//Stucki error diffusion (Thanks to Franco Gerevini)
		int TotalCoeffSum = 42;
		long error, nlevel, coeff=1;
		BYTE level;

		for (long y = 0; y < head.biHeight; y++) {
			info.nProgress = (long)(100 * y / head.biHeight);
			if (info.nEscape) 
				break;
			for (long x = 0; x < head.biWidth; x++) {
				level = BlindGetPixelIndex(x, y);
				if (level > 128) {
					tmp.SetPixelIndex(x, y, 1);
					error = level - 255;
				} else {
					tmp.SetPixelIndex(x, y, 0);
					error = level;
				}

				nlevel = GetPixelIndex(x + 1, y) + (error * 8) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(x + 1, y, level);
				nlevel = GetPixelIndex(x + 2, y) + (error * 4) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(x + 2, y, level);
				int i;
				for (i = -2; i < 3; i++) {
					switch (i) {
					case -2:
						coeff = 2;
						break;
					case -1:
						coeff = 4;
						break;
					case 0:
						coeff = 8; 
						break;
					case 1:
						coeff = 4; 
						break;
					case 2:
						coeff = 2; 
						break;
					}
					nlevel = GetPixelIndex(x + i, y + 1) + (error * coeff) / TotalCoeffSum;
					level = (BYTE)min(255, max(0, (int)nlevel));
					SetPixelIndex(x + i, y + 1, level);
				}
				for (i = -2; i < 3; i++) {
					switch (i) {
					case -2:
						coeff = 1;
						break;
					case -1:
						coeff = 2;
						break;
					case 0:
						coeff = 4; 
						break;
					case 1:
						coeff = 2; 
						break;
					case 2:
						coeff = 1; 
						break;
					}
					nlevel = GetPixelIndex(x + i, y + 2) + (error * coeff) / TotalCoeffSum;
					level = (BYTE)min(255, max(0, (int)nlevel));
					SetPixelIndex(x + i, y + 2, level);
				}
			}
		}
		break;
	}
	case 4:
	{
		//Jarvis, Judice and Ninke error diffusion (Thanks to Franco Gerevini)
		int TotalCoeffSum = 48;
		long error, nlevel, coeff=1;
		BYTE level;

		for (long y = 0; y < head.biHeight; y++) {
			info.nProgress = (long)(100 * y / head.biHeight);
			if (info.nEscape) 
				break;
			for (long x = 0; x < head.biWidth; x++) {
				level = BlindGetPixelIndex(x, y);
				if (level > 128) {
					tmp.SetPixelIndex(x, y, 1);
					error = level - 255;
				} else {
					tmp.SetPixelIndex(x, y, 0);
					error = level;
				}

				nlevel = GetPixelIndex(x + 1, y) + (error * 7) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(x + 1, y, level);
				nlevel = GetPixelIndex(x + 2, y) + (error * 5) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(x + 2, y, level);
				int i;
				for (i = -2; i < 3; i++) {
					switch (i) {
					case -2:
						coeff = 3;
						break;
					case -1:
						coeff = 5;
						break;
					case 0:
						coeff = 7; 
						break;
					case 1:
						coeff = 5; 
						break;
					case 2:
						coeff = 3; 
						break;
					}
					nlevel = GetPixelIndex(x + i, y + 1) + (error * coeff) / TotalCoeffSum;
					level = (BYTE)min(255, max(0, (int)nlevel));
					SetPixelIndex(x + i, y + 1, level);
				}
				for (i = -2; i < 3; i++) {
					switch (i) {
					case -2:
						coeff = 1;
						break;
					case -1:
						coeff = 3;
						break;
					case 0:
						coeff = 5; 
						break;
					case 1:
						coeff = 3; 
						break;
					case 2:
						coeff = 1; 
						break;
					}
					nlevel = GetPixelIndex(x + i, y + 2) + (error * coeff) / TotalCoeffSum;
					level = (BYTE)min(255, max(0, (int)nlevel));
					SetPixelIndex(x + i, y + 2, level);
				}
			}
		}
		break;
	}
	case 5:
	{
		//Sierra error diffusion (Thanks to Franco Gerevini)
		int TotalCoeffSum = 32;
		long error, nlevel, coeff=1;
		BYTE level;

		for (long y = 0; y < head.biHeight; y++) {
			info.nProgress = (long)(100 * y / head.biHeight);
			if (info.nEscape) 
				break;
			for (long x = 0; x < head.biWidth; x++) {
				level = BlindGetPixelIndex(x, y);
				if (level > 128) {
					tmp.SetPixelIndex(x, y, 1);
					error = level - 255;
				} else {
					tmp.SetPixelIndex(x, y, 0);
					error = level;
				}

				nlevel = GetPixelIndex(x + 1, y) + (error * 5) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(x + 1, y, level);
				nlevel = GetPixelIndex(x + 2, y) + (error * 3) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(x + 2, y, level);
				int i;
				for (i = -2; i < 3; i++) {
					switch (i) {
					case -2:
						coeff = 2;
						break;
					case -1:
						coeff = 4;
						break;
					case 0:
						coeff = 5; 
						break;
					case 1:
						coeff = 4; 
						break;
					case 2:
						coeff = 2; 
						break;
					}
					nlevel = GetPixelIndex(x + i, y + 1) + (error * coeff) / TotalCoeffSum;
					level = (BYTE)min(255, max(0, (int)nlevel));
					SetPixelIndex(x + i, y + 1, level);
				}
				for (i = -1; i < 2; i++) {
					switch (i) {
					case -1:
						coeff = 2;
						break;
					case 0:
						coeff = 3; 
						break;
					case 1:
						coeff = 2; 
						break;
					}
					nlevel = GetPixelIndex(x + i, y + 2) + (error * coeff) / TotalCoeffSum;
					level = (BYTE)min(255, max(0, (int)nlevel));
					SetPixelIndex(x + i, y + 2, level);
				}
			}
		}
		break;
	}
	case 6:
	{
		//Stevenson and Arce error diffusion (Thanks to Franco Gerevini)
		int TotalCoeffSum = 200;
		long error, nlevel;
		BYTE level;

		for (long y = 0; y < head.biHeight; y++) {
			info.nProgress = (long)(100 * y / head.biHeight);
			if (info.nEscape) 
				break;
			for (long x = 0; x < head.biWidth; x++) {
				level = BlindGetPixelIndex(x, y);
				if (level > 128) {
					tmp.SetPixelIndex(x, y, 1);
					error = level - 255;
				} else {
					tmp.SetPixelIndex(x, y, 0);
					error = level;
				}

				int tmp_index_x = x + 2;
				int tmp_index_y = y;
				int tmp_coeff = 32;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);

				tmp_index_x = x - 3;
				tmp_index_y = y + 1;
				tmp_coeff = 12;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);

				tmp_index_x = x - 1;
				tmp_coeff = 26;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);

				tmp_index_x = x + 1;
				tmp_coeff = 30;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);

				tmp_index_x = x + 3;
				tmp_coeff = 16;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);

				tmp_index_x = x - 2;
				tmp_index_y = y + 2;
				tmp_coeff = 12;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);

				tmp_index_x = x;
				tmp_coeff = 26;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);

				tmp_index_x = x + 2;
				tmp_coeff = 12;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);

				tmp_index_x = x - 3;
				tmp_index_y = y + 3;
				tmp_coeff = 5;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);

				tmp_index_x = x - 1;
				tmp_coeff = 12;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);

				tmp_index_x = x + 1;
				tmp_coeff = 12;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);

				tmp_index_x = x + 3;
				tmp_coeff = 5;
				nlevel = GetPixelIndex(tmp_index_x, tmp_index_y) + (error * tmp_coeff) / TotalCoeffSum;
				level = (BYTE)min(255, max(0, (int)nlevel));
				SetPixelIndex(tmp_index_x, tmp_index_y, level);
			}
		}
		break;
	}
	case 7:
	{
		// Bayer ordered dither
		int order = 4;
		//create Bayer matrix
		if (order>4) order = 4;
		int size = (1 << (2*order));
		BYTE* Bmatrix = (BYTE*) malloc(size * sizeof(BYTE));
		for(int i = 0; i < size; i++) {
			int n = order;
			int x = i / n;
			int y = i % n;
			int dither = 0;
			while (n-- > 0){
				dither = (((dither<<1)|((x&1) ^ (y&1)))<<1) | (y&1);
				x >>= 1;
				y >>= 1;
			}
			Bmatrix[i] = (BYTE)(dither);
		}

		int scale = max(0,(8-2*order));
		int level;
		for (long y=0;y<head.biHeight;y++){
			info.nProgress = (long)(100*y/head.biHeight);
			if (info.nEscape) break;
			for (long x=0;x<head.biWidth;x++){
				level = BlindGetPixelIndex(x,y) >> scale;
				if(level > Bmatrix[ (x % order) + order * (y % order) ]){
					tmp.SetPixelIndex(x,y,1);
				} else {
					tmp.SetPixelIndex(x,y,0);
				}
			}
		}

		free(Bmatrix);

		break;
	}
	default:
	{
		// Floyd-Steinberg error diffusion (Thanks to Steve McMahon)
		long error,nlevel,coeff=1;
		BYTE level;

		for (long y=0;y<head.biHeight;y++){
			info.nProgress = (long)(100*y/head.biHeight);
			if (info.nEscape) break;
			for (long x=0;x<head.biWidth;x++){

				level = BlindGetPixelIndex(x,y);
				if (level > 128){
					tmp.SetPixelIndex(x,y,1);
					error = level-255;
				} else {
					tmp.SetPixelIndex(x,y,0);
					error = level;
				}

				nlevel = GetPixelIndex(x+1,y) + (error * 7)/16;
				level = (BYTE)min(255,max(0,(int)nlevel));
				SetPixelIndex(x+1,y,level);
				for(int i=-1; i<2; i++){
					switch(i){
					case -1:
						coeff=3; break;
					case 0:
						coeff=5; break;
					case 1:
						coeff=1; break;
					}
					nlevel = GetPixelIndex(x+i,y+1) + (error * coeff)/16;
					level = (BYTE)min(255,max(0,(int)nlevel));
					SetPixelIndex(x+i,y+1,level);
				}
			}
		}
	}
	}

	tmp.SetPaletteColor(0,0,0,0);
	tmp.SetPaletteColor(1,255,255,255);
	Transfer(tmp);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 *	CropRotatedRectangle
 * \param topx,topy : topmost and leftmost point of the rectangle 
          (topmost, and if there are 2 topmost points, the left one)
 * \param  width     : size of the right hand side of rect, from (topx,topy) roundwalking clockwise
 * \param  height    : size of the left hand side of rect, from (topx,topy) roundwalking clockwise
 * \param  angle     : angle of the right hand side of rect, from (topx,topy)
 * \param  iDst      : pointer to destination image (if 0, this image is modified)
 * \author  [VATI]
 */
bool CxImage::CropRotatedRectangle( long topx, long topy, long width, long height, float angle, CxImage* iDst)
{
	if (!pDib) return false;

	
	long startx,starty,endx,endy;
	double cos_angle = cos(angle/*/57.295779513082320877*/);
    double sin_angle = sin(angle/*/57.295779513082320877*/);

	// if there is nothing special, call the original Crop():
	if ( fabs(angle)<0.0002 )
		return Crop( topx, topy, topx+width, topy+height, iDst);

	startx = min(topx, topx - (long)(sin_angle*(double)height));
	endx   = topx + (long)(cos_angle*(double)width);
	endy   = topy + (long)(cos_angle*(double)height + sin_angle*(double)width);
	// check: corners of the rectangle must be inside
	if ( IsInside( startx, topy )==false ||
		 IsInside( endx, endy ) == false )
		 return false;

	// first crop to bounding rectangle
	CxImage tmp(*this, true, false, true);
	// tmp.Copy(*this, true, false, true);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}
    if (!tmp.Crop( startx, topy, endx, endy)){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}
	
	// the midpoint of the image now became the same as the midpoint of the rectangle
	// rotate new image with minus angle amount
    if ( false == tmp.Rotate( (float)(-angle*57.295779513082320877) ) ) // Rotate expects angle in degrees
		return false;

	// crop rotated image to the original selection rectangle
    endx   = (tmp.head.biWidth+width)/2;
	startx = (tmp.head.biWidth-width)/2;
	starty = (tmp.head.biHeight+height)/2;
    endy   = (tmp.head.biHeight-height)/2;
    if ( false == tmp.Crop( startx, starty, endx, endy ) )
		return false;

	if (iDst) iDst->Transfer(tmp);
	else Transfer(tmp);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Crop(const RECT& rect, CxImage* iDst)
{
	return Crop(rect.left, rect.top, rect.right, rect.bottom, iDst);
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Crop(long left, long top, long right, long bottom, CxImage* iDst)
{
	if (!pDib) return false;

	long startx = max(0L,min(left,head.biWidth));
	long endx = max(0L,min(right,head.biWidth));
	long starty = head.biHeight - max(0L,min(top,head.biHeight));
	long endy = head.biHeight - max(0L,min(bottom,head.biHeight));

	if (startx==endx || starty==endy) return false;

	if (startx>endx) {long tmp=startx; startx=endx; endx=tmp;}
	if (starty>endy) {long tmp=starty; starty=endy; endy=tmp;}

	CxImage tmp(endx-startx,endy-starty,head.biBitCount,info.dwType);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	tmp.SetPalette(GetPalette(),head.biClrUsed);
	tmp.info.nBkgndIndex = info.nBkgndIndex;
	tmp.info.nBkgndColor = info.nBkgndColor;

	switch (head.biBitCount) {
	case 1:
	case 4:
	{
		for(long y=starty, yd=0; y<endy; y++, yd++){
			info.nProgress = (long)(100*(y-starty)/(endy-starty)); //<Anatoly Ivasyuk>
			for(long x=startx, xd=0; x<endx; x++, xd++){
				tmp.SetPixelIndex(xd,yd,GetPixelIndex(x,y));
			}
		}
		break;
	}
	case 8:
	case 24:
	{
		int linelen = tmp.head.biWidth * tmp.head.biBitCount >> 3;
		BYTE* pDest = tmp.info.pImage;
		BYTE* pSrc = info.pImage + starty * info.dwEffWidth + (startx*head.biBitCount >> 3);
		for(long y=starty; y<endy; y++){
			info.nProgress = (long)(100*(y-starty)/(endy-starty)); //<Anatoly Ivasyuk>
			memcpy(pDest,pSrc,linelen);
			pDest+=tmp.info.dwEffWidth;
			pSrc+=info.dwEffWidth;
		}
    }
	}

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()){ //<oboolo>
		tmp.AlphaCreate();
		if (!tmp.AlphaIsValid()) return false;
		BYTE* pDest = tmp.pAlpha;
		BYTE* pSrc = pAlpha + startx + starty*head.biWidth;
		for (long y=starty; y<endy; y++){
			memcpy(pDest,pSrc,endx-startx);
			pDest+=tmp.head.biWidth;
			pSrc+=head.biWidth;
		}
	}
#endif //CXIMAGE_SUPPORT_ALPHA

	//select the destination
	if (iDst) iDst->Transfer(tmp);
	else Transfer(tmp);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * \param xgain, ygain : can be from 0 to 1.
 * \param xpivot, ypivot : is the center of the transformation.
 * \param bEnableInterpolation : if true, enables bilinear interpolation.
 * \return true if everything is ok 
 */
bool CxImage::Skew(float xgain, float ygain, long xpivot, long ypivot, bool bEnableInterpolation)
{
	if (!pDib) return false;
	float nx,ny;

	CxImage tmp(*this);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	long xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}
	for(long y=ymin; y<ymax; y++){
		info.nProgress = (long)(100*(y-ymin)/(ymax-ymin));
		if (info.nEscape) break;
		for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				nx = x + (xgain*(y - ypivot));
				ny = y + (ygain*(x - xpivot));
#if CXIMAGE_SUPPORT_INTERPOLATION
				if (bEnableInterpolation){
					tmp.SetPixelColor(x,y,GetPixelColorInterpolated(nx, ny, CxImage::IM_BILINEAR, CxImage::OM_BACKGROUND),true);
				} else
#endif //CXIMAGE_SUPPORT_INTERPOLATION
				{
					if (head.biClrUsed==0){
						tmp.SetPixelColor(x,y,GetPixelColor((long)nx,(long)ny));
					} else {
						tmp.SetPixelIndex(x,y,GetPixelIndex((long)nx,(long)ny));
					}
#if CXIMAGE_SUPPORT_ALPHA
					tmp.AlphaSet(x,y,AlphaGet((long)nx,(long)ny));
#endif //CXIMAGE_SUPPORT_ALPHA
				}
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Expands the borders.
 * \param left, top, right, bottom = additional dimensions, should be greater than 0.
 * \param canvascolor = border color. canvascolor.rgbReserved will set the alpha channel (if any) in the border.
 * \param iDst = pointer to destination image (if it's 0, this image is modified)
 * \return true if everything is ok 
 * \author [Colin Urquhart]; changes [DP]
 */
bool CxImage::Expand(long left, long top, long right, long bottom, RGBQUAD canvascolor, CxImage* iDst)
{
    if (!pDib) return false;

    if ((left < 0) || (right < 0) || (bottom < 0) || (top < 0)) return false;

    long newWidth = head.biWidth + left + right;
    long newHeight = head.biHeight + top + bottom;

    right = left + head.biWidth - 1;
    top = bottom + head.biHeight - 1;
    
    CxImage tmp;
	tmp.CopyInfo(*this);
	if (!tmp.Create(newWidth, newHeight, head.biBitCount, info.dwType)){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

    tmp.SetPalette(GetPalette(),head.biClrUsed);

    switch (head.biBitCount) {
    case 1:
    case 4:
		{
			BYTE pixel = tmp.GetNearestIndex(canvascolor);
			for(long y=0; y < newHeight; y++){
				info.nProgress = (long)(100*y/newHeight);
				for(long x=0; x < newWidth; x++){
					if ((y < bottom) || (y > top) || (x < left) || (x > right)) {
						tmp.SetPixelIndex(x,y, pixel);
					} else {
						tmp.SetPixelIndex(x,y,GetPixelIndex(x-left,y-bottom));
					}
				}
			}
			break;
		}
    case 8:
    case 24:
		{
			if (head.biBitCount == 8) {
				BYTE pixel = tmp.GetNearestIndex( canvascolor);
				memset(tmp.info.pImage, pixel,  + (tmp.info.dwEffWidth * newHeight));
			} else {
				for (long y = 0; y < newHeight; ++y) {
					BYTE *pDest = tmp.info.pImage + (y * tmp.info.dwEffWidth);
					for (long x = 0; x < newWidth; ++x) {
						*pDest++ = canvascolor.rgbBlue;
						*pDest++ = canvascolor.rgbGreen;
						*pDest++ = canvascolor.rgbRed;
					}
				}
			}

			BYTE* pDest = tmp.info.pImage + (tmp.info.dwEffWidth * bottom) + (left*(head.biBitCount >> 3));
			BYTE* pSrc = info.pImage;
			for(long y=bottom; y <= top; y++){
				info.nProgress = (long)(100*y/(1 + top - bottom));
				memcpy(pDest,pSrc,(head.biBitCount >> 3) * (right - left + 1));
				pDest+=tmp.info.dwEffWidth;
				pSrc+=info.dwEffWidth;
			}
		}
    }

#if CXIMAGE_SUPPORT_SELECTION
	if (SelectionIsValid()){
		if (!tmp.SelectionCreate())
			return false;
		BYTE* pSrc = SelectionGetPointer();
		BYTE* pDst = tmp.SelectionGetPointer(left,bottom);
		for(long y=bottom; y <= top; y++){
			memcpy(pDst,pSrc, (right - left + 1));
			pSrc+=head.biWidth;
			pDst+=tmp.head.biWidth;
		}
		tmp.info.rSelectionBox.left = info.rSelectionBox.left + left;
		tmp.info.rSelectionBox.right = info.rSelectionBox.right + left;
		tmp.info.rSelectionBox.top = info.rSelectionBox.top + bottom;
		tmp.info.rSelectionBox.bottom = info.rSelectionBox.bottom + bottom;
	}
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()){
		if (!tmp.AlphaCreate())
			return false;
		tmp.AlphaSet(canvascolor.rgbReserved);
		BYTE* pSrc = AlphaGetPointer();
		BYTE* pDst = tmp.AlphaGetPointer(left,bottom);
		for(long y=bottom; y <= top; y++){
			memcpy(pDst,pSrc, (right - left + 1));
			pSrc+=head.biWidth;
			pDst+=tmp.head.biWidth;
		}
	}
#endif //CXIMAGE_SUPPORT_ALPHA

    //select the destination
	if (iDst) iDst->Transfer(tmp);
    else Transfer(tmp);

    return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Expand(long newx, long newy, RGBQUAD canvascolor, CxImage* iDst)
{
	//thanks to <Colin Urquhart>

    if (!pDib) return false;

    if ((newx < head.biWidth) || (newy < head.biHeight)) return false;

    int nAddLeft = (newx - head.biWidth) / 2;
    int nAddTop = (newy - head.biHeight) / 2;

    return Expand(nAddLeft, nAddTop, newx - (head.biWidth + nAddLeft), newy - (head.biHeight + nAddTop), canvascolor, iDst);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Resamples the image with the correct aspect ratio, and fills the borders.
 * \param newx, newy = thumbnail size.
 * \param canvascolor = border color.
 * \param iDst = pointer to destination image (if it's 0, this image is modified).
 * \return true if everything is ok.
 * \author [Colin Urquhart]
 */
bool CxImage::Thumbnail(long newx, long newy, RGBQUAD canvascolor, CxImage* iDst)
{
    if (!pDib) return false;

    if ((newx <= 0) || (newy <= 0)) return false;

    CxImage tmp(*this);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

    // determine whether we need to shrink the image
    if ((head.biWidth > newx) || (head.biHeight > newy)) {
        float fScale;
        float fAspect = (float) newx / (float) newy;
        if (fAspect * head.biHeight > head.biWidth) {
            fScale = (float) newy / head.biHeight;
        } else {
            fScale = (float) newx / head.biWidth;
        }
        tmp.Resample((long) (fScale * head.biWidth), (long) (fScale * head.biHeight), 0);
    }

    // expand the frame
    tmp.Expand(newx, newy, canvascolor, iDst);

    //select the destination
    if (iDst) iDst->Transfer(tmp);
    else Transfer(tmp);
    return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Perform circle_based transformations.
 * \param type - for different transformations
 * - 0 for normal (proturberant) FishEye
 * - 1 for reverse (concave) FishEye
 * - 2 for Swirle 
 * - 3 for Cilinder mirror
 * - 4 for bathroom
 *
 * \param rmax - effect radius. If 0, the whole image is processed
 * \param Koeff - only for swirle
 * \author Arkadiy Olovyannikov ark(at)msun(dot)ru
 */
bool CxImage::CircleTransform(int type,long rmax,float Koeff)
{
	if (!pDib) return false;

	long nx,ny;
	double angle,radius,rnew;

	CxImage tmp(*this);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	long xmin,xmax,ymin,ymax,xmid,ymid;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}
	
	xmid = (long) (tmp.GetWidth()/2);
	ymid = (long) (tmp.GetHeight()/2);

	if (!rmax) rmax=(long)sqrt((float)((xmid-xmin)*(xmid-xmin)+(ymid-ymin)*(ymid-ymin)));
	if (Koeff==0.0f) Koeff=1.0f;

	for(long y=ymin; y<ymax; y++){
		info.nProgress = (long)(100*(y-ymin)/(ymax-ymin));
		if (info.nEscape) break;
		for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (BlindSelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				nx=xmid-x;
				ny=ymid-y;
				radius=sqrt((float)(nx*nx+ny*ny));
				if (radius<rmax) {
					angle=atan2((double)ny,(double)nx);
					if (type==0)	  rnew=radius*radius/rmax;
					else if (type==1) rnew=sqrt(radius*rmax);
					else if (type==2) {rnew=radius;angle += radius / Koeff;}
					else rnew = 1; // potentially uninitialized
					if (type<3){
						nx = xmid + (long)(rnew * cos(angle));
						ny = ymid - (long)(rnew * sin(angle));
					}
					else if (type==3){
						nx = (long)fabs((angle*xmax/6.2831852));
						ny = (long)fabs((radius*ymax/rmax));
					}
					else {
						nx=x+(x%32)-16;
						ny=y;
					}
//					nx=max(xmin,min(nx,xmax));
//					ny=max(ymin,min(ny,ymax));
				}
				else { nx=-1;ny=-1;}
				if (head.biClrUsed==0){
					tmp.SetPixelColor(x,y,GetPixelColor(nx,ny));
				} else {
					tmp.SetPixelIndex(x,y,GetPixelIndex(nx,ny));
				}
#if CXIMAGE_SUPPORT_ALPHA
				tmp.AlphaSet(x,y,AlphaGet(nx,ny));
#endif //CXIMAGE_SUPPORT_ALPHA
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Faster way to almost properly shrink image. Algorithm produces results comparable with "high resoultion shrink"
 * when resulting image is much smaller (that would be 3 times or more) than original. When
 * resulting image is only slightly smaller, results are closer to nearest pixel.
 * This algorithm works by averaging, but it does not calculate fractions of pixels. It adds whole
 * source pixels to the best destionation. It is not geometrically "correct".
 * It's main advantage over "high" resulution shrink is speed, so it's useful, when speed is most
 * important (preview thumbnails, "map" view, ...).
 * Method is optimized for RGB24 images.
 * 
 * \param  newx, newy - size of destination image (must be smaller than original!)
 * \param  iDst - pointer to destination image (if it's 0, this image is modified)
 * \param  bChangeBpp - flag points to change result image bpp (if it's true, this result image bpp = 24 (useful for B/W image thumbnails))
 *
 * \return true if everything is ok
 * \author [bd], 9.2004; changes [Artiom Mirolubov], 1.2005
 */
bool CxImage::QIShrink(long newx, long newy, CxImage* const iDst, bool bChangeBpp)
{
	if (!pDib) return false;
	
	if (newx>head.biWidth || newy>head.biHeight) { 
		//let me repeat... this method can't enlarge image
		strcpy(info.szLastError,"QIShrink can't enlarge image");
		return false;
	}

	if (newx==head.biWidth && newy==head.biHeight) {
		//image already correct size (just copy and return)
		if (iDst) iDst->Copy(*this);
		return true;
	}//if
	
	//create temporary destination image
	CxImage newImage;
	newImage.CopyInfo(*this);
	newImage.Create(newx,newy,(bChangeBpp)?24:head.biBitCount,GetType());
	newImage.SetPalette(GetPalette());
	if (!newImage.IsValid()){
		strcpy(info.szLastError,newImage.GetLastError());
		return false;
	}

	//and alpha channel if required
#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()) newImage.AlphaCreate();
#endif

    const int oldx = head.biWidth;
    const int oldy = head.biHeight;

    int accuCellSize = 4;
#if CXIMAGE_SUPPORT_ALPHA
	BYTE *alphaPtr;
	if (AlphaIsValid()) accuCellSize=5;
#endif

    unsigned int *accu = new unsigned int[newx*accuCellSize];      //array for suming pixels... one pixel for every destination column
    unsigned int *accuPtr;                              //pointer for walking through accu
    //each cell consists of blue, red, green component and count of pixels summed in this cell
    memset(accu, 0, newx * accuCellSize * sizeof(unsigned int));  //clear accu

    if (!IsIndexed()) {
		//RGB24 version with pointers
		BYTE *destPtr, *srcPtr, *destPtrS, *srcPtrS;        //destination and source pixel, and beginnings of current row
		srcPtrS=(BYTE*)BlindGetPixelPointer(0,0);
		destPtrS=(BYTE*)newImage.BlindGetPixelPointer(0,0);
		int ex=0, ey=0;                                               //ex and ey replace division... 
		int dy=0;
		//(we just add pixels, until by adding newx or newy we get a number greater than old size... then
		// it's time to move to next pixel)
        
		for(int y=0; y<oldy; y++){                                    //for all source rows
			info.nProgress = (long)(100*y/oldy); if (info.nEscape) break;
			ey += newy;                                                   
			ex = 0;                                                       //restart with ex = 0
			accuPtr=accu;                                                 //restart from beginning of accu
			srcPtr=srcPtrS;                                               //and from new source line
#if CXIMAGE_SUPPORT_ALPHA
			alphaPtr = AlphaGetPointer(0, y);
#endif

			for(int x=0; x<oldx; x++){                                    //for all source columns
				ex += newx;
				*accuPtr     += *(srcPtr++);                                  //add current pixel to current accu slot
				*(accuPtr+1) += *(srcPtr++);
				*(accuPtr+2) += *(srcPtr++);
				(*(accuPtr+3)) ++;
#if CXIMAGE_SUPPORT_ALPHA
				if (alphaPtr) *(accuPtr+4) += *(alphaPtr++);
#endif
				if (ex>oldx) {                                                //when we reach oldx, it's time to move to new slot
					accuPtr += accuCellSize;
					ex -= oldx;                                                   //(substract oldx from ex and resume from there on)
				}//if (ex overflow)
			}//for x

			if (ey>=oldy) {                                                 //now when this happens
				ey -= oldy;                                                     //it's time to move to new destination row
				destPtr = destPtrS;                                             //reset pointers to proper initial values
				accuPtr = accu;
#if CXIMAGE_SUPPORT_ALPHA
				alphaPtr = newImage.AlphaGetPointer(0, dy++);
#endif
				for (int k=0; k<newx; k++) {                                    //copy accu to destination row (divided by number of pixels in each slot)
					*(destPtr++) = (BYTE)(*(accuPtr) / *(accuPtr+3));
					*(destPtr++) = (BYTE)(*(accuPtr+1) / *(accuPtr+3));
					*(destPtr++) = (BYTE)(*(accuPtr+2) / *(accuPtr+3));
#if CXIMAGE_SUPPORT_ALPHA
					if (alphaPtr) *(alphaPtr++) = (BYTE)(*(accuPtr+4) / *(accuPtr+3));
#endif
					accuPtr += accuCellSize;
				}//for k
				memset(accu, 0, newx * accuCellSize * sizeof(unsigned int));                   //clear accu
				destPtrS += newImage.info.dwEffWidth;
			}//if (ey overflow)

			srcPtrS += info.dwEffWidth;                                     //next round we start from new source row
		}//for y
    } else {
		//standard version with GetPixelColor...
		int ex=0, ey=0;                                               //ex and ey replace division... 
		int dy=0;
		//(we just add pixels, until by adding newx or newy we get a number greater than old size... then
		// it's time to move to next pixel)
		RGBQUAD rgb;
        
		for(int y=0; y<oldy; y++){                                    //for all source rows
			info.nProgress = (long)(100*y/oldy); if (info.nEscape) break;
			ey += newy;                                                   
			ex = 0;                                                       //restart with ex = 0
			accuPtr=accu;                                                 //restart from beginning of accu
			for(int x=0; x<oldx; x++){                                    //for all source columns
				ex += newx;
				rgb = GetPixelColor(x, y, true);
				*accuPtr     += rgb.rgbBlue;                                  //add current pixel to current accu slot
				*(accuPtr+1) += rgb.rgbRed;
				*(accuPtr+2) += rgb.rgbGreen;
				(*(accuPtr+3)) ++;
#if CXIMAGE_SUPPORT_ALPHA
				if (pAlpha) *(accuPtr+4) += rgb.rgbReserved;
#endif
				if (ex>oldx) {                                                //when we reach oldx, it's time to move to new slot
					accuPtr += accuCellSize;
					ex -= oldx;                                                   //(substract oldx from ex and resume from there on)
				}//if (ex overflow)
			}//for x

			if (ey>=oldy) {                                                 //now when this happens
				ey -= oldy;                                                     //it's time to move to new destination row
				accuPtr = accu;
				for (int dx=0; dx<newx; dx++) {                                 //copy accu to destination row (divided by number of pixels in each slot)
					rgb.rgbBlue = (BYTE)(*(accuPtr) / *(accuPtr+3));
					rgb.rgbRed  = (BYTE)(*(accuPtr+1) / *(accuPtr+3));
					rgb.rgbGreen= (BYTE)(*(accuPtr+2) / *(accuPtr+3));
#if CXIMAGE_SUPPORT_ALPHA
					if (pAlpha) rgb.rgbReserved = (BYTE)(*(accuPtr+4) / *(accuPtr+3));
#endif
					newImage.SetPixelColor(dx, dy, rgb, pAlpha!=0);
					accuPtr += accuCellSize;
				}//for dx
				memset(accu, 0, newx * accuCellSize * sizeof(unsigned int));                   //clear accu
				dy++;
			}//if (ey overflow)
		}//for y
    }//if

    delete [] accu;                                                 //delete helper array
	
	//copy new image to the destination
	if (iDst) 
		iDst->Transfer(newImage);
	else 
		Transfer(newImage);
    return true;

}

////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_TRANSFORMATION
