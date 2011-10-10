// xImaInt.cpp : interpolation functions
/* 02/2004 - Branko Brevensek 
 * CxImage version 6.0.0 02/Feb/2008 - Davide Pizzolato - www.xdp.it
 */

#include "ximage.h"
#include "ximath.h"

#if CXIMAGE_SUPPORT_INTERPOLATION

////////////////////////////////////////////////////////////////////////////////
/**
 * Recalculates coordinates according to specified overflow method.
 * If pixel (x,y) lies within image, nothing changes.
 *
 *  \param x, y - coordinates of pixel
 *  \param ofMethod - overflow method
 * 
 *  \return x, y - new coordinates (pixel (x,y) now lies inside image)
 *
 *  \author ***bd*** 2.2004
 */
void CxImage::OverflowCoordinates(long &x, long &y, OverflowMethod const ofMethod)
{
  if (IsInside(x,y)) return;  //if pixel is within bounds, no change
  switch (ofMethod) {
    case OM_REPEAT:
      //clip coordinates
      x=max(x,0); x=min(x, head.biWidth-1);
      y=max(y,0); y=min(y, head.biHeight-1);
      break;
    case OM_WRAP:
      //wrap coordinates
      x = x % head.biWidth;
      y = y % head.biHeight;
      if (x<0) x = head.biWidth + x;
      if (y<0) y = head.biHeight + y;
      break;
    case OM_MIRROR:
      //mirror pixels near border
      if (x<0) x=((-x) % head.biWidth);
      else if (x>=head.biWidth) x=head.biWidth-(x % head.biWidth + 1);
      if (y<0) y=((-y) % head.biHeight);
      else if (y>=head.biHeight) y=head.biHeight-(y % head.biHeight + 1);
      break;
    default:
      return;
  }//switch
}

////////////////////////////////////////////////////////////////////////////////
/**
 * See OverflowCoordinates for integer version 
 * \author ***bd*** 2.2004
 */
void CxImage::OverflowCoordinates(float &x, float &y, OverflowMethod const ofMethod)
{
  if (x>=0 && x<head.biWidth && y>=0 && y<head.biHeight) return;  //if pixel is within bounds, no change
  switch (ofMethod) {
    case OM_REPEAT:
      //clip coordinates
      x=max(x,0); x=min(x, head.biWidth-1);
      y=max(y,0); y=min(y, head.biHeight-1);
      break;
    case OM_WRAP:
      //wrap coordinates
      x = (float)fmod(x, (float) head.biWidth);
      y = (float)fmod(y, (float) head.biHeight);
      if (x<0) x = head.biWidth + x;
      if (y<0) y = head.biHeight + y;
      break;
    case OM_MIRROR:
      //mirror pixels near border
      if (x<0) x=(float)fmod(-x, (float) head.biWidth);
      else if (x>=head.biWidth) x=head.biWidth-((float)fmod(x, (float) head.biWidth) + 1);
      if (y<0) y=(float)fmod(-y, (float) head.biHeight);
      else if (y>=head.biHeight) y=head.biHeight-((float)fmod(y, (float) head.biHeight) + 1);
      break;
    default:
      return;
  }//switch
}

////////////////////////////////////////////////////////////////////////////////
/**
 * Method return pixel color. Different methods are implemented for out of bounds pixels.
 * If an image has alpha channel, alpha value is returned in .RGBReserved.
 *
 *  \param x,y : pixel coordinates
 *  \param ofMethod : out-of-bounds method:
 *    - OF_WRAP - wrap over to pixels on other side of the image
 *    - OF_REPEAT - repeat last pixel on the edge
 *    - OF_COLOR - return input value of color
 *    - OF_BACKGROUND - return background color (if not set, return input color)
 *    - OF_TRANSPARENT - return transparent pixel
 *
 *  \param rplColor : input color (returned for out-of-bound coordinates in OF_COLOR mode and if other mode is not applicable)
 *
 * \return color : color of pixel
 * \author ***bd*** 2.2004
 */
