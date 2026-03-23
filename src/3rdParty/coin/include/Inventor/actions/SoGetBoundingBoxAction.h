#ifndef COIN_SOGETBOUNDINGBOXACTION_H
#define COIN_SOGETBOUNDINGBOXACTION_H

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
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbXfBox3f.h>

class SoGetBoundingBoxActionP;

class COIN_DLL_API SoGetBoundingBoxAction : public SoAction {
  typedef SoAction inherited;

  SO_ACTION_HEADER(SoGetBoundingBoxAction);

public:
  static void initClass(void);

  SoGetBoundingBoxAction(const SbViewportRegion & vp);
  virtual ~SoGetBoundingBoxAction(void);

  enum ResetType { TRANSFORM = 0x1, BBOX = 0x2, ALL = TRANSFORM | BBOX };

  void setViewportRegion(const SbViewportRegion & newregion);
  const SbViewportRegion & getViewportRegion(void) const;

  SbBox3f getBoundingBox(void) const;
  SbXfBox3f & getXfBoundingBox(void);

  const SbVec3f & getCenter(void) const;

  void setInCameraSpace(const SbBool flag);
  SbBool isInCameraSpace(void) const;

  void setResetPath(const SoPath * path, const SbBool resetbefore = TRUE,
                    const ResetType what = ALL);
  const SoPath * getResetPath(void) const;
  SbBool isResetPath(void) const;
  SbBool isResetBefore(void) const;
  SoGetBoundingBoxAction::ResetType getWhatReset(void) const;


  void checkResetBefore(void);
  void checkResetAfter(void);

  void extendBy(const SbBox3f & box);
  void extendBy(const SbXfBox3f & box);

  void setCenter(const SbVec3f & center, const SbBool transformcenter);
  SbBool isCenterSet(void) const;
  void resetCenter(void);

protected:
  virtual void beginTraversal(SoNode * node);

private:
  enum { CENTER_SET = 0x1, CAMERA_SPACE = 0x2, RESET_BEFORE= 0x4 };

  SbXfBox3f bbox;
  SbVec3f center;
  SbViewportRegion vpregion;
  ResetType resettype;
  const SoPath * resetpath;
  unsigned int flags;

private:
  SbLazyPimplPtr<SoGetBoundingBoxActionP> pimpl;

  SoGetBoundingBoxAction(const SoGetBoundingBoxAction & rhs);
  SoGetBoundingBoxAction & operator = (const SoGetBoundingBoxAction & rhs);
}; // SoGetBoundingBoxAction

#endif // !COIN_SOGETBOUNDINGBOXACTION_H
