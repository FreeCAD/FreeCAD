/*
 * File:	ximawbmp.cpp
 * Purpose:	Platform Independent WBMP Image Class Loader and Writer
 * 12/Jul/2002 Davide Pizzolato - www.xdp.it
 * CxImage version 6.0.0 02/Feb/2008
 */

#include "ximawbmp.h"

#if CXIMAGE_SUPPORT_WBMP

#include "ximaiter.h"

////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageWBMP::Decode(CxFile *hFile)
{
	if (hFile == NULL) return false;

	WBMPHEADER wbmpHead;

  cx_try
  {
	ReadOctet(hFile, &wbmpHead.Type);

	DWORD dat;
	ReadOctet(hFile, &dat);
	wbmpHead.FixHeader = (BYTE)dat;

	ReadOctet(hFile, &wbmpHead.ImageWidth);
	ReadOctet(hFile, &wbmpHead.ImageHeight);

	if (hFile->Eof())
		cx_throw("Not a WBMP");

	if (wbmpHead.Type != 0)
		cx_throw("Unsupported WBMP type");			

	head.biWidth = wbmpHead.ImageWidth;
	head.biHeight= wbmpHead.ImageHeight;

	if (head.biWidth<=0 || head.biHeight<=0)
		cx_throw("Corrupted WBMP");

	if (info.nEscape == -1){
		info.dwType = CXIMAGE_FORMAT_WBMP;
		return true;
	}

	Create(head.biWidth, head.biHeight, 1, CXIMAGE_FORMAT_WBMP);
	if (!IsValid()) cx_throw("WBMP Create failed");
	SetGrayPalette();

	int linewidth=(head.biWidth+7)/8;
    CImageIterator iter(this);
	iter.Upset();
    for (long y=0; y < head.biHeight; y++){
		hFile->Read(iter.GetRow(),linewidth,1);
		iter.PrevRow();
    }

  } cx_catch {
	if (strcmp(message,"")) strncpy(info.szLastError,message,255);
	return FALSE;
  }
    return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImageWBMP::ReadOctet(CxFile * hFile, DWORD *data)
{
	BYTE c;
	*data = 0;
	do {
		if (hFile->Eof()) return false;
		c = (BYTE)hFile->GetC();
		*data <<= 7;
		*data |= (c & 0x7F);
	} while ((c&0x80)!=0);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageWBMP::Encode(CxFile * hFile)
{
	if (EncodeSafeCheck(hFile)) return false;

	//check format limits
	if (head.biBitCount!=1){
		strcpy(info.szLastError,"Can't save this image as WBMP");
		return false;
	}

	WBMPHEADER wbmpHead;
	wbmpHead.Type=0;
	wbmpHead.FixHeader=0;
	wbmpHead.ImageWidth=head.biWidth;
	wbmpHead.ImageHeight=head.biHeight;

    // Write the file header
	hFile->PutC('\0');
	hFile->PutC('\0');
	WriteOctet(hFile,wbmpHead.ImageWidth);
	WriteOctet(hFile,wbmpHead.ImageHeight);
    // Write the pixels
	int linewidth=(wbmpHead.ImageWidth+7)/8;
    CImageIterator iter(this);
	iter.Upset();
    for (DWORD y=0; y < wbmpHead.ImageHeight; y++){
		hFile->Write(iter.GetRow(),linewidth,1);
		iter.PrevRow();
    }
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImageWBMP::WriteOctet(CxFile * hFile, const DWORD data)
{
	int ns = 0;
	while (data>>(ns+7)) ns+=7;
	while (ns>0){
		if (!hFile->PutC(0x80 | (BYTE)(data>>ns))) return false;
		ns-=7;
	}
	if (!(hFile->PutC((BYTE)(0x7F & data)))) return false;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_WBMP

