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
  \class SbImage SbImage.h Inventor/SbImage.h
  \brief The SbImage class is an abstract data type for 2D and 3D images.

  \ingroup coin_base

  \COIN_CLASS_EXTENSION
  \since Coin 1.0
*/

// FIXME: this class could be used to handle image reusage, since it's
// quite common that the same image is used several times in a scene
// and for different contexts. The API should stay the same though.
// 20001026 mortene (original comment by pederb).

/*!
  \typedef SbBool SbImageScheduleReadCB(const SbString &, SbImage *, void *)

  The type definition of the callback function that is called when a file is
  scheduled for reading.
*/

/*!
  \typedef SbBool SbImageReadImageCB(const SbString &, SbImage *, void *)

  The type definition of the callback function that is called to actually
  read the image file.
*/

#include <Inventor/SbImage.h>

#include <cstring>
#include <cstdlib>

#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3s.h>
#include <Inventor/SbString.h>
#include <Inventor/SoInput.h> // for SoInput::searchForFile()
#include <Inventor/lists/SbStringList.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/tidbits.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbRWMutex.h>
#endif // COIN_THREADSAFE

#include "glue/simage_wrapper.h"
#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::memcmp;
using std::memcpy;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

class SbImageP {
public:
  typedef struct {
    SbImageReadImageCB * cb;
    void * closure;
  } ReadImageCBData;

  enum DataType {
    INTERNAL_DATA,
    SIMAGE_DATA,
    SETVALUEPTR_DATA
  };

  SbImageP(void)
    : bytes(NULL),
      datatype(SETVALUEPTR_DATA),
      size(0,0,0),
      bpp(0),
      schedulecb(NULL),
      scheduleclosure(NULL)
#ifdef COIN_THREADSAFE
    , rwmutex(SbRWMutex::READ_PRECEDENCE)
#endif // COIN_THREADSAFE
  { }
  void freeData(void) {
    if (this->bytes) {
      switch (this->datatype) {
      default:
        assert(0 && "unknown data type");
        break;
      case INTERNAL_DATA:
        delete[] this->bytes;
        this->bytes = NULL;
        break;
      case SIMAGE_DATA:
        simage_wrapper()->simage_free_image(this->bytes);
        this->bytes = NULL;
        break;
      case SETVALUEPTR_DATA:
        this->bytes = NULL;
        break;
      }
    }
    this->datatype = SETVALUEPTR_DATA;
  }

  unsigned char * bytes;
  DataType datatype;
  SbVec3s size;
  int bpp;
  SbString schedulename;
  SbImageScheduleReadCB * schedulecb;
  void * scheduleclosure;

  static SbList <ReadImageCBData> * readimagecallbacks;

#ifdef COIN_THREADSAFE
  SbRWMutex rwmutex;
  void readLock(void) {
    //    fprintf(stderr,"readlock: %p\n", this);
    this->rwmutex.readLock();
    //fprintf(stderr,"readlock achieved: %p\n", this);
  }
  void readUnlock(void) {
    //fprintf(stderr,"readUnlock: %p\n", this);
    this->rwmutex.readUnlock();
  }
  void writeLock(void) {
    //fprintf(stderr,"writelock: %p\n", this);
    this->rwmutex.writeLock();
    //fprintf(stderr,"writelock achieved: %p\n", this);
  }
  void writeUnlock(void) {
    //fprintf(stderr,"writeUnlock: %p\n", this);
    this->rwmutex.writeUnlock();
  }
#else // COIN_THREADSAFE
  void readLock(void) { }
  void readUnlock(void) { }
  void writeLock(void) { }
  void writeUnlock(void) { }
#endif // ! COIN_THREADSAFE

};

extern "C" {

static void SbImage_cleanup_callback(void) {
  delete SbImageP::readimagecallbacks;
  SbImageP::readimagecallbacks = NULL;
}

} // extern "C"

SbList <SbImageP::ReadImageCBData> * SbImageP::readimagecallbacks = NULL;

//////////////////////////////////////////////////////////////////////////

#define PRIVATE(image) ((image)->pimpl)

/*!
  Default constructor.
*/
SbImage::SbImage(void)
{
  PRIVATE(this) = new SbImageP;
}

/*!
  Constructor which sets 2D data using setValue().
  \sa setValue()
*/
SbImage::SbImage(const unsigned char * bytes,
                 const SbVec2s & size, const int bytesperpixel)
{
  PRIVATE(this) = new SbImageP;
  this->setValue(size, bytesperpixel, bytes);
}