RGBQUAD CxImage::GetPixelColorWithOverflow(long x, long y, OverflowMethod const ofMethod, RGBQUAD* const rplColor)
{
  RGBQUAD color;          //color to return
  if ((!IsInside(x,y)) || pDib==NULL) {     //is pixel within bouns?:
    //pixel is out of bounds or no DIB
    if (rplColor!=NULL)
      color=*rplColor;
    else {
      color.rgbRed=color.rgbGreen=color.rgbBlue=255; color.rgbReserved=0; //default replacement colour: white transparent
    }//if
    if (pDib==NULL) return color;
    //pixel is out of bounds:
    switch (ofMethod) {
      case OM_TRANSPARENT:
#if CXIMAGE_SUPPORT_ALPHA
        if (AlphaIsValid()) {
          //alpha transparency is supported and image has alpha layer
          color.rgbReserved=0;
        } else {
#endif //CXIMAGE_SUPPORT_ALPHA
          //no alpha transparency
          if (GetTransIndex()>=0) {
            color=GetTransColor();    //single color transparency enabled (return transparent color)
          }//if
#if CXIMAGE_SUPPORT_ALPHA
        }//if
#endif //CXIMAGE_SUPPORT_ALPHA
        return color;
      case OM_BACKGROUND:
		  //return background color (if it exists, otherwise input value)
		  if (info.nBkgndIndex >= 0) {
			  if (head.biBitCount<24) color = GetPaletteColor((BYTE)info.nBkgndIndex);
			  else color = info.nBkgndColor;
		  }//if
		  return color;
      case OM_REPEAT:
      case OM_WRAP:
      case OM_MIRROR:
        OverflowCoordinates(x,y,ofMethod);
        break;
      default:
        //simply return replacement color (OM_COLOR and others)
        return color;
    }//switch
  }//if
  //just return specified pixel (it's within bounds)
  return BlindGetPixelColor(x,y);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * This method reconstructs image according to chosen interpolation method and then returns pixel (x,y).
 * (x,y) can lie between actual image pixels. If (x,y) lies outside of image, method returns value
 * according to overflow method.
 * This method is very useful for geometrical image transformations, where destination pixel
 * can often assume color value lying between source pixels.
 *
 *  \param (x,y) - coordinates of pixel to return
 *           GPCI method recreates "analogue" image back from digital data, so x and y
 *           are float values and color value of point (1.1,1) will generally not be same
 *           as (1,1). Center of first pixel is at (0,0) and center of pixel right to it is (1,0).
 *           (0.5,0) is half way between these two pixels.
 *  \param inMethod - interpolation (reconstruction) method (kernel) to use:
 *    - IM_NEAREST_NEIGHBOUR - returns colour of nearest lying pixel (causes stairy look of 
 *                            processed images)
 *    - IM_BILINEAR - interpolates colour from four neighbouring pixels (softens image a bit)
 *    - IM_BICUBIC - interpolates from 16 neighbouring pixels (can produce "halo" artifacts)
 *    - IM_BICUBIC2 - interpolates from 16 neighbouring pixels (perhaps a bit less halo artifacts 
                     than IM_BICUBIC)
 *    - IM_BSPLINE - interpolates from 16 neighbouring pixels (softens image, washes colours)
 *                  (As far as I know, image should be prefiltered for this method to give 
 *                   good results... some other time :) )
 *                  This method uses bicubic interpolation kernel from CXImage 5.99a and older
 *                  versions.
 *    - IM_LANCZOS - interpolates from 12*12 pixels (slow, ringing artifacts)
 *
 *  \param ofMethod - overflow method (see comments at GetPixelColorWithOverflow)
 *  \param rplColor - pointer to color used for out of borders pixels in OM_COLOR mode
 *              (and other modes if colour can't calculated in a specified way)
 *
 *  \return interpolated color value (including interpolated alpha value, if image has alpha layer)
 * 
 *  \author ***bd*** 2.2004
 */
RGBQUAD CxImage::GetPixelColorInterpolated(
  float x,float y, 
  InterpolationMethod const inMethod, 
  OverflowMethod const ofMethod, 
  RGBQUAD* const rplColor)
{
  //calculate nearest pixel
  int xi=(int)(x); if (x<0) xi--;   //these replace (incredibly slow) floor (Visual c++ 2003, AMD Athlon)
  int yi=(int)(y); if (y<0) yi--;
  RGBQUAD color;                    //calculated colour

  switch (inMethod) {
    case IM_NEAREST_NEIGHBOUR:
      return GetPixelColorWithOverflow((long)(x+0.5f), (long)(y+0.5f), ofMethod, rplColor);
    default: {
      //IM_BILINEAR: bilinear interpolation
      if (xi<-1 || xi>=head.biWidth || yi<-1 || yi>=head.biHeight) {  //all 4 points are outside bounds?:
        switch (ofMethod) {
          case OM_COLOR: case OM_TRANSPARENT: case OM_BACKGROUND:
            //we don't need to interpolate anything with all points outside in this case
            return GetPixelColorWithOverflow(-999, -999, ofMethod, rplColor);
          default:
            //recalculate coordinates and use faster method later on
            OverflowCoordinates(x,y,ofMethod);
            xi=(int)(x); if (x<0) xi--;   //x and/or y have changed ... recalculate xi and yi
            yi=(int)(y); if (y<0) yi--;
        }//switch
      }//if
      //get four neighbouring pixels
      if ((xi+1)<head.biWidth && xi>=0 && (yi+1)<head.biHeight && yi>=0 && head.biClrUsed==0) {
        //all pixels are inside RGB24 image... optimize reading (and use fixed point arithmetic)
        WORD wt1=(WORD)((x-xi)*256.0f), wt2=(WORD)((y-yi)*256.0f);
        WORD wd=wt1*wt2>>8;
        WORD wb=wt1-wd;
        WORD wc=wt2-wd;
        WORD wa=256-wt1-wc;
        WORD wrr,wgg,wbb;
        BYTE *pxptr=(BYTE*)info.pImage+yi*info.dwEffWidth+xi*3;
        wbb=wa*(*pxptr++); wgg=wa*(*pxptr++); wrr=wa*(*pxptr++);
        wbb+=wb*(*pxptr++); wgg+=wb*(*pxptr++); wrr+=wb*(*pxptr);
        pxptr+=(info.dwEffWidth-5); //move to next row
        wbb+=wc*(*pxptr++); wgg+=wc*(*pxptr++); wrr+=wc*(*pxptr++); 
        wbb+=wd*(*pxptr++); wgg+=wd*(*pxptr++); wrr+=wd*(*pxptr); 
        color.rgbRed=(BYTE) (wrr>>8); color.rgbGreen=(BYTE) (wgg>>8); color.rgbBlue=(BYTE) (wbb>>8);
#if CXIMAGE_SUPPORT_ALPHA
        if (pAlpha) {
          WORD waa;
          //image has alpha layer... we have to do the same for alpha data
          pxptr=AlphaGetPointer(xi,yi);                           //pointer to first byte
          waa=wa*(*pxptr++); waa+=wb*(*pxptr);   //first two pixels
          pxptr+=(head.biWidth-1);                                //move to next row
          waa+=wc*(*pxptr++); waa+=wd*(*pxptr);   //and second row pixels
          color.rgbReserved=(BYTE) (waa>>8);
        } else
#endif
		{ //Alpha not supported or no alpha at all
			color.rgbReserved = 0;
		}
        return color;
      } else {
        //default (slower) way to get pixels (not RGB24 or some pixels out of borders)
        float t1=x-xi, t2=y-yi;
        float d=t1*t2;
        float b=t1-d;
        float c=t2-d;
        float a=1-t1-c;
        RGBQUAD rgb11,rgb21,rgb12,rgb22;
        rgb11=GetPixelColorWithOverflow(xi, yi, ofMethod, rplColor);
        rgb21=GetPixelColorWithOverflow(xi+1, yi, ofMethod, rplColor);
        rgb12=GetPixelColorWithOverflow(xi, yi+1, ofMethod, rplColor);
        rgb22=GetPixelColorWithOverflow(xi+1, yi+1, ofMethod, rplColor);
        //calculate linear interpolation
        color.rgbRed=(BYTE) (a*rgb11.rgbRed+b*rgb21.rgbRed+c*rgb12.rgbRed+d*rgb22.rgbRed);
        color.rgbGreen=(BYTE) (a*rgb11.rgbGreen+b*rgb21.rgbGreen+c*rgb12.rgbGreen+d*rgb22.rgbGreen);
        color.rgbBlue=(BYTE) (a*rgb11.rgbBlue+b*rgb21.rgbBlue+c*rgb12.rgbBlue+d*rgb22.rgbBlue);
#if CXIMAGE_SUPPORT_ALPHA
        if (AlphaIsValid())
			color.rgbReserved=(BYTE) (a*rgb11.rgbReserved+b*rgb21.rgbReserved+c*rgb12.rgbReserved+d*rgb22.rgbReserved);
		else
#endif
		{ //Alpha not supported or no alpha at all
			color.rgbReserved = 0;
		}
        return color;
      }//if
    }//default
    case IM_BICUBIC: 
    case IM_BICUBIC2:
    case IM_BSPLINE:
	case IM_BOX:
	case IM_HERMITE:
	case IM_HAMMING:
	case IM_SINC:
	case IM_BLACKMAN:
	case IM_BESSEL:
	case IM_GAUSSIAN:
	case IM_QUADRATIC:
	case IM_MITCHELL:
	case IM_CATROM:
	case IM_HANNING:
	case IM_POWER:
      //bicubic interpolation(s)
      if (((xi+2)<0) || ((xi-1)>=head.biWidth) || ((yi+2)<0) || ((yi-1)>=head.biHeight)) { //all points are outside bounds?:
        switch (ofMethod) {
          case OM_COLOR: case OM_TRANSPARENT: case OM_BACKGROUND:
            //we don't need to interpolate anything with all points outside in this case
            return GetPixelColorWithOverflow(-999, -999, ofMethod, rplColor);
            break;
          default:
            //recalculate coordinates and use faster method later on
            OverflowCoordinates(x,y,ofMethod);
            xi=(int)(x); if (x<0) xi--;   //x and/or y have changed ... recalculate xi and yi
            yi=(int)(y); if (y<0) yi--;
        }//switch
      }//if

      //some variables needed from here on
      int xii,yii;                      //x any y integer indexes for loops
      float kernel, kernelyc;           //kernel cache
      float kernelx[12], kernely[4];    //precalculated kernel values
      float rr,gg,bb,aa;                //accumulated color values
      //calculate multiplication factors for all pixels
	  int i;
      switch (inMethod) {
        case IM_BICUBIC:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelCubic((float)(xi+i-1-x));
            kernely[i]=KernelCubic((float)(yi+i-1-y));
          }//for i
          break;
        case IM_BICUBIC2:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelGeneralizedCubic((float)(xi+i-1-x), -0.5);
            kernely[i]=KernelGeneralizedCubic((float)(yi+i-1-y), -0.5);
          }//for i
          break;
        case IM_BSPLINE:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelBSpline((float)(xi+i-1-x));
            kernely[i]=KernelBSpline((float)(yi+i-1-y));
          }//for i
          break;
        case IM_BOX:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelBox((float)(xi+i-1-x));
            kernely[i]=KernelBox((float)(yi+i-1-y));
          }//for i
          break;
        case IM_HERMITE:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelHermite((float)(xi+i-1-x));
            kernely[i]=KernelHermite((float)(yi+i-1-y));
          }//for i
          break;
        case IM_HAMMING:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelHamming((float)(xi+i-1-x));
            kernely[i]=KernelHamming((float)(yi+i-1-y));
          }//for i
          break;
        case IM_SINC:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelSinc((float)(xi+i-1-x));
            kernely[i]=KernelSinc((float)(yi+i-1-y));
          }//for i
          break;
        case IM_BLACKMAN:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelBlackman((float)(xi+i-1-x));
            kernely[i]=KernelBlackman((float)(yi+i-1-y));
          }//for i
          break;
        case IM_BESSEL:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelBessel((float)(xi+i-1-x));
            kernely[i]=KernelBessel((float)(yi+i-1-y));
          }//for i
          break;
        case IM_GAUSSIAN:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelGaussian((float)(xi+i-1-x));
            kernely[i]=KernelGaussian((float)(yi+i-1-y));
          }//for i
          break;
        case IM_QUADRATIC:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelQuadratic((float)(xi+i-1-x));
            kernely[i]=KernelQuadratic((float)(yi+i-1-y));
          }//for i
          break;
        case IM_MITCHELL:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelMitchell((float)(xi+i-1-x));
            kernely[i]=KernelMitchell((float)(yi+i-1-y));
          }//for i
          break;
        case IM_CATROM:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelCatrom((float)(xi+i-1-x));
            kernely[i]=KernelCatrom((float)(yi+i-1-y));
          }//for i
          break;
        case IM_HANNING:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelHanning((float)(xi+i-1-x));
            kernely[i]=KernelHanning((float)(yi+i-1-y));
          }//for i
          break;
        case IM_POWER:
          for (i=0; i<4; i++) {
            kernelx[i]=KernelPower((float)(xi+i-1-x));
            kernely[i]=KernelPower((float)(yi+i-1-y));
          }//for i
          break;
      }//switch
      rr=gg=bb=aa=0;
      if (((xi+2)<head.biWidth) && xi>=1 && ((yi+2)<head.biHeight) && (yi>=1) && !IsIndexed()) {
        //optimized interpolation (faster pixel reads) for RGB24 images with all pixels inside bounds
        BYTE *pxptr, *pxptra;
        for (yii=yi-1; yii<yi+3; yii++) {
          pxptr=(BYTE *)BlindGetPixelPointer(xi-1, yii);    //calculate pointer to first byte in row
          kernelyc=kernely[yii-(yi-1)];
#if CXIMAGE_SUPPORT_ALPHA
          if (AlphaIsValid()) {
            //alpha is supported and valid (optimized bicubic int. for image with alpha)
            pxptra=AlphaGetPointer(xi-1, yii);
            kernel=kernelyc*kernelx[0];
            bb+=kernel*(*pxptr++); gg+=kernel*(*pxptr++); rr+=kernel*(*pxptr++); aa+=kernel*(*pxptra++);
            kernel=kernelyc*kernelx[1];
            bb+=kernel*(*pxptr++); gg+=kernel*(*pxptr++); rr+=kernel*(*pxptr++); aa+=kernel*(*pxptra++);
            kernel=kernelyc*kernelx[2];
            bb+=kernel*(*pxptr++); gg+=kernel*(*pxptr++); rr+=kernel*(*pxptr++); aa+=kernel*(*pxptra++);
            kernel=kernelyc*kernelx[3];
            bb+=kernel*(*pxptr++); gg+=kernel*(*pxptr++); rr+=kernel*(*pxptr); aa+=kernel*(*pxptra);
          } else
#endif
          //alpha not supported or valid (optimized bicubic int. for no alpha channel)
          {
            kernel=kernelyc*kernelx[0];
            bb+=kernel*(*pxptr++); gg+=kernel*(*pxptr++); rr+=kernel*(*pxptr++);
            kernel=kernelyc*kernelx[1];
            bb+=kernel*(*pxptr++); gg+=kernel*(*pxptr++); rr+=kernel*(*pxptr++);
            kernel=kernelyc*kernelx[2];
            bb+=kernel*(*pxptr++); gg+=kernel*(*pxptr++); rr+=kernel*(*pxptr++);
            kernel=kernelyc*kernelx[3];
            bb+=kernel*(*pxptr++); gg+=kernel*(*pxptr++); rr+=kernel*(*pxptr);
          }
        }//yii
      } else {
        //slower more flexible interpolation for border pixels and paletted images
        RGBQUAD rgbs;
        for (yii=yi-1; yii<yi+3; yii++) {
          kernelyc=kernely[yii-(yi-1)];
          for (xii=xi-1; xii<xi+3; xii++) {
            kernel=kernelyc*kernelx[xii-(xi-1)];
            rgbs=GetPixelColorWithOverflow(xii, yii, ofMethod, rplColor);
            rr+=kernel*rgbs.rgbRed;
            gg+=kernel*rgbs.rgbGreen;
            bb+=kernel*rgbs.rgbBlue;
#if CXIMAGE_SUPPORT_ALPHA
            aa+=kernel*rgbs.rgbReserved;
#endif
          }//xii
        }//yii
      }//if
      //for all colors, clip to 0..255 and assign to RGBQUAD
      if (rr>255) rr=255; if (rr<0) rr=0; color.rgbRed=(BYTE) rr;
      if (gg>255) gg=255; if (gg<0) gg=0; color.rgbGreen=(BYTE) gg;
      if (bb>255) bb=255; if (bb<0) bb=0; color.rgbBlue=(BYTE) bb;
