#ifndef COIN_SOSHADEROBJECT_H
#define COIN_SOSHADEROBJECT_H

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

#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoMFNode.h>
#include <Inventor/C/glue/gl.h>

class SoGLShaderObject;
class SoGLShaderProgram;
class SoState;

// *************************************************************************

class COIN_DLL_API SoShaderObject : public SoNode {
  typedef SoNode inherited;
  SO_NODE_ABSTRACT_HEADER(SoShaderObject);

public:
  enum SourceType {
    ARB_PROGRAM,
    CG_PROGRAM,
    GLSL_PROGRAM,
    FILENAME
  };

  SoSFBool isActive;
  SoSFEnum sourceType;
  SoSFString sourceProgram;
  // FIXME: this field is an SoMFUniformShaderParameter in TGS
  // Inventor. We should also implement that field. 20050125 mortene.
  SoMFNode parameter;

  static void initClass(void);

  virtual void GLRender(SoGLRenderAction * action);
  virtual void search(SoSearchAction * action);
  void render(SoState * state);

  void updateParameters(SoState * state);

  SourceType getSourceType(void) const;
  SbString getSourceProgram(void) const;

protected:
  SoShaderObject(void);
  virtual ~SoShaderObject();
  virtual SbBool readInstance(SoInput * in, unsigned short flags);

private:
  class SoShaderObjectP * pimpl;
};

#endif /* ! COIN_SOSHADEROBJECT_H*/
