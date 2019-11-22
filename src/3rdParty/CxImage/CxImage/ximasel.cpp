// xImaSel.cpp : Selection functions
/* 07/08/2001 v1.00 - Davide Pizzolato - www.xdp.it
 * CxImage version 6.0.0 02/Feb/2008
 */

#include "ximage.h"

#if CXIMAGE_SUPPORT_SELECTION

////////////////////////////////////////////////////////////////////////////////
/**
 * Checks if the image has a valid selection.
 */
bool CxImage::SelectionIsValid()
{
	return pSelection!=0;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Gets the smallest rectangle that contains the selection 
 */
void CxImage::SelectionGetBox(RECT& r)
{
	memcpy(&r,&info.rSelectionBox,sizeof(RECT));
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Empties the selection.
 */
bool CxImage::SelectionClear(BYTE level)
{
	if (pSelection){
		if (level==0){
			memset(pSelection,0,head.biWidth * head.biHeight);
			info.rSelectionBox.left = head.biWidth;
			info.rSelectionBox.bottom = head.biHeight;
			info.rSelectionBox.right = info.rSelectionBox.top = 0;
		} else {
			memset(pSelection,level,head.biWidth * head.biHeight);
			info.rSelectionBox.right = head.biWidth;
			info.rSelectionBox.top = head.biHeight;
			info.rSelectionBox.left = info.rSelectionBox.bottom = 0;
		}
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Allocates an empty selection.
 */
bool CxImage::SelectionCreate()
{
	SelectionDelete();
	pSelection = (BYTE*)calloc(head.biWidth * head.biHeight, 1);
	return (pSelection!=0);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Deallocates the selction.
 */
bool CxImage::SelectionDelete()
{
	if (pSelection){
		free(pSelection);
		pSelection=NULL;
	}
	info.rSelectionBox.left = head.biWidth;
	info.rSelectionBox.bottom = head.biHeight;
	info.rSelectionBox.right = info.rSelectionBox.top = 0;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Checks if the coordinates are inside the selection.
 */
bool CxImage::SelectionIsInside(long x, long y)
{
	if (IsInside(x,y)){
		if (pSelection==NULL) return true;
		return pSelection[x+y*head.biWidth]!=0;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Checks if the coordinates are inside the selection.
 * "blind" version assumes that (x,y) is inside to the image.
 */
bool CxImage::BlindSelectionIsInside(long x, long y)
{
#ifdef _DEBUG
	if (!IsInside(x,y))
  #if CXIMAGE_SUPPORT_EXCEPTION_HANDLING
		throw 0;
  #else
		return 0;
  #endif
#endif
	if (pSelection==NULL) return true;
	return pSelection[x+y*head.biWidth]!=0;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Adds a rectangle to the existing selection.
 */
bool CxImage::SelectionAddRect(RECT r, BYTE level)
{
	if (pSelection==NULL) SelectionCreate();
	if (pSelection==NULL) return false;

	RECT r2;
	if (r.left<r.right) {r2.left=r.left; r2.right=r.right; } else {r2.left=r.right ; r2.right=r.left; }
	if (r.bottom<r.top) {r2.bottom=r.bottom; r2.top=r.top; } else {r2.bottom=r.top ; r2.top=r.bottom; }

	if (info.rSelectionBox.top <= r2.top) info.rSelectionBox.top = max(0L,min(head.biHeight,r2.top+1));
	if (info.rSelectionBox.left > r2.left) info.rSelectionBox.left = max(0L,min(head.biWidth,r2.left));
	if (info.rSelectionBox.right <= r2.right) info.rSelectionBox.right = max(0L,min(head.biWidth,r2.right+1));
	if (info.rSelectionBox.bottom > r2.bottom) info.rSelectionBox.bottom = max(0L,min(head.biHeight,r2.bottom));

	long ymin = max(0L,min(head.biHeight,r2.bottom));
	long ymax = max(0L,min(head.biHeight,r2.top+1));
	long xmin = max(0L,min(head.biWidth,r2.left));
	long xmax = max(0L,min(head.biWidth,r2.right+1));

	for (long y=ymin; y<ymax; y++)
		memset(pSelection + xmin + y * head.biWidth, level, xmax-xmin);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Adds an ellipse to the existing selection.
 */
bool CxImage::SelectionAddEllipse(RECT r, BYTE level)
{
	if (pSelection==NULL) SelectionCreate();
	if (pSelection==NULL) return false;

	long xradius = abs(r.right - r.left)/2;
	long yradius = abs(r.top - r.bottom)/2;
	if (xradius==0 || yradius==0) return false;

	long xcenter = (r.right + r.left)/2;
	long ycenter = (r.top + r.bottom)/2;

	if (info.rSelectionBox.left > (xcenter - xradius)) info.rSelectionBox.left = max(0L,min(head.biWidth,(xcenter - xradius)));
	if (info.rSelectionBox.right <= (xcenter + xradius)) info.rSelectionBox.right = max(0L,min(head.biWidth,(xcenter + xradius + 1)));
	if (info.rSelectionBox.bottom > (ycenter - yradius)) info.rSelectionBox.bottom = max(0L,min(head.biHeight,(ycenter - yradius)));
	if (info.rSelectionBox.top <= (ycenter + yradius)) info.rSelectionBox.top = max(0L,min(head.biHeight,(ycenter + yradius + 1)));

	long xmin = max(0L,min(head.biWidth,xcenter - xradius));
	long xmax = max(0L,min(head.biWidth,xcenter + xradius + 1));
	long ymin = max(0L,min(head.biHeight,ycenter - yradius));
	long ymax = max(0L,min(head.biHeight,ycenter + yradius + 1));

	long y,yo;
	for (y=ymin; y<min(ycenter,ymax); y++){
		for (long x=xmin; x<xmax; x++){
			yo = (long)(ycenter - yradius * sqrt(1-pow((float)(x - xcenter)/(float)xradius,2)));
			if (yo<y) pSelection[x + y * head.biWidth] = level;
		}
	}
	for (y=ycenter; y<ymax; y++){
		for (long x=xmin; x<xmax; x++){
			yo = (long)(ycenter + yradius * sqrt(1-pow((float)(x - xcenter)/(float)xradius,2)));
			if (yo>y) pSelection[x + y * head.biWidth] = level;
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Inverts the selection.
 * Note: the SelectionBox is set to "full image", call SelectionGetBox before (if necessary)
 */
bool CxImage::SelectionInvert()
{
	if (pSelection) {
		BYTE *iSrc=pSelection;
		long n=head.biHeight*head.biWidth;
		for(long i=0; i < n; i++){
			*iSrc=(BYTE)~(*(iSrc));
			iSrc++;
		}

		SelectionRebuildBox();

		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Imports an existing region from another image with the same width and height.
 */
bool CxImage::SelectionCopy(CxImage &from)
{
	if (from.pSelection == NULL || head.biWidth != from.head.biWidth || head.biHeight != from.head.biHeight) return false;
	if (pSelection==NULL) pSelection = (BYTE*)malloc(head.biWidth * head.biHeight);
	if (pSelection==NULL) return false;
	memcpy(pSelection,from.pSelection,head.biWidth * head.biHeight);
	memcpy(&info.rSelectionBox,&from.info.rSelectionBox,sizeof(RECT));
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Adds a polygonal region to the existing selection. points points to an array of POINT structures.
 * Each structure specifies the x-coordinate and y-coordinate of one vertex of the polygon.
 * npoints specifies the number of POINT structures in the array pointed to by points.
 */
bool CxImage::SelectionAddPolygon(POINT *points, long npoints, BYTE level)
{
	if (points==NULL || npoints<3) return false;

	if (pSelection==NULL) SelectionCreate();
	if (pSelection==NULL) return false;

	BYTE* plocal = (BYTE*)calloc(head.biWidth*head.biHeight, 1);
	RECT localbox = {head.biWidth,0,0,head.biHeight};

	long x,y,i=0;
	POINT *current;
	POINT *next = NULL;
	POINT *start = NULL;
	//trace contour
	while (i < npoints){
		current = &points[i];
		if (current->x!=-1){
			if (i==0 || (i>0 && points[i-1].x==-1)) start = &points[i];

			if ((i+1)==npoints || points[i+1].x==-1)
				next = start;
			else
				next = &points[i+1];

			float beta;
			if (current->x != next->x){
				beta = (float)(next->y - current->y)/(float)(next->x - current->x);
				if (current->x < next->x){
					for (x=current->x; x<=next->x; x++){
						y = (long)(current->y + (x - current->x) * beta);
						if (IsInside(x,y)) plocal[x + y * head.biWidth] = 255;
					}
				} else {
					for (x=current->x; x>=next->x; x--){
						y = (long)(current->y + (x - current->x) * beta);
						if (IsInside(x,y)) plocal[x + y * head.biWidth] = 255;
					}
				}
			}
			if (current->y != next->y){
				beta = (float)(next->x - current->x)/(float)(next->y - current->y);
				if (current->y < next->y){
					for (y=current->y; y<=next->y; y++){
						x = (long)(current->x + (y - current->y) * beta);
						if (IsInside(x,y)) plocal[x + y * head.biWidth] = 255;
					}
				} else {
					for (y=current->y; y>=next->y; y--){
						x = (long)(current->x + (y - current->y) * beta);
						if (IsInside(x,y)) plocal[x + y * head.biWidth] = 255;
					}
				}
			}
		}

		RECT r2;
		if (current->x < next->x) {r2.left=current->x; r2.right=next->x; } else {r2.left=next->x ; r2.right=current->x; }
		if (current->y < next->y) {r2.bottom=current->y; r2.top=next->y; } else {r2.bottom=next->y ; r2.top=current->y; }
		if (localbox.top < r2.top) localbox.top = max(0L,min(head.biHeight-1,r2.top+1));
		if (localbox.left > r2.left) localbox.left = max(0L,min(head.biWidth-1,r2.left-1));
		if (localbox.right < r2.right) localbox.right = max(0L,min(head.biWidth-1,r2.right+1));
		if (localbox.bottom > r2.bottom) localbox.bottom = max(0L,min(head.biHeight-1,r2.bottom-1));

		i++;
	}

	//fill the outer region
	long npix=(localbox.right - localbox.left)*(localbox.top - localbox.bottom);
	POINT* pix = (POINT*)calloc(npix,sizeof(POINT));
	BYTE back=0, mark=1;
	long fx, fy, fxx, fyy, first, last;
	long xmin = 0;
	long xmax = 0;
	long ymin = 0;
	long ymax = 0;

	for (int side=0; side<4; side++){
		switch(side){
		case 0:
			xmin=localbox.left; xmax=localbox.right+1; ymin=localbox.bottom; ymax=localbox.bottom+1;
			break;
		case 1:
			xmin=localbox.right; xmax=localbox.right+1; ymin=localbox.bottom; ymax=localbox.top+1;
			break;
		case 2:
			xmin=localbox.left; xmax=localbox.right+1; ymin=localbox.top; ymax=localbox.top+1;
			break;
		case 3:
			xmin=localbox.left; xmax=localbox.left+1; ymin=localbox.bottom; ymax=localbox.top+1;
			break;
		}
		//fill from the border points
		for(y=ymin;y<ymax;y++){
			for(x=xmin;x<xmax;x++){
				if (plocal[x+y*head.biWidth]==0){
					// Subject: FLOOD FILL ROUTINE              Date: 12-23-97 (00:57)       
					// Author:  Petter Holmberg                 Code: QB, QBasic, PDS        
					// Origin:  petter.holmberg@usa.net         Packet: GRAPHICS.ABC
					first=0;
					last=1;
					while(first!=last){
						fx = pix[first].x;
						fy = pix[first].y;
						fxx = fx + x;
						fyy = fy + y;
						for(;;)
						{
							if ((plocal[fxx + fyy*head.biWidth] == back) &&
								fxx>=localbox.left && fxx<=localbox.right && fyy>=localbox.bottom && fyy<=localbox.top )
							{
								plocal[fxx + fyy*head.biWidth] = mark;
								if (fyy > 0 && plocal[fxx + (fyy - 1)*head.biWidth] == back){
									pix[last].x = fx;
									pix[last].y = fy - 1;
									last++;
									if (last == npix) last = 0;
								}
								if ((fyy + 1)<head.biHeight && plocal[fxx + (fyy + 1)*head.biWidth] == back){
									pix[last].x = fx;
									pix[last].y = fy + 1;
									last++;
									if (last == npix) last = 0;
								}
							} else {
								break;
							}
							fx++;
							fxx++;
						};

						fx = pix[first].x - 1;
						fy = pix[first].y;
						fxx = fx + x;
						fyy = fy + y;

						for( ;; )
						{
							if ((plocal[fxx + fyy*head.biWidth] == back) &&
								fxx>=localbox.left && fxx<=localbox.right && fyy>=localbox.bottom && fyy<=localbox.top )
							{
								plocal[fxx + (y + fy)*head.biWidth] = mark;
								if (fyy > 0 && plocal[fxx + (fyy - 1)*head.biWidth] == back){
									pix[last].x = fx;
									pix[last].y = fy - 1;
									last++;
									if (last == npix) last = 0;
								}
								if ((fyy + 1)<head.biHeight && plocal[fxx + (fyy + 1)*head.biWidth] == back){
									pix[last].x = fx;
									pix[last].y = fy + 1;
									last++;
									if (last == npix) last = 0;
								}
							} else {
								break;
							}
							fx--;
							fxx--;
						}
						
						first++;
						if (first == npix) first = 0;
					}
				}
			}
		}
	}

	//transfer the region
	long yoffset;
	for (y=localbox.bottom; y<=localbox.top; y++){
		yoffset = y * head.biWidth;
		for (x=localbox.left; x<=localbox.right; x++)
			if (plocal[x + yoffset]!=1) pSelection[x + yoffset]=level;
	}
	if (info.rSelectionBox.top <= localbox.top) info.rSelectionBox.top = min(head.biHeight,localbox.top + 1);
	if (info.rSelectionBox.left > localbox.left) info.rSelectionBox.left = min(head.biWidth,localbox.left);
	if (info.rSelectionBox.right <= localbox.right) info.rSelectionBox.right = min(head.biWidth,localbox.right + 1);
	if (info.rSelectionBox.bottom > localbox.bottom) info.rSelectionBox.bottom = min(head.biHeight,localbox.bottom);

	free(plocal);
	free(pix);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Adds to the selection all the pixels matching the specified color.
 */
bool CxImage::SelectionAddColor(RGBQUAD c, BYTE level)
{
    if (pSelection==NULL) SelectionCreate();
	if (pSelection==NULL) return false;

	RECT localbox = {head.biWidth,0,0,head.biHeight};

    for (long y = 0; y < head.biHeight; y++){
        for (long x = 0; x < head.biWidth; x++){
            RGBQUAD color = BlindGetPixelColor(x, y);
            if (color.rgbRed   == c.rgbRed &&
				color.rgbGreen == c.rgbGreen &&
                color.rgbBlue  == c.rgbBlue)
            {
                pSelection[x + y * head.biWidth] = level;

				if (localbox.top < y) localbox.top = y;
				if (localbox.left > x) localbox.left = x;
				if (localbox.right < x) localbox.right = x;
				if (localbox.bottom > y) localbox.bottom = y;
            }
        }
    }

	if (info.rSelectionBox.top <= localbox.top) info.rSelectionBox.top = localbox.top + 1;
	if (info.rSelectionBox.left > localbox.left) info.rSelectionBox.left = localbox.left;
	if (info.rSelectionBox.right <= localbox.right) info.rSelectionBox.right = localbox.right + 1;
	if (info.rSelectionBox.bottom > localbox.bottom) info.rSelectionBox.bottom = localbox.bottom;

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Adds a single pixel to the existing selection.
 */
bool CxImage::SelectionAddPixel(long x, long y, BYTE level)
{
    if (pSelection==NULL) SelectionCreate();
	if (pSelection==NULL) return false;

    if (IsInside(x,y)) {
        pSelection[x + y * head.biWidth] = level; // set the correct mask bit

		if (info.rSelectionBox.top <= y) info.rSelectionBox.top = y+1;
		if (info.rSelectionBox.left > x) info.rSelectionBox.left = x;
		if (info.rSelectionBox.right <= x) info.rSelectionBox.right = x+1;
		if (info.rSelectionBox.bottom > y) info.rSelectionBox.bottom = y;

        return true;
    }

    return false;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Exports the selection channel in a 8bpp grayscale image.
 */
bool CxImage::SelectionSplit(CxImage *dest)
{
	if (!pSelection || !dest) return false;

	CxImage tmp(head.biWidth,head.biHeight,8);
	if (!tmp.IsValid()){
		strcpy(info.szLastError,tmp.GetLastError());
		return false;
	}

	for(long y=0; y<head.biHeight; y++){
		for(long x=0; x<head.biWidth; x++){
			tmp.BlindSetPixelIndex(x,y,pSelection[x+y*head.biWidth]);
		}
	}

	tmp.SetGrayPalette();
	dest->Transfer(tmp);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Creates the selection channel from a gray scale image.
 * black = unselected
 */
bool CxImage::SelectionSet(CxImage &from)
{
	if (!from.IsGrayScale() || head.biWidth != from.head.biWidth || head.biHeight != from.head.biHeight){
		strcpy(info.szLastError,"CxImage::SelectionSet: wrong width or height, or image is not gray scale");
		return false;
	}

	if (pSelection==NULL) pSelection = (BYTE*)malloc(head.biWidth * head.biHeight);

	BYTE* src = from.info.pImage;
	BYTE* dst = pSelection;
	if (src==NULL || dst==NULL){
		strcpy(info.szLastError,"CxImage::SelectionSet: null pointer");
		return false;
	}

	for (long y=0; y<head.biHeight; y++){
		memcpy(dst,src,head.biWidth);
		dst += head.biWidth;
		src += from.info.dwEffWidth;
	}

	SelectionRebuildBox();

	return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Sets the Selection level for a single pixel
 * internal use only: doesn't set SelectionBox. Use SelectionAddPixel
 */
void CxImage::SelectionSet(const long x,const long y,const BYTE level)
{
	if (pSelection && IsInside(x,y)) pSelection[x+y*head.biWidth]=level;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Gets the Selection level for a single pixel 
 */
BYTE CxImage::SelectionGet(const long x,const long y)
{
	if (pSelection && IsInside(x,y)) return pSelection[x+y*head.biWidth];
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Rebuilds the SelectionBox 
 */
void CxImage::SelectionRebuildBox()
{
	info.rSelectionBox.left = head.biWidth;
	info.rSelectionBox.bottom = head.biHeight;
	info.rSelectionBox.right = info.rSelectionBox.top = 0;

	if (!pSelection)
		return;

	long x,y;

	for (y=0; y<head.biHeight; y++){
		for (x=0; x<info.rSelectionBox.left; x++){
			if (pSelection[x+y*head.biWidth]){
				info.rSelectionBox.left = x;
				continue;
			}
		}
	}

	for (y=0; y<head.biHeight; y++){
		for (x=head.biWidth-1; x>=info.rSelectionBox.right; x--){
			if (pSelection[x+y*head.biWidth]){
				info.rSelectionBox.right = x+1;
				continue;
			}
		}
	}

	for (x=0; x<head.biWidth; x++){
		for (y=0; y<info.rSelectionBox.bottom; y++){
			if (pSelection[x+y*head.biWidth]){
				info.rSelectionBox.bottom = y;
				continue;
			}
		}
	}

	for (x=0; x<head.biWidth; x++){
		for (y=head.biHeight-1; y>=info.rSelectionBox.top; y--){
			if (pSelection[x+y*head.biWidth]){
				info.rSelectionBox.top = y+1;
				continue;
			}
		}
	}

}
////////////////////////////////////////////////////////////////////////////////
/**
 * Gets the Selection level for a single pixel 
 * "blind" version assumes that (x,y) is inside to the image.
 */
BYTE CxImage::BlindSelectionGet(const long x,const long y)
{
#ifdef _DEBUG
	if (!IsInside(x,y) || (pSelection==0))
  #if CXIMAGE_SUPPORT_EXCEPTION_HANDLING
		throw 0;
  #else
		return 0;
  #endif
#endif
	return pSelection[x+y*head.biWidth];
}
////////////////////////////////////////////////////////////////////////////////
/**
 * Returns pointer to selection data for pixel (x,y).
 */
BYTE* CxImage::SelectionGetPointer(const long x,const long y)
{
	if (pSelection && IsInside(x,y)) return pSelection+x+y*head.biWidth;
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionFlip()
{
	if (!pSelection) return false;

	BYTE *buff = (BYTE*)malloc(head.biWidth);
	if (!buff) return false;

	BYTE *iSrc,*iDst;
	iSrc = pSelection + (head.biHeight-1)*head.biWidth;
	iDst = pSelection;
	for (long i=0; i<(head.biHeight/2); ++i)
	{
		memcpy(buff, iSrc, head.biWidth);
		memcpy(iSrc, iDst, head.biWidth);
		memcpy(iDst, buff, head.biWidth);
		iSrc-=head.biWidth;
		iDst+=head.biWidth;
	}

	free(buff);

	long top = info.rSelectionBox.top;
	info.rSelectionBox.top = head.biHeight - info.rSelectionBox.bottom;
	info.rSelectionBox.bottom = head.biHeight - top;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SelectionMirror()
{
	if (!pSelection) return false;
	BYTE* pSelection2 = (BYTE*)malloc(head.biWidth * head.biHeight);
	if (!pSelection2) return false;
	
	BYTE *iSrc,*iDst;
	long wdt=head.biWidth-1;
	iSrc=pSelection + wdt;
	iDst=pSelection2;
	for(long y=0; y < head.biHeight; y++){
		for(long x=0; x <= wdt; x++)
			*(iDst+x)=*(iSrc-x);
		iSrc+=head.biWidth;
		iDst+=head.biWidth;
	}
	free(pSelection);
	pSelection=pSelection2;
	
	long left = info.rSelectionBox.left;
	info.rSelectionBox.left = head.biWidth - info.rSelectionBox.right;
	info.rSelectionBox.right = head.biWidth - left;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_WINDOWS
/**
 * Converts the selection in a HRGN object.
 */
bool CxImage::SelectionToHRGN(HRGN& region)
{
	if (pSelection && region){           
        for(int y = 0; y < head.biHeight; y++){
            HRGN hTemp = NULL;
            int iStart = -1;
            int x = 0;
			for(; x < head.biWidth; x++){
                if (pSelection[x + y * head.biWidth] != 0){
					if (iStart == -1) iStart = x;
					continue;
                }else{
                    if (iStart >= 0){
                        hTemp = CreateRectRgn(iStart, y, x, y + 1);
                        CombineRgn(region, hTemp, region, RGN_OR);
                        DeleteObject(hTemp);
                        iStart = -1;
                    }
                }
            }
            if (iStart >= 0){
                hTemp = CreateRectRgn(iStart, y, x, y + 1);
                CombineRgn(region, hTemp, region, RGN_OR);
                DeleteObject(hTemp);
                iStart = -1;
            }
        }
		return true;
    }
	return false;
}
#endif //CXIMAGE_SUPPORT_WINDOWS
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_SELECTION