#if CXIMAGE_SUPPORT_ALPHA
      if (AlphaIsValid()) {
        if (aa>255) aa=255; if (aa<0) aa=0; color.rgbReserved=(BYTE) aa;
      } else
#endif
		{ //Alpha not supported or no alpha at all
			color.rgbReserved = 0;
		}
      return color;
    case IM_LANCZOS:
      //lanczos window (16*16) sinc interpolation
      if (((xi+6)<0) || ((xi-5)>=head.biWidth) || ((yi+6)<0) || ((yi-5)>=head.biHeight)) {
        //all points are outside bounds
        switch (ofMethod) {
          case OM_COLOR: case OM_TRANSPARENT: case OM_BACKGROUND:
            //we don't need to interpolate anything with all points outside in this case
            return GetPixelColorWithOverflow(-999, -999, ofMethod, rplColor);
            break;
          default:
            //recalculate coordinates and use faster method later on
            OverflowCoordinates(x,y,ofMethod);
            xi=(int)(x); if (x<0) xi--;   //x and/or y have changed ... recalculate xi and yi
            yi=(int)(y); if (y<0) yi--;
        }//switch
      }//if

      for (xii=xi-5; xii<xi+7; xii++) kernelx[xii-(xi-5)]=KernelLanczosSinc((float)(xii-x), 6.0f);
      rr=gg=bb=aa=0;

      if (((xi+6)<head.biWidth) && ((xi-5)>=0) && ((yi+6)<head.biHeight) && ((yi-5)>=0) && !IsIndexed()) {
        //optimized interpolation (faster pixel reads) for RGB24 images with all pixels inside bounds
        BYTE *pxptr, *pxptra;
        for (yii=yi-5; yii<yi+7; yii++) {
          pxptr=(BYTE *)BlindGetPixelPointer(xi-5, yii);    //calculate pointer to first byte in row
          kernelyc=KernelLanczosSinc((float)(yii-y),6.0f);
#if CXIMAGE_SUPPORT_ALPHA
          if (AlphaIsValid()) {
            //alpha is supported and valid
            pxptra=AlphaGetPointer(xi-1, yii);
            for (xii=0; xii<12; xii++) {
              kernel=kernelyc*kernelx[xii];
              bb+=kernel*(*pxptr++); gg+=kernel*(*pxptr++); rr+=kernel*(*pxptr++); aa+=kernel*(*pxptra++);
            }//for xii
          } else
#endif
          //alpha not supported or valid
          {
            for (xii=0; xii<12; xii++) {
              kernel=kernelyc*kernelx[xii];
              bb+=kernel*(*pxptr++); gg+=kernel*(*pxptr++); rr+=kernel*(*pxptr++);
            }//for xii
          }
        }//yii
      } else {
        //slower more flexible interpolation for border pixels and paletted images
        RGBQUAD rgbs;
        for (yii=yi-5; yii<yi+7; yii++) {
          kernelyc=KernelLanczosSinc((float)(yii-y),6.0f);
          for (xii=xi-5; xii<xi+7; xii++) {
            kernel=kernelyc*kernelx[xii-(xi-5)];
            rgbs=GetPixelColorWithOverflow(xii, yii, ofMethod, rplColor);
            rr+=kernel*rgbs.rgbRed;
            gg+=kernel*rgbs.rgbGreen;
            bb+=kernel*rgbs.rgbBlue;
#if CXIMAGE_SUPPORT_ALPHA
            aa+=kernel*rgbs.rgbReserved;
#endif
          }//xii
        }//yii
      }//if
      //for all colors, clip to 0..255 and assign to RGBQUAD
      if (rr>255) rr=255; if (rr<0) rr=0; color.rgbRed=(BYTE) rr;
      if (gg>255) gg=255; if (gg<0) gg=0; color.rgbGreen=(BYTE) gg;
      if (bb>255) bb=255; if (bb<0) bb=0; color.rgbBlue=(BYTE) bb;