/*!
  Constructor which sets 3D data using setValue().

  \COIN_FUNCTION_EXTENSION

  \sa setValue()
  \since Coin 2.0
*/
SbImage::SbImage(const unsigned char * bytes,
                 const SbVec3s & size, const int bytesperpixel)
{
  PRIVATE(this) = new SbImageP;
  this->setValue(size, bytesperpixel, bytes);
}

/*!
  Copy constructor

  \since Coin 4.0
 */
SbImage::SbImage(const SbImage & that)
{
  PRIVATE(this) = new SbImageP;
  *this=that;
}

/*!
  Destructor.
*/
SbImage::~SbImage(void)
{
  PRIVATE(this)->freeData();
  delete PRIVATE(this);
}

/*!
  Apply a read lock on this image. This will make it impossible for
  other threads to change the image while this lock is active. Other
  threads can do read-only operations on this image, of course.

  For the single thread version of Coin, this method does nothing.

  \sa readUnlock()
  \since Coin 2.0
*/
void
SbImage::readLock(void) const
{
  PRIVATE(this)->readLock();
}

/*!
  Release a read lock on this image.

  For the single thread version of Coin, this method does nothing.

  \sa readLock()
  \since Coin 2.0
*/
void
SbImage::readUnlock(void) const
{
  PRIVATE(this)->readUnlock();
}

/*!
  Convenience 2D version of setValuePtr.

  \sa setValue()
  \since Coin 2.0
*/
void
SbImage::setValuePtr(const SbVec2s & size, const int bytesperpixel,
                     const unsigned char * bytes)
{
  SbVec3s tmpsize(size[0], size[1], 0);
  this->setValuePtr(tmpsize, bytesperpixel, bytes);
}

/*!
  Sets the image data without copying the data. \a bytes will be used
  directly, and the data will not be freed when the image instance is
  destructed.

  If the depth of the image (size[2]) is zero, the image is considered
  a 2D image.

  \sa setValue()
  \since Coin 2.0
*/
void
SbImage::setValuePtr(const SbVec3s & size, const int bytesperpixel,
                     const unsigned char * bytes)
{
  PRIVATE(this)->writeLock();
  PRIVATE(this)->schedulename = "";
  PRIVATE(this)->schedulecb = NULL;
  PRIVATE(this)->freeData();
  PRIVATE(this)->bytes = const_cast<unsigned char *>(bytes);
  PRIVATE(this)->datatype = SbImageP::SETVALUEPTR_DATA;
  PRIVATE(this)->size = size;
  PRIVATE(this)->bpp = bytesperpixel;
  PRIVATE(this)->writeUnlock();
}

/*!
  Convenience 2D version of setValue.
*/
void
SbImage::setValue(const SbVec2s & size, const int bytesperpixel,
                  const unsigned char * bytes)
{
  SbVec3s tmpsize(size[0], size[1], 0);
  this->setValue(tmpsize, bytesperpixel, bytes);
}

/*!
  Sets the image to \a size and \a bytesperpixel. If \a bytes !=
  NULL, data is copied from \a bytes into this class' image data. If
  \a bytes == NULL, the image data is left uninitialized.

  The image data will always be allocated in multiples of four. This
  means that if you set an image with size == (1,1,1) and bytesperpixel
  == 1, four bytes will be allocated to hold the data. This is mainly
  done to simplify the export code in SoSFImage and normally you'll
  not have to worry about this feature.

  If the depth of the image (size[2]) is zero, the image is considered
  a 2D image.

  \since Coin 2.0
*/
void
SbImage::setValue(const SbVec3s & size, const int bytesperpixel,
                  const unsigned char * bytes)
{
  PRIVATE(this)->writeLock();
  PRIVATE(this)->schedulename = "";
  PRIVATE(this)->schedulecb = NULL;
  size_t buffersize = size_t(size[0]) * size_t(size[1]) * size_t(size[2] == 0 ? 1 : size[2]) * size_t(bytesperpixel);
  if (PRIVATE(this)->bytes && PRIVATE(this)->datatype == SbImageP::INTERNAL_DATA) {
    // check for special case where we don't have to reallocate
    if (bytes && (size == PRIVATE(this)->size) && (bytesperpixel == PRIVATE(this)->bpp)) {
      (void)memcpy(PRIVATE(this)->bytes, bytes, buffersize);
      PRIVATE(this)->writeUnlock();
      return;
    }
  }
  PRIVATE(this)->freeData();
  PRIVATE(this)->size = size;
  PRIVATE(this)->bpp = bytesperpixel;
  if (buffersize) {
    // Align buffers because the binary file format has the data aligned
    // (simplifies export code in SoSFImage).
    size_t alignedbuffersize = ((buffersize + 3) / 4) * 4;
    PRIVATE(this)->bytes = new unsigned char[alignedbuffersize];
    PRIVATE(this)->datatype = SbImageP::INTERNAL_DATA;

    if (bytes) {
      // Important: don't copy buffersize num bytes here!
      (void)memcpy(PRIVATE(this)->bytes, bytes, buffersize);
    }
  }
  PRIVATE(this)->writeUnlock();
}

