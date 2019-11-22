/*
 * File:	ximaska.cpp
 * Purpose:	Platform Independent SKA Image Class Loader and Writer
 * 25/Sep/2007 Davide Pizzolato - www.xdp.it
 * CxImage version 6.0.0 02/Feb/2008
 */

#include "ximaska.h"

#if CXIMAGE_SUPPORT_SKA

////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageSKA::Decode(CxFile *hFile)
{
	if (hFile==NULL)
		return false;

	// read the  header
	SKAHEADER ska_header;
	hFile->Read(&ska_header,sizeof(SKAHEADER),1);

    ska_header.Width = ntohs(ska_header.Width);
    ska_header.Height = ntohs(ska_header.Height);
    ska_header.dwUnknown = ntohl(ska_header.dwUnknown);

	// check header
	if (ska_header.dwUnknown != 0x01000000 ||
		ska_header.Width > 0x7FFF || ska_header.Height > 0x7FFF ||
		ska_header.BppExp != 3)
		return false;

	if (info.nEscape == -1){
		head.biWidth = ska_header.Width ;
		head.biHeight= ska_header.Height;
		info.dwType = CXIMAGE_FORMAT_SKA;
		return true;
	}

	int bpp = 1<<ska_header.BppExp;

	Create(ska_header.Width,ska_header.Height,bpp,CXIMAGE_FORMAT_SKA);
	if (!IsValid())
		return false;

	// read the palette
	int nColors = 1<<bpp;
	rgb_color* ppal = (rgb_color*)malloc(nColors*sizeof(rgb_color));
	if (!ppal) return false;
	hFile->Read(ppal,nColors*sizeof(rgb_color),1);
	SetPalette(ppal,nColors);
	free(ppal);

	//read the image
	hFile->Read(GetBits(),ska_header.Width*ska_header.Height,1);

	//reorder rows
	if (GetEffWidth() != ska_header.Width){
		BYTE *src,*dst;
		src = GetBits() + ska_header.Width*(ska_header.Height-1);
		dst = GetBits(ska_header.Height-1);
		for(int y=0;y<ska_header.Height;y++){
			memcpy(dst,src,ska_header.Width);
			src -= ska_header.Width;
			dst -= GetEffWidth();
		}
	}

	Flip();

	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageSKA::Encode(CxFile * hFile)
{
	if (EncodeSafeCheck(hFile)) return false;

	if(head.biBitCount > 8)	{
		strcpy(info.szLastError,"SKA Images must be 8 bit or less");
		return false;
	}

	SKAHEADER ska_header;

	ska_header.Width = (unsigned short)GetWidth();
	ska_header.Height = (unsigned short)GetHeight();
	ska_header.BppExp = 3;
	ska_header.dwUnknown = 0x01000000;

    ska_header.Width = ntohs(ska_header.Width);
    ska_header.Height = ntohs(ska_header.Height);
    ska_header.dwUnknown = ntohl(ska_header.dwUnknown);

	hFile->Write(&ska_header,sizeof(SKAHEADER),1);

    ska_header.Width = ntohs(ska_header.Width);
    ska_header.Height = ntohs(ska_header.Height);
    ska_header.dwUnknown = ntohl(ska_header.dwUnknown);

	if (head.biBitCount<8) IncreaseBpp(8);

	rgb_color pal[256];
	for(int idx=0; idx<256; idx++){
		GetPaletteColor(idx,&(pal[idx].r),&(pal[idx].g),&(pal[idx].b));
	}

	hFile->Write(pal,256*sizeof(rgb_color),1);

	BYTE* src = GetBits(ska_header.Height-1);
	for(int y=0;y<ska_header.Height;y++){
		hFile->Write(src,ska_header.Width,1);
		src -= GetEffWidth();
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_SKA

