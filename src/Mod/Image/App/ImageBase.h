/***************************************************************************
 *                                                                         *
 *   This is a class for holding and handling basic image data             *
 *                                                                         *
 *   Author:    Graeme van der Vlugt                                       *
 *   Copyright: Imetric 3D GmbH                                            *
 *   Year:      2004                                                       *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 ***************************************************************************/

#ifndef IMAGEBASE_H
#define IMAGEBASE_H

namespace Image
{

#define IB_CF_GREY8     1       // 8-bit grey level images
#define IB_CF_GREY16    2		// 16-bit grey level images
#define IB_CF_GREY32    3		// 32-bit grey level images
#define IB_CF_RGB24     4		// 24-bit (8,8,8) RGB color images
#define IB_CF_RGB48     5		// 48-bit (16,16,16) RGB color images
#define IB_CF_BGR24     6		// 24-bit (8,8,8) BGR color images
#define IB_CF_BGR48     7		// 48-bit (16,16,16) BGR color images
#define IB_CF_RGBA32    8		// 32-bit (8,8,8,8) RGBA color images (A = alpha)
#define IB_CF_RGBA64    9		// 64-bit (16,16,16,16) RGBA color images (A = alpha)
#define IB_CF_BGRA32    10		// 32-bit (8,8,8,8) BGRA color images (A = alpha)
#define IB_CF_BGRA64    11		// 64-bit (16,16,16,16) BGRA color images (A = alpha)

class ImageExport ImageBase
{
public:

    ImageBase();
    virtual ~ImageBase();
    ImageBase(const ImageBase &rhs);
    ImageBase & operator=(const ImageBase &rhs);

    bool hasValidData() const { return (_pPixelData != 0); }
    void* getPixelDataPtr() { return (void *)_pPixelData; }
    bool isOwner() const { return _owner; }
    unsigned long getWidth() const { return _width; }
    unsigned long getHeight() const { return _height; }
    int getFormat() const { return _format; }
    unsigned short getNumSigBitsPerSample() const { return _numSigBitsPerSample; }
    unsigned short getNumSamples() const { return _numSamples; }
    unsigned short getNumBitsPerSample() const { return _numBitsPerSample; }
    unsigned short getNumBytesPerPixel() const { return _numBytesPerPixel; }
    
    virtual void clear();
    virtual int createCopy(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample);
    virtual int pointTo(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample, bool takeOwnership);

    virtual int getSample(int x, int y, unsigned short sampleIndex, double &value);

protected:

    int _setColorFormat(int format, unsigned short numSigBitsPerSample);
    int _allocate();

    unsigned char* _pPixelData;			// pointer to the pixel data
    bool _owner;						// flag defining if the object owns the pixel data or not
    unsigned long _width;				// width of image (number of pixels in horizontal direction)
    unsigned long _height;				// height of image (number of pixels in vertical direction)
    int _format;				        // colour format of the pixel data
    unsigned short _numSigBitsPerSample;// number of significant bits per sample (always <= _numBitsPerSample)

    // Dependent parameters
    unsigned short _numSamples;		    // number of samples per pixel (e.g. 1 for grey, 3 for rgb, 4 for rgba)
    unsigned short _numBitsPerSample;	// number of bits per sample (e.g. 8 for Grey8)
    unsigned short _numBytesPerPixel;	// number of bytes per pixel (e.g. 1 for Grey8)
};

} // namespace ImageApp

#endif // IMAGEBASE_H
