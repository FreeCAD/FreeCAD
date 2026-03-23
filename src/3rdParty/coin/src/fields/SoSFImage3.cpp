/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*!
  \class SoSFImage3 SoSFImage3.h Inventor/fields/SoSFImage3.h
  \brief The SoSFImage3 class is used to store 3D (volume) images.

  \ingroup coin_fields

  The SoSFImage3 class provides storage for inline 3D image maps. 3D
  images in Coin are mainly used for 3D texture mapping support.

  SoSFImage3 instances can be exported and imported as any other field
  class in Coin.

  The components of an SoSFImage3 is: its image dimensions (width,
  height and depth), the number of bytes used for describing each
  pixel (number of components) and an associated pixel buffer. The
  size of the pixel buffer will be width*height*depth*components.

  For texture maps, the components / bytes-per-pixel setting
  translates as follows: use 1 for a grayscale imagemap, 2 for
  grayscale + opacity (i.e. alpha value), 3 for RGB (1 byte each for
  red, green and blue) and 4 components means 3 bytes for RGB + 1 byte
  opacity value (aka RGBA).

  This field is serializable into the Inventor / Coin file format in
  the following manner:

  \code
  FIELDNAME X Y Z C 0xRRGGBBAA 0xRRGGBBAA ...
  \endcode

  "X", "Y" and "Z" are the image dimensions along the given axes, "C"
  is the number of components in the image. The number of 0xRRGGBBAA
  pixel color specifications needs to equal the exact number of
  pixels, which is X*Y*Z. Each part of the pixel color value is in the
  range 0x00 to 0xff (hexadecimal, 0 to 255 decimal).

  For 3-component images, the pixel-format is 0xXXRRGGBB, where the
  byte in the pixel color value marked as "XX" is ignored and can be
  left out.

  For 2-component images, the pixel-format is 0xXXXXGGAA, where the
  bytes in the pixel color values marked as "XX" are ignored and can
  be left out. "GG" is the part which gives a grayscale value and "AA"
  is for opacity.

  For 1-component images, the pixel-format is 0xXXXXXXGG, where the
  bytes in the pixel color values marked as "XX" are ignored and can
  be left out.

  The pixels are read as being ordered in rows along X (width),
  columns along Y (height, bottom to top) and Z "planes" (depth, front
  to back).

  Here's a simple example of the file format serialization, for a
  2x2x2 RGB-image inside an SoTexture3 node:

  \code
  Texture3 {
    images 2 2 2 3

    0x000000 0x00ff00
    0xff0000 0xffff00

    0x000000 0x0000ff
    0x00ff00 0x00ffff
  }
  \endcode

  The image above is colored black+green on the first line and
  red+yellow on the second line in the first Z plane.  The second Z
  plane is colored black+blue on the first line and green+cyan on the
  second line.

  \COIN_CLASS_EXTENSION

  \sa SoTexture3, SoSFImage
  \since Coin 2.0
  \since TGS Inventor 2.6
*/

// *************************************************************************

#include <Inventor/fields/SoSFImage3.h>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/SbImage.h>

#include "fields/SoSubFieldP.h"

// *************************************************************************

PRIVATE_TYPEID_SOURCE(SoSFImage3);
PRIVATE_EQUALITY_SOURCE(SoSFImage3);

// *************************************************************************

// (Declarations hidden in macro in SoSFImage3.h, so don't use Doxygen
// commenting.)
#ifndef DOXYGEN_SKIP_THIS

/* Constructor, initializes fields to represent an empty image. */
SoSFImage3::SoSFImage3(void)
  : image(new SbImage)
{
}

/* Free all resources associated with the image. */
SoSFImage3::~SoSFImage3()
{
  delete this->image;
}

/* Copy the image of \a field into this field. */
const SoSFImage3 &
SoSFImage3::operator=(const SoSFImage3 & field)
{
  int nc = 0;
  SbVec3s size(0,0,0);
  unsigned char * bytes = field.image->getValue(size, nc);

  this->setValue(size, nc, bytes);
  return *this;
}

#endif // DOXYGEN_SKIP_THIS


/*!
  \copydetails SoField::initClass(void)
*/
void
SoSFImage3::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFImage3);
}

SbBool
SoSFImage3::readValue(SoInput * in)
{
  SbVec3s size;
  int nc;
  if (!in->read(size[0]) || !in->read(size[1]) || !in->read(size[2]) ||
      !in->read(nc)) {
    SoReadError::post(in, "Premature end of file reading images dimensions");
    return FALSE;
  }

  // Note: empty images (dimensions 0x0x0) are allowed.

  if (size[0] < 0 || size[1] < 0 || size[2] < 0 || nc < 0 || nc > 4) {
    SoReadError::post(in, "Invalid image specification %dx%dx%dx%d",
                      size[0], size[1], size[2], nc);
    return FALSE;
  }

  int buffersize = int(size[0]) * int(size[1]) * int(size[2]) * nc;

  if (buffersize == 0 &&
      (size[0] != 0 || size[1] != 0 || size[2] != 0 || nc != 0)) {
    SoReadError::post(in, "Invalid image specification %dx%dx%dx%d",
                      size[0], size[1], size[2], nc);
    return FALSE;
  }

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoSFImage3::readValue",
                         "image dimensions: %dx%dx%dx%d",
                         size[0], size[1], size[2], nc);
