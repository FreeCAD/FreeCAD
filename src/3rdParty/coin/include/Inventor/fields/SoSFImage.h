#ifndef COIN_SOSFIMAGE_H
#define COIN_SOSFIMAGE_H

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

#include <Inventor/fields/SoSField.h>
#include <Inventor/fields/SoSubField.h>
#include <Inventor/SbVec2s.h>

class SbImage;

class COIN_DLL_API SoSFImage : public SoSField {
  typedef SoSField inherited;

  SO_SFIELD_CONSTRUCTOR_HEADER(SoSFImage);
  SO_SFIELD_REQUIRED_HEADER(SoSFImage);

public:
  enum CopyPolicy {
    COPY,
    NO_COPY,
    NO_COPY_AND_DELETE,
    NO_COPY_AND_FREE
  };

  static void initClass(void);

  const unsigned char * getValue(SbVec2s & size, int & nc) const;
  const SbImage & getValue() const;

  void setValue(const SbVec2s & size, const int nc,
                const unsigned char * pixels, CopyPolicy copypolicy = COPY);

  int operator==(const SoSFImage & field) const;
  int operator!=(const SoSFImage & field) const { return ! operator == (field); }

  unsigned char * startEditing(SbVec2s & size, int & nc);
  void finishEditing(void);

  void setSubValue(const SbVec2s & dims, const SbVec2s & offset, unsigned char * pixels);
  void setSubValues(const SbVec2s * dims, const SbVec2s * offsets, int num, unsigned char ** pixelblocks);
  unsigned char * getSubTexture(int idx, SbVec2s & dims, SbVec2s & offset) const;
  SbBool hasSubTextures(int & numsubtextures);

  void setNeverWrite(SbBool flag);
  SbBool isNeverWrite(void) const;

  SbBool hasTransparency(void) const;

private:
  virtual SbBool readValue(SoInput * in);
  virtual void writeValue(SoOutput * out) const;

  class SoSFImageP * pimpl;
};

#endif // !COIN_SOSFIMAGE_H