#if CXIMAGE_SUPPORT_ALPHA
      if (AlphaIsValid()) {
        if (aa>255) aa=255; if (aa<0) aa=0; color.rgbReserved=(BYTE) aa;   
      } else
#endif
		{ //Alpha not supported or no alpha at all
			color.rgbReserved = 0;
		}
      return color;
  }//switch
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Helper function for GetAreaColorInterpolated.
 * Adds 'surf' portion of image pixel with color 'color' to (rr,gg,bb,aa).
 */
void CxImage::AddAveragingCont(RGBQUAD const &color, float const surf, float &rr, float &gg, float &bb, float &aa)
{
  rr+=color.rgbRed*surf;
  gg+=color.rgbGreen*surf;
  bb+=color.rgbBlue*surf;
#if CXIMAGE_SUPPORT_ALPHA
  aa+=color.rgbReserved*surf;
#endif
}
////////////////////////////////////////////////////////////////////////////////
/**
 * This method is similar to GetPixelColorInterpolated, but this method also properly handles 
 * subsampling.
 * If you need to sample original image with interval of more than 1 pixel (as when shrinking an image), 
 * you should use this method instead of GetPixelColorInterpolated or aliasing will occur.
 * When area width and height are both less than pixel, this method gets pixel color by interpolating
 * color of frame center with selected (inMethod) interpolation by calling GetPixelColorInterpolated. 
 * If width and height are more than 1, method calculates color by averaging color of pixels within area.
 * Interpolation method is not used in this case. Pixel color is interpolated by averaging instead.
 * If only one of both is more than 1, method uses combination of interpolation and averaging.
 * Chosen interpolation method is used, but since it is averaged later on, there is little difference
 * between IM_BILINEAR (perhaps best for this case) and better methods. IM_NEAREST_NEIGHBOUR again
 * leads to aliasing artifacts.
 * This method is a bit slower than GetPixelColorInterpolated and when aliasing is not a problem, you should
 * simply use the later. 
 *
 * \param  xc, yc - center of (rectangular) area
 * \param  w, h - width and height of area
 * \param  inMethod - interpolation method that is used, when interpolation is used (see above)
 * \param  ofMethod - overflow method used when retrieving individual pixel colors
 * \param  rplColor - replacement colour to use, in OM_COLOR
 *
 * \author ***bd*** 2.2004
 */
