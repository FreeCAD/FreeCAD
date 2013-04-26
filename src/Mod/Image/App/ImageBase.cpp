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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <cmath>
# include <cstring>
#endif

#include "ImageBase.h"
#include <Base/Exception.h>

using namespace Image;

// Constructor (constructs an empty image)
ImageBase::ImageBase()
{
    _pPixelData = NULL;
    _owner = true;
    _width = 0;
    _height = 0;
    _setColorFormat(IB_CF_GREY8, 8);
}

// Destructor
ImageBase::~ImageBase()
{
    try
    {
        clear();
    }
    catch(...) {}
}

// Copy constructor
ImageBase::ImageBase(const ImageBase &rhs)
{
	// Do the copy
    if (rhs._owner == true)
    {
        // rhs is the owner - do a deep copy
        _pPixelData = NULL;
        _owner = false; // avoids a superfluous delete
        if (createCopy((void *)(rhs._pPixelData), rhs._width, rhs._height, rhs._format, rhs._numSigBitsPerSample) != 0)
            throw Base::Exception("ImageBase::ImageBase. Error creating copy of image");
    }
    else
    {
        // rhs is not the owner - do a shallow copy
        _pPixelData = rhs._pPixelData;
        _owner = rhs._owner;
        _width = rhs._width;
        _height = rhs._height;
        _setColorFormat(rhs._format, rhs._numSigBitsPerSample);
    }
}

// = operator
ImageBase & ImageBase::operator=(const ImageBase &rhs)
{
	if (this == &rhs)
		return *this;

	// Implement any deletion necessary
	clear();

	// Do the copy
    if (rhs._owner == true)
    {
        // rhs is the owner - do a deep copy
        _owner = false; // avoids a superfluous delete
        if (createCopy((void *)(rhs._pPixelData), rhs._width, rhs._height, rhs._format, rhs._numSigBitsPerSample) != 0)
            throw Base::Exception("ImageBase::operator=. Error creating copy of image");
    }
    else
    {
        // rhs is not the owner - do a shallow copy
        _pPixelData = rhs._pPixelData;
        _owner = rhs._owner;
        _width = rhs._width;
        _height = rhs._height;
        _setColorFormat(rhs._format, rhs._numSigBitsPerSample);
    }

	return *this;
}


// Clears the image data
// It only deletes the pixel data if this object is the owner of the data
void ImageBase::clear()
{
    // If object is the owner of the data then delete the allocated memory
    if (_owner == true)
    {
        delete [] _pPixelData;
        _pPixelData = NULL;
    }
    // Else just reset the pointer (the owner of the pixel data must be responsible for deleting it)
    else
    {
        _pPixelData = NULL;
    }

    // Re-initialise the other variables
    _owner = true;
    _width = 0;
    _height = 0;
    _setColorFormat(IB_CF_GREY8, 8);
}

// Sets the color format and the dependent parameters
// Returns 0 for OK, -1 for invalid color format
int ImageBase::_setColorFormat(int format, unsigned short numSigBitsPerSample)
{
    switch (format)
    {
        case IB_CF_GREY8:
            _numSamples = 1;
            _numBitsPerSample = 8;
            _numBytesPerPixel = 1;
            break;
        case IB_CF_GREY16:
            _numSamples = 1;
            _numBitsPerSample = 16;
            _numBytesPerPixel = 2;
            break;
        case IB_CF_GREY32:
            _numSamples = 1;
            _numBitsPerSample = 32;
            _numBytesPerPixel = 4;
            break;
        case IB_CF_RGB24:
            _numSamples = 3;
            _numBitsPerSample = 8;
            _numBytesPerPixel = 3;
            break;
        case IB_CF_RGB48:
            _numSamples = 3;
            _numBitsPerSample = 16;
            _numBytesPerPixel = 6;
            break;
        case IB_CF_BGR24:
            _numSamples = 3;
            _numBitsPerSample = 8;
            _numBytesPerPixel = 3;
            break;
        case IB_CF_BGR48:
            _numSamples = 3;
            _numBitsPerSample = 16;
            _numBytesPerPixel = 6;
            break;
        case IB_CF_RGBA32:
            _numSamples = 4;
            _numBitsPerSample = 8;
            _numBytesPerPixel = 4;
            break;
        case IB_CF_RGBA64:
            _numSamples = 4;
            _numBitsPerSample = 16;
            _numBytesPerPixel = 8;
            break;
        case IB_CF_BGRA32:
            _numSamples = 4;
            _numBitsPerSample = 8;
            _numBytesPerPixel = 4;
            break;
        case IB_CF_BGRA64:
            _numSamples = 4;
            _numBitsPerSample = 16;
            _numBytesPerPixel = 8;
            break;
        default:
            return -1;
    }

    if ((numSigBitsPerSample == 0) || (numSigBitsPerSample > _numBitsPerSample))
        _numSigBitsPerSample = _numBitsPerSample;
    else
        _numSigBitsPerSample = numSigBitsPerSample;

    _format = format;
    return 0;
}