/*!
  Returns the 2D image data.
*/
unsigned char *
SbImage::getValue(SbVec2s & size, int & bytesperpixel) const
{
  SbVec3s tmpsize;
  unsigned char *bytes = this->getValue(tmpsize, bytesperpixel);
  size.setValue(tmpsize[0], tmpsize[1]);
  return bytes;
}

/*!
  Returns the 3D image data.

  \since Coin 2.0
*/
unsigned char *
SbImage::getValue(SbVec3s & size, int & bytesperpixel) const
{
  PRIVATE(this)->readLock();
  if (PRIVATE(this)->schedulecb) {
    // start a thread to read the image.
    SbBool scheduled = PRIVATE(this)->schedulecb(PRIVATE(this)->schedulename, const_cast<SbImage *>(this),
                                        PRIVATE(this)->scheduleclosure);
    if (scheduled) {
      PRIVATE(this)->schedulecb = NULL;
    }
  }
  size = PRIVATE(this)->size;
  bytesperpixel = PRIVATE(this)->bpp;
  unsigned char * bytes = PRIVATE(this)->bytes;
  PRIVATE(this)->readUnlock();
  return bytes;

}

/*!
  Given a \a basename for a file and and array of directories to
  search (in \a dirlist, of length \a numdirs), returns the full name
  of the file found.

  In addition to looking at the root of each directory in \a dirlist,
  we also look into the subdirectories \e texture/, \e textures/, \e
  images/, \e pics/ and \e pictures/ of each \a dirlist directory.

  If no file matching \a basename could be found, returns an empty
  string.
*/
SbString
SbImage::searchForFile(const SbString & basename,
                       const SbString * const * dirlist, const int numdirs)
{
  int i;
  SbStringList directories;
  SbStringList subdirectories;

  for (i = 0; i < numdirs; i++) {
    directories.append(const_cast<SbString *>(dirlist[i]));
  }
  subdirectories.append(new SbString("texture"));
  subdirectories.append(new SbString("textures"));
  subdirectories.append(new SbString("images"));
  subdirectories.append(new SbString("pics"));
  subdirectories.append(new SbString("pictures"));

  SbString ret = SoInput::searchForFile(basename, directories, subdirectories);
  for (i = 0; i < subdirectories.getLength(); i++) {
    delete subdirectories[i];
  }
  return ret;
}