RGBQUAD CxImage::GetAreaColorInterpolated(
  float const xc, float const yc, float const w, float const h, 
  InterpolationMethod const inMethod, 
  OverflowMethod const ofMethod, 
  RGBQUAD* const rplColor)
{
	RGBQUAD color;      //calculated colour
	
	if (h<=1 && w<=1) {
		//both width and height are less than one... we will use interpolation of center point
		return GetPixelColorInterpolated(xc, yc, inMethod, ofMethod, rplColor);
	} else {
		//area is wider and/or taller than one pixel:
		CxRect2 area(xc-w/2.0f, yc-h/2.0f, xc+w/2.0f, yc+h/2.0f);   //area
		int xi1=(int)(area.botLeft.x+0.49999999f);                //low x
		int yi1=(int)(area.botLeft.y+0.49999999f);                //low y
		
		
		int xi2=(int)(area.topRight.x+0.5f);                      //top x
		int yi2=(int)(area.topRight.y+0.5f);                      //top y (for loops)
		
		float rr,gg,bb,aa;                                        //red, green, blue and alpha components
		rr=gg=bb=aa=0;
		int x,y;                                                  //loop counters
		float s=0;                                                //surface of all pixels
		float cps;                                                //surface of current crosssection
		if (h>1 && w>1) {
			//width and height of area are greater than one pixel, so we can employ "ordinary" averaging
			CxRect2 intBL, intTR;     //bottom left and top right intersection
			intBL=area.CrossSection(CxRect2(((float)xi1)-0.5f, ((float)yi1)-0.5f, ((float)xi1)+0.5f, ((float)yi1)+0.5f));
			intTR=area.CrossSection(CxRect2(((float)xi2)-0.5f, ((float)yi2)-0.5f, ((float)xi2)+0.5f, ((float)yi2)+0.5f));
			float wBL, wTR, hBL, hTR;
			wBL=intBL.Width();            //width of bottom left pixel-area intersection
			hBL=intBL.Height();           //height of bottom left...
			wTR=intTR.Width();            //width of top right...
			hTR=intTR.Height();           //height of top right...
			
			AddAveragingCont(GetPixelColorWithOverflow(xi1,yi1,ofMethod,rplColor), wBL*hBL, rr, gg, bb, aa);    //bottom left pixel
			AddAveragingCont(GetPixelColorWithOverflow(xi2,yi1,ofMethod,rplColor), wTR*hBL, rr, gg, bb, aa);    //bottom right pixel
			AddAveragingCont(GetPixelColorWithOverflow(xi1,yi2,ofMethod,rplColor), wBL*hTR, rr, gg, bb, aa);    //top left pixel
			AddAveragingCont(GetPixelColorWithOverflow(xi2,yi2,ofMethod,rplColor), wTR*hTR, rr, gg, bb, aa);    //top right pixel
			//bottom and top row
			for (x=xi1+1; x<xi2; x++) {
				AddAveragingCont(GetPixelColorWithOverflow(x,yi1,ofMethod,rplColor), hBL, rr, gg, bb, aa);    //bottom row
				AddAveragingCont(GetPixelColorWithOverflow(x,yi2,ofMethod,rplColor), hTR, rr, gg, bb, aa);    //top row
			}
			//leftmost and rightmost column
			for (y=yi1+1; y<yi2; y++) {
				AddAveragingCont(GetPixelColorWithOverflow(xi1,y,ofMethod,rplColor), wBL, rr, gg, bb, aa);    //left column
				AddAveragingCont(GetPixelColorWithOverflow(xi2,y,ofMethod,rplColor), wTR, rr, gg, bb, aa);    //right column
			}
			for (y=yi1+1; y<yi2; y++) {
				for (x=xi1+1; x<xi2; x++) { 
					color=GetPixelColorWithOverflow(x,y,ofMethod,rplColor);
					rr+=color.rgbRed;
					gg+=color.rgbGreen;
					bb+=color.rgbBlue;
#if CXIMAGE_SUPPORT_ALPHA
					aa+=color.rgbReserved;
#endif
				}//for x
			}//for y
		} else {
			//width or height greater than one:
			CxRect2 intersect;                                          //intersection with current pixel
			CxPoint2 center;
			for (y=yi1; y<=yi2; y++) {
				for (x=xi1; x<=xi2; x++) {
					intersect=area.CrossSection(CxRect2(((float)x)-0.5f, ((float)y)-0.5f, ((float)x)+0.5f, ((float)y)+0.5f));
					center=intersect.Center();
					color=GetPixelColorInterpolated(center.x, center.y, inMethod, ofMethod, rplColor);
					cps=intersect.Surface();
					rr+=color.rgbRed*cps;
					gg+=color.rgbGreen*cps;
					bb+=color.rgbBlue*cps;
#if CXIMAGE_SUPPORT_ALPHA
					aa+=color.rgbReserved*cps;
#endif
				}//for x
			}//for y      
		}//if
		
		s=area.Surface();
		rr/=s; gg/=s; bb/=s; aa/=s;
		if (rr>255) rr=255; if (rr<0) rr=0; color.rgbRed=(BYTE) rr;
		if (gg>255) gg=255; if (gg<0) gg=0; color.rgbGreen=(BYTE) gg;
		if (bb>255) bb=255; if (bb<0) bb=0; color.rgbBlue=(BYTE) bb;
#if CXIMAGE_SUPPORT_ALPHA
		if (AlphaIsValid()) {
			if (aa>255) aa=255; if (aa<0) aa=0; color.rgbReserved=(BYTE) aa;
		}//if
#endif
	}//if
	return color;
}

