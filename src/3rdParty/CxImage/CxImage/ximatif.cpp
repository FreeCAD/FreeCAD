/*
 * File:	ximatif.cpp
 * Purpose:	Platform Independent TIFF Image Class Loader and Writer
 * 07/Aug/2001 Davide Pizzolato - www.xdp.it
 * CxImage version 6.0.0 02/Feb/2008
 */

#include "ximatif.h"

#if CXIMAGE_SUPPORT_TIF

#define FIX_16BPP_DARKIMG // + VK: if uncomment, dark 16bpp images are fixed

#include "../tiff/tiffio.h"

#define CVT(x)			(((x) * 255L) / ((1L<<16)-1))
#define	SCALE(x)		(((x)*((1L<<16)-1))/255)
#define CalculateLine(width,bitdepth)	(((width * bitdepth) + 7) / 8)
#define CalculatePitch(line)	(line + 3 & ~3)

extern "C" TIFF* _TIFFOpenEx(CxFile* stream, const char* mode);

////////////////////////////////////////////////////////////////////////////////
CxImageTIF::~CxImageTIF()
{
	if (m_tif2) TIFFClose(m_tif2);
}
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageTIF::Decode(CxFile * hFile)
{
	//Comment this line if you need more information on errors
	// TIFFSetErrorHandler(NULL);	//<Patrick Hoffmann>

	//Open file and fill the TIFF structure
	// m_tif = TIFFOpen(imageFileName,"rb");
	TIFF* m_tif = _TIFFOpenEx(hFile, "rb");

	uint32 height=0;
	uint32 width=0;
	uint16 bitspersample=1;
	uint16 samplesperpixel=1;
	uint32 rowsperstrip=(DWORD)-1;
	uint16 photometric=0;
	uint16 compression=1;
	uint16 orientation=ORIENTATION_TOPLEFT; //<vho>
	uint16 res_unit; //<Trifon>
	uint32 x, y;
	float resolution, offset;
	BOOL isRGB;
	BYTE *bits;		//pointer to source data
	BYTE *bits2;	//pointer to destination data

  cx_try
  {
	//check if it's a tiff file
	if (!m_tif)
		cx_throw("Error encountered while opening TIFF file");

	// <Robert Abram> - 12/2002 : get NumFrames directly, instead of looping
	// info.nNumFrames=0;
	// while(TIFFSetDirectory(m_tif,(uint16)info.nNumFrames)) info.nNumFrames++;
	info.nNumFrames = TIFFNumberOfDirectories(m_tif);

	if (!TIFFSetDirectory(m_tif, (uint16)info.nFrame))
		cx_throw("Error: page not present in TIFF file");			

	//get image info
	TIFFGetField(m_tif, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(m_tif, TIFFTAG_IMAGELENGTH, &height);
	TIFFGetField(m_tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
	TIFFGetField(m_tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	TIFFGetField(m_tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);   
	TIFFGetField(m_tif, TIFFTAG_PHOTOMETRIC, &photometric);
	TIFFGetField(m_tif, TIFFTAG_ORIENTATION, &orientation);

	if (info.nEscape == -1) {
		// Return output dimensions only
		head.biWidth = width;
		head.biHeight = height;
		info.dwType = CXIMAGE_FORMAT_TIF;
		cx_throw("output dimensions returned");
	}

	TIFFGetFieldDefaulted(m_tif, TIFFTAG_RESOLUTIONUNIT, &res_unit);
	if (TIFFGetField(m_tif, TIFFTAG_XRESOLUTION, &resolution))
	{
		if (res_unit == RESUNIT_CENTIMETER) resolution = (float)(resolution*2.54f + 0.5f);
		SetXDPI((long)resolution);
	}
	if (TIFFGetField(m_tif, TIFFTAG_YRESOLUTION, &resolution))
	{
		if (res_unit == RESUNIT_CENTIMETER) resolution = (float)(resolution*2.54f + 0.5f);
		SetYDPI((long)resolution);
	}

	if (TIFFGetField(m_tif, TIFFTAG_XPOSITION, &offset))	info.xOffset = (long)offset;
	if (TIFFGetField(m_tif, TIFFTAG_YPOSITION, &offset))	info.yOffset = (long)offset;

	head.biClrUsed=0;
	info.nBkgndIndex =-1;

	if (rowsperstrip>height){
		rowsperstrip=height;
		TIFFSetField(m_tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
	}

	isRGB = /*(bitspersample >= 8) && (VK: it is possible so for RGB to have < 8 bpp!)*/
		(photometric == PHOTOMETRIC_RGB) ||
		(photometric == PHOTOMETRIC_YCBCR) ||
		(photometric == PHOTOMETRIC_SEPARATED) ||
		(photometric == PHOTOMETRIC_LOGL) ||
		(photometric == PHOTOMETRIC_LOGLUV);

	if (isRGB){
		head.biBitCount=24;
	}else{
		if ((photometric==PHOTOMETRIC_MINISBLACK)||(photometric==PHOTOMETRIC_MINISWHITE)||(photometric==PHOTOMETRIC_PALETTE)){
			if	(bitspersample == 1){
				head.biBitCount=1;		//B&W image
				head.biClrUsed =2;
			} else if (bitspersample == 4) {
				head.biBitCount=4;		//16 colors gray scale
				head.biClrUsed =16;
			} else {
				head.biBitCount=8;		//gray scale
				head.biClrUsed =256;
			}
		} else if (bitspersample == 4) {
			head.biBitCount=4;			// 16 colors
			head.biClrUsed=16;
		} else {
			head.biBitCount=8;			//256 colors
			head.biClrUsed=256;
		}

		if ((bitspersample > 8) && (photometric==PHOTOMETRIC_PALETTE))	// + VK + (BIG palette! => convert to RGB)
		{	head.biBitCount=24;
			head.biClrUsed =0;
		}
	}

	if (info.nEscape) cx_throw("Cancelled"); // <vho> - cancel decoding

	Create(width,height,head.biBitCount,CXIMAGE_FORMAT_TIF);	//image creation
	if (!pDib) cx_throw("CxImageTIF can't create image");

#if CXIMAGE_SUPPORT_ALPHA
	if (samplesperpixel==4) AlphaCreate();	//add alpha support for 32bpp tiffs
	if (samplesperpixel==2 && bitspersample==8) AlphaCreate();	//add alpha support for 8bpp + alpha
#endif //CXIMAGE_SUPPORT_ALPHA

	TIFFGetField(m_tif, TIFFTAG_COMPRESSION, &compression);
	SetCodecOption(compression); // <DPR> save original compression type

	if (isRGB) {
		// Read the whole image into one big RGBA buffer using
		// the traditional TIFFReadRGBAImage() API that we trust.
		uint32* raster;		// retrieve RGBA image
		uint32 *row;

		raster = (uint32*)_TIFFmalloc(width * height * sizeof (uint32));
		if (raster == NULL) cx_throw("No space for raster buffer");
			
		// Read the image in one chunk into an RGBA array
		if(!TIFFReadRGBAImage(m_tif, width, height, raster, 1)) {
				_TIFFfree(raster);
				cx_throw("Corrupted TIFF file!");
		}

		// read the raster lines and save them in the DIB
		// with RGB mode, we have to change the order of the 3 samples RGB
		row = &raster[0];
		bits2 = info.pImage;
		for (y = 0; y < height; y++) {

			if (info.nEscape){ // <vho> - cancel decoding
				_TIFFfree(raster);
				cx_throw("Cancelled");
			}

			bits = bits2;
			for (x = 0; x < width; x++) {
				*bits++ = (BYTE)TIFFGetB(row[x]);
				*bits++ = (BYTE)TIFFGetG(row[x]);
				*bits++ = (BYTE)TIFFGetR(row[x]);
#if CXIMAGE_SUPPORT_ALPHA
				if (samplesperpixel==4) AlphaSet(x,y,(BYTE)TIFFGetA(row[x]));
#endif //CXIMAGE_SUPPORT_ALPHA
			}
			row += width;
			bits2 += info.dwEffWidth;
		}
		_TIFFfree(raster);
	} else {
		int BIG_palette = (bitspersample > 8) &&	// + VK
						  (photometric==PHOTOMETRIC_PALETTE);		
		if (BIG_palette && (bitspersample > 24))	// + VK
			cx_throw("Too big palette to handle");		// + VK

		RGBQUAD *pal;
		pal=(RGBQUAD*)calloc(BIG_palette ? 1<<bitspersample : 256,sizeof(RGBQUAD)); 
			// ! VK: it coasts nothing but more correct to use 256 as temp palette storage
			// ! VK: but for case of BIG palette it just copied
		if (pal==NULL) cx_throw("Unable to allocate TIFF palette");

		int bpp = bitspersample <= 8 ? bitspersample : 8; // + VK (to use instead of bitspersample for case of > 8)

		// set up the colormap based on photometric	
		switch(photometric) {
			case PHOTOMETRIC_MINISBLACK:	// bitmap and greyscale image types
			case PHOTOMETRIC_MINISWHITE:
				if (bitspersample == 1) {	// Monochrome image
					if (photometric == PHOTOMETRIC_MINISBLACK) {
						pal[1].rgbRed = pal[1].rgbGreen = pal[1].rgbBlue = 255;
					} else {
						pal[0].rgbRed = pal[0].rgbGreen = pal[0].rgbBlue = 255;
					}
				} else {		// need to build the scale for greyscale images
					if (photometric == PHOTOMETRIC_MINISBLACK) {
						for (int i=0; i<(1<<bpp); i++){
							pal[i].rgbRed = pal[i].rgbGreen = pal[i].rgbBlue = (BYTE)(i*(255/((1<<bpp)-1)));
						}
					} else {
						for (int i=0; i<(1<<bpp); i++){
							pal[i].rgbRed = pal[i].rgbGreen = pal[i].rgbBlue = (BYTE)(255-i*(255/((1<<bpp)-1)));
						}
					}
				}
				break;
			case PHOTOMETRIC_PALETTE:	// color map indexed
				uint16 *red;
				uint16 *green;
				uint16 *blue;
				TIFFGetField(m_tif, TIFFTAG_COLORMAP, &red, &green, &blue); 

				// Is the palette 16 or 8 bits ?
				BOOL Palette16Bits = /*FALSE*/ BIG_palette;
				if (!BIG_palette) {
					int n= 1<<bpp;
					while (n-- > 0) {
						if (red[n] >= 256 || green[n] >= 256 || blue[n] >= 256) {
							Palette16Bits=TRUE;
							break;
						}
					}
				}

				// load the palette in the DIB
				for (int i = (1 << ( BIG_palette ? bitspersample : bpp )) - 1; i >= 0; i--) {
					if (Palette16Bits) {
						pal[i].rgbRed =(BYTE) CVT(red[i]);
						pal[i].rgbGreen = (BYTE) CVT(green[i]);
						pal[i].rgbBlue = (BYTE) CVT(blue[i]);           
					} else {
						pal[i].rgbRed = (BYTE) red[i];
						pal[i].rgbGreen = (BYTE) green[i];
						pal[i].rgbBlue = (BYTE) blue[i];        
					}
				}
				break;
		}
		if (!BIG_palette) { // + VK (BIG palette is stored until image is ready)
			SetPalette(pal,/*head.biClrUsed*/ 1<<bpp);	//palette assign // * VK
			free(pal); 
			pal = NULL; 
		}

		// read the tiff lines and save them in the DIB
		uint32 nrow;
		uint32 ys;
		int line = CalculateLine(width, bitspersample * samplesperpixel);
		
		long bitsize = TIFFStripSize(m_tif);
		//verify bitsize: could be wrong if StripByteCounts is missing.
		if (bitsize<(long)(head.biSizeImage*samplesperpixel))
			bitsize = head.biSizeImage*samplesperpixel;

		if ((bitspersample > 8) && (bitspersample != 16))	// + VK (for bitspersample == 9..15,17..32..64
			bitsize *= (bitspersample + 7)/8; 

		int tiled_image = TIFFIsTiled(m_tif);
		uint32 tw=0, tl=0;
		BYTE* tilebuf=NULL;
		if (tiled_image){
			TIFFGetField(m_tif, TIFFTAG_TILEWIDTH, &tw);
			TIFFGetField(m_tif, TIFFTAG_TILELENGTH, &tl);
			rowsperstrip = tl;
			bitsize = TIFFTileSize(m_tif) * (int)(1+width/tw);
			tilebuf = (BYTE*)malloc(TIFFTileSize(m_tif));
		}
		
		bits = (BYTE*)malloc(bitspersample==16? bitsize*2 : bitsize); // * VK
		BYTE * bits16 = NULL;										  // + VK
		int line16    = 0;											  // + VK

		if (!tiled_image && bitspersample==16) {					  // + VK +
			line16 = line;
			line   = CalculateLine(width, 8 * samplesperpixel);
			bits16 = bits;
			bits   = (BYTE*)malloc(bitsize);
		}

		if (bits==NULL){
			if (bits16) free(bits16);								  // + VK
			if (pal)	free(pal);									  // + VK
			if (tilebuf)free(tilebuf);								  // + VK	
			cx_throw("CxImageTIF can't allocate memory");
		}

#ifdef FIX_16BPP_DARKIMG // + VK: for each line, store shift count bits used to fix it
		BYTE* row_shifts = NULL;
		if (bits16) row_shifts = (BYTE*)malloc(height); 
#endif

		for (ys = 0; ys < height; ys += rowsperstrip) {

			if (info.nEscape){ // <vho> - cancel decoding
				free(bits);
				cx_throw("Cancelled");
			}

			nrow = (ys + rowsperstrip > height ? height - ys : rowsperstrip);

			if (tiled_image){
				uint32 imagew = TIFFScanlineSize(m_tif);
				uint32 tilew  = TIFFTileRowSize(m_tif);
				int iskew = imagew - tilew;
				uint8* bufp = (uint8*) bits;

				uint32 colb = 0;
				for (uint32 col = 0; col < width; col += tw) {
					if (TIFFReadTile(m_tif, tilebuf, col, ys, 0, 0) < 0){
						free(tilebuf);
						free(bits);
						cx_throw("Corrupted tiled TIFF file!");
					}

					if (colb + tw > imagew) {
						uint32 owidth = imagew - colb;
						uint32 oskew = tilew - owidth;
						TileToStrip(bufp + colb, tilebuf, nrow, owidth, oskew + iskew, oskew );
					} else {
						TileToStrip(bufp + colb, tilebuf, nrow, tilew, iskew, 0);
					}
					colb += tilew;
				}

			} else {
				if (TIFFReadEncodedStrip(m_tif, TIFFComputeStrip(m_tif, ys, 0), 
					(bits16? bits16 : bits), nrow * (bits16 ? line16 : line)) == -1) { // * VK

#ifdef NOT_IGNORE_CORRUPTED
					free(bits);
					if (bits16) free(bits16);  // + VK
					cx_throw("Corrupted TIFF file!");
#else
					break;
#endif
				}
			}

			for (y = 0; y < nrow; y++) {
				long offset=(nrow-y-1)*line;
				if ((bitspersample==16) && !BIG_palette) {	// * VK
					long offset16 = (nrow-y-1)*line16;		// + VK
					if (bits16)	{							// + VK +
#ifdef FIX_16BPP_DARKIMG
						int the_shift;
						BYTE hi_byte, hi_max=0;
						DWORD xi;
						for (xi=0;xi<(uint32)line;xi++) {
							hi_byte = bits16[xi*2+offset16+1];
							if(hi_byte>hi_max)
								hi_max = hi_byte;
						}
						the_shift = (hi_max == 0) ? 8 : 0;
						if (!the_shift)
							while( ! (hi_max & 0x80) ) {
								the_shift++;
								hi_max <<= 1;
							}
						row_shifts[height-ys-nrow+y] = the_shift;
						the_shift = 8 - the_shift;
						for (xi=0;xi<(uint32)line;xi++) 
							bits[xi+offset]= ((bits16[xi*2+offset16+1]<<8) | bits16[xi*2+offset16]) >> the_shift;
#else
						for (DWORD xi=0;xi<(uint32)line;xi++) 
							bits[xi+offset]=bits16[xi*2+offset16+1];
#endif
					} else {
						for (DWORD xi=0;xi<width;xi++)
							bits[xi+offset]=bits[xi*2+offset+1];
							}
				}
				if (samplesperpixel==1) { 
					if (BIG_palette)
						if (bits16) {
							long offset16 = (nrow-y-1)*line16;		// + VK
							MoveBitsPal( info.pImage + info.dwEffWidth * (height-ys-nrow+y),
									 bits16 + offset16, width, bitspersample, pal );
						} else
							MoveBitsPal( info.pImage + info.dwEffWidth * (height-ys-nrow+y),
									 bits + offset, width, bitspersample, pal );
					else if ((bitspersample == head.biBitCount) || 
						(bitspersample == 16))	//simple 8bpp, 4bpp image or 16bpp
						memcpy(info.pImage+info.dwEffWidth*(height-ys-nrow+y),bits+offset,info.dwEffWidth);
					else
						MoveBits( info.pImage + info.dwEffWidth * (height-ys-nrow+y),
								  bits + offset, width, bitspersample );
				} else if (samplesperpixel==2) { //8bpp image with alpha layer
					int xi=0;
					int ii=0;
					int yi=height-ys-nrow+y;
#if CXIMAGE_SUPPORT_ALPHA
					if (!pAlpha) AlphaCreate();			// + VK
#endif //CXIMAGE_SUPPORT_ALPHA
					while (ii<line){
						SetPixelIndex(xi,yi,bits[ii+offset]);
#if CXIMAGE_SUPPORT_ALPHA
						AlphaSet(xi,yi,bits[ii+offset+1]);
#endif //CXIMAGE_SUPPORT_ALPHA
						ii+=2;
						xi++;
						if (xi>=(int)width){
							yi--;
							xi=0;
						}
					}
				} else { //photometric==PHOTOMETRIC_CIELAB
					if (head.biBitCount!=24){ //fix image
						Create(width,height,24,CXIMAGE_FORMAT_TIF);
#if CXIMAGE_SUPPORT_ALPHA
						if (samplesperpixel==4) AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA
					}

					int xi=0;
					uint32 ii=0;
					int yi=height-ys-nrow+y;
					RGBQUAD c;
					int l,a,b,bitsoffset;
					double p,cx,cy,cz,cr,cg,cb;
					while (ii</*line*/width){		// * VK
						bitsoffset = ii*samplesperpixel+offset;
						l=bits[bitsoffset];
						a=bits[bitsoffset+1];
						b=bits[bitsoffset+2];
						if (a>127) a-=256;
						if (b>127) b-=256;
						// lab to xyz
						p = (l/2.55 + 16) / 116.0;
						cx = pow( p + a * 0.002, 3);
						cy = pow( p, 3);
						cz = pow( p - b * 0.005, 3);
						// white point
						cx*=0.95047;
						//cy*=1.000;
						cz*=1.0883;
						// xyz to rgb
						cr =  3.240479 * cx - 1.537150 * cy - 0.498535 * cz;
						cg = -0.969256 * cx + 1.875992 * cy + 0.041556 * cz;
						cb =  0.055648 * cx - 0.204043 * cy + 1.057311 * cz;

						if ( cr > 0.00304 ) cr = 1.055 * pow(cr,0.41667) - 0.055;
							else            cr = 12.92 * cr;
						if ( cg > 0.00304 ) cg = 1.055 * pow(cg,0.41667) - 0.055;
							else            cg = 12.92 * cg;
						if ( cb > 0.00304 ) cb = 1.055 * pow(cb,0.41667) - 0.055;
							else            cb = 12.92 * cb;

						c.rgbRed  =(BYTE)max(0,min(255,(int)(cr*255)));
						c.rgbGreen=(BYTE)max(0,min(255,(int)(cg*255)));
						c.rgbBlue =(BYTE)max(0,min(255,(int)(cb*255)));

						SetPixelColor(xi,yi,c);
#if CXIMAGE_SUPPORT_ALPHA
						if (samplesperpixel==4) AlphaSet(xi,yi,bits[bitsoffset+3]);
#endif //CXIMAGE_SUPPORT_ALPHA
						ii++;
						xi++;
						if (xi>=(int)width){
							yi--;
							xi=0;
						}
					}
				}
			}
		}
		free(bits);
		if (bits16) free(bits16);

#ifdef FIX_16BPP_DARKIMG
		if (row_shifts && (samplesperpixel == 1) && (bitspersample==16) && !BIG_palette) {
			// 1. calculate maximum necessary shift
			int min_row_shift = 8;
			for( y=0; y<height; y++ ) {
				if (min_row_shift > row_shifts[y]) min_row_shift = row_shifts[y];
			}
			// 2. for rows having less shift value, correct such rows:
			for( y=0; y<height; y++ ) {
				if (min_row_shift < row_shifts[y]) {
					int need_shift = row_shifts[y] - min_row_shift;
					BYTE* data = info.pImage + info.dwEffWidth * y;
					for( x=0; x<width; x++, data++ )
						*data >>= need_shift;
				}
			}
		}
		if (row_shifts)	free( row_shifts );
#endif

		if (tiled_image) free(tilebuf);
		if (pal)		 free(pal);

		switch(orientation){
		case ORIENTATION_TOPRIGHT: /* row 0 top, col 0 rhs */
			Mirror();
			break;
		case ORIENTATION_BOTRIGHT: /* row 0 bottom, col 0 rhs */
			Flip();
			Mirror();
			break;
		case ORIENTATION_BOTLEFT: /* row 0 bottom, col 0 lhs */
			Flip();
			break;
		case ORIENTATION_LEFTTOP: /* row 0 lhs, col 0 top */
			RotateRight();
			Mirror();
			break;
		case ORIENTATION_RIGHTTOP: /* row 0 rhs, col 0 top */
			RotateLeft();
			break;
		case ORIENTATION_RIGHTBOT: /* row 0 rhs, col 0 bottom */
			RotateLeft();
			Mirror();
			break;
		case ORIENTATION_LEFTBOT: /* row 0 lhs, col 0 bottom */
			RotateRight();
			break;
		}

	}
  } cx_catch {
	  if (strcmp(message,"")) strncpy(info.szLastError,message,255);
	  if (m_tif) TIFFClose(m_tif);
	  if (info.nEscape == -1 && info.dwType == CXIMAGE_FORMAT_TIF) return true;
	  return false;
  }
	TIFFClose(m_tif);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageTIF::Encode(CxFile * hFile, bool bAppend)
{
  cx_try
  {
	if (hFile==NULL) cx_throw(CXIMAGE_ERR_NOFILE);
	if (pDib==NULL) cx_throw(CXIMAGE_ERR_NOIMAGE);

	// <RJ> replaced "w+b" with "a", to append an image directly on an existing file
	if (m_tif2==NULL) m_tif2=_TIFFOpenEx(hFile, "a");
	if (m_tif2==NULL) cx_throw("initialization fail");

	if (bAppend || m_pages) m_multipage=true;
	m_pages++;

	if (!EncodeBody(m_tif2,m_multipage,m_pages,m_pages)) cx_throw("Error saving TIFF file");
	if (bAppend) {
		if (!TIFFWriteDirectory(m_tif2)) cx_throw("Error saving TIFF directory");
	}
  } cx_catch {
	  if (strcmp(message,"")) strncpy(info.szLastError,message,255);
	  if (m_tif2){
		  TIFFClose(m_tif2);
		  m_tif2=NULL;
		  m_multipage=false;
		  m_pages=0;
	  }
	  return false;
  }
	if (!bAppend){
		TIFFClose(m_tif2);
		m_tif2=NULL;
		m_multipage=false;
		m_pages=0;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
// Thanks to Abe <God(dot)bless(at)marihuana(dot)com>
bool CxImageTIF::Encode(CxFile * hFile, CxImage ** pImages, int pagecount)
{
  cx_try
  {
	if (hFile==NULL) cx_throw("invalid file pointer");
	if (pImages==NULL || pagecount<=0) cx_throw("multipage TIFF, no images!");

	int i;
	for (i=0; i<pagecount; i++){
		if (pImages[i]==NULL)
			cx_throw("Bad image pointer");
		if (!(pImages[i]->IsValid()))
			cx_throw("Empty image");
	}

	CxImageTIF ghost;
	for (i=0; i<pagecount; i++){
		ghost.Ghost(pImages[i]);
		if (!ghost.Encode(hFile,true)) cx_throw("Error saving TIFF file");
	}
  } cx_catch {
	  if (strcmp(message,"")) strncpy(info.szLastError,message,255);
	  return false;
  }
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImageTIF::EncodeBody(TIFF *m_tif, bool multipage, int page, int pagecount)
{
	uint32 height=head.biHeight;
	uint32 width=head.biWidth;
	uint16 bitcount=head.biBitCount;
	uint16 bitspersample;
	uint16 samplesperpixel;
	uint16 photometric=0;
	uint16 compression;
//	uint16 pitch;
//	int line;
	uint32 x, y;

	samplesperpixel = ((bitcount == 24) || (bitcount == 32)) ? (BYTE)3 : (BYTE)1;
#if CXIMAGE_SUPPORT_ALPHA
	if (bitcount==24 && AlphaIsValid()) { bitcount=32; samplesperpixel=4; }
#endif //CXIMAGE_SUPPORT_ALPHA

	bitspersample = bitcount / samplesperpixel;

	//set the PHOTOMETRIC tag
	RGBQUAD *rgb = GetPalette();
	switch (bitcount) {
		case 1:
			if (CompareColors(&rgb[0],&rgb[1])<0) {
				/* <abe> some viewers do not handle PHOTOMETRIC_MINISBLACK:
				 * let's transform the image in PHOTOMETRIC_MINISWHITE
				 */
				//invert the colors
				RGBQUAD tempRGB=GetPaletteColor(0);
				SetPaletteColor(0,GetPaletteColor(1));
				SetPaletteColor(1,tempRGB);
				//invert the pixels
				BYTE *iSrc=info.pImage;
				for (unsigned long i=0;i<head.biSizeImage;i++){
					*iSrc=(BYTE)~(*(iSrc));
					iSrc++;
				}
				photometric = PHOTOMETRIC_MINISWHITE;
				//photometric = PHOTOMETRIC_MINISBLACK;
			} else {
				photometric = PHOTOMETRIC_MINISWHITE;
			}
			break;
		case 4:	// Check if the DIB has a color or a greyscale palette
		case 8:
			photometric = PHOTOMETRIC_MINISBLACK; //default to gray scale
			for (x = 0; x < head.biClrUsed; x++) {
				if ((rgb->rgbRed != x)||(rgb->rgbRed != rgb->rgbGreen)||(rgb->rgbRed != rgb->rgbBlue)){
					photometric = PHOTOMETRIC_PALETTE;
					break;
				}
				rgb++;
			}
			break;
		case 24:
		case 32:
			photometric = PHOTOMETRIC_RGB;			
			break;
	}

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid() && bitcount==8) samplesperpixel=2; //8bpp + alpha layer
#endif //CXIMAGE_SUPPORT_ALPHA

//	line = CalculateLine(width, bitspersample * samplesperpixel);
//	pitch = (uint16)CalculatePitch(line);

	//prepare the palette struct
	RGBQUAD pal[256];
	if (GetPalette()){
		BYTE b;
		memcpy(pal,GetPalette(),GetPaletteSize());
		for(WORD a=0;a<head.biClrUsed;a++){	//swap blue and red components
			b=pal[a].rgbBlue; pal[a].rgbBlue=pal[a].rgbRed; pal[a].rgbRed=b;
		}
	}

	// handle standard width/height/bpp stuff
	TIFFSetField(m_tif, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(m_tif, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(m_tif, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
	TIFFSetField(m_tif, TIFFTAG_BITSPERSAMPLE, bitspersample);
	TIFFSetField(m_tif, TIFFTAG_PHOTOMETRIC, photometric);
	TIFFSetField(m_tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);	// single image plane 
	TIFFSetField(m_tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

	uint32 rowsperstrip = TIFFDefaultStripSize(m_tif, (uint32) -1);  //<REC> gives better compression
	TIFFSetField(m_tif, TIFFTAG_ROWSPERSTRIP, rowsperstrip);

	// handle metrics
	TIFFSetField(m_tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	TIFFSetField(m_tif, TIFFTAG_XRESOLUTION, (float)info.xDPI);
	TIFFSetField(m_tif, TIFFTAG_YRESOLUTION, (float)info.yDPI);
//	TIFFSetField(m_tif, TIFFTAG_XPOSITION, (float)info.xOffset);
//	TIFFSetField(m_tif, TIFFTAG_YPOSITION, (float)info.yOffset);

	// multi-paging - Thanks to Abe <God(dot)bless(at)marihuana(dot)com>
	if (multipage)
	{
		char page_number[20];
		sprintf(page_number, "Page %d", page);

		TIFFSetField(m_tif, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
		TIFFSetField(m_tif, TIFFTAG_PAGENUMBER, page,pagecount);
		TIFFSetField(m_tif, TIFFTAG_PAGENAME, page_number);
	} else {
		TIFFSetField(m_tif, TIFFTAG_SUBFILETYPE, 0);
	}

	// palettes (image colormaps are automatically scaled to 16-bits)
	if (photometric == PHOTOMETRIC_PALETTE) {
		uint16 *r, *g, *b;
		r = (uint16 *) _TIFFmalloc(sizeof(uint16) * 3 * 256);
		g = r + 256;
		b = g + 256;

		for (int i = 255; i >= 0; i--) {
			b[i] = (uint16)SCALE((uint16)pal[i].rgbRed);
			g[i] = (uint16)SCALE((uint16)pal[i].rgbGreen);
			r[i] = (uint16)SCALE((uint16)pal[i].rgbBlue);
		}

		TIFFSetField(m_tif, TIFFTAG_COLORMAP, r, g, b);
		_TIFFfree(r);
	}

	// compression
	if (GetCodecOption(CXIMAGE_FORMAT_TIF)) {
		compression = (WORD)GetCodecOption(CXIMAGE_FORMAT_TIF);
	} else {
		switch (bitcount) {
			case 1 :
				compression = COMPRESSION_CCITTFAX4;
				break;
			case 4 :
			case 8 :
				compression = COMPRESSION_LZW;
				break;
			case 24 :
			case 32 :
				compression = COMPRESSION_JPEG;
				break;
			default :
				compression = COMPRESSION_NONE;
				break;
		}
	}
	TIFFSetField(m_tif, TIFFTAG_COMPRESSION, compression);

	switch (compression) {
	case COMPRESSION_JPEG:
		TIFFSetField(m_tif, TIFFTAG_JPEGQUALITY, GetJpegQuality());
		TIFFSetField(m_tif, TIFFTAG_ROWSPERSTRIP, ((7+rowsperstrip)>>3)<<3);
   		break;
	case COMPRESSION_LZW:
		if (bitcount>=8) TIFFSetField(m_tif, TIFFTAG_PREDICTOR, 2);
		break;
	}

	// read the DIB lines from bottom to top and save them in the TIF

	BYTE *bits;
	switch(bitcount) {				
		case 1 :
		case 4 :
		case 8 :
		{
			if (samplesperpixel==1){
				bits = (BYTE*)malloc(info.dwEffWidth);
				if (!bits) return false;
				for (y = 0; y < height; y++) {
					memcpy(bits,info.pImage + (height - y - 1)*info.dwEffWidth,info.dwEffWidth);
					if (TIFFWriteScanline(m_tif,bits, y, 0)==-1){
						free(bits);
						return false;
					}
				}
				free(bits);
			}
#if CXIMAGE_SUPPORT_ALPHA
			else { //8bpp + alpha layer
				bits = (BYTE*)malloc(2*width);
				if (!bits) return false;
				for (y = 0; y < height; y++) {
					for (x=0;x<width;x++){
						bits[2*x]=BlindGetPixelIndex(x,height - y - 1);
						bits[2*x+1]=AlphaGet(x,height - y - 1);
					}
					if (TIFFWriteScanline(m_tif,bits, y, 0)==-1) {
						free(bits);
						return false;
					}
				}
				free(bits);
			}
#endif //CXIMAGE_SUPPORT_ALPHA
			break;
		}				
		case 24:
		{
			BYTE *buffer = (BYTE *)malloc(info.dwEffWidth);
			if (!buffer) return false;
			for (y = 0; y < height; y++) {
				// get a pointer to the scanline
				memcpy(buffer, info.pImage + (height - y - 1)*info.dwEffWidth, info.dwEffWidth);
				// TIFFs store color data RGB instead of BGR
				BYTE *pBuf = buffer;
				for (x = 0; x < width; x++) {
					BYTE tmp = pBuf[0];
					pBuf[0] = pBuf[2];
					pBuf[2] = tmp;
					pBuf += 3;
				}
				// write the scanline to disc
				if (TIFFWriteScanline(m_tif, buffer, y, 0)==-1){
					free(buffer);
					return false;
				}
			}
			free(buffer);
			break;
		}				
		case 32 :
		{
#if CXIMAGE_SUPPORT_ALPHA
			BYTE *buffer = (BYTE *)malloc((info.dwEffWidth*4)/3);
			if (!buffer) return false;
			for (y = 0; y < height; y++) {
				// get a pointer to the scanline
				memcpy(buffer, info.pImage + (height - y - 1)*info.dwEffWidth, info.dwEffWidth);
				// TIFFs store color data RGB instead of BGR
				BYTE *pSrc = buffer + 3 * width;
				BYTE *pDst = buffer + 4 * width;
				for (x = 0; x < width; x++) {
					pDst-=4;
					pSrc-=3;
					pDst[3] = AlphaGet(width-x-1,height-y-1);
					pDst[2] = pSrc[0];
					pDst[1] = pSrc[1];
					pDst[0] = pSrc[2];
				}
				// write the scanline to disc
				if (TIFFWriteScanline(m_tif, buffer, y, 0)==-1){
					free(buffer);
					return false;
				}
			}
			free(buffer);
#endif //CXIMAGE_SUPPORT_ALPHA
			break;
		}				
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
void CxImageTIF::TileToStrip(uint8* out, uint8* in,	uint32 rows, uint32 cols, int outskew, int inskew)
{
	while (rows-- > 0) {
		uint32 j = cols;
		while (j-- > 0)
			*out++ = *in++;
		out += outskew;
		in += inskew;
	}
}
////////////////////////////////////////////////////////////////////////////////
TIFF* CxImageTIF::TIFFOpenEx(CxFile * hFile)
{
	if (hFile) return _TIFFOpenEx(hFile, "rb");
	return NULL;
}
////////////////////////////////////////////////////////////////////////////////
void CxImageTIF::TIFFCloseEx(TIFF* tif)
{
	if (tif) TIFFClose(tif);
}
////////////////////////////////////////////////////////////////////////////////
void CxImageTIF::MoveBits( BYTE* dest, BYTE* from, int count, int bpp )
{	int offbits = 0;
	uint16 w;
	uint32 d;
	if (bpp <= 8) {
		while (count-- > 0) {
			if (offbits + bpp <= 8)
				w = *from >> (8 - offbits - bpp);
			else {
		        w = *from++ << (offbits + bpp - 8);
				w |= *from >> (16 - offbits - bpp);
			}
			offbits += bpp;
			if (offbits >= 8) {
				offbits -= 8;
		        if (offbits == 0) from++;
			}	
			*dest++ = (BYTE)w & ((1 << bpp)-1);
		}
	} else if (bpp < 16) {
		while (count-- > 0) {
			d = (*from << 24) | (from[1]<<16) | (from[2]<<8) | from[3];
			d >>= (24 - offbits);
			*dest++ = (BYTE) ( d );
			offbits += bpp;
			while (offbits >= 8) {
				from++;
				offbits -= 8;
			}
		}
	} else if (bpp < 32) {
		while (count-- > 0) {
			d = (*from << 24) | (from[1]<<16) | (from[2]<<8) | from[3];
			//d = *(uint32*)from;
			*dest++ = (BYTE) ( d >> (offbits + bpp - 8) );
			offbits += bpp;
			while (offbits >= 8) {
				from++;
				offbits -= 8;
			}
		}
	} else {
		while (count-- > 0) {
			d = *(uint32*)from;
			*dest++ = (BYTE) (d >> 24);
			from += 4;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
void CxImageTIF::MoveBitsPal( BYTE* dest, BYTE*from, int count, int bpp, RGBQUAD* pal )
{	int offbits = 0;
	uint32 d;
	uint16 palidx;
	while (count-- > 0) {
		d = (*from << 24) | ( *( from + 1 ) << 16 )
						  | ( *( from + 2 ) << 8 )
						  | ( *( from + 3 ) );
		palidx = (uint16) (d >> (32 - offbits - bpp));
		if (bpp < 16) {
			palidx <<= 16-bpp;
			palidx = (palidx >> 8) | (palidx <<8);
			palidx >>= 16-bpp;
		} else palidx = (palidx >> 8) | (palidx << 8);
		*dest++ = pal[palidx].rgbBlue;
		*dest++ = pal[palidx].rgbGreen;
		*dest++ = pal[palidx].rgbRed;
		offbits += bpp;
		while (offbits >= 8) {
			from++;
			offbits -= 8;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////

#endif // CXIMAGE_SUPPORT_TIF