// Allocate own space for an image based on the current color space and image size parameters
// Returns:
//		 0 for OK
//		-1 for error
int ImageBase::_allocate()
{
    // Check that pixel data pointer is null
    if (_pPixelData != NULL)
        return -1;

    // Allocate the space needed to store the pixel data
    _owner = true;
    try
    {
        _pPixelData = new unsigned char [_width * _height * _numBytesPerPixel];
    }
    catch(...)
    {
        // memory allocation error
        return -1;
    }

    return 0;
}

// Load an image by copying the pixel data
// This object will take ownership of the copied pixel data
// (the source image is still controlled by the caller)
// If numSigBitsPerSample = 0 then the full range is assumed to be significant
// Returns:
//		 0 for OK
//		-1 for invalid color format
//		-2 for memory allocation error
int ImageBase::createCopy(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample)
{
    // Clear any existing data
    clear();

    // Set the color format and the dependent parameters
    if (_setColorFormat(format, numSigBitsPerSample) != 0)
        return -1;

    // Set the image size
    _width = width;
    _height = height;

    // Allocate our own memory for the pixel data
    if (_allocate() != 0)
    {
        clear();
        return -2;
    }

    // Copy the pixel data
    memcpy((void *)_pPixelData, pSrcPixelData, _width * _height * _numBytesPerPixel);

    return 0;
}

// Make this object point to another image source
// If takeOwnership is false then:
//      This object will not own (control) or copy the pixel data
//      (the source image is still controlled by the caller)
// Else if takeOwnership is true then:
//      This object will take ownership (control) of the pixel data
//      (the source image is not (should not be) controlled by the caller anymore)
//      In this case the memory must have been allocated with the new operator (because this class will use the delete operator)
// If numSigBitsPerSample = 0 then the full range is assumed to be significant
// Returns:
//		 0 for OK
//		-1 for invalid color format
int ImageBase::pointTo(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample, bool takeOwnership)
{
    // Clear any existing data
    clear();

    // Set the color format and the dependent parameters
    if (_setColorFormat(format, numSigBitsPerSample) != 0)
        return -1;

    // Set the image size
    _width = width;
    _height = height;

    // Point to the source pixel data
    _owner = false;
    _pPixelData = (unsigned char *)pSrcPixelData;

    // Flag ownership
    if (takeOwnership == true)
        _owner = true;
    else
        _owner = false;

    return 0;
}

// Gets the value of a sample at the given pixel position
// Returns 0 for valid value or -1 if coordinates or sample index are out of range or 
// if there is no image data
int ImageBase::getSample(int x, int y, unsigned short sampleIndex, double &value)
{
    if ((_pPixelData == NULL) || 
        (sampleIndex >= _numSamples) ||
        (x < 0) || (x >= (int)_width) || 
        (y < 0) || (y >= (int)_height))
        return -1;

    // Get pointer to sample
    switch (_format)
    {
        case IB_CF_GREY8:
        case IB_CF_RGB24:
        case IB_CF_BGR24:
        case IB_CF_RGBA32:
        case IB_CF_BGRA32:
            {
                unsigned char* pSample = _pPixelData + _numSamples * (y * _width + x) + sampleIndex;
                value = (double)(*pSample);
            }
            break;
        case IB_CF_GREY16:
        case IB_CF_RGB48:
        case IB_CF_BGR48:
        case IB_CF_RGBA64:
        case IB_CF_BGRA64:
            {
                uint16_t* pPix16 = (uint16_t *)_pPixelData;
                uint16_t* pSample = pPix16 + _numSamples * (y * _width + x) + sampleIndex;
                value = (double)(*pSample);
            }
            break;
        case IB_CF_GREY32:
            {
                uint32_t* pPix32 = (uint32_t *)_pPixelData;
                uint32_t* pSample = pPix32 + y * _width + x;
                value = (double)(*pSample);
            }
            break;
        default:
            return -1;
    }
    return 0;
}