////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelBSpline(const float x)
{
	if (x>2.0f) return 0.0f;
	// thanks to Kristian Kratzenstein
	float a, b, c, d;
	float xm1 = x - 1.0f; // Was calculatet anyway cause the "if((x-1.0f) < 0)"
	float xp1 = x + 1.0f;
	float xp2 = x + 2.0f;

	if ((xp2) <= 0.0f) a = 0.0f; else a = xp2*xp2*xp2; // Only float, not float -> double -> float
	if ((xp1) <= 0.0f) b = 0.0f; else b = xp1*xp1*xp1;
	if (x <= 0) c = 0.0f; else c = x*x*x;  
	if ((xm1) <= 0.0f) d = 0.0f; else d = xm1*xm1*xm1;

	return (0.16666666666666666667f * (a - (4.0f * b) + (6.0f * c) - (4.0f * d)));

	/* equivalent <Vladimír Kloucek>
	if (x < -2.0)
		return(0.0f);
	if (x < -1.0)
		return((2.0f+x)*(2.0f+x)*(2.0f+x)*0.16666666666666666667f);
	if (x < 0.0)
		return((4.0f+x*x*(-6.0f-3.0f*x))*0.16666666666666666667f);
	if (x < 1.0)
		return((4.0f+x*x*(-6.0f+3.0f*x))*0.16666666666666666667f);
	if (x < 2.0)
		return((2.0f-x)*(2.0f-x)*(2.0f-x)*0.16666666666666666667f);
	return(0.0f);
	*/
}

