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

#ifndef COIN_SOVRMLELEVATIONGRID_H
#define COIN_SOVRMLELEVATIONGRID_H

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/VRMLnodes/SoVRMLGeometry.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoSFNode.h>

class SoChildList;

class SoVRMLElevationGridP;

class COIN_DLL_API SoVRMLElevationGrid : public SoVRMLGeometry
{
  typedef SoVRMLGeometry inherited;
  SO_NODE_HEADER(SoVRMLElevationGrid);

public:
  static void initClass(void);
  SoVRMLElevationGrid(void);

  SoSFBool ccw;
  SoSFBool solid;
  SoSFFloat creaseAngle;
  SoSFInt32 zDimension;
  SoSFInt32 xDimension;
  SoSFFloat zSpacing;
  SoSFFloat xSpacing;
  SoMFFloat height;
  SoSFNode texCoord;
  SoSFNode normal;
  SoSFNode color;
  SoSFBool colorPerVertex;
  SoSFBool normalPerVertex;

  virtual void GLRender(SoGLRenderAction * action);
  virtual void rayPick(SoRayPickAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);

protected:
  virtual ~SoVRMLElevationGrid(void);

  virtual void notify(SoNotList * list);
  virtual void generatePrimitives( SoAction * action );
  virtual void computeBBox(SoAction * action, SbBox3f & bbox,
                           SbVec3f & center);

private:
  friend class SoVRMLElevationGridP;

  enum Binding {
    OVERALL,
    PER_QUAD,
    PER_VERTEX
  };

  Binding findMaterialBinding(void) const;
  Binding findNormalBinding(void) const;

  const SbVec3f * updateNormalCache(Binding & nbind);

  SoVRMLElevationGridP * pimpl;

}; // class SoVRMLElevationGrid

#endif // ! COIN_SOVRMLELEVATIONGRID_H