/*!
  Reads image data from \a filename. In Coin, simage is used to
  load image files, and several common file formats are supported.
  simage can be downloaded from our web pages.  If loading
  fails for some reason this method returns FALSE, and the instance
  is set to an empty image. If the file is successfully loaded, the
  file image data is copied into this class.

  If \a numdirectories > 0, this method will search for \a filename
  in all directories in \a searchdirectories.
*/
SbBool
SbImage::readFile(const SbString & filename,
                  const SbString * const * searchdirectories,
                  const int numdirectories)
{
  // FIXME: Add 3D image support when that is added to simage (kintel 20011118)

  if (filename.getLength() == 0) {
    // This is really an internal error, should perhaps assert. <mortene>.
    SoDebugError::post("SbImage::readFile",
                       "attempted to read file from empty filename.");
    return FALSE;
  }

  SbString finalname = SbImage::searchForFile(filename, searchdirectories,
                                              numdirectories);

  // use callback to load the image if it is set
  if (SbImageP::readimagecallbacks) {
    for (int i = 0; i < SbImageP::readimagecallbacks->getLength(); i++) {
      SbImageP::ReadImageCBData cbdata = (*SbImageP::readimagecallbacks)[i];
      if (finalname.getLength() > 0 && cbdata.cb(finalname, this, cbdata.closure)) return TRUE;
      if (cbdata.cb(filename, this, cbdata.closure)) return TRUE;
    }
    if (!simage_wrapper()->available) {
      return FALSE;
    }
  }

  if (finalname.getLength() == 0) {
    SoDebugError::post("SbImage::readFile",
                       "couldn't find '%s'.", filename.getString());
    return FALSE;
  }
  
  // try simage
  if (!simage_wrapper()->available) {
    SoDebugError::postWarning("SbImage::readFile",
                              "The simage library is not available, "
                              "cannot import any images from disk.");
    return FALSE;
  }

  assert(simage_wrapper()->simage_read_image);
  int w, h, nc;
  unsigned char * simagedata =
    simage_wrapper()->simage_read_image(finalname.getString(), 
                                        &w, &h, &nc);
  if (simagedata) {
    //FIXME: Add 3'rd dimension (kintel 20011110)
    this->setValuePtr(
                    SbVec3s(static_cast<short>(w),
                           static_cast<short>(h),
                           static_cast<short>(0)
                           ),
                    nc, simagedata);
    // NB, this is a trick. We use setValuePtr() to set the size
    // and data pointer, and then we change the data type to simage
    // peder, 2002-03-22
    PRIVATE(this)->datatype = SbImageP::SIMAGE_DATA;
    return TRUE;
  }
#if COIN_DEBUG
  else {
    SoDebugError::post("SbImage::readFile", "(%s) %s",
                       filename.getString(),
                       // FIXME: "getlasterror" is a crap strategy, as
                       // it places extra burden on the client to
                       // lock. Should keep a single entry-lock within
                       // simage_wrapper() to work around
                       // this. 20020628 mortene.
                       simage_wrapper()->simage_get_last_error ?
                       simage_wrapper()->simage_get_last_error() :
                       "Unknown error");
  }
#endif // COIN_DEBUG
    
  this->setValue(SbVec3s(0,0,0), 0, NULL);
  return FALSE;
}

/*!
  \fn int SbImage::operator!=(const SbImage & image) const
  Compare image of \a image with the image in this class and
  return \c FALSE if they are equal.
*/


/*!
  Compare image of \a image with the image in this class and
  return \c TRUE if they are equal.
*/
int
SbImage::operator==(const SbImage & image) const
{
  this->readLock();
  int ret = 0;
  if (!PRIVATE(this)->schedulecb && !PRIVATE(&image)->schedulecb) {
    if (PRIVATE(this)->size != PRIVATE(&image)->size) ret = 0;
    else if (PRIVATE(this)->bpp != PRIVATE(&image)->bpp) ret = 0;
    else if (PRIVATE(this)->bytes == NULL || PRIVATE(&image)->bytes == NULL) {
      ret = (PRIVATE(this)->bytes == PRIVATE(&image)->bytes);
    }
    else {
      size_t buffersize = size_t(PRIVATE(this)->size[0]) *
          size_t(PRIVATE(this)->size[1]) *
          size_t(PRIVATE(this)->size[2] == 0 ? 1 : PRIVATE(this)->size[2]) *
          size_t(PRIVATE(this)->bpp);
      ret = memcmp(PRIVATE(this)->bytes, PRIVATE(&image)->bytes,
                   buffersize) == 0;
    }
  }
  this->readUnlock();
  return ret;
}

/*!
  Assignment operator.
*/
SbImage & 
SbImage::operator=(const SbImage & image)
{
  if (*this != image ) {
    PRIVATE(this)->writeLock();
    PRIVATE(this)->freeData();
    PRIVATE(this)->writeUnlock();

    if (PRIVATE(&image)->bytes) {
      PRIVATE(&image)->readLock();

      switch (PRIVATE(&image)->datatype) {
      default:
        assert(0 && "unknown data type");
        break;
      case SbImageP::INTERNAL_DATA:
      case SbImageP::SIMAGE_DATA:
        // need to copy data both for INTERNAL and SIMAGE data, since
        // we can only free the data once when the data is of SIMAGE type.
        this->setValue(PRIVATE(&image)->size,
                       PRIVATE(&image)->bpp,
                       PRIVATE(&image)->bytes);
        break;
      case SbImageP::SETVALUEPTR_DATA:
        // just set the data ptr
        this->setValuePtr(PRIVATE(&image)->size,
                          PRIVATE(&image)->bpp,
                          PRIVATE(&image)->bytes);
        break;
      }
      PRIVATE(&image)->readUnlock();
    }
  }
  return *this;
}


