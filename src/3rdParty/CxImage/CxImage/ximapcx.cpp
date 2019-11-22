/*
 * File:	ximapcx.cpp
 * Purpose:	Platform Independent PCX Image Class Loader and Writer
 * 05/Jan/2002 Davide Pizzolato - www.xdp.it
 * CxImage version 6.0.0 02/Feb/2008
 *
 * based on ppmtopcx.c - convert a portable pixmap to PCX
 * Copyright (C) 1994 by Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 * based on ppmtopcx.c by Michael Davidson
 */

#include "ximapcx.h"

#if CXIMAGE_SUPPORT_PCX

#include "xmemfile.h"

#define PCX_MAGIC 0X0A  // PCX magic number
#define PCX_256_COLORS 0X0C  // magic number for 256 colors
#define PCX_HDR_SIZE 128  // size of PCX header
#define PCX_MAXCOLORS 256
#define PCX_MAXPLANES 4
#define PCX_MAXVAL 255

////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
bool CxImagePCX::Decode(CxFile *hFile)
{
	if (hFile == NULL) return false;

	PCXHEADER pcxHeader;
	int i, x, y, y2, nbytes, count, Height, Width;
	BYTE c, ColorMap[PCX_MAXCOLORS][3];
	BYTE *pcximage = NULL, *lpHead1 = NULL, *lpHead2 = NULL;
	BYTE *pcxplanes, *pcxpixels;

  cx_try
  {
	if (hFile->Read(&pcxHeader,sizeof(PCXHEADER),1)==0) cx_throw("Can't read PCX image");

	PCX_toh(&pcxHeader);

    if (pcxHeader.Manufacturer != PCX_MAGIC) cx_throw("Error: Not a PCX file");
    // Check for PCX run length encoding
    if (pcxHeader.Encoding != 1) cx_throw("PCX file has unknown encoding scheme");
 
    Width = (pcxHeader.Xmax - pcxHeader.Xmin) + 1;
    Height = (pcxHeader.Ymax - pcxHeader.Ymin) + 1;
	info.xDPI = pcxHeader.Hres;
	info.yDPI = pcxHeader.Vres;

	if (info.nEscape == -1){
		head.biWidth = Width;
		head.biHeight= Height;
		info.dwType = CXIMAGE_FORMAT_PCX;
		return true;
	}

    // Check that we can handle this image format
    if (pcxHeader.ColorPlanes > 4)
		cx_throw("Can't handle image with more than 4 planes");

	// Create the image
	if (pcxHeader.ColorPlanes >= 3 && pcxHeader.BitsPerPixel == 8){
		Create (Width, Height, 24, CXIMAGE_FORMAT_PCX);
#if CXIMAGE_SUPPORT_ALPHA
		if (pcxHeader.ColorPlanes==4) AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA
	} else if (pcxHeader.ColorPlanes == 4 && pcxHeader.BitsPerPixel == 1)
		Create (Width, Height, 4, CXIMAGE_FORMAT_PCX);
	else
		Create (Width, Height, pcxHeader.BitsPerPixel, CXIMAGE_FORMAT_PCX);

	if (info.nEscape) cx_throw("Cancelled"); // <vho> - cancel decoding

	//Read the image and check if it's ok
    nbytes = pcxHeader.BytesPerLine * pcxHeader.ColorPlanes * Height;
    lpHead1 = pcximage = (BYTE*)malloc(nbytes);
    while (nbytes > 0){
		if (hFile == NULL || hFile->Eof()) cx_throw("corrupted PCX");

		hFile->Read(&c,1,1);
		if ((c & 0XC0) != 0XC0){ // Repeated group
			*pcximage++ = c;
			--nbytes;
			continue;
		}
		count = c & 0X3F; // extract count
		hFile->Read(&c,1,1);
		if (count > nbytes) cx_throw("repeat count spans end of image");

		nbytes -= count;
		while (--count >=0) *pcximage++ = c;
	}
    pcximage = lpHead1;

	//store the palette
    for (i = 0; i < 16; i++){
		ColorMap[i][0] = pcxHeader.ColorMap[i][0];
		ColorMap[i][1] = pcxHeader.ColorMap[i][1];
		ColorMap[i][2] = pcxHeader.ColorMap[i][2];
	}
    if (pcxHeader.BitsPerPixel == 8 && pcxHeader.ColorPlanes == 1){
		hFile->Read(&c,1,1);
		if (c != PCX_256_COLORS) cx_throw("bad color map signature");
		
		for (i = 0; i < PCX_MAXCOLORS; i++){
			hFile->Read(&ColorMap[i][0],1,1);
			hFile->Read(&ColorMap[i][1],1,1);
			hFile->Read(&ColorMap[i][2],1,1);
		}
	}
    if (pcxHeader.BitsPerPixel == 1 && pcxHeader.ColorPlanes == 1){
		ColorMap[0][0] = ColorMap[0][1] = ColorMap[0][2] = 0;
		ColorMap[1][0] = ColorMap[1][1] = ColorMap[1][2] = 255;
	}

	for (DWORD idx=0; idx<head.biClrUsed; idx++) SetPaletteColor((BYTE)idx,ColorMap[idx][0],ColorMap[idx][1],ColorMap[idx][2]);

    lpHead2 = pcxpixels = (BYTE *)malloc(Width + pcxHeader.BytesPerLine * 8);
    // Convert the image
    for (y = 0; y < Height; y++){

		if (info.nEscape) cx_throw("Cancelled"); // <vho> - cancel decoding

		y2=Height-1-y;
		pcxpixels = lpHead2;
		pcxplanes = pcximage + (y * pcxHeader.BytesPerLine * pcxHeader.ColorPlanes);

		if (pcxHeader.ColorPlanes == 3 && pcxHeader.BitsPerPixel == 8){
			// Deal with 24 bit color image
			for (x = 0; x < Width; x++){
				SetPixelColor(x,y2,RGB(pcxplanes[x],pcxplanes[pcxHeader.BytesPerLine + x],pcxplanes[2*pcxHeader.BytesPerLine + x]));
			}
			continue;
#if CXIMAGE_SUPPORT_ALPHA
		} else if (pcxHeader.ColorPlanes == 4 && pcxHeader.BitsPerPixel == 8){
			for (x = 0; x < Width; x++){
				SetPixelColor(x,y2,RGB(pcxplanes[x],pcxplanes[pcxHeader.BytesPerLine + x],pcxplanes[2*pcxHeader.BytesPerLine + x]));
				AlphaSet(x,y2,pcxplanes[3*pcxHeader.BytesPerLine + x]);
			}
			continue;
#endif //CXIMAGE_SUPPORT_ALPHA
		} else if (pcxHeader.ColorPlanes == 1) {
			if (!PCX_UnpackPixels(pcxpixels, pcxplanes, pcxHeader.BytesPerLine, pcxHeader.ColorPlanes, pcxHeader.BitsPerPixel)){
				cx_throw("PCX_UnpackPixels: Can't handle packed pixels with more than 1 plane");
			}
		} else {
			if (!PCX_PlanesToPixels(pcxpixels, pcxplanes, pcxHeader.BytesPerLine, pcxHeader.ColorPlanes, pcxHeader.BitsPerPixel)){
				cx_throw("PCX_PlanesToPixels: more than 4 planes or more than 1 bit per pixel");
			}
		}
		for (x = 0; x < Width; x++)	SetPixelIndex(x,y2,pcxpixels[x]);
	}

  } cx_catch {
	if (strcmp(message,"")) strncpy(info.szLastError,message,255);
	if (lpHead1){ free(lpHead1); lpHead1 = NULL; }
    if (lpHead2){ free(lpHead2); lpHead2 = NULL; }
	return false;
  }
	if (lpHead1){ free(lpHead1); lpHead1 = NULL; }
    if (lpHead2){ free(lpHead2); lpHead2 = NULL; }
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
bool CxImagePCX::Encode(CxFile * hFile)
{
	if (EncodeSafeCheck(hFile)) return false;

  cx_try
  {
	PCXHEADER pcxHeader;
	memset(&pcxHeader,0,sizeof(pcxHeader));
	pcxHeader.Manufacturer = PCX_MAGIC;
	pcxHeader.Version = 5;
	pcxHeader.Encoding = 1;
	pcxHeader.Xmin = 0;
	pcxHeader.Ymin = 0;
	pcxHeader.Xmax = (WORD)head.biWidth-1;
	pcxHeader.Ymax = (WORD)head.biHeight-1;
	pcxHeader.Hres = (WORD)info.xDPI;
	pcxHeader.Vres = (WORD)info.yDPI;
	pcxHeader.Reserved = 0;
	pcxHeader.PaletteType = head.biClrUsed==0;

	switch(head.biBitCount){
	case 24:
	case 8:
		{
			pcxHeader.BitsPerPixel = 8;
			pcxHeader.ColorPlanes = head.biClrUsed==0 ? 3 : 1;
#if CXIMAGE_SUPPORT_ALPHA
			if (AlphaIsValid() && head.biClrUsed==0) pcxHeader.ColorPlanes =4;
#endif //CXIMAGE_SUPPORT_ALPHA
			pcxHeader.BytesPerLine = (WORD)head.biWidth;
			break;
		}
	default: //(4 1)
		pcxHeader.BitsPerPixel = 1;
		pcxHeader.ColorPlanes = head.biClrUsed==16 ? 4 : 1;
		pcxHeader.BytesPerLine = (WORD)((head.biWidth * pcxHeader.BitsPerPixel + 7)>>3);
	}

    if (pcxHeader.BitsPerPixel == 1 && pcxHeader.ColorPlanes == 1){
		pcxHeader.ColorMap[0][0] = pcxHeader.ColorMap[0][1] = pcxHeader.ColorMap[0][2] = 0;
		pcxHeader.ColorMap[1][0] = pcxHeader.ColorMap[1][1] = pcxHeader.ColorMap[1][2] = 255;
	}
	if (pcxHeader.BitsPerPixel == 1 && pcxHeader.ColorPlanes == 4){
		RGBQUAD c;
		for (int i = 0; i < 16; i++){
			c=GetPaletteColor(i);
			pcxHeader.ColorMap[i][0] = c.rgbRed;
			pcxHeader.ColorMap[i][1] = c.rgbGreen;
			pcxHeader.ColorMap[i][2] = c.rgbBlue;
		}
	}

	pcxHeader.BytesPerLine = (pcxHeader.BytesPerLine + 1)&(~1);

	PCX_toh(&pcxHeader);
	if (hFile->Write(&pcxHeader, sizeof(pcxHeader), 1) == 0 )
	   cx_throw("cannot write PCX header");
	PCX_toh(&pcxHeader);

	CxMemFile buffer;
	buffer.Open();

	BYTE c,n;
	long x,y;
	if (head.biClrUsed==0){
		for (y = head.biHeight-1; y >=0 ; y--){
			for (int p=0; p<pcxHeader.ColorPlanes; p++){
				c=n=0;
				for (x = 0; x<head.biWidth; x++){
					if (p==0)
						PCX_PackPixels(BlindGetPixelColor(x,y).rgbRed,c,n,buffer);
					else if (p==1)
						PCX_PackPixels(BlindGetPixelColor(x,y).rgbGreen,c,n,buffer);
					else if (p==2)
						PCX_PackPixels(BlindGetPixelColor(x,y).rgbBlue,c,n,buffer);
#if CXIMAGE_SUPPORT_ALPHA
					else if (p==3)
						PCX_PackPixels(BlindAlphaGet(x,y),c,n,buffer);
#endif //CXIMAGE_SUPPORT_ALPHA
				}
				PCX_PackPixels(-1-(head.biWidth&0x1),c,n,buffer);
			}
		}

		hFile->Write(buffer.GetBuffer(false),buffer.Tell(),1);

	} else if (head.biBitCount==8) {

		for (y = head.biHeight-1; y >=0 ; y--){
			c=n=0;
			for (x = 0; x<head.biWidth; x++){
				PCX_PackPixels(GetPixelIndex(x,y),c,n,buffer);
			}
			PCX_PackPixels(-1-(head.biWidth&0x1),c,n,buffer);
		}

		hFile->Write(buffer.GetBuffer(false),buffer.Tell(),1);

		if (head.biBitCount == 8){
			hFile->PutC(0x0C);
			BYTE* pal = (BYTE*)malloc(768);
			RGBQUAD c;
			for (int i=0;i<256;i++){
				c=GetPaletteColor(i);
				pal[3*i+0] = c.rgbRed;
				pal[3*i+1] = c.rgbGreen;
				pal[3*i+2] = c.rgbBlue;
			}
			hFile->Write(pal,768,1);
			free(pal);
		}
	} else { //(head.biBitCount==4) || (head.biBitCount==1)

		RGBQUAD *rgb = GetPalette();
		bool binvert = false;
		if (CompareColors(&rgb[0],&rgb[1])>0) binvert=(head.biBitCount==1);
		
		BYTE* plane = (BYTE*)malloc(pcxHeader.BytesPerLine);
		BYTE* raw = (BYTE*)malloc(head.biWidth);

		for(y = head.biHeight-1; y >=0 ; y--) {

			for( x = 0; x < head.biWidth; x++)	raw[x] = (BYTE)GetPixelIndex(x,y);

			if (binvert) for( x = 0; x < head.biWidth; x++)	raw[x] = 1-raw[x];

			for( x = 0; x < pcxHeader.ColorPlanes; x++ ) {
				PCX_PixelsToPlanes(raw, head.biWidth, plane, x);
				PCX_PackPlanes(plane, pcxHeader.BytesPerLine, buffer);
			}
		}

		free(plane);
		free(raw);

		hFile->Write(buffer.GetBuffer(false),buffer.Tell(),1);

	}

  } cx_catch {
	if (strcmp(message,"")) strncpy(info.szLastError,message,255);
	return false;
  }
    return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
// Convert multi-plane format into 1 pixel per byte
// from unpacked file data bitplanes[] into pixel row pixels[]
// image Height rows, with each row having planes image planes each
// bytesperline bytes
bool CxImagePCX::PCX_PlanesToPixels(BYTE * pixels, BYTE * bitplanes, short bytesperline, short planes, short bitsperpixel)
{
	int i, j, npixels;
	BYTE * p;
	if (planes > 4) return false;
	if (bitsperpixel != 1) return false;

	// Clear the pixel buffer
	npixels = (bytesperline * 8) / bitsperpixel;
	p = pixels;
	while (--npixels >= 0) *p++ = 0;

	// Do the format conversion
	for (i = 0; i < planes; i++){
		int pixbit, bits, mask;
		p = pixels;
		pixbit = (1 << i);  // pixel bit for this plane
		for (j = 0; j < bytesperline; j++){
			bits = *bitplanes++;
			for (mask = 0X80; mask != 0; mask >>= 1, p++)
				if (bits & mask) *p |= pixbit;
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
// convert packed pixel format into 1 pixel per byte
// from unpacked file data bitplanes[] into pixel row pixels[]
// image Height rows, with each row having planes image planes each
// bytesperline bytes
bool CxImagePCX::PCX_UnpackPixels(BYTE * pixels, BYTE * bitplanes, short bytesperline, short planes, short bitsperpixel)
{
	register int bits;
	if (planes != 1) return false;
	
	if (bitsperpixel == 8){  // 8 bits/pixels, no unpacking needed
		while (bytesperline-- > 0) *pixels++ = *bitplanes++;
	} else if (bitsperpixel == 4){  // 4 bits/pixel, two pixels per byte
		while (bytesperline-- > 0){
			bits = *bitplanes++;
			*pixels++ = (BYTE)((bits >> 4) & 0X0F);
			*pixels++ = (BYTE)((bits) & 0X0F);
		}
	} else if (bitsperpixel == 2){  // 2 bits/pixel, four pixels per byte
		while (bytesperline-- > 0){
			bits = *bitplanes++;
			*pixels++ = (BYTE)((bits >> 6) & 0X03);
			*pixels++ = (BYTE)((bits >> 4) & 0X03);
			*pixels++ = (BYTE)((bits >> 2) & 0X03);
			*pixels++ = (BYTE)((bits) & 0X03);
		}
	} else if (bitsperpixel == 1){  // 1 bits/pixel, 8 pixels per byte
		while (bytesperline-- > 0){
			bits = *bitplanes++;
			*pixels++ = ((bits & 0X80) != 0);
			*pixels++ = ((bits & 0X40) != 0);
			*pixels++ = ((bits & 0X20) != 0);
			*pixels++ = ((bits & 0X10) != 0);
			*pixels++ = ((bits & 0X08) != 0);
			*pixels++ = ((bits & 0X04) != 0);
			*pixels++ = ((bits & 0X02) != 0);
			*pixels++ = ((bits & 0X01) != 0);
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/* PCX_PackPixels(const long p,BYTE &c, BYTE &n, long &l, CxFile &f)
 * p = current pixel (-1 ends the line -2 ends odd line)
 * c = previous pixel
 * n = number of consecutive pixels
 */
void CxImagePCX::PCX_PackPixels(const long p,BYTE &c, BYTE &n, CxFile &f)
{
	if (p!=c && n){
		if (n==1 && c<0xC0){
			f.PutC(c);
		} else {
			f.PutC(0xC0|n);
			f.PutC(c);
		}
		n=0;
	}
	if (n==0x3F) {
		f.PutC(0xFF);
		f.PutC(c);
		n=0;
	}
	if (p==-2) f.PutC(0);
	c=(BYTE)p;
	n++;
}
////////////////////////////////////////////////////////////////////////////////
void CxImagePCX::PCX_PackPlanes(BYTE* buff, const long size, CxFile &f)
{
    BYTE *start,*end;
    BYTE c, previous, count;

	start = buff;
    end = buff + size;
    previous = *start++;
    count    = 1;

    while (start < end) {
        c = *start++;
        if (c == previous && count < 63) {
            ++count;
            continue;
        }

        if (count > 1 || (previous & 0xc0) == 0xc0) {
            f.PutC( count | 0xc0 );
        }
        f.PutC(previous);
        previous = c;
        count   = 1;
    }

    if (count > 1 || (previous & 0xc0) == 0xc0) {
        count |= 0xc0;
        f.PutC(count);
    }
    f.PutC(previous);
}
////////////////////////////////////////////////////////////////////////////////
void CxImagePCX::PCX_PixelsToPlanes(BYTE* raw, long width, BYTE* buf, long plane)
{
    int cbit, x, mask;
    unsigned char *cp = buf-1;

    mask = 1 << plane;
    cbit = -1;
    for( x = 0; x < width; x++ ) {
        if( cbit < 0 ) {
            cbit = 7;
            *++cp = 0;
        }
        if( raw[x] & mask )
            *cp |= (1<<cbit);
        --cbit;
    }
}
////////////////////////////////////////////////////////////////////////////////
void CxImagePCX::PCX_toh(PCXHEADER* p)
{
	p->Xmin = ntohs(p->Xmin);
	p->Ymin = ntohs(p->Ymin);
	p->Xmax = ntohs(p->Xmax);
	p->Ymax = ntohs(p->Ymax);
	p->Hres = ntohs(p->Hres);
	p->Vres = ntohs(p->Vres);
	p->BytesPerLine = ntohs(p->BytesPerLine);
	p->PaletteType = ntohs(p->PaletteType);
}
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_PCX