#endif // debug

  if (!buffersize) {
    this->image->setValue(SbVec3s(0,0,0), 0, NULL);
    return TRUE;
  }

  // allocate image data and get new pointer back
  this->image->setValue(size, nc, NULL);
  unsigned char * pixblock = this->image->getValue(size, nc);

  // The binary image format of 2.1 and later tries to be less
  // wasteful when storing images.
  if (in->isBinary() && in->getIVVersion() >= 2.1f) {
    if (!in->readBinaryArray(pixblock, buffersize)) {
      SoReadError::post(in, "Premature end of file reading images data");
      return FALSE;
    }
  }
  else {
    int byte = 0;
    int numpixels = int(size[0]) * int(size[1]) * int(size[2]);
    for (int i = 0; i < numpixels; i++) {
      unsigned int l;
      if (!in->read(l)) {
        SoReadError::post(in, "Premature end of file reading images data");
        return FALSE;
      }
      for (int j = 0; j < nc; j++) {
        pixblock[byte++] =
          static_cast<unsigned char>((l >> (8 * (nc-j-1))) & 0xFF);
      }
    }
  }
  return TRUE;
}

void
SoSFImage3::writeValue(SoOutput * out) const
{
  int nc;
  SbVec3s size;
  unsigned char * pixblock = this->image->getValue(size, nc);

  out->write(size[0]);
  if (!out->isBinary()) out->write(' ');
  out->write(size[1]);
  if (!out->isBinary()) out->write(' ');
  out->write(size[2]);
  if (!out->isBinary()) out->write(' ');
  out->write(nc);

  if (out->isBinary()) {
    int buffersize = int(size[0]) * int(size[1]) * int(size[2]) * nc;
    if (buffersize) { // in case of an empty image
      out->writeBinaryArray(pixblock, buffersize);
      int padsize = ((buffersize + 3) / 4) * 4 - buffersize;
      if (padsize) {
        unsigned char pads[3] = {'\0','\0','\0'};
        out->writeBinaryArray(pads, padsize);
      }
    }
  }
  else {
    out->write('\n');
    out->indent();

    int numpixels = int(size[0]) * int(size[1]) * int(size[2]);
    for (int i = 0; i < numpixels; i++) {
      unsigned int data = 0;
      for (int j = 0; j < nc; j++) {
        if (j) data <<= 8;
        data |= static_cast<unsigned int>(pixblock[i * nc + j]);
      }
      out->write(data);
      if (((i+1)%8 == 0) && (i+1 != numpixels)) {
        out->write('\n');
        out->indent();
      }
      else {
        out->write(' ');
      }
    }
  }
}


/*!
  \fn int SoSFImage3::operator!=(const SoSFImage3 & field) const
  Compare image of \a field with the image in this field and
  return \c FALSE if they are equal.
*/

/*!
  Compare image of \a field with the image in this field and
  return \c TRUE if they are equal.
*/
int
SoSFImage3::operator==(const SoSFImage3 & field) const
{
  return (*this->image) == (*field.image);
}

/*!
  Return pixel buffer, set \a size to contain the image dimensions and
  \a nc to the number of components in the image.
*/
const unsigned char *
SoSFImage3::getValue(SbVec3s & size, int & nc) const
{
  return this->image->getValue(size, nc);
}

/*!
  Initialize this field to \a size and \a nc.

  If \a bytes is not \c NULL, the image data is copied from \a bytes
  into this field.  If \a bytes is \c NULL, the image data is cleared
  by setting all bytes to 0 (note that the behavior on passing a \c
  NULL pointer is specific for Coin, Open Inventor will crash if you
  try it).
*/
void
SoSFImage3::setValue(const SbVec3s & size, const int nc,
                     const unsigned char * bytes)
{
  this->image->setValue(size, nc, bytes);
  this->valueChanged();
}

/*!
  Return pixel buffer, set \a size to contain the image dimensions and
  \a nc to the number of components in the image.

  The field's container will not be notified about the changes
  until you call finishEditing().
*/
unsigned char *
SoSFImage3::startEditing(SbVec3s & size, int & nc)
{
  return this->image->getValue(size, nc);
}

/*!
  Notify the field's auditors that the image data have been
  modified.
*/
void
SoSFImage3::finishEditing(void)
{
  this->valueChanged();
}

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoSFImage3 field;
  BOOST_CHECK_MESSAGE(SoSFImage3::getClassTypeId() != SoType::badType(),
                      "SoSFImage3 class not initialized");
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
