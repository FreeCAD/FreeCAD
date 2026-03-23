#ifndef COIN_SOGLSLSHADEROBJECT_H
#define COIN_SOGLSLSHADEROBJECT_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif

// *************************************************************************

#include "shaders/SoGLShaderObject.h"

#include "glue/glp.h"

class SbName;
class SoState;

// *************************************************************************

class SoGLSLShaderObject : public SoGLShaderObject
{
  friend class SoGLSLShaderParameter;
public:
  SoGLSLShaderObject(const uint32_t cachecontext);
  virtual ~SoGLSLShaderObject();

  virtual SoShader::Type shaderType(void) const;
  virtual SoGLShaderParameter* getNewParameter(void) const;

  virtual SbBool isLoaded(void) const;
  virtual void load(const char * sourceString);
  virtual void unload(void);

  void attach(COIN_GLhandle programHandle);
  void detach(void);
  SbBool isAttached(void) const;

  // source should be the name of the calling function
  static SbBool didOpenGLErrorOccur(const SbString & source);
  static void printInfoLog(const cc_glglue * g, COIN_GLhandle handle, int objType);

  virtual void updateCoinParameter(SoState * state, const SbName & name, SoShaderParameter * param, const int value);

private:
  COIN_GLhandle programHandle;
  COIN_GLhandle shaderHandle;
  SbBool isattached;
  int32_t programid;
};

#endif /* ! COIN_SOGLSLSHADEROBJECT_H */
