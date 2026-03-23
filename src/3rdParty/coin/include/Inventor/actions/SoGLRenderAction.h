#ifndef COIN_SOGLRENDERACTION_H
#define COIN_SOGLRENDERACTION_H

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
#include <Inventor/SbBasic.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/system/inttypes.h>
#include <Inventor/lists/SoPathList.h>
#include <Inventor/lists/SbList.h>

typedef void SoGLRenderPassCB(void * userdata);
typedef void SoGLPreRenderCB(void * userdata, class SoGLRenderAction * action);
typedef float SoGLSortedObjectOrderCB(void * userdata, SoGLRenderAction * action);

class SoGLRenderActionP;

class COIN_DLL_API SoGLRenderAction : public SoAction {
  typedef SoAction inherited;

  SO_ACTION_HEADER(SoGLRenderAction);

public:
  static void initClass(void);

  SoGLRenderAction(const SbViewportRegion & viewportregion);
  virtual ~SoGLRenderAction(void);

  enum TransparencyType {
    SCREEN_DOOR,
    ADD, DELAYED_ADD, SORTED_OBJECT_ADD,
    BLEND, DELAYED_BLEND, SORTED_OBJECT_BLEND,
    // The remaining are Coin extensions to the common Inventor API
    SORTED_OBJECT_SORTED_TRIANGLE_ADD,
    SORTED_OBJECT_SORTED_TRIANGLE_BLEND,
    NONE, SORTED_LAYERS_BLEND
  };

  enum TransparentDelayedObjectRenderType {
    ONE_PASS,
    NONSOLID_SEPARATE_BACKFACE_PASS
  };

  enum AbortCode {
    CONTINUE, ABORT, PRUNE, DELAY
  };

  enum SortedObjectOrderStrategy {
    BBOX_CENTER,
    BBOX_CLOSEST_CORNER,
    BBOX_FARTHEST_CORNER,
    CUSTOM_CALLBACK
  };

  typedef AbortCode SoGLRenderAbortCB(void * userdata);

  void setViewportRegion(const SbViewportRegion & newregion);
  const SbViewportRegion & getViewportRegion(void) const;
  void setUpdateArea(const SbVec2f & origin, const SbVec2f & size);
  void getUpdateArea(SbVec2f & origin, SbVec2f & size) const;
  void setAbortCallback(SoGLRenderAbortCB * const func, void * const userdata);
  void getAbortCallback(SoGLRenderAbortCB * & func_out, void * & userdata_out) const;
  void setTransparencyType(const TransparencyType type);
  TransparencyType getTransparencyType(void) const;
  void setTransparentDelayedObjectRenderType(TransparentDelayedObjectRenderType type);
  TransparentDelayedObjectRenderType getTransparentDelayedObjectRenderType(void) const;
  void setSmoothing(const SbBool smooth);
  SbBool isSmoothing(void) const;
  void setNumPasses(const int num);
  int getNumPasses(void) const;
  void setPassUpdate(const SbBool flag);
  SbBool isPassUpdate(void) const;
  void setPassCallback(SoGLRenderPassCB * const func, void * const userdata);
  void setCacheContext(const uint32_t context);
  uint32_t getCacheContext(void) const;

  void addDelayedPath(SoPath * path);
  SbBool isRenderingDelayedPaths(void) const;

  SbBool handleTransparency(SbBool istransparent = FALSE);
  void setCurPass(const int passnum, const int numpasses);
  int getCurPass(void) const;
  SbBool abortNow(void);

  void setRenderingIsRemote(SbBool isremote);
  SbBool getRenderingIsRemote(void) const;

  virtual void invalidateState(void);

  void addPreRenderCallback(SoGLPreRenderCB * func, void * userdata);
  void removePreRenderCallback(SoGLPreRenderCB * func, void * userdata);
 
  void setSortedLayersNumPasses(int num);
  int getSortedLayersNumPasses(void) const;

  void setSortedObjectOrderStrategy(const SortedObjectOrderStrategy strategy,
                                    SoGLSortedObjectOrderCB * cb = NULL,
                                    void * closure = NULL);

  void setDelayedObjDepthWrite(SbBool write);
  SbBool getDelayedObjDepthWrite(void) const;

  SbBool isRenderingTranspPaths(void) const;
  SbBool isRenderingTranspBackfaces(void) const;

protected:
  friend class SoGLRenderActionP; // calls beginTraversal
  virtual void beginTraversal(SoNode * node);
  virtual void endTraversal(SoNode * node);

private:
  SbPimplPtr<SoGLRenderActionP> pimpl;

  SoGLRenderAction(const SoGLRenderAction & rhs);
  SoGLRenderAction & operator = (const SoGLRenderAction & rhs);

}; // SoGLRenderAction

#endif // !COIN_SOGLRENDERACTION_H
