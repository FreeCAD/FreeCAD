#ifndef COIN_SBIMAGE_H
#define COIN_SBIMAGE_H

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

#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3s.h>
#include <Inventor/SbString.h>
#include <cstddef> // for NULL

class SbImage;

typedef SbBool SbImageScheduleReadCB(const SbString &, SbImage *, void *);
typedef SbBool SbImageReadImageCB(const SbString &, SbImage *, void *);

class COIN_DLL_API SbImage {
public:
  SbImage(void);
  SbImage(const unsigned char * bytes,
          const SbVec2s & size, const int bytesperpixel);
  SbImage(const unsigned char * bytes,
          const SbVec3s & size, const int bytesperpixel);
  SbImage(const SbImage & image);
  ~SbImage();

  void setValue(const SbVec2s & size, const int bytesperpixel,
                const unsigned char * bytes);
  void setValue(const SbVec3s & size, const int bytesperpixel,
                const unsigned char * bytes);
  void setValuePtr(const SbVec2s & size, const int bytesperpixel,
                   const unsigned char * bytes);
  void setValuePtr(const SbVec3s & size, const int bytesperpixel,
                   const unsigned char * bytes);
  unsigned char * getValue(SbVec2s & size, int & bytesperpixel) const;
  unsigned char * getValue(SbVec3s & size, int & bytesperpixel) const;
  SbVec3s getSize(void) const;

  SbBool readFile(const SbString & filename,
                  const SbString * const * searchdirectories = NULL,
                  const int numdirectories = 0);

  int operator==(const SbImage & image) const;
  int operator!=(const SbImage & image) const {
    return ! operator == (image);
  }
  SbImage & operator=(const SbImage & image);

  static void addReadImageCB(SbImageReadImageCB * cb, void * closure);
  static void removeReadImageCB(SbImageReadImageCB * cb, void * closure);

  static SbString searchForFile(const SbString & basename,
                                const SbString * const * dirlist,
                                const int numdirs);

  SbBool hasData(void) const;

private:

  class SbImageP * pimpl;
  
public:

  // methods for delaying image loading until it is actually needed.
  void readLock(void) const;
  void readUnlock(void) const;

  SbBool scheduleReadFile(SbImageScheduleReadCB * cb,
                          void * closure,
                          const SbString & filename,
                          const SbString * const * searchdirectories = NULL,
                          const int numdirectories = 0);
};

#endif // !COIN_SBIMAGE_H