////////////////////////////////////////////////////////////////////////////////
/**
 * Bilinear interpolation kernel:
  \verbatim
          /
         | 1-t           , if  0 <= t <= 1
  h(t) = | t+1           , if -1 <= t <  0
         | 0             , otherwise
          \
  \endverbatim
 * ***bd*** 2.2004
 */
float CxImage::KernelLinear(const float t)
{
//  if (0<=t && t<=1) return 1-t;
//  if (-1<=t && t<0) return 1+t;
//  return 0;
	
	//<Vladimír Kloucek>
	if (t < -1.0f)
		return 0.0f;
	if (t < 0.0f)
		return 1.0f+t;
	if (t < 1.0f)
		return 1.0f-t;
	return 0.0f;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * Bicubic interpolation kernel (a=-1):
  \verbatim
          /
         | 1-2|t|**2+|t|**3          , if |t| < 1
  h(t) = | 4-8|t|+5|t|**2-|t|**3     , if 1<=|t|<2
         | 0                         , otherwise
          \
  \endverbatim
 * ***bd*** 2.2004
 */
float CxImage::KernelCubic(const float t)
{
  float abs_t = (float)fabs(t);
  float abs_t_sq = abs_t * abs_t;
  if (abs_t<1) return 1-2*abs_t_sq+abs_t_sq*abs_t;
  if (abs_t<2) return 4 - 8*abs_t +5*abs_t_sq - abs_t_sq*abs_t;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * Bicubic kernel (for a=-1 it is the same as BicubicKernel):
  \verbatim
          /
         | (a+2)|t|**3 - (a+3)|t|**2 + 1     , |t| <= 1
  h(t) = | a|t|**3 - 5a|t|**2 + 8a|t| - 4a   , 1 < |t| <= 2
         | 0                                 , otherwise
          \
  \endverbatim
 * Often used values for a are -1 and -1/2.
 */
float CxImage::KernelGeneralizedCubic(const float t, const float a)
{
  float abs_t = (float)fabs(t);
  float abs_t_sq = abs_t * abs_t;
  if (abs_t<1) return (a+2)*abs_t_sq*abs_t - (a+3)*abs_t_sq + 1;
  if (abs_t<2) return a*abs_t_sq*abs_t - 5*a*abs_t_sq + 8*a*abs_t - 4*a;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * Lanczos windowed sinc interpolation kernel with radius r.
  \verbatim
          /
  h(t) = | sinc(t)*sinc(t/r)       , if |t|<r
         | 0                       , otherwise
          \
  \endverbatim
 * ***bd*** 2.2004
 */
float CxImage::KernelLanczosSinc(const float t, const float r)
{
  if (fabs(t) > r) return 0;
  if (t==0) return 1;
  float pit=PI*t;
  float pitd=pit/r;
  return (float)((sin(pit)/pit) * (sin(pitd)/pitd));
}

////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelBox(const float x)
{
	if (x < -0.5f)
		return 0.0f;
	if (x < 0.5f)
		return 1.0f;
	return 0.0f;
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelHermite(const float x)
{
	if (x < -1.0f)
		return 0.0f;
	if (x < 0.0f)
		return (-2.0f*x-3.0f)*x*x+1.0f;
	if (x < 1.0f)
		return (2.0f*x-3.0f)*x*x+1.0f;
	return 0.0f;
//	if (fabs(x)>1) return 0.0f;
//	return(0.5f+0.5f*(float)cos(PI*x));
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelHanning(const float x)
{
	if (fabs(x)>1) return 0.0f;
	return (0.5f+0.5f*(float)cos(PI*x))*((float)sin(PI*x)/(PI*x));
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelHamming(const float x)
{
	if (x < -1.0f)
		return 0.0f;
	if (x < 0.0f)
		return 0.92f*(-2.0f*x-3.0f)*x*x+1.0f;
	if (x < 1.0f)
		return 0.92f*(2.0f*x-3.0f)*x*x+1.0f;
	return 0.0f;
//	if (fabs(x)>1) return 0.0f;
//	return(0.54f+0.46f*(float)cos(PI*x));
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelSinc(const float x)
{
	if (x == 0.0)
		return(1.0);
	return((float)sin(PI*x)/(PI*x));
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelBlackman(const float x)
{
	//if (fabs(x)>1) return 0.0f;
	return (0.42f+0.5f*(float)cos(PI*x)+0.08f*(float)cos(2.0f*PI*x));
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelBessel_J1(const float x)
{
	double p, q;
	
	register long i;
	
	static const double
	Pone[] =
	{
		0.581199354001606143928050809e+21,
		-0.6672106568924916298020941484e+20,
		0.2316433580634002297931815435e+19,
		-0.3588817569910106050743641413e+17,
		0.2908795263834775409737601689e+15,
		-0.1322983480332126453125473247e+13,
		0.3413234182301700539091292655e+10,
		-0.4695753530642995859767162166e+7,
		0.270112271089232341485679099e+4
	},
	Qone[] =
	{
		0.11623987080032122878585294e+22,
		0.1185770712190320999837113348e+20,
		0.6092061398917521746105196863e+17,
		0.2081661221307607351240184229e+15,
		0.5243710262167649715406728642e+12,
		0.1013863514358673989967045588e+10,
		0.1501793594998585505921097578e+7,
		0.1606931573481487801970916749e+4,
		0.1e+1
	};
		
	p = Pone[8];
	q = Qone[8];
	for (i=7; i >= 0; i--)
	{
		p = p*x*x+Pone[i];
		q = q*x*x+Qone[i];
	}
	return (float)(p/q);
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelBessel_P1(const float x)
{
	double p, q;
	
	register long i;
	
	static const double
	Pone[] =
	{
		0.352246649133679798341724373e+5,
		0.62758845247161281269005675e+5,
		0.313539631109159574238669888e+5,
		0.49854832060594338434500455e+4,
		0.2111529182853962382105718e+3,
		0.12571716929145341558495e+1
	},
	Qone[] =
	{
		0.352246649133679798068390431e+5,
		0.626943469593560511888833731e+5,
		0.312404063819041039923015703e+5,
		0.4930396490181088979386097e+4,
		0.2030775189134759322293574e+3,
		0.1e+1
	};
		
	p = Pone[5];
	q = Qone[5];
	for (i=4; i >= 0; i--)
	{
		p = p*(8.0/x)*(8.0/x)+Pone[i];
		q = q*(8.0/x)*(8.0/x)+Qone[i];
	}
	return (float)(p/q);
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelBessel_Q1(const float x)
{
	double p, q;
	
	register long i;
	
	static const double
	Pone[] =
	{
		0.3511751914303552822533318e+3,
		0.7210391804904475039280863e+3,
		0.4259873011654442389886993e+3,
		0.831898957673850827325226e+2,
		0.45681716295512267064405e+1,
		0.3532840052740123642735e-1
	},
	Qone[] =
	{
		0.74917374171809127714519505e+4,
		0.154141773392650970499848051e+5,
		0.91522317015169922705904727e+4,
		0.18111867005523513506724158e+4,
		0.1038187585462133728776636e+3,
		0.1e+1
	};
		
	p = Pone[5];
	q = Qone[5];
	for (i=4; i >= 0; i--)
	{
		p = p*(8.0/x)*(8.0/x)+Pone[i];
		q = q*(8.0/x)*(8.0/x)+Qone[i];
	}
	return (float)(p/q);
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelBessel_Order1(float x)
{
	float p, q;
	
	if (x == 0.0)
		return (0.0f);
	p = x;
	if (x < 0.0)
		x=(-x);
	if (x < 8.0)
		return(p*KernelBessel_J1(x));
	q = (float)sqrt(2.0f/(PI*x))*(float)(KernelBessel_P1(x)*(1.0f/sqrt(2.0f)*(sin(x)-cos(x)))-8.0f/x*KernelBessel_Q1(x)*
		(-1.0f/sqrt(2.0f)*(sin(x)+cos(x))));
	if (p < 0.0f)
		q = (-q);
	return (q);
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelBessel(const float x)
{
	if (x == 0.0f)
		return(PI/4.0f);
	return(KernelBessel_Order1(PI*x)/(2.0f*x));
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelGaussian(const float x)
{
	return (float)(exp(-2.0f*x*x)*0.79788456080287f/*sqrt(2.0f/PI)*/);
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelQuadratic(const float x)
{
	if (x < -1.5f)
		return(0.0f);
	if (x < -0.5f)
		return(0.5f*(x+1.5f)*(x+1.5f));
	if (x < 0.5f)
		return(0.75f-x*x);
	if (x < 1.5f)
		return(0.5f*(x-1.5f)*(x-1.5f));
	return(0.0f);
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelMitchell(const float x)
{
#define KM_B (1.0f/3.0f)
#define KM_C (1.0f/3.0f)
#define KM_P0 ((  6.0f - 2.0f * KM_B ) / 6.0f)
#define KM_P2 ((-18.0f + 12.0f * KM_B + 6.0f * KM_C) / 6.0f)
#define KM_P3 (( 12.0f - 9.0f  * KM_B - 6.0f * KM_C) / 6.0f)
#define KM_Q0 ((  8.0f * KM_B + 24.0f * KM_C) / 6.0f)
#define KM_Q1 ((-12.0f * KM_B - 48.0f * KM_C) / 6.0f)
#define KM_Q2 ((  6.0f * KM_B + 30.0f * KM_C) / 6.0f)
#define KM_Q3 (( -1.0f * KM_B -  6.0f * KM_C) / 6.0f)
	
	if (x < -2.0)
		return(0.0f);
	if (x < -1.0)
		return(KM_Q0-x*(KM_Q1-x*(KM_Q2-x*KM_Q3)));
	if (x < 0.0f)
		return(KM_P0+x*x*(KM_P2-x*KM_P3));
	if (x < 1.0f)
		return(KM_P0+x*x*(KM_P2+x*KM_P3));
	if (x < 2.0f)
		return(KM_Q0+x*(KM_Q1+x*(KM_Q2+x*KM_Q3)));
	return(0.0f);
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelCatrom(const float x)
{
	if (x < -2.0)
		return(0.0f);
	if (x < -1.0)
		return(0.5f*(4.0f+x*(8.0f+x*(5.0f+x))));
	if (x < 0.0)
		return(0.5f*(2.0f+x*x*(-5.0f-3.0f*x)));
	if (x < 1.0)
		return(0.5f*(2.0f+x*x*(-5.0f+3.0f*x)));
	if (x < 2.0)
		return(0.5f*(4.0f+x*(-8.0f+x*(5.0f-x))));
	return(0.0f);
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::KernelPower(const float x, const float a)
{
	if (fabs(x)>1) return 0.0f;
	return (1.0f - (float)fabs(pow(x,a)));
}
////////////////////////////////////////////////////////////////////////////////

#endif
