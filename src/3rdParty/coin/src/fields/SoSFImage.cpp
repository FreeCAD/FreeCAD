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
  \class SoSFImage SoSFImage.h Inventor/fields/SoSFImage.h
  \brief The SoSFImage class is used to store pixel images.

  \ingroup coin_fields

  The SoSFImage class provides storage for inline 2D image
  maps. Images in Coin are mainly used for texture mapping support.

  SoSFImage instances can be exported and imported as any other field
  class in Coin.

  The components of an SoSFImage is: its image dimensions (width and
  height), the number of bytes used for describing each pixel (number
  of components) and an associated pixel buffer. The size of the pixel
  buffer will be width*height*components.

  For texture maps, the components / bytes-per-pixel setting
  translates as follows: use 1 for a grayscale imagemap, 2 for
  grayscale + opacity (i.e. alpha value), 3 for RGB (1 byte each for
  red, green and blue) and 4 components means 3 bytes for RGB + 1 byte
  opacity value (aka RGBA).

  This field is serializable into the Inventor / Coin file format in
  the following manner:

  \code
  FIELDNAME X Y C 0xRRGGBBAA 0xRRGGBBAA ...
  \endcode

  "X" and "Y" are the image dimensions along the given axes, "C" is
  the number of components in the image. The number of 0xRRGGBBAA
  pixel color specifications needs to equal the exact number of
  pixels, which of course is given by X*Y. Each part of the pixel
  color value is in the range 0x00 to 0xff (hexadecimal, 0 to 255
  decimal).

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

  The pixels are read as being ordered in rows along X (width) and
  columns along Y (height, bottom to top).

  Here's a simple example of the file format serialization, for a 2x2
  RGB-image inside an SoTexture2 node, as mapped onto an SoCube:

  \code
  Complexity { textureQuality 0.1 }   # set low to avoid smoothing

  Texture2 {
     image 2 2 4

     0xffffffff 0x00ff0088   # white   semi-transparent green
     0xff0000ff 0xffff00ff   #  red    yellow
  }

  Cube { }
  \endcode

  The mini-scene graph above results in the following mapping on the
  cube:<br>

  <center>
  \image html sosfimage.png "Rendering of Example Scenegraph"
  </center>

  The cube has only been \e slightly rotated, so as you can see from
  the snapshot, the Y-rows are mapped from bottom to top, while the
  X-column pixels are mapped onto the cube from left to right.

  \sa SoTexture2, SoSFImage3
*/

// *************************************************************************

#include <Inventor/fields/SoSFImage.h>

#include "coindefs.h"

#include <cstdlib> // free()

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/SbImage.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/errors/SoDebugError.h>

#include "fields/SoSubFieldP.h"

// *************************************************************************

class SoSFImageP {
public:
  SoSFImageP(void) {
    this->image = new SbImage;
    this->freeimage = NULL;
    this->deleteimage = NULL;
  }
  ~SoSFImageP() {
    delete this->image;
    free(this->freeimage);
    delete[] this->deleteimage;
  }
  SbImage * image;
  unsigned char * deleteimage; // free this data using delete[]
  unsigned char * freeimage; // free this data using free()
};

#define PRIVATE(p) ((p)->pimpl)

// *************************************************************************

PRIVATE_TYPEID_SOURCE(SoSFImage);
PRIVATE_EQUALITY_SOURCE(SoSFImage);

// *************************************************************************

// (Declarations hidden in macro in SoSFImage.h, so don't use Doxygen
// commenting.)
#ifndef DOXYGEN_SKIP_THIS

/* Constructor, initializes fields to represent an empty image. */
SoSFImage::SoSFImage(void)
{
  PRIVATE(this) = new SoSFImageP;
}

/* Free all resources associated with the image. */
SoSFImage::~SoSFImage()
{
  delete PRIVATE(this);
}

/* Copy the image of \a field into this field. */
const SoSFImage &
SoSFImage::operator=(const SoSFImage & field)
{
  int nc = 0;
  SbVec2s size(0,0);
  unsigned char * bytes = PRIVATE(&field)->image->getValue(size, nc);

  this->setValue(size, nc, bytes);
  return *this;
}

