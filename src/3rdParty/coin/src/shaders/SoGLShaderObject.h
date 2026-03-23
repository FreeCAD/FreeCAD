#ifndef COIN_SOGLSHADEROBJECT_H
#define COIN_SOGLSHADEROBJECT_H

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

#include <Inventor/C/glue/gl.h>
#include <Inventor/SbString.h>
#include "SoShader.h"

class SoGLShaderParameter;
class SoShaderParameter;
class SoState;

// *************************************************************************

class SoGLShaderObject
{
public:
  SoGLShaderObject(const uint32_t cachecontext);
  virtual ~SoGLShaderObject() { }

  const cc_glglue * GLContext(void) const;
  uint32_t getCacheContext(void) const;

  virtual SbBool isLoaded(void) const = 0;
  virtual void load(const char * sourceString) = 0;
  virtual void unload(void) = 0;
  virtual SoShader::Type shaderType(void) const = 0;
  virtual SoGLShaderParameter* getNewParameter(void) const = 0;
  virtual void updateCoinParameter(SoState * state, const SbName & name,
                                   SoShaderParameter * param, const int val);

  uint32_t getShaderObjectId(void) const;

public:

  enum ShaderType {
    VERTEX,
    FRAGMENT,
    GEOMETRY
  };

  void setShaderType(const ShaderType type);
  ShaderType getShaderType(void) const;

  void setIsActive(SbBool flag);
  SbBool isActive(void) const;

  void setParametersDirty(SbBool flag);
  SbBool getParametersDirty(void) const;

#if defined(SOURCE_HINT)
  SbString sourceHint; // either the file name or the first line of source code
#endif

protected:
  const cc_glglue * glctx;
  uint32_t cachecontext;

private:
  ShaderType shadertype;
  SbBool isActiveFlag ;
  SbBool paramsdirty;
  uint32_t id;
};

#endif /* ! COIN_SOGLSHADEROBJECT_H */