/*!
  Schedule a file for reading. \a cb will be called the first time
  getValue() is called for this image, and the callback should then
  start a thread to read the image. Do not read the image in the
  callback, as this will lock up the application.

  \sa readFile()
  \since Coin 2.0
*/
SbBool
SbImage::scheduleReadFile(SbImageScheduleReadCB * cb,
                          void * closure,
                          const SbString & filename,
                          const SbString * const * searchdirectories,
                          const int numdirectories)
{
  this->setValue(SbVec3s(0,0,0), 0, NULL);
  PRIVATE(this)->writeLock();
  PRIVATE(this)->schedulecb = NULL;
  PRIVATE(this)->schedulename =
    this->searchForFile(filename, searchdirectories, numdirectories);
  int len = PRIVATE(this)->schedulename.getLength();
  if (len > 0) {
    PRIVATE(this)->schedulecb = cb;
    PRIVATE(this)->scheduleclosure = closure;
  }
  PRIVATE(this)->writeUnlock();
  return len > 0;
}

/*!
  Returns \a TRUE if the image is not empty. This can be useful, since
  getValue() will start loading the image if scheduleReadFile() has
  been used to set the image data.

  \since Coin 2.0
*/
SbBool 
SbImage::hasData(void) const
{
  SbBool ret;
  this->readLock();
  ret = PRIVATE(this)->bytes != NULL;
  this->readUnlock();
  return ret;
}

/*!
  Returns the size of the image. If this is a 2D image, the
  z component is zero. If this is a 3D image, the z component is
  >= 1.

  \since Coin 2.0
 */
SbVec3s
SbImage::getSize(void) const
{
  return PRIVATE(this)->size;
}

/*!
  Add a callback which will be called whenever Coin wants to read an
  image file.  The callback should return TRUE if it was able to
  successfully read and set the image data, and FALSE otherwise.

  The callback(s) will be called before attempting to use simage to
  load images.
  
  \sa removeReadImageCB()
  \since Coin 3.0
*/
void 
SbImage::addReadImageCB(SbImageReadImageCB * cb, void * closure)
{
  if (!SbImageP::readimagecallbacks) {
    SbImageP::readimagecallbacks = new SbList <SbImageP::ReadImageCBData>;
    cc_coin_atexit(static_cast<coin_atexit_f*>(SbImage_cleanup_callback));
  }
  SbImageP::ReadImageCBData data;
  data.cb = cb;
  data.closure = closure;
  
  SbImageP::readimagecallbacks->append(data);
}

/*!
  Remove a read image callback added with addReadImageCB().

  \sa addReadImageCB()
  \since Coin 3.0
*/
void 
SbImage::removeReadImageCB(SbImageReadImageCB * cb, void * closure)
{
  if (SbImageP::readimagecallbacks) {
    for (int i = 0; i < SbImageP::readimagecallbacks->getLength(); i++) {
      SbImageP::ReadImageCBData data = (*SbImageP::readimagecallbacks)[i];
      if (data.cb == cb && data.closure == closure) {
        SbImageP::readimagecallbacks->remove(i);
        return;
      }
    }
  }
}

#undef PRIVATE

#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(copyConstruct) 
{
  unsigned char buf [4];

  for (int i=0;i<sizeof(buf); ++i) {
    buf[i]=i;
  }

  SbImage bar(buf,SbVec2s(2,2),1);

  SbImage foo(bar);

  SbVec2s tmp1;
  int tmp2;


  for (size_t i=0;i<sizeof(buf); ++i) {
    BOOST_CHECK_MESSAGE(foo.getValue(tmp1,tmp2)[i]==bar.getValue(tmp1,tmp2)[i],"Input value error");
  }

  for (size_t i=0;i<sizeof(buf); ++i) {
    foo.getValue(tmp1,tmp2)[i]=(unsigned char)(sizeof(buf)-i);
  }

  for (size_t i=0;i<sizeof(buf); ++i) {
    BOOST_CHECK_MESSAGE(foo.getValue(tmp1,tmp2)[i]==sizeof(buf)-bar.getValue(tmp1,tmp2)[i],"Error after changing second buffer");
  }

}
#endif //COIN_TEST_SUITE

