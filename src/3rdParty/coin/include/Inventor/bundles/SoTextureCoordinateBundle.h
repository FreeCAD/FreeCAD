#ifndef COIN_SOTEXTURECOORDINATEBUNDLE_H
#define COIN_SOTEXTURECOORDINATEBUNDLE_H

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

#include <Inventor/bundles/SoBundle.h>
#include <Inventor/SbBasic.h>
#include <Inventor/system/inttypes.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>

#include <Inventor/SbVec4f.h>

class SoTextureCoordinateCache;
class SoShape;


class COIN_DLL_API SoTextureCoordinateBundle : public SoBundle {
  typedef SoBundle inherited;
public:
  SoTextureCoordinateBundle(SoAction * const action,
                            const SbBool forRendering,
                            const SbBool setUpDefault = TRUE);
  ~SoTextureCoordinateBundle();

  SbBool needCoordinates(void) const;
  SbBool isFunction(void) const;

  const SbVec4f &get(const int index);
  const SbVec4f &get(const SbVec3f &point, const SbVec3f &normal);

  void send(const int index) const {
    glElt->send(index);
  }
  void send(const int index, const SbVec3f &point,
            const SbVec3f &normal) const {
    glElt->send(index, point, normal);
  }

  SbBool needIndices(void) const;

private:
  const SoMultiTextureCoordinateElement *coordElt;
  const SoGLMultiTextureCoordinateElement *glElt;
  unsigned int flags;

  // misc stuff for default texture coordinate mapping
  static const SbVec4f & defaultCB(void * userdata,
                                   const SbVec3f & point,
                                   const SbVec3f & normal);
  static const SbVec4f & defaultCBMulti(void * userdata,
                                        const SbVec3f & point,
                                        const SbVec3f & normal);
  SoShape * shapenode;
  SbVec3f defaultorigo;
  SbVec3f defaultsize;
  SbVec4f dummyInstance;
  int defaultdim0, defaultdim1;
  void initDefaultCallback(SoAction * action);
  void initDefault(SoAction * action, const int unit);
};


#endif // !COIN_SOTEXTURECOORDINATEBUNDLE_H
