#ifndef COIN_SOGETPRIMITIVECOUNTACTION_H
#define COIN_SOGETPRIMITIVECOUNTACTION_H

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

#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoSubAction.h>
#include <Inventor/tools/SbLazyPimplPtr.h>
#include <Inventor/elements/SoDecimationTypeElement.h>

class SoGetPrimitiveCountActionP;
class SbViewportRegion;

class COIN_DLL_API SoGetPrimitiveCountAction : public SoAction {
  typedef SoAction inherited;

  SO_ACTION_HEADER(SoGetPrimitiveCountAction);

public:
  static void initClass(void);

  SoGetPrimitiveCountAction(void);
  SoGetPrimitiveCountAction(const SbViewportRegion & vp);

  virtual ~SoGetPrimitiveCountAction(void);

  int getTriangleCount(void) const;
  int getLineCount(void) const;
  int getPointCount(void) const;
  int getTextCount(void) const;
  int getImageCount(void) const;
  SbBool containsNoPrimitives(void);
  SbBool containsNonTriangleShapes(void);

  SbBool is3DTextCountedAsTriangles(void);
  void setCount3DTextAsTriangles(const SbBool flag);

  SbBool canApproximateCount(void);
  void setCanApproximate(const SbBool flag);

  void setDecimationValue(SoDecimationTypeElement::Type type,
                          float percentage = 1.0);
  SoDecimationTypeElement::Type getDecimationType(void);
  float getDecimationPercentage(void);

  void addNumTriangles(const int num);
  void addNumLines(const int num);
  void addNumPoints(const int num);
  void addNumText(const int num);
  void addNumImage(const int num);
  void incNumTriangles(void);
  void incNumLines(void);
  void incNumPoints(void);
  void incNumText(void);
  void incNumImage(void);

protected:
  virtual void beginTraversal(SoNode * node);

private:
  int numtris;
  int numlines;
  int numpoints;
  int numtexts;
  int numimages;

  SbBool textastris;
  SbBool approx;
  SbBool nonvertexastris;
  SoDecimationTypeElement::Type decimationtype;
  float decimationpercentage;

private:
  void commonConstructor(const SbViewportRegion & vp);
  SbLazyPimplPtr<SoGetPrimitiveCountActionP> pimpl;

  // NOT IMPLEMENTED:
  SoGetPrimitiveCountAction(const SoGetPrimitiveCountAction & rhs);
  SoGetPrimitiveCountAction & operator = (const SoGetPrimitiveCountAction & rhs);
}; // SoGetPrimitiveCountAction

#endif // !COIN_SOGETPRIMITIVECOUNTACTION_H
