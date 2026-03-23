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

#ifndef COIN_SOVRMLGEOMETRY_H
#define COIN_SOVRMLGEOMETRY_H

#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoSubNode.h>

class SoVRMLGeometryP;

class COIN_DLL_API SoVRMLGeometry : public SoShape
{
  typedef SoShape inherited;
  SO_NODE_ABSTRACT_HEADER(SoVRMLGeometry);

public:
  static void initClass(void);

  virtual void search(SoSearchAction * action);
  virtual void copyContents(const SoFieldContainer * from, SbBool copyConn);

protected:
  SoVRMLGeometry(void);
  virtual ~SoVRMLGeometry();

  void setupShapeHints(SoState * state, const SbBool ccw, const SbBool solid);
  virtual SbBool shouldGLRender(SoGLRenderAction * action);
  virtual SoChildList * getChildren(void) const;
  virtual void notify(SoNotList * list);

private:
  SoVRMLGeometryP * pimpl;
};

#endif // ! COIN_SOVRMLGEOMETRY_H
