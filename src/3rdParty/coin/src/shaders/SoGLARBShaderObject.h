#ifndef COIN_SOGLARBSHADEROBJECT_H
#define COIN_SOGLARBSHADEROBJECT_H

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

#include "SoGLShaderObject.h"

class SoGLShaderParameter;

// *************************************************************************

class SoGLARBShaderObject : public SoGLShaderObject
{
  friend class SoGLARBShaderParameter;
public:
  virtual SbBool isLoaded(void) const;
  virtual void load(const char *sourceString);
  virtual void unload(void);
  virtual SoShader::Type shaderType(void) const;
  virtual SoGLShaderParameter* getNewParameter(void) const;

  void enable(void);
  void disable(void);
public:
  SoGLARBShaderObject(const uint32_t cachecontext);
  virtual ~SoGLARBShaderObject();

private:
  GLuint arbProgramID;
  GLenum target; // either GL_FRAGMENT_PROGRAM_ARB or GL_VERTEX_PROGRAM_ARB
};

#endif /* ! COIN_SOGLARBSHADEROBJECT_H */
