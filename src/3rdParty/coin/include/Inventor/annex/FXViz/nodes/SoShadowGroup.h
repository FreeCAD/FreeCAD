#ifndef COIN_SOSHADOWGROUP_H
#define COIN_SOSHADOWGROUP_H

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

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFInt32.h>

class SoShadowGroupP;

class COIN_DLL_API SoShadowGroup : public SoSeparator {
  typedef SoSeparator inherited;

  SO_NODE_HEADER(SoShadowGroup);

public:
  static void initClass(void);
  static void init(void);

  static SbBool isSupported(void);

  SoShadowGroup(void);

  enum VisibilityFlag {
    ABSOLUTE_RADIUS,
    LONGEST_BBOX_EDGE_FACTOR,
    PROJECTED_BBOX_DEPTH_FACTOR
  };

  SoSFBool isActive;
  SoSFFloat intensity;
  SoSFFloat precision;
  SoSFFloat quality;
  SoSFFloat smoothBorder;
  SoSFBool shadowCachingEnabled;
  SoSFFloat visibilityNearRadius;
  SoSFFloat visibilityRadius;
  SoSFEnum visibilityFlag;
  
  SoSFFloat epsilon;
  SoSFFloat threshold;

  virtual void GLRenderBelowPath(SoGLRenderAction * action);
  virtual void GLRenderInPath(SoGLRenderAction * action);

  virtual void notify(SoNotList * nl);

  void enableSubgraphSearchOnNotify(const SbBool onoff);

protected:
  virtual ~SoShadowGroup();

private:
  SoShadowGroupP * pimpl;

};

#endif // COIN_SOSHADOWGROUP_H