#endif // DOXYGEN_SKIP_THIS


/*!
  \copydetails SoField::initClass(void)
*/
void
SoSFImage::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFImage);
}

SbBool
SoSFImage::readValue(SoInput * in)
{
  SbVec2s size;
  int nc;
  if (!in->read(size[0]) || !in->read(size[1]) ||
      !in->read(nc)) {
    SoReadError::post(in, "Premature end of file");
    return FALSE;
  }

  // Note: empty images (dimensions 0x0x0) are allowed.

  if (size[0] < 0 || size[1] < 0 || nc < 0 || nc > 4) {
    SoReadError::post(in, "Invalid image specification %dx%dx%d",
                      size[0], size[1], nc);
    return FALSE;
  }

  int buffersize = int(size[0]) * int(size[1]) * nc;

  if (buffersize == 0 &&
      (size[0] != 0 || size[1] != 0 || nc != 0)) {
    SoReadError::post(in, "Invalid image specification %dx%dx%d",
                      size[0], size[1], nc);
    return FALSE;
  }

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoSFImage::readValue", "image dimensions: %dx%dx%d",
                         size[0], size[1], nc);
#endif // debug

  if (!buffersize) {
    PRIVATE(this)->image->setValue(SbVec2s(0,0), 0, NULL);
    return TRUE;
  }

  // allocate image data and get new pointer back
  PRIVATE(this)->image->setValue(size, nc, NULL);
  unsigned char * pixblock = PRIVATE(this)->image->getValue(size, nc);

  // The binary image format of 2.1 and later tries to be less
  // wasteful when storing images.
  if (in->isBinary() && in->getIVVersion() >= 2.1f) {
    if (!in->readBinaryArray(pixblock, buffersize)) {
      SoReadError::post(in, "Premature end of file");
      return FALSE;
    }
    // images are padded to a 4-byte alignment
    int padsize = ((buffersize + 3) / 4) * 4 - buffersize;
    if (padsize) {
      unsigned char pads[3]; // pad is at most 3 bytes
      if (!in->readBinaryArray(pads, padsize)) {
        SoReadError::post(in, "Premature end of file");
        return FALSE;
      }
    }
  }
  else {
    int byte = 0;
    int numpixels = int(size[0]) * int(size[1]);
    for (int i = 0; i < numpixels; i++) {
      unsigned int l;
      if (!in->read(l)) {
        SoReadError::post(in, "Premature end of file");
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
SoSFImage::writeValue(SoOutput * out) const
{
  int nc;
  SbVec2s size;
  unsigned char * pixblock = PRIVATE(this)->image->getValue(size, nc);

  out->write(size[0]);
  if (!out->isBinary()) out->write(' ');
  out->write(size[1]);
  if (!out->isBinary()) out->write(' ');
  out->write(nc);

  if (out->isBinary()) {
    int buffersize = int(size[0]) * int(size[1]) * nc;
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

    int numpixels = int(size[0]) * int(size[1]);
    for (int i = 0; i < numpixels; i++) {
      unsigned int data = 0;
      for (int j = 0; j < nc; j++) {
        if (j) data <<= 8;
        data |= static_cast<uint32_t>(pixblock[i * nc + j]);
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
  \fn int SoSFImage::operator!=(const SoSFImage & field) const
  Compare image of \a field with the image in this field and
  return \c FALSE if they are equal.
*/

/*!
  Compare image of \a field with the image in this field and
  return \c TRUE if they are equal.
*/
int
SoSFImage::operator==(const SoSFImage & field) const
{
  return (*PRIVATE(this)->image) == (*(PRIVATE(&field)->image));
}


/*!
  Return pixel buffer, set \a size to contain the image dimensions and
  \a nc to the number of components in the image.
*/
const unsigned char *
SoSFImage::getValue(SbVec2s & size, int & nc) const
{
  this->evaluate();
  return PRIVATE(this)->image->getValue(size, nc);
}

/*!
  \retval SbImage contained by this SoSFImage
*/
const SbImage &
SoSFImage::getValue() const
{
  this->evaluate();
  return *PRIVATE(this)->image;
}

/*!
  Initialize this field to \a size and \a nc.

  If \a pixels is not \c NULL, the image data is copied from \a pixels
  into this field.  If \a pixels is \c NULL, the image data is cleared
  by setting all bytes to 0 (note that the behavior on passing a \c
  NULL pointer is specific for Coin, Open Inventor will crash if you
  try it).

  The image dimensions is given by the \a size argument, and the \a nc
  argument specifies the number of bytes-pr-pixel. A 24-bit RGB image
  would for instance have an \a nc equal to 3.

  The \a copypolicy argument makes it possible to share image data
  with SoSFImage without the data being copied (thereby using less
  memory resources). The default is to copy image data from the \a
  pixels source into an internal copy.

  \e Important \e note: if you call this with \a copypolicy as either
  \c NO_COPY_AND_DELETE or \c NO_COPY_AND_FREE, and your application
  is running on Microsoft Windows, be aware that you will get
  mysterious crashes if your application is not using the same C
  library runtime as the Coin library.

  The cause of this is that a memory block would then be allocated by
  the application on the memory heap of one C library runtime (say,
  for instance \c MSVCRT.LIB), but attempted deallocated in the memory
  heap of another C library runtime (e.g. \c MSVCRTD.LIB), which
  typically leads to hard-to-debug crashes.

  \since The CopyPolicy argument was added in Coin 2.0.
  \since CopyPolicy was added to TGS Inventor 3.0.
*/
void
SoSFImage::setValue(const SbVec2s & size, const int nc,
                    const unsigned char * pixels,
                    SoSFImage::CopyPolicy copypolicy)
{
  // free old data
  free(PRIVATE(this)->freeimage);
  PRIVATE(this)->freeimage = NULL;
  delete[] PRIVATE(this)->deleteimage;
  PRIVATE(this)->deleteimage = NULL;
  // set new data
  switch (copypolicy) {
  default:
    assert(0 && "unknown copy policy");
  case COPY:
    PRIVATE(this)->image->setValue(size, nc, pixels);
    break;
  case NO_COPY:
    PRIVATE(this)->image->setValuePtr(size, nc, pixels);
    break;

    // FIXME: as for the "multiple C runtimes" problem mentioned in
    // the API docs above, would it be possible to put in a check for
    // whether or not the memory block is within the same C library
    // heap as for the Coin library itself? I seem to remember that
    // there is such a function in the Win32 API.  20050518 mortene.
    //
    // UPDATE 20050518 mortene: _CrtIsValidHeapPointer() is mentioned
    // in the MSDN docs, and seems to be a step in the right
    // direction. This may however be a macro, which uses some
    // underlying Win32 API call -- check that.

  case NO_COPY_AND_DELETE:
    PRIVATE(this)->image->setValuePtr(size, nc, pixels);
    PRIVATE(this)->deleteimage = const_cast<unsigned char *>(pixels);
    break;
  case NO_COPY_AND_FREE:
    PRIVATE(this)->image->setValuePtr(size, nc, pixels);
    PRIVATE(this)->freeimage = const_cast<unsigned char *>(pixels);
    break;
  }
  this->valueChanged();
}

/*!
  Return pixel buffer. Return the image size and components in
  \a size and \a nc.

  You cannot use this method to set a new image size. Use setValue()
  to change the size of the image buffer.

  The field's container will not be notified about the changes
  until you call finishEditing().
*/
unsigned char *
SoSFImage::startEditing(SbVec2s & size, int & nc)
{
  return PRIVATE(this)->image->getValue(size, nc);
}

/*!
  Notify the field's auditors that the image data have been
  modified.
*/
void
SoSFImage::finishEditing(void)
{
  this->valueChanged();
}

/*!
  Not yet implemented for Coin. Get in touch if you need this method.

  \since Coin 2.0
  \since TGS Inventor 3.0
 */
void
SoSFImage::setSubValue(
                     const SbVec2s & COIN_UNUSED_ARG(dims),
                     const SbVec2s & COIN_UNUSED_ARG(offset),
                     unsigned char * COIN_UNUSED_ARG(pixels)
                     )
{
  // FIXME: unimplemented yet. 20030226 mortene.
  SoDebugError::postWarning("SoSFImage::setSubValue",
                            "Not yet implemented for Coin. "
                            "Get in touch if you need this functionality.");
}

/*!
  Not yet implemented for Coin. Get in touch if you need this method.

  \since Coin 2.0
  \since TGS Inventor 3.0
 */
void
SoSFImage::setSubValues(
                     const SbVec2s * COIN_UNUSED_ARG(dims),
                     const SbVec2s * COIN_UNUSED_ARG(offsets),
                     int COIN_UNUSED_ARG(num),
                     unsigned char ** COIN_UNUSED_ARG(pixelblocks)
                     )
{
  // FIXME: unimplemented yet. 20030226 mortene.
  SoDebugError::postWarning("SoSFImage::setSubValues",
                            "Not yet implemented for Coin. "
                            "Get in touch if you need this functionality.");
}

/*!
  Not yet implemented for Coin. Get in touch if you need this method.

  \since Coin 2.0
  \since TGS Inventor 3.0
 */
unsigned char *
SoSFImage::getSubTexture(
                      int COIN_UNUSED_ARG(idx),
                      SbVec2s & COIN_UNUSED_ARG(dims),
                      SbVec2s & COIN_UNUSED_ARG(offset)
                      ) const
{
  // FIXME: unimplemented yet. 20030226 mortene.
  SoDebugError::postWarning("SoSFImage::getSubTexture",
                            "Not yet implemented for Coin. "
                            "Get in touch if you need this functionality.");
  return NULL;
}

/*!
  Returns whether or not sub textures was set up for this field.

  If \c TRUE is returned, the \a numsubtextures argument will be set
  to the number of sub textures in this image. This number can be used
  for iterating over all textures with the SoSFImage::getSubTextures()
  method.

  \since Coin 2.0
  \since TGS Inventor 3.0
 */
SbBool
SoSFImage::hasSubTextures(int & numsubtextures)
{
  // FIXME: unimplemented yet. 20030226 mortene.
  numsubtextures = 0;
  return FALSE;
}

/*!
  Set this flag to true to avoid writing out the texture to file. This
  can save a lot on file size.

  Default value is \c FALSE (i.e. write texture data to file.)

  (Note: yet unimplemented for Coin.)

  \since Coin 2.0
  \since TGS Inventor ?.?
 */
void
SoSFImage::setNeverWrite(SbBool COIN_UNUSED_ARG(flag))
{
  // FIXME: unimplemented yet. 20030226 mortene.
  SoDebugError::postWarning("SoSFImage::setNeverWrite",
                            "Not yet implemented for Coin. "
                            "Get in touch if you need this functionality.");
}

/*!
  Returns value of "never write texture data" flag.

  \sa SoSFImage::setNeverWrite()

  \since Coin 2.0
  \since TGS Inventor ?.?
 */
SbBool
SoSFImage::isNeverWrite(void) const
{
  // FIXME: unimplemented yet. 20030226 mortene.
  return FALSE;
}

/*!
  Returns \c TRUE if at least one pixel of the image in this field is
  not completely opaque, otherwise \c FALSE.

  \since Coin 2.0
  \since TGS Inventor ?.?
 */
SbBool
SoSFImage::hasTransparency(void) const
{
  // FIXME: unimplemented yet. 20030226 mortene.
  SoDebugError::postWarning("SoSFImage::hasTransparency",
                            "Not yet implemented for Coin. "
                            "Get in touch if you need this functionality.");
  return TRUE;
}

#undef PRIVATE

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoSFImage field;
  BOOST_CHECK_MESSAGE(SoSFImage::getClassTypeId() != SoType::badType(),
                      "SoSFImage class not initialized");
  BOOST_CHECK_MESSAGE(field.getTypeId() != SoType::badType(),
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
