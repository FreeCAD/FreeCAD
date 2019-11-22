/*
 * File:	ximaico.cpp
 * Purpose:	Platform Independent ICON Image Class Loader and Writer (MS version)
 * 07/Aug/2001 Davide Pizzolato - www.xdp.it
 * CxImage version 6.0.0 02/Feb/2008
 */

#include "ximaico.h"

#if CXIMAGE_SUPPORT_ICO

////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageICO::Decode(CxFile *hFile)
{
	if (hFile==NULL) return false;

	DWORD off = hFile->Tell(); //<yuandi>
	int	page=info.nFrame;	//internal icon structure indexes

	// read the first part of the header
	ICONHEADER icon_header;
	hFile->Read(&icon_header,sizeof(ICONHEADER),1);

	icon_header.idType = ntohs(icon_header.idType);
	icon_header.idCount = ntohs(icon_header.idCount);

	// check if it's an icon or a cursor
	if ((icon_header.idReserved == 0) && ((icon_header.idType == 1)||(icon_header.idType == 2))) {

		info.nNumFrames = icon_header.idCount;

		// load the icon descriptions
		ICONDIRENTRY *icon_list = (ICONDIRENTRY *)malloc(icon_header.idCount * sizeof(ICONDIRENTRY));
		int c;
		for (c = 0; c < icon_header.idCount; c++) {
			hFile->Read(icon_list + c, sizeof(ICONDIRENTRY), 1);

			icon_list[c].wPlanes = ntohs(icon_list[c].wPlanes);
			icon_list[c].wBitCount = ntohs(icon_list[c].wBitCount);
			icon_list[c].dwBytesInRes = ntohl(icon_list[c].dwBytesInRes);
			icon_list[c].dwImageOffset = ntohl(icon_list[c].dwImageOffset);
		}
		
		if ((page>=0)&&(page<icon_header.idCount)){

			if (info.nEscape == -1) {
				// Return output dimensions only
				head.biWidth = icon_list[page].bWidth;
				head.biHeight = icon_list[page].bHeight;
#if CXIMAGE_SUPPORT_PNG
				if (head.biWidth==0 && head.biHeight==0)
				{	// Vista icon support
					hFile->Seek(off + icon_list[page].dwImageOffset, SEEK_SET);
					CxImage png;
					png.SetEscape(-1);
					if (png.Decode(hFile,CXIMAGE_FORMAT_PNG)){
						Transfer(png);
						info.nNumFrames = icon_header.idCount;
					}
				}
#endif //CXIMAGE_SUPPORT_PNG
				free(icon_list);
				info.dwType = CXIMAGE_FORMAT_ICO;
				return true;
			}

			// get the bit count for the colors in the icon <CoreyRLucier>
			BITMAPINFOHEADER bih;
			hFile->Seek(off + icon_list[page].dwImageOffset, SEEK_SET);

			if (icon_list[page].bWidth==0 && icon_list[page].bHeight==0)
			{	// Vista icon support
#if CXIMAGE_SUPPORT_PNG
				CxImage png;
				if (png.Decode(hFile,CXIMAGE_FORMAT_PNG)){
					Transfer(png);
					info.nNumFrames = icon_header.idCount;
				}
				SetType(CXIMAGE_FORMAT_ICO);
#endif //CXIMAGE_SUPPORT_PNG
			}
			else
			{	// standard icon
				hFile->Read(&bih,sizeof(BITMAPINFOHEADER),1);

				bihtoh(&bih);

				c = bih.biBitCount;

				// allocate memory for one icon
				Create(icon_list[page].bWidth,icon_list[page].bHeight, c, CXIMAGE_FORMAT_ICO);	//image creation

				// read the palette
				RGBQUAD pal[256];
				if (bih.biClrUsed)
					hFile->Read(pal,bih.biClrUsed*sizeof(RGBQUAD), 1);
				else
					hFile->Read(pal,head.biClrUsed*sizeof(RGBQUAD), 1);

				SetPalette(pal,head.biClrUsed);	//palette assign

				//read the icon
				if (c<=24){
					hFile->Read(info.pImage, head.biSizeImage, 1);
				} else { // 32 bit icon
					BYTE* buf=(BYTE*)malloc(4*head.biHeight*head.biWidth);
					BYTE* src = buf;
					hFile->Read(buf, 4*head.biHeight*head.biWidth, 1);
#if CXIMAGE_SUPPORT_ALPHA
					if (!AlphaIsValid()) AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA
					for (long y = 0; y < head.biHeight; y++) {
						BYTE* dst = GetBits(y);
						for(long x=0;x<head.biWidth;x++){
							*dst++=src[0];
							*dst++=src[1];
							*dst++=src[2];
#if CXIMAGE_SUPPORT_ALPHA
							AlphaSet(x,y,src[3]);
#endif //CXIMAGE_SUPPORT_ALPHA
							src+=4;
						}
					}
					free(buf);
				}
				// apply the AND and XOR masks
				int maskwdt = ((head.biWidth+31) / 32) * 4;	//line width of AND mask (always 1 Bpp)
				int masksize = head.biHeight * maskwdt;				//size of mask
				BYTE *mask = (BYTE *)malloc(masksize);
				if (hFile->Read(mask, masksize, 1)){

					bool bGoodMask=false;
					for (int im=0;im<masksize;im++){
						if (mask[im]!=255){
							bGoodMask=true;
							break;
						}
					}

					if (bGoodMask){
#if CXIMAGE_SUPPORT_ALPHA
						bool bNeedAlpha = false;
						if (!AlphaIsValid()){
							AlphaCreate();
						} else { 
							bNeedAlpha=true; //32bit icon
						}
						int x,y;
						for (y = 0; y < head.biHeight; y++) {
							for (x = 0; x < head.biWidth; x++) {
								if (((mask[y*maskwdt+(x>>3)]>>(7-x%8))&0x01)){
									AlphaSet(x,y,0);
									bNeedAlpha=true;
								}
							}
						}
						if (!bNeedAlpha) AlphaDelete();
#endif //CXIMAGE_SUPPORT_ALPHA

						//check if there is only one transparent color
						RGBQUAD cc,ct;
						long* pcc = (long*)&cc;
						long* pct = (long*)&ct;
						int nTransColors=0;
						int nTransIndex=0;
						for (y = 0; y < head.biHeight; y++){
							for (x = 0; x < head.biWidth; x++){
								if (((mask[y*maskwdt+(x>>3)] >> (7-x%8)) & 0x01)){
									cc = GetPixelColor(x,y,false);
									if (nTransColors==0){
										nTransIndex = GetPixelIndex(x,y);
										nTransColors++;
										ct = cc;
									} else {
										if (*pct!=*pcc){
											nTransColors++;
										}
									}
								}
							}
						}
						if (nTransColors==1){
							SetTransColor(ct);
							SetTransIndex(nTransIndex);
#if CXIMAGE_SUPPORT_ALPHA
							AlphaDelete(); //because we have a unique transparent color in the image
#endif //CXIMAGE_SUPPORT_ALPHA
						}

						// <vho> - Transparency support w/o Alpha support
						if (c <= 8){ // only for icons with less than 256 colors (XP icons need alpha).
							  
							// find a color index, which is not used in the image
							// it is almost sure to find one, bcs. nobody uses all possible colors for an icon

							BYTE colorsUsed[256];
							memset(colorsUsed, 0, sizeof(colorsUsed));

							for (y = 0; y < head.biHeight; y++){
								for (x = 0; x < head.biWidth; x++){
									colorsUsed[BlindGetPixelIndex(x,y)] = 1;
								}
							}

							int iTransIdx = -1;
							for (x = (int)(head.biClrUsed-1); x>=0 ; x--){
								if (colorsUsed[x] == 0){
									iTransIdx = x; // this one is not in use. we may use it as transparent color
									break;
								}
							}

							// Go thru image and set unused color as transparent index if needed
							if (iTransIdx >= 0){
								bool bNeedTrans = false;
								for (y = 0; y < head.biHeight; y++){
									for (x = 0; x < head.biWidth; x++){
										// AND mask (Each Byte represents 8 Pixels)
										if (((mask[y*maskwdt+(x>>3)] >> (7-x%8)) & 0x01)){
											// AND mask is set (!=0). This is a transparent part
											SetPixelIndex(x, y, (BYTE)iTransIdx);
											bNeedTrans = true;
										}
									}
								}
								// set transparent index if needed
								if (bNeedTrans)	SetTransIndex(iTransIdx);
#if CXIMAGE_SUPPORT_ALPHA
								AlphaDelete(); //because we have a transparent color in the palette
#endif //CXIMAGE_SUPPORT_ALPHA
							}
						}
					} else {
						SetTransIndex(0); //empty mask, set black as transparent color
						Negative();
					}
				} 
				free(mask);
			}
			free(icon_list);
			// icon has been loaded successfully!
			return true;
		}
		free(icon_list);
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
// Thanks to <Alas>
bool CxImageICO::Encode(CxFile * hFile, CxImage ** pImages, int nPageCount)
{
  cx_try
  {
	if (hFile==NULL) cx_throw("invalid file pointer");
	if (pImages==NULL || nPageCount<=0) cx_throw("multipage ICO, no images!");

	int i;
	for (i=0; i<nPageCount; i++){
		if (pImages[i]==NULL)
			cx_throw("Bad image pointer");
		if (!(pImages[i]->IsValid()))
			cx_throw("Empty image");
	}

	CxImageICO ghost;
	for (i=0; i<nPageCount; i++){	//write headers
		ghost.Ghost(pImages[i]);
		ghost.info.nNumFrames = nPageCount;
		if (i==0) {
			if (!ghost.Encode(hFile,false,nPageCount))
				cx_throw("Error writing ICO file header");
		}
		if (!ghost.Encode(hFile,true,nPageCount)) 
			cx_throw("Error saving ICO image header");
	}
	for (i=0; i<nPageCount; i++){	//write bodies
		ghost.Ghost(pImages[i]);
		ghost.info.nNumFrames = nPageCount;
		if (!ghost.Encode(hFile,true,i)) 
			cx_throw("Error saving ICO body");
	}

  } cx_catch {
	  if (strcmp(message,"")) strncpy(info.szLastError,message,255);
	  return false;
  }
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImageICO::Encode(CxFile * hFile, bool bAppend, int nPageCount)
{
	if (EncodeSafeCheck(hFile)) return false;

#if CXIMAGE_SUPPORT_PNG == 0
	//check format limits
	if ((head.biWidth>255)||(head.biHeight>255)){
		strcpy(info.szLastError,"Can't save this image as icon");
		return false;
	}
#endif

	//prepare the palette struct
	RGBQUAD* pal=GetPalette();
	if (head.biBitCount<=8 && pal==NULL) return false;

	int maskwdt=((head.biWidth+31)/32)*4; //mask line width
	int masksize=head.biHeight * maskwdt; //size of mask
	int bitcount=head.biBitCount;
	int imagesize=head.biSizeImage;
#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid() && head.biClrUsed==0){
		bitcount=32;
		imagesize=4*head.biHeight*head.biWidth;
	}
#endif

	//fill the icon headers
	int nPages = nPageCount;
	if (nPages<1) nPages = 1;

	ICONHEADER icon_header={0,1,nPages};

	if (!bAppend)
		m_dwImageOffset = sizeof(ICONHEADER) + nPages * sizeof(ICONDIRENTRY);

	DWORD dwBytesInRes = sizeof(BITMAPINFOHEADER)+head.biClrUsed*sizeof(RGBQUAD)+imagesize+masksize;

	ICONDIRENTRY icon_list={
		(BYTE)head.biWidth,
		(BYTE)head.biHeight,
		(BYTE)head.biClrUsed,
		0, 0,
		(WORD)bitcount,
		dwBytesInRes,
		m_dwImageOffset
	};

	BITMAPINFOHEADER bi={
		sizeof(BITMAPINFOHEADER),
		head.biWidth,
		2*head.biHeight,
		1,
		(WORD)bitcount,
		0, imagesize,
		0, 0, 0, 0
	};

#if CXIMAGE_SUPPORT_PNG // Vista icon support
	CxImage png(*this);
	CxMemFile memfile;
	if (head.biWidth>255 || head.biHeight>255){
		icon_list.bWidth = icon_list.bHeight = 0;
		memfile.Open();
		png.Encode(&memfile,CXIMAGE_FORMAT_PNG);
		icon_list.dwBytesInRes = dwBytesInRes = memfile.Size();
	}
#endif //CXIMAGE_SUPPORT_PNG

	if (!bAppend){
		icon_header.idType = ntohs(icon_header.idType);
		icon_header.idCount = ntohs(icon_header.idCount);
		hFile->Write(&icon_header,sizeof(ICONHEADER),1);	//write the file header
		icon_header.idType = ntohs(icon_header.idType);
		icon_header.idCount = ntohs(icon_header.idCount);
	}


	if ((bAppend && nPageCount==info.nNumFrames) || (!bAppend && nPageCount==0)){
		icon_list.wPlanes = ntohs(icon_list.wPlanes);
		icon_list.wBitCount = ntohs(icon_list.wBitCount);
		icon_list.dwBytesInRes = ntohl(icon_list.dwBytesInRes);
		icon_list.dwImageOffset = ntohl(icon_list.dwImageOffset);
		hFile->Write(&icon_list,sizeof(ICONDIRENTRY),1);	//write the image entry
		icon_list.wPlanes = ntohs(icon_list.wPlanes);
		icon_list.wBitCount = ntohs(icon_list.wBitCount);
		icon_list.dwBytesInRes = ntohl(icon_list.dwBytesInRes);
		icon_list.dwImageOffset = ntohl(icon_list.dwImageOffset);

		m_dwImageOffset += dwBytesInRes;			//update offset for next header
	}

	if ((bAppend && nPageCount<info.nNumFrames) || (!bAppend && nPageCount==0))
	{
#if CXIMAGE_SUPPORT_PNG
		if (icon_list.bWidth==0 && icon_list.bHeight==0) {	// Vista icon support
			hFile->Write(memfile.GetBuffer(false),dwBytesInRes,1);
		} else
#endif //CXIMAGE_SUPPORT_PNG
		{	// standard icon
			bihtoh(&bi);
			hFile->Write(&bi,sizeof(BITMAPINFOHEADER),1);			//write the image header
			bihtoh(&bi);

			bool bTransparent = info.nBkgndIndex >= 0;
			RGBQUAD ct = GetTransColor();
			if (pal){
				if (bTransparent) SetPaletteColor((BYTE)info.nBkgndIndex,0,0,0,0);
			 	hFile->Write(pal,head.biClrUsed*sizeof(RGBQUAD),1); //write palette
				if (bTransparent) SetPaletteColor((BYTE)info.nBkgndIndex,ct);
			}

#if CXIMAGE_SUPPORT_ALPHA
			if (AlphaIsValid() && head.biClrUsed==0){
				BYTE* buf=(BYTE*)malloc(imagesize);
				BYTE* dst = buf;
				for (long y = 0; y < head.biHeight; y++) {
					BYTE* src = GetBits(y);
					for(long x=0;x<head.biWidth;x++){
						*dst++=*src++;
						*dst++=*src++;
						*dst++=*src++;
						*dst++=AlphaGet(x,y);
					}
				}
				hFile->Write(buf,imagesize, 1);
				free(buf);
			} else {
				hFile->Write(info.pImage,imagesize,1);	//write image
			}
#else
			hFile->Write(info.pImage,imagesize,1);	//write image
#endif

			//save transparency mask
			BYTE* mask=(BYTE*)calloc(masksize,1);	//create empty AND/XOR masks
			if (!mask) return false;

			//prepare the variables to build the mask
			BYTE* iDst;
			int pos,i;
			RGBQUAD c={0,0,0,0};
			long* pc = (long*)&c;
			long* pct= (long*)&ct;
#if CXIMAGE_SUPPORT_ALPHA
			bool bAlphaPaletteIsValid = AlphaPaletteIsValid();
			bool bAlphaIsValid = AlphaIsValid();
#endif
			//build the mask
			for (int y = 0; y < head.biHeight; y++) {
				for (int x = 0; x < head.biWidth; x++) {
					i=0;
#if CXIMAGE_SUPPORT_ALPHA
					if (bAlphaIsValid && AlphaGet(x,y)==0) i=1;
					if (bAlphaPaletteIsValid && BlindGetPixelColor(x,y).rgbReserved==0) i=1;
#endif
					c=GetPixelColor(x,y,false);
					if (bTransparent && *pc==*pct) i=1;
					iDst = mask + y*maskwdt + (x>>3);
					pos = 7-x%8;
					*iDst &= ~(0x01<<pos);
					*iDst |= ((i & 0x01)<<pos);
				}
			}
			//write AND/XOR masks
			hFile->Write(mask,masksize,1);
			free(mask);
		}
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_ICO

