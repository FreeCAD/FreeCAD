#ifndef COIN_SONORMALBUNDLE_H
#define COIN_SONORMALBUNDLE_H

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
#include <Inventor/misc/SoNormalGenerator.h>
#include <Inventor/elements/SoGLNormalElement.h>

class SoNormalBundleP;

class COIN_DLL_API SoNormalBundle : public SoBundle {
public:
  SoNormalBundle(SoAction * action, SbBool forrendering);
  ~SoNormalBundle();

  SbBool shouldGenerate(int numneeded);
  void initGenerator(int initnum = 100);
  void beginPolygon(void);
  void polygonVertex(const SbVec3f & v);
  void endPolygon(void);
  
  void triangle(const SbVec3f & p1,
                const SbVec3f & p2,
                const SbVec3f & p3);
  void generate(int startindex = 0,
                SbBool addtostate = TRUE);
  const SbVec3f * getGeneratedNormals(void) const;
  int getNumGeneratedNormals(void) const;
  void set(int32_t num, const SbVec3f * normals);
  const SbVec3f & get(int index) const;
  void send(int index) const;
  
  SoNormalGenerator * generator; // SoINTERNAL public
  
private:
  const SoNormalElement * elem;
  const SoGLNormalElement * glelem;
  SoNode * node;
  SoNormalBundleP * pimpl; // for future use
};


#endif // !COIN_SONORMALBUNDLE_H
